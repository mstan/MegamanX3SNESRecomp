/* Run the shared real-ROM state checks with the desktop timing contract. */
#define main MmxStateChecksMain
#include "../snesrecomp/runner/tests/mmx_state_runtime.c"
#undef main
int main(int argc,char **argv) {
  RtlEnableExtendedFrameTiming();
  return MmxStateChecksMain(argc,argv);
}
