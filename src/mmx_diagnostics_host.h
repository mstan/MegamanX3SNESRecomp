/* Diagnostic hooks around the framework-owned X2/X3 desktop adapter. */
#include "desktop/host_main.h"
#include "desktop/config.h"
#include "desktop/sdl_compat.h"
#include "mmx_replay.h"
#include <stdlib.h>
#include <string.h>
static void (*diagnostic_prepare)(int,int,int *,int *);
static void diagnostic_prepare_frame(int dw,int dh,int *w,int *h) {
  diagnostic_prepare(dw,dh,w,h);
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
  diagnostic_prepare=base->prepare_frame;
  game.prepare_frame=diagnostic_prepare_frame;
  game.after_run_frame=diagnostic_after_frame;
  game.after_config=diagnostic_after_config;
  game.on_shutdown=MmxReplayShutdown;
  int result=snesrecomp_desktop_main(&game,argc,argv);
  MmxReplayShutdown();
  return MmxReplayPlaying() && (MmxReplayFailed() || !MmxReplayFinished()) ? 2:result;
}
