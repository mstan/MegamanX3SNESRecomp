/* The actual engine rewind ring restores an enrolled scene and discards its
 * future, including native buster pools and the visible CHR/OAM history. */
static void native_initialized_rewind_checks(const char *normal,const char *reversed,uint8_t *start,uint8_t *work,uint8_t *future,size_t capacity){
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && X3CoopRuntimeState()->initialized,"initialized rewind loads its actual native roster");
    snes_rewind_configure();for(unsigned i=0;i<12;++i){frame(0);snes_rewind_note_frame();}
    unsigned x[2]={support_position(0,5),support_position(1,5)};size_t n=RtlSaveSnapshotToMemory(start,capacity);
    uint32_t buttons=(SNES_PAD_RIGHT|SNES_PAD_Y)|((SNES_PAD_RIGHT|SNES_PAD_Y)<<12);
    for(unsigned i=0;i<6;++i){frame(buttons);snes_rewind_note_frame();}
    check(support_position(0,5)>x[0] && support_position(1,5)>x[1],"both native actors move before the initialized rewind");
    const X3CoopState *s=X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat){bool shot=false;const uint8_t *pool=s->current==seat?g_ram+X3_COOP_SHOTS:s->players[seat].shots;
      for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)shot=shot || pool[j];check(shot,"both seats create real native buster pools in the recorded future");}
    size_t f=RtlSaveSnapshotToMemory(future,capacity);CaptureSimulationFrame(1);
    size_t pixel_size=(size_t)g_ppu->renderPitch*224;uint8_t *pixels=malloc(pixel_size);check(pixels!=NULL,"initialized rewind retains its actual visible future");memcpy(pixels,g_ppu->renderBuffer,pixel_size);
    check(snes_rewind_open(),"initialized co-op opens the actual rewind interface");for(unsigned i=0;i<6;++i)snes_rewind_step(-1);snes_rewind_commit();
    size_t restored=RtlSaveSnapshotToMemory(work,capacity);same(start,n,work,restored,"actual rewind restores the complete initialized two-actor scene");
    check(X3CoopRuntimeState()->initialized && support_position(0,5)==x[0] && support_position(1,5)==x[1],"rewind retains both actors and their original positions");
    for(unsigned i=0;i<6;++i){frame(buttons);snes_rewind_note_frame();}
    restored=RtlSaveSnapshotToMemory(work,capacity);same(future,f,work,restored,"initialized rewind discards and reproduces the complete movement/combat future");
    CaptureSimulationFrame(1);check(!memcmp(pixels,g_ppu->renderBuffer,pixel_size),"native actor animation, CHR and projectile output after rewind/replay are pixel-identical");
    free(pixels);snes_rewind_shutdown();
  }
}
