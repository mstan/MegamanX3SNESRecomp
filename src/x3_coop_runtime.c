#include "x3_coop_runtime.h"
#include "x3_coop_view.h"
#include "x3_coop_present.h"
#include "common_rtl.h"
#include "snes/cart.h"
#include "snes/snes.h"
#include "snes/ppu.h"
#include "snes/interp_bridge.h"
#include "snes/saveload.h"
#include <stdio.h>
#include <string.h>

extern uint8_t g_ram[0x20000];
extern Snes *g_snes;
static bool enabled;
static unsigned p1_character;
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
/* Native world actors update once. Borrow a hostile's target or a pickup's
 * recipient only for that call; canonical ownership stays at state.anchor. */
typedef struct WorldTarget {uint16_t slot,s;uint8_t active,seat;} WorldTarget;
static WorldTarget target;
/* A native health refill remains in the shared item pool while all world
 * pools are frozen. Its original recipient outlives an individual item call. */
typedef struct PickupRefill {uint16_t slot;uint8_t seat,active;} PickupRefill;
static PickupRefill refill;
enum { SHARED_SUPPORTS=31 };
static uint8_t riders[SHARED_SUPPORTS];
typedef struct SolidPass {
  NativePass native;
  uint16_t pc;
  uint8_t seat,restore,repeat;
  uint8_t entry_scratch[24],result_scratch[24];
} SolidPass;
static SolidPass solid;
/* The native column loop remains interruptible and owns all allocation.
 * Only its pending column selection is extra simulation state. */
typedef struct ViewSpawn {uint8_t columns[64],count,cursor,active;} ViewSpawn;
static ViewSpawn view_spawn;
typedef struct ViewBounds {NativePass native;uint8_t camera[64],scratch[24];} ViewBounds;
static ViewBounds view_bounds;
static bool partner_ready(void);
static void settle_deaths(void);
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
static bool view_scene(void) {return g_ram[0x1f22] || g_ram[0x1f24] || g_ram[0x1f63];}
static bool view_ready(void) {
  return enabled && state.initialized && !state.wipe && X3CoopViewsIndependent() &&
      !view_scene() && g_ram[0xd1]==4 && g_ram[0xd2]==4;
}
static bool view_record_wanted(unsigned flag) {
  if(flag<0xfa02 || flag>0xfffb)return false;
  unsigned record=word(g_ram+flag+3);
  size_t offset=0x1e0000+(record&32767);
  if(record<32768 || offset+7>g_snes->cart->romImageSize)return false;
  const uint8_t *r=g_snes->cart->rom+offset;
  unsigned kind=r[0]&15,x=word(r+5)&8191,y=word(r+1)&8191;
  if(kind>3)return false;
  /* Resident parallax controllers write the shared native BG registers.
   * Keep their retail lifetime until private presentation owns those writes. */
  if(kind==1 && r[3]==4) {
    int dx=(int16_t)((x&~31u)-word(g_ram+0x1e5d));
    int dy=(int16_t)(y-word(g_ram+0x1e60));
    return dx>=-31 && dx<257 && dy>=-32 && dy<256;
  }
  /* A leading native column includes records up to31px beyond its origin.
   * This frontier stays within even the narrow D636 retirement rectangle.
   * Admitting at D58A's wider retention margin can consume an event before
   * its first D636 draw, leaving a held visit for an already-retired actor. */
  return X3CoopViewsContains(g_ram,&state,x&~31u,y,31,257,32,256,0,false);
}
static void view_spawn_begin(void) {
  memset(&view_spawn,0,sizeof(view_spawn));
  for(unsigned col=0;col<256;++col) {
    unsigned first=word(g_ram+0xf800+col*2),end=word(g_ram+0xf802+col*2);
    if(first<0xfa02 || end>0xfffc || end<first || (end-first)%5)continue;
    bool candidate=false;
    for(unsigned flag=first;flag<end;flag+=5) {
      if(!view_record_wanted(flag))X3CoopViewsMark(flag,false);
      else if(g_ram[flag])X3CoopViewsMark(flag,true);
      else if(!X3CoopViewsSeen(flag))candidate=true;
    }
    if(candidate && view_spawn.count<sizeof(view_spawn.columns))
      view_spawn.columns[view_spawn.count++]=(uint8_t)col;
  }
  view_spawn.active=view_spawn.count!=0;
}
static void view_world_boundary(CpuState *cpu,uint32_t pc) {
  unsigned at=pc&0x7fffff;
  if(at==0x00da38) {
    if(enabled && state.initialized && X3CoopViewsIndependent()) {
      X3CoopCapture(&state,g_ram);
      X3CoopViewsEndFrame(g_ram,&state,view_scene(),false);
    }
    return;
  }
  /* The native refill owns a shared freeze until its cleanup runs. Removing
   * its recipient's view must not cull the item before that cleanup. */
  bool retain_refill=enabled && state.initialized && refill.active &&
      refill.slot==cpu->D && at==0x02d58a;
  if(!view_ready() && !retain_refill)return;
  if(at==0x00de43) {
    X3CoopCapture(&state,g_ram);
    X3CoopViewsUpdate(g_ram,&state,false,false);view_spawn_begin();
    if(!view_spawn.active) {interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xdecf);return;}
    putword(g_ram,view_spawn.columns[0]*32);putword(g_ram+2,0);putword(g_ram+4,0x1fff);
    putword(g_ram+6,view_spawn.count);
    interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xdec0);return;
  }
  if(at==0x00dec3 && view_spawn.active) {
    if(++view_spawn.cursor<view_spawn.count)
      putword(g_ram,view_spawn.columns[view_spawn.cursor]*32-32);
    return;
  }
  if(at==0x00decf) {memset(&view_spawn,0,sizeof(view_spawn));return;}
  if(at==0x00df1e) {
    unsigned flag=word(g_ram+0x14);
    if(!view_record_wanted(flag) || X3CoopViewsSeen(flag))
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xdf82);
    return;
  }
  if(at==0x00df4c) {
    unsigned flag=word(g_ram+0x14);
    if(flag>=0xfa02 && flag<=0xfffb && g_ram[flag])X3CoopViewsMark(flag,true);
    return;
  }
  unsigned d=cpu->D;
  if(d>0x1fe0)return;
  int left,right,top,bottom;unsigned next;
  switch(at) {
    case 0x02d58a:left=top=64;right=320;bottom=288;next=0xd5a8;break;
    case 0x02d5ab:
      left=cpu->A&255;right=256+left;top=64;bottom=288;next=0xd5a8;
      putword(g_ram,left);putword(g_ram+2,left*2+256);break;
    case 0x02d5da:
      left=top=cpu->A&255;right=bottom=256+left;next=0xd631;
      putword(g_ram,left);putword(g_ram+2,left*2+224);putword(g_ram+4,left*2+256);break;
    case 0x02d611:left=96;right=352;top=80;bottom=304;next=0xd631;break;
    case 0x02d636:left=top=32;right=288;bottom=256;next=0xd656;break;
    case 0x00df9f:left=32;right=288;top=0;bottom=242;next=0xdfbc;break;
    default:return;
  }
  int x=word(g_ram+d+5),y=word(g_ram+d+8);
  X3CoopView chosen=X3CoopViewsForPlayer(g_ram,&state,state.anchor,false);
  bool owned=(d>=X3_COOP_AUX && d<X3_COOP_AUX+X3_COOP_AUX_BYTES) ||
      (d>=X3_COOP_SHOTS && d<X3_COOP_SHOTS+X3_COOP_SHOT_BYTES);
  bool inside=false;
  for(unsigned seat=0;seat<2;++seat) {
    if(state.players[seat].life!=X3_COOP_ALIVE || (owned && seat!=state.current))continue;
    X3CoopView v=X3CoopViewsForPlayer(g_ram,&state,seat,false);
    int dx=(int16_t)(x-v.x),dy=(int16_t)(y-v.y);
    if(dx>=-left && dx<right && dy>=-top && dy<bottom){inside=true;chosen=v;break;}
  }
  if(retain_refill){inside=true;chosen.x=x;chosen.y=y;}
  unsigned value=(uint16_t)(x-chosen.x+left),limit=(unsigned)(left+right);
  if(value<limit){value=(uint16_t)(y-chosen.y+top);limit=(unsigned)(top+bottom);}
  cpu->A=(uint16_t)value;cpu_mirrors_to_p(cpu);cpu->P&=(uint8_t)~0x20;cpu_p_to_mirrors(cpu);
  cpu->_flag_C=!inside;cpu->_flag_Z=value==limit;cpu->_flag_N=((value-limit)&32768)!=0;
  cpu_mirrors_to_p(cpu);
  if(at==0x00df9f){next=inside?0xdfc1:0xdfbc;}
  else if(at!=0x02d58a && at!=0x02d5ab && inside)next=0xd65b;
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|next);
}
static void view_bounds_boundary(CpuState *cpu,uint32_t pc) {
  if(!view_ready() || cpu->D!=0x1e58)return;
  if((pc&65535)==0xe2a2 && !view_bounds.native.pass) {
    if(state.current!=state.anchor || !partner_ready())return;
    view_bounds.native.entry=regs(cpu);view_bounds.native.pass=1;return;
  }
  if((pc&65535)!=0xe316 || !view_bounds.native.pass)return;
  if(cpu->S!=view_bounds.native.entry.s){enabled=false;return;}
  if(view_bounds.native.pass==1) {
    view_bounds.native.result=regs(cpu);
    memcpy(view_bounds.camera,g_ram+0x1e58,64);memcpy(view_bounds.scratch,g_ram,24);
    unsigned partner=state.anchor^1;
    X3CoopCapture(&state,g_ram);
    X3CoopViewsUpdate(g_ram,&state,false,false);
    X3CoopView v=X3CoopViewsFollowPlayer(g_ram,&state,partner);
    const X3CoopViewState *views=X3CoopViewsState();
    X3CoopProject(&state,g_ram,partner);
    for(unsigned i=0;i<4;++i)putword(g_ram+0x1e66+i*2,
        views->camera_valid&(1u<<partner)?views->current[partner][i]:views->limits[partner][i]);
    putword(g_ram+0x1e5d,(unsigned)v.x);putword(g_ram+0x1e60,(unsigned)v.y);
    restore(cpu,&view_bounds.native.entry);view_bounds.native.pass=2;
    interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xe2a2);
  } else {
    X3CoopProject(&state,g_ram,state.anchor);
    memcpy(g_ram+0x1e58,view_bounds.camera,64);memcpy(g_ram,view_bounds.scratch,24);
    restore(cpu,&view_bounds.native.result);view_bounds.native.pass=0;
    settle_deaths();
  }
}
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
static bool fall_player(unsigned seat) {
  if(seat==state.anchor && state.players[seat^1].life==X3_COOP_ALIVE)
    X3CoopViewsAdoptPlayer(g_ram,&state,seat^1,view_scene());
  return X3CoopFall(&state,seat);
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
      fall_player(seat);
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
     draw.pass || target.active || solid.native.pass || view_bounds.native.pass)return;
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
static bool item_actor(unsigned d){return d>=0x1518 && d<0x1818 && !((d-0x1518)%48);}
static int support_slot(unsigned d) {
  if(d>=0xd18 && d<0x10d8 && !((d-0xd18)%64))return (int)((d-0xd18)/64);
  if(d>=0x1518 && d<0x1818 && !((d-0x1518)%48))return 15+(int)((d-0x1518)/48);
  return -1;
}
static void target_boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized)return;
  unsigned at=pc&0xffff,d=cpu->D;
  if(at==0xdd1a || at==0xdcb3 || at==0xdafa) {
    bool pickup=at==0xdafa && item_actor(d) && g_ram[d+10]==2;
    bool pending=pickup && refill.active && refill.slot==d;
    if(state.wipe || target.active || (!pickup && !enemy_actor(d)) || !g_ram[d] ||
       (!pickup && !(g_ram[d+0x27]&127)) ||
       ((g_ram[0x1f22] || g_ram[0x1f24]) && !pending) || controller.pass || terrain.pass ||
       contact.native.pass || objects.native.pass || draw.pass || solid.native.pass || state.current!=state.anchor)return;
    unsigned seat=pending?refill.seat:
        X3CoopNearest(&state,g_ram,word(g_ram+d+5),word(g_ram+d+8));
    bool retired=state.players[seat].life!=X3_COOP_ALIVE;
    if(retired && !pending)return;
    if(!pickup && seat==state.anchor)return;
    target=(WorldTarget){(uint16_t)d,cpu->S,1,(uint8_t)seat};
    if(retired)X3CoopProjectRetired(&state,g_ram,seat);
    else X3CoopProject(&state,g_ram,seat);
    return;
  }
  /* Native item/enemy/hostile-shot continuations follow the actual RTS;
   * the original JSR frame has been consumed. */
  if(target.active && d==target.slot && cpu->S==(uint16_t)(target.s+2)) {
    if(item_actor(d)){
      if(g_ram[d+2]==6 && (g_ram[d]&128) && g_ram[0x1f28])
        refill=(PickupRefill){(uint16_t)d,target.seat,1};
      else if(refill.active && refill.slot==d)memset(&refill,0,sizeof(refill));
    }
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
static void shared_health_bank(CpuState *cpu,uint32_t pc){
  if(!enabled || !state.initialized || !target.active || target.slot!=cpu->D ||
     !item_actor(cpu->D) || g_ram[cpu->D+10]!=2 ||
     state.players[state.current].life!=X3_COOP_ALIVE)return;
  /* Full-campaign co-op shares the native subtanks. Keep the original
   * capacity, amount, timer and sounds; omit the stock Zero-only discard. */
  cpu->A&=0xff00;cpu->_flag_Z=1;cpu->_flag_N=0;cpu_mirrors_to_p(cpu);
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|((pc+3)&65535));
}
static void solid_boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized)return;
  unsigned at=pc&65535,d=cpu->D;
  int slot=support_slot(d);if(slot<0)return;
  if((pc&0x7fffff)==0x00dd1a || (pc&0x7fffff)==0x00dafa) {
    /* The native initializer owns slot reuse. A new actor cannot inherit
     * a previous platform's classified rider bits, even with the same class. */
    if(!g_ram[d] || !g_ram[d+1])riders[slot]=0;
    return;
  }
  NativePass *p=&solid.native;
  if(at==0xc021) {
    if(p->pass && solid.pc==0xbfae && p->entry.d==d)
      riders[slot]|=(uint8_t)(1u<<state.current);
    return;
  }
  if(state.wipe)return;
  if(at==0xbfae || at==0xc07f) {
    if(p->pass || !g_ram[d] || controller.pass || terrain.pass || contact.native.pass ||
       objects.native.pass || draw.pass)return;
    unsigned first=state.current;
    bool repeat=partner_ready();
    if(at==0xc07f) {
      if(!(riders[slot]&128))return; /* Unknown script: retain native ownership. */
      unsigned mask=riders[slot]&3;
      for(unsigned seat=0;seat<2;++seat)
        if(state.players[seat].life!=X3_COOP_ALIVE || !(state.players[seat].body[0x27]&127))mask&=~(1u<<seat);
      if(!mask) {interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xc09d);return;}
      if(!(mask&(1u<<first)))first^=1;
      repeat=(mask&(1u<<(first^1)))!=0;
    } else riders[slot]=128;
    solid.pc=(uint16_t)at;solid.restore=state.current;solid.seat=(uint8_t)first;
    solid.repeat=(uint8_t)repeat;p->entry=regs(cpu);p->pass=1;
    memcpy(solid.entry_scratch,g_ram,24);
    if(first!=state.current)X3CoopProject(&state,g_ram,first);
    return;
  }
  if(!p->pass || cpu->S!=p->entry.s || d!=p->entry.d ||
     (solid.pc==0xbfae?at!=0xbfce && at!=0xc07e:at!=0xc09d))return;
  if(p->pass==1 && solid.repeat && state.players[solid.seat^1].life==X3_COOP_ALIVE) {
    p->result=regs(cpu);memcpy(solid.result_scratch,g_ram,24);
    X3CoopProject(&state,g_ram,solid.seat^1);
    restore(cpu,&p->entry);memcpy(g_ram,solid.entry_scratch,24);p->pass=2;
    interp_bridge_pre_opcode_redirect((pc&0xff0000)|solid.pc);return;
  }
  unsigned pass=p->pass;
  X3CoopCapture(&state,g_ram);
  if(state.players[solid.restore].life==X3_COOP_ALIVE)X3CoopProject(&state,g_ram,solid.restore);
  if(pass==2) {restore(cpu,&p->result);memcpy(g_ram,solid.result_scratch,24);}
  p->pass=0;settle_deaths();
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
       contact.native.pass || solid.native.pass || state.current!=state.anchor) {enabled=false;return;}
    draw.entry=regs(cpu);draw.pass=(uint8_t)(seat==state.anchor?1:2);
    X3CoopProject(&state,g_ram,seat);
  } else if(pc==0x038fe1 && draw.pass && cpu->S==draw.entry.s && cpu->D==draw.entry.d) {
    X3CoopProject(&state,g_ram,state.anchor);draw.pass=0;
  }
}
static void presentation_boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized)return;
  pc&=0x7fffff;
  if(pc==0x038dd7) {
    X3CoopPresentationBegin(&state,g_ram,cpu_read16(cpu,cpu->DB,0x6621),cpu_read16(cpu,cpu->DB,0x6623));
  } else if(pc==0x038df0) {
    /* JSL7E:26A0 has waited for the original sprite program to finish. */
    X3CoopPresentationComplete();
  } else if(pc==0x038f70) {
    /* Registered after draw_boundary: tagged owned pools have been lent
     * to their real submitter before taking this native descriptor copy. */
    unsigned slot=(cpu->D+cpu->X)&65535,owner=0;
    if(slot==X3_COOP_BODY || owned_object(slot))owner=state.current+1;
    X3CoopPresentationSubmit(&state,g_ram,slot,owner);
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
       !terrain.pass && !objects.native.pass && !draw.pass && !solid.native.pass) {
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
       !controller.pass && !terrain.pass && !contact.native.pass && !draw.pass && !solid.native.pass) {
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
  if(pc==0x04bca2 && state.initialized){
    X3CoopPresentationBodyDma(state.current,g_ram+X3_COOP_BODY,g_snes->cart->rom,g_snes->cart->romImageSize);
    return;
  }
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
    X3CoopPresentationBodyDma(state.current,g_ram+X3_COOP_BODY,g_snes->cart->rom,g_snes->cart->romImageSize);
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
      X3CoopInit(&state,g_ram,p1_character);
      X3CoopPlayer *p=&state.players[p1_character^1];
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
      X3CoopPresentationEnroll(&state,g_snes->ppu->vram,g_snes->cart->rom,g_snes->cart->romImageSize);
      fprintf(stderr,"[x3-coop] native actor qualification enrolled X and Zero\n");
    }
    if(controller.pass){
      if(controller.pass==2 && refill.active && g_ram[0x1f2b] && state.current!=refill.seat)
        interp_bridge_pre_opcode_redirect((pc&0xff0000)|0x80bc);
      return;
    }
    if(g_ram[X3_COOP_BODY+2]==0x0c) {
      X3CoopCapture(&state,g_ram);
      fall_player(state.current);
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
        fall_player(state.current);
        g_ram[X3_COOP_BODY+0xb6]=0;
        return;
      }
    }
    controller.entry=regs(cpu);controller.pass=1;
    input(state.anchor);
    /* Native refills skip terrain collision globally and park only their
     * own body. The other actor must not move through that collision pause. */
    if(refill.active && g_ram[0x1f2b] && state.current!=refill.seat)
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0x80bc);
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
        fall_player(partner);
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
  memset(&refill,0,sizeof(refill));
  memset(&solid,0,sizeof(solid));memset(riders,0,sizeof(riders));
  memset(&view_spawn,0,sizeof(view_spawn));X3CoopViewsReset();
  memset(&view_bounds,0,sizeof(view_bounds));
  X3CoopPresentationReset();
}
void X3CoopRuntimeSetP1Character(unsigned character) {
  if(!state.initialized && character<=X3_COOP_ZERO)p1_character=character;
}
void X3CoopRuntimeFrame(void) {
  if(!enabled)return;
  X3CoopViewsSetRom(g_snes->cart->rom,g_snes->cart->romImageSize);
  if(!interp_bridge_add_pre_opcode_hook(0x048008,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x0480bc,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04809e,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04bca2,boundary) ||
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
  static const uint32_t target_pcs[]={0x00dd1a,0x00dcb3,0x00dccf,0x00dcf3,0x00dc68,0x00dc8c,
      0x00dafa,0x00daaf,0x00dad3};
  for(unsigned i=0;i<sizeof(target_pcs)/sizeof(*target_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(target_pcs[i],target_boundary))enabled=false;
  static const uint32_t target_queries[]={0x02e142,0x02daa7,0x02dac6};
  for(unsigned i=0;i<sizeof(target_queries)/sizeof(*target_queries);++i)
    if(!interp_bridge_add_pre_opcode_hook(target_queries[i],target_query))enabled=false;
  if(!interp_bridge_add_pre_opcode_hook(0x01d837,shared_health_bank))enabled=false;
  static const uint32_t solid_pcs[]={0x04bfae,0x04bfce,0x04c021,0x04c07e,
      0x04c07f,0x04c09d,0x00dd1a,0x00dafa};
  for(unsigned i=0;i<sizeof(solid_pcs)/sizeof(*solid_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(solid_pcs[i],solid_boundary))enabled=false;
  static const uint32_t view_pcs[]={0x00da38,0x00de43,0x00dec3,0x00decf,
      0x00df1e,0x00df4c,0x00df9f,0x02d58a,0x02d5ab,0x02d5da,0x02d611,0x02d636};
  for(unsigned i=0;i<sizeof(view_pcs)/sizeof(*view_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(view_pcs[i],view_world_boundary))enabled=false;
  if(!interp_bridge_add_pre_opcode_hook(0x00e2a2,view_bounds_boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x00e316,view_bounds_boundary))enabled=false;
  static const uint32_t presentation_pcs[]={0x038dd7,0x038df0,0x038f70};
  for(unsigned i=0;i<sizeof(presentation_pcs)/sizeof(*presentation_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(presentation_pcs[i],presentation_boundary))enabled=false;
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
  uint8_t refill_bytes[8]={'X','3','R','1',0,0,refill.seat,refill.active};
  putword(refill_bytes+4,refill.slot);sli->func(sli,refill_bytes,sizeof(refill_bytes));
  uint8_t views[X3_COOP_VIEW_STATE_BYTES];X3CoopViewsSave(views);
  sli->func(sli,views,sizeof(views));sli->func(sli,&view_spawn,sizeof(view_spawn));
  sli->func(sli,&view_bounds,sizeof(view_bounds));
  X3CoopPresentationSave(sli);
  sli->func(sli,&solid,sizeof(solid));sli->func(sli,riders,sizeof(riders));
}
static bool view_spawn_valid(const ViewSpawn *v) {
  if(v->count>sizeof(v->columns) || v->cursor>v->count || v->active>1)return false;
  if(v->active && !v->count)return false;
  if(!v->active && (v->count || v->cursor))return false;
  for(unsigned i=1;i<v->count;++i)if(v->columns[i]<=v->columns[i-1])return false;
  return true;
}
static bool passes_valid(const X3CoopState *s,const NativePass *c,const NativePass *t,
                         const ContactPass *hit,const ObjectPass *obj,const NativePass *art,
                         const WorldTarget *world,const PickupRefill *healing,const SolidPass *support,const uint8_t *rider_bits) {
  unsigned a=c->pass,b=t->pass,h=hit->native.pass,o=obj->native.pass,v=art->pass,q=support->native.pass;
  if(a>2 || b>2 || h>2 || o>2 || v>2 || q>2 || (a!=0)+(b!=0)+(h!=0)+(o!=0)+(v!=0)+(q!=0)>1)return false;
  for(unsigned i=0;i<SHARED_SUPPORTS;++i)
    if(rider_bits[i] && (rider_bits[i]&252)!=128)return false;
  if(world->active>1 || world->seat>1)return false;
  if(!s->initialized)return !(a || b || h || o || v || q || world->active);
  if(world->active && ((!enemy_actor(world->slot) && !item_actor(world->slot)) || a || b || o || v))return false;
  const NativePass *active=a?c:b?t:h?&hit->native:o?&obj->native:v?art:q?&support->native:NULL;
  unsigned base=h?hit->seat:q?support->seat:s->anchor;
  if(active && (base>1 || s->wipe || s->current!=(base^(active->pass==2)) ||
     s->players[s->current].life!=X3_COOP_ALIVE))return false;
  if(world->active) {
    bool cleanup=healing->active && healing->slot==world->slot && healing->seat==world->seat;
    unsigned seat=s->players[world->seat].life==X3_COOP_ALIVE || cleanup?world->seat:s->anchor;
    if(h?hit->seat!=seat:q?support->restore!=seat:!s->wipe && s->current!=seat)return false;
  }
  if((a && c->entry.d!=X3_COOP_BODY) || (b && t->entry.d!=X3_COOP_BODY))return false;
  if(h && ((hit->pc!=0xcb31 && hit->pc!=0xcb74) || !enemy_actor(hit->native.entry.d)))return false;
  if(o && (!object_end(obj->pc) || object_end(obj->pc)!=obj->end))return false;
  if(v && (art->entry.d || !owned_object(art->entry.x)))return false;
  if(q && ((support->pc!=0xbfae && support->pc!=0xc07f) || support->restore>1 ||
      support->repeat>1 || support_slot(support->native.entry.d)<0))return false;
  return true;
}
static bool refill_decode(PickupRefill *out,const uint8_t bytes[8],const X3CoopState *s){
  if(memcmp(bytes,"X3R1",4) || bytes[6]>1 || bytes[7]>1)return false;
  PickupRefill r={(uint16_t)word(bytes+4),bytes[6],bytes[7]};
  if(r.active?(!s->initialized || !item_actor(r.slot)):(r.slot || r.seat))return false;
  *out=r;return true;
}
bool X3CoopRuntimeLoad(SaveLoadInfo *sli) {
  uint8_t bytes[4096];X3CoopState loaded;
  NativePass c,t,art;ContactPass hit;ObjectPass obj;WorldTarget world;SolidPass support;uint8_t rider_bits[SHARED_SUPPORTS];
  sli->func(sli,bytes,X3CoopEncodedSize());
  sli->func(sli,&c,sizeof(c));sli->func(sli,&t,sizeof(t));
  sli->func(sli,&hit,sizeof(hit));sli->func(sli,&obj,sizeof(obj));
  sli->func(sli,&art,sizeof(art));
  sli->func(sli,&world,sizeof(world));
  uint8_t refill_bytes[8];PickupRefill healing;
  sli->func(sli,refill_bytes,sizeof(refill_bytes));
  uint8_t views[X3_COOP_VIEW_STATE_BYTES];ViewSpawn spawn;
  sli->func(sli,views,sizeof(views));sli->func(sli,&spawn,sizeof(spawn));
  ViewBounds bounds;sli->func(sli,&bounds,sizeof(bounds));
  if(!X3CoopPresentationLoad(sli))return false;
  sli->func(sli,&support,sizeof(support));sli->func(sli,rider_bits,sizeof(rider_bits));
  if(!X3CoopViewsLoad(views,sizeof(views),false) || !view_spawn_valid(&spawn) || bounds.native.pass>2)return false;
  if(!X3CoopDecode(&loaded,bytes,X3CoopEncodedSize()) || !refill_decode(&healing,refill_bytes,&loaded) ||
     !passes_valid(&loaded,&c,&t,&hit,&obj,&art,&world,&healing,&support,rider_bits))return false;
  state=loaded;controller=c;terrain=t;contact=hit;objects=obj;draw=art;target=world;solid=support;
  refill=healing;
  /* A state can load before the first reset-frame environment setup. An
   * enrolled machine must resume its actor schedule on that startup path. */
  if(loaded.initialized){enabled=true;p1_character=loaded.players[0].character;}
  memcpy(riders,rider_bits,sizeof(riders));view_spawn=spawn;view_bounds=bounds;
  X3CoopViewsLoad(views,sizeof(views),true);return true;
}
bool X3CoopRuntimePreflight(const void *data,size_t size) {
  size_t payload=X3CoopEncodedSize(),tail=payload+sizeof(controller)+sizeof(terrain)+sizeof(contact)+sizeof(objects)+sizeof(draw)+sizeof(target)+8+sizeof(solid)+sizeof(riders)+X3_COOP_VIEW_STATE_BYTES+sizeof(view_spawn)+sizeof(view_bounds)+X3CoopPresentationEncodedSize();
  if(!data || size<tail)return false;
  X3CoopState decoded;
  const uint8_t *p=(const uint8_t *)data+size-tail;
  if(!X3CoopDecode(&decoded,p,payload))return false;
  /* Validate before the common loader changes WRAM, CPU or either pass. */
  NativePass c,t,art;ContactPass hit;ObjectPass obj;WorldTarget world;SolidPass support;p+=payload;
  memcpy(&c,p,sizeof(c));p+=sizeof(c);memcpy(&t,p,sizeof(t));p+=sizeof(t);
  memcpy(&hit,p,sizeof(hit));p+=sizeof(hit);memcpy(&obj,p,sizeof(obj));p+=sizeof(obj);
  memcpy(&art,p,sizeof(art));p+=sizeof(art);memcpy(&world,p,sizeof(world));p+=sizeof(world);
  PickupRefill healing;if(!refill_decode(&healing,p,&decoded))return false;p+=8;
  if(!X3CoopViewsLoad(p,X3_COOP_VIEW_STATE_BYTES,false))return false;
  p+=X3_COOP_VIEW_STATE_BYTES;ViewSpawn spawn;memcpy(&spawn,p,sizeof(spawn));
  p+=sizeof(spawn);ViewBounds bounds;memcpy(&bounds,p,sizeof(bounds));
  p+=sizeof(bounds);
  if(!X3CoopPresentationPreflight(p,X3CoopPresentationEncodedSize()))return false;
  p+=X3CoopPresentationEncodedSize();memcpy(&support,p,sizeof(support));p+=sizeof(support);
  if(!passes_valid(&decoded,&c,&t,&hit,&obj,&art,&world,&healing,&support,p))return false;
  if(bounds.native.pass>2)return false;
  if(bounds.native.pass && (!decoded.initialized || decoded.wipe || bounds.native.entry.d!=0x1e58 ||
      c.pass || t.pass || hit.native.pass || obj.native.pass || art.pass || world.active || support.native.pass ||
      decoded.current!=(decoded.anchor^(bounds.native.pass==2))))return false;
  return view_spawn_valid(&spawn) && (!spawn.active || (decoded.initialized && !decoded.wipe));
}
