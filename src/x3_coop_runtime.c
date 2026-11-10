#include "x3_coop_runtime.h"
#include "common_rtl.h"
#include "snes/cart.h"
#include "snes/snes.h"
#include "snes/interp_bridge.h"
#include "snes/saveload.h"
#include <stdio.h>
#include <string.h>

extern uint8_t g_ram[0x20000];
extern Snes *g_snes;
static bool enabled;
static X3CoopState state;
typedef struct Registers {uint16_t a,x,y,s,d;uint8_t db,p;} Registers;
typedef struct NativePass {Registers entry,result;uint8_t pass;} NativePass;
static NativePass controller,terrain;
typedef struct ContactPass {
  NativePass native;
  uint16_t pc;
  uint8_t seat;
  uint8_t entry_scratch[24],result_scratch[24];
} ContactPass;
static ContactPass contact;
typedef struct ObjectPass {NativePass native;uint16_t pc,end;} ObjectPass;
static ObjectPass objects;
/* Late native draw requests outlive the actor update that queued them. */
static NativePass draw;
/* The world scheduler calls each native hostile once. Borrow its target only
 * for that call; canonical camera/scene ownership stays at state.anchor. */
typedef struct WorldTarget {uint16_t slot,s;uint8_t active,seat;} WorldTarget;
static WorldTarget target;
static unsigned object_end(unsigned pc) {
  switch(pc) {
    case 0xda7f:return 0xda9f; /* Three native armor objects. */
    case 0xdb9f:return 0xdbc0; /* Nine player projectile slots. */
    case 0xdbc1:return 0xdbee; /* Frozen projectile update. */
    case 0xdc06:return 0xdc22; /* Charge and small owned effects. */
    case 0xdc23:return 0xdc4b; /* Frozen owned-effect update. */
    default:return 0;
  }
}
static Registers regs(CpuState *c) {
  cpu_mirrors_to_p(c);
  return (Registers){c->A,c->X,c->Y,c->S,c->D,c->DB,c->P};
}
static void restore(CpuState *c,const Registers *r) {
  c->A=r->a;c->X=r->x;c->Y=r->y;c->D=r->d;c->DB=r->db;
  c->P=r->p;cpu_p_to_mirrors(c);
  /* Guest elapsed time and return-frame ownership are never rewound. */
}
static unsigned word(const uint8_t *p) {return p[0]|p[1]<<8;}
static void putword(uint8_t *p,unsigned v) {p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void input(unsigned seat) {
  uint8_t *body=g_ram+X3_COOP_BODY;
  /* NMI has already remapped the native body's actions. Reading that word
   * as its own previous value erases P1's fresh jump/fire edges. */
  X3CoopPlayer *player=&state.players[seat];
  unsigned previous=player->previous_actions;
  unsigned held=X3CoopActions(RtlGetPadState(seat),g_ram+0xffc0);
  putword(body+0x36,held);putword(body+0x38,previous);putword(body+0x3a,held&~previous);
  player->previous_actions=(uint16_t)held;
  putword(body+0x22,word(body+5));putword(body+0x24,word(body+8));
  g_ram[0x1f1e]=0;
}
static void anchor_palette(void) {
  memcpy(g_ram+X3_COOP_PALETTE,state.players[state.anchor].palette,X3_COOP_PALETTE_BYTES);
  g_ram[0xa1]=1;
}
static void settle_deaths(void) {
  if(!enabled || !state.initialized || state.wipe)return;
  /* $00:9C60 checks native HP after the world/contact pass, before another
   * player-controller entry can hand off. Project a living survivor before
   * that LDA, so the original stage task cannot queue a team fade for one
   * actor's fatal enemy contact. Do not overwrite the native HP result. */
  X3CoopCapture(&state,g_ram);
  for(unsigned seat=0;seat<2;++seat) {
    const X3CoopPlayer *p=&state.players[seat];
    if(p->life==X3_COOP_ALIVE && (!(p->body[0x27]&127) || p->body[2]==0x0c))
      X3CoopFall(&state,seat);
  }
  if(!state.wipe && (state.players[state.current].life!=X3_COOP_ALIVE ||
                    (!target.active && state.current!=state.anchor))) {
    X3CoopProject(&state,g_ram,state.anchor);
    anchor_palette();
  }
  if(state.wipe)g_ram[X3_COOP_BODY+0xb6]=0;
}
static void team_death_decision(CpuState *cpu,uint32_t pc) {
  (void)cpu;(void)pc;
  if(controller.pass || terrain.pass || contact.native.pass || objects.native.pass ||
     draw.pass || target.active)return;
  settle_deaths();
}
static bool partner_ready(void) {
  return enabled && state.initialized && !state.wipe &&
      state.players[state.anchor].life==X3_COOP_ALIVE &&
      state.players[state.anchor^1].life==X3_COOP_ALIVE;
}
static bool hostile_projectile(unsigned d) {
  return d>=0x1318 && d<0x1518 && !((d-0x1318)%64);
}
static bool enemy_actor(unsigned d) {
  return (d>=0x0d18 && d<0x10d8 && !((d-0x0d18)%64)) || hostile_projectile(d);
}
static void target_boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized)return;
  unsigned at=pc&0xffff,d=cpu->D;
  if(at==0xdd1a || at==0xdcb3) {
    if(state.wipe || target.active || !enemy_actor(d) || !g_ram[d] || !(g_ram[d+0x27]&127) ||
       g_ram[0x1f22] || g_ram[0x1f24] || controller.pass || terrain.pass ||
       contact.native.pass || objects.native.pass || draw.pass || state.current!=state.anchor)return;
    unsigned seat=X3CoopNearest(&state,g_ram,word(g_ram+d+5),word(g_ram+d+8));
    if(seat==state.anchor)return;
    target=(WorldTarget){(uint16_t)d,cpu->S,1,(uint8_t)seat};
    X3CoopProject(&state,g_ram,seat);
    return;
  }
  /* These four loop continuations follow the actual RTS (normal/frozen,
   * enemy/hostile shot), so the original JSR frame has been consumed. */
  if(target.active && d==target.slot && cpu->S==(uint16_t)(target.s+2)) {
    target.active=0;
    settle_deaths();
    if(!state.wipe)X3CoopProject(&state,g_ram,state.anchor);
  }
}
static void target_query(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || state.wipe || !enemy_actor(cpu->D) || !g_ram[cpu->D])return;
  /* The native common facing/proximity helpers also run during enemy
   * construction, before HP is initialized. Replace just their coordinate
   * load; keep original range arithmetic, flags, branch and AI cadence. */
  unsigned seat=target.active?target.seat:X3CoopNearest(&state,g_ram,
      word(g_ram+cpu->D+5),word(g_ram+cpu->D+8));
  if(state.players[seat].life!=X3_COOP_ALIVE)return;
  const uint8_t *body=seat==state.current?g_ram+X3_COOP_BODY:state.players[seat].body;
  unsigned axis=(pc&65535)==0xdac6?8:5;
  cpu->A=(uint16_t)word(body+axis);
  cpu->_flag_Z=cpu->A==0;cpu->_flag_N=(cpu->A&32768)!=0;cpu_mirrors_to_p(cpu);
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|((pc+3)&65535));
}
static bool owned_object(unsigned d) {
  return (d>=X3_COOP_AUX && d<X3_COOP_AUX+X3_COOP_AUX_BYTES && !((d-X3_COOP_AUX)%32)) ||
      (d>=X3_COOP_SHOTS && d<X3_COOP_SHOTS+X3_COOP_SHOT_BYTES && !((d-X3_COOP_SHOTS)%64));
}
static void draw_boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized)return;
  pc&=0x7fffff;
  if((pc>>16)==2) {
    /* TDC has produced the real slot pointer. Bit15 is unused by the native
     * WRAM pools, and travels with this one request through all six queues. */
    if(owned_object(cpu->D))cpu->A=(uint16_t)(cpu->D|(state.current<<15));
    return;
  }
  if(pc==0x038f70) {
    unsigned address=(cpu->D+cpu->X)&65535,slot=address&32767,seat=address>>15;
    if(!owned_object(slot))return;
    cpu->X&=32767;
    if(state.wipe || state.players[seat].life!=X3_COOP_ALIVE) {
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0x8fe1);return;
    }
    if(draw.pass || controller.pass || terrain.pass || objects.native.pass ||
       contact.native.pass || state.current!=state.anchor) {enabled=false;return;}
    draw.entry=regs(cpu);draw.pass=(uint8_t)(seat==state.anchor?1:2);
    X3CoopProject(&state,g_ram,seat);
  } else if(pc==0x038fe1 && draw.pass && cpu->S==draw.entry.s && cpu->D==draw.entry.d) {
    X3CoopProject(&state,g_ram,state.anchor);draw.pass=0;
  }
}
static void contact_boundary(CpuState *cpu,uint32_t pc) {
  unsigned at=pc&0xffff,d=cpu->D;
  NativePass *p=&contact.native;
  if(!enabled || !state.initialized || state.wipe)return;
  if(at==0xcb31 || at==0xcb74) {
    /* Ordinary hostile actors only. Pickups, capsules, supports and scripts
     * retain their native owner until their specific contracts are mapped. */
    if(!enemy_actor(d) || !g_ram[d] || !partner_ready())return;
    if(!p->pass && (state.current==state.anchor || target.active) && !controller.pass &&
       !terrain.pass && !objects.native.pass && !draw.pass) {
      p->entry=regs(cpu);contact.pc=(uint16_t)at;contact.seat=state.current;p->pass=1;
      memcpy(contact.entry_scratch,g_ram,24);
    }
    return;
  }
  /* Shared terminal instructions also serve nested helpers. Match the real
   * caller's balanced return frame and actor before closing a contact pass. */
  if(!p->pass || cpu->S!=p->entry.s || cpu->D!=p->entry.d)return;
  if(p->pass==1) {
    bool hit=(cpu->A&255)!=0;
    /* A hostile shot is consumed on its first body hit. A player-shot scan
     * is consumed by hit, shield or reflection; only CBC3 is a true miss. */
    if((contact.pc==0xcb31 && hostile_projectile(d) && hit) ||
       (contact.pc==0xcb74 && at!=0xcbc3)) {p->pass=0;settle_deaths();return;}
    p->result=regs(cpu);memcpy(contact.result_scratch,g_ram,24);
    if(!X3CoopProject(&state,g_ram,contact.seat^1))return;
    restore(cpu,&p->entry);memcpy(g_ram,contact.entry_scratch,24);p->pass=2;
    interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|contact.pc);
    return;
  }
  bool first_hit=(p->result.a&255)!=0;
  bool second_missed=contact.pc==0xcb74?at==0xcbc3:!(cpu->A&255);
  X3CoopProject(&state,g_ram,contact.seat);
  if(first_hit || second_missed) {
    restore(cpu,&p->result);memcpy(g_ram,contact.result_scratch,24);
  }
  p->pass=0;settle_deaths();
}
static void object_boundary(CpuState *cpu,uint32_t pc) {
  unsigned at=pc&0xffff,end=object_end(at);
  NativePass *p=&objects.native;
  if(!enabled || !state.initialized || state.wipe)return;
  if(end) {
    if(!p->pass && partner_ready() && state.current==state.anchor &&
       !controller.pass && !terrain.pass && !contact.native.pass && !draw.pass) {
      p->entry=regs(cpu);p->pass=1;objects.pc=(uint16_t)at;objects.end=(uint16_t)end;
    }
    return;
  }
  if(!p->pass || at!=objects.end || cpu->S!=p->entry.s)return;
  if(p->pass==1) {
    p->result=regs(cpu);
    if(!X3CoopProject(&state,g_ram,state.anchor^1))return;
    restore(cpu,&p->entry);p->pass=2;
    interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|objects.pc);
  } else {
    X3CoopProject(&state,g_ram,state.anchor);restore(cpu,&p->result);p->pass=0;
  }
}
static void boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || cpu->D!=X3_COOP_BODY || cpu->emulation)return;
  pc&=0x7fffff;
  if(pc==0x04823f && state.initialized && controller.pass &&
     !state.wipe && g_ram[X3_COOP_BODY+2]==0x0c) {
    /* Fatal contact can select death inside this very controller pass. Skip
     * its state call before $04:9258 freezes every shared object pool. The
     * balanced controller epilogue records the corpse; the next entry hands
     * the world to the survivor, or runs native death for a complete wipe. */
    interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|0x8244);
    return;
  }
  if(pc==0x04b591 || pc==0x04b5ab) {
    if(!state.initialized)return;
    if(pc==0x04b591 && !terrain.pass) {
      terrain.entry=regs(cpu);terrain.pass=1;
    } else if(pc==0x04b5ab && terrain.pass) {
      if(cpu->S!=terrain.entry.s){enabled=false;return;}
      if(terrain.pass==1 && state.players[state.anchor^1].life==X3_COOP_ALIVE) {
        terrain.result=regs(cpu);terrain.pass=2;
        X3CoopProject(&state,g_ram,state.anchor^1);
        restore(cpu,&terrain.entry);
        interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|0xb591);
      } else {
        if(terrain.pass==2) {
          X3CoopProject(&state,g_ram,state.anchor);
          restore(cpu,&terrain.result);
        }
        terrain.pass=0;
      }
    }
    return;
  }
  if(pc==0x04809e && controller.pass==2) {
    /* The second body has its own presentation, so its dynamic CHR must
     * never replace X's native body pages in the shared DMA queue. */
    interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|0x80a2);
    return;
  }
  if(pc==0x048008) {
    if(state.initialized && state.wipe) {
      /* The native retry reinitializes the body. Do not enroll another actor
       * while the common death routine is still running. */
      if(g_ram[X3_COOP_BODY+1]==0)X3CoopRuntimeReset();
      else return;
    }
    /* Safe initial physics qualification: active, living native X only.
     * Scripted intro/formation and native exchange are not join points. */
    if(!state.initialized) {
      unsigned caller=word(g_ram+cpu->S+4);
      if(caller!=0xd98e || g_ram[0x1f22] ||
         g_ram[X3_COOP_BODY+1]!=2 || g_ram[X3_COOP_BODY+2]>0x16 ||
         !g_ram[X3_COOP_BODY+0x27] || g_ram[X3_COOP_BODY+0xb6])return;
      X3CoopInit(&state,g_ram,0);
      X3CoopPlayer *p=&state.players[1];
      p->body[0xb6]=2; /* Native playable Zero selector, not an imported actor. */
      p->body[0x16]=0x4a;
      const uint8_t *rom=g_snes->cart->rom;
      putword(p->body+0x31,word(rom+0x3326a)); /* Data bank $86:B26A. */
      p->body[0x17]|=0x80;
      p->body[0x57]=p->body[0x58]=p->body[0x5a]=p->body[0x5b]=0;
      p->body[0xb7]=0;
      /* Native D0/D2 lists select Zero's body and buster/charge ramps.
       * Keep them private; both actors use the same original palette slots. */
      if(g_snes->cart->romImageSize>=0x635c0) {
        memcpy(p->palette,g_snes->cart->rom+0x635a0,32);
        memcpy(p->palette+32,g_snes->cart->rom+0x63580,32);
      }
      fprintf(stderr,"[x3-coop] native actor qualification enrolled X and Zero\n");
    }
    if(controller.pass)return;
    if(g_ram[X3_COOP_BODY+2]==0x0c) {
      X3CoopCapture(&state,g_ram);
      X3CoopFall(&state,state.current);
      if(state.wipe) {
        /* Whole-team retry uses X's native death path. Zero's stock permanent
         * departure must not remove the team retry in full-campaign mode. */
        g_ram[X3_COOP_BODY+0xb6]=0;
        return;
      }
      X3CoopProject(&state,g_ram,state.anchor);
      anchor_palette();
      if(g_ram[X3_COOP_BODY+2]==0x0c) {
        X3CoopCapture(&state,g_ram);
        X3CoopFall(&state,state.current);
        g_ram[X3_COOP_BODY+0xb6]=0;
        return;
      }
    }
    controller.entry=regs(cpu);controller.pass=1;
    input(state.anchor);
  } else if(pc==0x0480bc && controller.pass) {
    if(cpu->S!=controller.entry.s) {
      fprintf(stderr,"[x3-coop] unbalanced native controller return\n");
      enabled=false;return;
    }
    if(controller.pass==1) {
      controller.result=regs(cpu);
      unsigned partner=state.anchor^1;
      if(state.players[partner].life!=X3_COOP_ALIVE) {
        X3CoopCapture(&state,g_ram);controller.pass=0;return;
      }
      if(state.players[partner].body[2]==0x0c) {
        X3CoopFall(&state,partner);
        X3CoopCapture(&state,g_ram);controller.pass=0;return;
      }
      X3CoopProject(&state,g_ram,state.anchor^1);
      restore(cpu,&controller.entry);controller.pass=2;
      input(state.current);
      interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|0x8008);
    } else {
      X3CoopProject(&state,g_ram,state.anchor);
      restore(cpu,&controller.result);controller.pass=0;
    }
  }
}
void X3CoopRuntimeEnable(bool value) {enabled=value;}
bool X3CoopRuntimeEnabled(void) {return enabled;}
const X3CoopState *X3CoopRuntimeState(void) {return &state;}
void X3CoopRuntimeReset(void) {
  memset(&state,0,sizeof(state));memset(&controller,0,sizeof(controller));
  memset(&terrain,0,sizeof(terrain));
  memset(&contact,0,sizeof(contact));memset(&objects,0,sizeof(objects));
  memset(&draw,0,sizeof(draw));
  memset(&target,0,sizeof(target));
}
void X3CoopRuntimeFrame(void) {
  if(!enabled)return;
  if(!interp_bridge_add_pre_opcode_hook(0x048008,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x0480bc,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04809e,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04823f,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04b591,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04b5ab,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x009c60,team_death_decision)) {
    fprintf(stderr,"[x3-coop] controller hook capacity exhausted\n");enabled=false;
  }
  static const uint32_t contact_pcs[]={0x04cb31,0x04cb74,0x04cb73,0x04cbc3,
      0x04cd69,0x04ce3b,0x04ced8,0x04cedb,0x04cfe3};
  static const uint32_t object_pcs[]={0x00da7f,0x00da9f,0x00db9f,0x00dbc0,
      0x00dbc1,0x00dbee,0x00dc06,0x00dc22,0x00dc23,0x00dc4b};
  for(unsigned i=0;i<sizeof(contact_pcs)/sizeof(*contact_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(contact_pcs[i],contact_boundary))enabled=false;
  for(unsigned i=0;i<sizeof(object_pcs)/sizeof(*object_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(object_pcs[i],object_boundary))enabled=false;
  static const uint32_t draw_pcs[]={0x02d686,0x02d69b,0x02d6b0,0x02d6c5,
      0x02d6da,0x02d6ef,0x038f70,0x038fe1};
  for(unsigned i=0;i<sizeof(draw_pcs)/sizeof(*draw_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(draw_pcs[i],draw_boundary))enabled=false;
  static const uint32_t target_pcs[]={0x00dd1a,0x00dcb3,0x00dccf,0x00dcf3,0x00dc68,0x00dc8c};
  for(unsigned i=0;i<sizeof(target_pcs)/sizeof(*target_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(target_pcs[i],target_boundary))enabled=false;
  static const uint32_t target_queries[]={0x02e142,0x02daa7,0x02dac6};
  for(unsigned i=0;i<sizeof(target_queries)/sizeof(*target_queries);++i)
    if(!interp_bridge_add_pre_opcode_hook(target_queries[i],target_query))enabled=false;
}
void X3CoopRuntimeSave(SaveLoadInfo *sli) {
  uint8_t bytes[4096]={0};
  X3CoopCapture(&state,g_ram);
  X3CoopEncode(&state,bytes,X3CoopEncodedSize());
  sli->func(sli,bytes,X3CoopEncodedSize());
  sli->func(sli,&controller,sizeof(controller));
  sli->func(sli,&terrain,sizeof(terrain));
  sli->func(sli,&contact,sizeof(contact));
  sli->func(sli,&objects,sizeof(objects));
  sli->func(sli,&draw,sizeof(draw));
  sli->func(sli,&target,sizeof(target));
}
static bool passes_valid(const X3CoopState *s,const NativePass *c,const NativePass *t,
                         const ContactPass *hit,const ObjectPass *obj,const NativePass *art,
                         const WorldTarget *world) {
  unsigned a=c->pass,b=t->pass,h=hit->native.pass,o=obj->native.pass,v=art->pass;
  if(a>2 || b>2 || h>2 || o>2 || v>2 || (a!=0)+(b!=0)+(h!=0)+(o!=0)+(v!=0)>1)return false;
  if(world->active>1 || world->seat>1)return false;
  if(!s->initialized)return !(a || b || h || o || v || world->active);
  if(world->active && (!enemy_actor(world->slot) || a || b || o || v))return false;
  const NativePass *active=a?c:b?t:h?&hit->native:o?&obj->native:v?art:NULL;
  unsigned base=h?hit->seat:s->anchor;
  if(active && (base>1 || s->wipe || s->current!=(base^(active->pass==2)) ||
     s->players[s->current].life!=X3_COOP_ALIVE))return false;
  if(world->active) {
    unsigned seat=s->players[world->seat].life==X3_COOP_ALIVE?world->seat:s->anchor;
    if(h?hit->seat!=world->seat:!s->wipe && s->current!=seat)return false;
  }
  if((a && c->entry.d!=X3_COOP_BODY) || (b && t->entry.d!=X3_COOP_BODY))return false;
  if(h && ((hit->pc!=0xcb31 && hit->pc!=0xcb74) || !enemy_actor(hit->native.entry.d)))return false;
  if(o && (!object_end(obj->pc) || object_end(obj->pc)!=obj->end))return false;
  if(v && (art->entry.d || !owned_object(art->entry.x)))return false;
  return true;
}
bool X3CoopRuntimeLoad(SaveLoadInfo *sli) {
  uint8_t bytes[4096];X3CoopState loaded;
  NativePass c,t,art;ContactPass hit;ObjectPass obj;WorldTarget world;
  sli->func(sli,bytes,X3CoopEncodedSize());
  sli->func(sli,&c,sizeof(c));sli->func(sli,&t,sizeof(t));
  sli->func(sli,&hit,sizeof(hit));sli->func(sli,&obj,sizeof(obj));
  sli->func(sli,&art,sizeof(art));
  sli->func(sli,&world,sizeof(world));
  if(!X3CoopDecode(&loaded,bytes,X3CoopEncodedSize()) || !passes_valid(&loaded,&c,&t,&hit,&obj,&art,&world))return false;
  state=loaded;controller=c;terrain=t;contact=hit;objects=obj;draw=art;target=world;return true;
}
bool X3CoopRuntimePreflight(const void *data,size_t size) {
  size_t payload=X3CoopEncodedSize(),tail=payload+sizeof(controller)+sizeof(terrain)+sizeof(contact)+sizeof(objects)+sizeof(draw)+sizeof(target);
  if(!data || size<tail)return false;
  X3CoopState decoded;
  const uint8_t *p=(const uint8_t *)data+size-tail;
  if(!X3CoopDecode(&decoded,p,payload))return false;
  /* Validate before the common loader changes WRAM, CPU or either pass. */
  NativePass c,t,art;ContactPass hit;ObjectPass obj;WorldTarget world;p+=payload;
  memcpy(&c,p,sizeof(c));p+=sizeof(c);memcpy(&t,p,sizeof(t));p+=sizeof(t);
  memcpy(&hit,p,sizeof(hit));p+=sizeof(hit);memcpy(&obj,p,sizeof(obj));p+=sizeof(obj);
  memcpy(&art,p,sizeof(art));p+=sizeof(art);memcpy(&world,p,sizeof(world));
  return passes_valid(&decoded,&c,&t,&hit,&obj,&art,&world);
}
