/* Desktop session policy. The shared host owns transport, rollback and input
 * admission; this title supplies the required co-op plan and identity. */
#if defined(SNES_HAS_LOBBY_CLIENT) && defined(SNESRECOMP_NET_ROLLBACK)
#include "recomp_launcher.h"
#include "mod_runtime.h"
#include "netplay/snes_netplay.h"
#include "netplay/snes_netplay_rb.h"
#include "netplay/snes_state_digest.h"
#include "host_paths.h"
#include "sha256.h"
#include "x3_coop_runtime.h"
#include "x3_coop_view.h"

static const char x3_net_package[]="megaman-x3.coop";
static const uint8_t x3_net_rom[32]={MMX_ROM_SHA256_BYTES};
static uint8_t x3_net_executable[32];
static uint32_t x3_net_sim_settings;

static uint32_t x3_net_word(const uint8_t *bytes){
  return (uint32_t)bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
}
static int x3_net_identify(void){
  char path[4096];size_t size=0;
  if(!snesrecomp_exe_path(path,sizeof(path)))return 0;
  void *binary=SDL_LoadFile(path,&size);
  if(!binary || !size){SDL_free(binary);return 0;}
  sha256_compute(binary,size,x3_net_executable);SDL_free(binary);
  return x3_net_word(x3_net_executable)!=0;
}
static void x3_net_profile_settled(uint32_t profile,uint32_t transcript){
  uint8_t content[76],digest[32];memcpy(content,x3_net_rom,32);
  /* The generic driver treats build differences as advisory. X3 has no
   * qualified cross-build pairs, so make this part of required content too. */
  memcpy(content+32,x3_net_executable,32);
  for(unsigned i=0;i<4;++i){content[64+i]=(uint8_t)(profile>>(8*i));content[68+i]=(uint8_t)(transcript>>(8*i));
    content[72+i]=(uint8_t)(x3_net_sim_settings>>(8*i));}
  sha256_compute(content,sizeof(content),digest);
  snes_netplay_rb_set_identity(x3_net_word(x3_net_executable),x3_net_word(digest));
}
static void x3_net_capabilities(void){
  enum { CAP_SSE2=1, CAP_SSE3=2, CAP_AVX2=4, CAP_NEON=8 };
  uint32_t cpu=(SDL_HasSSE2()?CAP_SSE2:0)|(SDL_HasSSE3()?CAP_SSE3:0)|
               (SDL_HasAVX2()?CAP_AVX2:0)|(SDL_HasNEON()?CAP_NEON:0);
  uint32_t arch=0;
#if defined(__x86_64__) || defined(_M_X64)
  arch=1;
#elif defined(__i386__) || defined(_M_IX86)
  arch=2;
#elif defined(__aarch64__) || defined(_M_ARM64)
  arch=3;
#elif defined(__arm__) || defined(_M_ARM)
  arch=4;
#endif
  /* Only the existing portable C simulation is offered. CPU availability is
   * diagnostic and does not switch execution, PPU, or APU backends. */
  snes_netplay_set_profile_caps(0x58334331u,cpu,1,1,arch,x3_net_profile_settled);
}
static const char *x3_net_cosmetic_allow(void){
  static char allow[112];char digest[65];
  if(!snes_mod_runtime_package_digest_c(x3_net_package,"0.0.1",digest,sizeof(digest)))return "";
  snprintf(allow,sizeof(allow),"%s@0.0.1#%s",x3_net_package,digest);
  return allow;
}
static int x3_net_temporary_plan(void){
  const RecompLauncherCModProvider *mods=snes_mod_runtime_launcher_provider_c();
  return mods && snes_mod_runtime_begin_temporary_c() &&
      mods->feature_enable(mods->ctx,x3_net_package,"coop",1);
}
static void x3_net_mode_changed(int enabled){
  if(enabled)(void)x3_net_temporary_plan();
  else snes_mod_runtime_end_temporary_c();
}
static void x3_net_configure_launcher(RecompLauncherCGameInfo *info){
  if(info->netplay_supported && info->mods && info->netplay)
    info->netplay_mode_changed=x3_net_mode_changed;
}
static int x3_net_prepare(int from_lobby,char *reason,size_t cap){
  const char *const overrides[]={"SNESRECOMP_X3_COOP_PROTOTYPE","SNESRECOMP_X3_COOP_INDEPENDENT",
      "SNESRECOMP_X3_COOP_P1","SNESRECOMP_X3_COOP_DOORS","SNESRECOMP_X3_COOP_VIEW",
      "SNESRECOMP_EXECUTION_MODE","SNESRECOMP_FORCE_FLOOR"};
  for(unsigned i=0;i<sizeof(overrides)/sizeof(*overrides);++i)if(getenv(overrides[i])){
    snprintf(reason,cap,"Clear the diagnostic setting %s before joining a session.",overrides[i]);return 0;
  }
  bool oracle_active=false;
#ifdef ENABLE_ORACLE_BACKEND
  oracle_active=g_config.enable_snes9x_oracle;
#endif
  if(MmxReplayPlaying() || oracle_active){
    snprintf(reason,cap,"Netplay does not support an oracle backend or diagnostic input replay.");return 0;
  }
  if(!from_lobby){
    if(!x3_net_temporary_plan()){
      snprintf(reason,cap,"Restore the bundled X / Zero Co-op package.");return 0;
    }
    snes_mod_runtime_set_cosmetic_allow_c(x3_net_cosmetic_allow());
  }
  if(!snes_mod_runtime_feature_enabled_c(x3_net_package,"coop")){
    snprintf(reason,cap,"Enable X / Zero Co-op in the room's Mods settings.");return 0;
  }
  return 1;
}
static int x3_net_ready(char *reason,size_t cap){
  if(!X3CoopSettingsEnabled() || !x3_net_identify()){
    snprintf(reason,cap,"Could not activate co-op or identify the running executable.");return 0;
  }
  /* Hardware OBJ quotas affect guest-visible PPU flags. Keep the user's
   * selected setting, but require the same choice before either frame0. */
  x3_net_sim_settings=g_config.no_sprite_limits?1u:0u;
  snes_state_digest_set_extra(X3CoopRuntimeDigest);
  x3_net_capabilities();return 1;
}
static void x3_net_view(void){
  if(!snes_netplay_active())return;
  bool independent=X3CoopViewsIndependent();
  if(!X3CoopRuntimeReady()){
    char mode[32]={0};
    snes_mod_runtime_feature_option_value_c(x3_net_package,"coop","cameras",mode,sizeof(mode));
    independent=!strcmp(mode,"independent");
  }
  X3Display_SetLocalSplit(false);
  X3Display_SetPresentationSeat(independent?snes_netplay_local_slot():-1);
}
#if SNESRECOMP_TRACE
extern uint8_t g_ram[0x20000];
static bool x3_net_qa(void){
  static int enabled=-1;
  if(enabled<0){const char *value=getenv("SNESRECOMP_X3_NET_QA");enabled=value && !strcmp(value,"1");}
  return enabled && snes_netplay_active() && !strcmp(snes_netplay_transport_name(),"lan");
}
static bool x3_net_combat_qa(void){
  static int enabled=-1;if(enabled<0){const char *value=getenv("SNESRECOMP_X3_NET_QA_COMBAT");enabled=value && !strcmp(value,"1");}return enabled!=0;
}
static uint16_t x3_net_diagnostic_pad(uint32_t tick,int seat,uint16_t live){
  if(!x3_net_qa())return live;
  static const unsigned start[]={600,726,852,978};
  if(seat==0)for(unsigned i=0;i<sizeof(start)/sizeof(*start);++i){
    if(tick<start[i] || tick>=start[i]+6)continue;
    static unsigned shown;
    if(!(shown&(1u<<i))){shown|=1u<<i;fprintf(stderr,"[x3-net-qa] P1 Start tick=%u phase=%u/%u/%u\n",
        (unsigned)tick,g_ram[0xd1],g_ram[0xd2],g_ram[0xd3]);}
    return 0x008;
  }
  const X3CoopState *s=X3CoopRuntimeState();
  if(!s->initialized){
    /* Confirm actual opening dialogue pages, then leave its native closing
     * animation and actor enrollment in control of when play begins. */
    if(seat==0 && g_ram[0xcc8] && g_ram[0xcca]==0x0e && tick%120<4)return 0x001;
    return 0;
  }
  static uint32_t enrolled;
  if(!enrolled)enrolled=tick;
  uint32_t age=tick-enrolled;
  if(x3_net_combat_qa())return age>=16 && (age<232 || (age<360 && age%4==0))?0x002:0;
  if(seat==1 && age>=60 && age<110)return 0x040;
  if(seat==0 && age>=140 && age<180)return 0x080;
  return 0;
}
static void x3_net_diagnostic_progress(void){
  if(!x3_net_qa())return;
  uint32_t tick=snes_netplay_sim_tick();
  const X3CoopState *s=X3CoopRuntimeState();
  static uint32_t reported,enrolled;
  if(s->initialized && !enrolled)enrolled=tick;
  int age=enrolled?(int)(tick-enrolled):-1;
  bool combat_cut=x3_net_combat_qa() && (age==16 || age==200 || age==228 || age==232 || age==233 || age==236 || age==240 || age==260 || age==300 || age==340 || age==360 || age==400 || age==480);
  if(!combat_cut && age!=0 && age!=50 && age!=130 && age!=210 && tick%600)return;
  if(tick==reported)return;
  reported=tick;
  uint8_t personal[4096];size_t personal_size=X3CoopEncodedSize();uint32_t personal_hash=2166136261u;
  if(personal_size>sizeof(personal) || !X3CoopEncode(s,personal,personal_size))personal_hash=0;
  else for(size_t i=0;i<personal_size;++i){personal_hash^=personal[i];personal_hash*=16777619u;}
  fprintf(stderr,"[x3-net-qa] tick=%u age=%d view=%d initialized=%d life=%u/%u x=%u/%u phase=%u/%u/%u stage=%u characters=%u/%u stock=%u personal=%08x\n",(unsigned)tick,age,
      X3Display_PresentationSeat(),s->initialized,s->players[0].life,s->players[1].life,
      s->players[0].body[5]+256u*s->players[0].body[6],s->players[1].body[5]+256u*s->players[1].body[6],
      g_ram[0xd1],g_ram[0xd2],g_ram[0xd3],g_ram[0x1fae],s->players[0].character,s->players[1].character,X3CoopRuntimeStockZero(),personal_hash);
  if(x3_net_combat_qa()){
    unsigned shots[2]={0,0};for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)shots[seat]+=s->players[seat].shots[i]!=0;
    uint32_t fx=2166136261u;for(unsigned i=0x1818;i<0x1d18;++i){fx^=g_ram[i];fx*=16777619u;}
    fprintf(stderr,"[x3-net-combat] tick=%u age=%d charge=%u/%u shots=%u/%u fx=%08x episodes=%u resim=%llu desync=%u confirmed=%u life=%u/%u\n",(unsigned)tick,age,s->players[0].body[0x58],s->players[1].body[0x58],shots[0],shots[1],fx,snes_netplay_rb_episode_count(),(unsigned long long)snes_netplay_rb_resim_ticks(),snes_netplay_rb_desync_count(),snes_netplay_rb_confirmed_through(),s->players[0].life,s->players[1].life);
  }
}
#endif
#endif
