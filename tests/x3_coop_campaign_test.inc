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
  bool remote=getenv("X3_COOP_CHECKPOINT_PARTNER")!=NULL;
  for(unsigned point=1;point<=2;++point)for(unsigned reversed=0;reversed<2;++reversed)for(unsigned survivor=0;survivor<2;++survivor){
    check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==5 && !g_ram[0x1fb5],"native checkpoint starts from the unearned authored stage5 route");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    if(reversed){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
    unsigned fallen=survivor^1,px=point==1?2944:3200,py=point==1?1152:3008;
    if(!remote)check(X3CoopFall(&setup,fallen),"checkpoint approach supplies only the partner's fallen lifecycle prerequisite");
    setup.current=setup.anchor=(uint8_t)(remote?fallen:survivor);if(remote)setup.players[fallen].body[0x88]=255;team_hazard_body(setup.players[survivor].body,px-36,py-40);
    setup.players[survivor].body[0x88]=255;
    check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"checkpoint route installs the actual surviving roster with explicit damage protection");
    door_install_player(&setup);X3CoopViewsConfigure(true);
    unsigned cx=px-128,cy=py&~255u;X3CoopViewState views={0};if(remote)views=*X3CoopViewsState();views.independent=views.initialized=1;views.stage=5;views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){if(remote && seat==fallen)continue;views.camera[seat][0]=(uint16_t)cx;views.camera[seat][1]=(uint16_t)cy;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=(uint16_t)cy;views.limits[seat][3]=views.current[seat][3]=(uint16_t)cy;}X3CoopViewsSetState(&views);
    if(!remote){view_word(0x1e5d,cx);view_word(0x1e7a,cx);view_word(0x1e60,cy);view_word(0x1e7c,cy);
    view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6e,0);view_word(0x1e70,8192);view_word(0x1e6a,cy);view_word(0x1e6c,cy);view_word(0x1e78,cy);view_word(0x1e7e,cy);}
    for(unsigned i=0;i<3;++i)frame(0);
    CpuState caller=g_cpu;view_native_call(0x00de3a,0,false);
    g_cpu.A=caller.A;g_cpu.X=caller.X;g_cpu.Y=caller.Y;g_cpu.D=caller.D;g_cpu.PB=caller.PB;g_cpu.DB=caller.DB;g_cpu.P=caller.P;cpu_p_to_mirrors(&g_cpu);
    unsigned slot=0;for(unsigned d=0x1d18;d<0x1e18;d+=16)if(g_ram[d] && g_ram[d+10]==11 && g_ram[d+11]==point && view_read(d+5)==px && view_read(d+8)==py)slot=d;
    fprintf(stderr,"checkpoint approach point%u roster%u survivor%u slot%04x player%u,%u old%u\n",point,reversed,survivor,slot,support_position(survivor,5),support_position(survivor,8),g_ram[0x1fb5]);
    check(slot && !g_ram[0x1fb5] && support_position(survivor,5)<px,"retail allocator admits the real unearned checkpoint before the survivor crosses it");
    X3CoopPlayer retired=X3CoopRuntimeState()->players[fallen];unsigned lives=g_ram[0x1fb4],hp=contact_hp(survivor),crossed=0;
    n=RtlSaveSnapshotToMemory(base,capacity);
    if(remote){
      /* A body beyond the line in a distant view, and an eligible body exactly
       * on the line, must not earn this forward checkpoint. These are handler
       * controls; the actual positive crossing below uses ordinary frames. */
      for(unsigned boundary=0;boundary<2;++boundary){
        check(RtlLoadSnapshotFromMemory(base,n),"checkpoint negative control restores its pending native task");
        X3CoopState control=*X3CoopRuntimeState();
        unsigned seat=boundary?survivor:fallen;
        control.players[seat].body[5]=(uint8_t)(boundary?px:px+128);
        control.players[seat].body[6]=(uint8_t)((boundary?px:px+128)>>8);
        memcpy(work,base,n);
        check(X3CoopEncode(&control,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"checkpoint negative control changes only its actor prerequisite");
        door_install_player(&control);view_native_call(0x3dff1d,slot,true);
        check(!g_ram[0x1fb5],boundary?"the native forward checkpoint excludes equality at the authored line":"a beyond-line body in another native region cannot earn this checkpoint");
      }
    }
    for(unsigned run=0;run<2;++run){
      check(RtlLoadSnapshotFromMemory(base,n),"native checkpoint crossing restores its exact pending world");unsigned f=0;
      for(;f<120 && g_ram[0x1fb5]!=point;++f)frame((SNES_PAD_RIGHT|(f<4?SNES_PAD_A:0)|(f%32<4?SNES_PAD_B:0))<<(survivor*12));
      fprintf(stderr,"checkpoint crossing point%u roster%u survivor%u run%u frames%u new%u player%u,%u life%u/%u\n",point,reversed,survivor,run,f,g_ram[0x1fb5],support_position(survivor,5),support_position(survivor,8),X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life);
      check(f<120 && g_ram[0x1fb5]==point && support_position(survivor,5)>px,"real native survivor input crosses and commits the authored checkpoint");
      for(unsigned settle=0;settle<16;++settle)frame(0);
      const X3CoopState *s=X3CoopRuntimeState();
      check((remote?s->players[fallen].life==X3_COOP_ALIVE:(s->players[fallen].life==X3_COOP_FALLEN && !memcmp(&retired,&s->players[fallen],sizeof(retired)))) && s->players[survivor].life==X3_COOP_ALIVE && !s->wipe && g_ram[0x1fb4]==lives && contact_hp(survivor)==hp,"actual checkpoint preserves the entire fallen actor and the healthy survivor without retry, healing or revival");
      X3CoopState rejoin=*s;if(!remote)check(!X3CoopRejoin(&rejoin,fallen),"the newly earned native checkpoint cannot authorize a fallen rejoin");
      size_t finish=RtlSaveSnapshotToMemory(run?work:after,capacity);
      if(!run){crossed=f;check(finish==n,"native checkpoint retains the complete current-build snapshot shape");}
      else{check(f==crossed,"native checkpoint repeats on the same actual crossing frame");same(after,n,work,finish,"native checkpoint and fallen persistence replay the complete machine exactly");}
    }
  }
}

static const uint8_t *campaign_body(unsigned seat){const X3CoopState *s=X3CoopRuntimeState();return s->current==seat?g_ram+X3_COOP_BODY:s->players[seat].body;}
static void campaign_position_only(X3CoopPlayer *p,unsigned x,unsigned y){uint8_t *b=p->body;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[4]=b[7]=0;}
typedef struct BuffaloHeldSample {uint8_t body[X3_COOP_BODY_BYTES],enemy[64],aux[X3_COOP_AUX_BYTES],shots[X3_COOP_SHOT_BYTES],palette[64];} BuffaloHeldSample;
static void buffalo_held_sample(BuffaloHeldSample *out,unsigned seat,bool original){const X3CoopState *s=X3CoopRuntimeState();const X3CoopPlayer *p=&s->players[seat];bool live=original || s->current==seat;memcpy(out->body,live?g_ram+X3_COOP_BODY:p->body,sizeof(out->body));out->body[0x17]&=127;memcpy(out->enemy,g_ram+0xd18,64);memcpy(out->aux,live?g_ram+X3_COOP_AUX:p->auxiliaries,sizeof(out->aux));memcpy(out->shots,live?g_ram+X3_COOP_SHOTS:p->shots,sizeof(out->shots));memcpy(out->palette,live?g_ram+X3_COOP_PALETTE:p->palette,64);}
static void native_buffalo_destroy_captor(unsigned victim,uint8_t *base,uint8_t *work,size_t capacity){
 unsigned shooter=victim^1;size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState shot=*X3CoopRuntimeState();X3CoopCapture(&shot,g_ram);for(unsigned seat=0;seat<2;++seat)shot.players[seat].body[0x27]=shot.players[seat].maximum_health;X3CoopPlayer *shooter_player=&shot.players[shooter];campaign_position_only(shooter_player,view_read(0xd1d)+72,shooter_player->character==X3_COOP_ZERO?919:927);shooter_player->body[0x30]=255;shooter_player->body[0x33]=0;memcpy(work,base,n);check(X3CoopEncode(&shot,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"free shooter positions and boss1HP assist do not supply a hit, defeat, captive release or clear");door_install_player(&shot);g_ram[0xd3f]=1;g_ram[0xd4f]=0;n=RtlSaveSnapshotToMemory(base,capacity);unsigned killed=0;uint8_t *future=malloc(capacity);check(future!=NULL,"actual captor-destruction future allocates");
 for(unsigned run=0;run<2;++run){check(RtlLoadSnapshotFromMemory(base,n),"actual occupied captor restores before the free partner fires");for(unsigned f=0;f<8;++f)frame((uint32_t)SNES_PAD_LEFT<<(shooter*12));check(!(campaign_body(shooter)[0x11]&64),"the free native shooter earns real facing toward the occupied boss");bool emitted=false,dead=false;unsigned fired=0;for(;fired<180;++fired){frame((uint32_t)(SNES_PAD_LEFT|(fired%8==0?SNES_PAD_Y:0))<<(shooter*12));const X3CoopState *s=X3CoopRuntimeState();const uint8_t *shots=s->current==shooter?g_ram+X3_COOP_SHOTS:s->players[shooter].shots;for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)emitted|=shots[j]!=0;if(g_ram[0xd19]==4 && !(g_ram[0xd3f]&127)){dead=true;break;}}
  fprintf(stderr,"native BUFFALO CAPTOR KILL run%u victim%u char%u shooter%u char%u fired%u emitted%u dead%u boss%u/%u/%u HP%u grip%02x\n",run,victim,shot.players[victim].character,shooter,shot.players[shooter].character,fired,emitted,dead,g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd3f],g_ram[0xd53]);check(emitted && dead,"the free partner's actual native projectile earns the occupied captor's lethal hit and original defeat state");for(unsigned f=0;f<20;++f)frame(0);size_t done=RtlSaveSnapshotToMemory(work,capacity);check(done==n,"captor destruction retains the complete current-build snapshot shape");unsigned attachment=work[done-X3_COOP_MENU_STATE_BYTES-31]&12;const uint8_t *held=campaign_body(victim);fprintf(stderr,"native BUFFALO CAPTOR RELEASE run%u victim%u attachment%u body%u/%u reaction%u script%02x flagsA5%02x life%u/%u reward%02x\n",run,victim,attachment,held[2],held[3],held[0x30],held[0xa3],held[0xa5],X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life,g_ram[0x1fc8]);if(attachment || held[0x30])check(RtlSaveSnapshot("native-buffalo-destroyed-captor.sav"),"actual destroyed-captor attachment state saves without a file picker");check(!held[0x30] && held[0xa3]!=0x96 && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual native captor defeat releases the living victim while its original one-owner defeat script continues");if(getenv("X3_COOP_BUFFALO_KILL_RETURN")){unsigned elapsed=0,lives=g_ram[0x1fb4];for(;elapsed<6000;++elapsed){frame(elapsed%120<6?SNES_PAD_B|((uint32_t)SNES_PAD_B<<12):0);if(g_ram[0x1fae]!=2 && g_ram[0xd1]==4 && g_ram[0xd2]==4 && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && !g_ram[0x1f22] && !g_ram[0x1f24])break;}done=RtlSaveSnapshotToMemory(work,capacity);unsigned bound=0;for(unsigned i=0;i<31;++i)bound|=work[done-X3_COOP_MENU_STATE_BYTES-31+i]&12;fprintf(stderr,"native BUFFALO CAPTOR RETURN run%u victim%u frames%u stage%u phase%u/%u reward%02x life%u/%u lives%u/%u bound%u\n",run,victim,elapsed,g_ram[0x1fae],g_ram[0xd1],g_ram[0xd2],g_ram[0x1fc8],X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life,lives,g_ram[0x1fb4],bound);if(elapsed==6000)check(RtlSaveSnapshot("native-buffalo-captor-return-stalled.sav"),"actual retained native defeat continuation preserves its stalled machine");check(elapsed<6000 && g_ram[0x1fc8]==0xdc && !bound && g_ram[0x1fb4]==lives && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"one actual native reward and next-stage construction clear the finished owner and preserve both living actors without a life debit");}
  if(!run){killed=fired;memcpy(future,work,done);}else{check(fired==killed,"actual captor destruction repeats on the same native hit frame");same(future,done,work,done,"actual projectile kill, released captive and entire two-actor future replay identically");}}
 free(future);
}

static void native_buffalo_nonlethal_release(unsigned victim,uint8_t *base,uint8_t *work,size_t capacity){
  unsigned shooter=victim^1;check(X3CoopRuntimeState()->players[shooter].character==X3_COOP_X,"the actual weakness cast belongs to native X, never a Zero special weapon");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState shot=*X3CoopRuntimeState();X3CoopCapture(&shot,g_ram);
  for(unsigned seat=0;seat<2;++seat)shot.players[seat].body[0x27]=shot.players[seat].maximum_health;
  campaign_position_only(&shot.players[shooter],view_read(0xd1d)+72,927);shot.players[shooter].body[0x30]=255;shot.players[shooter].body[0x33]=0;
  check(X3CoopEncode(&shot,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"nonlethal rescue supplies only health, protection and free X approach position");door_install_player(&shot);g_ram[0x1fbe]=0x5c;g_ram[0xd4f]=0;n=RtlSaveSnapshotToMemory(base,capacity);
  uint8_t *future=malloc(capacity);check(future!=NULL,"actual nonlethal rescue replay buffer allocates");unsigned hit_frame=0,health=0;
  for(unsigned run=0;run<2;++run){check(RtlLoadSnapshotFromMemory(base,n),"actual nonlethal rescue restores the occupied native grip");for(unsigned f=0;f<8;++f)frame((uint32_t)SNES_PAD_LEFT<<(shooter*12));frame((uint32_t)(SNES_PAD_LEFT|SNES_PAD_R)<<(shooter*12));for(unsigned f=0;f<4;++f)frame((uint32_t)SNES_PAD_LEFT<<(shooter*12));
    check(campaign_body(shooter)[0x33]==4,"native weapon cycling selects the prerequisite-acquired Parasitic Bomb");unsigned fired=0;for(;fired<120;++fired){frame((uint32_t)(SNES_PAD_LEFT|(fired==0?SNES_PAD_Y:0))<<(shooter*12));if((g_ram[0xd3f]&127)<32)break;}
    unsigned hp=g_ram[0xd3f]&127;fprintf(stderr,"native BUFFALO NONLETHAL run%u victim%u fired%u HP%u state%u/%u/%u victimBody%u script%02x freeScript%02x\n",run,victim,fired,hp,g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],campaign_body(victim)[2],campaign_body(victim)[0xa3],campaign_body(shooter)[0xa3]);
    if(fired==120)check(RtlSaveSnapshot("native-buffalo-nonlethal-miss.sav"),"actual missed weakness cast preserves its native machine");
    check(fired<120 && hp && g_ram[0xd19]==2 && g_ram[0xd1a]==14 && campaign_body(victim)[0xa3]!=0x96 && (campaign_body(victim)[2]!=0x16 || campaign_body(victim)[0xa3]==0x88) && campaign_body(shooter)[0xa3]!=0x88,"the actual nonlethal native weakness hit releases its real victim and leaves the free shooter unscripted");
    for(unsigned f=0;f<16;++f)frame(0);check(X3CoopRuntimeWithdraw(victim),"the natively released weakness victim can withdraw after actual recovery");for(unsigned f=0;f<32;++f)frame(0);
    check(X3CoopRuntimeState()->players[victim].life==X3_COOP_ABSENT && X3CoopRuntimeState()->players[shooter].life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe && (g_ram[0xd3f]&127)==hp,"real released-victim withdrawal preserves the living shooter and native nonlethal boss recovery");
    size_t done=RtlSaveSnapshotToMemory(work,capacity);if(!run){memcpy(future,work,done);hit_frame=fired;health=hp;}else{check(fired==hit_frame && hp==health,"actual weakness rescue repeats its native hit frame and damage");same(future,done,work,done,"actual weakness release, withdrawal and whole native recovery future replay exactly");}
  }free(future);
}

static void native_buffalo_beam_checks(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==2 && g_ram[0xd22]==0x52 && g_ram[0xd3f]==32,"beam route restores the real ready Buffalo encounter");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
  if(getenv("X3_COOP_BUFFALO_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  setup.current=setup.anchor=0;
  for(unsigned seat=0;seat<2;++seat){campaign_position_only(&setup.players[seat],6070+seat*24,setup.players[seat].character==X3_COOP_ZERO?919:927);setup.players[seat].body[0x27]=setup.players[seat].maximum_health;setup.players[seat].body[0x88]=255;}
  check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"beam approach supplies only roster, position and health/protection prerequisites");door_install_player(&setup);X3CoopViewsConfigure(true);g_ram[0x1f27]=0;g_ram[0xd3f]=16;
  unsigned elapsed=0,slot=0,actions=0;
  for(;elapsed<12000 && !slot;++elapsed){
    if(!(elapsed%64)){n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState live=*X3CoopRuntimeState();X3CoopCapture(&live,g_ram);
      for(unsigned seat=0;seat<2;++seat)if(live.players[seat].life==X3_COOP_ALIVE){X3CoopPlayer *actor=&live.players[seat];actor->body[0x27]=actor->maximum_health;actor->body[0x88]=255;
        if(!(g_ram[0xd53]&128) && g_ram[0xd1a]!=12 && abs((int)(actor->body[5]|actor->body[6]<<8)-(int)view_read(0xd1d))<128)campaign_position_only(actor,view_read(0xd1d)<6144?6300-seat*24:5930+seat*24,actor->character==X3_COOP_ZERO?919:927);}
      check(X3CoopEncode(&live,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"beam wait refreshes only actual actor prerequisites");door_install_player(&live);
    }
    frame(0);if(g_ram[0xd1a]<16)actions|=1u<<g_ram[0xd1a];
    for(unsigned d=0x1318;d<0x1518;d+=64)if(g_ram[d] && g_ram[d+10]==0x20 && g_ram[d+0x36]==2){slot=d;break;}
    if(!(elapsed%500))fprintf(stderr,"native BUFFALO BEAM WAIT f%u state%u/%u/%u flags%02x HP%u actors%u/%u\n",elapsed,g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd53],g_ram[0xd3f],contact_hp(0),contact_hp(1));
    if(X3CoopRuntimeState()->players[0].life!=X3_COOP_ALIVE || X3CoopRuntimeState()->players[1].life!=X3_COOP_ALIVE)check(false,"native beam approach keeps its two actual actors alive");
  }
  fprintf(stderr,"native BUFFALO BEAM BORN f%u actions%04x slot%04x source%u/%u/%u boss%u/%u/%u flags%02x\n",elapsed,actions,slot,g_ram[slot],g_ram[slot+1],g_ram[slot+2],g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd53]);
  check(slot!=0,"actual half-health native Buffalo AI earns its beam without supplied attack, spawn or success flags");
  check(RtlSaveSnapshot("native-buffalo-earned-beam.sav"),"actual emitted beam exports directly through the test interface");
  unsigned victim=getenv("X3_COOP_BUFFALO_VICTIM")?(unsigned)atoi(getenv("X3_COOP_BUFFALO_VICTIM")):1;check(victim<2,"beam victim seat is valid");unsigned free_seat=victim^1;
  n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);setup.current=setup.anchor=(uint8_t)(getenv("X3_COOP_BUFFALO_OWNER_ANCHOR")?victim:free_seat);
  int sign=g_ram[slot+0x11]&64?1:-1;unsigned source_x=view_read(slot+5);
  for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *actor=&setup.players[seat];campaign_position_only(actor,(unsigned)((int)source_x+(seat==victim?sign*64:-sign*32)),actor->character==X3_COOP_ZERO?919:927);actor->body[0x27]=actor->maximum_health;actor->body[0x30]=actor->body[0x88]=actor->body[0x8a]=0;}
  check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"beam collision positions only its vulnerable victim ahead and free partner behind the actual muzzle");door_install_player(&setup);X3CoopViewState beam_views=*X3CoopViewsState();n=RtlSaveSnapshotToMemory(base,capacity);
  if(getenv("X3_COOP_BUFFALO_KILL_HELD")){
    for(unsigned f=0;f<60;++f)frame(0);check(campaign_body(victim)[0x6e] && g_ram[0x1cd90],"real native beam and child earn the frozen victim before the partner's lethal shot");
    native_buffalo_destroy_captor(victim,base,work,capacity);
    unsigned arriving=0;for(;arriving<180 && (campaign_body(0)[0x6e] || campaign_body(1)[0x6e] || campaign_body(0)[2]>0x16 || campaign_body(1)[2]>0x16);++arriving)frame(0);
    size_t finish=RtlSaveSnapshotToMemory(work,capacity),owners=combat_owners(work,finish);unsigned stale=0;
    for(unsigned i=0;i<40;++i)if(g_ram[0x1818+i*32+10]==0x1c && g_ram[0x1818+i*32+11]==4)stale|=work[owners+i];
    unsigned distance=(unsigned)abs((int)support_position(0,5)-(int)support_position(1,5));
    fprintf(stderr,"native BUFFALO FROZEN CLEAR finalstage%u arrive%u sharedScratch%u stale%u locks%u/%u xy%u,%u/%u,%u phase%u/%u\n",g_ram[0x1fae],arriving,g_ram[0x1cd90],stale,campaign_body(0)[0x6e],campaign_body(1)[0x6e],support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8),g_ram[0xd1],g_ram[0xd2]);
    if(arriving==180 || stale || distance>32)check(RtlSaveSnapshot("native-buffalo-frozen-clear-stale.sav"),"actual post-clear frozen state preserves its current-build machine");
    check(g_ram[0x1fae]!=2 && arriving<180 && !stale && !campaign_body(0)[0x6e] && !campaign_body(1)[0x6e] && distance<=32,"actual post-clear arrival releases both colocated native actors and retires every old freeze child owner");
    for(unsigned seat=0;seat<2;++seat){unsigned before=support_position(seat,5);for(unsigned f=0;f<24;++f)frame((uint32_t)SNES_PAD_RIGHT<<(seat*12));check(support_position(seat,5)>before+10,"each actual post-clear actor independently resumes native control");}return;
  }

  enum {BEAM_FRAMES=720};BuffaloHeldSample *trace=malloc(BEAM_FRAMES*sizeof(*trace));uint8_t *future=malloc(capacity);check(trace && future,"actual beam comparison buffers allocate");unsigned ran=0;
  for(unsigned paired=0;paired<2;++paired){check(RtlLoadSnapshotFromMemory(base,n),"original and paired beams restore the same actual native source");
    if(!paired){X3CoopState solo=*X3CoopRuntimeState();solo.current=solo.anchor=(uint8_t)victim;memcpy(work,base,n);check(X3CoopEncode(&solo,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"single-actor native beam reference selects its actual victim");door_install_player(&solo);X3CoopViewsConfigure(false);vehicle_install_view(&beam_views,victim);X3CoopRuntimeEnable(false);}
    bool frozen=false;unsigned clock=g_ram[0x9cc];
    for(unsigned f=0;f<(paired?ran:BEAM_FRAMES);++f){unsigned pad=f<90?0:(f&1?SNES_PAD_LEFT:SNES_PAD_RIGHT|SNES_PAD_B);frame(paired?(uint32_t)pad<<(victim*12):pad);BuffaloHeldSample now;buffalo_held_sample(&now,victim,!paired);memcpy(now.enemy,g_ram+slot,64);frozen|=now.body[0x6e]!=0;
      if(!(f%30))fprintf(stderr,"native BUFFALO BEAM CONTACT paired%u f%u victim%u body%u/%u frozen%u script%02x HP%u source%u/%u/%u flags%02x boss%u/%u/%u\n",paired,f,victim,now.body[2],now.body[3],now.body[0x6e],now.body[0xa3],now.body[0x27]&127,g_ram[slot],g_ram[slot+1],g_ram[slot+2],g_ram[slot+0x3f],g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b]);
      if(paired && f==60){size_t busy=RtlSaveSnapshotToMemory(future,capacity),owners=combat_owners(future,busy);unsigned captures=0,graphics=0;
        for(unsigned i=0;i<40;++i){unsigned fx=0x1818+i*32;if(!g_ram[fx] || g_ram[fx+10]!=0x1c)continue;
          if(g_ram[fx+11]==4){++captures;check(future[owners+i]==victim+1,"actual native freeze child belongs only to the actor caught");}
          if(g_ram[fx+11]==2){++graphics;check(!future[owners+i],"the actual beam graphic remains an unowned world child");}}
        check(captures==1 && graphics && campaign_body(victim)[0x6e] && !campaign_body(free_seat)[0x6e],"one actual freeze binds its real victim while the nearer free partner remains unbound");
        check(!X3CoopRuntimeWithdraw(victim) && !X3CoopRuntimeWithdraw(free_seat),"the actual native freeze waits for release before participation changes");same(future,busy,work,RtlSaveSnapshotToMemory(work,capacity),"captured participation refuses without changing the entire beam world");
      }
      if(!paired){trace[f]=now;ran=f+1;if(frozen && !now.body[0x6e] && now.body[2]!=0x1e)break;}
      else if(memcmp(&trace[f],&now,sizeof(now))){unsigned at=0;while(at<sizeof(now) && ((uint8_t*)&trace[f])[at]==((uint8_t*)&now)[at])++at;fprintf(stderr,"native BUFFALO BEAM MISMATCH f%u victim%u char%u byte%u native%02x paired%02x\n",f,victim,setup.players[victim].character,at,((uint8_t*)&trace[f])[at],((uint8_t*)&now)[at]);check(RtlSaveSnapshot("native-buffalo-beam-mismatch.sav"),"actual beam mismatch preserves its whole current-build machine");check(false,"actual beam collision, frozen body, own pools and source match the native victim on every frame");}
    }
    fprintf(stderr,"native BUFFALO BEAM FINISH paired%u frames%u source%u body%u lock%u clock%u->%u delta%u nativecapture%u\n",paired,ran,g_ram[slot],campaign_body(victim)[2],campaign_body(victim)[0x6e],clock,g_ram[0x9cc],(uint8_t)(g_ram[0x9cc]-clock),g_ram[0x1cd90]);
    check(frozen,"the real native beam actually freezes its vulnerable victim");check(ran<BEAM_FRAMES && !campaign_body(victim)[0x6e] && campaign_body(victim)[2]!=0x1e && (uint8_t)(g_ram[0x9cc]-clock)==(uint8_t)ran,"real escape inputs release the actual native freeze on its original frame with one world update");
    if(paired){unsigned cleanup=0;for(;cleanup<240 && g_ram[slot];++cleanup){unsigned before=g_ram[0x9cc];frame(0);check((uint8_t)(g_ram[0x9cc]-before)<=1,"native beam retirement never advances the shared world twice");}
      size_t done=RtlSaveSnapshotToMemory(future,capacity),owners=combat_owners(future,done);unsigned stale=0;
      for(unsigned i=0;i<40;++i)if(g_ram[0x1818+i*32+10]==0x1c && g_ram[0x1818+i*32+11]==4)stale|=future[owners+i];
      fprintf(stderr,"native BUFFALO BEAM RETIRED cleanup%u source%u capture%u stale%u locks%u/%u life%u/%u HP%u/%u\n",cleanup,g_ram[slot],g_ram[0x1cd90],stale,campaign_body(0)[0x6e],campaign_body(1)[0x6e],X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life,contact_hp(0),contact_hp(1));
      check(cleanup<240 && !g_ram[slot] && !g_ram[0x1cd90] && !stale && !campaign_body(0)[0x6e] && !campaign_body(1)[0x6e] && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual native beam retirement leaves both living actors and no captured body or stale child owner");
      check(RtlLoadSnapshotFromMemory(base,n),"actual beam restores for whole-machine replay");for(unsigned f=0;f<ran;++f){unsigned pad=f<90?0:(f&1?SNES_PAD_LEFT:SNES_PAD_RIGHT|SNES_PAD_B);frame((uint32_t)pad<<(victim*12));}for(unsigned f=0;f<cleanup;++f)frame(0);same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"native beam ownership, freeze, release and actual retirement replay the whole machine exactly");}

  }
  free(future);free(trace);
}

/* The actual Buffalo0A charge must earn its grip. Position/protection do not
 * provide an attack state, capture flag, scripted player state or clear. */
static void native_buffalo_capture_checks(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
 if(getenv("X3_COOP_BUFFALO_BEAM")){native_buffalo_beam_checks(fixture,base,work,capacity);return;}
 check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==2 && g_ram[0xd22]==0x52 && g_ram[0xd3f]==32,"Buffalo capture route restores the earned native32HP encounter");unsigned victim=getenv("X3_COOP_BUFFALO_VICTIM")?(unsigned)atoi(getenv("X3_COOP_BUFFALO_VICTIM")):1;check(victim<2,"Buffalo requested victim is valid");size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);if(getenv("X3_COOP_BUFFALO_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}unsigned free_seat=victim^1;setup.current=setup.anchor=(uint8_t)(getenv("X3_COOP_BUFFALO_OWNER_ANCHOR")?victim:free_seat);
 for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&setup.players[seat];campaign_position_only(p,seat==victim?6096:5912,p->character==X3_COOP_ZERO?919:927);p->body[0x88]=255;}
 memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"Buffalo route installs only requested roster, personal protection and approach positions");door_install_player(&setup);X3CoopViewsConfigure(true);g_ram[0x1f27]=0;
 unsigned elapsed=0,last=65535,actions=0;bool caught=false;
 for(;elapsed<3600;++elapsed){unsigned phase=(g_ram[0xd1a]<<8)|g_ram[0xd1b];if(phase!=last){fprintf(stderr,"native BUFFALO ACTION f%u state%u/%u/%u flags%02x XY%u,%u victim%u,%u HP%u/%u\n",elapsed,g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd53],view_read(0xd1d),view_read(0xd20),support_position(victim,5),support_position(victim,8),contact_hp(0),contact_hp(1));last=phase;}if(g_ram[0xd1a]<16)actions|=1u<<g_ram[0xd1a];
  bool approach=g_ram[0xd19]==2 && g_ram[0xd1a]==10 && g_ram[0xd1b]==2;if(approach || !(elapsed%128)){n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState live=*X3CoopRuntimeState();X3CoopCapture(&live,g_ram);for(unsigned seat=0;seat<2;++seat)if(live.players[seat].life==X3_COOP_ALIVE)live.players[seat].body[0x88]=255;if(approach){int sign=g_ram[0xd29]&64?1:-1;campaign_position_only(&live.players[victim],(unsigned)((int)view_read(0xd1d)+sign*24),view_read(0xd20)-6);campaign_position_only(&live.players[free_seat],view_read(0xd1d)<6144?6300:5900,live.players[free_seat].character==X3_COOP_ZERO?919:927);}check(X3CoopEncode(&live,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"native charge approach supplies only actual contact coordinates/protection");door_install_player(&live);}
  frame(0);const uint8_t *b=campaign_body(victim);if((g_ram[0xd53]&128) && (b[2]==0x74 || b[0xa3]==0x96)){caught=true;break;}check(contact_hp(0) && contact_hp(1),"protected capture approach preserves both actual living participants");
 }
 fprintf(stderr,"native BUFFALO EARNED GRIP victim%u char%u f%u caught%u actions%04x boss%u/%u/%u flags%02x body%u/%u script%02x HP%u/%u\n",victim,setup.players[victim].character,elapsed,caught,actions,g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd53],campaign_body(victim)[2],campaign_body(victim)[3],campaign_body(victim)[0xa3],contact_hp(0),contact_hp(1));check(caught,"the real native charge and collision earn Buffalo's scripted grip without supplied capture flags");check(RtlSaveSnapshot("native-buffalo-earned-grab.sav"),"actual earned Buffalo grip saves directly through the test interface");
 if(getenv("X3_COOP_BUFFALO_WEAKNESS")){native_buffalo_nonlethal_release(victim,base,work,capacity);return;}
 if(getenv("X3_COOP_BUFFALO_KILL_HELD")){native_buffalo_destroy_captor(victim,base,work,capacity);return;}
 n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState held=*X3CoopRuntimeState();X3CoopCapture(&held,g_ram);for(unsigned seat=0;seat<2;++seat)held.players[seat].body[0x27]=held.players[seat].maximum_health;campaign_position_only(&held.players[free_seat],view_read(0xd1d),held.players[free_seat].character==X3_COOP_ZERO?919:927);held.players[free_seat].body[0x88]=255;memcpy(work,base,n);check(X3CoopEncode(&held,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"free partner approaches the real occupied captor without altering either capture result");door_install_player(&held);check(X3CoopNearest(X3CoopRuntimeState(),g_ram,view_read(0xd1d),view_read(0xd20))==free_seat,"the free partner is now genuinely nearer than the earned native captive");X3CoopViewState held_views=*X3CoopViewsState();n=RtlSaveSnapshotToMemory(base,capacity);check(!X3CoopRuntimeWithdraw(victim) && !X3CoopRuntimeWithdraw(free_seat),"a genuinely occupied native grip waits for release before participation changes");same(base,n,work,RtlSaveSnapshotToMemory(work,capacity),"busy captured participation refuses without changing the full machine");unsigned free_x=support_position(free_seat,5);frame((uint32_t)SNES_PAD_RIGHT<<(free_seat*12));unsigned after_x=support_position(free_seat,5);fprintf(stderr,"native BUFFALO GRIP OWNERSHIP victim%u body%u script%02x at%u,%u free%u body%u script%02x x%u->%u boss%u,%u flags%02x\n",victim,campaign_body(victim)[2],campaign_body(victim)[0xa3],support_position(victim,5),support_position(victim,8),free_seat,campaign_body(free_seat)[2],campaign_body(free_seat)[0xa3],free_x,after_x,view_read(0xd1d),view_read(0xd20),g_ram[0xd53]);if((int16_t)(after_x-free_x)<-8 || (int16_t)(after_x-free_x)>8)check(RtlSaveSnapshot("native-buffalo-grab-mismatch.sav"),"actual captured-target mismatch preserves its current-build machine");check((int16_t)(after_x-free_x)>=-8 && (int16_t)(after_x-free_x)<=8 && campaign_body(free_seat)[2]!=0x74 && campaign_body(free_seat)[0xa3]!=0x96,"a nearer free actor keeps its own input while the earned Buffalo grip retains its real captive");

 enum {MAX_HELD=360};BuffaloHeldSample *trace=malloc(MAX_HELD*sizeof(*trace));uint8_t *future=malloc(capacity);check(trace && future,"actual native captured-body reference buffers allocate");unsigned ran=0;
 for(unsigned paired=0;paired<2;++paired){check(RtlLoadSnapshotFromMemory(base,n),"native and paired grips restore the same actual captive and nearer free partner");if(!paired){X3CoopState solo=*X3CoopRuntimeState();solo.current=solo.anchor=(uint8_t)victim;memcpy(work,base,n);check(X3CoopEncode(&solo,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"original grip reference selects its real captive");door_install_player(&solo);X3CoopViewsConfigure(false);vehicle_install_view(&held_views,victim);X3CoopRuntimeEnable(false);}unsigned clock=g_ram[0x9cc],released_at=0;for(unsigned f=0;f<(paired?ran:MAX_HELD);++f){uint32_t input=paired && f<8?(uint32_t)SNES_PAD_RIGHT<<(free_seat*12):0;if(getenv("X3_COOP_BUFFALO_SHAKE") && !(f&1))input|=(uint32_t)SNES_PAD_B<<(paired?victim*12:0);frame(input);BuffaloHeldSample now;buffalo_held_sample(&now,victim,!paired);if(!released_at && now.body[0xa3]==0x88)released_at=f+1;if(paired && getenv("X3_COOP_BUFFALO_SHAKE") && f==40){size_t ready=RtlSaveSnapshotToMemory(future,capacity);bool withdrew=X3CoopRuntimeWithdraw(victim);fprintf(stderr,"native BUFFALO RELEASED WITHDRAW result%u body%u script%02x boss%u/%u/%u flags%02x release%u\n",withdrew,now.body[2],now.body[0xa3],g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd53],released_at);if(!withdrew)check(RtlSaveSnapshot("native-buffalo-released-withdraw-refused.sav"),"actual released withdrawal refusal preserves its machine");check(withdrew,"a natively released captive can withdraw after its actual control resumes");check(RtlLoadSnapshotFromMemory(future,ready),"native released-captive withdrawal probe restores its exact ongoing world");}if(getenv("X3_COOP_BUFFALO_SHAKE") && (f<8 || f==29 || f==59))fprintf(stderr,"native BUFFALO SHAKE paired%u f%u body%u/%u script%02x edge%02x held%02x timer%u count%u raw%04x\n",paired,f,now.body[2],now.body[3],now.body[0xa3],now.body[0x3b],now.body[0x37],g_ram[0xd51],g_ram[0xd52],view_read(0xa8));if(!paired){trace[f]=now;ran=f+1;if(!(g_ram[0xd53]&128))break;}else if(memcmp(&trace[f],&now,sizeof(now))){unsigned diff=0;while(diff<sizeof(now) && ((uint8_t*)&trace[f])[diff]==((uint8_t*)&now)[diff])++diff;fprintf(stderr,"native BUFFALO HELD MISMATCH f%u victim%u char%u byte%u native%02x paired%02x boss%u/%u/%u flags%02x\n",f,victim,setup.players[victim].character,diff,((uint8_t*)&trace[f])[diff],((uint8_t*)&now)[diff],g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],g_ram[0xd53]);fprintf(stderr,"native HELD GEOMETRY actorX%u/%u enemyX%u/%u nativecameraNow%u canonical%u nativeLimitsCurrent%u/%u\n",trace[f].body[5]|trace[f].body[6]<<8,now.body[5]|now.body[6]<<8,trace[f].enemy[5]|trace[f].enemy[6]<<8,now.enemy[5]|now.enemy[6]<<8,held_views.camera[victim][0],view_read(0x1e5d),X3CoopViewsState()->current[victim][0],X3CoopViewsState()->current[victim][1]);check(RtlSaveSnapshot("native-buffalo-held-mismatch.sav"),"actual held comparison difference retains its machine");check(false,"actual held body, hurt/control fields, own pools/palette and complete native captor stay with the earned victim on every frame");}}
  fprintf(stderr,"native BUFFALO HELD QUALIFY paired%u victim%u char%u frames%u body%u/%u script%02x grip%02x HP%u/%u\n",paired,victim,setup.players[victim].character,ran,campaign_body(victim)[2],campaign_body(victim)[3],campaign_body(victim)[0xa3],g_ram[0xd53],contact_hp(0),contact_hp(1));check(ran<MAX_HELD && !(g_ram[0xd53]&128) && contact_hp(victim) && (uint8_t)(g_ram[0x9cc]-clock)==(uint8_t)ran,"real native wall/release cleanup completes on the original frame with a living captive and one world update");if(getenv("X3_COOP_BUFFALO_SHAKE")){fprintf(stderr,"native BUFFALO SHAKE RELEASE paired%u frame%u continuation%u\n",paired,released_at,ran);check(released_at && released_at<60,"actual captive Jump edges earn the native shake escape before the wall release");}
  if(paired){check(X3CoopRuntimeState()->players[free_seat].life==X3_COOP_ALIVE,"the nearer free actor remains independently alive throughout the native hold/release");size_t done=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(base,n),"actual occupied grip restores for complete replay");for(unsigned f=0;f<ran;++f){uint32_t input=f<8?(uint32_t)SNES_PAD_RIGHT<<(free_seat*12):0;if(getenv("X3_COOP_BUFFALO_SHAKE") && !(f&1))input|=(uint32_t)SNES_PAD_B<<(victim*12);frame(input);}same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"actual captured ownership, wall release and complete two-actor future replay identically");}
 }free(future);free(trace);
}
