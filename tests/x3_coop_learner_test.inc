static size_t learner_profile(uint8_t *snapshot,size_t size){
  size_t at=door_scene_chunk(snapshot,size)+16;
  check(!memcmp(snapshot+at,"X3J2",4),"learner profile uses the current participation wire");return at;
}
static void learner_contact(unsigned slot){
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
  contact_native_call(slot,0x04cb31);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
}
static unsigned learner_interrupted;
static void learner_pit_return(CpuState *cpu,uint32_t pc){
  (void)pc;
  if(!learner_interrupted || cpu->D!=0x1e58 || X3CoopRuntimeState()->current!=1)return;
  --learner_interrupted;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);
  size_t at=learner_profile(view_probe.bytes,view_probe.used);
  check((view_probe.bytes[at+7]&2) && X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),
        "native pit return encodes a valid interrupted learner rescue");
  size_t n=view_probe.used;view_probe.bytes[at+6]&=(uint8_t)~2;
  check(!X3CoopRuntimePreflight(view_probe.bytes,n),"pending rescue without its learner profile is rejected");
  view_probe.bytes[at+6]|=2;view_probe.used=0;view_probe.load=true;
  check(X3CoopRuntimeLoad(&view_probe.io) && view_probe.used==n,
        "pending learner rescue restores inside the original bounds pass");
}
static void native_learner_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *base=malloc(capacity),*pending=malloc(capacity);
  check(base && pending,"learner protection and replay buffers allocate");
  for(unsigned roster=0;roster<2;++roster){
    if(roster){const char *path=getenv("X3_COOP_LEARNER_REVERSED_STATE");
      check(path && *path && RtlLoadSnapshot(path),"learner uses actual reversed native roster");}
    else check(RtlLoadSnapshotFromMemory(original,size),"learner starts from actual native gameplay");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);
    for(unsigned victim=0;victim<2;++victim){
      check(RtlLoadSnapshotFromMemory(base,n),"learner damage position prerequisite restores");
      X3CoopState setup=*X3CoopRuntimeState();unsigned away_x=coop_x(victim)+160;
      setup.players[victim^1].body[5]=(uint8_t)away_x;setup.players[victim^1].body[6]=(uint8_t)(away_x>>8);
      shared_prepare(base,n,work,&setup);
      contact_geometry(0xd18,coop_x(victim),support_position(victim,8));
      size_t test=RtlSaveSnapshotToMemory(pending,capacity);
      learner_contact(0xd18);unsigned native_hp=contact_hp(victim);
      check(native_hp+2==(setup.players[victim].body[0x27]&127),"unassisted original contact applies two native damage points");
      pending[learner_profile(pending,test)+6]|=2;
      check(RtlLoadSnapshotFromMemory(pending,test) && X3CoopRuntimeLearner(),"learner profile restores before actual native contact");
      learner_contact(0xd18);
      check(contact_hp(victim)==(victim==1?(setup.players[1].body[0x27]&127):native_hp) &&
            contact_hp(victim^1)==(setup.players[victim^1].body[0x27]&127),
            "native ordinary-hit protection belongs only to P2, in either roster");
      size_t observed=RtlSaveSnapshotToMemory(expected,capacity);
      check(RtlLoadSnapshotFromMemory(pending,test),"learner hit precondition restores for replay");
      learner_contact(0xd18);
      same(expected,observed,work,RtlSaveSnapshotToMemory(work,capacity),"learner native contact replays the complete machine");
    }
  }
  for(unsigned roster=0;roster<2;++roster){
    const char *ground=getenv(roster?"X3_COOP_LEARNER_REVERSED_GROUND_STATE":"X3_COOP_LEARNER_GROUND_STATE");
    if(!ground || !*ground)continue;
    shared_pit_prepare(ground,1,base,work,capacity);
    size_t n=RtlSaveSnapshotToMemory(base,capacity);base[learner_profile(base,n)+6]|=2;
    check(RtlLoadSnapshotFromMemory(base,n),"learner starts before the same real authored pit");
    check(interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"learner observes original native pit damage request");
    check(interp_bridge_add_pre_opcode_hook(0x00e30e,learner_pit_return),"learner observes original native pit damage return");
    shared_pit_victim=1;shared_pit_damage=0;unsigned hp=contact_hp(1),walked=0;
    while(walked<64 && coop_x(1)<1216){frame(SNES_PAD_RIGHT<<12);++walked;}
    check(coop_x(1)>=1200 && coop_x(1)<1248 && coop_x(0)==1104 && view_read(0x1e5d)<=1000,
          "Unified learner enters the real opening while the camera keeps following P1");
    size_t interrupted=RtlSaveSnapshotToMemory(pending,capacity);learner_interrupted=1;
    unsigned falling=0;while(!shared_pit_damage && falling<120){frame(0);++falling;}
    check(shared_pit_damage==1 && !learner_interrupted &&
          X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && contact_hp(1)==hp,
          "native fatal pit request schedules a living learner rescue without committing death");
    for(unsigned f=0;f<8;++f)frame(0);
    check(shared_distance()<=64 && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && contact_hp(1)==hp,
          "learner returns to clear real ground with the same health and without a team retry");
    size_t result=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(pending,interrupted),"actual learner pit approach restores");
    for(unsigned f=0;f<falling+8;++f)frame(0);
    same(expected,result,work,RtlSaveSnapshotToMemory(work,capacity),"actual native pit rescue replays identically");
    fprintf(stderr,"learner roster%u real pit walk%u fall%u hp%u returns beside P1\n",roster,walked,falling,hp);

    check(RtlLoadSnapshotFromMemory(pending,interrupted),"same pit approach restores for unsafe rescue test");
    unsigned ground_y=support_position(0,8);shared_pit_damage=0;
    for(unsigned f=0;f<120 && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE;++f)frame(SNES_PAD_B);
    check(shared_pit_damage==1 && X3CoopRuntimeState()->players[1].life==X3_COOP_FALLEN &&
          X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && support_position(0,8)+32<ground_y,
          "an airborne P1 cannot turn a fatal P2 pit into an unsafe rescue");

    shared_pit_prepare(ground,0,base,work,capacity);
    n=RtlSaveSnapshotToMemory(base,capacity);base[learner_profile(base,n)+6]|=2;
    check(RtlLoadSnapshotFromMemory(base,n),"learner survivor begins before a real P1 pit");
    shared_pit_victim=0;shared_pit_damage=0;
    for(unsigned f=0;f<64 && coop_x(0)<1216;++f)frame(SNES_PAD_RIGHT);
    for(unsigned f=0;f<120 && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE;++f)frame(0);
    check(shared_pit_damage==1 && X3CoopRuntimeState()->players[0].life==X3_COOP_FALLEN &&
          X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && X3CoopRuntimeState()->anchor==1 &&
          !X3CoopRuntimeState()->wipe,"learner P2 inherits the world after a real P1 death");
    unsigned x=coop_x(1);for(unsigned f=0;f<24;++f)frame(SNES_PAD_LEFT<<12);
    check(coop_x(1)<x && (g_ppu->inidisp&0x8f)==15,"learner survivor keeps normal input and bright gameplay");
  }
  const char *fallen=getenv("X3_COOP_LEARNER_FALLEN_STATE");
  if(fallen && *fallen){
    check(RtlLoadSnapshot(fallen) && X3CoopRuntimeState()->players[1].life==X3_COOP_FALLEN,
          "learner persistence starts with a real fallen P2");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);base[learner_profile(base,n)+6]|=2;
    check(RtlLoadSnapshotFromMemory(base,n),"learner option restores with a previously fallen participant");
    for(unsigned f=0;f<30;++f)frame(SNES_PAD_RIGHT<<12);
    check(X3CoopRuntimeState()->players[1].life==X3_COOP_FALLEN && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE,
          "learner protection cannot re-enroll a previously fallen P2");
  }
  free(base);free(pending);check(RtlLoadSnapshotFromMemory(original,size),"learner probes restore original gameplay");
}
