/* Availability is compared with the original pause-menu predicate. Stage,
 * encounter-lock and lifecycle preparation below bypasses prior travel. */
static bool stock_menu_probe;
static void stock_menu_boundary(CpuState *cpu,uint32_t pc){
  (void)cpu;(void)pc;
  if(!stock_menu_probe)return;
  stock_menu_probe=false;
  /* The original choice is complete. Omit its graphics upload/wait only. */
  interp_bridge_pre_opcode_redirect(0x00a36f);
}
static void stock_install(uint8_t *original,size_t size,uint8_t *work,unsigned reversed,unsigned mode){
  check(RtlLoadSnapshotFromMemory(original,size),"stock setup starts from the original native terrain");
  X3CoopState setup=*X3CoopRuntimeState();
  if(reversed){X3CoopPlayer p=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=p;}
  unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;
  setup.players[x].body[0x27]=11;setup.players[zero].body[0x27]=19;
  setup.anchor=setup.current=(uint8_t)x;
  if(mode==0)check(X3CoopWithdraw(&setup,zero),"stock initial Zero is living but withdrawn");
  if(mode==2)check(X3CoopWithdraw(&setup,x),"forced-return X withdrawal uses the lifecycle API");
  if(mode==3)check(X3CoopFall(&setup,x),"forced-return fallen X uses the lifecycle API");
  setup.current=setup.anchor;
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()),"stock roster encodes");
  work[door_scene_chunk(work,size)+16+6]=1;
  check(RtlLoadSnapshotFromMemory(work,size),"stock participation profile restores with the complete machine");
  door_install_player(&setup);g_ram[0x1fe1]&=254;g_ram[0x1fb2]&=191;g_ram[0x1f5d]&=127;
}
static void stock_next_stage(void){
  stage_request();bool pending=false;unsigned elapsed=0;
  do{frame(0);++elapsed;pending|=!X3CoopRuntimeReady();}
  while((!pending || !X3CoopRuntimeReady()) && elapsed<1500);
  check(elapsed<1500 && pending,"stock roster passes the real native stage constructor and arrival");
}
static void stock_door_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity,bool stock){
  uint8_t *admitted=malloc(capacity);check(admitted!=NULL,"stock gate replay buffer allocates");
  for(unsigned scenario=stock?0:1;scenario<(stock?2u:3u);++scenario){
    unsigned slot=door_approach(original,size,work,capacity,scenario==1?X3_COOP_X:X3_COOP_ZERO);
    X3CoopState setup=*X3CoopRuntimeState();
    unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;
    if(scenario){
      check(X3CoopRuntimeWithdraw(scenario==1?zero:x),"single gate prerequisite withdraws the other actor through the balanced runtime API");
      setup=*X3CoopRuntimeState();
    }
    size_t prepared=RtlSaveSnapshotToMemory(work,capacity),scene_at=door_scene_chunk(work,prepared);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,prepared),X3CoopEncodedSize()),"stock authored gate roster encodes");
    work[scene_at+10]=(uint8_t)(scenario!=0); /* Both-present rule with one participant. */
    work[scene_at+16+6]=(uint8_t)stock;
    check(RtlLoadSnapshotFromMemory(work,prepared),"stock gate restores without admission results");
    door_install_player(&setup);g_ram[0x1fe1]|=1;
    pickup_native_items(0x00daa0);
    check(g_ram[slot+1]==4 && g_ram[0x1f28],"authored stock gate admits Zero or X with unavailable Zero under Both policy");
    size_t pending=RtlSaveSnapshotToMemory(admitted,capacity);frame(0);
    size_t after=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(admitted,pending),"admitted stock gate restores");frame(0);
    size_t replay=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,after,work,replay,"stock gate commitment and participant handoff replay identically");
    unsigned elapsed=0;do{frame(0);++elapsed;}while(X3CoopRuntimeSceneActive() && elapsed<900);
    unsigned owner=scenario==2?zero:x,retired=owner^1;
    fprintf(stderr,"gate stock%u scenario%u elapsed%u owner%u life%u/%u xy%u,%u camera%u,%u locks=%u/%u/%u phase%u/%u\n",
        stock,scenario,elapsed,X3CoopRuntimeState()->anchor,X3CoopRuntimeState()->players[owner].life,
        X3CoopRuntimeState()->players[retired].life,coop_x(owner),support_position(owner,8),view_read(0x1e5d),view_read(0x1e60),
        g_ram[0x1f22],g_ram[0x1f24],g_ram[0x1f28],g_ram[0xd1],g_ram[0xd2]);
    check(elapsed<900 && !X3CoopRuntimeState()->wipe && X3CoopRuntimeState()->anchor==owner &&
          X3CoopRuntimeState()->players[owner].life==X3_COOP_ALIVE &&
          X3CoopRuntimeState()->players[retired].life==X3_COOP_ABSENT &&
          g_ram[X3_COOP_BODY+0xb6]==(scenario==2?2:0) &&
          g_ram[0x1fd2]==X3CoopRuntimeState()->players[owner].maximum_health &&
          !g_ram[0x1f22] && !g_ram[0x1f24] && !g_ram[0x1f28] && !g_ram[0x1f37] &&
          !(g_ram[0x1f5d]&32) && coop_x(owner)>2552,
          "original single-participant gate walk and miniboss introduction retain the native character without an exchange wait");
  }
  free(admitted);
}
static void native_stock_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  check(interp_bridge_add_pre_opcode_hook(0x00cbfd,stock_menu_boundary),"native stock menu-result probe registers");
  uint8_t *prepared=malloc(capacity);check(prepared!=NULL,"stock replay buffer allocates");
  for(unsigned reversed=0;reversed<2;++reversed){
    stock_install(original,size,work,reversed,0);
    unsigned x=X3CoopRuntimeState()->players[0].character==X3_COOP_X?0:1,zero=x^1;
    size_t n=RtlSaveSnapshotToMemory(prepared,capacity);
    for(unsigned stage=0;stage<=14;++stage)for(unsigned rule=0;rule<6;++rule){
      check(RtlLoadSnapshotFromMemory(prepared,n),"native availability comparison restores");
      g_ram[0x1fae]=(uint8_t)stage;
      if(rule==1)g_ram[X3_COOP_BODY+2]=0x2c;
      if(rule==2)g_ram[X3_COOP_BODY+2]=0x76;
      if(rule==3)g_ram[0x1f5d]|=128;
      if(rule==4)g_ram[0x1fe1]|=1;
      if(rule==5)g_ram[0x1fb2]|=64;
      bool available=X3CoopRuntimeZeroAvailable();
      stock_menu_probe=true;view_native_call(0x00cbb2,0,false);
      check(!stock_menu_probe && (g_ram[0x1f11]==0)==available,
            "stock availability agrees with the original executed native menu predicate");
    }
    check(RtlLoadSnapshotFromMemory(prepared,n),"stock public call restores native stage ground");
    for(unsigned i=0;i<110;++i)frame(SNES_PAD_SELECT<<(zero*12));
    check(X3CoopRuntimeStockZero() && X3CoopRuntimeState()->players[zero].life==X3_COOP_ALIVE &&
          (g_ram[0x1fe1]&1) && X3CoopRuntimeState()->players[zero].body[0x27]==19,
          "real Select control calls stock Zero once without replacing his health");
    frame(0);check(X3CoopRuntimeWithdraw(zero),"stock living Zero can return to X");
    check(!X3CoopRuntimeRejoin(zero),"a used stock call cannot be repeated in the same stage");
    for(unsigned i=0;i<110;++i)frame(SNES_PAD_SELECT<<(zero*12));
    check(X3CoopRuntimeState()->players[zero].life==X3_COOP_ABSENT,
          "held stock return cannot bypass the native one-call flag");
    stock_next_stage();
    check(X3CoopRuntimeState()->players[zero].life==X3_COOP_ABSENT && !(g_ram[0x1fe1]&1) &&
          X3CoopRuntimeZeroAvailable() && X3CoopRuntimeRejoin(zero),
          "a new native stage resets its stock call while leaving Zero withdrawn until requested");

    for(unsigned mode=1;mode<=3;++mode){
      stock_install(original,size,work,reversed,mode);g_ram[0x1fe1]|=1;
      g_ram[0x1f5d]|=128; /* Explicit encounter prerequisite, not door coverage. */
      size_t before=RtlSaveSnapshotToMemory(prepared,capacity);unsigned lives=g_ram[0x1fb4];
      frame(0);
      const X3CoopState *s=X3CoopRuntimeState();
      check(s->players[zero].life==X3_COOP_ABSENT && !(g_ram[0x1fb2]&64),
            "a stock encounter withdraws Zero without falsely marking permanent death");
      if(mode<3)check(!s->wipe && s->anchor==x && s->players[x].life==X3_COOP_ALIVE &&
          (s->players[x].body[0x27]&127)==11,
          "stock forced return preserves or safely recalls the living X with personal health");
      else check(s->wipe && s->players[x].life==X3_COOP_FALLEN,
          "stock restriction with a fallen X initiates a true wipe without reviving him");
      size_t after=RtlSaveSnapshotToMemory(expected,capacity);
      check(RtlLoadSnapshotFromMemory(prepared,before),"stock encounter boundary snapshot restores");
      frame(0);size_t replay=RtlSaveSnapshotToMemory(work,capacity);
      same(expected,after,work,replay,"stock encounter return/wipe replays the complete machine identically");
      if(mode<3){unsigned position=coop_x(x);
        for(unsigned i=0;i<24;++i)frame(SNES_PAD_RIGHT<<(x*12));
        check(coop_x(x)>position && !X3CoopRuntimeState()->wipe,
              "X resumes actual native movement after stock forced return");
      }
      if(mode==3){
        unsigned elapsed=0;
        do{frame(0);++elapsed;}while((X3CoopRuntimeState()->wipe || !X3CoopRuntimeReady()) && elapsed<1500);
        s=X3CoopRuntimeState();
        check(elapsed<1500 && s->players[x].life==X3_COOP_ALIVE && s->players[zero].life==X3_COOP_ABSENT &&
              g_ram[0x1fb4]+1==lives,"restricted last survivor reaches one original native team retry and leaves stock Zero withdrawn");
      }
    }
    stock_install(original,size,work,reversed,1);
    X3CoopState setup=*X3CoopRuntimeState();
    setup.players[zero].body[0x27]=0;setup.players[zero].body[2]=12;
    size_t n2=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n2),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,n2),"stock fatal-result lifecycle prerequisite restores");
    door_install_player(&setup);frame(0);
    check(X3CoopRuntimeState()->players[zero].life==X3_COOP_FALLEN && (g_ram[0x1fb2]&64) &&
          !X3CoopRuntimeRejoin(zero),"stock Zero death commits the native permanent-loss flag without reviving him");
    /* This supplies a fatal native result; authored contact deaths are a
     * separate route gate, rather than an inference from this lifecycle test. */

    stock_install(original,size,work,reversed,2);g_ram[0x1fe1]|=1;
    for(unsigned i=0;i<20;++i)frame(SNES_PAD_B<<(zero*12));
    g_ram[0x1f5d]|=128;frame(0);unsigned in_air=coop_x(zero);
    for(unsigned i=0;i<4;++i)frame((SNES_PAD_Y|SNES_PAD_RIGHT)<<(zero*12));
    size_t pending=RtlSaveSnapshotToMemory(prepared,capacity),chunk=door_scene_chunk(prepared,pending)+16;
    check(prepared[chunk+7]==1 && X3CoopRuntimeState()->players[x].life==X3_COOP_ABSENT &&
          X3CoopRuntimeState()->players[zero].life==X3_COOP_ALIVE,
          "airborne stock return waits for real safe ground rather than placing X in the air");
    check(!X3CoopRuntimeState()->players[zero].body[0x57],"Zero cannot charge while a restricted return waits for landing");
    check(coop_x(zero)>in_air,"pending stock return retains native movement toward safe ground");
    check(RtlLoadSnapshotFromMemory(prepared,pending),"pending stock return restores");
    size_t restored=RtlSaveSnapshotToMemory(work,capacity);
    same(prepared,pending,work,restored,"pending stock return immediate restore preserves the complete machine");
    for(unsigned field=6;field<8;++field){
      memcpy(work,prepared,pending);work[chunk+field]=(uint8_t)(field==6?0:2);
      check(!RtlLoadSnapshotFromMemory(work,pending),"invalid stock policy/return combination rejects transactionally");
      restored=RtlSaveSnapshotToMemory(work,capacity);
      same(prepared,pending,work,restored,"refused stock return leaves the complete machine unchanged");
    }
    X3CoopState fatal=*X3CoopRuntimeState();fatal.players[zero].body[0x27]=0;fatal.players[zero].body[2]=12;
    memcpy(work,prepared,pending);
    check(X3CoopEncode(&fatal,work+contact_context_offset(work,pending),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,pending),"pending stock return accepts a fatal-result lifecycle prerequisite");
    door_install_player(&fatal);frame(0);
    size_t fallen=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopRuntimeState()->wipe && !work[door_scene_chunk(work,fallen)+16+7] &&
          RtlLoadSnapshotFromMemory(work,fallen),"death during a pending stock return clears the request and retains a loadable true wipe");
    check(RtlLoadSnapshotFromMemory(prepared,pending),"pending return restores before its native landing replay");
    for(unsigned i=0;i<120;++i)frame(0);
    check(X3CoopRuntimeState()->players[x].life==X3_COOP_ALIVE &&
          X3CoopRuntimeState()->players[zero].life==X3_COOP_ABSENT,
          "pending stock return completes after native landing without a menu or door wait");
  }
  for(unsigned roster=0;roster<2;++roster){
    const char *path=getenv(roster?"X3_COOP_STOCK_DEATH_REVERSED_STATE":"X3_COOP_STOCK_DEATH_STATE");
    if(!path || !*path)continue;
    check(RtlLoadSnapshot(path),"stock fatal-contact route uses a real native roster fixture");
    size_t base=RtlSaveSnapshotToMemory(prepared,capacity);
    stock_door_checks(prepared,base,work,expected,capacity,true);
    stock_door_checks(prepared,base,work,expected,capacity,false);
    stock_install(prepared,base,work,0,1);
    X3CoopState setup=*X3CoopRuntimeState();unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;
    setup.players[x].body[0x27]=16;setup.players[zero].body[0x27]=1;
    size_t low=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,low),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,low),"native stock fatal-contact route prepares only personal low health");
    door_install_player(&setup);g_ram[0x1fe1]|=1;
    for(unsigned i=0;i<480;++i){unsigned pad=SNES_PAD_RIGHT|(SNES_PAD_RIGHT<<12);
      if(i>=180 && i<240)pad|=SNES_PAD_B|(SNES_PAD_B<<12);frame(pad);}
    for(unsigned i=0;i<10;++i)frame(0);
    check(X3CoopRuntimeState()->players[zero].life==X3_COOP_FALLEN &&
          X3CoopRuntimeState()->players[x].life==X3_COOP_ALIVE && (g_ram[0x1fb2]&64),
          "actual native enemy contact permanently retires stock Zero while X survives");
    stock_next_stage();
    check(X3CoopRuntimeState()->players[zero].life==X3_COOP_FALLEN && (g_ram[0x1fb2]&64) &&
          !X3CoopRuntimeZeroAvailable() && !X3CoopRuntimeRejoin(zero),
          "native next-stage entry retains contact-earned stock Zero loss without revival");
  }
  free(prepared);check(RtlLoadSnapshotFromMemory(original,size),"stock probes restore the original full-campaign machine");
  check(!X3CoopRuntimeStockZero(),"stock policy never leaks into the default full-campaign state");
}
