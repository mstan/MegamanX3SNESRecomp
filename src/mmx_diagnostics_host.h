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
static void (*diagnostic_prepare)(int,int,int *,int *);
static SnesDesktopHostGame *diagnostic_game;
static int diagnostic_native_widescreen;
static void diagnostic_prepare_frame(int dw,int dh,int *w,int *h) {
  X3CoopSettingsPresentation();
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
static void diagnostic_after_frame(const SnesDesktopHostFrameStats *stats) {
  (void)stats;
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
  game.draw_frame=X3Display_DrawCoopFrame;
  game.compute_viewport=diagnostic_viewport;
  game.window_base_width=X3CoopSettingsWindowWidth;
  game.after_run_frame=diagnostic_after_frame;
  game.after_config=diagnostic_after_config;
  game.on_shutdown=MmxReplayShutdown;
  int result=snesrecomp_desktop_main(&game,argc,argv);
  MmxReplayShutdown();
  return MmxReplayPlaying() && (MmxReplayFailed() || !MmxReplayFinished()) ? 2:result;
}
