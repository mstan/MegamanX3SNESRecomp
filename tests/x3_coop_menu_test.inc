/* Original pause entry/render/restore is a prerequisite for paired controls. */
static void native_menu_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"pause rendering starts from actual two-actor gameplay");
  X3Display_SetLocalSplit(true);X3Display_SetPresentationSeat(-1);
  frame(SNES_PAD_START);for(unsigned i=0;i<90;++i)frame(0);
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
  memcpy(pixels,g_ppu->renderBuffer,256u*224u*4u);
  check(RtlLoadSnapshotFromMemory(paused,after),"paused co-op snapshot restores with its native UI");
  g_rtl_game_info->draw_ppu_frame();
  same(pixels,256u*224u*4u,g_ppu->renderBuffer,256u*224u*4u,
       "co-op pause retains exactly the native menu pixels without partner ghosts or borrowed colors");
  size_t restored=RtlSaveSnapshotToMemory(work,capacity);
  check(RtlLoadSnapshotFromMemory(paused,after),"paused state replays its idle operation");
  size_t immediate=RtlSaveSnapshotToMemory(expected,capacity);
  same(paused,after,expected,immediate,"native paused co-op immediate restore");
  (void)restored;g_config.widescreen=wide;
  frame(SNES_PAD_START);for(unsigned i=0;i<120;++i)frame(0);
  check(!g_ram[0x1f37] && X3CoopRuntimeReady(),"real native Start resumes gameplay after pause");
  unsigned x=coop_x(1);for(unsigned i=0;i<24;++i)frame(SNES_PAD_RIGHT<<12);
  check(coop_x(1)>x,"partner retains actual native control after the original pause exit");
  free(paused);free(pixels);check(RtlLoadSnapshotFromMemory(original,size),"pause prerequisite checks restore original gameplay");
}
