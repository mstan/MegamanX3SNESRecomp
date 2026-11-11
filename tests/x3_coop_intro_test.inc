/* Actual native Mac capture and story-Zero construction. The fixture starts
 * with two living participants and the original32HP capture actor. */
static unsigned intro_confirm(unsigned tick){
  return tick%60<6?(uint32_t)SNES_PAD_B<<(12*X3CoopRuntimeState()->anchor):0;
}
static unsigned intro_constructor_seen;
static void intro_constructor_probe(CpuState *cpu,uint32_t pc){
  (void)cpu;
  unsigned kind=(pc&0x7fffff)==0x03ac69?7:8;
  if(!X3CoopRuntimeSceneActive())return;
  X3CoopState retained=*X3CoopRuntimeState();
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);
  size_t at=door_scene_chunk(view_probe.bytes,view_probe.used);
  if(view_probe.bytes[at+11]==9)return; /* Retained Zero owns the separate original X cinematic. */
  check(view_probe.bytes[at+11]==kind && !memcmp(&retained,X3CoopRuntimeState(),sizeof(retained)),
      "actual interrupted native constructor retains both personal contexts");
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"actual pending native constructor passes preflight");
  view_probe.used=0;view_probe.load=true;
  check(X3CoopRuntimeLoad(&view_probe.io),"actual pending native constructor restores in place");
  intro_constructor_seen|=kind==7?1:2;
}
static void native_intro_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *handoff=malloc(capacity);check(handoff!=NULL,"intro handoff replay buffer allocates");
  check(interp_bridge_add_pre_opcode_hook(0x03ac69,intro_constructor_probe),"actual Zero constructor observation registers");
  for(unsigned reverse=0;reverse<2;++reverse)for(unsigned mode=getenv("X3_COOP_INTRO_UNAVAILABLE_ONLY")?1:0;mode<4;++mode){
    check(RtlLoadSnapshotFromMemory(original,size),"actual Mac capture approach restores");
    check(!g_ram[0x1fae] && g_ram[0xd18] && g_ram[0xd22]==0x62 && !g_ram[0xd23] &&
          g_ram[0xd3f]==32,"capture starts with the actual original Mac and no assisted outcome");
    X3CoopState setup=*X3CoopRuntimeState();
    X3CoopViewState views=*X3CoopViewsState();
    if(reverse){
      X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;
      setup.current^=1;setup.anchor^=1;
      for(unsigned i=0;i<4;++i){uint16_t t=views.limits[0][i];views.limits[0][i]=views.limits[1][i];views.limits[1][i]=t;
        t=views.current[0][i];views.current[0][i]=views.current[1][i];views.current[1][i]=t;}
      for(unsigned i=0;i<2;++i){uint16_t t=views.camera[0][i];views.camera[0][i]=views.camera[1][i];views.camera[1][i]=t;}
      views.valid=(uint8_t)(((views.valid&1)<<1)|((views.valid&2)>>1));
      views.camera_valid=(uint8_t)(((views.camera_valid&1)<<1)|((views.camera_valid&2)>>1));
    }
    unsigned x=reverse,zero=x^1;
    setup.players[x].body[0x27]=11;setup.players[zero].body[0x27]=19;
    if(mode==1 || mode==3)check(X3CoopWithdraw(&setup,zero),"intro unavailable Zero precondition withdraws through lifecycle API");
    if(mode==2)check(X3CoopFall(&setup,zero),"intro prior-fallen Zero precondition uses lifecycle API");
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()),"intro personal health and reversed seats encode");
    if(mode==3)work[door_scene_chunk(work,size)+16+6]=1;
    check(RtlLoadSnapshotFromMemory(work,size),"intro participant preconditions restore");
    g_ram[X3_COOP_BODY+0x27]=11;X3CoopViewsSetState(&views);
    unsigned elapsed=0,helper=mode?x:zero;intro_constructor_seen=0;
    for(;elapsed<6000;++elapsed){
      frame(intro_confirm(elapsed));
      const X3CoopState *s=X3CoopRuntimeState();
      bool capture_live=false;
      for(unsigned d=0xd18;d<0x10d8;d+=64)
        if(g_ram[d] && g_ram[d+10]==0x62 && !g_ram[d+11] && g_ram[d+1]==2)capture_live=true;
      if(X3CoopRuntimeSceneActive() && s->anchor==helper && s->current==helper &&
          s->players[helper].body[0xb6]==(mode?0:2) && !s->players[helper].body[2] && !s->players[helper].body[0x6e] &&
          !g_ram[0xcc8] && !g_ram[0x1f22] && !capture_live)break;
    }
    fprintf(stderr,"intro capture roster%u mode%u elapsed%u nativeSelector%u Xlife%u HP%u Zerolife%u HP%u\n",reverse,mode,elapsed,
        g_ram[0xa8e],X3CoopRuntimeState()->players[x].life,X3CoopRuntimeState()->players[x].body[0x27],
        X3CoopRuntimeState()->players[zero].life,X3CoopRuntimeState()->players[zero].body[0x27]);
    check(elapsed<6000,"original capture dialogue releases the available participant");
    check(intro_constructor_seen&1,"capture exercised the actual pending native Zero constructor");
    const X3CoopState *s=X3CoopRuntimeState();
    check(s->players[x].life==X3_COOP_ALIVE && s->players[x].character==X3_COOP_X &&
          !s->players[x].body[0xb6] && (s->players[x].body[0x27]&127)==11 && s->players[x].maximum_health==16,
          "captured X keeps his own body, health and native identity");
    check(!memcmp(s->players[x].palette,setup.players[x].palette,X3_COOP_PALETTE_BYTES),
          "original Zero graphics setup cannot replace retained X's native palette");
    if(mode){
      check(s->players[zero].life==(mode==2?X3_COOP_FALLEN:X3_COOP_ABSENT) && s->players[zero].body[0xb6]==2 &&
          (s->players[zero].body[0x27]&127)==19 && s->players[zero].maximum_health==28,
          "unavailable Zero remains retired with his original personal resources");
      size_t pending=RtlSaveSnapshotToMemory(handoff,capacity);unsigned before=coop_x(x),movement=0;
      for(;movement<600 && coop_x(x)<=before+8;++movement)frame((uint32_t)SNES_PAD_RIGHT<<(12*x));
      check(coop_x(x)>before+8 && X3CoopRuntimeState()->players[x].body[0xb6]==0,
          "living X actually moves after the original helper was already unavailable");
      size_t completed=RtlSaveSnapshotToMemory(expected,capacity);
      check(RtlLoadSnapshotFromMemory(handoff,pending),"unavailable-helper native entry restores");
      for(unsigned i=0;i<movement;++i)frame((uint32_t)SNES_PAD_RIGHT<<(12*x));
      size_t replayed=RtlSaveSnapshotToMemory(work,capacity);
      same(expected,completed,work,replayed,"unavailable-helper native movement replays the same complete machine");
      continue;
    }
    check(s->players[zero].life==X3_COOP_ALIVE && s->players[zero].character==X3_COOP_ZERO &&
          s->players[zero].body[0xb6]==2 && (s->players[zero].body[0x27]&127)==19 && s->players[zero].maximum_health==28,
          "story construction belongs to the existing Zero without refilling personal health");
    size_t handoff_size=RtlSaveSnapshotToMemory(handoff,capacity);
    check(handoff_size!=0,"native intro ownership handoff saves");
    if(getenv("X3_COOP_INTRO_PROBE"))check(RtlSaveSnapshot("intro-handoff-probe.sav"),"intro diagnostic handoff retains its native continuation");
    unsigned held_x=coop_x(x),before=coop_x(zero);
    /* The original constructor has set standing state before its blocking
     * graphics decompression returns. Require real controller movement,
     * retaining/replaying that pending native work too. */
    unsigned movement_frames=0;
    for(;movement_frames<600;++movement_frames){
      frame((uint32_t)SNES_PAD_RIGHT<<(12*zero));
      if(coop_x(zero)>before+8){++movement_frames;break;}
    }
    if(coop_x(x)!=held_x || coop_x(zero)<=before+8)
      fprintf(stderr,"intro roster%u elapsed%u X%u->%u Zero%u->%u nativeXY%u/%u action%u lock%u/%u flags%u/%u pad%08x actions%04x\n",
          reverse,elapsed,held_x,coop_x(x),before,coop_x(zero),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),
          g_ram[X3_COOP_BODY+2],g_ram[0x1f22],g_ram[0x1f24],g_ram[0x1f45],g_ram[0x1f63],
          (unsigned)RtlGetPadState(zero),view_read(X3_COOP_BODY+0x36));
    if(coop_x(zero)<=before+8)
      fprintf(stderr,"intro body%u/%u freezes%u/%u/%u/%u/%u/%u/%u native%u/%u Mac%u/%u map%04x\n",
          g_ram[X3_COOP_BODY],g_ram[X3_COOP_BODY+1],g_ram[0x1f25],g_ram[0x1f26],g_ram[0x1f27],
          g_ram[0x1f28],g_ram[0x1f29],g_ram[0x1f2a],g_ram[0x1f2b],g_ram[0xd1],g_ram[0xd2],
          g_ram[0xd22],g_ram[0xd19],view_read(0xffc0));
    if(coop_x(zero)<=before+8 && getenv("X3_COOP_INTRO_PROBE"))
      check(RtlSaveSnapshot("intro-stalled-probe.sav"),"intro diagnostic movement retains its native continuation");
    check(coop_x(x)==held_x && coop_x(zero)>before+8,"the Zero seat moves while captured X remains parked");
    size_t expected_size=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(handoff,handoff_size),"intro ownership handoff restores");
    for(unsigned i=0;i<movement_frames;++i)frame((uint32_t)SNES_PAD_RIGHT<<(12*zero));
    size_t actual=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,expected_size,work,actual,"native intro handoff and personal resources replay identically");
    check(RtlLoadSnapshotFromMemory(handoff,handoff_size),"intro malformed-state setup restores");
    memcpy(work,handoff,handoff_size);work[door_scene_chunk(work,handoff_size)+7]=1;
    check(!RtlLoadSnapshotFromMemory(work,handoff_size),"a Zero gameplay scene cannot contain an unfinished departure phase");
    actual=RtlSaveSnapshotToMemory(work,capacity);
    same(handoff,handoff_size,work,actual,"refused intro scene leaves the machine unchanged");
  }
  free(handoff);check(RtlLoadSnapshotFromMemory(original,size),"intro probes restore their original capture approach");
}

static unsigned intro_rescue_native_seen;
static void intro_rescue_native_probe(CpuState *cpu,uint32_t pc){
  const X3CoopState *s=X3CoopRuntimeState();
  fprintf(stderr,"intro rescue native%06x D%04x stage%u enabled%u current%u anchor%u selector%u HP%u\n",
      (unsigned)pc,cpu->D,g_ram[0x1fae],X3CoopRuntimeEnabled(),s->current,s->anchor,
      g_ram[0xa8e],g_ram[0x9ff]);
  fprintf(stderr,"intro rescue NPC xy%u/%u action%u bodyxy%u/%u action%u\n",
      view_read(cpu->D+5),view_read(cpu->D+8),g_ram[cpu->D+2],view_read(0x9dd),view_read(0x9e0),g_ram[0x9da]);
  intro_rescue_native_seen|=(pc&0x7fffff)==0x03ad70?1:2;
}
/* The original Mac was defeated by a real Zero shot upstream. This starts
 * before the original5HP restraint is hit, without any rescue result set. */
static void native_intro_rescue_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *cinematic=malloc(capacity);check(cinematic!=NULL,"native cinematic replay buffer allocates");
  check(interp_bridge_add_pre_opcode_hook(0x03ad70,intro_rescue_native_probe) &&
        interp_bridge_add_pre_opcode_hook(0x03ad87,intro_rescue_native_probe) &&
        interp_bridge_add_pre_opcode_hook(0x03ad83,intro_constructor_probe),"native X reconstruction observations register");
  for(unsigned reverse=0;reverse<2;++reverse)for(unsigned retired=0;retired<3;++retired){
    check(RtlLoadSnapshotFromMemory(original,size),"actual native restraint fixture restores");
    X3CoopState setup=*X3CoopRuntimeState();unsigned zero=reverse?0:1,x=zero^1;
    check(g_ram[0xd58] && g_ram[0xd59]==2 && g_ram[0xd62]==0x4c && g_ram[0xd7f]==5,
        "original captive restraint is alive at5HP");
    memcpy(work,original,size);size_t scene_at=door_scene_chunk(work,size);
    if(reverse){
      X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;
      setup.current^=1;setup.anchor^=1;work[scene_at+6]^=1;
      uint8_t raw[2];memcpy(raw,work+scene_at+12,2);memcpy(work+scene_at+12,work+scene_at+14,2);memcpy(work+scene_at+14,raw,2);
    }
    setup.players[x].body[0x27]=11;setup.players[zero].body[0x27]=19;
    /* Explicit previously-fallen precondition; this does not claim to test
     * a native cause of death. The actual restraint/rescue must not revive X. */
    if(retired)check(X3CoopFall(&setup,x),"previously fallen X precondition uses the lifecycle API");
    if(retired==2){
      /* The real prior-fallen-X route reaches this room without enrolling
       * the original capture scene. Keep its actual restraint/combat state. */
      uint8_t both=work[scene_at+10];memset(work+scene_at+4,0,12);work[scene_at+10]=both;
      size_t ram_at;check(RtlSnapshotGuestLayout(NULL,&ram_at)!=0,"untracked rescue uses the actual guest RAM layout");
      work[ram_at+0x1f45]=0; /* Actual prior-fallen-X route never acquired the capture token. */
    }
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"personal health and actual rescue roster restore");
    door_install_player(&setup);
    const char *fallen_output=getenv("X3_COOP_INTRO_FALLEN_OUTPUT");
    if(!reverse && retired==1 && fallen_output && *fallen_output)
      check(RtlSaveSnapshot(fallen_output),"previously fallen X fixture retains the original live restraint for launcher QA");
    g_ram[0xd7f]=1;
    for(unsigned i=0;i<3;++i)frame(0);
    check(g_ram[0xd59]==2 && g_ram[0xd7f]==1,"restraint survives health assistance before any attack");
    bool hit=false;intro_rescue_native_seen=0;
    for(unsigned cycle=0;cycle<16 && !hit;++cycle){
      view_word(0x9dd,6208);view_word(0x9e0,574);
      frame((uint32_t)SNES_PAD_RIGHT<<(zero*12));
      for(unsigned i=0;i<8;++i){
        frame(i<4?(uint32_t)SNES_PAD_Y<<(zero*12):0);
        if(g_ram[0xd59]==4 && (g_ram[0xd7f]&128)){hit=true;break;}
      }
    }
    check(hit,"a real native Zero shot destroys the captive restraint");
    size_t pending=RtlSaveSnapshotToMemory(expected,capacity);
    check(pending!=0,"the actual restraint death saves before X release");
    unsigned elapsed=0,cinematic_frame=0;size_t cinematic_size=0;intro_constructor_seen=0;
    for(;elapsed<6000;++elapsed){
      frame(intro_confirm(elapsed));
      if(retired && !cinematic_size && !X3CoopRuntimeReady() && X3CoopRuntimeSceneActive()){
        X3CoopState retained=*X3CoopRuntimeState();
        cinematic_size=RtlSaveSnapshotToMemory(cinematic,capacity);cinematic_frame=elapsed;
        check(cinematic_size && cinematic[door_scene_chunk(cinematic,cinematic_size)+11]==9,
            "actual original X cinematic saves while the participant stays fallen");
        check(!memcmp(&retained,X3CoopRuntimeState(),sizeof(retained)),
            "saving the temporary native X body cannot overwrite retained Zero");
        check(RtlLoadSnapshotFromMemory(cinematic,cinematic_size),"actual temporary X cinematic restores");
        size_t restored=RtlSaveSnapshotToMemory(work,capacity);
        same(cinematic,cinematic_size,work,restored,"temporary X cinematic immediate restore preserves the full machine");
      }
      const X3CoopState *s=X3CoopRuntimeState();
      unsigned owner=retired?zero:x;
      if(intro_rescue_native_seen==3 && !X3CoopRuntimeSceneActive() && s->current==owner && s->anchor==owner &&
          g_ram[0xa8e]==(retired?2:0) && !g_ram[0x9da] && !g_ram[0xcc8])break;
    }
    const X3CoopState *s=X3CoopRuntimeState();
    if(elapsed==6000)fprintf(stderr,"intro rescue roster%u retired%u seen%u current%u anchor%u selector%u HP%u scene%u text%u locks%u/%u\n",
        reverse,retired,intro_rescue_native_seen,s->current,s->anchor,g_ram[0xa8e],g_ram[0x9ff],X3CoopRuntimeSceneActive(),g_ram[0xcc8],g_ram[0x1f22],g_ram[0x1f24]);
    check(elapsed<6000 && intro_rescue_native_seen==3,"native rescue releases the living participant without reviving a fallen partner");
    if(!retired)check(intro_constructor_seen&2,"rescue exercised the actual pending native X constructor");
    fprintf(stderr,"intro rescued roster%u Xlife%u selector%u HP%u max%u Zerolife%u selector%u HP%u max%u\n",
        reverse,s->players[x].life,s->players[x].body[0xb6],s->players[x].body[0x27],s->players[x].maximum_health,
        s->players[zero].life,s->players[zero].body[0xb6],s->players[zero].body[0x27],s->players[zero].maximum_health);
    check(s->players[x].life==(retired?X3_COOP_FALLEN:X3_COOP_ALIVE) && !s->players[x].body[0xb6] &&
          (s->players[x].body[0x27]&127)==(retired?11:s->players[x].maximum_health) && s->players[zero].life==X3_COOP_ALIVE &&
          s->players[zero].body[0xb6]==2 && (s->players[zero].body[0x27]&127)==19 && s->players[zero].maximum_health==28,
          "original rescue preserves fallen X and Zero's personal health, refilling only a living X");
    size_t completed=RtlSaveSnapshotToMemory(work,capacity);
    check(RtlLoadSnapshotFromMemory(expected,pending),"native restraint death restores");
    for(unsigned i=0;i<=elapsed;++i)frame(intro_confirm(i));
    size_t replayed=RtlSaveSnapshotToMemory(expected,capacity);
    same(work,completed,expected,replayed,"actual native X rescue and roster release replay identically");
    if(retired){
      check(cinematic_size && RtlLoadSnapshotFromMemory(cinematic,cinematic_size),"original cinematic resumes with X still fallen");
      for(unsigned i=cinematic_frame+1;i<=elapsed;++i)frame(intro_confirm(i));
      replayed=RtlSaveSnapshotToMemory(expected,capacity);
      same(work,completed,expected,replayed,"temporary X cinematic and retained Zero replay to the same complete machine");
      check(RtlLoadSnapshotFromMemory(cinematic,cinematic_size),"cinematic invalid-state setup restores");
      memcpy(work,cinematic,cinematic_size);work[door_scene_chunk(work,cinematic_size)+7]=1;
      check(!RtlLoadSnapshotFromMemory(work,cinematic_size),"temporary cinematic refuses a participant departure phase");
      size_t restored=RtlSaveSnapshotToMemory(work,capacity);
      same(cinematic,cinematic_size,work,restored,"refused cinematic state leaves native CPU and retained actors unchanged");
    }
  }
  free(cinematic);
  check(RtlLoadSnapshotFromMemory(original,size),"rescue checks restore their original restraint approach");
}

static void intro_survivor_shot_frame(unsigned seat,unsigned tick){
  if(tick%25==0){
    view_word(0x9dd,6152);view_word(0x9e0,view_read(0xd20)+14);g_ram[0x9ff]=11;
  }
  frame((uint32_t)(tick%25==0?SNES_PAD_RIGHT:tick%25<=12?SNES_PAD_Y:0)<<(12*seat));
}
static void native_intro_helper_death_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  for(unsigned reverse=0;reverse<2;++reverse){
    check(RtlLoadSnapshotFromMemory(original,size),"actual native Mac encounter restores before helper death");
    X3CoopState setup=*X3CoopRuntimeState();unsigned zero=reverse?0:1,x=zero^1;
    memcpy(work,original,size);size_t scene_at=door_scene_chunk(work,size);
    if(reverse){
      X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;
      setup.current^=1;setup.anchor^=1;work[scene_at+6]^=1;
      uint8_t raw[2];memcpy(raw,work+scene_at+12,2);memcpy(work+scene_at+12,work+scene_at+14,2);memcpy(work+scene_at+14,raw,2);
    }
    setup.players[zero].body[0x27]=1;setup.players[x].body[0x27]=11;
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"helper health-only fatal-contact precondition restores");
    door_install_player(&setup);
    unsigned waited=0;bool first_fall=false;
    for(;waited<1800;++waited){
      frame(0);
      const X3CoopState *fall=X3CoopRuntimeState();
      if(!first_fall && (fall->players[x].life!=X3_COOP_ALIVE || fall->players[zero].life!=X3_COOP_ALIVE)){
        first_fall=true;fprintf(stderr,"intro first fall roster%u tick%u Xlife%u HP%u xy%u/%u Zerolife%u HP%u xy%u/%u\n",
            reverse,waited,fall->players[x].life,fall->players[x].body[0x27],coop_x(x),
            fall->players[x].body[8]|fall->players[x].body[9]<<8,fall->players[zero].life,
            fall->players[zero].body[0x27],coop_x(zero),fall->players[zero].body[8]|fall->players[zero].body[9]<<8);
      }
      if(X3CoopRuntimeState()->players[zero].life==X3_COOP_FALLEN)break;
    }
    const X3CoopState *s=X3CoopRuntimeState();
    check(waited<1800 && s->players[x].life==X3_COOP_ALIVE &&
          g_ram[0xd22]==0x62 && g_ram[0xd23]==1 && g_ram[0xd3f]==32,
          "original Mac contact kills the helper while captured X remains alive");
    /* Complete that frame's normal scene fallback, without writing its
     * control, capture, life, result or progression flags. */
    frame(0);g_ram[0xd3f]=1;
    for(unsigned i=0;i<3;++i)frame(0);
    check(g_ram[0xd19]==2 && g_ram[0xd3f]==1,"Mac survives health assistance before the survivor attacks");
    size_t survivor=RtlSaveSnapshotToMemory(work,capacity);
    unsigned fired=0;
    for(;fired<400;++fired){
      if(X3CoopRuntimeState()->players[x].life!=X3_COOP_ALIVE)break;
      intro_survivor_shot_frame(x,fired);
      if(g_ram[0xd19]==4 && (g_ram[0xd3f]&128)){++fired;break;}
    }
    s=X3CoopRuntimeState();
    fprintf(stderr,"intro helper death roster%u wait%u shots%u Xlife%u action%u HP%u Zero life%u Mac%u/%u\n",
        reverse,waited,fired,s->players[x].life,s->players[x].body[2],s->players[x].body[0x27],s->players[zero].life,g_ram[0xd19],g_ram[0xd3f]);
    check(s->players[x].life==X3_COOP_ALIVE && !s->players[x].body[0xb6] &&
          fired<400 && g_ram[0xd19]==4 && (g_ram[0xd3f]&128) && s->players[zero].life==X3_COOP_FALLEN,
          "a real X shot defeats Mac without reviving the fallen intro helper");
    size_t completed=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(work,survivor),"actual helper death and X control handoff restore");
    for(unsigned i=0;i<fired;++i)intro_survivor_shot_frame(x,i);
    size_t replayed=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,completed,work,replayed,"actual intro helper death and survivor control replay identically");
  }
  check(RtlLoadSnapshotFromMemory(original,size),"helper death checks restore the actual Mac approach");
}
