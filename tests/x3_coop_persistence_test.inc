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
