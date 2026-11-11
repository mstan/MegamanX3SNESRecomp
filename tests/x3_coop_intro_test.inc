/* Actual native Mac capture and story-Zero construction. The fixture starts
 * with two living participants and the original32HP capture actor. */
static void native_intro_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *handoff=malloc(capacity);check(handoff!=NULL,"intro handoff replay buffer allocates");
  for(unsigned reverse=0;reverse<2;++reverse){
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
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()),"intro personal health and reversed seats encode");
    check(RtlLoadSnapshotFromMemory(work,size),"two living intro participants restore");
    g_ram[X3_COOP_BODY+0x27]=11;X3CoopViewsSetState(&views);
    unsigned elapsed=0;
    for(;elapsed<6000;++elapsed){
      frame(elapsed%60<6?(SNES_PAD_B|(SNES_PAD_B<<12)):0);
      const X3CoopState *s=X3CoopRuntimeState();
      bool capture_live=false;
      for(unsigned d=0xd18;d<0x10d8;d+=64)
        if(g_ram[d] && g_ram[d+10]==0x62 && !g_ram[d+11] && g_ram[d+1]==2)capture_live=true;
      if(X3CoopRuntimeSceneActive() && s->anchor==zero && s->current==zero &&
          s->players[zero].body[0xb6]==2 && !s->players[zero].body[2] && !s->players[zero].body[0x6e] &&
          !g_ram[0xcc8] && !g_ram[0x1f22] && !capture_live)break;
    }
    check(elapsed<6000,"original capture dialogue releases the existing Zero seat");
    const X3CoopState *s=X3CoopRuntimeState();
    check(s->players[x].life==X3_COOP_ALIVE && s->players[x].character==X3_COOP_X &&
          !s->players[x].body[0xb6] && (s->players[x].body[0x27]&127)==11 && s->players[x].maximum_health==16,
          "captured X keeps his own body, health and native identity");
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
  intro_rescue_native_seen|=(pc&0x7fffff)==0x03ad7f?1:2;
}
/* The original Mac was defeated by a real Zero shot upstream. This starts
 * before the original5HP restraint is hit, without any rescue result set. */
static void native_intro_rescue_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  check(interp_bridge_add_pre_opcode_hook(0x03ad7f,intro_rescue_native_probe) &&
        interp_bridge_add_pre_opcode_hook(0x03ad87,intro_rescue_native_probe),"native X reconstruction observations register");
  for(unsigned reverse=0;reverse<2;++reverse){
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
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"personal health and actual rescue roster restore");
    door_install_player(&setup);g_ram[0xd7f]=1;
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
    unsigned elapsed=0;
    for(;elapsed<6000;++elapsed){
      frame(elapsed%60<6?(SNES_PAD_B|(SNES_PAD_B<<12)):0);
      const X3CoopState *s=X3CoopRuntimeState();
      if(!X3CoopRuntimeSceneActive() && s->current==x && s->anchor==x &&
          !g_ram[0xa8e] && !g_ram[0x9da] && !g_ram[0xcc8])break;
    }
    const X3CoopState *s=X3CoopRuntimeState();
    if(elapsed==6000)fprintf(stderr,"intro rescue roster%u seen%u current%u anchor%u selector%u HP%u scene%u text%u locks%u/%u\n",
        reverse,intro_rescue_native_seen,s->current,s->anchor,g_ram[0xa8e],g_ram[0x9ff],X3CoopRuntimeSceneActive(),g_ram[0xcc8],g_ram[0x1f22],g_ram[0x1f24]);
    check(elapsed<6000 && intro_rescue_native_seen==3,"native rescue returns X ownership and releases the retained partner");
    fprintf(stderr,"intro rescued roster%u Xlife%u selector%u HP%u max%u Zerolife%u selector%u HP%u max%u\n",
        reverse,s->players[x].life,s->players[x].body[0xb6],s->players[x].body[0x27],s->players[x].maximum_health,
        s->players[zero].life,s->players[zero].body[0xb6],s->players[zero].body[0x27],s->players[zero].maximum_health);
    check(s->players[x].life==X3_COOP_ALIVE && !s->players[x].body[0xb6] &&
          (s->players[x].body[0x27]&127)==s->players[x].maximum_health && s->players[zero].life==X3_COOP_ALIVE &&
          s->players[zero].body[0xb6]==2 && (s->players[zero].body[0x27]&127)==19,
          "original rescue refills X alone and preserves Zero's personal health and identity");
    size_t completed=RtlSaveSnapshotToMemory(work,capacity);
    check(RtlLoadSnapshotFromMemory(expected,pending),"native restraint death restores");
    for(unsigned i=0;i<=elapsed;++i)frame(i%60<6?(SNES_PAD_B|(SNES_PAD_B<<12)):0);
    size_t replayed=RtlSaveSnapshotToMemory(expected,capacity);
    same(work,completed,expected,replayed,"actual native X rescue and roster release replay identically");
  }
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
