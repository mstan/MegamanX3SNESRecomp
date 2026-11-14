/* Real stage1 heart and stage5 subtank. Coordinates/cameras prepare the
 * approach; original allocation, eligibility, award and timed task decide it. */
static unsigned persistent_approach(uint8_t *original,size_t size,uint8_t *work,size_t capacity,
                                    unsigned kind,unsigned px,unsigned py,bool reversed){
  check(RtlLoadSnapshotFromMemory(original,size),"persistent acquisition approach restores");
  X3CoopState setup=*X3CoopRuntimeState();
  if(reversed){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;
  setup.current=setup.anchor=(uint8_t)zero;
  setup.players[x].body[5]=(uint8_t)(px-96);setup.players[x].body[6]=(uint8_t)((px-96)>>8);
  setup.players[x].body[8]=(uint8_t)(py-40);setup.players[x].body[9]=(uint8_t)((py-40)>>8);
  setup.players[x].body[0x27]=8;memset(setup.players[x].body+0x1a,0,6);
  memcpy(setup.players[x].body+0x22,setup.players[x].body+5,2);
  memcpy(setup.players[x].body+0x24,setup.players[x].body+8,2);
  setup.players[x].body[4]=setup.players[x].body[7]=0;
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,size),"damaged remote native X and Zero-led world restore");
  door_install_player(&setup);X3CoopViewsConfigure(true);X3CoopRuntimeFrame();
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=g_ram[0x1fae];
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=(uint16_t)(s==x?px-128:view_read(0x1e5d));
    views.camera[s][1]=(uint16_t)(s==x?py&~255u:view_read(0x1e60));
    views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];}
  X3CoopViewsSetState(&views);
  /* Settle native pose/CHR away from the item after roster projection.
   * Native terrain may move the body; no acquisition result is supplied. */
  for(unsigned i=0;i<3;++i)frame(0);
  CpuState resume=g_cpu;view_native_call(0x00de3a,0,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  pickup_native_items(0x00daa0);pickup_native_items(0x00daa0);
  unsigned slot=0;
  for(unsigned d=0x1518;d<0x1818;d+=48)
    if(g_ram[d] && g_ram[d+10]==kind && view_read(d+5)==px && abs((int)view_read(d+8)-(int)py)<8)slot=d;
  if(!slot || g_ram[slot+1]!=2 || g_ram[slot+2]==6 || g_ram[0x1f28]){
    fprintf(stderr,"persistent setup kind%u stage%u slot=%04x owned=%02x/%02x flags=%u/%u cam=%u,%u actor=%u,%u\n",
        kind,g_ram[0x1fae],slot,g_ram[0x1fd1],g_ram[0x1fd4],g_ram[0x1f22],g_ram[0x1f24],
        X3CoopViewsState()->camera[x][0],X3CoopViewsState()->camera[x][1],support_position(x,5),support_position(x,8));
    for(unsigned d=0x1518;d<0x1818;d+=48)if(g_ram[d])fprintf(stderr,"item%04x kind%02x state%u/%u/%u xy=%u,%u bit=%02x\n",
        d,g_ram[d+10],g_ram[d+1],g_ram[d+2],g_ram[d+3],view_read(d+5),view_read(d+8),g_ram[d+11]);
  }
  check(slot && g_ram[slot+1]==2 && g_ram[slot+2]!=6 && !g_ram[0x1f28],
        "retail persistent-item allocation initializes without claiming its reward");
  const char *output=getenv("X3_COOP_ACQUISITION_OUTPUT");char path[1024];
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-%s-approach.sav",output,x+1,kind==11?"heart":"subtank");
    check(RtlSaveSnapshot(path),"unclaimed authored persistent item visual fixture saves");}
  setup=*X3CoopRuntimeState();unsigned y=view_read(slot+8)-13;
  setup.players[x].body[5]=(uint8_t)px;setup.players[x].body[6]=(uint8_t)(px>>8);
  setup.players[x].body[8]=(uint8_t)y;setup.players[x].body[9]=(uint8_t)(y>>8);
  memcpy(setup.players[x].body+0x22,setup.players[x].body+5,2);
  memcpy(setup.players[x].body+0x24,setup.players[x].body+8,2);
  memset(setup.players[x].body+0x1a,0,6);
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+contact_context_offset(work,prepared),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,prepared),"native X contact precondition restores without award flags");
  return slot;
}
static void native_persistent_pickup_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity,
                                          unsigned kind,unsigned px,unsigned py,unsigned variant,bool reversed){
  unsigned slot=persistent_approach(original,size,work,capacity,kind,px,py,reversed);
  X3CoopState setup=*X3CoopRuntimeState();unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;
  check(g_ram[slot+11]==variant,"native persistent item retains its authored acquisition bit");
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);unsigned flag=kind==11?0x1fd4:0x1fd1;
  unsigned initial_flag=g_ram[flag],initial_max=setup.players[x].maximum_health,initial_hp=setup.players[x].body[0x27]&127;
  uint8_t tanks[4];memcpy(tanks,g_ram+0x1fb7,4);
  X3CoopRuntimeEnable(false);setup.current=(uint8_t)x;door_install_player(&setup);
  view_word(0x1e5d,px-128);view_word(0x1e60,py&~255u);
  unsigned calls=0;
  do{pickup_native_items(0x00daa0);++calls;}while(!g_ram[0x1f28] && calls<8);
  if(!g_ram[0x1f28])fprintf(stderr,"persistent native ref kind%u item=%u/%u/%u xy=%u,%u actor=%u,%u state=%u block=%u\n",
      kind,g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],view_read(slot+5),view_read(slot+8),view_read(0x9dd),view_read(0x9e0),g_ram[0x9da],g_ram[0x1f1d]);
  check(g_ram[0x1f28] && (g_ram[flag]&variant),"original single-X update actually claims the persistent reward");
  uint8_t native_item[48];memcpy(native_item,g_ram+slot,48);
  check(RtlLoadSnapshotFromMemory(work,prepared),"co-op persistent claim starts from the matched native approach");
  for(unsigned i=0;i<calls;++i)pickup_native_items(0x00daa0);
  check(g_ram[0x1f28] && (g_ram[flag]&variant),"remote native X claims its persistent item while Zero leads the world");
  check(!memcmp(native_item,g_ram+slot,48),"co-op persistent claim matches exactly one original native award task");
  size_t pending=RtlSaveSnapshotToMemory(expected,capacity);size_t recipient=0;
  for(size_t i=0;i+8<pending;++i)if(!memcmp(expected+i,"X3R1",4)){recipient=i;break;}
  check(recipient && expected[recipient+6]==x && expected[recipient+7]==1,
        "timed persistent acquisition retains the actual native X recipient");
  uint8_t *start=malloc(pending);check(start!=NULL,"persistent pending replay buffer allocates");memcpy(start,expected,pending);
  for(unsigned mode=0;mode<4;++mode){
    check(RtlLoadSnapshotFromMemory(work,prepared),"persistent item eligibility approach restores");
    X3CoopState candidates=*X3CoopRuntimeState();
    memcpy(candidates.players[zero].body+5,candidates.players[x].body+5,2);
    memcpy(candidates.players[zero].body+8,candidates.players[x].body+8,2);
    if(mode==0){unsigned distant=px-256;candidates.players[x].body[5]=(uint8_t)distant;candidates.players[x].body[6]=(uint8_t)(distant>>8);}
    if(mode==1)check(X3CoopFall(&candidates,x),"fallen native X acquisition precondition applies");
    if(mode==2)check(X3CoopWithdraw(&candidates,x),"withdrawn native X acquisition precondition applies");
    memcpy(expected,work,prepared);
    check(X3CoopEncode(&candidates,expected+contact_context_offset(expected,prepared),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,prepared),"touching Zero and native X eligibility restore");
    door_install_player(&candidates);
    check(pickup_single_overlap(slot,&candidates.players[zero]),"native Zero geometry really touches the persistent item");
    check(RtlLoadSnapshotFromMemory(expected,prepared),"native persistent overlap probe restores");door_install_player(&candidates);
    for(unsigned i=0;i<calls;++i)pickup_native_items(0x00daa0);
    if(mode<3){
      check(g_ram[flag]==initial_flag && !g_ram[0x1f28],"Zero cannot bypass native X eligibility with distant, fallen or withdrawn X");
    }else{
      check(g_ram[flag]==(initial_flag|variant) && g_ram[0x1f28] && !memcmp(native_item,g_ram+slot,48),
            "simultaneous contact awards one original persistent acquisition to native X");
    }
  }
  check(RtlLoadSnapshotFromMemory(start,pending),"actual persistent award resumes after eligibility probes");
  const char *output=getenv("X3_COOP_ACQUISITION_OUTPUT");char path[1024];
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-%s-mid.sav",output,x+1,kind==11?"heart":"subtank");
    check(RtlSaveSnapshot(path),"native persistent award visual fixture saves");}
  unsigned positions[2][2];for(unsigned s=0;s<2;++s){positions[s][0]=support_position(s,5);positions[s][1]=support_position(s,8);}
  bool parked=true;
  for(unsigned i=0;i<8;++i){frame((SNES_PAD_RIGHT|SNES_PAD_B)|((SNES_PAD_RIGHT|SNES_PAD_B)<<12));
    for(unsigned s=0;s<2;++s)parked=parked && support_position(s,5)==positions[s][0] && support_position(s,8)==positions[s][1];}
  check(parked,"persistent award freeze parks both bodies despite independent movement/jump input");
  check(RtlLoadSnapshotFromMemory(start,pending),"persistent award movement probe restores");
  unsigned elapsed=0;
  do{frame(0);++elapsed;}while(g_ram[0x1f28] && elapsed<180);
  bool released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
  check(released && elapsed<180 && !g_ram[0x1f4f],"persistent native award completes and releases every shared freeze");
  const X3CoopState *completed=X3CoopRuntimeState();
  check(completed->players[x].life==X3_COOP_ALIVE && completed->players[zero].life==X3_COOP_ALIVE &&
        (completed->players[zero].body[0x27]&127)==28 && completed->players[zero].maximum_health==28,
        "persistent X award leaves native Zero health and participation intact");
  check(g_ram[flag]==(initial_flag|variant),"persistent campaign acquisition bit is committed once");
  if(kind==11){
    check(completed->players[x].maximum_health==initial_max+2 && contact_hp(x)==initial_hp+2,
          "heart increases only native X maximum and current health by the original two points");
    check(!memcmp(tanks,g_ram+0x1fb7,4),"heart acquisition leaves shared subtank reserves unchanged");
  }else{
    check(completed->players[x].maximum_health==initial_max && contact_hp(x)==initial_hp,
          "shared subtank acquisition leaves both personal health values unchanged");
    unsigned new_tanks=0;for(unsigned i=0;i<4;++i)new_tanks+=!(tanks[i]&128) && g_ram[0x1fb7+i]==128;
    check(new_tanks==1,"one native empty shared subtank is acquired");
  }
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-%s-complete.sav",output,x+1,kind==11?"heart":"subtank");
    check(RtlSaveSnapshot(path),"completed persistent award visual fixture saves");}
  size_t complete=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(start,pending),"pending native persistent award restores");
  for(unsigned i=0;i<elapsed;++i)frame(0);
  size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,complete,work,repeated,"persistent award and personal maximum replay to the identical full machine");
  if(getenv("X3_COOP_PERSISTENT_STREAMING"))native_permanent_streaming(kind,px,py,variant,work,expected,capacity);
  for(unsigned mode=0;mode<4;++mode){
    check(RtlLoadSnapshotFromMemory(start,pending),"pending persistent recipient lifecycle approach restores");
    if(mode>=2){
      for(unsigned i=0;i<120 && (kind==11?X3CoopRuntimeState()->players[x].maximum_health==initial_max:i<20);++i)frame(0);
      check(g_ram[0x1f28],"retirement interrupts the actual unfinished persistent award");
    }
    X3CoopState retired=*X3CoopRuntimeState();unsigned hp=retired.players[x].body[0x27]&127;
    check(mode&1?X3CoopFall(&retired,x):X3CoopWithdraw(&retired,x),"persistent recipient retires through its lifecycle API");
    size_t retired_size=RtlSaveSnapshotToMemory(expected,capacity);
    check(X3CoopEncode(&retired,expected+contact_context_offset(expected,retired_size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,retired_size),"retired persistent recipient state restores");
    for(unsigned i=0;i<180 && g_ram[0x1f28];++i)frame(0);
    const X3CoopState *result=X3CoopRuntimeState();
    bool unfrozen=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)unfrozen=unfrozen && !g_ram[a];
    check(unfrozen && result->players[x].life==(mode&1?X3_COOP_FALLEN:X3_COOP_ABSENT) &&
          (result->players[x].body[0x27]&127)==hp && contact_hp(zero)==28,
          "persistent cleanup releases the freeze without reviving or healing a retired recipient");
    check(g_ram[flag]==(initial_flag|variant) && result->players[x].maximum_health==initial_max+(kind==11?2:0),
          "committed persistent reward survives interrupted native presentation");
    X3CoopState rejoined=*result;
    if(mode&1){check(!X3CoopRejoin(&rejoined,x),"fallen persistent recipient cannot voluntarily return");}
    else{
      check(X3CoopRejoin(&rejoined,x),"living withdrawn persistent recipient can rejoin");
      size_t n=RtlSaveSnapshotToMemory(expected,capacity);
      check(X3CoopEncode(&rejoined,expected+contact_context_offset(expected,n),X3CoopEncodedSize()) &&
            RtlLoadSnapshotFromMemory(expected,n),"rejoined persistent recipient restores");
      /* This API probe has no public rejoin-placement policy. Restore its
       * prepared private route, avoiding a clamp to the distant survivor's
       * bounds. Safe public rejoin placement remains separately unqualified. */
      X3CoopViewState returning=*X3CoopViewsState();
      returning.valid|=(uint8_t)(1u<<x);returning.camera_valid|=(uint8_t)(1u<<x);
      X3CoopViewsSetState(&returning);
      for(unsigned i=0;i<3;++i)frame(0);unsigned before=support_position(x,5);
      X3CoopPlayer joining=X3CoopRuntimeState()->players[x];
      /* The authored subtank hangs above a pit. Use a short control window;
       * surviving that route or a safe rejoin location is not this probe. */
      for(unsigned i=0;i<8;++i)frame(SNES_PAD_LEFT<<(x*12));
      if(!(support_position(x,5)+4<before && contact_hp(x)==hp)){
        const X3CoopPlayer *p=&X3CoopRuntimeState()->players[x];
        fprintf(stderr,"persistent rejoin kind%u mode%u x=%u->%u y=%u hp=%u->%u life%u state=%u/%u->%u/%u lock=%u->%u queue=%02x/%02x->%02x/%02x old=%02x/%02x frozen=%u\n",
          kind,mode,before,support_position(x,5),support_position(x,8),hp,contact_hp(x),p->life,
          joining.body[2],joining.body[3],p->body[2],p->body[3],joining.body[0x6e],p->body[0x6e],
          joining.body[0xa3],joining.body[0xa5],p->body[0xa3],p->body[0xa5],p->body[0x6a],p->body[0xa6],g_ram[0x1f28]);
      }
      check(support_position(x,5)+4<before && contact_hp(x)==hp,
            "rejoined persistent collector resumes native control with retained health");
    }
  }
  free(start);check(RtlLoadSnapshotFromMemory(original,size),"native persistent probes restore the starting machine");
}
