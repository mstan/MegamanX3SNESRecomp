/* Actual stage5 spike row1632..1759/y1472; the other actor stands on the
 * adjacent authored plateau at1568/y1392. No damage/death result is supplied. */
static unsigned spike_victim,spike_damage,spike_controller,spike_terrain;
static void spike_damage_probe(CpuState *cpu,uint32_t pc){
  (void)pc;
  if(cpu->D!=X3_COOP_BODY || X3CoopRuntimeState()->current!=spike_victim ||
      (g_ram[0x9ff]&127) || g_ram[0x9fe]!=127)return;
  ++spike_damage;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"actual pending lethal-terrain state passes runtime preflight");
  /* Runtime wire: controller entry/result registers are12 bytes each. */
  spike_controller|=view_probe.bytes[X3CoopEncodedSize()+24]!=0;
  spike_terrain|=view_probe.bytes[X3CoopEncodedSize()+26+24]!=0;
}
static void native_spike_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,size_t capacity){
  uint8_t *replay=malloc(capacity);check(replay!=NULL,"fatal-contact replay buffer allocates");
  /* Default terrain contact runs after the controllers. Observe B5A9 after
   * the real collision, before B5AB projects the other body's terrain pass.
   * The ordinary AOT damage helper is not interpreted with learner off. */
  check(interp_bridge_add_pre_opcode_hook(0x04b5a9,spike_damage_probe) &&
        interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"native lethal-terrain and pit observations register");
  for(unsigned roster=0;roster<2;++roster)for(unsigned victim=0;victim<2;++victim){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==5 &&
        X3CoopRuntimeState()->players[0].character==roster && !X3CoopRuntimeLearner() &&
        !X3CoopRuntimeStockZero(),"spike route uses actual stage5 and the requested default roster");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat){
      uint8_t *b=setup.players[seat].body;unsigned x=seat==victim?1680:1568;
      unsigned y=seat==victim?1400:setup.players[seat].character==X3_COOP_ZERO?1367:1375;
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);
      b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);
      b[4]=b[7]=b[2]=b[3]=0;memset(b+0x1a,0,6);
    }
    shared_prepare(base,n,work,&setup);view_word(0x1e5d,1456);view_word(0x1e7a,1456);
    /* Match the actual upper-room camera band to this QA teleport. Keeping
     * the lower entry room's bounds makes native B59B push both bodies down
     *16 pixels per frame, burying the otherwise safe partner in its floor. */
    const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
    for(unsigned i=0;i<sizeof(vertical)/sizeof(*vertical);++i)view_word(vertical[i],1280);
    spike_victim=shared_pit_victim=victim;spike_damage=spike_controller=spike_terrain=shared_pit_damage=0;
    unsigned elapsed=0;size_t contact_size=0;
    for(;elapsed<180 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE;++elapsed){
      contact_size=RtlSaveSnapshotToMemory(base,capacity);frame(0);
    }
    const X3CoopState *s=X3CoopRuntimeState();
    fprintf(stderr,"authored spike roster%u seat%u frames%u fatalSelections%u controller%u terrain%u pit%u life%u/%u xy%u/%u\n",
        roster,victim,elapsed,spike_damage,spike_controller,spike_terrain,shared_pit_damage,
        s->players[0].life,s->players[1].life,coop_x(victim),support_position(victim,8));
    check(elapsed<180 && spike_damage==1 && !shared_pit_damage && s->players[victim].life==X3_COOP_FALLEN &&
        s->players[victim^1].life==X3_COOP_ALIVE && s->anchor==(victim^1) && !s->wipe,
        "actual spike contact kills only the actor above the authored row and retains the free partner");
    size_t fatal_size=RtlSaveSnapshotToMemory(work,capacity);
    CaptureSimulationFrame(1);size_t pixel_size=(size_t)g_ppu->renderPitch*224;
    uint8_t *pixels=malloc(pixel_size);check(pixels!=NULL,"actual fatal-output buffer allocates");
    memcpy(pixels,g_ppu->renderBuffer,pixel_size);
    check(contact_size && RtlLoadSnapshotFromMemory(base,contact_size),"last actual frame before spike contact restores");
    frame(0);size_t replay_size=RtlSaveSnapshotToMemory(replay,capacity);
    same(work,fatal_size,replay,replay_size,"fatal spike contact replays the complete machine and survivor handoff");
    CaptureSimulationFrame(1);
    check(!memcmp(pixels,g_ppu->renderBuffer,pixel_size),"actual displayed fatal-contact pixels replay identically");
    free(pixels);
    check(RtlLoadSnapshotFromMemory(work,fatal_size),"actual fatal-contact outcome restores");
    replay_size=RtlSaveSnapshotToMemory(replay,capacity);
    same(work,fatal_size,replay,replay_size,"actual fallen and survivor contexts restore immediately");
    unsigned before=coop_x(victim^1);
    for(unsigned f=0;f<24;++f)frame((uint32_t)SNES_PAD_LEFT<<((victim^1)*12));
    fprintf(stderr,"spike survivor roster%u victim%u x%u->%u y%u display%02x life%u world%u/%u\n",
        roster,victim,before,coop_x(victim^1),support_position(victim^1,8),g_ppu->inidisp,
        X3CoopRuntimeState()->players[victim^1].life,g_ram[0xd1],g_ram[0xd2]);
    check(coop_x(victim^1)<before && (g_ppu->inidisp&0x8f)==15,
        "spike survivor keeps actual movement and bright gameplay");
    for(unsigned a=0x1f25;a<=0x1f2b;++a)check(!g_ram[a],"fatal terrain leaves shared world pools unfrozen");
  }
  free(replay);
}
