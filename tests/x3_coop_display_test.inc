#include "../src/x3_display.h"
static void native_display_checks(const char *tag,uint8_t *original,size_t size,uint8_t *native,uint8_t *private,size_t capacity){
  bool wide=g_config.widescreen;g_config.widescreen=false;
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
  frame(0);frame(0);
  check(g_ppu->renderPitch==256*4,"native display comparison uses an actual256px field");
  memcpy(native,g_ppu->renderBuffer,256u*224u*4u);
  X3CoopViewsConfigure(true);X3Display_SetPresentationSeat(0);
  check(X3Display_BeginIndependentFrame(g_ppu),"private primary compositor is active for the comparison");
  g_rtl_game_info->draw_ppu_frame();
  check(X3Display_RenderedPrivateFrame(),"actual render call uses the private compositor rather than its native fallback");
  memcpy(private,g_ppu->renderBuffer,256u*224u*4u);
  unsigned differences=0,nonblack=0;int left=256,right=0,top=224,bottom=0;
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<256;++x){
    unsigned at=(y*256+x)*4;
    if(native[at] || native[at+1] || native[at+2])++nonblack;
    if(memcmp(native+at,private+at,3)){
      ++differences;if((int)x<left)left=x;if((int)x>right)right=x;if((int)y<top)top=y;if((int)y>bottom)bottom=y;
    }
  }
  fprintf(stderr,"private/native display comparison %s: differences=%u nonblack=%u bounds=%d,%d..%d,%d\n",tag,differences,nonblack,left,top,right,bottom);
  const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");
  if(out && *out){char path[1024];
    snprintf(path,sizeof(path),"%s-%s-native.ppm",out,tag);WritePpm(path,native,256*4,256,224,0,false);
    snprintf(path,sizeof(path),"%s-%s-private.ppm",out,tag);WritePpm(path,private,256*4,256,224,0,false);
  }
  check(nonblack>1000,"display comparison contains actual native stage content");
  check(!differences,"private primary view matches the native world, body and health HUD pixels");
  X3Display_SetPresentationSeat(-1);g_config.widescreen=wide;
  check(RtlLoadSnapshotFromMemory(original,size),"display probes restore their starting machine");
  (void)capacity;
}
static void independent_display_checks(uint8_t *original,size_t size,uint8_t *left,uint8_t *right,size_t capacity){
  bool wide=g_config.widescreen;g_config.widescreen=false;
  uint8_t *world=malloc(0x20000),*split=malloc(512u*224u*4u);
  check(world && split,"split display comparison buffers allocate");memcpy(world,g_ram,0x20000);
  uint64_t clock=g_cpu.master_cycles;
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(0);CaptureSimulationFrame(1);
  check(X3Display_RenderedPrivateFrame(),"separated P1 display uses its private renderer");
  memcpy(left,g_ppu->renderBuffer,256u*224u*4u);
  X3Display_SetPresentationSeat(1);g_rtl_game_info->draw_ppu_frame();
  check(X3Display_RenderedPrivateFrame(),"separated P2 display uses its private renderer");
  memcpy(right,g_ppu->renderBuffer,256u*224u*4u);
  check(memcmp(left,right,256u*224u*4u)!=0,"separated players render distinct authored world regions");
  X3Display_SetLocalSplit(true);g_rtl_game_info->draw_ppu_frame();
  check(X3Display_RenderedPrivateFrame() && X3Display_DrawLocalSplit(split,512*4,NULL,512,224,1),
        "split display composes both independently rendered fields");
  bool equal=true;
  for(unsigned y=0;y<224;++y)
    if(memcmp(split+y*512*4,left+y*256*4,256*4) || memcmp(split+(y*512+256)*4,right+y*256*4,256*4))equal=false;
  check(equal,"split fields exactly match each player's selected independent view");
  check(g_cpu.master_cycles==clock && !memcmp(world,g_ram,0x20000),
        "selected and split rendering do not advance the CPU or mutate shared world RAM");
  const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");
  if(out && *out){char path[1024];
    snprintf(path,sizeof(path),"%s-p1.ppm",out);WritePpm(path,left,256*4,256,224,0,false);
    snprintf(path,sizeof(path),"%s-p2.ppm",out);WritePpm(path,right,256*4,256,224,0,false);
    snprintf(path,sizeof(path),"%s-split.ppm",out);WritePpm(path,split,512*4,512,224,0,false);
  }
  size_t baseline=0;
  for(int mode=-1;mode<3;++mode){
    check(RtlLoadSnapshotFromMemory(original,size),"display-mode simulation comparison starts from the same machine");
    X3Display_SetLocalSplit(mode==2);X3Display_SetPresentationSeat(mode>=0 && mode<2?mode:-1);
    for(unsigned i=0;i<10;++i)frame(0);
    if(mode==-1)baseline=RtlSaveSnapshotToMemory(left,capacity);
    else {size_t candidate=RtlSaveSnapshotToMemory(right,capacity);
      same(left,baseline,right,candidate,"local presentation choice preserves the complete simulation snapshot");}
  }
  free(world);free(split);X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);g_config.widescreen=wide;
  check(RtlLoadSnapshotFromMemory(original,size),"split display probes restore their starting machine");(void)capacity;
}
