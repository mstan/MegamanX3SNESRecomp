/* Current initialized snapshots must reject malformed personal/pass/view/menu
 * input before mutating any guest or title state. Exact supported wire fields
 * are used; this is not a fuzzer or an old-state migration contract. */
static void native_transactional_snapshot_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && X3CoopRuntimeState()->initialized,"transactional snapshot loads its actual initialized roster");size_t n=RtlSaveSnapshotToMemory(base,capacity);
    size_t actor=contact_context_offset(base,n),stride=(X3CoopEncodedSize()-8)/2,scene=door_scene_chunk(base,n),views=scene+16+8,bounds=views+X3_COOP_VIEW_STATE_BYTES+67,presentation=0;
    for(size_t i=bounds+114;i+8<n;++i)if(!memcmp(base+i,"X3P5",4)){presentation=i;break;}
    check(presentation && actor+X3CoopEncodedSize()<scene && n>=X3_COOP_MENU_STATE_BYTES+31,"current transactional snapshot locates its real serialized chunks");
    struct InvalidField {const char *name;size_t at;uint8_t value;} cases[]={
      {"invalid current seat",actor+5,2},{"invalid anchor",actor+6,2},
      {"invalid lifecycle",actor+8+stride-5,3},{"duplicate characters",actor+8+stride+stride-4,base[actor+8+stride-4]},
      {"invalid maximum health",actor+8+stride-1,0},
      {"invalid controller pass",actor+X3CoopEncodedSize()+24,3},{"invalid terrain pass",actor+X3CoopEncodedSize()+26+24,3},
      {"invalid scene owner",scene+6,2},{"invalid scene kind",scene+11,255},{"invalid physical history",scene+12,(uint8_t)(base[scene+12]|1)},
      {"invalid participation profile",scene+16+6,128},
      {"invalid independent policy",views,2},{"invalid view mask",views+1,4},{"invalid camera coordinate",views+6,0x21},
      {"invalid hostile captive owner",views+X3_COOP_VIEW_STATE_BYTES+54,31},{"invalid spawn count",views+X3_COOP_VIEW_STATE_BYTES+64,65},{"unknown frontier flags",views+X3_COOP_VIEW_STATE_BYTES+62,128},{"contradictory horizontal frontier",views+X3_COOP_VIEW_STATE_BYTES+62,35},{"mixed initial/moving frontier",views+X3_COOP_VIEW_STATE_BYTES+62,49},{"unknown camera pass",bounds+24,4},{"orphan personal camera backup",bounds+24,3},
      {"invalid shared-effect owner",bounds+114,3},{"invalid presentation identity",presentation+6,2},{"invalid rider bits",n-X3_COOP_MENU_STATE_BYTES-31,4},{"two captive owners",n-X3_COOP_MENU_STATE_BYTES-31,140},{"unknown attachment bits",n-X3_COOP_MENU_STATE_BYTES-31,144},{"invalid menu header",n-X3_COOP_MENU_STATE_BYTES,255}
    };
    for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);++i){memcpy(work,base,n);work[cases[i].at]=cases[i].value;bool accepted=RtlLoadSnapshotFromMemory(work,n);if(accepted)fprintf(stderr,"malformed snapshot accepted roster%u case%s offset%zu\n",roster,cases[i].name,cases[i].at);check(!accepted,"malformed current actor/pass/view/scene/presentation/menu input is refused before load");size_t an=RtlSaveSnapshotToMemory(after,capacity);same(base,n,after,an,"malformed current input leaves the complete machine unchanged");}
    size_t cuts[]={actor+4,actor+8+stride/2,views+150,n-1};for(unsigned i=0;i<sizeof(cuts)/sizeof(*cuts);++i){check(!RtlLoadSnapshotFromMemory(base,cuts[i]),"truncated current actor/view/menu chunk is refused before load");same(base,n,after,RtlSaveSnapshotToMemory(after,capacity),"truncated current input leaves the complete machine unchanged");}
    fprintf(stderr,"transactional snapshot roster%u malformed%u truncations%u machineBytes%zu\n",roster,(unsigned)(sizeof(cases)/sizeof(*cases)),(unsigned)(sizeof(cuts)/sizeof(*cuts)),n);
  }
}

/* The peer digest must see state outside canonical WRAM and stay read-only. */
static void native_coop_digest_checks(const char *fixture,uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  check(RtlLoadSnapshot(fixture) && X3CoopRuntimeReady(),"co-op digest starts from real initialized native gameplay");
  size_t n=RtlSaveSnapshotToMemory(base,capacity),actor=contact_context_offset(base,n),stride=(X3CoopEncodedSize()-8)/2;
  uint8_t *ram=malloc(0x20000);check(ram!=NULL,"co-op digest WRAM reference allocates");memcpy(ram,g_ram,0x20000);
  X3CoopState prior=*X3CoopRuntimeState();uint32_t digest=X3CoopRuntimeDigest();
  check(digest && X3CoopRuntimeDigest()==digest && !memcmp(&prior,X3CoopRuntimeState(),sizeof(prior)) && !memcmp(ram,g_ram,0x20000),"complete co-op digest repeats without changing either live actor or native WRAM");
  same(base,n,work,RtlSaveSnapshotToMemory(work,capacity),"co-op digest leaves the complete initialized machine unchanged");
  unsigned partner=prior.current^1;memcpy(work,base,n);size_t hp=actor+8+partner*stride+0x27;check((work[hp]&127)>1,"stored partner has real personal health to distinguish");--work[hp];
  check(RtlLoadSnapshotFromMemory(work,n) && !memcmp(ram,g_ram,0x20000),"stored-partner-only health change retains identical canonical native WRAM");
  check(X3CoopRuntimeDigest()!=digest,"peer confirmation detects stored partner state that the generic native digest cannot see");
  check(RtlLoadSnapshotFromMemory(base,n),"co-op digest restores its original actor state");memcpy(work,base,n);
  size_t owner=door_scene_chunk(base,n)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;work[owner]=(uint8_t)((work[owner]+1)%3);
  check(RtlLoadSnapshotFromMemory(work,n) && !memcmp(ram,g_ram,0x20000),"valid shared-effect owner change retains identical native WRAM");
  check(X3CoopRuntimeDigest()!=digest,"peer confirmation detects host-side shared-effect ownership outside the personal actor codec");
  check(RtlLoadSnapshotFromMemory(base,n),"co-op digest restores complete original ownership");
  int seat=X3Display_PresentationSeat();bool split=X3Display_LocalSplit();
  for(unsigned view=0;view<3;++view){X3Display_SetLocalSplit(view==2);X3Display_SetPresentationSeat(view==2?-1:(int)view);check(X3CoopRuntimeDigest()==digest,"local private/split presentation choices do not change the complete simulation digest");}
  X3Display_SetLocalSplit(split);X3Display_SetPresentationSeat(seat);
  same(base,n,after,RtlSaveSnapshotToMemory(after,capacity),"restored digest and cosmetic view probes preserve the complete machine");free(ram);
}

/* Match the rollback driver's snapshot and render-suppressed replay path. */
static void native_rollback_combat_checks(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==2,"local rollback combat starts from the real native Buffalo encounter");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x88]=255;
  check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"local rollback combat supplies only personal protection");door_install_player(&setup);g_ram[0xd3f]=127;
  for(unsigned f=0;f<216;++f)frame((SNES_PAD_Y*0x1001u)|(3u<<30));
  size_t cut=RtlNetplaySnapshotToMemory(base,capacity);check(cut>0,"actual rollback snapshot captures native charged combat");
  enum{count=32};uint8_t *native_ram=malloc(count*0x20000u),*extra=malloc(count*sizeof(view_probe.bytes));size_t extra_size[count];check(native_ram && extra,"bounded local rollback references allocate");
  for(unsigned f=0;f<count;++f){frame((f && f%4==0?SNES_PAD_Y*0x1001u:0)|(3u<<30));view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;X3CoopRuntimeSave(&view_probe.io);extra_size[f]=view_probe.used;memcpy(extra+f*sizeof(view_probe.bytes),view_probe.bytes,view_probe.used);memcpy(native_ram+f*0x20000u,g_ram,0x20000u);}
  check(RtlRollbackLoadFromMemory(base,cut),"actual rollback residue and complete title state restore before charged release");bool rendered=g_snes->disableRender;g_snes->disableRender=true;
  for(unsigned f=0;f<count;++f){frame((f && f%4==0?SNES_PAD_Y*0x1001u:0)|(3u<<30));view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;X3CoopRuntimeSave(&view_probe.io);
    if(memcmp(native_ram+f*0x20000u,g_ram,0x20000u)){unsigned at=0;while(at<0x20000 && native_ram[f*0x20000u+at]==g_ram[at])++at;fprintf(stderr,"actual rollback native mismatch f%u RAM%05x expected%02x actual%02x world%u\n",f,at,native_ram[f*0x20000u+at],g_ram[at],g_ram[0x9cc]);check(false,"actual rollback replay retains identical native WRAM on every charged-combat update");}
    if(view_probe.used!=extra_size[f] || memcmp(extra+f*sizeof(view_probe.bytes),view_probe.bytes,view_probe.used)){size_t at=0;while(at<view_probe.used && extra[f*sizeof(view_probe.bytes)+at]==view_probe.bytes[at])++at;fprintf(stderr,"actual rollback extra mismatch f%u byte%zu expected%02x actual%02x\n",f,at,extra[f*sizeof(view_probe.bytes)+at],view_probe.bytes[at]);check(false,"actual rollback replay retains the complete co-op serializer stream on every update");}
  }
  g_snes->disableRender=rendered;free(native_ram);free(extra);check(true,"actual charged-combat rollback matches all thirty-two native WRAM/title-state updates");(void)work;
}
