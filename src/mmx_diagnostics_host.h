/* Diagnostic hooks around the framework-owned X2/X3 desktop adapter. */
#include "desktop/host_main.h"
#include "desktop/config.h"
#include "desktop/sdl_compat.h"
#include "mmx_replay.h"
#include "x3_display.h"
#include "x3_coop_settings.h"
#include <stdlib.h>
#include <string.h>
static void (*diagnostic_prepare)(int,int,int *,int *);
static SnesDesktopHostGame *diagnostic_game;
static int diagnostic_native_widescreen;
static void diagnostic_prepare_frame(int dw,int dh,int *w,int *h) {
  X3CoopSettingsPresentation();
  bool coop=X3CoopSettingsEnabled();
  /* Local presentation width must not select different guest PPU margins.
   * Co-op currently qualifies a common native-width simulation field. */
  diagnostic_game->native_widescreen=coop?0:diagnostic_native_widescreen;
  if(coop){*w=X3Display_LocalSplit()?512:256;*h=224;}
  else diagnostic_prepare(dw,dh,w,h);
  int extra; MmxReplayViewport(w,&extra);
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
  if(X3Display_LocalSplit()){
    X3DisplayViewport view;X3Display_ComputeViewport(fw,fh,dw,dh,g_config.ignore_aspect_ratio,false,&view);
    out->x=view.x;out->y=view.y;out->width=view.width;out->height=view.height;
  }else SnesDisplayAspect_ComputeViewport(fw,fh,dw,dh,SnesDisplayAspect_Clamp(g_config.display_aspect),
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
  game.draw_frame=X3Display_DrawLocalSplit;
  game.compute_viewport=diagnostic_viewport;
  game.window_base_width=X3CoopSettingsWindowWidth;
  game.after_run_frame=diagnostic_after_frame;
  game.after_config=diagnostic_after_config;
  game.on_shutdown=MmxReplayShutdown;
  int result=snesrecomp_desktop_main(&game,argc,argv);
  MmxReplayShutdown();
  return MmxReplayPlaying() && (MmxReplayFailed() || !MmxReplayFinished()) ? 2:result;
}
