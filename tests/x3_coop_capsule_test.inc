/* Native capsule coordinates/indices come from the cartridge's stage tables.
 * Route/roster/camera positioning is preparation, not route QA. */
static unsigned capsule_table(unsigned address,unsigned bytes){
  unsigned at=((address>>16)&127)*32768+(address&32767);
  check(at+bytes<=g_snes->cart->romImageSize,"native capsule table read is bounded");
  return g_snes->cart->rom[at]|(bytes==2?g_snes->cart->rom[at+1]<<8:0);
}
static unsigned capsule_x(void){return capsule_table(0x06ccbe + g_ram[0x1fae]*4,2);}
static unsigned capsule_y(void){return capsule_table(0x06ccc0+g_ram[0x1fae]*4,2);}
static unsigned capsule_mask(void){return capsule_table(0x06bbfd+capsule_table(0x06ccfa+g_ram[0x1fae],1),1);}
static void capsule_position(uint8_t *body,unsigned offset,unsigned value){body[offset]=(uint8_t)value;body[offset+1]=(uint8_t)(value>>8);}
static void capsule_world_call(void){
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;target_world_call();
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
}
static bool capsule_constructor_seen;
static void capsule_constructor_probe(CpuState *cpu,uint32_t pc){
  unsigned d=cpu->D;if(pc!=0x13c000 || d<0xd18 || d>=0x10d8 || g_ram[d+10]!=0x4d || g_ram[d+1])return;
  const X3CoopState *s=X3CoopRuntimeState();
  fprintf(stderr,"capsule constructor owner%u char%u native%u HP%u/max%u objectHP%u\n",
      s->current,s->players[s->current].character,g_ram[0xa8e],g_ram[0x9ff]&127,g_ram[0x1fd2],g_ram[d+0x27]);
  check(s->players[s->current].character==X3_COOP_X && !g_ram[0xa8e] &&
        g_ram[0x1fd2]==s->players[s->current].maximum_health,"native capsule constructor evaluates X's real acquisition requirements");
  capsule_constructor_seen=true;
}
static unsigned capsule_approach(uint8_t *original,size_t size,uint8_t *work,size_t capacity,bool reversed){
  check(RtlLoadSnapshotFromMemory(original,size),"native capsule approach restores");
  X3CoopState setup=*X3CoopRuntimeState();
  if(reversed){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  unsigned x=setup.players[0].character==X3_COOP_X?0:1;
  setup.current=setup.anchor=(uint8_t)(x^1);
  unsigned px=capsule_x(),py=capsule_y();
  uint8_t *b=setup.players[x].body;capsule_position(b,5,px-80);capsule_position(b,8,py-40);
  memcpy(b+0x22,b+5,2);memcpy(b+0x24,b+8,2);memset(b+0x1a,0,6);b[4]=b[7]=0;
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+contact_context_offset(work,size),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,size),"remote native X capsule route restores");
  door_install_player(&setup);X3CoopViewsConfigure(true);X3CoopRuntimeFrame();
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=g_ram[0x1fae];
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){
    views.camera[s][0]=(uint16_t)(s==x?((px-128)&~255u):view_read(0x1e5d));
    views.camera[s][1]=(uint16_t)(s==x?(py&~255):view_read(0x1e60));
    views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];
  }
  X3CoopViewsSetState(&views);capsule_constructor_seen=false;
  check(interp_bridge_add_pre_opcode_hook(0x13c000,capsule_constructor_probe),"native capsule constructor ownership probe registers");
  for(unsigned i=0;i<3;++i)frame(0);
  CpuState resume=g_cpu;view_native_call(0x00de3a,0,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  capsule_world_call();capsule_world_call();
  unsigned slot=0;
  for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x4d)slot=d;
  if(!slot)fprintf(stderr,"capsule setup stage%u flags=%02x/%02x camera=%u,%u\n",g_ram[0x1fae],g_ram[0x1fd1],g_ram[0x1fd7],
      X3CoopViewsState()->camera[x][0],X3CoopViewsState()->camera[x][1]);
  check(slot && view_read(slot+5)==px && view_read(slot+8)==py && g_ram[slot+1]==2 && g_ram[slot+2]==2,
  if(!slot && getenv("X3_COOP_CAPSULE_OUTPUT")){char path[1024];snprintf(path,sizeof(path),"%s-unqualified.sav",getenv("X3_COOP_CAPSULE_OUTPUT"));check(RtlSaveSnapshot(path),"missing native capsule retains its actual content/admission state");}
        "retail allocator and native constructor prepare the actual stage capsule");
  check(capsule_constructor_seen,"actual native capsule construction was observed");
  setup=*X3CoopRuntimeState();b=setup.players[x].body;
  capsule_position(b,5,px-38);capsule_position(b,8,py-16); /* Original opening rectangle. */
  memcpy(b+0x22,b+5,2);memcpy(b+0x24,b+8,2);memset(b+0x1a,0,6);
  size_t n=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,n),"touching native X capsule precondition restores without admission flags");
  return slot;
}
static unsigned capsule_wait_open(unsigned slot,unsigned x,bool paired){
  unsigned ticks=0;bool ready=false;
  for(;ticks<1800;++ticks){
    bool modal=g_ram[0x1f4f] || g_ram[0x1f45] || g_ram[0x1f63];
    unsigned buttons=modal && ticks%40<4?SNES_PAD_B:0;
    unsigned owner=paired && X3CoopRuntimeState()->players[x].life!=X3_COOP_ALIVE?x^1:x;
    frame((buttons<<(paired?owner*12:0)) | (paired && owner==x?((SNES_PAD_RIGHT|SNES_PAD_B)<<((x^1)*12)):0));
    ready=g_ram[slot+2]==6 && !g_ram[0x1f4f] && !g_ram[0x1f45] && !g_ram[0x1f63] && !g_ram[X3_COOP_BODY+0x6e];
    if(ready && (!paired || !X3CoopRuntimeSceneActive()))break;
  }
  fprintf(stderr,"capsule open paired%u xseat%u ticks%u item=%u/%u/%u body=%u/%u xy=%u,%u lock%u flags=%u/%u/%u text=%u phase=%u/%u\n",
      paired,x,ticks,g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],g_ram[0x9da],g_ram[0x9db],view_read(0x9dd),view_read(0x9e0),
      g_ram[0xa46],g_ram[0x1f4f],g_ram[0x1f45],g_ram[0x1f63],g_ram[0x1f36],g_ram[0xd1],g_ram[0xd2]);
  check(ready && ticks<1800,"original capsule opening/dialogue releases native control");return ticks;
}
typedef struct CapsuleDemo {uint64_t hash;unsigned frames,x,y;} CapsuleDemo;
static CapsuleDemo capsule_wait_award(unsigned slot,unsigned x,bool paired){
  CapsuleDemo demo={1469598103934665603ull,0,0,0};bool completed=false;unsigned ticks=0;
  for(;ticks<2400;++ticks){
    bool recorded=g_ram[slot+2]==8 && g_ram[slot+3]==14;
    unsigned input=paired && g_ram[0x1f45]?(SNES_PAD_LEFT|SNES_PAD_B|SNES_PAD_Y)<<(x*12):0;
    frame(input | (paired?(SNES_PAD_RIGHT|SNES_PAD_B)<<((x^1)*12):0));
    /* Native04:8FB0 waits for a new Shoot edge in the radar survey. */
    if(g_ram[0x1f45]==2)input=(ticks%40<4?SNES_PAD_Y:0)<<(paired?x*12:0);
    if(recorded){
      static const unsigned offsets[]={2,3,5,6,8,9,0x1a,0x1b,0x1c,0x1d,0x31,0x33,0x36,0x37,0x38,0x39,0x3a,0x3b};
      for(unsigned i=0;i<sizeof(offsets)/sizeof(*offsets);++i){demo.hash^=g_ram[X3_COOP_BODY+offsets[i]];demo.hash*=1099511628211ull;}
      ++demo.frames;
    }
    bool ready=g_ram[slot+2]==8 && g_ram[slot+3]>=16 && !g_ram[0x1f4f] && !g_ram[0x1f45] &&
        !g_ram[0x1f63] && !g_ram[X3_COOP_BODY+0x6e];
    if(ready && !completed){completed=true;demo.x=view_read(0x9dd);demo.y=view_read(0x9e0);}
    if(completed && (!paired || !X3CoopRuntimeSceneActive()))break;
  }
  fprintf(stderr,"capsule award paired%u xseat%u ticks%u item=%u/%u/%u demo=%u/%llx xy=%u,%u lock%u flags=%u/%u/%u armor=%02x\n",
      paired,x,ticks,g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],demo.frames,(unsigned long long)demo.hash,
      demo.x,demo.y,g_ram[0xa46],g_ram[0x1f4f],g_ram[0x1f45],g_ram[0x1f63],g_ram[0x1fd1]);
  check(completed && ticks<2400 && demo.frames,"native capsule acquisition and its recorded demonstration finish");return demo;
  if(!completed && getenv("X3_COOP_CAPSULE_OUTPUT")){char path[1024];snprintf(path,sizeof(path),"%s-award-stalled.sav",getenv("X3_COOP_CAPSULE_OUTPUT"));check(RtlSaveSnapshot(path),"unqualified native capsule demonstration retains its actual stalled state");}
}
static void capsule_enter(unsigned slot){
  view_word(0x9dd,capsule_x());view_word(0x9fa,capsule_x());memset(g_ram+0x9f2,0,6);
  capsule_world_call();
  check(g_ram[slot+2]==8 && g_ram[0x1f4f],"native X steps into the open capsule without supplied award/completion flags");
}
static void capsule_retirement_checks(const uint8_t *opening,size_t open_size,const uint8_t *award,size_t award_size,
                                     unsigned slot,unsigned x,uint8_t *work,size_t capacity){
  for(unsigned mode=0;mode<6;++mode){
    bool acquisition=mode==2 || mode==3 || mode==5;
    bool middle=mode==1 || mode==3 || mode>=4;
    unsigned seat=mode>=4?x^1:x;bool fallen=mode==1 || mode==3 || mode==5;
    check(RtlLoadSnapshotFromMemory(acquisition?award:opening,acquisition?award_size:open_size),
          "pending native capsule lifecycle approach restores");
    if(middle){
      if(acquisition){for(unsigned i=0;i<1500 && g_ram[slot+3]!=14;++i)frame(0);check(g_ram[slot+3]==14,"retirement interrupts the actual native recorded demonstration");}
      else for(unsigned i=0;i<500;++i)frame(i%40<4?SNES_PAD_B<<(x*12):0);
    }
    X3CoopState retired=*X3CoopRuntimeState();unsigned hp=retired.players[seat].body[0x27]&127;
    unsigned survivor=seat^1,survivor_hp=retired.players[survivor].body[0x27]&127;
    check(fallen?X3CoopFall(&retired,seat):X3CoopWithdraw(&retired,seat),"native capsule participant retires through its lifecycle API");
    size_t n=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&retired,work+contact_context_offset(work,n),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,n),"retired native capsule participant restores");
    if(seat==x){
      check(interp_bridge_add_pre_opcode_hook(0x13c000,pickup_continuation_probe),"retired capsule continuation probe registers");
      pickup_probe_slot=slot;pickup_probe_pc=0x13c000;pickup_probe_seat=x;pickup_probe_seen=false;
    }
    if(acquisition)capsule_wait_award(slot,x,true);else capsule_wait_open(slot,x,true);
    const X3CoopState *result=X3CoopRuntimeState();
    fprintf(stderr,"capsule retirement mode%u seat%u life%u survivor%u xy=%u,%u flags=%u/%u/%u/%u hp=%u/%u\n",
      mode,seat,result->players[seat].life,survivor,support_position(survivor,5),support_position(survivor,8),
      g_ram[0x1f4f],g_ram[0x1f45],g_ram[0x1f63],g_ram[0x1f23],contact_hp(seat),contact_hp(survivor));
    check(!X3CoopRuntimeSceneActive() && result->players[seat].life==(fallen?X3_COOP_FALLEN:X3_COOP_ABSENT) &&
          contact_hp(seat)==hp && contact_hp(survivor)==survivor_hp && result->players[survivor].life==X3_COOP_ALIVE,
          "capsule cleanup preserves the survivor and retired participant without healing or revival");
    if(seat==x)check(pickup_probe_seen,"actual retired-X capsule script borrow saves and restores in place");
    check((g_ram[0x1fd1]&15)==(acquisition?capsule_mask():0),"capsule interruption preserves exactly the earned native armor acquisition");
    const char *output=getenv("X3_COOP_CAPSULE_OUTPUT");char path[1024];
    if(output && *output){snprintf(path,sizeof(path),"%s-p%u-retirement%u.sav",output,x+1,mode);
      check(RtlSaveSnapshot(path),"native capsule retirement visual fixture saves");}
    /* Let native terrain classify the newly arrived body before supplying
     * a fresh jump edge; the capsule room has a wall beside this landing. */
    for(unsigned i=0;i<2;++i)frame(0);
    unsigned before=support_position(survivor,5),before_y=support_position(survivor,8);
    bool moved=false,healthy=true;
    for(unsigned i=0;i<30;++i){
      frame((SNES_PAD_LEFT|(i%12<4?SNES_PAD_B:0))<<(survivor*12));
      moved=moved || support_position(survivor,5)+8<before || support_position(survivor,8)+16<before_y;
      healthy=healthy && contact_hp(survivor)==survivor_hp && (g_ppu->inidisp&0x8f)==15;
    }
    if(!moved || contact_hp(survivor)!=survivor_hp || (g_ppu->inidisp&0x8f)!=15)
      fprintf(stderr,"capsule survivor mode%u xy=%u,%u->%u,%u hp%u state%u/%u lock%u bright%02x freezes=%u/%u/%u/%u\n",
          mode,before,before_y,support_position(survivor,5),support_position(survivor,8),contact_hp(survivor),
          g_ram[0x9da],g_ram[0x9db],g_ram[0xa46],g_ppu->inidisp,g_ram[0x1f25],g_ram[0x1f26],g_ram[0x1f28],g_ram[0x1f2b]);
    if(!moved)fprintf(stderr,"capsule survivor native=%u buttons=%04x/%04x masks=%02x,%02x,%02x,%02x,%02x,%02x held=%04x edge=%04x body7c=%u ground=%02x\n",
        g_ram[0xa8e],RtlGetPadState(0),RtlGetPadState(1),g_ram[0xffc0],g_ram[0xffc1],g_ram[0xffc2],g_ram[0xffc3],g_ram[0xffc4],g_ram[0xffc5],
        view_read(0xa0e),view_read(0xa12),g_ram[0xa54],g_ram[0xa03]);
    check(moved && healthy,
          "capsule survivor resumes native movement in a bright living world");
    if(fallen){X3CoopState rejoin=*X3CoopRuntimeState();check(!X3CoopRejoin(&rejoin,seat),"fallen capsule participant cannot voluntarily return");}
  }
}
static void native_capsule_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity,bool reversed){
  unsigned slot=capsule_approach(original,size,work,capacity,reversed);
  X3CoopState setup=*X3CoopRuntimeState();unsigned x=setup.players[0].character==X3_COOP_X?0:1;
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  X3CoopRuntimeEnable(false);X3CoopViewsAdoptPlayer(g_ram,&setup,x,false);
  setup.current=(uint8_t)x;door_install_player(&setup);capsule_world_call();
  if(g_ram[slot+2]!=4)fprintf(stderr,"native capsule claim slot%04x state%u/%u/%u player=%u,%u char%u lock%u\n",
      slot,g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],view_read(0x9dd),view_read(0x9e0),g_ram[0xa8e],g_ram[0xa46]);
  check(g_ram[slot+2]==4 && g_ram[0x1f4f],"original single-X contact opens the authored capsule");
  uint8_t native_capsule[64];memcpy(native_capsule,g_ram+slot,64);
  capsule_wait_open(slot,0,false);capsule_enter(slot);
  CapsuleDemo reference=capsule_wait_award(slot,0,false);
  check((g_ram[0x1fd1]&15)==capsule_mask(),"original stage capsule awards its native armor bit");
  for(unsigned mode=0;mode<5;++mode){
    check(RtlLoadSnapshotFromMemory(work,prepared),"capsule native-character eligibility approach restores");
    X3CoopState candidates=*X3CoopRuntimeState();unsigned zero=x^1;
    memcpy(candidates.players[zero].body+5,candidates.players[x].body+5,2);
    memcpy(candidates.players[zero].body+8,candidates.players[x].body+8,2);
    if(mode<3){candidates.players[x].body[5]=0xd8;candidates.players[x].body[6]=3;}
    if(mode==1)check(X3CoopFall(&candidates,x),"fallen capsule X eligibility precondition applies");
    if(mode==2)check(X3CoopWithdraw(&candidates,x),"withdrawn capsule X eligibility precondition applies");
    if(mode==3)candidates.players[x].body[0x27]=0;
    memcpy(expected,work,prepared);
    check(X3CoopEncode(&candidates,expected+contact_context_offset(expected,prepared),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,prepared),"touching Zero capsule eligibility restores");
    door_install_player(&candidates);
    X3CoopViewState touching=*X3CoopViewsState();
    memcpy(touching.camera[zero],touching.camera[x],sizeof(touching.camera[x]));
    memcpy(touching.limits[zero],touching.limits[x],sizeof(touching.limits[x]));
    memcpy(touching.current[zero],touching.current[x],sizeof(touching.current[x]));X3CoopViewsSetState(&touching);
    check(RtlSaveSnapshotToMemory(expected,capacity)==prepared,"touching Zero's prepared capsule view saves");
    check(pickup_single_overlap(slot,&candidates.players[zero]),"native Zero geometry actually touches the capsule opening rectangle");
    check(RtlLoadSnapshotFromMemory(expected,prepared),"capsule geometry probe restores");door_install_player(&candidates);
    capsule_world_call();
    if(mode<4 && (g_ram[slot+2]!=2 || g_ram[0x1f4f]))fprintf(stderr,"capsule eligibility mode%u active%u phase%u/%u/%u modal%u char%u camera=%u,%u\n",
        mode,g_ram[slot],g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],g_ram[0x1f4f],g_ram[0xa8e],view_read(0x1e5d),view_read(0x1e60));
    check(mode<4?(g_ram[slot+2]==2 && !g_ram[0x1f4f]):
          (g_ram[slot+2]==4 && g_ram[0x1f4f] && !memcmp(native_capsule,g_ram+slot,64)),
          "native capsule eligibility excludes Zero-only contacts and admits one simultaneous X contact");
  }
  check(RtlLoadSnapshotFromMemory(work,prepared),"paired capsule starts from matched native contact");
  check(interp_bridge_add_pre_opcode_hook(0x13c164,pickup_continuation_probe),"native capsule opening continuation probe registers");
  pickup_probe_slot=slot;pickup_probe_pc=0x13c164;pickup_probe_seat=x;pickup_probe_seen=false;
  capsule_world_call();
  check(g_ram[slot+2]==4 && g_ram[0x1f4f] && !memcmp(native_capsule,g_ram+slot,64),
        "co-op capsule opening matches one original native update");
  check(X3CoopRuntimeState()->anchor==x && X3CoopRuntimeState()->current==x,
        "native capsule adopts X before committing the shared camera and locks");
  check(pickup_probe_seen,"native capsule opening owns a restorable interrupted world borrow");
  size_t open_size=RtlSaveSnapshotToMemory(expected,capacity);uint8_t *opening=malloc(open_size);
  check(opening!=NULL,"pending capsule opening replay buffer allocates");memcpy(opening,expected,open_size);
  const unsigned fields[]={6,7,8,9,11,11,12,14},values[]={x^1,4,8,15,0,3,1,1};
  for(unsigned i=0;i<sizeof(fields)/sizeof(*fields);++i){
    memcpy(work,opening,open_size);work[door_scene_chunk(work,open_size)+fields[i]]=(uint8_t)values[i];
    check(!RtlLoadSnapshotFromMemory(work,open_size),"malformed native capsule owner/type/input history rejects before mutation");
    size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);
    same(opening,open_size,work,unchanged,"rejected capsule state leaves the complete machine unchanged");
  }
  const char *output=getenv("X3_COOP_CAPSULE_OUTPUT");char path[1024];
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-open.sav",output,x+1);check(RtlSaveSnapshot(path),"native capsule opening visual fixture saves");}
  capsule_wait_open(slot,x,true);
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-open-complete.sav",output,x+1);check(RtlSaveSnapshot(path),"released capsule dialogue visual fixture saves");}
  size_t open_complete=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(opening,open_size),"pending native capsule opening restores");capsule_wait_open(slot,x,true);
  size_t open_repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,open_complete,work,open_repeated,"capsule dialogue, participant transport and raw input history replay identically");
  X3CoopState entering=*X3CoopRuntimeState();view_word(0x9dd,capsule_x());view_word(0x9fa,capsule_x());memset(g_ram+0x9f2,0,6);
  /* X is the live anchor: capture only the approach position, not script state. */
  X3CoopCapture(&entering,g_ram);
  size_t n=RtlSaveSnapshotToMemory(expected,capacity);
  check(X3CoopEncode(&entering,expected+contact_context_offset(expected,n),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(expected,n),"native capsule entry position restores");
  check(interp_bridge_add_pre_opcode_hook(0x13c250,pickup_continuation_probe),"native capsule award continuation probe registers");
  pickup_probe_slot=slot;pickup_probe_pc=0x13c250;pickup_probe_seat=x;pickup_probe_seen=false;
  capsule_enter(slot);
  check(pickup_probe_seen,"native capsule acquisition owns a restorable interrupted world borrow");
  size_t award_size=RtlSaveSnapshotToMemory(expected,capacity);uint8_t *award=malloc(award_size);
  check(award!=NULL,"pending capsule award replay buffer allocates");memcpy(award,expected,award_size);
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-award.sav",output,x+1);check(RtlSaveSnapshot(path),"native capsule award visual fixture saves");}
  CapsuleDemo paired=capsule_wait_award(slot,x,true);
  check(reference.frames==paired.frames && reference.hash==paired.hash && reference.x==paired.x && reference.y==paired.y,
        "both-seat co-op preserves the exact native recorded demonstration despite conflicting controller input");
  check((g_ram[0x1fd1]&15)==capsule_mask() && contact_hp(x)==16 && contact_hp(x^1)==28,
        "capsule awards native X armor once while preserving both personal health values");
  if(output && *output){snprintf(path,sizeof(path),"%s-p%u-complete.sav",output,x+1);check(RtlSaveSnapshot(path),"completed native capsule visual fixture saves");}
  size_t complete=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(award,award_size),"pending native capsule award restores");capsule_wait_award(slot,x,true);
  size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,complete,work,repeated,"capsule award, recorded control and participant arrival replay to the identical full machine");
  capsule_retirement_checks(opening,open_size,award,award_size,slot,x,expected,capacity);
  free(opening);free(award);
  check(RtlLoadSnapshotFromMemory(original,size),"native capsule probes restore starting machine");
  (void)expected;
}
