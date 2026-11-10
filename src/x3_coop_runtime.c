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
static void boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || cpu->D!=X3_COOP_BODY || cpu->emulation)return;
  pc&=0x7fffff;
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
}
void X3CoopRuntimeFrame(void) {
  if(!enabled)return;
  if(!interp_bridge_add_pre_opcode_hook(0x048008,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x0480bc,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04809e,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04b591,boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x04b5ab,boundary)) {
    fprintf(stderr,"[x3-coop] controller hook capacity exhausted\n");enabled=false;
  }
}
void X3CoopRuntimeSave(SaveLoadInfo *sli) {
  uint8_t bytes[4096]={0};
  X3CoopCapture(&state,g_ram);
  X3CoopEncode(&state,bytes,X3CoopEncodedSize());
  sli->func(sli,bytes,X3CoopEncodedSize());
  sli->func(sli,&controller,sizeof(controller));
  sli->func(sli,&terrain,sizeof(terrain));
}
bool X3CoopRuntimeLoad(SaveLoadInfo *sli) {
  uint8_t bytes[4096];X3CoopState loaded;
  sli->func(sli,bytes,X3CoopEncodedSize());
  sli->func(sli,&controller,sizeof(controller));
  sli->func(sli,&terrain,sizeof(terrain));
  if(!X3CoopDecode(&loaded,bytes,X3CoopEncodedSize()) || controller.pass>2 || terrain.pass>2)return false;
  state=loaded;return true;
}
bool X3CoopRuntimePreflight(const void *data,size_t size) {
  size_t payload=X3CoopEncodedSize(),tail=payload+sizeof(controller)+sizeof(terrain);
  if(!data || size<tail)return false;
  X3CoopState decoded;
  const uint8_t *p=(const uint8_t *)data+size-tail;
  if(!X3CoopDecode(&decoded,p,payload))return false;
  /* Validate before the common loader changes WRAM, CPU or either pass. */
  unsigned controller_pass=p[payload+offsetof(NativePass,pass)];
  unsigned terrain_pass=p[payload+sizeof(controller)+offsetof(NativePass,pass)];
  return controller_pass<=2 && terrain_pass<=2 &&
      !(controller_pass && terrain_pass) &&
      (decoded.initialized || (!controller_pass && !terrain_pass));
}
