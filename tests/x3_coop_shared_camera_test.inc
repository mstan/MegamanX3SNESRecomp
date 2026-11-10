/* Native target/bounds probes. Teleports position bodies for the original
 * camera logic; these are not authored pit-route acceptance. */
static void shared_camera_call(unsigned pc){
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0x1e58;
  g_cpu.P=(uint8_t)((g_cpu.P|0x10)&~0x20);cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc) && g_cpu.S==resume.S,"shared native camera helper returns with the real guest stack");
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
}
static void shared_prepare(const uint8_t *base,size_t n,uint8_t *work,X3CoopState *setup){
  memcpy(work,base,n);
  check(X3CoopEncode(setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,n),"shared camera position prerequisite restores");
  door_install_player(setup);X3CoopViewsConfigure(false);X3CoopRuntimeFrame();
}
static unsigned shared_distance(void){
  unsigned x=coop_x(0),other=coop_x(1);return x>other?x-other:other-x;
}
static unsigned shared_pit_victim,shared_pit_damage;
static void shared_pit_probe(CpuState *cpu,uint32_t pc){
  if((pc&0x7fffff)==0x00e30a && cpu->D==0x1e58 && g_ram[0x9fe]==127 &&
      X3CoopRuntimeState()->current==shared_pit_victim)++shared_pit_damage;
}
static void shared_pit_prepare(const char *ground,unsigned victim,uint8_t *base,uint8_t *work,size_t capacity){
    check(RtlLoadSnapshot(ground) && g_ram[0x1fae]==5,"authored pit starts from the real stage5 terrain map");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    /* The live layout has a raised floor at928..1183, y2720, then an
     * open column1184..1263 all the way below the original bottom. Place
     * both on that real ledge and let movement/gravity enter the hole. */
    for(unsigned seat=0;seat<2;++seat){
      uint8_t *body=setup.players[seat].body;
      unsigned x=seat==victim?1168:1104,y=setup.players[seat].character==X3_COOP_ZERO?2695:2703;
      body[5]=body[0x22]=(uint8_t)x;body[6]=body[0x23]=(uint8_t)(x>>8);
      body[8]=body[0x24]=(uint8_t)y;body[9]=body[0x25]=(uint8_t)(y>>8);
      body[4]=body[7]=body[2]=body[3]=0;memset(body+0x1a,0,6);
    }
    shared_prepare(base,n,work,&setup);view_word(0x1e5d,1000);view_word(0x1e7a,1000);
}
static void native_shared_authored_pits(const char *ground,uint8_t *base,uint8_t *work,size_t capacity){
  check(interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"original native pit-damage call is observed");
  for(unsigned victim=0;victim<2;++victim){
    shared_pit_prepare(ground,victim,base,work,capacity);
    unsigned anchor=X3CoopRuntimeState()->anchor;
    shared_pit_victim=victim;shared_pit_damage=0;unsigned walked=0;
    while(walked<64 && coop_x(victim)<1216 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE){
      frame(SNES_PAD_RIGHT<<(victim*12));++walked;
    }
    check(coop_x(victim)>=1200 && coop_x(victim)<1248,"actual native movement enters the authored pit opening");
    if(victim!=anchor)check(coop_x(anchor)==1104 && view_read(0x1e5d)>1000,
        "actual partner-led motion advances the native shared camera while the world actor stands still");
    unsigned falling=0;
    while(falling<120 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE){frame(0);++falling;}
    const X3CoopState *s=X3CoopRuntimeState();
    fprintf(stderr,"authored Unified pit seat%u walk%u fall%u xy%u,%u damage%u life%u/%u\n",
        victim,walked,falling,coop_x(victim),support_position(victim,8),shared_pit_damage,s->players[0].life,s->players[1].life);
    check(shared_pit_damage==1 && s->players[victim].life==X3_COOP_FALLEN &&
          s->players[victim^1].life==X3_COOP_ALIVE && s->anchor==(victim^1) && !s->wipe,
          "a real native pit kills only the entering actor and transfers world ownership once");
    unsigned x=coop_x(victim^1);for(unsigned f=0;f<24;++f)frame(SNES_PAD_LEFT<<((victim^1)*12));
    check(coop_x(victim^1)<x && (g_ppu->inidisp&0x8f)==15,
          "real pit survivor retains movement and bright native gameplay");
  }
}
static void native_shared_camera_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *base=malloc(capacity);check(base!=NULL,"shared camera reference buffer allocates");
  for(unsigned roster=0;roster<2;++roster){
    if(roster){const char *path=getenv("X3_COOP_SHARED_REVERSED_STATE");
      check(path && *path && RtlLoadSnapshot(path),"shared camera uses a real reversed native roster");}
    else check(RtlLoadSnapshotFromMemory(original,size),"shared camera starts from actual native gameplay");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState initial=*X3CoopRuntimeState();
    unsigned anchor=initial.anchor,partner=anchor^1;
    for(unsigned axis=0;axis<2;++axis)for(int direction=-1;direction<=1;direction+=2){
      unsigned offset=axis?8:5,address=axis?0x1e60:0x1e5d;
      X3CoopState setup=initial;int origin=support_position(anchor,offset),other=origin+direction*97;
      setup.players[partner].body[offset]=(uint8_t)other;setup.players[partner].body[offset+1]=(uint8_t)(other>>8);
      shared_prepare(base,n,work,&setup);view_word(address,0);
      unsigned zone=view_read(axis?0x1e82:0x1e86),pc=axis?0x00e0a7:0x00e088;
      check(origin>97+(int)zone,"prepared positions enter the original camera's positive tracking branch");
      shared_camera_call(pc);
      check(view_read(address)==(unsigned)(origin+direction*48)-(unsigned)zone,
            "native Unified target follows both actors and rounds an odd gap toward the world owner");
      check(support_position(anchor,offset)==(unsigned)origin && support_position(partner,offset)==(unsigned)other,
            "shared camera target adjustment never overwrites either native body position");
    }
    for(unsigned victim=0;victim<2;++victim)for(unsigned position=0;position<3;++position){
      check(RtlLoadSnapshotFromMemory(base,n),"native camera bound fixture restores");
      unsigned current=view_read(0x1e6c),target=view_read(0x1e7e),bottom=(current>target?current:target)+224;
      bool beyond=position==1;X3CoopState setup=initial;unsigned y=position==2?0xfff0:bottom+31+beyond;
      setup.players[victim].body[8]=(uint8_t)y;setup.players[victim].body[9]=(uint8_t)(y>>8);
      shared_prepare(base,n,work,&setup);
      check(view_read(0x1e60)<=current && bottom<8192,"native camera is above the actual authored lower bound");
      size_t prepared=RtlSaveSnapshotToMemory(expected,capacity);
      shared_camera_call(0x00e2a2);
      check(X3CoopRuntimeState()->players[victim].life==(beyond?X3_COOP_FALLEN:X3_COOP_ALIVE) &&
            X3CoopRuntimeState()->players[victim^1].life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe,
            "original camera bounds distinguish an offscreen body from a real pit threshold for either actor");
      if(beyond)check(X3CoopRuntimeState()->anchor==(victim^1),"native pit damage leaves the other living actor in charge");
      size_t result=RtlSaveSnapshotToMemory(work,capacity);uint8_t *observed=malloc(result);
      check(observed!=NULL,"native pit-bound replay buffer allocates");memcpy(observed,work,result);
      check(RtlLoadSnapshotFromMemory(expected,prepared),"shared camera pit prerequisite restores for replay");
      shared_camera_call(0x00e2a2);size_t replay=RtlSaveSnapshotToMemory(work,capacity);
      same(observed,result,work,replay,"balanced native Unified camera bounds replay the complete machine");free(observed);
    }
  }
  const char *ground=getenv("X3_COOP_SHARED_GROUND_STATE");
  if(ground && *ground)for(unsigned lead=0;lead<2;++lead)for(unsigned air=0;air<2;++air){
    check(RtlLoadSnapshot(ground),"shared catch-up starts on actual stage5 terrain");
    X3CoopViewsConfigure(false);unsigned lag=lead^1,initial_y=support_position(lead,8);
    if(air){
      for(unsigned f=0;f<12;++f)frame(SNES_PAD_B<<(lead*12));
      check(support_position(lead,8)+24<initial_y,"real leader jump creates the unsafe airborne return prerequisite");
    }
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    unsigned lag_x=coop_x(lead)+224; /* Within the actual room's horizontal bounds. */
    setup.players[lag].body[5]=setup.players[lag].body[0x22]=(uint8_t)lag_x;
    setup.players[lag].body[6]=setup.players[lag].body[0x23]=(uint8_t)(lag_x>>8);
    setup.players[lag].body[0x27]=11;
    shared_prepare(base,n,work,&setup);check(shared_distance()>192,"catch-up begins beyond the native shared separation limit");
    size_t prepared=RtlSaveSnapshotToMemory(base,capacity);
    for(unsigned f=0;f<4;++f)frame(0);
    const X3CoopState *s=X3CoopRuntimeState();
    check(s->players[lag].life==X3_COOP_ALIVE && s->players[lead].life==X3_COOP_ALIVE &&
          contact_hp(lag)==11 && s->anchor==setup.anchor,"catch-up preserves both life states, personal HP and world ownership");
    check(air?shared_distance()>192:shared_distance()<=64,
          air?"catch-up waits when the real leader is airborne without a safe landing":
              "safe catch-up returns a distant living actor beside either leading seat");
    size_t result=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(base,prepared),"shared catch-up prerequisite restores for replay");
    for(unsigned f=0;f<4;++f)frame(0);
    size_t replay=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,result,work,replay,"native catch-up and its unsafe refusal replay the complete machine");
  }
  if(ground && *ground)native_shared_authored_pits(ground,base,work,capacity);
  free(base);check(RtlLoadSnapshotFromMemory(original,size),"shared camera probes restore their original gameplay");
}
