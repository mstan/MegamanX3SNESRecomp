/* Earned-intro stage2 fixture, original32HP Buffalo, no supplied death/clear.
 * Health/position assistance isolates survivor combat and campaign ownership. */
static unsigned campaign_survivor,campaign_victim,campaign_boss_targets;
static bool campaign_probing;
static void campaign_target_probe(CpuState *cpu,uint32_t pc){
  (void)pc;const X3CoopState *s=X3CoopRuntimeState();
  if(!campaign_probing || cpu->D!=0xd18 || g_ram[0xd22]!=0x52 || g_ram[0xd19]!=2 ||
      s->players[campaign_victim].life!=X3_COOP_FALLEN)return;
  ++campaign_boss_targets;
  check(s->current==campaign_survivor && g_ram[X3_COOP_BODY+0xb6]==s->players[campaign_survivor].character*2 && (g_ram[0x9ff]&127),
      "original Buffalo update receives the living survivor target after native partner death");
}
static void native_campaign_survivor_checks(uint8_t *original,size_t size,uint8_t *work){
  campaign_probing=true;
  check(interp_bridge_add_pre_opcode_hook(0x00dd1a,campaign_target_probe),"actual boss target observation registers");
  for(unsigned reverse=getenv("X3_COOP_CAMPAIGN_REVERSED_ONLY")?1:0;reverse<2;++reverse){
    check(RtlLoadSnapshotFromMemory(original,size) && g_ram[0x1fae]==2 && g_ram[0x1fd3]==1 &&
        g_ram[0xd22]==0x52 && g_ram[0xd3f]==32 && !g_ram[0x1fc8],"original Buffalo approach retains earned intro and an unearned reward");
    X3CoopState setup=*X3CoopRuntimeState();
    if(reverse){X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;
      setup.current^=1;setup.anchor^=1;}
    campaign_survivor=(reverse?1:0)^(getenv("X3_COOP_CAMPAIGN_ZERO_SURVIVOR")?1:0);campaign_victim=campaign_survivor^1;campaign_boss_targets=0;
    unsigned maximum=setup.players[campaign_survivor].maximum_health;
    setup.players[campaign_victim].body[0x27]=1;setup.players[campaign_victim].body[0x30]=0;
    setup.players[campaign_survivor].body[0x27]=(uint8_t)maximum;team_hazard_body(setup.players[campaign_survivor].body,6010,925);
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,size),"personal health/position and reversed campaign roster preconditions restore");
    door_install_player(&setup);X3CoopViewsConfigure(false);
    unsigned contact=0;
    for(;contact<1200 && X3CoopRuntimeState()->players[campaign_victim].life==X3_COOP_ALIVE;++contact){
      if(X3CoopRuntimeState()->current==campaign_survivor)g_ram[0x9ff]=(uint8_t)maximum;frame(0);
    }
    check(contact<1200 && X3CoopRuntimeState()->players[campaign_survivor].life==X3_COOP_ALIVE &&
        g_ram[0xd19]==2 && g_ram[0xd3f]==32,"actual original boss encounter kills the partner while the survivor and the uninjured boss remain active");
    g_ram[0xd3f]=1;for(unsigned i=0;i<3;++i){if(X3CoopRuntimeState()->current==campaign_survivor)g_ram[0x9ff]=(uint8_t)maximum;frame(0);}
    check(g_ram[0xd3f]==1,"boss health assistance survives idle frames before any attack");
    bool hit=false;unsigned fired=0;
    for(;fired<600 && !hit;++fired){
      if(X3CoopRuntimeState()->current==campaign_survivor){
        g_ram[0x9ff]=(uint8_t)maximum;
        if(fired%24==0){view_word(0x9dd,view_read(0xd1d)+72);view_word(0x9e0,view_read(0xd20)+14);}
      }
      frame((uint32_t)(SNES_PAD_LEFT|(fired%24<12?SNES_PAD_Y:0))<<(campaign_survivor*12));
      hit=g_ram[0xd19]==4 && !g_ram[0xd3f];
    }
    check(hit && campaign_boss_targets && X3CoopRuntimeState()->players[campaign_victim].life==X3_COOP_FALLEN,
        "a real survivor native buster defeats Buffalo while native AI targets the survivor and the partner remains fallen");
    unsigned transition=0;
    for(;transition<6000;++transition){
      frame(transition%120<6?(uint32_t)SNES_PAD_B<<(campaign_survivor*12):0);
      if(g_ram[0xd1]==4 && g_ram[0xd2]==4 && g_ram[0x1fae]!=2 &&
          X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0x9d9]==2 && g_ram[0x9da]<=0x16 &&
          !g_ram[0x1f22] && !g_ram[0xa46] && g_ram[0xa8e]==X3CoopRuntimeState()->players[campaign_survivor].character*2)break;
    }
    const X3CoopState *s=X3CoopRuntimeState();
    fprintf(stderr,"Buffalo survivor roster%u character%u contact%u fired%u targets%u transition%u stage%u phase%u/%u reward%02x life%u/%u selector%u\n",
        reverse,s->players[campaign_survivor].character,contact,fired,campaign_boss_targets,transition,g_ram[0x1fae],g_ram[0xd1],g_ram[0xd2],g_ram[0x1fc8],
        s->players[0].life,s->players[1].life,g_ram[0xa8e]);
    const char *stalled=getenv("X3_COOP_CAMPAIGN_STALLED_OUTPUT");
    if(transition==6000 && stalled && *stalled)check(RtlSaveSnapshot(stalled),"actual stalled reward screen retains a reviewable native state");
    check(transition<6000 && (g_ram[0x1fc8]&64) && s->players[campaign_survivor].life==X3_COOP_ALIVE &&
        s->players[campaign_survivor].body[0xb6]==s->players[campaign_survivor].character*2 && s->players[campaign_victim].life==X3_COOP_FALLEN &&
        s->players[campaign_victim].body[0xb6]==s->players[campaign_victim].character*2,"original reward, exit and actual next stage retain the sole living survivor without reviving the partner");
    unsigned before=support_position(campaign_survivor,5),clock=g_ram[0x9cc];
    for(unsigned i=0;i<24;++i)frame((uint32_t)SNES_PAD_RIGHT<<(campaign_survivor*12));
    check(support_position(campaign_survivor,5)>before && (uint8_t)(g_ram[0x9cc]-clock)==24 &&
        X3CoopRuntimeState()->players[campaign_victim].life==X3_COOP_FALLEN,
        "actual next-stage survivor moves with one world tick per frame and the partner still fallen");
  }
  campaign_probing=false;
  check(RtlLoadSnapshotFromMemory(original,size),"campaign survivor probes restore their original earned-intro approach");
}

/* Both actual fatal contacts come from the original live Buffalo encounter. */
static void native_boss_retry_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  for(unsigned reverse=0;reverse<2;++reverse){
    check(RtlLoadSnapshotFromMemory(original,size),"boss retry restores its original earned-intro arena");
    X3CoopState setup=*X3CoopRuntimeState();
    if(reverse){X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;setup.current^=1;setup.anchor^=1;}
    unsigned x=reverse?1:0,zero=x^1,lives=g_ram[0x1fb4],checkpoint=g_ram[0x1fb5];
    setup.players[zero].body[0x27]=1;setup.players[zero].body[0x30]=0;setup.players[x].body[0x27]=16;
    team_hazard_body(setup.players[x].body,6010,925);memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),"boss retry personal health and roster prerequisites restore");
    door_install_player(&setup);X3CoopViewsConfigure(false);unsigned first=0;
    for(;first<1200 && X3CoopRuntimeState()->players[zero].life==X3_COOP_ALIVE;++first){if(X3CoopRuntimeState()->current==x)g_ram[0x9ff]=16;frame(0);}
    check(first<1200 && X3CoopRuntimeState()->players[x].life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe,"real boss contact leaves X as the last survivor");
    setup=*X3CoopRuntimeState();setup.players[x].body[0x27]=1;setup.players[x].body[0x30]=0;
    team_hazard_body(setup.players[x].body,view_read(0xd1d)-28,view_read(0xd20)+8);
    size_t n=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"last survivor approaches the actual boss without supplying death or retry results");
    door_install_player(&setup);size_t cut=RtlSaveSnapshotToMemory(original,capacity);
    X3CoopRuntimeEnable(false);unsigned solo=0;
    for(;solo<3600;++solo){frame(0);if(g_ram[0xd1]==4 && g_ram[0xd2]==4 && g_ram[0x9d9]==2 && g_ram[0x9da]<=0x16 && !g_ram[0x1f22] && !g_ram[0xa46] && (g_ram[0x9ff]&127)==16 && g_ram[0x1fb4]+1==lives)break;}
    unsigned native_x=view_read(0x9dd),native_y=view_read(0x9e0),native_checkpoint=g_ram[0x1fb5];
    fprintf(stderr,"original boss retry frames%u checkpoint%u position%u,%u action%u lock%u\n",solo,native_checkpoint,native_x,native_y,g_ram[0x9da],g_ram[0xa46]);
    check(solo<3600 && native_checkpoint==checkpoint,"matched original X boss death reaches its native checkpoint");
    check(RtlLoadSnapshotFromMemory(original,cut),"paired boss retry restores the exact original fatal-contact prerequisite");
    bool wiped=false;unsigned elapsed=0;
    for(;elapsed<3600;++elapsed){frame(0);const X3CoopState *s=X3CoopRuntimeState();if(s->wipe)wiped=true;
      if(!(g_ram[0x1fb4]==lives || g_ram[0x1fb4]+1==lives))check(false,"boss wipe can debit only one shared life");
      if(wiped && !s->wipe && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4)break;
    }
    const X3CoopState *s=X3CoopRuntimeState();fprintf(stderr,"boss retry roster%u first%u frames%u wiped%u checkpoint%u/%u life%u/%u camera%u,%u position%u,%u locks%02x/%02x\n",reverse,first,elapsed,wiped,checkpoint,g_ram[0x1fb5],lives,g_ram[0x1fb4],view_read(0x1e5d),view_read(0x1e60),coop_x(x),support_position(x,8),g_ram[0x1f22],g_ram[0x1f4f]);
    check(elapsed<3600 && wiped && g_ram[0x1fae]==2 && g_ram[0x1fb5]==native_checkpoint && coop_x(x)==native_x && support_position(x,8)==native_y && g_ram[0x1fb4]+1==lives && !g_ram[0x1f22] && !g_ram[0x1f4f],"last-survivor boss death reaches the original checkpoint with released arena locks and one debit");
    check(s->players[x].life==X3_COOP_ALIVE && s->players[zero].life==X3_COOP_ALIVE && contact_hp(x)==16 && contact_hp(zero)==28,"native boss retry reconstructs the complete original roster");
    n=RtlSaveSnapshotToMemory(expected,capacity);check(RtlLoadSnapshotFromMemory(original,cut),"last-survivor boss approach restores");
    for(unsigned i=0;i<=elapsed;++i)frame(0);size_t replay=RtlSaveSnapshotToMemory(work,capacity);same(expected,n,work,replay,"last-survivor boss fatal contact and checkpoint retry replay the complete machine");
    unsigned before=coop_x(x);for(unsigned i=0;i<24;++i)frame((uint32_t)SNES_PAD_RIGHT<<(x*12));
    check(coop_x(x)>before && !X3CoopRuntimeState()->wipe,"retried boss checkpoint restores real survivor-seat control");
    /* The caller supplies its original approach for the second roster. */
    check(RtlLoadSnapshot(getenv("X3_COOP_BOSS_RETRY_STATE")),"boss retry restores its immutable original arena fixture");
    size=RtlSaveSnapshotToMemory(original,capacity);
  }
}

/* Actual stage5 checkpoint events3C:E386/E474, special class0B/params1,2.
 * Teleport and API retirement prepare the approach; real input must cross the
 * native trigger at3D:FF1D. No checkpoint, success or reenrollment flag is set. */
static void native_checkpoint_persistence_checks(const char *fixture,uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned point=1;point<=2;++point)for(unsigned reversed=0;reversed<2;++reversed)for(unsigned survivor=0;survivor<2;++survivor){
    check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==5 && !g_ram[0x1fb5],"native checkpoint starts from the unearned authored stage5 route");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    if(reversed){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
    unsigned fallen=survivor^1,px=point==1?2944:3200,py=point==1?1152:3008;
    check(X3CoopFall(&setup,fallen),"checkpoint approach supplies only the partner's fallen lifecycle prerequisite");
    setup.current=setup.anchor=(uint8_t)survivor;team_hazard_body(setup.players[survivor].body,px-36,py-40);
    setup.players[survivor].body[0x88]=255;
    check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"checkpoint route installs the actual surviving roster with explicit damage protection");
    door_install_player(&setup);X3CoopViewsConfigure(true);
    unsigned cx=px-128,cy=py&~255u;X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=5;views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=(uint16_t)cx;views.camera[seat][1]=(uint16_t)cy;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=(uint16_t)cy;views.limits[seat][3]=views.current[seat][3]=(uint16_t)cy;}X3CoopViewsSetState(&views);
    view_word(0x1e5d,cx);view_word(0x1e7a,cx);view_word(0x1e60,cy);view_word(0x1e7c,cy);
    view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6e,0);view_word(0x1e70,8192);view_word(0x1e6a,cy);view_word(0x1e6c,cy);view_word(0x1e78,cy);view_word(0x1e7e,cy);
    for(unsigned i=0;i<3;++i)frame(0);
    CpuState caller=g_cpu;view_native_call(0x00de3a,0,false);
    g_cpu.A=caller.A;g_cpu.X=caller.X;g_cpu.Y=caller.Y;g_cpu.D=caller.D;g_cpu.PB=caller.PB;g_cpu.DB=caller.DB;g_cpu.P=caller.P;cpu_p_to_mirrors(&g_cpu);
    unsigned slot=0;for(unsigned d=0x1d18;d<0x1e18;d+=16)if(g_ram[d] && g_ram[d+10]==11 && g_ram[d+11]==point && view_read(d+5)==px && view_read(d+8)==py)slot=d;
    fprintf(stderr,"checkpoint approach point%u roster%u survivor%u slot%04x player%u,%u old%u\n",point,reversed,survivor,slot,support_position(survivor,5),support_position(survivor,8),g_ram[0x1fb5]);
    check(slot && !g_ram[0x1fb5] && support_position(survivor,5)<px,"retail allocator admits the real unearned checkpoint before the survivor crosses it");
    X3CoopPlayer retired=X3CoopRuntimeState()->players[fallen];unsigned lives=g_ram[0x1fb4],hp=contact_hp(survivor),crossed=0;
    n=RtlSaveSnapshotToMemory(base,capacity);
    for(unsigned run=0;run<2;++run){
      check(RtlLoadSnapshotFromMemory(base,n),"native checkpoint crossing restores its exact pending world");unsigned f=0;
      for(;f<120 && g_ram[0x1fb5]!=point;++f)frame((SNES_PAD_RIGHT|(f<4?SNES_PAD_A:0)|(f%32<4?SNES_PAD_B:0))<<(survivor*12));
      fprintf(stderr,"checkpoint crossing point%u roster%u survivor%u run%u frames%u new%u player%u,%u life%u/%u\n",point,reversed,survivor,run,f,g_ram[0x1fb5],support_position(survivor,5),support_position(survivor,8),X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life);
      check(f<120 && g_ram[0x1fb5]==point && support_position(survivor,5)>px,"real native survivor input crosses and commits the authored checkpoint");
      for(unsigned settle=0;settle<16;++settle)frame(0);
      const X3CoopState *s=X3CoopRuntimeState();
      check(s->players[fallen].life==X3_COOP_FALLEN && !memcmp(&retired,&s->players[fallen],sizeof(retired)) && s->players[survivor].life==X3_COOP_ALIVE && !s->wipe && g_ram[0x1fb4]==lives && contact_hp(survivor)==hp,"actual checkpoint preserves the entire fallen actor and the healthy survivor without retry, healing or revival");
      X3CoopState rejoin=*s;check(!X3CoopRejoin(&rejoin,fallen),"the newly earned native checkpoint cannot authorize a fallen rejoin");
      size_t finish=RtlSaveSnapshotToMemory(run?work:after,capacity);
      if(!run){crossed=f;check(finish==n,"native checkpoint retains the complete current-build snapshot shape");}
      else{check(f==crossed,"native checkpoint repeats on the same actual crossing frame");same(after,n,work,finish,"native checkpoint and fallen persistence replay the complete machine exactly");}
    }
  }
}
