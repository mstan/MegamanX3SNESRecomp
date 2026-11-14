#include "../src/x3_coop_settings.h"
#include "../src/x3_display.h"
#include "../src/x3_coop_hud.h"

/* Consume earned survivor states in safe native gameplay. Physical device
 * hotplug remains an owner/input-backend check; neutral and restored pad words
 * exercise this title's input/lifecycle boundary without pretending otherwise. */
static void persistence_roster(unsigned living,unsigned character,unsigned lives){
  const X3CoopState *s=X3CoopRuntimeState();unsigned fallen=living^1;
  check(s->initialized && !s->wipe && s->players[living].life==X3_COOP_ALIVE &&
      s->players[fallen].life==X3_COOP_FALLEN && contact_hp(living) && !contact_hp(fallen) &&
      s->players[living].character==character && s->players[fallen].character==(character^1) &&
      campaign_body(living)[0xb6]==character*2 && campaign_body(fallen)[0xb6]==(character^1)*2 &&
      g_ram[0x1fb4]==lives,"pause, restore and host options retain the actual living/fallen roster and shared lives");
}
static void native_fallen_persistence(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  uint8_t *seed=malloc(capacity),*paused=malloc(capacity),*future=malloc(capacity);
  check(seed && paused && future,"fallen persistence complete-future buffers allocate");
  for(unsigned reversed=0;reversed<2;++reversed){
    check(RtlLoadSnapshot(fixture),"fallen persistence loads its actually earned survivor without a file picker");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    if(reversed){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;setup.current^=1;setup.anchor^=1;}
    unsigned living=setup.anchor,character=setup.players[living].character,fallen=living^1;
    check(setup.players[living].life==X3_COOP_ALIVE && setup.players[fallen].life==X3_COOP_FALLEN,
        "the persistence prerequisite never fabricates or revives a fallen participant");
    setup.players[living].body[0x88]=255; /* Bounded personal contact protection. */
    check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),
        "persistence preparation changes only physical-seat assignment and personal protection");door_install_player(&setup);
    n=RtlSaveSnapshotToMemory(seed,capacity);
    for(unsigned run=0;run<2;++run){
      X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3CoopHudSetLayout(X3_COOP_HUD_COMPACT);
      check(RtlLoadSnapshotFromMemory(seed,n),"the same earned fallen roster restores before the complete persistence sequence");
      unsigned lives=g_ram[0x1fb4];bool stock=X3CoopRuntimeStockZero(),learner=X3CoopRuntimeLearner();
      persistence_roster(living,character,lives);
      size_t before=RtlSaveSnapshotToMemory(base,capacity);
      X3CoopSettingsResume();X3CoopSettingsPresentation();
      X3CoopRuntimeSetP1Character(setup.players[0].character^1);X3CoopRuntimeSetStockZero(!stock);
      X3CoopRuntimeSetLearner(!learner);X3CoopRuntimeSetDoorPolicy(true);
      check(X3CoopRuntimeStockZero()==stock && X3CoopRuntimeLearner()==learner && !X3CoopRuntimeRejoin(fallen),
          "startup-only simulation choices and rejoin cannot re-enroll an initialized fallen actor");
      size_t after=RtlSaveSnapshotToMemory(work,capacity);
      same(base,before,work,after,"resume callbacks and refused live profile changes leave the whole simulation unchanged");
      for(unsigned tick=0;tick<24;++tick)frame(0);
      menu_press(fallen,0x1000);X3CoopMenuInfo info;
      check(!X3CoopMenuRead(&info),"returning input on the fallen seat cannot claim native pause");
      menu_open(living);unsigned cursor=menu_read().cursor[fallen];
      for(unsigned layout=0;layout<4;++layout){
        X3Display_SetLocalSplit(layout==1);X3Display_SetPresentationSeat(layout>=2?(int)layout-2:-1);
        X3CoopHudSetLayout(layout==1?X3_COOP_HUD_HORIZONTAL:X3_COOP_HUD_COMPACT);
        frame(0);frame(0);persistence_roster(living,character,lives);
      }
      X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3CoopHudSetLayout(X3_COOP_HUD_COMPACT);
      menu_press(fallen,0x0400);menu_press(fallen,0x4000);
      check(menu_read().cursor[fallen]==cursor && !menu_read().healing[fallen],"presentation changes cannot activate the fallen pause panel");
      size_t pause_size=RtlSaveSnapshotToMemory(paused,capacity);
      check(RtlLoadSnapshotFromMemory(paused,pause_size),"an actual paused survivor restores the complete current-build machine");
      X3CoopSettingsResume();persistence_roster(living,character,lives);
      menu_press(living,0x1000);for(unsigned tick=0;tick<60;++tick)frame(0);
      check(!X3CoopMenuRead(&info),"the actual living owner resumes native gameplay after paused restore");
      for(unsigned tick=0;tick<90;++tick)frame((uint32_t)SNES_PAD_SELECT<<(fallen*12));
      persistence_roster(living,character,lives);unsigned x=support_position(living,5);
      for(unsigned tick=0;tick<24;++tick)frame((uint32_t)SNES_PAD_RIGHT<<(living*12));
      check(support_position(living,5)>x,"restored input still moves the actual survivor after all persistence operations");
      persistence_roster(living,character,lives);size_t done=RtlSaveSnapshotToMemory(work,capacity);
      if(!run)memcpy(future,work,done);else same(future,done,work,done,"native pause, state restore, input return and host options replay the entire fallen-roster future exactly");
    }
  }
  free(seed);free(paused);free(future);
}

/* Earned captures/riders and real attacks precede native deaths. A retry must
 * construct fresh personal contexts and retire their old shared ownership. */
static unsigned retry_live_shots(unsigned seat){
  const X3CoopState *s=X3CoopRuntimeState();const uint8_t *shots=s->current==seat?g_ram+X3_COOP_SHOTS:s->players[seat].shots;unsigned live=0;
  for(unsigned at=0;at<X3_COOP_SHOT_BYTES;at+=64)live+=shots[at]!=0;return live;
}
static void native_dirty_team_retry(bool capture,uint8_t *base,uint8_t *work,size_t capacity){
  X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
  check(setup.players[0].life==X3_COOP_ALIVE && setup.players[1].life==X3_COOP_ALIVE && !setup.wipe,"dirty retry starts with both actual living participants");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);unsigned victim=2,riders=0,captors=0;
  for(unsigned i=0;i<31;++i){unsigned a=base[contact_menu_offset(base,n)-31+i];riders|=a&3;captors|=a&12;}
  if(capture){for(unsigned seat=0;seat<2;++seat)if(campaign_body(seat)[0xa3]==0x96)victim=seat;check(victim<2 && captors,"dirty retry consumes an actually earned native capture and its retained owner");setup.players[victim].body[0x27]=1;}
  else check(riders==3,"dirty retry consumes two actually earned riders on an authored native support");
  unsigned shooter=capture?victim^1:0;
  check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"dirty retry preparation retains the real roster and supplies only captured-victim oneHP when applicable");door_install_player(&setup);n=RtlSaveSnapshotToMemory(base,capacity);
  uint8_t *future=malloc(capacity);check(future!=NULL,"dirty team retry whole-future buffer allocates");unsigned first_ready=0;
  for(unsigned run=0;run<2;++run){
    check(RtlLoadSnapshotFromMemory(base,n),"the same earned live capture or riders restore before attacks and native deaths");
    unsigned stage=g_ram[0x1fae],cpp=g_ram[0x1fb5],lives=g_ram[0x1fb4],story=g_ram[0x1fb2],hunters=g_ram[0x1fd8];uint8_t acquired[9];for(unsigned i=0;i<9;++i)acquired[i]=g_ram[0x1fbc+2*i]&0xc0;
    unsigned fired=0;for(;fired<16 && !retry_live_shots(shooter);++fired)frame((uint32_t)(SNES_PAD_LEFT|(fired%8<4?SNES_PAD_Y:0))<<(shooter*12));
    check(retry_live_shots(shooter)>0,"a real physical buster input leaves a live owned shot before the team deaths");
    size_t before=RtlSaveSnapshotToMemory(work,capacity);X3CoopState harm=*X3CoopRuntimeState();X3CoopCapture(&harm,g_ram);
    for(unsigned seat=0;seat<2;++seat)if(!capture || seat==shooter)campaign_position_only(&harm.players[seat],support_position(seat,5),view_read(0x1e60)+400);
    check(X3CoopEncode(&harm,work+contact_context_offset(work,before),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,before),"dirty retry uses position-only native pit approaches without supplying death or retry results");door_install_player(&harm);
    bool wipe=false;unsigned elapsed=0;
    for(;elapsed<6000;++elapsed){
      frame(elapsed%120<6?SNES_PAD_B|((uint32_t)SNES_PAD_B<<12):0);const X3CoopState *s=X3CoopRuntimeState();wipe|=s->wipe;
      if(g_ram[0x1fb4]!=lives && g_ram[0x1fb4]+1!=lives)check(false,"dirty team retry cannot spend multiple shared lives");
      bool ready=wipe && s->initialized && !s->wipe && g_ram[0x1fb4]+1==lives && g_ram[0xd1]==4 && g_ram[0xd2]==4 && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && !g_ram[0x1f22] && !g_ram[0x1f24] && !g_ram[0x1f4f];
      for(unsigned seat=0;seat<2;++seat){const uint8_t *b=campaign_body(seat);ready&=s->players[seat].life==X3_COOP_ALIVE && b[1]==2 && b[2]<0x1c && !b[0x6e] && (!b[0xa3] || b[0xa3]==0x80);}
      if(ready)break;
    }
    const X3CoopState *s=X3CoopRuntimeState();size_t done=RtlSaveSnapshotToMemory(work,capacity);unsigned bound=0,owned=0,shots[2]={retry_live_shots(0),retry_live_shots(1)};
    for(unsigned i=0;i<31;++i)bound|=work[done-X3_COOP_MENU_STATE_BYTES-31+i]&15;size_t owners=combat_owners(work,done);for(unsigned i=0;i<40;++i)owned+=work[owners+i]!=0;
    fprintf(stderr,"native DIRTY TEAM RETRY capture%u run%u roster%u victim%u fired%u ready%u wipe%u stage%u/%u CPP%u/%u lives%u/%u shots%u/%u bound%u effects%u hurt%u/%u protect%u/%u charge%u/%u\n",capture,run,setup.players[0].character,victim,fired,elapsed,wipe,stage,g_ram[0x1fae],cpp,g_ram[0x1fb5],lives,g_ram[0x1fb4],shots[0],shots[1],bound,owned,campaign_body(0)[0x30],campaign_body(1)[0x30],campaign_body(0)[0x88],campaign_body(1)[0x88],campaign_body(0)[0x58],campaign_body(1)[0x58]);
    check(RtlSaveSnapshot("native-dirty-team-retry-outcome.sav"),"the actual dirty-context native retry retains its complete result");
    check(elapsed<6000 && wipe && g_ram[0x1fae]==stage && g_ram[0x1fb5]==cpp && g_ram[0x1fb2]==story && g_ram[0x1fd8]==hunters,"dirty native retry restores its actual checkpoint and retained campaign once");
    check(!shots[0] && !shots[1] && !bound && !owned,"native team retry leaves no live old shot, capture, support contact or shared effect owner");
    for(unsigned i=0;i<9;++i)check((g_ram[0x1fbc+2*i]&0xc0)==acquired[i],"native retry retains acquired campaign weapon resources");
    for(unsigned seat=0;seat<2;++seat){const uint8_t *b=campaign_body(seat);const X3CoopPlayer *p=&s->players[seat];
      check(p->character==setup.players[seat].character && p->maximum_health==setup.players[seat].maximum_health && b[0xb6]==p->character*2 && (b[0x27]&127)==p->maximum_health && !p->shot_command && !b[0x57] && !b[0x58] && !b[0x30] && !b[0x88] && !b[0x8a] && !b[0x6b],"both native retry bodies regain full own health and clear stale charge, hurt and protection");
    }
    for(unsigned seat=0;seat<2;++seat){unsigned x=support_position(seat,5);for(unsigned f=0;f<24;++f)frame((uint32_t)SNES_PAD_RIGHT<<(seat*12));check(support_position(seat,5)>x,"each requested physical native controller moves after the dirty team retry");}
    for(unsigned f=0;f<120;++f)frame(0);check(g_ram[0x1fb4]+1==lives && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"the reconstructed pair stays alive with one stable native retry cost");
    done=RtlSaveSnapshotToMemory(work,capacity);if(!run){first_ready=elapsed;memcpy(future,work,done);}else{check(elapsed==first_ready,"dirty contexts reach the same native retry update");same(future,done,work,done,"actual live attack, dirty ownership, native deaths and complete clean retry replay the whole machine exactly");}
  }
  free(future);
}
