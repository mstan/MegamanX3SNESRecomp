/* Original pause entry/render/restore is a prerequisite for paired controls. */
#include "../src/x3_coop_menu.h"
static bool menu_fade_entered;
static void menu_fade_enter_probe(CpuState *cpu,uint32_t pc){
  (void)cpu;(void)pc;menu_fade_entered=true;
}
static void native_menu_fade_checks(uint8_t *original,size_t size,size_t capacity){
  enum { FIELDS=32,PICTURE=512*224*4 };
  uint8_t *reference=malloc((size_t)FIELDS*PICTURE),*image=malloc(PICTURE),*cut=malloc(capacity);
  uint8_t *mid=malloc(capacity),*future=malloc(capacity),*replayed=malloc(capacity);
  check(reference && image && cut && mid && future && replayed,"pause fade allocates bounded native field references");
  check(interp_bridge_add_pre_opcode_hook(0x00c5ef,menu_fade_enter_probe),"pause fade observes original menu initialization after black");
  for(unsigned roster=0;roster<2;++roster)for(unsigned mode=0;mode<4;++mode){
    if(roster){const char *path=getenv("X3_COOP_MENU_FADE_REVERSED_STATE");
      check(path && RtlLoadSnapshot(path),"pause fade loads the actually constructed reversed roster");}
    else check(RtlLoadSnapshotFromMemory(original,size),"pause fade restores its native gameplay");
    X3CoopViewsConfigure(true);
    X3Display_SetLocalSplit(mode==2);X3Display_SetPresentationSeat(mode==3?1:-1);
    bool wide=g_config.widescreen;g_config.widescreen=false;
    size_t n=RtlSaveSnapshotToMemory(cut,capacity);unsigned counts[2]={0};
    unsigned width=mode==2?512:mode==1 || mode==3?342:256;
    for(unsigned owner=0;owner<2;++owner){
      check(RtlLoadSnapshotFromMemory(cut,n),"either requester starts at the same complete gameplay field");
      menu_fade_entered=false;
      size_t mn=0;unsigned remaining=0;
      for(unsigned field=0;field<FIELDS && !menu_fade_entered;++field){
        frame(field?0:(uint32_t)SNES_PAD_START<<(owner*12));
        if(mn)++remaining;
        if(menu_fade_entered)break;
        check(X3CoopRuntimeEnabled(),"original pause fade retains co-op scheduling");
        if(!X3CoopNativePauseActive(g_ram) || !(g_ppu->inidisp&15))continue;
        X3Display_SetCoopPanelWidth(mode==1 || mode==3?342:256);g_rtl_game_info->draw_ppu_frame();
        if(!X3Display_DrawCoopFrame(image,width*4,g_ppu->renderBuffer,width,224,0))
          for(unsigned y=0;y<224;++y)memcpy(image+y*256u*4u,g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch,256*4);
        unsigned picture=counts[owner]++;
        uint8_t *expected=reference+(size_t)picture*PICTURE;
        if(!owner)memcpy(expected,image,width*224u*4u);
        else if(memcmp(expected,image,width*224u*4u)){
          fprintf(stderr,"pause fade mismatch roster%u mode%u field%u image%u brightness%u character%u anchor%u\n",roster,mode,field,picture,g_ppu->inidisp&15,g_ram[X3_COOP_BODY+0xb6],X3CoopRuntimeState()->anchor);
          check(0,"either request preserves identical native gameplay sprites throughout the original fade");
        }
        if(field==3){
          mn=RtlSaveSnapshotToMemory(mid,capacity);
          check(RtlLoadSnapshotFromMemory(mid,mn),"actual gameplay fade restores before the original native menu initializer");
          same(mid,mn,replayed,RtlSaveSnapshotToMemory(replayed,capacity),"pause gameplay fade restores the complete machine immediately");
          memcpy(replayed,mid,mn);replayed[contact_menu_offset(mid,mn)+124]=2;
          check(!RtlLoadSnapshotFromMemory(replayed,mn),"malformed gameplay-fade state is refused transactionally");
          same(mid,mn,replayed,RtlSaveSnapshotToMemory(replayed,capacity),"refused gameplay fade preserves the complete machine");
        }
      }
      check(menu_fade_entered && counts[owner]>4,"actual Start traverses visible gameplay fade before native menu initialization");
      size_t fn=RtlSaveSnapshotToMemory(future,capacity);
      check(mn && remaining && RtlLoadSnapshotFromMemory(mid,mn),"pause replay restores its actual middle-of-fade field");
      menu_fade_entered=false;for(unsigned i=0;i<remaining;++i)frame(0);
      same(future,fn,replayed,RtlSaveSnapshotToMemory(replayed,capacity),"gameplay fade through original menu initialization replays the entire machine exactly");
    }
    check(counts[0]==counts[1],"both requesters retain original gameplay fade cadence");
    fprintf(stderr,"native pause FADE roster%u mode%u width%u visible fields%u identical for both requesters, full replay exact\n",roster,mode,width,counts[0]);
    g_config.widescreen=wide;
  }
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3Display_SetCoopPanelWidth(256);
  free(reference);free(image);free(cut);free(mid);free(future);free(replayed);
  check(RtlLoadSnapshotFromMemory(original,size),"pause fade restores its gameplay prerequisite");
}
static void native_menu_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"pause rendering starts from actual two-actor gameplay");
  X3Display_SetLocalSplit(true);X3Display_SetPresentationSeat(-1);
  fprintf(stderr,"pause entry native phase%u/%u pause%u lock%u life%u/%u hp%u/%u flags7e%u/%u\n",g_ram[0xd1],g_ram[0xd2],g_ram[0x1f37],g_ram[0x1f4f],X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life,contact_hp(0),contact_hp(1),X3CoopRuntimeState()->players[0].body[0x7e],X3CoopRuntimeState()->players[1].body[0x7e]);
  unsigned requesting_seat=getenv("X3_COOP_NATIVE_MENU_SEAT")?(unsigned)atoi(getenv("X3_COOP_NATIVE_MENU_SEAT")):0;
  check(requesting_seat<2,"native pause image uses a real requesting input seat");
  bool original_menu=getenv("X3_COOP_MENU_NATIVE_ORIGINAL")!=NULL;
  if(original_menu){X3CoopState solo=*X3CoopRuntimeState();check(X3CoopProject(&solo,g_ram,requesting_seat),"original menu reference installs the actual requesting character");X3CoopRuntimeEnable(false);}
  frame((uint32_t)SNES_PAD_START<<(original_menu?0:requesting_seat*12));for(unsigned i=0;i<90;++i)frame(0);
  if(original_menu){
    check(g_ram[0x1f37]&1,"genuine original native Start reaches its own pause screen");
    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);g_rtl_game_info->draw_ppu_frame();
    const char *path=getenv("X3_COOP_NATIVE_MENU_OUTPUT");check(path && *path,"original menu reference has an explicit private capture destination");
    WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);
    check(RtlLoadSnapshotFromMemory(original,size),"original native pause inspection restores its prerequisite");return;
  }
  fprintf(stderr,"pause after native phase%u/%u pause%u lock%u enabled%u\n",g_ram[0xd1],g_ram[0xd2],g_ram[0x1f37],g_ram[0x1f4f],X3CoopRuntimeEnabled());
  check((g_ram[0x1f37]&1) && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE &&
        X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual Start opens the original native pause with both actors retained");
  check(!X3Display_RenderedPrivateFrame(),"native pause never renders a stale private game field over its menu");
  uint8_t actors[0xb00],palettes[2][64];unsigned positions[2][2];
  memcpy(actors,g_ram+0xd18,sizeof(actors));
  for(unsigned seat=0;seat<2;++seat){memcpy(palettes[seat],X3CoopRuntimeState()->players[seat].palette,64);
    for(unsigned axis=0;axis<2;++axis)positions[seat][axis]=support_position(seat,axis?8:5);}
  size_t before=RtlSaveSnapshotToMemory(expected,capacity);
  size_t first=0;for(size_t i=16384;i+4<before;++i)if(!memcmp(expected+i,"X3P5",4)){first=i-16384;break;}
  check(first!=0,"paused frame retains its complete presentation history");
  for(unsigned i=0;i<30;++i)frame(0);
  check(!memcmp(actors,g_ram+0xd18,sizeof(actors)),"native pause freezes shared enemies, hostile shots and items once");
  size_t after=RtlSaveSnapshotToMemory(work,capacity);
  same(expected+first,X3CoopPresentationEncodedSize(),work+first,X3CoopPresentationEncodedSize(),
       "menu OAM and graphics never replace saved gameplay presentation history");
  for(unsigned seat=0;seat<2;++seat){
    check(!memcmp(palettes[seat],X3CoopRuntimeState()->players[seat].palette,64),"native UI staging never overwrites a private actor palette");
    for(unsigned axis=0;axis<2;++axis)check(positions[seat][axis]==support_position(seat,axis?8:5),"both paused bodies retain their positions");}
  uint8_t *paused=malloc(capacity),*pixels=malloc(256u*224u*4u);check(paused && pixels,"pause reference buffers allocate");
  memcpy(paused,work,after);X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
  bool wide=g_config.widescreen;g_config.widescreen=false;
  X3CoopRuntimeEnable(false);g_rtl_game_info->draw_ppu_frame();
  for(unsigned y=0;y<224;++y)memcpy(pixels+(size_t)y*256*4,g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch,256*4);
  check(RtlLoadSnapshotFromMemory(paused,after),"paused co-op snapshot restores with its native UI");
  X3CoopMenuNativeReference(true);
  g_rtl_game_info->draw_ppu_frame();
  for(unsigned y=0;y<224;++y)memcpy(work+(size_t)y*256*4,g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch,256*4);
  same(pixels,256u*224u*4u,work,256u*224u*4u,
       "co-op pause retains exactly the native menu pixels without partner ghosts or borrowed colors");
  const char *native_output=getenv("X3_COOP_NATIVE_MENU_OUTPUT");
  if(native_output && *native_output)WritePpm(native_output,work,256*4,256,224,0,false);
  if(getenv("X3_COOP_MENU_DETACHED_PROBE")){
    Ppu *view=ppu_init();uint8_t *picture=malloc(256u*224u*4u);
    check(view && picture,"native menu detached picture allocates its own device and pixels");
    memcpy(&view->inidisp,&g_ppu->inidisp,PPU_SAVESTATE_REGS_SIZE);
    memcpy(view->cgram,g_ppu->cgram,PPU_SAVESTATE_MEM_SIZE);
    memcpy(view->brightnessMult,g_ppu->brightnessMult,sizeof(view->brightnessMult));
    memcpy(view->brightnessMultHalf,g_ppu->brightnessMultHalf,sizeof(view->brightnessMultHalf));
    view->renderBuffer=picture;view->renderPitch=256*4;view->evenFrame=g_ppu->evenFrame;
    for(unsigned line=0;line<=224;++line)PpuDrawDetachedLine(view,(int)line);
    if(native_output && *native_output){char path[1024];snprintf(path,sizeof(path),"%s-detached.ppm",native_output);WritePpm(path,picture,256*4,256,224,0,false);}
    unsigned differences=0;for(unsigned y=0;y<224;++y)for(unsigned x=0;x<256;++x)
      differences+=memcmp(picture+(y*256+x)*4,g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch+(x+g_ppu->extraLeftRight)*4,3)!=0;
    fprintf(stderr,"native DETACHED MENU seat%u differences%u\n",requesting_seat,differences);
    check(!differences,"detached original PPU menu raster matches the complete native center field exactly");
    view->renderBuffer=NULL;ppu_free(view);free(picture);
  }
  X3CoopMenuNativeReference(false);
  size_t restored=RtlSaveSnapshotToMemory(work,capacity);
  check(RtlLoadSnapshotFromMemory(paused,after),"paused state replays its idle operation");
  size_t immediate=RtlSaveSnapshotToMemory(expected,capacity);
  same(paused,after,expected,immediate,"native paused co-op immediate restore");
  (void)restored;g_config.widescreen=wide;
  frame((uint32_t)SNES_PAD_START<<(requesting_seat*12));for(unsigned i=0;i<120;++i)frame(0);
  if(g_ram[0x1f37] || !X3CoopRuntimeReady()){
    X3CoopMenuInfo info={0};bool active=X3CoopMenuRead(&info);
    fprintf(stderr,"native menu EXIT FAILURE seat%u enabled%u active%u owner%u phase%u/%u pause%02x UI01/02/03/%02x/%02x/%02x page%02x input%02x%02x/%02x%02x cursor%u/%u heal%u/%u\n",requesting_seat,X3CoopRuntimeEnabled(),active,info.owner,g_ram[0xd1],g_ram[0xd2],g_ram[0x1f37],g_ram[0x1ed9],g_ram[0x1eda],g_ram[0x1edb],g_ram[0x1f0c],g_ram[0xa11],g_ram[0xa10],g_ram[0xa13],g_ram[0xa12],info.cursor[0],info.cursor[1],info.healing[0],info.healing[1]);
    check(RtlSaveSnapshot("native-pause-exit-unqualified.sav"),"failed native pause exit retains its exact current machine");
  }
  check(!g_ram[0x1f37] && X3CoopRuntimeReady(),"real native Start resumes gameplay after pause");
  unsigned x=coop_x(1);for(unsigned i=0;i<24;++i)frame(SNES_PAD_RIGHT<<12);
  check(coop_x(1)>x,"partner retains actual native control after the original pause exit");
  free(paused);free(pixels);check(RtlLoadSnapshotFromMemory(original,size),"pause prerequisite checks restore original gameplay");
}
