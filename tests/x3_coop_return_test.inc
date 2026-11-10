/* Real native stage5 ground. Roster/health/retirement API preparation is not
 * authored hazard coverage. Public controls use the actual simulation pads. */
static void native_participation_controls(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *pending=malloc(capacity);check(pending!=NULL,"participation replay buffer allocates");
  for(unsigned reversed=0;reversed<2;++reversed)for(unsigned seat=0;seat<2;++seat){
    check(RtlLoadSnapshotFromMemory(original,size),"public participation controls start from native terrain");
    X3CoopState setup=*X3CoopRuntimeState();
    if(reversed){X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;}
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"public participation roster precondition restores");
    door_install_player(&setup);X3CoopViewsConfigure(true);
    unsigned held=SNES_PAD_SELECT<<(seat*12);
    for(unsigned i=0;i<45;++i)frame(held);
    size_t middle=RtlSaveSnapshotToMemory(pending,capacity),chunk=door_scene_chunk(pending,middle)+16;
    check(!memcmp(pending+chunk,"X3J1",4) && pending[chunk+4+seat]>0 && pending[chunk+4+seat]<90 &&
          X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE,
          "Select hold is simulation state and does not withdraw before its threshold");
    for(unsigned field=4;field<8;++field){
      memcpy(work,pending,middle);work[chunk+field]=(uint8_t)(field<6?92:1);
      check(!RtlLoadSnapshotFromMemory(work,middle),"malformed participation hold/latch rejects transactionally");
      size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);
      same(pending,middle,work,unchanged,"refused participation state leaves complete machine unchanged");
    }
    unsigned frames=0;
    while(X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE && frames<120){frame(held);++frames;}
    check(frames<120 && X3CoopRuntimeState()->players[seat].life==X3_COOP_ABSENT,
          "either seat can withdraw through its real Select control");
    size_t departed=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(pending,middle),"partial Select hold restores for replay");
    for(unsigned i=0;i<frames;++i)frame(held);
    size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,departed,work,repeated,"partial Select hold replays withdrawal and ownership identically");
    for(unsigned i=0;i<120;++i)frame(held);
    check(X3CoopRuntimeState()->players[seat].life==X3_COOP_ABSENT,
          "continuing to hold Select cannot immediately toggle the withdrawn actor back");
    frame(0);
    for(unsigned i=0;i<12;++i)frame(SNES_PAD_B<<((seat^1)*12));
    size_t airborne=RtlSaveSnapshotToMemory(expected,capacity);
    check(!X3CoopRuntimeRejoin(seat),"return waits when the survivor is airborne without a nearby terrain floor");
    repeated=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,airborne,work,repeated,"unsafe airborne return leaves both actors, views and machine unchanged");
    for(unsigned i=0;i<90;++i)frame(0);
    frames=0;
    while(X3CoopRuntimeState()->players[seat].life==X3_COOP_ABSENT && frames<160){frame(held);++frames;}
    check(frames<160 && X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE,
          "released and held Select safely returns either native character");
    const X3CoopState *s=X3CoopRuntimeState();
    check(contact_hp(seat)==setup.players[seat].body[0x27] &&
          s->players[seat].character==setup.players[seat].character &&
          s->players[seat].body[0xb6]==setup.players[seat].body[0xb6],
          "public return preserves native identity and personal health");
    frame(0);unsigned x=coop_x(seat);
    for(unsigned i=0;i<20;++i)frame(SNES_PAD_RIGHT<<(seat*12));
    check(coop_x(seat)>x && (g_ppu->inidisp&0x8f)==15,"publicly returned actor resumes real native movement");
  }
  check(RtlLoadSnapshotFromMemory(original,size),"simultaneous participation requests start from native terrain");
  for(unsigned i=0;i<150;++i)frame(SNES_PAD_SELECT|(SNES_PAD_SELECT<<12));
  const X3CoopState *s=X3CoopRuntimeState();
  check(!s->wipe && s->players[0].life==X3_COOP_ABSENT && s->players[1].life==X3_COOP_ALIVE,
        "simultaneous Select requests leave one living participant");
  free(pending);check(RtlLoadSnapshotFromMemory(original,size),"public participation probes restore their starting machine");
}
static void native_return_armor(uint8_t *original,size_t size,uint8_t *work,size_t capacity){
  for(unsigned reversed=0;reversed<2;++reversed){
    check(RtlLoadSnapshotFromMemory(original,size),"native armored return starts from original stage5");
    X3CoopState setup=*X3CoopRuntimeState();
    if(reversed){X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;}
    unsigned x=setup.players[0].character==X3_COOP_X?0:1;setup.current=setup.anchor=(uint8_t)x;
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"armored return roster precondition restores");
    door_install_player(&setup);g_ram[0x1fd1]=15; /* Shared acquired-parts precondition, not acquisition coverage. */
    cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
    view_native_call(0x04b5bb,X3_COOP_BODY,false);
    g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
    g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
    uint8_t native[96];memcpy(native,g_ram+X3_COOP_AUX,sizeof(native));
    check(g_ram[X3_COOP_BODY+0x16]==0x18,"original native constructor enables X's acquired body group");
    check(X3CoopRuntimeWithdraw(x) && X3CoopRuntimeRejoin(x),"acquired X armor survives runtime withdrawal and safe return");
    const X3CoopPlayer *p=&X3CoopRuntimeState()->players[x];
    same(native,sizeof(native),p->auxiliaries,sizeof(native),"returned armor children match original native construction exactly");
    check(p->body[0x16]==0x18,"returned X retains the acquired native body group");
    for(unsigned i=0;i<30;++i)frame(0);
    p=&X3CoopRuntimeState()->players[x];
    for(unsigned part=0;part<3;++part)check(p->auxiliaries[part*32] && p->auxiliaries[part*32+0x10]==0x5d,
                                         "native armor children continue updating after safe return");
  }
  check(RtlLoadSnapshotFromMemory(original,size),"armor return probes restore their starting machine");(void)capacity;
}
static void native_armored_door_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"armored door starts from original stage1 fixture");
  X3CoopState setup=*X3CoopRuntimeState();unsigned x=setup.players[0].character==X3_COOP_X?0:1;
  setup.current=setup.anchor=(uint8_t)x;memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,size),"armored door native owner precondition restores");
  door_install_player(&setup);g_ram[0x1fd1]=15;
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;view_native_call(0x04b5bb,X3_COOP_BODY,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  uint8_t *prepared=malloc(capacity);check(prepared!=NULL,"armored native door buffer allocates");
  size_t n=RtlSaveSnapshotToMemory(prepared,capacity);
  native_door_checks(prepared,n,work,expected,capacity,X3_COOP_ZERO);
  free(prepared);check(RtlLoadSnapshotFromMemory(original,size),"armored door restores original resources");
}
static void return_corridor_check(uint8_t *original,size_t size){
  check(RtlLoadSnapshotFromMemory(original,size),"return corridor starts from native stage5 ground");
  /* Isolated geometry contract: the original metatile writer constructs a
   * thin barrier between two clear boxes. This is not an authored-wall pass. */
  view_word(X3_COOP_BODY+5,400);view_word(X3_COOP_BODY+0x22,400);
  for(unsigned y=2688;y<2736;y+=16){
    view_word(0,416);view_word(2,y);view_word(8,72); /* Actual native solid metatile3B. */
    cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;view_native_call(0x04bec0,0,true);
    g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
    g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  }
  X3CoopViewsConfigure(true);X3CoopViewState views=*X3CoopViewsState();
  views.valid=views.camera_valid=3;views.initialized=1;views.stage=5;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=384;views.camera[s][1]=2560;
    views.limits[s][0]=views.current[s][0]=0;views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=2560;views.limits[s][3]=views.current[s][3]=2560;}
  X3CoopViewsSetState(&views);view_word(0x1e5d,384);
  check(X3CoopRuntimeWithdraw(1) && X3CoopRuntimeRejoin(1),"return finds a clear box on the survivor's side of the barrier");
  check(coop_x(1)==400,"safe return cannot teleport across a blocked corridor to a clear distant box");
  for(unsigned i=0;i<30;++i)frame(0);
  check(X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && contact_hp(1)==28,
        "same-side return remains on native solid ground without damage");
  check(RtlLoadSnapshotFromMemory(original,size),"isolated corridor geometry restores its original machine");
}
static void native_return_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *returning=malloc(capacity);check(returning!=NULL,"return replay buffer allocates");
  for(unsigned reversed=0;reversed<2;++reversed)for(unsigned withdrawn=0;withdrawn<2;++withdrawn){
    check(RtlLoadSnapshotFromMemory(original,size),"safe return starts on native stage5 terrain");
    X3CoopState setup=*X3CoopRuntimeState();
    if(reversed){X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;}
    setup.current=setup.anchor=(uint8_t)withdrawn;
    unsigned hp[2];
    for(unsigned seat=0;seat<2;++seat){hp[seat]=setup.players[seat].character==X3_COOP_ZERO?19:11;
      setup.players[seat].body[0x27]=(uint8_t)hp[seat];}
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"safe return roster/resources precondition restores");
    door_install_player(&setup);X3CoopViewsConfigure(true);
    for(unsigned i=0;i<3;++i)frame(0);
    check(X3CoopRuntimeWithdraw(withdrawn),"runtime withdrawal transfers actual native world ownership");
    const X3CoopState *s=X3CoopRuntimeState();unsigned survivor=withdrawn^1;
    check(s->players[withdrawn].life==X3_COOP_ABSENT && s->anchor==survivor && s->current==survivor &&
          g_ram[X3_COOP_BODY+0xb6]==(s->players[survivor].character==X3_COOP_ZERO?2:0),
          "withdrawal leaves the survivor's real native character projected");
    size_t absent=RtlSaveSnapshotToMemory(expected,capacity);
    check(!X3CoopRuntimeWithdraw(survivor),"runtime refuses removing the last living participant");
    size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,absent,work,unchanged,"refused last withdrawal leaves complete machine unchanged");
    X3CoopViewState views=*X3CoopViewsState();
    views.valid|=(uint8_t)(1u<<withdrawn);views.camera_valid|=(uint8_t)(1u<<withdrawn);
    views.camera[withdrawn][0]=views.camera[withdrawn][1]=0;
    views.limits[withdrawn][0]=views.current[withdrawn][0]=0;
    views.limits[withdrawn][1]=views.current[withdrawn][1]=256;
    views.limits[withdrawn][2]=views.current[withdrawn][2]=0;
    views.limits[withdrawn][3]=views.current[withdrawn][3]=224;
    X3CoopViewsSetState(&views);
    size_t waiting=RtlSaveSnapshotToMemory(returning,capacity);
    check(X3CoopRuntimeRejoin(withdrawn),"runtime return finds actual clear native ground beside the survivor");
    s=X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat)
      check(s->players[seat].life==X3_COOP_ALIVE && contact_hp(seat)==hp[seat],
            "runtime return retains each actor's own personal health");
    unsigned start=coop_x(withdrawn);
    for(unsigned i=0;i<30;++i)frame(SNES_PAD_RIGHT<<(withdrawn*12));
    s=X3CoopRuntimeState();
    unsigned y=s->players[withdrawn].body[8]|s->players[withdrawn].body[9]<<8;
    fprintf(stderr,"return roster%u seat%u native%u HP%u/max%u x%u->%u y%u cameras%u,%u\n",
        reversed,withdrawn,s->players[withdrawn].body[0xb6],contact_hp(withdrawn),s->players[withdrawn].maximum_health,
        start,coop_x(withdrawn),y,X3CoopViewsState()->camera[withdrawn][0],X3CoopViewsState()->camera[withdrawn][1]);
    check(coop_x(withdrawn)>start && y>2600 && s->players[withdrawn].life==X3_COOP_ALIVE &&
          contact_hp(withdrawn)==hp[withdrawn] && (g_ppu->inidisp&0x8f)==15,
          "returned actor moves on native ground without old-room clamping, damage or survivor fade");
    size_t completed=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(returning,waiting) && X3CoopRuntimeRejoin(withdrawn),"absent actor and stale-view history restore for return replay");
    for(unsigned i=0;i<30;++i)frame(SNES_PAD_RIGHT<<(withdrawn*12));
    unchanged=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,completed,work,unchanged,"safe return, native movement and private view reset replay identically");
    check(RtlLoadSnapshotFromMemory(expected,completed),"safe return completed snapshot restores");
    unchanged=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,completed,work,unchanged,"safe return completed immediate restore");
    /* A fallen participant must remain excluded even at a valid landing. */
    X3CoopState fallen=*s;check(X3CoopFall(&fallen,withdrawn),"safe return fallen setup uses lifecycle API");
    check(X3CoopEncode(&fallen,expected+contact_context_offset(expected,completed),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,completed),"fallen participation precondition restores");
    door_install_player(&fallen);
    check(!X3CoopRuntimeRejoin(withdrawn),"safe landing cannot revive a fallen actor");
    for(unsigned i=0;i<120;++i)frame(SNES_PAD_SELECT<<(withdrawn*12));
    check(X3CoopRuntimeState()->players[withdrawn].life==X3_COOP_FALLEN,
          "public Select control cannot revive an API-prepared fallen actor");
  }
  free(returning);check(RtlLoadSnapshotFromMemory(original,size),"safe return probes restore their starting machine");
  native_return_armor(original,size,work,capacity);
  return_corridor_check(original,size);
  native_participation_controls(original,size,work,expected,capacity);
}
