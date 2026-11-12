#include "x3_coop_runtime.h"
#include "x3_coop_view.h"
#include "x3_coop_present.h"
#include "x3_coop_menu.h"
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
static bool stock_zero,stock_exit_pending;
static bool learner_enabled,learner_rescue;
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
/* Native world actors update once. Borrow a hostile's target, a pickup's
 * recipient or an effect's creator for that call; keep the canonical anchor. */
typedef struct WorldTarget {uint16_t slot,s;uint8_t active,seat;} WorldTarget;
static WorldTarget target;
/* Retail shared effects retain their creator after the personal shot pass
 * returns. The pool still runs once; only its player context is borrowed. */
enum { SHARED_EFFECTS=40, EFFECT_BASE=0x1818, EFFECT_STRIDE=32 };
static uint8_t effect_owner[SHARED_EFFECTS];
static int effect_index(unsigned d){
  return d>=EFFECT_BASE && d<EFFECT_BASE+SHARED_EFFECTS*EFFECT_STRIDE &&
      !((d-EFFECT_BASE)%EFFECT_STRIDE)?(int)((d-EFFECT_BASE)/EFFECT_STRIDE):-1;
}
static void retire_vehicle_missiles(unsigned seat){
  /* Native02:D8C4 allocates Ride Armor missiles in X's personal shot pool.
   * Cancelling that pool bypasses their normal shared capacity decrement.
   * The class1 cinematic armor has a different contract. */
  if(state.players[seat].character==X3_COOP_X && g_ram[0xcc8] && !g_ram[0xcd2])g_ram[0xd15]=0;
}
static void retire_shared_effects(unsigned seat){
  for(unsigned i=0;i<SHARED_EFFECTS;++i)if(effect_owner[i]==seat+1){
    memset(g_ram+EFFECT_BASE+i*EFFECT_STRIDE,0,EFFECT_STRIDE);effect_owner[i]=0;
  }
}
/* A native resource refill remains in the shared item pool while all world
 * pools are frozen. Its original recipient outlives an individual item call. */
typedef struct PickupRefill {uint16_t slot;uint8_t seat,active;} PickupRefill;
static PickupRefill refill;
/* One native script owns the shared world. The other living participant is
 * parked until its original cleanup, then arrives at the same safe floor. */
enum { SCENE_DOOR,SCENE_CAPSULE_OPEN,SCENE_CAPSULE_AWARD,SCENE_STAGE_ENTRY,
       SCENE_INTRO_CAPTURE,SCENE_INTRO_ZERO,SCENE_INTRO_RETURN,
       SCENE_INTRO_ZERO_CONSTRUCT,SCENE_INTRO_X_CONSTRUCT,SCENE_INTRO_RETIRED_X };
typedef struct NativeScene {uint16_t slot;uint8_t owner,phase,tick,stage,both,kind;uint16_t raw[2];} NativeScene;
static NativeScene scene;
enum { PARTICIPATION_HOLD=90, PROFILE_STOCK=1, PROFILE_LEARNER=2 };
static uint8_t participation_hold[2];
static bool stage_pending(void){return scene.phase && scene.kind==SCENE_STAGE_ENTRY;}
static bool intro_cinematic(void){return scene.phase && scene.kind==SCENE_INTRO_RETIRED_X;}
static void scene_tick(void);
static void participation_tick(void);
static void stock_tick(void);
static void shared_catchup_tick(void);
static void learner_tick(void);
static void intro_helper_handoff(void);
static bool scene_parked(unsigned seat){return scene.phase && seat!=scene.owner;}
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
static bool menu_pending(void);
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
bool X3CoopRuntimeSceneActive(void){return enabled && state.initialized && scene.phase;}
bool X3CoopRuntimeReady(void){return enabled && state.initialized && !stage_pending() && !intro_cinematic();}
/* Native03:A1BC selects the Ride Armor HUD dispatcher03:9155 with0A.
 * This is a playable vehicle mode, not a shared cinematic/freeze. */
static bool vehicle_mode(void){
  if(!enabled || !state.initialized || !g_ram[0xcc8] || g_ram[0xcd2] ||
      !(g_ram[0xcd3]&64) || g_ram[0x1f22]!=10)return false;
  unsigned x=state.players[0].character==X3_COOP_X?0:1;
  const uint8_t *b=x==state.current?g_ram+X3_COOP_BODY:state.players[x].body;
  return state.players[x].life==X3_COOP_ALIVE && b[2]==0x2c;
}
bool X3CoopRuntimeViewScene(void){return X3CoopRuntimeSceneActive() ||
    (g_ram[0x1f22] && !vehicle_mode()) || g_ram[0x1f24] || g_ram[0x1f63];}
static bool view_scene(void){return X3CoopRuntimeViewScene();}
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
  if(stage_pending())return;
  unsigned at=pc&0x7fffff;
  if(at==0x00da38) {
    if(enabled && state.initialized && intro_cinematic()){scene_tick();return;}
    if(enabled && state.initialized)stock_tick();
    if(enabled && state.initialized)scene_tick();
    if(enabled && state.initialized && X3CoopViewsIndependent()) {
      X3CoopCapture(&state,g_ram);
      X3CoopViewsEndFrame(g_ram,&state,view_scene(),false);
    }
    if(enabled && state.initialized)participation_tick();
    if(enabled && state.initialized)learner_tick();
    if(enabled && state.initialized)shared_catchup_tick();
    return;
  }
  /* The native refill owns a shared freeze until its cleanup runs. Removing
   * its recipient's view must not cull the item before that cleanup. */
  bool retain_refill=enabled && state.initialized && refill.active &&
      refill.slot==cpu->D && at==0x02d58a;
  bool retain_door=enabled && state.initialized && scene.phase && scene.kind<=SCENE_CAPSULE_AWARD && scene.slot==cpu->D &&
      at==0x02d58a && g_ram[cpu->D+1]<10;
  if(!view_ready() && !retain_refill && !retain_door)return;
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
  int effect=effect_index(d);
  unsigned effect_seat=effect>=0?effect_owner[effect]:0;
  bool owned=effect_seat || (d>=X3_COOP_AUX && d<X3_COOP_AUX+X3_COOP_AUX_BYTES) ||
      (d>=X3_COOP_SHOTS && d<X3_COOP_SHOTS+X3_COOP_SHOT_BYTES);
  bool inside=false;
  for(unsigned seat=0;seat<2;++seat) {
    if(state.players[seat].life!=X3_COOP_ALIVE ||
       (owned && seat!=(effect_seat?effect_seat-1:state.current)))continue;
    X3CoopView v=X3CoopViewsForPlayer(g_ram,&state,seat,false);
    int dx=(int16_t)(x-v.x),dy=(int16_t)(y-v.y);
    if(dx>=-left && dx<right && dy>=-top && dy<bottom){inside=true;chosen=v;break;}
  }
  if(retain_refill || retain_door){inside=true;chosen.x=x;chosen.y=y;}
  unsigned value=(uint16_t)(x-chosen.x+left),limit=(unsigned)(left+right);
  if(value<limit){value=(uint16_t)(y-chosen.y+top);limit=(unsigned)(top+bottom);}
  cpu->A=(uint16_t)value;cpu_mirrors_to_p(cpu);cpu->P&=(uint8_t)~0x20;cpu_p_to_mirrors(cpu);
  cpu->_flag_C=!inside;cpu->_flag_Z=value==limit;cpu->_flag_N=((value-limit)&32768)!=0;
  cpu_mirrors_to_p(cpu);
  if(at==0x00df9f){next=inside?0xdfc1:0xdfbc;}
  else if(at!=0x02d58a && at!=0x02d5ab && inside)next=0xd65b;
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|next);
}
#include "x3_coop_shared_camera.inc"
/* Controller, terrain and camera-bound passes are serialized and exclusive.
 * Reuse their existing camera backup for a complete personal view borrow.
 * Pass3 means a personal controller/terrain projection, not another world pass. */
static void personal_view_begin(unsigned seat){
  bool vehicle=target.active && target.slot==0xcc8 && !scene.phase && X3CoopViewsIndependent();
  if((!view_ready() && !vehicle) || seat==state.anchor || state.current!=state.anchor || view_bounds.native.pass)return;
  const X3CoopViewState *v=X3CoopViewsState();if(!(v->valid&(1u<<seat)))return;
  memcpy(view_bounds.camera,g_ram+0x1e58,64);view_bounds.native.pass=3;
  X3CoopViewsAdoptPlayer(g_ram,&state,seat,false);
}
static void personal_view_end(void){
  if(view_bounds.native.pass!=3)return;
  memcpy(g_ram+0x1e58,view_bounds.camera,64);view_bounds.native.pass=0;
}
static void view_bounds_boundary(CpuState *cpu,uint32_t pc) {
  if(cpu->D!=0x1e58 || view_bounds.native.pass==3)return;
  if((pc&65535)==0xe2a2 && !view_bounds.native.pass) {
    if((!view_ready() && !shared_camera_ready()) || state.current!=state.anchor || !partner_ready())return;
    /* For a shared view both bodies see the same pre-clamp candidate.
     * These scratch bytes are replaced by the original result at return. */
    if(!X3CoopViewsIndependent()){
      putword(view_bounds.scratch,word(g_ram+0x1e5d));
      putword(view_bounds.scratch+2,word(g_ram+0x1e60));
    }
    view_bounds.native.entry=regs(cpu);view_bounds.native.pass=1;return;
  }
  if((pc&65535)!=0xe316 || !view_bounds.native.pass)return;
  if(cpu->S!=view_bounds.native.entry.s){enabled=false;return;}
  if(view_bounds.native.pass==1) {
    view_bounds.native.result=regs(cpu);
    unsigned shared_x=word(view_bounds.scratch),shared_y=word(view_bounds.scratch+2);
    memcpy(view_bounds.camera,g_ram+0x1e58,64);memcpy(view_bounds.scratch,g_ram,24);
    unsigned partner=state.anchor^1;
    X3CoopCapture(&state,g_ram);
    if(X3CoopViewsIndependent()){
      X3CoopViewsUpdate(g_ram,&state,false,false);
      X3CoopView v=X3CoopViewsFollowPlayer(g_ram,&state,partner);
      const X3CoopViewState *views=X3CoopViewsState();
      X3CoopProject(&state,g_ram,partner);
      for(unsigned i=0;i<4;++i)putword(g_ram+0x1e66+i*2,
          views->camera_valid&(1u<<partner)?views->current[partner][i]:views->limits[partner][i]);
      putword(g_ram+0x1e5d,(unsigned)v.x);putword(g_ram+0x1e60,(unsigned)v.y);
    }else{
      X3CoopProject(&state,g_ram,partner);
      putword(g_ram+0x1e5d,shared_x);putword(g_ram+0x1e60,shared_y);
    }
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
  bool returning=stock_zero && stock_exit_pending && player->character==X3_COOP_ZERO;
  if(returning)body[0x57]=body[0x58]=body[0x5a]=body[0x5b]=0;
  bool native_input=scene.kind==SCENE_CAPSULE_AWARD && seat==scene.owner && g_ram[0x1f45];
  /* The capsule writes recorded native pad words and temporarily changes the
   * mapping. NMI has already remapped those actions for its owning X body;
   * host input must not replace them, including the later arm demonstration. */
  unsigned held=native_input?word(body+0x36):X3CoopActions(RtlGetPadState(seat),g_ram+0xffc0);
  /* A moving support or airborne approach can delay X's safe return. Keep
   * movement/jump/dash available so Zero can reach ground, without fighting
   * or opening another modal in the restricted encounter. */
  if(returning)held&=0x8f80;
  if(!native_input){putword(body+0x36,held);putword(body+0x38,previous);putword(body+0x3a,held&~previous);}
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
  bool fallen=X3CoopFall(&state,seat);
  if(fallen){learner_rescue=false;retire_shared_effects(seat);retire_vehicle_missiles(seat);}
  if(fallen && (state.wipe || state.players[seat].character==X3_COOP_ZERO))stock_exit_pending=false;
  /* Original04:933C selects the Zero-loss branch;00:9D11 commits this
   * campaign flag after its fade. Co-op has no individual world fade. */
  if(fallen && stock_zero && state.players[seat].character==X3_COOP_ZERO)g_ram[0x1fb2]|=0x40;
  return fallen;
}
static void settle_deaths(void) {
  if(!enabled || !state.initialized || state.wipe || stage_pending() || intro_cinematic())return;
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
  if(!controller.pass && !terrain.pass && !contact.native.pass && !objects.native.pass &&
     !draw.pass && !target.active && !solid.native.pass && !view_bounds.native.pass)
    intro_helper_handoff();
  if(state.wipe)g_ram[X3_COOP_BODY+0xb6]=0;
}
static void team_death_decision(CpuState *cpu,uint32_t pc) {
  (void)cpu;(void)pc;
  if(menu_pending())return;
  if(controller.pass || terrain.pass || contact.native.pass || objects.native.pass ||
     draw.pass || target.active || solid.native.pass || view_bounds.native.pass)return;
  settle_deaths();
}
static bool partner_ready(void) {
  return enabled && state.initialized && !state.wipe && !scene.phase &&
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
static bool shared_pickup(unsigned d){return item_actor(d) && g_ram[d+10]>=2 && g_ram[d+10]<=4;}
static bool x_pickup(unsigned d){return item_actor(d) &&
    (g_ram[d+10]==1 || g_ram[d+10]==5 || g_ram[d+10]==11);}
static bool native_door(unsigned d){return item_actor(d) &&
    (g_ram[d+10]==7 || g_ram[d+10]==0x10 || g_ram[d+10]==0x11);}
static bool native_capsule(unsigned d){return enemy_actor(d) && !hostile_projectile(d) && g_ram[d+10]==0x4d;}
static unsigned native_pad(unsigned seat){
  unsigned input=RtlGetPadState(seat),native=0;
  for(unsigned bit=0;bit<12;++bit)if(input&(1u<<bit))native|=0x8000u>>bit;
  return native;
}
#include "x3_coop_input.inc"
static void scene_clear(void){
  if(scene.phase && scene.kind && !g_ram[0x1f63]){
    /* Consume the modal's final edge before returning JOY1 to native play. */
    unsigned held=native_pad(0);putword(g_ram+0xa8,held);putword(g_ram+0xaa,held);putword(g_ram+0xac,0);
    scene.raw[0]=scene.raw[1]=(uint16_t)held;
  }else if(enabled && !g_ram[0x1f63])primary_input_restore();
  uint8_t both=scene.both;uint16_t raw[2];memcpy(raw,scene.raw,sizeof(raw));
  memset(&scene,0,sizeof(scene));scene.both=both;memcpy(scene.raw,raw,sizeof(raw));
}
static void scene_cancel_transients(unsigned seat){
  X3CoopPlayer *p=&state.players[seat];
  memset(p->auxiliaries,0,sizeof(p->auxiliaries));memset(p->shots,0,sizeof(p->shots));
  p->shot_command=0;p->previous_actions=0;
  retire_shared_effects(seat);retire_vehicle_missiles(seat);
  p->native_flags&=15; /* Original01:8725 retires these owned effect flags. */
  p->body[0x57]=p->body[0x58]=p->body[0x5a]=p->body[0x5b]=0;
  /* These counters/latches describe the shots and children just discarded,
   * not retained resources. Zero keeps readiness outside the charge meters. */
  p->body[0x35]=p->body[0x7d]=p->body[0x89]=p->body[0xaa]=p->body[0xb7]=0;
  for(unsigned i=0;i<SHARED_SUPPORTS;++i)riders[i]&=(uint8_t)~(1u<<seat);
}
static void scene_begin(unsigned slot,unsigned kind){
  if(scene.phase && (scene.slot!=slot || kind!=SCENE_CAPSULE_AWARD))return;
  learner_rescue=false;
  X3CoopCapture(&state,g_ram);scene.slot=(uint16_t)slot;scene.owner=state.current;
  scene.stage=g_ram[0x1fae];scene.tick=0;scene.kind=(uint8_t)kind;
  unsigned other=scene.owner^1;const X3CoopPlayer *p=&state.players[other];
  bool present=p->life==X3_COOP_ALIVE && (p->body[0x27]&127) && p->body[2]!=12;
  scene.phase=(uint8_t)(present?1:2);if(present)scene_cancel_transients(other);
}
static int scene_feet(const uint8_t *body){
  unsigned address=word(body+0x20);size_t at=0x30000+(address&32767);
  if(address<32768 || at+4>g_snes->cart->romImageSize)return 0;
  const uint8_t *rect=g_snes->cart->rom+at;return (int8_t)rect[1]+rect[3];
}
static void scene_position(unsigned seat,const uint8_t *owner){
  X3CoopPlayer *p=&state.players[seat];uint8_t *b=p->body;
  int y=(int)word(owner+8)+scene_feet(owner)-scene_feet(b);
  if(y<0)y=0;if(y>8191)y=8191;
  putword(b+5,word(owner+5));putword(b+8,(unsigned)y);b[4]=b[7]=0;
  putword(b+0x22,word(b+5));putword(b+0x24,word(b+8));memset(b+0x1a,0,6);
}
static void restore_x_armor(unsigned seat){
  X3CoopPlayer *p=&state.players[seat];
  if(p->character!=X3_COOP_X)return;
  /* Native04:B5BB rebuilds these three persistent armor children. Clearing
   * transient pools for transport/withdrawal must not permanently drop them. */
  unsigned armor=g_ram[0x1fd1];p->body[0x16]=(armor&1)?0x18:0;
  memset(p->auxiliaries,0,3*32);
  for(unsigned part=0;part<3;++part)if(armor&(2u<<part)){
    uint8_t *child=p->auxiliaries+part*32;child[0]=1;child[0x0b]=(uint8_t)part;child[0x10]=0x5d;
  }
}
static void scene_place(unsigned seat,const uint8_t *owner){
  scene_position(seat,owner);X3CoopPlayer *p=&state.players[seat];uint8_t *b=p->body;
  b[2]=b[3]=0;b[0x0e]=1;b[0x26]=32;
  b[0x6a]=b[0x6d]=b[0x6e]=b[0xa3]=0;b[0xa5]&=0x7f;
  memset(b+0x37,0,4);p->previous_actions=0;
  restore_x_armor(seat);
}
static void scene_release_views(void){
  X3CoopViewsCommitNative(g_ram,&state);
}
static void scene_install(unsigned seat){
  const X3CoopPlayer *p=&state.players[seat];
  memcpy(g_ram+X3_COOP_BODY,p->body,sizeof(p->body));
  memcpy(g_ram+X3_COOP_AUX,p->auxiliaries,sizeof(p->auxiliaries));
  memcpy(g_ram+X3_COOP_SHOTS,p->shots,sizeof(p->shots));
  g_ram[0x1f1e]=p->shot_command;g_ram[0x1fd2]=p->maximum_health;anchor_palette();
  g_ram[0x1fff]=p->native_flags;
}
/* Read-only04:BF2F/BEA5 terrain lookup. Use the live layout/metatiles so a
 * destroyed wall is immediately reflected; no guest collision pass is run. */
static bool terrain_class(int x,int y,unsigned *type){
  if(x<0 || y<0 || x>=8192 || y>=8192)return false;
  unsigned screen=g_ram[0xe800+(y>>8)*32+(x>>8)];
  unsigned cell=0x2800+screen*512+((y&240)<<1)+((x&240)>>3);
  if(cell+1>=0x10000)return false;
  unsigned address=word(g_ram+0x9c2)|((unsigned)g_ram[0x9c4]<<16);
  address+=word(g_ram+cell); /* Native long,Y carries into the data bank. */
  size_t at=((address>>16)&127)*32768+(address&32767);
  if((address&65535)<32768 || at>=g_snes->cart->romImageSize)return false;
  *type=g_snes->cart->rom[at]&63;return true;
}
static bool landing_clear(int x,int floor,unsigned height,int source_x,int source_y){
  /* Native floor classes34..3D are flat solids. Never return onto spikes,
   * slopes, one-way or transient supports; wait for a clear terrain floor. */
  for(int dx=-8;dx<=8;dx+=4){unsigned type;
    if(!terrain_class(x+dx,floor,&type) || type<0x34 || type>0x3d)return false;
  }
  for(int y=floor-(int)height;y<floor;++y)for(int dx=-10;dx<=10;dx+=5){unsigned type;
    if(!terrain_class(x+dx,y,&type) || (type && type!=13 && type!=14))return false;
  }
  /* A clear box across a closed door/wall is not a usable return. Keep the
   * route back to the living participant clear too, including shared actors. */
  int left=x<source_x?x:source_x,right=x>source_x?x:source_x;
  for(int cx=left;cx<=right;++cx){unsigned type;
    if(!terrain_class(cx,source_y-8,&type) || (type && type!=13 && type!=14))return false;
  }
  /* Leave space for native enemies, projectiles and shared supports. Their
   * live native collision rectangles take precedence over a clear map tile. */
  for(unsigned pool=0;pool<3;++pool){
    unsigned begin=pool==0?0x0cc8:pool==1?0x0d18:0x1518;
    unsigned end=pool==0?0x0d18:pool==1?0x1518:0x1818,stride=pool==0?80:pool==1?64:48;
    for(unsigned d=begin;d<end;d+=stride){
      if(!g_ram[d])continue;
      int ox=word(g_ram+d+5),oy=word(g_ram+d+8),half_x=24,half_y=32;
      unsigned address=word(g_ram+d+0x20);size_t at=0x30000+(address&32767);
      if(address>=32768 && at+4<=g_snes->cart->romImageSize){const uint8_t *r=g_snes->cart->rom+at;
        ox+=(g_ram[d+17]&64)?-(int8_t)r[0]:(int8_t)r[0];
        oy+=(g_ram[d+17]&128)?-(int8_t)r[1]:(int8_t)r[1];half_x=r[2];half_y=r[3];}
      if(ox+half_x>=left-12 && ox-half_x<=right+12 &&
         oy+half_y>=floor-(int)height && oy-half_y<floor)return false;
    }
  }
  return true;
}
static bool participation_ready(void){
  if(!X3CoopRuntimeReady() || menu_pending() || state.wipe || state.current!=state.anchor || scene.phase || refill.active ||
     controller.pass || terrain.pass || contact.native.pass || objects.native.pass || draw.pass ||
     target.active || solid.native.pass || view_bounds.native.pass || view_spawn.active ||
     g_ram[0xd1]!=4 || g_ram[0xd2]!=4 || g_ram[0x1f22] || g_ram[0x1f24])return false;
  for(unsigned a=0x1f25;a<=0x1f2b;++a)if(g_ram[a])return false;
  /* A scripted/captured body must first complete its native release. */
  for(unsigned s=0;s<2;++s)if(state.players[s].life==X3_COOP_ALIVE){
    const uint8_t *b=s==state.current?g_ram+X3_COOP_BODY:state.players[s].body;
    if(b[0x6e] || b[0x64] || b[2]>0x16 || b[2]==12 || !(b[0x27]&127) ||
       b[0xb6]!=(state.players[s].character==X3_COOP_ZERO?2:0))return false;
  }
  return true;
}
bool X3CoopRuntimeWithdraw(unsigned seat){
  if(seat>1 || !participation_ready() || state.players[seat].life!=X3_COOP_ALIVE ||
     state.players[seat^1].life!=X3_COOP_ALIVE)return false;
  X3CoopCapture(&state,g_ram);
  if(seat==state.anchor)X3CoopViewsAdoptPlayer(g_ram,&state,seat^1,false);
  if(!X3CoopWithdraw(&state,seat))return false;
  learner_rescue=false;
  if(state.players[seat].character==X3_COOP_ZERO)stock_exit_pending=false;
  scene_cancel_transients(seat);X3CoopProject(&state,g_ram,state.anchor);anchor_palette();return true;
}
bool X3CoopRuntimeZeroAvailable(void){
  /* Original00:CBB2 pause-menu predicate for calling Zero from X. */
  unsigned stage=g_ram[0x1fae];
  unsigned x=state.initialized?(state.players[0].character==X3_COOP_X?0:1):0;
  unsigned action=state.initialized?state.players[x].body[2]:g_ram[X3_COOP_BODY+2];
  if(state.initialized && state.current==x)action=g_ram[X3_COOP_BODY+2];
  return action!=0x2c && action!=0x76 && stage && stage!=9 && stage!=12 && stage!=13 &&
      !(g_ram[0x1f5d]&0x80) && !(g_ram[0x1fe1]&1) && !(g_ram[0x1fb2]&0x40);
}
typedef struct SafeLanding {uint16_t shape,x,y;} SafeLanding;
static bool find_landing(unsigned seat,const uint8_t *owner,X3CoopView view,SafeLanding *landing){
  unsigned shape=state.players[seat].character==X3_COOP_ZERO?0xb40e:0xb404;
  size_t at=0x30000+(shape&32767);
  if(at+9>g_snes->cart->romImageSize)return false;
  const uint8_t *rect=g_snes->cart->rom+at;
  unsigned height=state.players[seat].character==X3_COOP_ZERO?48:40;
  unsigned address=word(owner+0x20);size_t owner_at=0x30000+(address&32767);
  if(address<32768 || owner_at+9>g_snes->cart->romImageSize)return false;
  const uint8_t *own_rect=g_snes->cart->rom+owner_at;
  int px=word(owner+5),py=word(owner+8)+(int8_t)own_rect[6]+own_rect[8]+1;
  const int gaps[]={32,-32,48,-48,64,-64,16,-16,0};
  for(unsigned i=0;i<sizeof(gaps)/sizeof(*gaps);++i){int x=px+gaps[i];
    if(x-12<view.x || x+12>=view.x+256)continue;
    for(int offset=-16;offset<=24;++offset){int floor=py+offset;
      if(floor-(int)height<view.y || floor>=view.y+224 || !landing_clear(x,floor,height,px,word(owner+8)))continue;
      *landing=(SafeLanding){(uint16_t)shape,(uint16_t)x,(uint16_t)(floor-(int8_t)rect[6]-rect[8]-1)};
      return true;
    }
  }
  return false;
}
static void land_player(unsigned seat,const SafeLanding *landing){
  scene_cancel_transients(seat);uint8_t *b=state.players[seat].body;
  putword(b+0x20,landing->shape);putword(b+5,landing->x);putword(b+8,landing->y);
  b[4]=b[7]=0;memcpy(b+0x22,b+5,2);memcpy(b+0x24,b+8,2);memset(b+0x1a,0,6);
  b[1]=2;b[2]=b[3]=0;b[0x0e]=1;b[0x26]=32;
  b[0x64]=b[0x6a]=b[0x6d]=b[0x6e]=b[0xa3]=0;b[0xa5]&=127;
  memset(b+0x36,0,6);
}
static bool rejoin_player(unsigned seat){
  if(seat>1 || !X3CoopRuntimeReady() || state.wipe || state.players[seat].life!=X3_COOP_ABSENT ||
     !(state.players[seat].body[0x27]&127) || state.players[seat].body[2]==12)return false;
  SafeLanding landing;X3CoopView view=X3CoopViewsForPlayer(g_ram,&state,state.anchor,false);
  if(!find_landing(seat,g_ram+X3_COOP_BODY,view,&landing))return false;
  X3CoopCapture(&state,g_ram);land_player(seat,&landing);
  if(!X3CoopRejoin(&state,seat))return false;
  restore_x_armor(seat);
  X3CoopViewsAdoptPlayer(g_ram,&state,state.anchor,false);X3CoopViewsCommitNative(g_ram,&state);
  return true;
}
static void shared_catchup_tick(void){
  if(!shared_camera_ready() || !participation_ready())return;
  const uint8_t *a=g_ram+X3_COOP_BODY,*b=state.players[state.anchor^1].body;
  if(shared_beyond_pit(a) || shared_beyond_pit(b))return;
  int dx=(int16_t)word(a+5)-(int16_t)word(b+5),dy=(int16_t)word(a+8)-(int16_t)word(b+8);
  if(dx<0)dx=-dx;if(dy<0)dy=-dy;
  unsigned axis=dx>SHARED_GAP_X?5:dy>SHARED_GAP_Y?8:0;
  if(!axis)return;
  unsigned lead=shared_leader(axis),lag=lead^1;
  const uint8_t *owner=lead==state.current?g_ram+X3_COOP_BODY:state.players[lead].body;
  X3CoopView view=X3CoopViewsForPlayer(g_ram,&state,lead,false);SafeLanding landing;
  if(!find_landing(lag,owner,view,&landing))return;
  /* Borrow the leader only if necessary to edit the stored lagging body.
   * Catch-up changes neither participant's life, health nor weapon choice. */
  X3CoopCapture(&state,g_ram);
  if(state.current==lag)X3CoopProject(&state,g_ram,lead);
  land_player(lag,&landing);restore_x_armor(lag);
  X3CoopProject(&state,g_ram,state.anchor);
}
#include "x3_coop_learner.inc"
bool X3CoopRuntimeRejoin(unsigned seat){
  if(seat>1 || !participation_ready())return false;
  bool zero=state.players[seat].character==X3_COOP_ZERO;
  if(stock_zero && zero && !X3CoopRuntimeZeroAvailable())return false;
  if(!rejoin_player(seat))return false;
  if(stock_zero && zero)g_ram[0x1fe1]|=1; /* Original04:8DF5, one call per stage. */
  return true;
}
static void stock_tick(void){
  if(!stock_zero || !X3CoopRuntimeReady() || state.wipe)return;
  settle_deaths();if(state.wipe)return;
  unsigned x=state.players[0].character==X3_COOP_X?0:1,zero=x^1;
  if(state.players[zero].life!=X3_COOP_ALIVE){stock_exit_pending=false;return;}
  if(g_ram[0x1f5d]&0x80)stock_exit_pending=true;
  if(!stock_exit_pending)return;
  learner_rescue=false;
  if(state.players[x].life==X3_COOP_ABSENT && !rejoin_player(x))return;
  X3CoopCapture(&state,g_ram);
  if(state.players[x].life==X3_COOP_ALIVE){
    if(state.anchor==zero)X3CoopViewsAdoptPlayer(g_ram,&state,x,view_scene());
    X3CoopWithdraw(&state,zero);scene_cancel_transients(zero);
    X3CoopProject(&state,g_ram,x);anchor_palette();
  }else{
    /* A fallen X cannot return for a restricted room. Neither participant
     * can now continue: use one normal team retry, without reviving X here. */
    state.players[zero].life=X3_COOP_ABSENT;scene_cancel_transients(zero);
    state.wipe=1;state.anchor=(uint8_t)x;scene_clear();
    X3CoopProjectRetired(&state,g_ram,x);g_ram[X3_COOP_BODY+0xb6]=0;
  }
  stock_exit_pending=false;
}
static void participation_tick(void){
  /* Count original world updates, not host presents. Both pads belong to the
   * common simulation, so selected views and online seats cannot change this
   * deterministic order. A successful request latches until Select releases. */
  bool ready=participation_ready();
  for(unsigned seat=0;seat<2;++seat){
    if(!(native_pad(seat)&0x2000)){participation_hold[seat]=0;continue;}
    if(!ready){participation_hold[seat]=PARTICIPATION_HOLD+1;continue;}
    uint8_t *held=&participation_hold[seat];
    if(*held<PARTICIPATION_HOLD)++*held;
    if(*held!=PARTICIPATION_HOLD)continue;
    if(state.players[seat].life==X3_COOP_ABSENT){
      /* Keep a held request pending while the partner crosses unsafe ground. */
      if(X3CoopRuntimeRejoin(seat))*held=PARTICIPATION_HOLD+1;
    }else{
      (void)X3CoopRuntimeWithdraw(seat);*held=PARTICIPATION_HOLD+1;
    }
  }
}
#include "x3_coop_intro.inc"
static bool scene_finished(void){
  if(intro_cinematic())return false;
  if(scene.kind==SCENE_INTRO_CAPTURE || scene.kind==SCENE_INTRO_ZERO || intro_constructing())return false;
  if(scene.kind==SCENE_INTRO_RETURN){
    for(unsigned a=0x1f25;a<=0x1f2b;++a)if(g_ram[a])return false;
    return !g_ram[0xcc8] && !g_ram[0x1f22] && !g_ram[0x1f45] && !g_ram[0x1f4f] &&
      !g_ram[0x1f63] && !g_ram[X3_COOP_BODY+0x6e] && !(g_ram[X3_COOP_BODY+0xa3]&128) &&
      g_ram[X3_COOP_BODY+0xb6]==(state.players[scene.owner].character==X3_COOP_ZERO?2:0) &&
      g_ram[X3_COOP_BODY+2]<0x1c;
  }
  if(state.players[scene.owner].life==X3_COOP_ALIVE && g_ram[X3_COOP_BODY+0x6e])return false;
  if(scene.kind==SCENE_DOOR)return !g_ram[0x1f28] && (!g_ram[scene.slot] || g_ram[scene.slot+1]>=10);
  /* Opening releases X to step into the pod. Acquisition retains the other
   * actor through13:C3FB's recorded input and any follow-on arm sequence. */
  if(g_ram[0x1f4f] || g_ram[0x1f45] || g_ram[0x1f63])return false;
  return !g_ram[scene.slot] || (scene.kind==SCENE_CAPSULE_OPEN?g_ram[scene.slot+2]==6:
      g_ram[scene.slot+2]==8 && g_ram[scene.slot+3]>=16);
}
static void scene_tick(void){
  if(!scene.phase || stage_pending())return;
  settle_deaths();
  if(state.wipe || scene.stage!=g_ram[0x1fae]){scene_clear();return;}
  if(state.players[scene.owner].life!=X3_COOP_ALIVE){
    if(scene.kind){
      /* The required X context finishes the existing native capsule task,
       * with projected zero HP and no player-controller/attack pass. The
       * survivor stays parked in its committed room until native cleanup. */
      /* Keep the survivor's modal input/history intact while parking its
       * physical position. Reinitializing its actions every tick erases
       * the very confirmation buttons the shared dialogue is waiting for. */
      scene_position(state.anchor,state.players[scene.owner].body);scene_install(state.anchor);
    }else{
    /* Continue the existing native task with the survivor. Transfer only its
     * forced position/action, retaining the survivor's character and HP. */
    unsigned survivor=state.anchor;const uint8_t *old=state.players[scene.owner].body;
    scene_cancel_transients(survivor);scene_place(survivor,old);
    uint8_t *b=state.players[survivor].body;unsigned phase=g_ram[scene.slot+1];
    b[0xa3]=(uint8_t)(0x80|(!g_ram[scene.slot] || phase>=10?0x1e:phase<=4?0x18:phase==6?0x1c:0x1e));
    b[0x6e]=(uint8_t)(phase==8?128:1);
    scene.owner=(uint8_t)survivor;scene.phase=2;scene.tick=0;
    scene_install(survivor);
    }
  }
  unsigned other=scene.owner^1;
  bool present=state.players[other].life==X3_COOP_ALIVE &&
      (state.players[other].body[0x27]&127) && state.players[other].body[2]!=12;
  if(scene.phase==1){if(++scene.tick>=8){scene.phase=2;scene.tick=0;}}
  if(scene.phase==2 && scene_finished()){
    if(scene.kind && state.players[scene.owner].life!=X3_COOP_ALIVE){
      X3CoopPlayer *p=&state.players[scene.owner];p->body[0x6e]=p->body[0xa3]=0;p->body[0xa5]&=127;
      if(p->life==X3_COOP_ABSENT)p->body[2]=p->body[3]=0;
    }
    if(!present){scene_release_views();scene_clear();return;}
    X3CoopCapture(&state,g_ram);scene_place(other,state.players[scene.owner].body);
    if(other==state.current)scene_install(other);
    scene.phase=3;scene.tick=0;
  }else if(scene.phase==3 && (!present || ++scene.tick>=8)){scene_release_views();scene_clear();}
}
static bool scene_rom(unsigned a,unsigned n,unsigned *value){
  size_t at=((a>>16)&127)*32768+(a&32767);
  if((a&65535)<32768 || at>g_snes->cart->romImageSize || n>g_snes->cart->romImageSize-at)return false;
  *value=0;for(unsigned i=0;i<n;++i)*value|=(unsigned)g_snes->cart->rom[at+i]<<(i*8);return true;
}
void X3CoopRuntimeScenePresentation(X3CoopState *copy){
  if(!X3CoopRuntimeSceneActive())return;
  if(intro_cinematic()){
    /* Original NPC/cinematic OAM owns this interval; retained participants
     * must not appear as additional playable bodies beside that scene. */
    for(unsigned seat=0;seat<2;++seat)copy->players[seat].body[0x0e]=0;
    return;
  }
  unsigned other=scene.owner^1;X3CoopPlayer *p=&copy->players[other];
  if(p->life!=X3_COOP_ALIVE)return;
  uint8_t *b=p->body;b[0x0e]=0;
  if(scene.phase==2)return;
  /* Native04:B967 animation records; groups00 and4A share the original
   * 7F departure/7E arrival sequences. Only the queued visual copy changes. */
  unsigned directory,offset,animation=scene.phase==1?0x7f:0x7e,elapsed=scene.tick;
  if(!scene_rom(0x3f8000+b[0x16]*3,3,&directory) ||
     !scene_rom(directory+animation*2,2,&offset))return;
  unsigned at=(directory&0xff0000)|((directory+offset)&65535);
  for(unsigned i=0;i<16;++i,at+=3){unsigned duration,pose;
    if(!scene_rom(at,1,&duration) || !duration || !scene_rom(at+2,1,&pose))return;
    if(elapsed<duration){b[0x17]=(uint8_t)(pose|128);b[0x0e]=1;
      X3CoopPresentationBodyDma(other,b,g_snes->cart->rom,g_snes->cart->romImageSize);return;}
    elapsed-=duration;
  }
}
static int support_slot(unsigned d) {
  if(d>=0xd18 && d<0x10d8 && !((d-0xd18)%64))return (int)((d-0xd18)/64);
  if(d>=0x1518 && d<0x1818 && !((d-0x1518)%48))return 15+(int)((d-0x1518)/48);
  return -1;
}
static void refill_retired_cleanup(unsigned d,unsigned seat){
  X3CoopPlayer *p=&state.players[seat];
  if(p->life==X3_COOP_ALIVE)return;
  /* Retired bodies are deliberately never captured: their projected HP is
   * zero. Keep only the original04:D130 action release for a later voluntary
   * return, without importing that zero HP or reviving a fallen participant. */
  p->body[0xa3]=g_ram[X3_COOP_BODY+0xa3];
  p->body[0xa5]=g_ram[X3_COOP_BODY+0xa5];
  p->body[0x6e]=g_ram[X3_COOP_BODY+0x6e];
  if(g_ram[d+10]==11 && (g_ram[0x1fd4]&g_ram[d+11])){
    /* Native01:E968 commits the heart before its two timed pulses. Retiring
     * cancels healing, but cannot undo that permanent acquisition. Match
     * native04:DBA6's capacity calculation from the shared collected mask. */
    unsigned hearts=g_ram[0x1fd4],maximum=16;
    for(;hearts;hearts>>=1)maximum+=2*(hearts&1);
    p->maximum_health=(uint8_t)maximum;
  }
}
/* Retail03:9639 and03:D777 both reject Zero. The one native Ride
 * Armor controller therefore borrows X, including empty-armor contact and
 * cleanup after his retirement. Its shared pool still advances once. */
static void vehicle_boundary(CpuState *cpu,uint32_t pc){
  unsigned at=pc&0x7fffff;
  if((at==0x00da4b || at==0x00da6e) && target.active && target.slot==0xcc8 && cpu->S==target.s){
    personal_view_end();target.active=0;settle_deaths();
    if(!state.wipe)X3CoopProject(&state,g_ram,state.anchor);
    return;
  }
  if((at!=0x00da48 && at!=0x00da6b) || !X3CoopRuntimeReady() || state.wipe || target.active || !g_ram[0xcc8] ||
      g_ram[0xcd2] || controller.pass || terrain.pass || contact.native.pass ||
      objects.native.pass || draw.pass || solid.native.pass || state.current!=state.anchor)return;
  unsigned x=state.players[0].character==X3_COOP_X?0:1;
  if(x!=state.current){
    target=(WorldTarget){0xcc8,cpu->S,1,(uint8_t)x};personal_view_begin(x);
    if(state.players[x].life==X3_COOP_ALIVE)X3CoopProject(&state,g_ram,x);
    else X3CoopProjectRetired(&state,g_ram,x);
  }
  /* Native00:E49E maps only the canonical body before this world call.
   * Project the pilot's current port now, leaving previous_actions for the
   * one later actor-controller commit. Do not repeat its other side effects. */
  if(!(scene.kind==SCENE_CAPSULE_AWARD && x==scene.owner && g_ram[0x1f45])){
    unsigned held=state.players[x].life==X3_COOP_ALIVE && !scene_parked(x)?
        X3CoopActions(RtlGetPadState(x),g_ram+0xffc0):0;
    unsigned previous=state.players[x].previous_actions;
    putword(g_ram+X3_COOP_BODY+0x36,held);putword(g_ram+X3_COOP_BODY+0x38,previous);
    putword(g_ram+X3_COOP_BODY+0x3a,held&~previous);
  }
}
static void vehicle_cull(CpuState *cpu,uint32_t pc){
  (void)pc;
  if(!X3CoopRuntimeReady() || state.wipe || cpu->D!=0xcc8 || !g_ram[0xcc8] || g_ram[0xcd2])return;
  /* Complete the original03:A5EC comparison against a living view. Retain
   * its exact wraparound arithmetic and half-open rectangle, never the gap
   * between distant views. No camera or guest time is rewritten. */
  for(unsigned i=0;i<2;++i){unsigned seat=state.anchor^i;
    const uint8_t *b=seat==state.current?g_ram+X3_COOP_BODY:state.players[seat].body;
    if(state.players[seat].life!=X3_COOP_ALIVE || !(b[0x27]&127))continue;
    X3CoopView view=X3CoopViewsForPlayer(g_ram,&state,seat,false);
    uint16_t dx=(uint16_t)(word(g_ram+0xccd)-view.x+128);
    uint16_t raw=(uint16_t)(word(g_ram+0xcd0)-view.y),dy=(uint16_t)(raw+96);
    if(dx>=512 || dy>=416)continue;
    cpu->A=dy;cpu->_flag_C=0;cpu->_flag_Z=0;
    cpu->_flag_N=((uint16_t)(dy-416)&32768)!=0;
    cpu->_flag_V=((~(raw^96)&(raw^dy))&32768)!=0;
    cpu_mirrors_to_p(cpu);return;
  }
}
static unsigned vehicle_menu_owner(void){
  if(!(g_ram[0x1f3f]&128))return 2;
  for(unsigned seat=0;seat<2;++seat){
    const uint8_t *b=seat==state.current?g_ram+X3_COOP_BODY:state.players[seat].body;
    if(state.players[seat].life==X3_COOP_ALIVE && state.players[seat].character==X3_COOP_X &&
        (b[2]==0x66 || b[2]==0x2c))return seat;
  }
  return 2;
}
static void target_boundary(CpuState *cpu,uint32_t pc) {
  if(!X3CoopRuntimeReady())return;
  unsigned at=pc&0xffff,d=cpu->D;
  if(at==0xdd1a || at==0xdcb3 || at==0xdafa) {
    bool capsule=native_capsule(d),intro=intro_capture_actor(d),x_only=x_pickup(d) || capsule || intro;
    /* Hostile constructors choose aim before populating native HP. */
    bool constructing_hostile=at==0xdcb3 && hostile_projectile(d) && !g_ram[d+1];
    bool pickup=at==0xdafa && (x_only || shared_pickup(d));
    bool door=at==0xdafa && native_door(d) && g_ram[d+1]<=2;
    unsigned station_owner=enemy_actor(d) && g_ram[d+10]==0x2f?vehicle_menu_owner():2;
    bool pending=station_owner<2 || (pickup && refill.active && refill.slot==d) || ((capsule || intro) && !g_ram[d+1]) ||
        (scene.phase && scene.kind && state.players[scene.owner].life!=X3_COOP_ALIVE);
    if(state.wipe || target.active || (!pickup && !door && !enemy_actor(d)) || !g_ram[d] ||
       (!pickup && !door && !capsule && !intro && !pending && !constructing_hostile && !(g_ram[d+0x27]&127)) ||
       (((g_ram[0x1f22] && !vehicle_mode()) || g_ram[0x1f24]) && !pending) || controller.pass || terrain.pass ||
       contact.native.pass || objects.native.pass || draw.pass || solid.native.pass || state.current!=state.anchor)return;
    /* Keep native X eligibility for weapon energy and persistent heart/tank
     * acquisitions, including when Zero is nearer or owns the shared world. */
    unsigned seat=station_owner<2?station_owner:(capsule || intro)?(state.players[0].character==X3_COOP_X?0:1):scene.phase?scene.owner:pending?refill.seat:pickup && x_only?
        (state.players[0].character==X3_COOP_X?0:1):
        X3CoopNearest(&state,g_ram,word(g_ram+d+5),word(g_ram+d+8));
    bool retired=state.players[seat].life!=X3_COOP_ALIVE;
    if(retired && !pending)return;
    if(!pickup && !door && !capsule && !intro && seat==state.anchor)return;
    target=(WorldTarget){(uint16_t)d,cpu->S,1,(uint8_t)seat};
    if(retired)X3CoopProjectRetired(&state,g_ram,seat);
    else X3CoopProject(&state,g_ram,seat);
    return;
  }
  /* Native item/enemy/hostile-shot continuations follow the actual RTS;
   * the original JSR frame has been consumed. */
  if(target.active && d==target.slot && cpu->S==(uint16_t)(target.s+2)) {
    intro_capture_begin(d);
    if(scene.phase && scene.kind && target.seat==scene.owner && state.players[scene.owner].life!=X3_COOP_ALIVE &&
       enemy_actor(d) && (g_ram[d+10]==0x4d || g_ram[d+10]==0x4e || g_ram[d+10]==0x5c)){
      /* Script-owned armor/palette/action writes survive retirement, while
       * the temporary zero HP must never heal, revive or erase stored HP. */
      X3CoopPlayer *p=&state.players[scene.owner];uint8_t hp=p->body[0x27];
      memcpy(p->body,g_ram+X3_COOP_BODY,sizeof(p->body));p->body[0x27]=hp;
      memcpy(p->auxiliaries,g_ram+X3_COOP_AUX,sizeof(p->auxiliaries));
      memcpy(p->palette,g_ram+X3_COOP_PALETTE,sizeof(p->palette));
      p->native_flags=g_ram[0x1fff];
    }
    if(item_actor(d)){
      if((x_pickup(d) || g_ram[d+10]==2) && g_ram[d+2]==6 && (g_ram[d]&128) && g_ram[0x1f28])
        refill=(PickupRefill){(uint16_t)d,target.seat,1};
      else if(refill.active && refill.slot==d){
        refill_retired_cleanup(d,refill.seat);memset(&refill,0,sizeof(refill));
      }
    }
    target.active=0;
    settle_deaths();
    if(!state.wipe)X3CoopProject(&state,g_ram,state.anchor);
  }
}
static void target_query(CpuState *cpu,uint32_t pc) {
  if(!X3CoopRuntimeReady() || state.wipe || !enemy_actor(cpu->D) || !g_ram[cpu->D])return;
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
/* The checkpoint task reads the body in the shared window. Its comparison
 * may admit either living actor in the task's actual native culling region;
 * a distant actor in another view must not commit this checkpoint. */
static void checkpoint_query(CpuState *cpu,uint32_t pc){
  (void)pc;unsigned d=cpu->D;
  if(!X3CoopRuntimeReady() || state.wipe || d<0x1d18 || d>=0x1e18 ||
     (d-0x1d18)%16 || !g_ram[d] || g_ram[d+10]!=11)return;
  unsigned parameter=g_ram[d+11],axis=(parameter&64)?8:5,chosen=2;
  uint16_t value=cpu->A,position=0;
  for(unsigned i=0;i<2;++i){unsigned seat=state.current^i;
    const uint8_t *body=seat==state.current?g_ram+X3_COOP_BODY:state.players[seat].body;
    if(state.players[seat].life!=X3_COOP_ALIVE || !(body[0x27]&127))continue;
    X3CoopView view=X3CoopViewsForPlayer(g_ram,&state,seat,false);
    /* Exact02:D58A region: [-64,320) by [-64,288). */
    if((uint16_t)(word(g_ram+d+5)-view.x+64)>=384 ||
       (uint16_t)(word(g_ram+d+8)-view.y+64)>=352)continue;
    uint16_t candidate=(uint16_t)word(body+axis);bool carry=value>=candidate;
    if(chosen==2){chosen=seat;position=candidate;}
    if(carry==((parameter&128)!=0)){chosen=seat;position=candidate;break;}
  }
  if(chosen<2){cpu->_flag_C=value>=position;cpu->_flag_Z=value==position;
    cpu->_flag_N=((uint16_t)(value-position)&32768)!=0;}
  else cpu->_flag_C=(parameter&128)==0;
  cpu_mirrors_to_p(cpu);
}
static void world_gravity_query(CpuState *cpu,uint32_t pc){
  if(!X3CoopRuntimeReady())return;
  /* Native02:DDD3 polls X's first Gravity Well slot for shared reactions.
   * A nearby Zero may be projected for the querying NPC; he does not own
   * that weapon. Replace the completed loads, retaining every original
   * instruction, branch, stack operation and guest cycle. */
  unsigned x=state.players[0].character==X3_COOP_X?0:1;
  const uint8_t *shots=x==state.current?g_ram+X3_COOP_SHOTS:state.players[x].shots;
  unsigned offset=(pc&65535)==0xddd9?10:(pc&65535)==0xdde4?14:(pc&65535)==0xdde9?0:0x30;
  unsigned value=state.players[x].life==X3_COOP_ALIVE?shots[offset]:0;
  cpu->A=(uint16_t)((cpu->A&0xff00)|value);
  cpu->_flag_Z=value==0;cpu->_flag_N=(value&128)!=0;cpu_mirrors_to_p(cpu);
}
static void shared_pickup_character(CpuState *cpu,uint32_t pc){
  if(!enabled || !state.initialized || !target.active || target.slot!=cpu->D ||
     !shared_pickup(cpu->D) ||
     state.players[state.current].life!=X3_COOP_ALIVE)return;
  if(((pc&65535)==0xd837)!=(g_ram[cpu->D+10]==2))return;
  /* Full-campaign co-op shares native subtanks and team lives. Keep their
   * capacity, amount, timer and sounds; omit stock Zero-only exclusions. */
  cpu->A&=0xff00;cpu->_flag_Z=1;cpu->_flag_N=0;cpu_mirrors_to_p(cpu);
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|((pc+3)&65535));
}
static void full_campaign_door_sensor(CpuState *cpu,uint32_t pc){
  if(!X3CoopRuntimeReady() || !item_actor(cpu->D) || g_ram[cpu->D+10]!=10)return;
  /* Complete the borrowed native item update first. The balanced world
   * boundary applies stock withdrawal without invalidating its continuation. */
  if(stock_zero && g_ram[X3_COOP_BODY+0xb6])stock_exit_pending=true;
  /* Native01:CF02 asks Zero to exchange back to X before restricted rooms.
   * With both native actors enrolled,00:A30C would reset the shared camera
   * and interrupt the already-admitted door. Full-campaign co-op retains
   * Zero; the separate exit sensor still clears the native encounter lock. */
  cpu->A&=0xff00;cpu->_flag_Z=1;cpu->_flag_N=0;cpu_mirrors_to_p(cpu);
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xcf05);
}
static void solid_boundary(CpuState *cpu,uint32_t pc) {
  if(!X3CoopRuntimeReady())return;
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
    if(state.players[state.current].life!=X3_COOP_ALIVE){
      if(at==0xc07f)interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xc09d);
      return; /* BFAE retains its original zero-HP rejection. */
    }
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
  if(!X3CoopRuntimeReady())return;
  pc&=0x7fffff;
  if(X3CoopNativePauseActive(g_ram)){
    if(pc==0x038f70 && owned_object((cpu->D+cpu->X)&32767))cpu->X&=32767;
    return;
  }
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
  if(!X3CoopRuntimeReady() || X3CoopNativePauseActive(g_ram))return;
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
    else {int i=effect_index(slot);if(i>=0)owner=effect_owner[i];}
    X3CoopPresentationSubmit(&state,g_ram,slot,owner);
  }
}
static void door_boundary(CpuState *cpu,uint32_t pc){
  unsigned at=pc&65535,d=cpu->D;NativePass *p=&contact.native;
  if(!X3CoopRuntimeReady() || state.wipe)return;
  if(at==0xe0ae){
    /* Story Zero crosses the original rescue-room door while X remains
     * captured. Keep that native door inside the existing intro ownership;
     * a second co-op scene would release or wait for the captive actor. */
    if(scene.phase && scene.kind==SCENE_INTRO_ZERO)return;
    if(scene.phase && scene.slot!=d){interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xe0f0);return;}
      /* A single participant still needs scene ownership through the native
       * forced walk/intro. Only the optional contact retry needs a partner. */
      if(!p->pass && target.active && target.slot==d && native_door(d) &&
         state.players[state.current].life==X3_COOP_ALIVE &&
       !controller.pass && !terrain.pass && !objects.native.pass && !draw.pass && !solid.native.pass){
      p->entry=regs(cpu);contact.pc=0xe0ae;contact.seat=state.current;p->pass=1;
      memcpy(contact.entry_scratch,g_ram,24);
    }
    return;
  }
  if((at!=0xe0e0 && at!=0xe0f0 && at!=0xe0f1) || !p->pass || contact.pc!=0xe0ae ||
     cpu->S!=p->entry.s || d!=p->entry.d)return;
  bool hit=at==0xe0f1;
  if(p->pass==1 && (scene.both?hit:!hit)){
    const X3CoopPlayer *other=&state.players[contact.seat^1];
    if(other->life==X3_COOP_ALIVE && (other->body[0x27]&127) && other->body[2]!=0x0c){
      p->result=regs(cpu);memcpy(contact.result_scratch,g_ram,24);
      X3CoopProject(&state,g_ram,contact.seat^1);restore(cpu,&p->entry);
      memcpy(g_ram,contact.entry_scratch,24);p->pass=2;
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xe0ae);return;
    }
  }
  if(p->pass==2 && scene.both){
    X3CoopProject(&state,g_ram,contact.seat);restore(cpu,&p->result);
    memcpy(g_ram,contact.result_scratch,24);
    interp_bridge_pre_opcode_redirect((pc&0xff0000)|(hit?0xe0f1:0xe0f0));
  }
  if(hit){
    /* This is before the original door mutates locks/camera/body state.
     * Adopt the entrant's old independent view while the old anchor is still
     * identifiable, then let the native scene own that body and world once. */
    X3CoopViewsAdoptPlayer(g_ram,&state,state.current,view_scene());
    state.anchor=state.current;target.seat=state.current;anchor_palette();
    scene_begin(d,SCENE_DOOR);
  }else if(p->pass==2 && !scene.both){
    X3CoopProject(&state,g_ram,contact.seat);restore(cpu,&p->result);
    memcpy(g_ram,contact.result_scratch,24);
    interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xe0f0);
  }
  p->pass=0;
}
static void capsule_boundary(CpuState *cpu,uint32_t pc){
  if(!X3CoopRuntimeReady() || !native_capsule(cpu->D))return;
  if(pc!=0x13c5bd && (state.wipe || state.players[state.current].life!=X3_COOP_ALIVE ||
     !(g_ram[X3_COOP_BODY+0x27]&127))){
    interp_bridge_pre_opcode_redirect(pc==0x13c164?0x13c1b4:0x13c25c);return;
  }
  if(state.wipe ||
     !target.active || target.slot!=cpu->D || state.players[state.current].character!=X3_COOP_X)return;
  if(pc==0x13c5bd){
    unsigned index;
    /* The optional helmet survey and arm demonstration need a living controller. Their armor
     * has already been awarded; retire through the native completed return
     * instead of creating new demonstration locks for a fallen/absent X. */
    if(state.players[scene.owner].life!=X3_COOP_ALIVE && scene_rom(0x06ccfa+g_ram[0x1fae],1,&index) && index<=1)
      interp_bridge_pre_opcode_redirect(0x13c5da);
    return;
  }
  if(scene.phase && scene.slot!=cpu->D){
    interp_bridge_pre_opcode_redirect(pc==0x13c164?0x13c1b4:0x13c25c);return;
  }
  X3CoopViewsAdoptPlayer(g_ram,&state,state.current,view_scene());
  state.anchor=state.current;target.seat=state.current;anchor_palette();
  scene_begin(cpu->D,pc==0x13c164?SCENE_CAPSULE_OPEN:SCENE_CAPSULE_AWARD);
}
static void contact_boundary(CpuState *cpu,uint32_t pc) {
  unsigned at=pc&0xffff,d=cpu->D;
  NativePass *p=&contact.native;
  if(!X3CoopRuntimeReady() || state.wipe)return;
  if(p->pass && contact.pc==0xe0ae)return;
  if(at==0xcc5c){
    if(!p->pass && target.active && target.slot==d && shared_pickup(d) &&
       cpu->X==X3_COOP_BODY && partner_ready() && !refill.active && !controller.pass &&
       !terrain.pass && !objects.native.pass && !draw.pass && !solid.native.pass){
      p->entry=regs(cpu);contact.pc=0xcc5c;contact.seat=state.current;p->pass=1;
      memcpy(contact.entry_scratch,g_ram,24);
    }
    return;
  }
  if(p->pass && contact.pc==0xcc5c){
    if((at!=0xcc63 && at!=0xcc6b && at!=0xcd64) || cpu->S!=p->entry.s || d!=p->entry.d)return;
    if(p->pass==1){
      if(cpu->_flag_C){p->pass=0;return;}
      const X3CoopPlayer *other=&state.players[contact.seat^1];
      if(other->life!=X3_COOP_ALIVE || !(other->body[0x27]&127) || other->body[2]==0x0c){p->pass=0;return;}
      p->result=regs(cpu);memcpy(contact.result_scratch,g_ram,24);
      if(!X3CoopProject(&state,g_ram,contact.seat^1)){p->pass=0;return;}
      restore(cpu,&p->entry);memcpy(g_ram,contact.entry_scratch,24);p->pass=2;
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xcc5c);return;
    }
    if(cpu->_flag_C){
      /* Keep the actual collector projected through the remainder of this
       * single native item update, then latch any resulting refill. */
      target.seat=state.current;
    }else{
      X3CoopProject(&state,g_ram,contact.seat);
      restore(cpu,&p->result);memcpy(g_ram,contact.result_scratch,24);
    }
    p->pass=0;return;
  }
  if(at==0xcb31 || at==0xcb74) {
    /* Ordinary hostile actors only. Pickups, capsules, supports and scripts
     * retain their native owner until their specific contracts are mapped. */
    if(!enemy_actor(d) || native_capsule(d) || !g_ram[d] || !partner_ready())return;
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
    /* Buffalo ice (hostile class20,08:9D9C..9DAB) remains a body-contact
     * hazard after CB31 in both its moving and grounded phases. Other
     * single-use hostile hits stop here. Player-shot scans are consumed by
     * hit, shield or reflection; only CBC3 is a true miss. */
    if((contact.pc==0xcb31 && hostile_projectile(d) && g_ram[d+10]!=0x20 && hit) ||
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
  if(!X3CoopRuntimeReady() || state.wipe)return;
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
static void effect_boundary(CpuState *cpu,uint32_t pc){
  if(!X3CoopRuntimeReady())return;
  unsigned at=pc&0x7fffff;
  if(at==0x02d8c1){
    int i=effect_index(cpu->X);if(i<0)return;
    /* D859 and the other allocators share this successful return. Reset on
     * every reuse, including an unowned world allocation in a former slot. */
    bool personal=cpu->D==X3_COOP_BODY || owned_object(cpu->D) ||
        controller.pass || terrain.pass || objects.native.pass || draw.pass ||
        (contact.native.pass && contact.pc==0xcb74) ||
        (target.active && effect_index(target.slot)>=0 && effect_owner[effect_index(target.slot)]);
    effect_owner[i]=(uint8_t)(personal && !state.wipe &&
        state.players[state.current].life==X3_COOP_ALIVE?state.current+1:0);
    return;
  }
  unsigned d=cpu->D;int i=effect_index(d);if(i<0)return;
  if(at==0x00db0f || at==0x00db34){
    if(!g_ram[d])effect_owner[i]=0;
    if(effect_owner[i] && state.players[effect_owner[i]-1].life!=X3_COOP_ALIVE){
      memset(g_ram+d,0,EFFECT_STRIDE);effect_owner[i]=0;
    }
    return;
  }
  if(at==0x00db1b || at==0x00db42){
    if(!effect_owner[i] || state.wipe || target.active || state.current!=state.anchor ||
       controller.pass || terrain.pass || contact.native.pass || objects.native.pass ||
       draw.pass || solid.native.pass)return;
    unsigned seat=effect_owner[i]-1;
    target=(WorldTarget){(uint16_t)d,(uint16_t)(cpu->S-2),1,(uint8_t)seat};
    X3CoopProject(&state,g_ram,seat);
    return;
  }
  if((at==0x00db1e || at==0x00db45) && target.active && target.slot==d &&
     cpu->S==(uint16_t)(target.s+2)){
    X3CoopCapture(&state,g_ram);target.active=0;
    if(!g_ram[d])effect_owner[i]=0;
    X3CoopProject(&state,g_ram,state.anchor);
  }
}
static void initialize_zero(void){
  unsigned x=state.players[0].character==X3_COOP_X?0:1;
  X3CoopPlayer *p=&state.players[x^1];
  p->body[0xb6]=2; /* Native playable Zero selector, not an imported actor. */
  p->native_flags=2;
  p->body[0x16]=0x4a;
  const uint8_t *rom=g_snes->cart->rom;
  putword(p->body+0x31,word(rom+0x3326a)); /* Data bank $86:B26A. */
  p->body[0x17]|=0x80;
  p->body[0x57]=p->body[0x58]=p->body[0x5a]=p->body[0x5b]=0;
  p->body[0xb7]=0;
  /* Native D0/D2 lists select Zero's body and buster/charge ramps. */
  if(g_snes->cart->romImageSize>=0x635c0){
    memcpy(p->palette,rom+0x635a0,32);memcpy(p->palette+32,rom+0x63580,32);
  }
}
static void stage_begin(void){
  /* The cartridge has discarded the old world body and is about to build X.
   * It is not the previous anchor's body: capturing it could turn a living
   * Zero into X or enroll a fallen actor during a fresh stage/checkpoint. */
  scene_clear();
  for(unsigned seat=0;seat<2;++seat)scene_cancel_transients(seat);
  scene.slot=0;scene.owner=state.anchor;scene.phase=2;scene.stage=g_ram[0x1fae];scene.kind=SCENE_STAGE_ENTRY;
  memset(&refill,0,sizeof(refill));memset(riders,0,sizeof(riders));
  memset(&view_spawn,0,sizeof(view_spawn));memset(&view_bounds,0,sizeof(view_bounds));
  memset(participation_hold,0,sizeof(participation_hold));
  stock_exit_pending=false;
  learner_rescue=false;
  X3CoopViewsReset();X3CoopPresentationReset();
}
static void stock_start(const X3CoopPlayer *fresh_x){
  if(!stock_zero)return;
  unsigned x=state.players[0].character==X3_COOP_X?0:1,zero=x^1;
  /* Stock stages start with X. A living withdrawn X may be recalled, but a
   * fallen X still needs an actual team retry. Calling Zero stays explicit. */
  if(state.players[zero].life==X3_COOP_ALIVE)state.players[zero].life=X3_COOP_ABSENT;
  if(fresh_x && state.players[x].life==X3_COOP_ABSENT){
    uint8_t hp=state.players[x].body[0x27],max=state.players[x].maximum_health;
    state.players[x]=*fresh_x;state.players[x].body[0x27]=hp;state.players[x].maximum_health=max;
  }
  state.anchor=(uint8_t)x;
  state.wipe=state.players[x].life!=X3_COOP_ALIVE;
}
static void stage_bind(void){
  X3CoopState previous=state;
  X3CoopInit(&state,g_ram,previous.players[0].character);initialize_zero();
  unsigned x=state.players[0].character==X3_COOP_X?0:1;
  X3CoopPlayer fresh_x=state.players[x];
  for(unsigned seat=0;seat<2;++seat)
    if(previous.players[seat].life!=X3_COOP_ALIVE)state.players[seat]=previous.players[seat];
  state.anchor=previous.anchor;
  stock_start(&fresh_x);
  if(state.wipe)X3CoopProjectRetired(&state,g_ram,state.anchor);
  else X3CoopProject(&state,g_ram,state.anchor);
  anchor_palette();scene_clear();
  X3CoopPresentationEnroll(&state,g_snes->ppu->vram,g_snes->cart->rom,g_snes->cart->romImageSize);
}
static void boundary(CpuState *cpu,uint32_t pc) {
  if(!enabled || cpu->D!=X3_COOP_BODY || cpu->emulation)return;
  if(intro_cinematic())return;
  pc&=0x7fffff;
  if(pc==0x048008 && state.initialized && !state.wipe && !controller.pass &&
     !g_ram[X3_COOP_BODY+1] && !stage_pending()){
    /* Scripted reconstruction/exchange calls are not a new gameplay stage. */
    if(word(g_ram+cpu->S+4)!=0xd98e)return;
    stage_begin();
  }
  if(stage_pending()){
    /* Let the original arrival/scripted constructor finish before rebinding.
     * Only the normal gameplay caller is an enrollment boundary. */
    if(pc!=0x048008 || word(g_ram+cpu->S+4)!=0xd98e || scene.stage!=g_ram[0x1fae] || g_ram[0x1f22] ||
       g_ram[X3_COOP_BODY+1]!=2 || g_ram[X3_COOP_BODY+2]>0x16 ||
       !(g_ram[X3_COOP_BODY+0x27]&127) || g_ram[X3_COOP_BODY+0xb6])return;
    stage_bind();
  }
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
      if(terrain.pass==1 && !scene.phase && state.players[state.anchor^1].life==X3_COOP_ALIVE) {
        terrain.result=regs(cpu);terrain.pass=2;
        personal_view_begin(state.anchor^1);
        X3CoopProject(&state,g_ram,state.anchor^1);
        restore(cpu,&terrain.entry);
        interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|0xb591);
      } else {
        if(terrain.pass==2) {
          personal_view_end();
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
      initialize_zero();
      stock_start(NULL);
      X3CoopPresentationEnroll(&state,g_snes->ppu->vram,g_snes->cart->rom,g_snes->cart->romImageSize);
      fprintf(stderr,"[x3-coop] native actor qualification enrolled X and Zero\n");
    }
    if(controller.pass){
      if(scene_parked(state.current) ||
         (controller.pass==2 && refill.active && g_ram[0x1f2b] && state.current!=refill.seat))
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
    if(scene_parked(state.current) || (refill.active && g_ram[0x1f2b] && state.current!=refill.seat))
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
      personal_view_begin(state.anchor^1);
      X3CoopProject(&state,g_ram,state.anchor^1);
      restore(cpu,&controller.entry);controller.pass=2;
      input(state.current);
      interp_bridge_pre_opcode_redirect(((unsigned)cpu->PB<<16)|0x8008);
    } else {
      personal_view_end();
      X3CoopProject(&state,g_ram,state.anchor);
      restore(cpu,&controller.result);controller.pass=0;
    }
  }
}
#if SNESRECOMP_TRACE
/* Native two-peer death gate: bypass travel using the original stage request,
 * then place bodies above authored spikes. Native collision, lifecycle, fades,
 * resource debit and retry decide every result. The existing frame serial
 * makes these prerequisites replayable with ordinary rollback snapshots. */
static void net_world_prerequisites(uint32_t serial){
  const char *qa=getenv("SNESRECOMP_X3_NET_QA"),*world=getenv("SNESRECOMP_X3_NET_QA_WORLD");
  if(!qa || strcmp(qa,"1") || !world || (strcmp(world,"death") && strcmp(world,"checkpoint") && strcmp(world,"door") && strcmp(world,"boss")))return;
  bool checkpoint=!strcmp(world,"checkpoint"),door=!strcmp(world,"door"),boss=!strcmp(world,"boss");
  if(serial==3900){
    g_ram[0x1fae]=door?1:boss?2:5;g_ram[0x1faf]=g_ram[0x1fb3]=0;
    g_ram[0x1fb5]=g_ram[0x1fb6]=0;g_ram[0x1ebf]=1;
    g_ram[0xd1]=2;g_ram[0xd2]=4;g_ram[0xd3]=0;
    if(boss)g_ram[0x1fd3]=1; /* Prior intro clear is a prerequisite, not the Buffalo reward under test. */
    fprintf(stderr,"[x3-net-world-prerequisite] serial=%u native-stage-request=%u\n",serial,g_ram[0x1fae]);
  }
  if(boss){
    /* Match the established native checkpoint2 entrance fixture. It bypasses
     * earlier travel; the guest still admits the actual final gate and boss. */
    if(serial<5000 && g_ram[0x1fae]==2 && g_ram[0xd1]==4 && !g_ram[0xd2])g_ram[0x1fb5]=2;
    if(serial>=5400 && serial<6200 && g_ram[0x1fae]==2 && X3CoopRuntimeReady() && !scene.phase &&
       !controller.pass && !terrain.pass && !contact.native.pass && !target.active &&
       state.players[1].life==X3_COOP_ALIVE && (state.players[1].body[0x27]&127)){
      unsigned d=0;for(unsigned slot=0xd18;slot<0x10d8;slot+=64)if(g_ram[slot] && g_ram[slot+10]==0x52 && g_ram[slot+1]==2 && (g_ram[slot+0x27]&127))d=slot;
      if(d){if((g_ram[d+0x27]&127)>1)fprintf(stderr,"[x3-net-boss-ready] serial=%u slot=%u primary=%u hp=%u\n",serial,d,g_ram[d+1],g_ram[d+0x27]&127);g_ram[d+0x27]=1;
        if(!(serial%24)){
          X3CoopCapture(&state,g_ram);uint8_t *b=state.players[1].body;
          unsigned x=word(g_ram+d+5)+72,y=word(g_ram+d+8)+14;
          putword(b+5,x);putword(b+0x22,x);putword(b+8,y);putword(b+0x24,y);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);
          for(unsigned seat=0;seat<2;++seat)state.players[seat].body[0x88]=255;
          memcpy(g_ram+X3_COOP_BODY,state.players[state.current].body,X3_COOP_BODY_BYTES);
          fprintf(stderr,"[x3-net-world-prerequisite] serial=%u actual-boss-HP=1 guest-attack-position=%u,%u\n",serial,x,y);
        }
      }
    }
    return;
  }
  if(checkpoint || door){
    if(serial!=5400 || !X3CoopRuntimeReady() || scene.phase || g_ram[0x1fae]!=(door?1:5))return;
    X3CoopCapture(&state,g_ram);X3CoopProject(&state,g_ram,0);state.anchor=0;
    unsigned px=door?2544:2908,py=door?1435:1112;
    uint8_t *b=state.players[1].body;putword(b+5,px);putword(b+0x22,px);putword(b+8,py);putword(b+0x24,py);
    b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x88]=255;
    state.players[0].body[0x88]=g_ram[X3_COOP_BODY+0x88]=255;
    X3CoopViewState views=*X3CoopViewsState();views.independent=views.initialized=1;views.stage=door?1:5;views.valid=views.camera_valid=3;
    unsigned cy=door?1280:1024;views.camera[1][0]=door?2416:2816;views.camera[1][1]=(uint16_t)cy;views.limits[1][0]=views.current[1][0]=0;views.limits[1][1]=views.current[1][1]=8192;
    views.limits[1][2]=views.current[1][2]=views.limits[1][3]=views.current[1][3]=(uint16_t)cy;X3CoopViewsSetState(&views);
    fprintf(stderr,"[x3-net-world-prerequisite] serial=%u non-anchor-approach=%s\n",serial,door?"door":"checkpoint");return;
  }
  if((serial!=5400 && serial!=5600) || !X3CoopRuntimeReady() || scene.phase || g_ram[0x1fae]!=5)return;
  X3CoopCapture(&state,g_ram);
  for(unsigned seat=0;seat<2;++seat){
    if(serial==5600 && seat!=0)continue;
    uint8_t *b=state.players[seat].body;bool hazard=serial==5600 || seat==1;
    unsigned x=hazard?1680:1568,y=state.players[seat].character==X3_COOP_ZERO?(hazard?1392:1367):(hazard?1400:1375);
    putword(b+5,x);putword(b+0x22,x);putword(b+8,y);putword(b+0x24,y);
    b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);
  }
  memcpy(g_ram+X3_COOP_BODY,state.players[state.current].body,X3_COOP_BODY_BYTES);
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=5;views.valid=views.camera_valid=3;
  for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=1456;views.camera[seat][1]=1280;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=views.limits[seat][3]=views.current[seat][3]=1280;}
  X3CoopViewsSetState(&views);putword(g_ram+0x1e5d,1456);putword(g_ram+0x1e7a,1456);
  putword(g_ram+0x1e66,0);putword(g_ram+0x1e68,8192);putword(g_ram+0x1e6e,0);putword(g_ram+0x1e70,8192);
  const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
  for(unsigned i=0;i<6;++i)putword(g_ram+vertical[i],1280);
  fprintf(stderr,"[x3-net-world-prerequisite] serial=%u spike-approach=%u\n",serial,serial==5400?1:0);
}
#endif
#include "x3_coop_menu_runtime.inc"
void X3CoopRuntimeEnable(bool value) {
  if(enabled && !value && !g_ram[0x1f63])primary_input_restore();
  enabled=value;
}
bool X3CoopRuntimeEnabled(void) {return enabled;}
const X3CoopState *X3CoopRuntimeState(void) {return &state;}
void X3CoopRuntimeReset(void) {
  memset(&menu,0,sizeof(menu));
  learner_rescue=false;
  memset(&state,0,sizeof(state));memset(&controller,0,sizeof(controller));
  memset(participation_hold,0,sizeof(participation_hold));
  stock_exit_pending=false;
  memset(&terrain,0,sizeof(terrain));
  memset(&contact,0,sizeof(contact));memset(&objects,0,sizeof(objects));
  memset(&draw,0,sizeof(draw));
  memset(&target,0,sizeof(target));memset(effect_owner,0,sizeof(effect_owner));
  memset(&refill,0,sizeof(refill));
  scene_clear();
  memset(&solid,0,sizeof(solid));memset(riders,0,sizeof(riders));
  memset(&view_spawn,0,sizeof(view_spawn));X3CoopViewsReset();
  memset(&view_bounds,0,sizeof(view_bounds));
  X3CoopPresentationReset();
}
void X3CoopRuntimeSetP1Character(unsigned character) {
  if(!state.initialized && character<=X3_COOP_ZERO)p1_character=character;
}
void X3CoopRuntimeSetDoorPolicy(bool both_present){if(!state.initialized)scene.both=both_present;}
void X3CoopRuntimeSetStockZero(bool stock){if(!state.initialized)stock_zero=stock;}
bool X3CoopRuntimeStockZero(void){return stock_zero;}
void X3CoopRuntimeSetLearner(bool value){if(!state.initialized)learner_enabled=value;}
bool X3CoopRuntimeLearner(void){return enabled && learner_enabled;}
void X3CoopRuntimeFrame(void) {
  if(!enabled)return;
  X3CoopViewsSetRom(g_snes->cart->rom,g_snes->cart->romImageSize);
  static const uint32_t intro_pcs[]={0x03ac57,0x03ac8c,0x03ad70,0x03ad7f,0x03ad87,0x03aed9};
  if(!interp_bridge_add_pre_opcode_hook(0x00e49e,modal_input_boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x7e25ef,native_input_boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x7e2621,native_input_boundary))enabled=false;
  for(unsigned i=0;i<sizeof(intro_pcs)/sizeof(*intro_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(intro_pcs[i],intro_boundary))enabled=false;
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
      0x04cd69,0x04ce3b,0x04ced8,0x04cedb,0x04cfe3,0x04cc5c,0x04cc63,0x04cc6b,0x04cd64};
  static const uint32_t effect_pcs[]={0x02d8c1,0x00db0f,0x00db34,0x00db1b,0x00db42,0x00db1e,0x00db45};
  for(unsigned i=0;i<sizeof(effect_pcs)/sizeof(*effect_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(effect_pcs[i],effect_boundary))enabled=false;
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
  if(!interp_bridge_add_pre_opcode_hook(0x3dff43,checkpoint_query) ||
     !interp_bridge_add_pre_opcode_hook(0x3dff4a,checkpoint_query))enabled=false;
  static const uint32_t vehicle_pcs[]={0x00da48,0x00da4b,0x00da6b,0x00da6e};
  for(unsigned i=0;i<sizeof(vehicle_pcs)/sizeof(*vehicle_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(vehicle_pcs[i],vehicle_boundary))enabled=false;
  if(!interp_bridge_add_pre_opcode_hook(0x03a60a,vehicle_cull))enabled=false;
  static const uint32_t target_pcs[]={0x00dd1a,0x00dcb3,0x00dccf,0x00dcf3,0x00dc68,0x00dc8c,
      0x00dafa,0x00daaf,0x00dad3};
  for(unsigned i=0;i<sizeof(target_pcs)/sizeof(*target_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(target_pcs[i],target_boundary))enabled=false;
  static const uint32_t target_queries[]={0x02e142,0x02daa7,0x02dac6};
  for(unsigned i=0;i<sizeof(target_queries)/sizeof(*target_queries);++i)
    if(!interp_bridge_add_pre_opcode_hook(target_queries[i],target_query))enabled=false;
  static const uint32_t gravity_queries[]={0x02ddd9,0x02dde4,0x02dde9,0x02ddee};
  for(unsigned i=0;i<sizeof(gravity_queries)/sizeof(*gravity_queries);++i)
    if(!interp_bridge_add_pre_opcode_hook(gravity_queries[i],world_gravity_query))enabled=false;
  static const uint32_t pickup_characters[]={0x01d837,0x01d9cd,0x01da25,0x01da7c};
  for(unsigned i=0;i<sizeof(pickup_characters)/sizeof(*pickup_characters);++i)
    if(!interp_bridge_add_pre_opcode_hook(pickup_characters[i],shared_pickup_character))enabled=false;
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
  static const uint32_t shared_pcs[]={0x00e08b,0x00e096,0x00e0aa,0x00e0b5,0x00e2e2,0x00e2f1};
  for(unsigned i=0;i<sizeof(shared_pcs)/sizeof(*shared_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(shared_pcs[i],shared_camera_boundary))enabled=false;
  /* A current-build snapshot may change the profile between frames. The
   * callback and generated entry guard both consult the loaded value. */
  if(!interp_bridge_add_pre_opcode_hook(0x04cd6e,learner_damage_boundary) ||
      !interp_bridge_add_pre_opcode_hook(0x04d06a,learner_damage_boundary) ||
      !interp_bridge_add_pre_opcode_hook(0x04d06f,learner_damage_boundary))enabled=false;
  static const uint32_t presentation_pcs[]={0x038dd7,0x038df0,0x038f70};
  for(unsigned i=0;i<sizeof(presentation_pcs)/sizeof(*presentation_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(presentation_pcs[i],presentation_boundary))enabled=false;
  static const uint32_t door_pcs[]={0x01e0ae,0x01e0e0,0x01e0f0,0x01e0f1};
  for(unsigned i=0;i<sizeof(door_pcs)/sizeof(*door_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(door_pcs[i],door_boundary))enabled=false;
  if(!interp_bridge_add_pre_opcode_hook(0x01cf02,full_campaign_door_sensor))enabled=false;
  if(!interp_bridge_add_pre_opcode_hook(0x13c164,capsule_boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x13c250,capsule_boundary) ||
     !interp_bridge_add_pre_opcode_hook(0x13c5bd,capsule_boundary))enabled=false;
  static const uint32_t menu_pcs[]={0x00a2fa,0x00a36f,0x00c6d6,0x00c8d0,0x00c93e,
      0x00cf13,0x00cf1c,0x00ceac,0x00cb64,0x00cb67};
  for(unsigned i=0;i<sizeof(menu_pcs)/sizeof(*menu_pcs);++i)
    if(!interp_bridge_add_pre_opcode_hook(menu_pcs[i],menu_boundary))enabled=false;
}
void X3CoopRuntimeSave(SaveLoadInfo *sli) {
  uint8_t bytes[4096]={0};
  /* Arrival temporarily owns a newly constructed native X. Retained personal
   * contexts do not own that body until the ordinary gameplay rebind. */
  if(!stage_pending() && !intro_constructing() && !intro_cinematic())X3CoopCapture(&state,g_ram);
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
  uint8_t scene_bytes[16]={'X','3','S','2',0,0,scene.owner,scene.phase,scene.tick,scene.stage,scene.both,scene.kind};
  putword(scene_bytes+12,scene.raw[0]);putword(scene_bytes+14,scene.raw[1]);
  putword(scene_bytes+4,scene.slot);sli->func(sli,scene_bytes,sizeof(scene_bytes));
  uint8_t participation[8]={'X','3','J','2',participation_hold[0],participation_hold[1],
      (stock_zero?PROFILE_STOCK:0)|(learner_enabled?PROFILE_LEARNER:0),
      (stock_exit_pending?PROFILE_STOCK:0)|(learner_rescue?PROFILE_LEARNER:0)};
  sli->func(sli,participation,sizeof(participation));
  uint8_t views[X3_COOP_VIEW_STATE_BYTES];X3CoopViewsSave(views);
  sli->func(sli,views,sizeof(views));sli->func(sli,&view_spawn,sizeof(view_spawn));
  sli->func(sli,&view_bounds,sizeof(view_bounds));sli->func(sli,effect_owner,sizeof(effect_owner));
  X3CoopPresentationSave(sli);
  sli->func(sli,&solid,sizeof(solid));sli->func(sli,riders,sizeof(riders));
  uint8_t menu_bytes[MENU_BYTES];menu_encode(menu_bytes);sli->func(sli,menu_bytes,sizeof(menu_bytes));
}
/* Reuse the complete snapshot stream for peer confirmation. Its normal save
 * synchronizes the projected body; hashing must leave the live context intact. */
typedef struct CoopDigestStream {SaveLoadInfo io;uint32_t hash;} CoopDigestStream;
static void coop_digest_bytes(SaveLoadInfo *io,void *data,size_t size){
  CoopDigestStream *stream=(CoopDigestStream *)io;const uint8_t *bytes=data;
  for(size_t i=0;i<size;++i){stream->hash^=bytes[i];stream->hash*=16777619u;}
}
uint32_t X3CoopRuntimeDigest(void){
  X3CoopState prior;memcpy(&prior,&state,sizeof(prior));
  CoopDigestStream stream={{coop_digest_bytes,NULL},2166136261u};
  X3CoopRuntimeSave(&stream.io);memcpy(&state,&prior,sizeof(state));return stream.hash;
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
                         const WorldTarget *world,const PickupRefill *healing,const NativeScene *script,
                         const SolidPass *support,const uint8_t *rider_bits) {
  unsigned a=c->pass,b=t->pass,h=hit->native.pass,o=obj->native.pass,v=art->pass,q=support->native.pass;
  if(a>2 || b>2 || h>2 || o>2 || v>2 || q>2 || (a!=0)+(b!=0)+(h!=0)+(o!=0)+(v!=0)+(q!=0)>1)return false;
  for(unsigned i=0;i<SHARED_SUPPORTS;++i)
    if(rider_bits[i] && (rider_bits[i]&252)!=128)return false;
  if(world->active>1 || world->seat>1)return false;
  if(!s->initialized)return !(a || b || h || o || v || q || world->active);
  if(script->phase && script->kind==SCENE_STAGE_ENTRY){
    if(s->wipe || s->current!=s->anchor || script->owner!=s->anchor ||
       a || b || h || o || v || q || world->active || healing->active)return false;
    for(unsigned i=0;i<SHARED_SUPPORTS;++i)if(rider_bits[i])return false;
  }
  if(script->phase && script->kind==SCENE_INTRO_RETIRED_X &&
     (s->wipe || s->current!=s->anchor || script->owner!=s->anchor ||
      a || b || h || o || v || q || world->active || healing->active))return false;
  if(world->active && ((!enemy_actor(world->slot) && !item_actor(world->slot) && effect_index(world->slot)<0 && world->slot!=0xcc8) || a || b || o || v))return false;
  const NativePass *active=a?c:b?t:h?&hit->native:o?&obj->native:v?art:q?&support->native:NULL;
  unsigned base=h?hit->seat:q?support->seat:s->anchor;
  if(active && (base>1 || s->wipe || s->current!=(base^(active->pass==2)) ||
     s->players[s->current].life!=X3_COOP_ALIVE))return false;
  if(world->active) {
    bool cleanup=world->slot==0xcc8 || (healing->active && healing->slot==world->slot && healing->seat==world->seat) ||
        (script->phase && script->kind && script->owner==world->seat);
    unsigned seat=s->players[world->seat].life==X3_COOP_ALIVE || cleanup?world->seat:s->anchor;
    if(h?hit->seat!=seat:q?support->restore!=seat:!s->wipe && s->current!=seat)return false;
  }
  if((a && c->entry.d!=X3_COOP_BODY) || (b && t->entry.d!=X3_COOP_BODY))return false;
  if(h){
    if(hit->pc==0xcc5c || hit->pc==0xe0ae){
      if(!item_actor(hit->native.entry.d) || !world->active || world->slot!=hit->native.entry.d ||
         world->seat!=hit->seat || (hit->pc==0xcc5c && hit->native.entry.x!=X3_COOP_BODY))return false;
    }else if((hit->pc!=0xcb31 && hit->pc!=0xcb74) || !enemy_actor(hit->native.entry.d))return false;
  }
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
static bool scene_decode(NativeScene *out,const uint8_t bytes[16],const X3CoopState *s){
  if(memcmp(bytes,"X3S2",4) || bytes[6]>1 || bytes[7]>3 || bytes[8]>7 || bytes[9]>14 || bytes[10]>1 || bytes[11]>SCENE_INTRO_RETIRED_X)return false;
  NativeScene next={(uint16_t)word(bytes+4),bytes[6],bytes[7],bytes[8],bytes[9],bytes[10],bytes[11],
      {(uint16_t)word(bytes+12),(uint16_t)word(bytes+14)}};
  if((next.raw[0]|next.raw[1])&15)return false;
  bool stage=next.kind==SCENE_STAGE_ENTRY;
  bool zero_return=next.kind==SCENE_INTRO_RETURN && s->players[next.owner].character==X3_COOP_ZERO &&
      s->players[next.owner^1].life!=X3_COOP_ALIVE;
  bool cinematic=next.kind==SCENE_INTRO_RETIRED_X;
  if(cinematic && (s->players[next.owner].character!=X3_COOP_ZERO ||
     s->players[next.owner].life!=X3_COOP_ALIVE || s->players[next.owner^1].life==X3_COOP_ALIVE))return false;
  if(next.kind>=SCENE_INTRO_CAPTURE && (next.stage ||
      ((next.kind==SCENE_INTRO_ZERO || next.kind>=SCENE_INTRO_ZERO_CONSTRUCT) && next.phase!=2) ||
      (next.kind==SCENE_INTRO_CAPTURE && next.phase>2)))return false;
  if(stage && (next.phase!=2 || next.tick ||
      s->wipe || next.owner!=s->anchor || s->current!=s->anchor))return false;
  bool slot=stage?!next.slot:cinematic && next.slot==0x0cc8?true:
      next.kind?enemy_actor(next.slot) && !hostile_projectile(next.slot):item_actor(next.slot);
  if(next.phase?(!s->initialized || !slot || (next.phase==2 && next.tick) ||
      (next.kind && !stage && !zero_return && !cinematic && next.kind!=SCENE_INTRO_ZERO && next.kind!=SCENE_INTRO_X_CONSTRUCT &&
       s->players[next.owner].character!=X3_COOP_X)):
      (next.slot || next.owner || next.tick || next.stage || next.kind))return false;
  if(next.phase && s->players[next.owner].life==X3_COOP_ALIVE && next.owner!=s->anchor)return false;
  *out=next;return true;
}
static bool participation_valid(const uint8_t bytes[8],const NativeScene *s,const X3CoopState *players){
  unsigned zero=players->players[0].character==X3_COOP_ZERO?0:1;
  unsigned profile=bytes[6],pending=bytes[7];
  return !memcmp(bytes,"X3J2",4) && bytes[4]<=PARTICIPATION_HOLD+1 && bytes[5]<=PARTICIPATION_HOLD+1 &&
      !(profile&~(PROFILE_STOCK|PROFILE_LEARNER)) && !(pending&~profile) &&
      (!(pending&PROFILE_STOCK) || (!players->wipe && players->players[zero].life==X3_COOP_ALIVE)) &&
      (!(pending&PROFILE_LEARNER) || (!players->wipe && !s->phase &&
          players->players[0].life==X3_COOP_ALIVE && players->players[1].life==X3_COOP_ALIVE)) &&
      (players->initialized || (!bytes[4] && !bytes[5] && !bytes[7])) &&
      (!(s->phase && s->kind==SCENE_STAGE_ENTRY) || (!bytes[4] && !bytes[5] && !bytes[7]));
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
  uint8_t scene_bytes[16];NativeScene scripted;sli->func(sli,scene_bytes,sizeof(scene_bytes));
  uint8_t participation[8];sli->func(sli,participation,sizeof(participation));
  uint8_t views[X3_COOP_VIEW_STATE_BYTES];ViewSpawn spawn;
  sli->func(sli,views,sizeof(views));sli->func(sli,&spawn,sizeof(spawn));
  ViewBounds bounds;sli->func(sli,&bounds,sizeof(bounds));
  uint8_t owners[SHARED_EFFECTS];sli->func(sli,owners,sizeof(owners));
  for(unsigned i=0;i<SHARED_EFFECTS;++i)if(owners[i]>2)return false;
  if(!X3CoopPresentationLoad(sli))return false;
  sli->func(sli,&support,sizeof(support));sli->func(sli,rider_bits,sizeof(rider_bits));
  uint8_t menu_bytes[MENU_BYTES];PairedMenu paired;sli->func(sli,menu_bytes,sizeof(menu_bytes));
  if(!X3CoopViewsLoad(views,sizeof(views),false) || !view_spawn_valid(&spawn) || bounds.native.pass>3)return false;
  if(!X3CoopDecode(&loaded,bytes,X3CoopEncodedSize()) || !refill_decode(&healing,refill_bytes,&loaded) ||
     !scene_decode(&scripted,scene_bytes,&loaded) || !participation_valid(participation,&scripted,&loaded) ||
     !passes_valid(&loaded,&c,&t,&hit,&obj,&art,&world,&healing,&scripted,&support,rider_bits))return false;
  if(!menu_decode(&paired,menu_bytes,&loaded) || (paired.phase &&
      ((participation[7]&PROFILE_LEARNER) || scripted.phase || healing.active || c.pass || t.pass || hit.native.pass || obj.native.pass || art.pass ||
       world.active || support.native.pass || spawn.active || bounds.native.pass)))return false;
  if(scripted.phase && (scripted.kind==SCENE_STAGE_ENTRY || scripted.kind==SCENE_INTRO_RETIRED_X) &&
     (spawn.active || bounds.native.pass))return false;
  if(bounds.native.pass==3 && (!views[0] || !loaded.initialized || (c.pass!=2 && t.pass!=2 && !(world.active && world.slot==0xcc8))))return false;
  if(world.active && effect_index(world.slot)>=0 && owners[effect_index(world.slot)]!=world.seat+1)return false;
  if(!loaded.initialized)for(unsigned i=0;i<SHARED_EFFECTS;++i)if(owners[i])return false;
  memcpy(effect_owner,owners,sizeof(effect_owner));
  state=loaded;controller=c;terrain=t;contact=hit;objects=obj;draw=art;target=world;solid=support;
  refill=healing;scene=scripted;
  menu=paired;
  memcpy(participation_hold,participation+4,sizeof(participation_hold));
  stock_zero=(participation[6]&PROFILE_STOCK)!=0;stock_exit_pending=(participation[7]&PROFILE_STOCK)!=0;
  learner_enabled=(participation[6]&PROFILE_LEARNER)!=0;learner_rescue=(participation[7]&PROFILE_LEARNER)!=0;
  /* A state can load before the first reset-frame environment setup. An
   * enrolled machine must resume its actor schedule on that startup path. */
  if(loaded.initialized){enabled=true;p1_character=loaded.players[0].character;}
  memcpy(riders,rider_bits,sizeof(riders));view_spawn=spawn;view_bounds=bounds;
  X3CoopViewsLoad(views,sizeof(views),true);return true;
}
bool X3CoopRuntimePreflight(const void *data,size_t size) {
  size_t payload=X3CoopEncodedSize(),tail=payload+sizeof(controller)+sizeof(terrain)+sizeof(contact)+sizeof(objects)+sizeof(draw)+sizeof(target)+8+16+8+sizeof(solid)+sizeof(riders)+MENU_BYTES+X3_COOP_VIEW_STATE_BYTES+sizeof(view_spawn)+sizeof(view_bounds)+sizeof(effect_owner)+X3CoopPresentationEncodedSize();
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
  NativeScene scripted;if(!scene_decode(&scripted,p,&decoded))return false;p+=16;
  if(!participation_valid(p,&scripted,&decoded))return false;
  unsigned pending=p[7];p+=8;
  if(!X3CoopViewsLoad(p,X3_COOP_VIEW_STATE_BYTES,false))return false;
  uint8_t personal_mode=p[0];
  p+=X3_COOP_VIEW_STATE_BYTES;ViewSpawn spawn;memcpy(&spawn,p,sizeof(spawn));
  p+=sizeof(spawn);ViewBounds bounds;memcpy(&bounds,p,sizeof(bounds));
  p+=sizeof(bounds);
  const uint8_t *owners=p;p+=sizeof(effect_owner);
  for(unsigned i=0;i<SHARED_EFFECTS;++i)if(owners[i]>2 || (!decoded.initialized && owners[i]))return false;
  if(world.active && effect_index(world.slot)>=0 && owners[effect_index(world.slot)]!=world.seat+1)return false;
  if(!X3CoopPresentationPreflight(p,X3CoopPresentationEncodedSize()))return false;
  p+=X3CoopPresentationEncodedSize();memcpy(&support,p,sizeof(support));p+=sizeof(support);
  if(!passes_valid(&decoded,&c,&t,&hit,&obj,&art,&world,&healing,&scripted,&support,p))return false;
  PairedMenu paired;
  if(!menu_decode(&paired,p+sizeof(riders),&decoded) || (paired.phase &&
      ((pending&PROFILE_LEARNER) || scripted.phase || healing.active || c.pass || t.pass || hit.native.pass || obj.native.pass || art.pass ||
       world.active || support.native.pass || spawn.active || bounds.native.pass)))return false;
  if(paired.phase && size>tail){
    size_t ram_at;size_t guest=RtlSnapshotGuestLayout(NULL,&ram_at);
    if(!guest || ram_at>size || size-ram_at<0x20000)return false;
    const uint8_t *ram=(const uint8_t *)data+ram_at;
    bool paused=X3CoopNativePauseActive(ram),exiting=(ram[0x1f37]&0x80)!=0;
    if((paired.phase==1 && exiting) || (paired.phase==2 && !paused) ||
       (paired.phase==3 && !paused && !exiting))return false;
  }
  if(scripted.phase && (scripted.kind==SCENE_STAGE_ENTRY || scripted.kind==SCENE_INTRO_RETIRED_X) &&
     (spawn.active || bounds.native.pass))return false;
  if(scripted.phase && scripted.kind==SCENE_STAGE_ENTRY && size>tail){
    size_t ram_at;size_t guest=RtlSnapshotGuestLayout(NULL,&ram_at);
    if(!guest || ram_at>size || size-ram_at<0x20000 ||
       scripted.stage!=((const uint8_t *)data)[ram_at+0x1fae])return false;
  }
  if(bounds.native.pass>3)return false;
  if(bounds.native.pass==3 && (!personal_mode || !decoded.initialized || (c.pass!=2 && t.pass!=2)))return false;
  if(bounds.native.pass && bounds.native.pass<3 && (!decoded.initialized || decoded.wipe || bounds.native.entry.d!=0x1e58 ||
      c.pass || t.pass || hit.native.pass || obj.native.pass || art.pass || world.active || support.native.pass ||
      decoded.current!=(decoded.anchor^(bounds.native.pass==2))))return false;
  return view_spawn_valid(&spawn) && (!spawn.active || (decoded.initialized && !decoded.wipe));
}
