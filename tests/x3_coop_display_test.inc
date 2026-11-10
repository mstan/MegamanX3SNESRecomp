#include "../src/x3_display.h"
#include "../src/x3_native_resources.h"
static void native_resource_checks(uint8_t *original,size_t size,uint8_t *bytes,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"native resource fixture restores");
  const unsigned keys[]={0,2,40};
  for(unsigned n=0;n<3;++n){
    unsigned key=keys[n];size_t length=0;
    check(X3ResourceDecode(g_snes->cart->rom,g_snes->cart->romImageSize,key,bytes,capacity,&length),
          "native compressed stage resource decodes");
    unsigned cache=g_ram[0x18000+key*2]|g_ram[0x18001+key*2]<<8;
    check(cache+length<=32768 && !memcmp(bytes,g_ram+0x10000+cache,length),
          "read-only resource decoder matches the actual native decompressed cache");
    if(key==2){
      X3ResourceSegment segments[X3_RESOURCE_SEGMENTS];unsigned count=0;
      check(X3ResourceLayout(g_snes->cart->rom,g_snes->cart->romImageSize,key,0x1000,segments,&count) && count,
            "native enemy file uses its original upload segmentation");
      bool equal=true;
      for(unsigned j=0;j<count;++j){const X3ResourceSegment *s=&segments[j];
        check(s->source+s->length<=length,"native segment fits its decompressed file");
        for(unsigned i=0;i<s->length;++i)
          if(bytes[s->source+i]!=(uint8_t)(g_ppu->vram[s->destination+i/2]>>((i&1)*8)))equal=false;
      }
      check(equal,"read-only native upload layout matches actual enemy VRAM bytes");
      uint16_t colors[256]={0};
      check(X3ResourcePalette(g_snes->cart->rom,g_snes->cart->romImageSize,8,64,colors),"native enemy palette list resolves");
      check(!memcmp(colors+0xc0,g_ppu->cgram+0xc0,32),"read-only resource palette matches native enemy CGRAM");
    }
  }
}
static void remote_resource_checks(uint8_t *original,size_t size,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"remote resource setup restores");
  X3CoopState setup=*X3CoopRuntimeState();
  setup.players[1].body[5]=0x64;setup.players[1].body[6]=5; /*1380*/
  setup.players[1].body[8]=0x70;setup.players[1].body[9]=2; /*624*/
  memset(setup.players[1].body+0x1a,0,6);
  memcpy(work,original,size);size_t offset=contact_context_offset(original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),
        "remote room teleport precondition restores");
  X3CoopViewsConfigure(true);
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  views.camera[0][0]=256;views.camera[0][1]=1024;
  views.camera[1][0]=1280;views.camera[1][1]=512;
  for(unsigned seat=0;seat<2;++seat){
    views.limits[seat][0]=views.current[seat][0]=seat?1280:256;
    views.limits[seat][1]=views.current[seat][1]=seat?1536:1024;
    views.limits[seat][2]=views.current[seat][2]=seat?512:1024;
    views.limits[seat][3]=views.current[seat][3]=seat?512:1024;
  }
  /* The teleport/camera setup bypasses route/camera qualification. Original
   * union admission and the actual enemy constructor still decide the result. */
  X3CoopViewsSetState(&views);CpuState resume=g_cpu;
  view_native_call(0x00de3a,0,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.DB=resume.DB;g_cpu.PB=resume.PB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  for(unsigned i=0;i<3;++i)frame(0);
  /* Class2F is the authored Ride Armor station: initialization relocates it
   * from event1424,592 to its stage-table position1712,432. Move the QA actor
   * into that actual upper room, using its native E23A limits. */
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  setup=*X3CoopRuntimeState();
  setup.players[1].body[5]=0x7c;setup.players[1].body[6]=6; /*1660*/
  setup.players[1].body[8]=0xd0;setup.players[1].body[9]=1; /*464*/
  memset(setup.players[1].body+0x1a,0,6);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,prepared),
        "native Ride Armor station room teleport restores");
  views=*X3CoopViewsState();views.camera[1][0]=1532;views.camera[1][1]=304;
  views.limits[1][1]=views.current[1][1]=2048;views.limits[1][2]=views.current[1][2]=256;
  X3CoopViewsSetState(&views);
  for(unsigned i=0;i<3;++i)frame(0);
  bool found=false;
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  for(unsigned i=0;i<visible->count;++i){const X3CoopWorldDraw *d=&visible->actors[i];
    if(d->address>=0xd18 && d->address<0x10d8 && d->body[10]==0x2f && d->body[0x16]==0x3f)found=true;
  }
  if(!found){
    fprintf(stderr,"remote resource: draws=%u camera=%u,%u p2=%u,%u phase=%u\n",visible->count,
        visible->views[1][0],visible->views[1][1],coop_x(1),X3CoopRuntimeState()->players[1].body[8]|X3CoopRuntimeState()->players[1].body[9]<<8,g_ram[0x1f18]);
    for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d])
      fprintf(stderr,"remote enemy %04x class=%02x state=%02x/%02x group=%02x xy=%u,%u visible=%02x\n",d,g_ram[d+10],g_ram[d+1],g_ram[d+2],g_ram[d+0x16],g_ram[d+5]|g_ram[d+6]<<8,g_ram[d+8]|g_ram[d+9]<<8,g_ram[d+14]);
  }
  check(found,"actual stage1 Ride Armor station submits native group3F in P2's remote room");
  check(g_ram[0x1f18]==0 && !g_ram[0x1823c] && !g_ram[0x1833c],
        "remote actor is visible before the anchor loads file3C's placement and palette");
  memcpy(work,g_ram,0x20000);uint64_t clock=g_cpu.master_cycles;
  X3Display_SetPresentationSeat(1);CaptureSimulationFrame(1);
  check(X3Display_RenderedPrivateFrame(),"remote resource screenshot uses actual private scanout");
  check(clock==g_cpu.master_cycles && !memcmp(work,g_ram,0x20000),
        "resolving and painting remote resources does not mutate the CPU or shared world");
  const char *output=getenv("X3_COOP_REMOTE_RESOURCE_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"remote resource visual fixture saves");
  const char *image=getenv("X3_COOP_DISPLAY_OUTPUT");
  if(image && *image){char path[1024];snprintf(path,sizeof(path),"%s-remote.ppm",image);
    WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);}
  X3Display_SetPresentationSeat(-1);
  check(RtlLoadSnapshotFromMemory(original,size),"remote resource probes restore");(void)capacity;
}
static void native_display_checks(const char *tag,uint8_t *original,size_t size,uint8_t *native,uint8_t *private,size_t capacity){
  bool wide=g_config.widescreen;g_config.widescreen=false;
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
  frame(0);frame(0);
  check(g_ppu->renderPitch==256*4,"native display comparison uses an actual256px field");
  memcpy(native,g_ppu->renderBuffer,256u*224u*4u);
  X3CoopViewsConfigure(true);X3Display_SetPresentationSeat(X3CoopRuntimeState()->anchor);
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
