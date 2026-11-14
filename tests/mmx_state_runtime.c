/* Run the shared real-ROM state checks with the desktop timing contract. */
#define main MmxStateChecksMain
#include MMX_SHARED_STATE_TEST
#undef main
#include "snes/interp_bridge.h"
#include "../src/x3_coop_runtime.h"
#include "../src/x3_coop_view.h"
#include "../src/x3_coop_menu.h"
static unsigned coop_x(unsigned seat) {
  const uint8_t *body=X3CoopRuntimeState()->players[seat].body;
  return body[5]|body[6]<<8;
}
static void door_install_player(const X3CoopState *setup);
static unsigned view_read(unsigned address);
static void view_word(unsigned address,unsigned value);
static void vehicle_install_view(const X3CoopViewState *views,unsigned seat);
static size_t combat_owners(const uint8_t *bytes,size_t n);
static void native_gameplay_update(unsigned keys,bool original);
static void native_return_on_support(size_t capacity);
static void team_hazard_body(uint8_t *b,unsigned x,unsigned y);
static void native_scene_fatal(bool transport,unsigned leader,uint8_t *base,uint8_t *work,size_t capacity);
static void native_dirty_team_retry(bool capture,uint8_t *base,uint8_t *work,size_t capacity);
static void campaign_position_only(X3CoopPlayer *p,unsigned x,unsigned y);
static const uint8_t *campaign_body(unsigned seat);
static void native_weapon_hurt_lifetime(const uint8_t *cut,size_t cn,unsigned seat,unsigned index,unsigned mode,uint8_t *work,uint8_t *after,size_t capacity);
static void native_permanent_streaming(unsigned kind,unsigned px,unsigned py,unsigned variant,uint8_t *work,uint8_t *expected,size_t capacity);
static void native_one_shot_revisit(uint8_t *work,uint8_t *expected,size_t capacity);
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
#include "x3_coop_hud_test.inc"
#include "x3_coop_stock_test.inc"
#include "x3_coop_menu_test.inc"
#include "x3_coop_paired_menu_test.inc"
#include "x3_coop_flags_test.inc"
#include "x3_coop_shared_camera_test.inc"
#include "x3_coop_hazard_test.inc"
#include "x3_coop_team_test.inc"
#include "x3_coop_campaign_test.inc"
#include "x3_coop_learner_test.inc"
#include "x3_coop_intro_test.inc"
#include "x3_coop_rewind_test.inc"
#include "x3_coop_combat_test.inc"
#include "x3_coop_weapon_test.inc"
#include "x3_coop_preflight_test.inc"
#include "x3_coop_midpass_test.inc"
#include "x3_coop_vehicle_test.inc"
#include "x3_coop_terrain_test.inc"
#include "x3_coop_persistence_test.inc"
#include "x3_coop_streaming_test.inc"
int main(int argc,char **argv) {
  RtlEnableExtendedFrameTiming();
  int result=MmxStateChecksMain(argc,argv);
  if(!result && argc==2) {
    size_t cap=2u*1024u*1024u;
    uint8_t *saved=malloc(cap),*damaged=malloc(cap),*after=malloc(cap);
    check(saved && damaged && after,"co-op preflight buffers");
    size_t size=RtlSaveSnapshotToMemory(saved,cap),marker=0;
    check(size>32,"co-op snapshot is present");
    for(size_t i=size-4;i>0;--i)if(!memcmp(saved+i,"X3C5",4)){marker=i;break;}
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
      memcpy(damaged,saved,size);damaged[contact_menu_offset(saved,size)-31]=132;
      check(!RtlLoadSnapshotFromMemory(damaged,size),"unenrolled snapshot cannot claim native captive ownership");
      after_size=RtlSaveSnapshotToMemory(after,cap);
      same(saved,size,after,after_size,"refused unenrolled attachment leaves the full machine unchanged");
    }
    const char *fixture=getenv("X3_COOP_TEST_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(false);
      check(RtlLoadSnapshot(fixture),"native two-actor fixture loads");
      check(X3CoopRuntimeEnabled(),"loading an enrolled state before boot enables its actor schedule");
      check(X3CoopRuntimeState()->initialized,"fixture contains enrolled actors");
      const char *unified_output=getenv("X3_COOP_UNIFIED_OUTPUT");
      if(unified_output && *unified_output){
        const char *native_output=getenv("X3_COOP_NATIVE_OUTPUT");
        check(native_output && *native_output && !g_ram[X3_COOP_BODY+0xb6],"matched runtime exports start with the native X world body");
        X3CoopViewsConfigure(false);
        check(RtlSaveSnapshot(unified_output),"matched runtime exports the actual Unified configuration");
        X3CoopRuntimeEnable(false);X3CoopRuntimeReset();
        check(RtlSaveSnapshot(native_output),"matched runtime exports disabled native play without enrolled contexts");
        check(RtlLoadSnapshot(fixture),"matched runtime export leaves the normal checks on their original fixture");
      }
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
    fixture=getenv("X3_COOP_WEAPON_STATE");
    if(fixture && *fixture)native_acquired_weapon_checks(fixture,saved,after,damaged,cap);
    fixture=getenv("X3_COOP_MIDPASS_STATE");
    if(fixture && *fixture){const char *reversed=getenv("X3_COOP_MIDPASS_REVERSED_STATE");check(reversed && *reversed,"interrupted native passes supply both actual rosters");native_interrupted_pass_checks(fixture,reversed,saved,after,damaged,cap);}
    fixture=getenv("X3_COOP_PREFLIGHT_STATE");
    if(fixture && *fixture){const char *reversed=getenv("X3_COOP_PREFLIGHT_REVERSED_STATE");check(reversed && *reversed,"transactional current snapshots supply both real rosters");native_transactional_snapshot_checks(fixture,reversed,saved,after,damaged,cap);}
    fixture=getenv("X3_COOP_COMBAT_STATE");
    if(fixture && *fixture){const char *reversed=getenv("X3_COOP_COMBAT_REVERSED_STATE");check(reversed && *reversed,"native concurrent combat supplies both actual rosters");native_concurrent_combat_checks(fixture,reversed,saved,after,damaged,cap);}
    fixture=getenv("X3_COOP_REWIND_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_REWIND_REVERSED_STATE");
      check(reversed && *reversed,"initialized rewind supplies both actual rosters");
      native_initialized_rewind_checks(fixture,reversed,saved,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_CONTACT_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_CONTACT_REVERSED_STATE");
      check(reversed && *reversed,"native register checks supply both actual roster fixtures");
      for(unsigned roster=0;roster<2;++roster){
        check(RtlLoadSnapshot(roster?reversed:fixture),"native register comparison fixture loads");
        size=RtlSaveSnapshotToMemory(saved,cap);native_contact_register_checks(saved,size);
      }
    }
    fixture=getenv("X3_COOP_IMMUNITY_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_IMMUNITY_REVERSED_STATE");
      check(reversed && *reversed,"native immunity supplies both actual roster fixtures");
      for(unsigned roster=0;roster<2;++roster){
        check(RtlLoadSnapshot(roster?reversed:fixture),"native immunity actual roster loads");
        size=RtlSaveSnapshotToMemory(saved,cap);native_contact_immunity_checks(saved,size,after,damaged,cap);
      }
    }
    fixture=getenv("X3_COOP_INTRO_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"actual native intro capture fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_intro_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_INTRO_SURVIVOR_DISPLAY_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"actual Zero-only intro display fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_intro_survivor_display_checks(saved,size,after);
    }
    fixture=getenv("X3_COOP_INTRO_RESCUE_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"actual native intro restraint fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_intro_rescue_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_INTRO_HELPER_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"actual native Mac helper fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_intro_helper_death_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_PIT_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_PIT_REVERSED_STATE");
      check(reversed && *reversed,"authored pit coverage supplies both real roster orders");
      for(unsigned roster=0;roster<2;++roster){
        const char *path=roster?reversed:fixture;
        check(RtlLoadSnapshot(path) && X3CoopRuntimeState()->players[0].character==roster &&
            !X3CoopRuntimeLearner() && !X3CoopRuntimeStockZero(),
            "authored pit uses the requested real roster and default unprotected campaign");
        native_shared_authored_pits(path,saved,after,cap);
      }
    }
    fixture=getenv("X3_COOP_VEHICLE_STATE");
    if(fixture && *fixture){
      const char *room=getenv("X3_COOP_REMOTE_RESOURCE_OUTPUT");check(room && *room && RtlLoadSnapshot(fixture),"vehicle route uses known local fixtures without a file picker");
      size=RtlSaveSnapshotToMemory(saved,cap);remote_resource_checks(saved,size,after,cap);native_vehicle_reference(room,saved,after,cap);
    }
    fixture=getenv("X3_COOP_TRANSITION_DOOR_STATE");
    if(fixture && *fixture){
      const char *other=getenv("X3_COOP_TRANSITION_DOOR_REVERSED_STATE");check(other && *other && getenv("X3_COOP_TRANSITION_MENU"),"door/menu overlap supplies both actual native rosters");
      for(unsigned roster=0;roster<2;++roster){
        check(RtlLoadSnapshot(roster?other:fixture),"transition-menu gate loads its actual native roster");size=RtlSaveSnapshotToMemory(saved,cap);
        for(unsigned character=0;character<2;++character)native_door_checks(saved,size,after,damaged,cap,character);
      }
    }
    fixture=getenv("X3_COOP_SPIKE_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_SPIKE_REVERSED_STATE");
      check(reversed && *reversed,"spike coverage supplies both actual roster fixtures");
      native_spike_checks(fixture,reversed,saved,after,cap);
    }
    fixture=getenv("X3_COOP_INDEPENDENT_PIT_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_INDEPENDENT_PIT_REVERSED_STATE");
      check(reversed && *reversed,"separated pit coverage supplies both actual rosters");
      native_independent_pit_checks(fixture,reversed,saved,after,cap);
    }
    fixture=getenv("X3_COOP_LAST_TRUE_PIT_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_LAST_TRUE_PIT_REVERSED_STATE");
      check(reversed && *reversed,"last true pit supplies both actual rosters");
      native_last_true_pit_checks(fixture,reversed,saved,after,cap);
    }
    fixture=getenv("X3_COOP_TEAM_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_TEAM_REVERSED_STATE");
      check(reversed && *reversed,"team retry coverage supplies both actual native rosters");
      native_team_retry_checks(fixture,reversed,saved,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_NATIVE_SCENE_FATAL_STATE");
    if(fixture && *fixture){check(RtlLoadSnapshot(fixture) && !g_ram[X3_COOP_BODY+0xb6],"matched original fatal scene loads its actually admitted native X body");X3CoopRuntimeEnable(false);native_original_transport_fatal(saved,after,cap);}
    fixture=getenv("X3_COOP_BUFFALO_MECHANICS_STATE");
    if(fixture && *fixture)native_buffalo_capture_checks(fixture,saved,after,cap);
    fixture=getenv("X3_COOP_CAMPAIGN_SURVIVOR_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"authored earned-intro boss survivor fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);native_campaign_survivor_checks(saved,size,after);
    }
    fixture=getenv("X3_COOP_BOSS_RETRY_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"authored boss retry fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);native_boss_retry_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_FLAGS_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native private-status fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_flags_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_MENU_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native pause-menu fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_menu_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_PAIRED_MENU_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"paired pause-menu fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_paired_menu_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_FALLEN_PERSISTENCE_STATE");
    if(fixture && *fixture)native_fallen_persistence(fixture,saved,after,cap);
    fixture=getenv("X3_COOP_SHARED_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native shared camera fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_shared_camera_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_LEARNER_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native learner fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_learner_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_STOCK_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native stock participation fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_stock_checks(saved,size,after,damaged,cap);
    }
    fixture=getenv("X3_COOP_HUD_STATE");
    if(fixture && *fixture){
      check(RtlLoadSnapshot(fixture),"native HUD fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      native_hud_checks(saved,size,after,damaged,cap);
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
    fixture=getenv("X3_COOP_ROUTE_STATE");
    if(fixture && *fixture){const char *reversed=getenv("X3_COOP_ROUTE_REVERSED_STATE");check(reversed && *reversed,"movement activation supplies both actual rosters");native_activation_route_checks(fixture,reversed,saved,cap);}
    fixture=getenv("X3_COOP_HANDOVER_STATE");
    if(fixture && *fixture){const char *reversed=getenv("X3_COOP_HANDOVER_REVERSED_STATE");check(reversed && *reversed,"P1 takeover supplies both actual P2-first route worlds");native_activation_handover(fixture,reversed,saved,after,damaged,cap);}
    if(getenv("X3_COOP_PERSISTENT_STREAMING")){
      const char *heart=getenv("X3_COOP_STREAMING_HEART_STATE"),*tank=getenv("X3_COOP_STREAMING_TANK_STATE");
      check(heart && tank,"permanent streaming supplies the two actual native item stages");
      for(unsigned item=0;item<2;++item){
        check(RtlLoadSnapshot(item?tank:heart) && g_ram[0x1fae]==(item?5:1),"permanent streaming loads the authored unclaimed item stage");size=RtlSaveSnapshotToMemory(saved,cap);
        for(unsigned roster=0;roster<2;++roster)native_persistent_pickup_checks(saved,size,after,damaged,cap,item?5:11,item?2944:5856,item?1912:1096,item?0x20:1,roster);
      }
    }
    if(getenv("X3_COOP_RETRY_STREAMING")){
      const char *heart=getenv("X3_COOP_RETRY_HEART_STATE"),*reversed=getenv("X3_COOP_RETRY_HEART_REVERSED_STATE"),*acquired=getenv("X3_COOP_ACQUISITION_OUTPUT"),*platform=getenv("X3_COOP_PLATFORM_ROUTE_OUTPUT"),*retry=getenv("X3_COOP_RETRY_STREAMING_OUTPUT");
      check(heart && reversed && acquired && platform && retry,"reward retry supplies actual earned heart fixtures and owned output paths");
      char path[1024],normal[1024],other[1024];
      if(!getenv("X3_COOP_RETRY_STREAMING_REUSE")){
        native_reward_retry_prepare(heart,reversed,saved,after,damaged,cap);
        snprintf(path,sizeof(path),"%s-p1-subtank-complete.sav",acquired);native_activation_platform_route(path,saved,cap,platform);
      }
      snprintf(normal,sizeof(normal),"%s-roster0.sav",platform);snprintf(other,sizeof(other),"%s-roster1.sav",platform);
      native_team_retry_checks(normal,other,saved,after,damaged,cap);
      snprintf(path,sizeof(path),"%s-roster1.sav",retry);native_activation_platform_route(path,saved,cap,NULL);
      check(RtlLoadSnapshot(path),"streaming negative probe restores the actual reward-preserving retry");
      native_permanent_streaming(5,2944,1912,0x20,saved,after,cap);
    }
    fixture=getenv("X3_COOP_ROUTE_PLATFORM_STATE");if(fixture && *fixture)native_activation_platform_route(fixture,saved,cap,NULL);
    fixture=getenv("X3_COOP_ACTIVATION_STATE");
    if(fixture && *fixture){
      const char *reversed=getenv("X3_COOP_ACTIVATION_REVERSED_STATE");check(reversed && *reversed,"activation/display qualification supplies both actual rosters");
      for(unsigned roster=0;roster<2;++roster){
        check(RtlLoadSnapshot(roster?reversed:fixture),"authored independent activation loads its actual roster");size=RtlSaveSnapshotToMemory(saved,cap);X3CoopState leader=*X3CoopRuntimeState();leader.current=leader.anchor=0;check(X3CoopEncode(&leader,saved+contact_context_offset(saved,size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(saved,size),"activation comparison explicitly selects seat0 as the native world leader");door_install_player(&leader);size=RtlSaveSnapshotToMemory(saved,cap);native_view_checks(saved,size,after,damaged,cap);
        X3CoopState setup=*X3CoopRuntimeState();
        for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=seat?1840:736,y=1136;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x88]=255;}
        size=RtlSaveSnapshotToMemory(saved,cap);check(X3CoopEncode(&setup,saved+contact_context_offset(saved,size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(saved,size),"actual two-region position prerequisite restores");door_install_player(&setup);X3CoopViewsConfigure(true);
        X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;views.valid=views.camera_valid=3;
        for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=(uint16_t)(seat?1728:608);views.camera[seat][1]=1024;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=1024;views.limits[seat][3]=views.current[seat][3]=1024;}X3CoopViewsSetState(&views);
        view_word(0x1e5d,views.camera[setup.anchor][0]);view_word(0x1e7a,views.camera[setup.anchor][0]);view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6e,0);view_word(0x1e70,8192);const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};for(unsigned i=0;i<6;++i)view_word(vertical[i],1024);
        for(unsigned i=0;i<30;++i)frame(0);check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"both real actors survive their independent authored regions");size=RtlSaveSnapshotToMemory(saved,cap);
        independent_display_checks(saved,size,after,damaged,cap);independent_route_checks(saved,size,after,damaged,cap);
      }
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
    fixture=getenv("X3_COOP_LADDER_STATE");
    if(fixture && *fixture)native_ladder_checks(fixture,saved,after,cap);
    fixture=getenv("X3_COOP_DESTROY_SUPPORT_STATE");
    if(fixture && *fixture)native_destructible_support_checks(fixture,saved,after,cap);
    fixture=getenv("X3_COOP_SEPARATE_PLATFORM_STATE");
    if(fixture && *fixture){const char *reversed=getenv("X3_COOP_SEPARATE_PLATFORM_REVERSED_STATE");check(reversed && *reversed,"separate authored supports supply both real rosters");native_separate_authored_support_checks(fixture,reversed,saved,after,damaged,cap);}
    fixture=getenv("X3_COOP_PLATFORM_STATE");
    if(fixture && *fixture) {
      if(getenv("X3_COOP_PLATFORM_TURN") || getenv("X3_COOP_DIFFICULT_SUPPORT") || getenv("X3_COOP_DIRTY_RETRY_SUPPORT")){
        for(unsigned roster=0;roster<2;++roster){check(RtlLoadSnapshot(fixture),"support reversal restores the same native world for either roster");size=RtlSaveSnapshotToMemory(saved,cap);X3CoopState setup;check(X3CoopDecode(&setup,saved+contact_context_offset(saved,size),X3CoopEncodedSize()),"support reversal decodes its actual native bodies");if(roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;setup.current^=1;setup.anchor^=1;}check(X3CoopEncode(&setup,saved+contact_context_offset(saved,size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(saved,size),"support reversal installs the complete matched roster");door_install_player(&setup);size=RtlSaveSnapshotToMemory(saved,cap);authored_support_checks(saved,size,after,cap);}
      }else{
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"authored support stage fixture loads");
      size=RtlSaveSnapshotToMemory(saved,cap);
      for(unsigned reversed=0;reversed<2;++reversed)native_persistent_pickup_checks(saved,size,after,damaged,cap,5,2944,1912,0x20,reversed);
      authored_support_checks(saved,size,after,cap);
      const char *support_reversed=getenv("X3_COOP_PLATFORM_REVERSED_STATE");
      if(support_reversed && *support_reversed){
        check(RtlLoadSnapshot(support_reversed),"authored support loads the actual reversed roster");
        size=RtlSaveSnapshotToMemory(saved,cap);authored_support_checks(saved,size,after,cap);
      }
      }
    }
    fixture=getenv("X3_COOP_ROLLBACK_COMBAT_STATE");
    if(fixture && *fixture)native_rollback_combat_checks(fixture,saved,damaged,cap);
    fixture=getenv("X3_COOP_DIGEST_STATE");
    if(fixture && *fixture)native_coop_digest_checks(fixture,saved,damaged,after,cap);
    fixture=getenv("X3_COOP_DIFFICULT_RETURN_STATE");
    if(fixture && *fixture)native_difficult_return_checks(fixture,saved,damaged,after,cap);
    fixture=getenv("X3_COOP_CHECKPOINT_STATE");
    if(fixture && *fixture)native_checkpoint_persistence_checks(fixture,saved,damaged,after,cap);
    fixture=getenv("X3_COOP_CAPSULE_STATE");
    if(fixture && *fixture){
      X3CoopRuntimeEnable(true);check(RtlLoadSnapshot(fixture),"authored native capsule stage fixture loads");
      const char *capsule_stage=getenv("X3_COOP_CAPSULE_STAGE");
      if(capsule_stage){unsigned stage=(unsigned)atoi(capsule_stage);check(stage>0 && stage<15 && capsule_table(0x06ccfa+stage,1)!=255,"requested capsule stage has an actual native capsule entry");if(stage!=g_ram[0x1fae]){stage_request();g_ram[0x1fae]=(uint8_t)stage;unsigned elapsed=0;for(;elapsed<3600;++elapsed){frame(0);if(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4 && g_ram[0x1fae]==stage)break;}check(elapsed<3600,"capsule preparation completes the actual native new-stage constructor");for(unsigned f=0;f<120;++f)frame(0);check(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"native capsule stage settles two living bodies and their initial room bounds");}}
      unsigned capsule_award=capsule_mask();
      if(capsule_award&0xf0){g_ram[0x1fd1]=(uint8_t)(capsule_award==0xf0?255:15);g_ram[0x1fd7]=15;
        if(capsule_award==0xf0){g_ram[0x1fd4]=255;size=RtlSaveSnapshotToMemory(saved,cap);X3CoopState full=*X3CoopRuntimeState();unsigned x=full.players[0].character==X3_COOP_X?0:1;full.players[x].maximum_health=full.players[x].body[0x27]=32;check(X3CoopEncode(&full,saved+contact_context_offset(saved,size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(saved,size),"golden capsule supplies only its real collected-capacity/hearts and full-health prerequisites");door_install_player(&full);}
      }
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
