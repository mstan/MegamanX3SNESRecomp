/* Run the shared real-ROM state checks with the desktop timing contract. */
#define main MmxStateChecksMain
#include MMX_SHARED_STATE_TEST
#undef main
#include "../src/x3_coop_runtime.h"
#include "../src/x3_coop_view.h"
static unsigned coop_x(unsigned seat) {
  const uint8_t *body=X3CoopRuntimeState()->players[seat].body;
  return body[5]|body[6]<<8;
}
#include "x3_coop_contact_test.inc"
#include "x3_coop_target_test.inc"
#include "x3_coop_support_test.inc"
#include "x3_coop_view_test.inc"
#include "x3_coop_present_test.inc"
#include "x3_coop_display_test.inc"
#include "x3_coop_pickup_test.inc"
#include "x3_coop_scene_test.inc"
#include "x3_coop_acquisition_test.inc"
#include "x3_coop_capsule_test.inc"
#include "x3_coop_stage_test.inc"
#include "x3_coop_return_test.inc"
int main(int argc,char **argv) {
  RtlEnableExtendedFrameTiming();
  int result=MmxStateChecksMain(argc,argv);
  if(!result && argc==2) {
    size_t cap=2u*1024u*1024u;
    uint8_t *saved=malloc(cap),*damaged=malloc(cap),*after=malloc(cap);
    check(saved && damaged && after,"co-op preflight buffers");
    size_t size=RtlSaveSnapshotToMemory(saved,cap),marker=0;
    check(size>32,"co-op snapshot is present");
    for(size_t i=size-4;i>0;--i)if(!memcmp(saved+i,"X3C4",4)){marker=i;break;}
    check(marker && marker+8<size,"co-op context chunk is present");
    memcpy(damaged,saved,size);damaged[marker+6]=2;
    check(!RtlLoadSnapshotFromMemory(damaged,size),"invalid actor anchor is refused before load");
    size_t after_size=RtlSaveSnapshotToMemory(after,cap);
    same(saved,size,after,after_size,"refused actor state leaves the machine unchanged");
    if(!X3CoopRuntimeState()->initialized){
      memcpy(damaged,saved,size);damaged[door_scene_chunk(saved,size)+16+4]=1;
      check(!RtlLoadSnapshotFromMemory(damaged,size),"unenrolled state cannot contain a pending participation request");
      after_size=RtlSaveSnapshotToMemory(after,cap);
      same(saved,size,after,after_size,"refused unenrolled participation state leaves the machine unchanged");
    }
    const char *fixture=getenv("X3_COOP_TEST_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(false);
      check(RtlLoadSnapshot(fixture),"native two-actor fixture loads");
      check(X3CoopRuntimeEnabled(),"loading an enrolled state before boot enables its actor schedule");
      check(X3CoopRuntimeState()->initialized,"fixture contains enrolled actors");
      size=RtlSaveSnapshotToMemory(saved,cap);
      const char *low_p2=getenv("X3_COOP_LOW_P2_OUTPUT");
      if(low_p2 && *low_p2){
        memcpy(damaged,saved,size);size_t offset=contact_context_offset(saved,size);
        X3CoopState setup=*X3CoopRuntimeState();setup.players[1].body[0x27]=1;
        check(X3CoopEncode(&setup,damaged+offset,X3CoopEncodedSize()),"private low-HP precondition encodes");
        check(RtlLoadSnapshotFromMemory(damaged,size) && RtlSaveSnapshot(low_p2),"private low-HP route precondition saves");
        check(RtlLoadSnapshotFromMemory(saved,size),"low-HP preparation restores its starting machine");
      }
      native_persistent_pickup_checks(saved,size,after,damaged,cap,11,5856,1096,1,false);
      for(unsigned character=0;character<2;++character)native_door_checks(saved,size,after,damaged,cap,character);
      native_armored_door_checks(saved,size,after,damaged,cap);
      native_display_checks("start",saved,size,after,damaged,cap);
      native_resource_checks(saved,size,after,cap);
      remote_resource_checks(saved,size,after,cap);
      native_presentation_checks(saved,size,after,damaged,cap);
      native_reused_pose_checks(saved,size,after,damaged,cap);
      native_health_limit_checks(saved,size,after,cap);
      authored_health_pickup_checks(saved,size,after,damaged,cap);
      pickup_contact_fallback_checks(saved,size,after,damaged,cap);
      native_shared_life_checks(saved,size,after,cap);
      native_weapon_pickup_checks(saved,size,after,damaged,cap);
      for(unsigned mode=0;mode<4;++mode)competing_pickup_checks(saved,size,after,damaged,cap,mode);
      native_view_checks(saved,size,after,damaged,cap);
      native_contact_checks(saved,size,after,cap);
      native_support_checks(saved,size,after,damaged,cap);
      unsigned x=coop_x(0),zero=coop_x(1);
      for(int i=0;i<30;++i)frame(SNES_PAD_RIGHT<<12);
      check(coop_x(0)==x && coop_x(1)>zero,"native partner movement leaves X in place");
      size_t expected_size=RtlSaveSnapshotToMemory(damaged,cap);
      check(RtlLoadSnapshotFromMemory(saved,size),"initialized co-op snapshot restores");
      after_size=RtlSaveSnapshotToMemory(after,cap);
      same(saved,size,after,after_size,"initialized co-op immediate restore");
      for(int i=0;i<30;++i)frame(SNES_PAD_RIGHT<<12);
      after_size=RtlSaveSnapshotToMemory(after,cap);
      same(damaged,expected_size,after,after_size,"native co-op deterministic replay");
    }
    fixture=getenv("X3_COOP_RETURN_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native safe return fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_return_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_STAGE_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native stage handoff fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_stage_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_REVERSED_STATE");
    if(fixture && *fixture){
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"native reversed roster fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_persistent_pickup_checks(saved,size,after,damaged,cap,11,5856,1096,1,false);
      for(unsigned character=0;character<2;++character)native_door_checks(saved,size,after,damaged,cap,character);
      native_armored_door_checks(saved,size,after,damaged,cap);
      native_display_checks("reversed",saved,size,after,damaged,cap);
      native_reversed_roster_checks(saved,size,after,damaged,cap);
      authored_health_pickup_checks(saved,size,after,damaged,cap);
      pickup_contact_fallback_checks(saved,size,after,damaged,cap);
      native_shared_life_checks(saved,size,after,cap);
      native_weapon_pickup_checks(saved,size,after,damaged,cap);
      for(unsigned mode=0;mode<4;++mode)competing_pickup_checks(saved,size,after,damaged,cap,mode);
    }
    fixture=getenv("X3_COOP_INDEPENDENT_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"normal independent P2 route fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      independent_display_checks(saved,size,after,damaged,cap);
      independent_route_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_TARGET_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"authored native enemy fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_display_checks("enemy",saved,size,after,damaged,cap);
      native_target_checks(saved,size,after,cap);
    }
    fixture=getenv("X3_COOP_PLATFORM_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"authored support stage fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      for(unsigned reversed=0;reversed<2;++reversed)native_persistent_pickup_checks(saved,size,after,damaged,cap,5,2944,1912,0x20,reversed);
      authored_support_checks(saved,size,after,cap);
    }
    fixture=getenv("X3_COOP_CAPSULE_STATE");
    if(fixture && *fixture){
      X3CoopRuntimeEnable(true);check(RtlLoadSnapshot(fixture),"authored native capsule stage fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      for(unsigned reversed=0;reversed<2;++reversed)native_capsule_checks(saved,size,after,damaged,cap,reversed);
    }
    fixture=getenv("X3_COOP_SURVIVOR_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"natural-contact survivor fixture loads");
      check(X3CoopRuntimeState()->players[0].life==X3_COOP_FALLEN &&
            X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,
            "X remains fallen with Zero alive");
      bool bright=true,unfrozen=true;
      for(int i=0;i<60;++i) {
        frame(0);
        bright=bright && g_ram[0xd1]==4 && g_ram[0xd2]==4 && (g_ppu->inidisp&0x8f)==15;
        for(unsigned a=0x1f25;a<=0x1f2b;++a)unfrozen=unfrozen && !g_ram[a];
      }
      check(bright,"survivor remains in bright active gameplay");
      check(unfrozen,"survivor world pools remain unfrozen");
      check(X3CoopRuntimeState()->players[0].life==X3_COOP_FALLEN,
            "surviving gameplay does not revive X");
    }
    fixture=getenv("X3_COOP_P2_SURVIVOR_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"natural-contact P2-death fixture loads");
      bool bright=true,unfrozen=true;
      for(int i=0;i<60;++i) {
        frame(0);
        bright=bright && g_ram[0xd1]==4 && g_ram[0xd2]==4 && (g_ppu->inidisp&0x8f)==15;
        for(unsigned a=0x1f25;a<=0x1f2b;++a)unfrozen=unfrozen && !g_ram[a];
      }
      check(bright && unfrozen,"P2-death survivor retains bright unfrozen gameplay");
      check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE &&
            X3CoopRuntimeState()->players[1].life==X3_COOP_FALLEN,
            "continued gameplay keeps Zero fallen and X alive");
    }
    fixture=getenv("X3_COOP_RETRY_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"native staggered-wipe retry fixture loads");
      const X3CoopState *s=X3CoopRuntimeState();
      check(s->initialized && !s->wipe && s->players[0].life==X3_COOP_ALIVE &&
            s->players[1].life==X3_COOP_ALIVE,"whole-team native retry reenrolls both actors");
      check((s->players[0].body[0x27]&127)==16 && (s->players[1].body[0x27]&127)==28 &&
            s->players[0].maximum_health==16 && s->players[1].maximum_health==28,
            "native retry gives both actors fresh health");
      check(g_ram[0x1fb4]==1,"staggered-wipe route debits one shared life from two");
    }
    free(saved);free(damaged);free(after);
    /* Exercise adaptive native/wide bounds in the same diagnostic stream. */
    for(int i=0;i<120;++i) {
      g_last_drawable_width=i<40?960:i<80?1920:1280;
      g_last_drawable_height=720;
      frame(i<60?SNES_PAD_RIGHT:0);
    }
    MmxReplayShutdown();
  }
  return result;
}
