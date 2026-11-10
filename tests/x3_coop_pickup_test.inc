/* Real stage1 health item; teleports/HP seed the approach, never the claim,
 * refill count, freeze, completion or retirement results. */
static unsigned pickup_probe_slot;
static bool pickup_probe_seen;
static void pickup_continuation_probe(CpuState *cpu,uint32_t pc){
  (void)pc;if(!pickup_probe_slot || cpu->D!=pickup_probe_slot)return;
  pickup_probe_slot=0;pickup_probe_seen=true;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),
        "interrupted native retired-recipient borrow passes snapshot preflight");
  view_probe.used=0;view_probe.load=true;
  check(X3CoopRuntimeLoad(&view_probe.io),"native retired-recipient continuation restores in place");
}
static void pickup_native_item_loop(void){
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x00dac2) && g_cpu.S==resume.S,"native frozen-item loop returns with its real stack");
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
}
static unsigned health_pickup_approach(uint8_t *original,size_t size,uint8_t *work,unsigned health){
  check(RtlLoadSnapshotFromMemory(original,size),"health pickup setup restores");
  X3CoopState setup=*X3CoopRuntimeState();
  unsigned seat=setup.players[0].character==X3_COOP_ZERO?0:1;
  setup.players[seat].body[5]=0x38;setup.players[seat].body[6]=7; /*1848*/
  setup.players[seat].body[8]=0x8c;setup.players[seat].body[9]=5; /*1420*/
  setup.players[seat].body[0x27]=(uint8_t)health;
  memset(setup.players[seat].body+0x1a,0,6);
  memcpy(work,original,size);size_t offset=contact_context_offset(original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),
        "private damaged-body approach restores");
  X3CoopViewsConfigure(true);
  /* The QA teleport bypasses the route and camera-admission history. Seed
   * the actual item's lower-room band; native admission/contact/refill remain
   * untouched. Camera traversal is a separate acceptance requirement. */
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){
    views.camera[s][0]=(uint16_t)(s==seat?1720:256);
    views.camera[s][1]=(uint16_t)(s==seat?1280:1024);
    views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];
  }
  X3CoopViewsSetState(&views);
  return seat;
}
static void authored_health_pickup_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  unsigned seat=health_pickup_approach(original,size,work,20),other=seat^1;
  size_t offset=contact_context_offset(original,size);X3CoopState setup;
  unsigned slot=0;bool frozen=false;size_t middle=0;
  for(unsigned i=0;i<60;++i){
    frame(0);
    for(unsigned d=0x1518;d<0x1818;d+=48)
      if(g_ram[d] && g_ram[d+10]==2 && (g_ram[d+5]|g_ram[d+6]<<8)==1848)slot=d;
    if(slot && g_ram[0x1f28]){frozen=true;middle=RtlSaveSnapshotToMemory(work,capacity);break;}
  }
  if(!frozen)fprintf(stderr,"pickup: slot=%04x phase=%u/%u recipient%u xy=%u,%u HP=%u itemstate=%u/%u camera=%u,%u\n",
      slot,g_ram[0xd1],g_ram[0xd2],seat,coop_x(seat),support_position(seat,8),contact_hp(seat),
      slot?g_ram[slot+2]:0,slot?g_ram[slot+3]:0,X3CoopViewsState()->camera[seat][0],X3CoopViewsState()->camera[seat][1]);
  check(slot && frozen && middle,"authored class02 health pickup starts native refill for the remote actor");
  check((g_ram[slot]&128) && g_ram[slot+2]==6 && g_ram[slot+3]==2,
        "original item owns its native frozen refill state");
  uint8_t *pending=malloc(middle);check(pending!=NULL,"pending refill snapshot buffer allocates");memcpy(pending,work,middle);
  size_t marker=0;for(size_t i=0;i+8<middle;++i)if(!memcmp(pending+i,"X3R1",4)){marker=i;break;}
  check(marker && pending[marker+6]==seat && pending[marker+7]==1 &&
        (pending[marker+4]|pending[marker+5]<<8)==slot,"pending native refill retains its recipient and real item slot");
  const char *output=getenv("X3_COOP_HEALTH_PICKUP_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"native pending refill visual fixture saves");
  unsigned positions[2][2];for(unsigned s=0;s<2;++s){positions[s][0]=support_position(s,5);positions[s][1]=support_position(s,8);}
  bool parked=true;
  for(unsigned i=0;i<8;++i){
    frame((SNES_PAD_RIGHT|SNES_PAD_B)|((SNES_PAD_RIGHT|SNES_PAD_B)<<12));
    parked=parked && g_ram[0x1f28];
    for(unsigned s=0;s<2;++s)parked=parked && support_position(s,5)==positions[s][0] && support_position(s,8)==positions[s][1];
  }
  check(parked,"native refill parks both actors despite movement/jump input while collision is frozen");
  check(RtlLoadSnapshotFromMemory(pending,middle),"native refill movement probe restores");
  for(unsigned i=0;i<48;++i)frame(0);
  bool released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
  check(released && !g_ram[0x1f4f] && g_ram[0xd1]==4 && g_ram[0xd2]==4 && (g_ppu->inidisp&0x8f)==15,
        "remote native refill releases every world freeze without fade or stage exit");
  check(contact_hp(seat)==28 && contact_hp(other)==16,
        "one authored pickup heals only its actual recipient to native Zero28HP");
  size_t completed=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(pending,middle),"pending native refill restores");
  for(unsigned i=0;i<48;++i)frame(0);
  size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,completed,work,repeated,"pending native refill replays to the identical complete machine");
  pending[marker+6]=2;
  check(!RtlLoadSnapshotFromMemory(pending,middle),"invalid native refill recipient is refused before load");
  repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,completed,work,repeated,"refused refill recipient leaves the machine unchanged");pending[marker+6]=(uint8_t)seat;
  check(RtlLoadSnapshotFromMemory(pending,middle),"pending modal-owner probe restores");
  g_ram[0x1f22]=1;
  for(unsigned i=0;i<8;++i)pickup_native_item_loop();
  check(contact_hp(seat)==22 && contact_hp(other)==16,
        "pending item retains its real recipient while a shared scene owns control");

  /* Isolated lifecycle cancellation probes; these do not claim a completed
   * launcher withdrawal or an authored enemy-death route during a freeze. */
  for(unsigned fallen=0;fallen<2;++fallen){
    check(RtlLoadSnapshotFromMemory(pending,middle),"pending retired-recipient probe restores");
    setup=*X3CoopRuntimeState();unsigned retained=setup.players[seat].body[0x27]&127;
    check(fallen?X3CoopFall(&setup,seat):X3CoopWithdraw(&setup,seat),"pending recipient retires through the lifecycle API");
    memcpy(work,pending,middle);
    check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,middle),
          "retired pending-refill context restores");
    check(interp_bridge_add_pre_opcode_hook(0x00dafa,pickup_continuation_probe),"retired-refill continuation probe registers");
    pickup_probe_slot=slot;pickup_probe_seen=false;
    for(unsigned i=0;i<12;++i)frame(0);
    check(pickup_probe_seen,"native cleanup actually borrows the retired recipient before returning to the survivor");
    released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
    check(released && !g_ram[0x1f4f],"retired-recipient native cleanup releases the world freeze");
    check(X3CoopRuntimeState()->players[seat].life==(fallen?X3_COOP_FALLEN:X3_COOP_ABSENT) &&
          (X3CoopRuntimeState()->players[seat].body[0x27]&127)==retained && contact_hp(other)==16,
          "cleanup neither revives nor heals the retired actor or unrelated survivor");
  }
  health_pickup_approach(original,size,work,28);
  g_ram[0x1fb7]=0x80;g_ram[0x1fb8]=g_ram[0x1fb9]=g_ram[0x1fba]=0;
  for(unsigned i=0;i<48;++i)frame(0);
  check(g_ram[0x1fb7]==0x82 && !g_ram[0x1fb8] && !g_ram[0x1fb9] && !g_ram[0x1fba],
        "full-health Zero banks one pickup into the shared native subtank with the original amount");
  check(contact_hp(seat)==28 && contact_hp(other)==16,"shared banking does not change either personal health value");
  free(pending);check(RtlLoadSnapshotFromMemory(original,size),"authored pickup probes restore their starting machine");
}
