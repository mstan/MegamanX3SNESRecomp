/* Run the shared real-ROM state checks with the desktop timing contract. */
#define main MmxStateChecksMain
#include "../snesrecomp/runner/tests/mmx_state_runtime.c"
#undef main
int main(int argc,char **argv) {
  RtlEnableExtendedFrameTiming();
  int result=MmxStateChecksMain(argc,argv);
  if(!result && argc==2) {
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
