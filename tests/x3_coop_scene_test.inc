/* Authored stage1 midboss entrance. Teleport/camera setup bypasses the route,
 * not native allocation, admission, locks, animation or forced movement. */
static unsigned door_approach(uint8_t *original,size_t size,uint8_t *work,size_t capacity,unsigned character){
  check(RtlLoadSnapshotFromMemory(original,size),"native gate approach restores");
  X3CoopState setup=*X3CoopRuntimeState();unsigned seat=setup.players[0].character==character?0:1;
  setup.anchor=setup.current=(uint8_t)(seat^1);
  setup.players[seat].body[5]=0x60;setup.players[seat].body[6]=9; /*2400*/
  setup.players[seat].body[8]=0x9b;setup.players[seat].body[9]=5; /*1435*/
  memset(setup.players[seat].body+0x1a,0,6);
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,size),"private remote gate approach restores");
  /* The fixture explicitly reverses native world ownership too. Copy the
   * whole owning context, not just character/body bytes over the old X. */
  const X3CoopPlayer *world=&setup.players[setup.anchor];
  memcpy(g_ram+X3_COOP_BODY,world->body,sizeof(world->body));
  memcpy(g_ram+X3_COOP_AUX,world->auxiliaries,sizeof(world->auxiliaries));
  memcpy(g_ram+X3_COOP_SHOTS,world->shots,sizeof(world->shots));
  memcpy(g_ram+X3_COOP_PALETTE,world->palette,sizeof(world->palette));
  g_ram[0x1f1e]=world->shot_command;g_ram[0x1fd2]=world->maximum_health;g_ram[0xa1]=1;
  X3CoopViewsConfigure(true);X3CoopRuntimeFrame();
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=(uint16_t)(s==seat?2416:256);
    views.camera[s][1]=(uint16_t)(s==seat?1280:1024);
    views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];}
  X3CoopViewsSetState(&views);
  CpuState resume=g_cpu;view_native_call(0x00de3a,0,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  pickup_native_items(0x00daa0);pickup_native_items(0x00daa0);
  unsigned slot=0;
  for(unsigned d=0x1518;d<0x1818;d+=48)
    if(g_ram[d] && g_ram[d+10]==0x11 && view_read(d+5)==2552 && view_read(d+8)==1431)slot=d;
  check(slot && g_ram[slot+1]==2 && !g_ram[0x1f28],
        "retail allocator and constructor prepare the authored gate without triggering it");
  setup=*X3CoopRuntimeState();setup.players[seat].body[5]=0xf0;setup.players[seat].body[6]=9; /*2544*/
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+contact_context_offset(work,prepared),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,prepared),"touching remote actor restores without admission flags");
  return slot;
}
static void native_door_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity,unsigned character){
  unsigned slot=door_approach(original,size,work,capacity,character);
  const X3CoopState setup=*X3CoopRuntimeState();unsigned remote=setup.anchor^1;
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  /* Native single-body control: use the same touching character and local
   * camera. The distant other body is absent from this reference update. */
  X3CoopRuntimeEnable(false);memcpy(g_ram+X3_COOP_BODY,setup.players[remote].body,X3_COOP_BODY_BYTES);
  view_word(0x1e5d,2416);view_word(0x1e60,1280);
  pickup_native_items(0x00daa0);
  check(g_ram[slot+1]==4 && g_ram[0x1f28],"original single-body gate admits the touching remote character");
  uint8_t native_item[48];memcpy(native_item,g_ram+slot,sizeof(native_item));
  check(RtlLoadSnapshotFromMemory(work,prepared),"paired gate starts from the matched native approach");
  pickup_native_items(0x00daa0);
  if(g_ram[slot+1]!=4)fprintf(stderr,"gate remote%u char%u: state=%u freeze=%u anchor=%u current=%u\n",
      remote,setup.players[remote].character,g_ram[slot+1],g_ram[0x1f28],
      X3CoopRuntimeState()->anchor,X3CoopRuntimeState()->current);
  check(g_ram[slot+1]==4 && g_ram[0x1f28],"remote actor admits the single authored native gate");
  check(!memcmp(native_item,g_ram+slot,sizeof(native_item)),"paired admission matches one original gate update");
  check(X3CoopRuntimeState()->anchor==remote,"native gate adopts the actual entering actor");
  check(view_read(0x1e5d)==2416 && view_read(0x1e60)==1280,
        "gate camera adopts the entering region before changing world ownership");
  size_t admitted=RtlSaveSnapshotToMemory(expected,capacity);
  const char *output=getenv("X3_COOP_DOOR_OUTPUT");char path[1024];
  for(unsigned i=0;i<300;++i){frame(0);
    if(i==60 && output && *output){
      snprintf(path,sizeof(path),"%s-p%u-%s-mid.sav",output,remote+1,character==X3_COOP_ZERO?"zero":"x");
      check(RtlSaveSnapshot(path),"pending native gate visual fixture saves");
    }
  }
  check(!g_ram[0x1f28] && view_read(0x1e5d)==2560 && support_position(remote,5)>2552,
        "native gate finishes its original forced walk and cleanup");
  if(output && *output){
    snprintf(path,sizeof(path),"%s-p%u-%s-complete.sav",output,remote+1,character==X3_COOP_ZERO?"zero":"x");
    check(RtlSaveSnapshot(path),"completed native gate visual fixture saves");
  }
  uint8_t *finished=malloc(capacity);check(finished!=NULL,"gate replay buffer allocates");
  size_t finished_size=RtlSaveSnapshotToMemory(finished,capacity);
  check(RtlLoadSnapshotFromMemory(expected,admitted),"admitted native gate snapshot restores");
  for(unsigned i=0;i<300;++i)frame(0);
  size_t replay=RtlSaveSnapshotToMemory(expected,capacity);
  same(finished,finished_size,expected,replay,"native gate forced-walk and cleanup deterministic replay");
  free(finished);
  for(unsigned mode=0;mode<5;++mode){
    check(RtlLoadSnapshotFromMemory(work,prepared),"native gate eligibility approach restores");
    X3CoopState candidates=*X3CoopRuntimeState();unsigned first=candidates.anchor;
    if(mode!=2 && mode!=3){
      candidates.players[first].body[5]=0xf8;candidates.players[first].body[6]=9;
      candidates.players[first].body[8]=0x97;candidates.players[first].body[9]=5;
      if(mode<2)candidates.players[first].body[2]=0x12; /* Retail admission exclusion. */
    }
    if(mode==1)candidates.players[remote].body[2]=0x12;
    if(mode==2)candidates.players[remote].body[0x27]=0;
    if(mode==3)check(X3CoopFall(&candidates,remote),"fallen gate candidate precondition applies");
    memcpy(expected,work,prepared);
    check(X3CoopEncode(&candidates,expected+contact_context_offset(expected,prepared),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,prepared),"gate candidates restore without admission results");
    memcpy(g_ram+X3_COOP_BODY,candidates.players[first].body,X3_COOP_BODY_BYTES);
    if(!mode){
      check(interp_bridge_add_pre_opcode_hook(0x01e0ae,pickup_continuation_probe),"gate predicate continuation probe registers");
      pickup_probe_slot=slot;pickup_probe_pc=0x01e0ae;pickup_probe_seat=remote;pickup_probe_seen=false;
    }
    pickup_native_items(0x00daa0);
    if(!mode)check(pickup_probe_seen,"blocked nearest candidate retries the whole gate predicate with a restorable continuation");
    bool hit=mode==0 || mode==4;
    check(hit?(g_ram[slot+1]==4 && g_ram[0x1f28]):(!g_ram[0x1f28] && (!g_ram[slot] || g_ram[slot+1]==2)),
          "native gate handles excluded, zero-HP, fallen and simultaneously touching candidates");
    check(X3CoopRuntimeState()->anchor==(mode==0?remote:first) &&
          X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,
          "gate miss restores canonical ownership; one actual entrant owns a successful gate");
  }
  check(RtlLoadSnapshotFromMemory(original,size),"native gate admission probes restore the starting machine");
}
