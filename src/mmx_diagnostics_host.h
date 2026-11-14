/* Diagnostic hooks around the framework-owned X2/X3 desktop adapter. */
#include "desktop/host_main.h"
#include "desktop/config.h"
#include "desktop/sdl_compat.h"
#include "mmx_replay.h"
#include "x3_display.h"
#include "x3_coop_settings.h"
#include "x3_rtl.h"
#include "snes/ppu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "x3_coop_netplay_host.inc"
static void (*diagnostic_prepare)(int,int,int *,int *);
static SnesDesktopHostGame *diagnostic_game;
static int diagnostic_native_widescreen;
static void diagnostic_prepare_frame(int dw,int dh,int *w,int *h) {
  X3CoopSettingsPresentation();
#if defined(SNES_HAS_LOBBY_CLIENT) && defined(SNESRECOMP_NET_ROLLBACK)
  x3_net_view();
#endif
  bool coop=X3CoopSettingsEnabled();
  /* The compositor samples extra pixels from the completed world. Leave the
   * guest PPU and activation budgets on their common co-op settings. */
  diagnostic_game->native_widescreen=coop?0:diagnostic_native_widescreen;
  int panes=X3Display_LocalSplit()?2:1;
  if(coop){
    double target=g_config.widescreen && !X3UseNativeFrame() && dw>0 && dh>0?(double)dw/dh/panes:0;
    SnesDisplayFrame field=SnesDisplayAspect_ComputeAdaptiveFrame(256,224,kPpuMaxFrameWidth,target,
        SnesDisplayAspect_Clamp(g_config.display_aspect));
    *w=field.width*panes;*h=224;
  }
  else diagnostic_prepare(dw,dh,w,h);
  int extra; MmxReplayViewport(w,&extra);
  X3Display_SetCoopPanelWidth(coop && *w>0 && *w%panes==0?(unsigned)(*w/panes):256);
}
/* Read-only capture of the actual compositor output for private peer QA. */
static int diagnostic_draw_frame(uint8_t *dst,size_t pitch,const uint8_t *field,int width,int height,double alpha){
  int drawn=X3Display_DrawCoopFrame(dst,pitch,field,width,height,alpha);
#if defined(SNES_HAS_LOBBY_CLIENT) && defined(SNESRECOMP_NET_ROLLBACK) && SNESRECOMP_TRACE
  const char *directory=getenv("SNESRECOMP_X3_NET_QA_CAPTURE");static uint32_t last;
  uint32_t tick=snes_netplay_sim_tick();
  if(directory && *directory && x3_net_qa() && ((tick>=5400 && tick<=5480) || (tick>=5520 && tick<=5680)) && !(tick%8) && tick!=last && width>0 && width<=kPpuMaxFrameWidth){
    last=tick;char path[4096];int n=snprintf(path,sizeof(path),"%s/%05u.ppm",directory,tick);
    if(n>0 && (size_t)n<sizeof(path)){
      FILE *f=fopen(path,"wb");
      if(f){const uint8_t *source=drawn?dst:field;size_t source_pitch=drawn?pitch:256u*4;uint8_t row[kPpuMaxFrameWidth*3];
        fprintf(f,"P6\n%d %d\n255\n",width,height);
        for(int y=0;y<height;++y){for(int x=0;x<width;++x){const uint8_t *pixel=source+(size_t)y*source_pitch+(size_t)x*4;row[x*3]=pixel[2];row[x*3+1]=pixel[1];row[x*3+2]=pixel[0];}fwrite(row,3,(size_t)width,f);}
        fclose(f);const X3CoopState *s=X3CoopRuntimeState();unsigned seat=(unsigned)X3Display_PresentationSeat();
        X3CoopView view=X3CoopViewsForPlayer(g_ram,s,seat,false);
        fprintf(stderr,"[x3-net-view-capture] tick=%u seat=%u camera=%d,%d size=%d,%d private=%u path=%s\n",tick,seat,view.x,view.y,width,height,X3Display_RenderedPrivateFrame(),path);
      }
    }
  }
#endif
  return drawn;
}
static void diagnostic_after_frame(const SnesDesktopHostFrameStats *stats) {
  (void)stats;
#if defined(SNES_HAS_LOBBY_CLIENT) && defined(SNESRECOMP_NET_ROLLBACK) && SNESRECOMP_TRACE
  x3_net_diagnostic_progress(stats->frame);
#endif
  if(MmxReplayPlaying() && (MmxReplayFinished() || MmxReplayFailed())) {
    SDL_Event quit={0}; quit.type=SDL_QUIT; SDL_PushEvent(&quit);
  }
}
static void diagnostic_after_config(void) {
  if(getenv("MMX_REPLAY_FAST")) {
    g_config.disable_frame_delay=true;
    g_config.fullscreen=0;
    g_config.output_method=kOutputMethod_SDL;
  }
}
static void diagnostic_viewport(int fw,int fh,int dw,int dh,SnesDisplayViewport *out) {
  SnesDisplayAspect_ComputeViewport(fw,fh,dw,dh,SnesDisplayAspect_Clamp(g_config.display_aspect),
                                   g_config.ignore_aspect_ratio,false,out);
}
static int MmxDiagnosticDesktopMain(const SnesDesktopHostGame *base,int argc,char **argv) {
  MmxStartupLogging();
  for(int arg=1;arg+1<argc;++arg)
    if(!strcmp(argv[arg],"--config")) MmxReplaySetConfigPath(argv[arg+1]);
  for(int arg=1;arg<argc;++arg) {
    if(strcmp(argv[arg],"--replay-diagnostic"))continue;
    if(arg+1==argc || !MmxReplayPlayback(argv[arg+1]))return 2;
    for(int i=arg;i+2<=argc;++i)argv[i]=argv[i+2];
    argc-=2;break;
  }
  SnesDesktopHostGame game=*base;
  game.num_players=2;
  diagnostic_game=&game;diagnostic_native_widescreen=game.native_widescreen;
  diagnostic_prepare=game.prepare_frame;
  game.prepare_frame=diagnostic_prepare_frame;
  game.draw_frame=diagnostic_draw_frame;
  game.compute_viewport=diagnostic_viewport;
  game.window_base_width=X3CoopSettingsWindowWidth;
  game.after_run_frame=diagnostic_after_frame;
  game.after_config=diagnostic_after_config;
  game.on_shutdown=MmxReplayShutdown;
#if defined(SNES_HAS_LOBBY_CLIENT) && defined(SNESRECOMP_NET_ROLLBACK)
  game.configure_launcher=x3_net_configure_launcher;
  game.prepare_netplay=x3_net_prepare;
  game.netplay_ready=x3_net_ready;
  game.netplay_cosmetic_allow=x3_net_cosmetic_allow;
  game.netplay_requires_seated_rollback=1;
#if SNESRECOMP_TRACE
  game.netplay_diagnostic_pad=x3_net_diagnostic_pad;
#endif
#endif
  int result=snesrecomp_desktop_main(&game,argc,argv);
  MmxReplayShutdown();
  return MmxReplayPlaying() && (MmxReplayFailed() || !MmxReplayFinished()) ? 2:result;
}
