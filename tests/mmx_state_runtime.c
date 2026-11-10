/* Run the shared real-ROM state checks with the desktop timing contract. */
#define main MmxStateChecksMain
#include MMX_SHARED_STATE_TEST
#undef main
#include "../src/x3_coop_runtime.h"
static unsigned coop_x(unsigned seat) {
  const uint8_t *body=X3CoopRuntimeState()->players[seat].body;
  return body[5]|body[6]<<8;
}
int main(int argc,char **argv) {
  RtlEnableExtendedFrameTiming();
  int result=MmxStateChecksMain(argc,argv);
  if(!result && argc==2) {
    size_t cap=2u*1024u*1024u;
    uint8_t *saved=malloc(cap),*damaged=malloc(cap),*after=malloc(cap);
    check(saved && damaged && after,"co-op preflight buffers");
    size_t size=RtlSaveSnapshotToMemory(saved,cap),marker=0;
    check(size>32,"co-op snapshot is present");
    for(size_t i=size-4;i>0;--i)if(!memcmp(saved+i,"X3C2",4)){marker=i;break;}
    check(marker && marker+8<size,"co-op context chunk is present");
    memcpy(damaged,saved,size);damaged[marker+6]=2;
    check(!RtlLoadSnapshotFromMemory(damaged,size),"invalid actor anchor is refused before load");
    size_t after_size=RtlSaveSnapshotToMemory(after,cap);
    same(saved,size,after,after_size,"refused actor state leaves the machine unchanged");
    const char *fixture=getenv("X3_COOP_TEST_STATE");
    if(fixture && *fixture) {
      X3CoopRuntimeEnable(true);
      check(RtlLoadSnapshot(fixture),"native two-actor fixture loads");
      check(X3CoopRuntimeState()->initialized,"fixture contains enrolled actors");
      size=RtlSaveSnapshotToMemory(saved,cap);
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
