/* Authored stage1 midboss entrance. Teleport/camera setup bypasses the route,
 * not native allocation, admission, locks, animation or forced movement. */
static void door_install_player(const X3CoopState *setup){
  const X3CoopPlayer *world=&setup->players[setup->current];
  memcpy(g_ram+X3_COOP_BODY,world->body,sizeof(world->body));
  memcpy(g_ram+X3_COOP_AUX,world->auxiliaries,sizeof(world->auxiliaries));
  memcpy(g_ram+X3_COOP_SHOTS,world->shots,sizeof(world->shots));
  memcpy(g_ram+X3_COOP_PALETTE,world->palette,sizeof(world->palette));
  g_ram[0x1f1e]=world->shot_command;g_ram[0x1fd2]=world->maximum_health;g_ram[0xa1]=1;
  g_ram[0x1fff]=world->native_flags;
}
static size_t door_scene_chunk(const uint8_t *bytes,size_t size){
  for(size_t i=0;i+16<=size;++i)if(!memcmp(bytes+i,"X3S2",4))return i;
  check(0,"door scene chunk exists");return 0;
}
static unsigned door_approach(uint8_t *original,size_t size,uint8_t *work,size_t capacity,unsigned character){
  check(RtlLoadSnapshotFromMemory(original,size),"native gate approach restores");
  X3CoopState setup=*X3CoopRuntimeState();unsigned seat=setup.players[0].character==character?0:1;
  setup.anchor=setup.current=(uint8_t)(seat^1);
  setup.players[seat].body[5]=0x60;setup.players[seat].body[6]=9; /*2400*/
  setup.players[seat].body[8]=0x9b;setup.players[seat].body[9]=5; /*1435*/
  memset(setup.players[seat].body+0x1a,0,6);
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,size),"private remote gate approach restores");
  /* The fixture explicitly reverses native world ownership too. Copy the
   * whole owning context, not just character/body bytes over the old X. */
  door_install_player(&setup);
  X3CoopViewsConfigure(true);X3CoopRuntimeFrame();
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=(uint16_t)(s==seat?2416:256);
    views.camera[s][1]=(uint16_t)(s==seat?1280:1024);
    views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];}
  X3CoopViewsSetState(&views);
  CpuState resume=g_cpu;view_native_call(0x00de3a,0,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  pickup_native_items(0x00daa0);pickup_native_items(0x00daa0);
  unsigned slot=0;
  for(unsigned d=0x1518;d<0x1818;d+=48)
    if(g_ram[d] && g_ram[d+10]==0x11 && view_read(d+5)==2552 && view_read(d+8)==1431)slot=d;
  check(slot && g_ram[slot+1]==2 && !g_ram[0x1f28],
        "retail allocator and constructor prepare the authored gate without triggering it");
  setup=*X3CoopRuntimeState();setup.players[seat].body[5]=0xf0;setup.players[seat].body[6]=9; /*2544*/
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+contact_context_offset(work,prepared),X3CoopEncodedSize()) &&
        RtlLoadSnapshotFromMemory(work,prepared),"touching remote actor restores without admission flags");
  return slot;
}
static uint32_t scene_menu_pad(unsigned frame_index){
  return getenv("X3_COOP_TRANSITION_MENU") && (frame_index<2 || frame_index==32 || frame_index==33)?
      SNES_PAD_START|((uint32_t)SNES_PAD_START<<12):0;
}
static void native_door_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity,unsigned character){
  unsigned slot=door_approach(original,size,work,capacity,character);
  const X3CoopState setup=*X3CoopRuntimeState();unsigned remote=setup.anchor^1;
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  /* Native single-body control: use the same touching character and local
   * camera. The distant other body is absent from this reference update. */
  X3CoopRuntimeEnable(false);memcpy(g_ram+X3_COOP_BODY,setup.players[remote].body,X3_COOP_BODY_BYTES);
  g_ram[0x1fff]=setup.players[remote].native_flags;
  view_word(0x1e5d,2416);view_word(0x1e60,1280);
  pickup_native_items(0x00daa0);
  check(g_ram[slot+1]==4 && g_ram[0x1f28],"original single-body gate admits the touching remote character");
  uint8_t native_item[48];memcpy(native_item,g_ram+slot,sizeof(native_item));
  check(RtlLoadSnapshotFromMemory(work,prepared),"paired gate starts from the matched native approach");
  check(interp_bridge_add_pre_opcode_hook(0x01e0f1,pickup_continuation_probe),"native gate scene continuation probe registers");
  pickup_probe_slot=slot;pickup_probe_pc=0x01e0f1;pickup_probe_seat=remote;pickup_probe_seen=false;
  pickup_native_items(0x00daa0);
  if(g_ram[slot+1]!=4)fprintf(stderr,"gate remote%u char%u: state=%u freeze=%u anchor=%u current=%u\n",
      remote,setup.players[remote].character,g_ram[slot+1],g_ram[0x1f28],
      X3CoopRuntimeState()->anchor,X3CoopRuntimeState()->current);
  check(g_ram[slot+1]==4 && g_ram[0x1f28],"remote actor admits the single authored native gate");
  check(pickup_probe_seen,"actual admitted native gate retains a restorable scene and world borrow");
  check(!memcmp(native_item,g_ram+slot,sizeof(native_item)),"paired admission matches one original gate update");
  check(X3CoopRuntimeState()->anchor==remote,"native gate adopts the actual entering actor");
  check(view_read(0x1e5d)==2416 && view_read(0x1e60)==1280,
        "gate camera adopts the entering region before changing world ownership");
  size_t admitted=RtlSaveSnapshotToMemory(expected,capacity);
  if(getenv("X3_COOP_DOOR_FATAL")){native_scene_fatal(false,remote,work,expected,capacity);return;}
  uint8_t *admitted_state=malloc(admitted),*finished=malloc(capacity);
  check(admitted_state && finished,"native scene replay and refusal buffers allocate");
  memcpy(admitted_state,expected,admitted);size_t scene_chunk=door_scene_chunk(expected,admitted);
  const unsigned bad_field[]={4,6,6,7,8,9,10,11},bad_value[]={0,2,remote^1,4,8,15,2,1};
  for(unsigned i=0;i<8;++i){
    memcpy(finished,expected,admitted);finished[scene_chunk+bad_field[i]]=(uint8_t)bad_value[i];
    check(!RtlLoadSnapshotFromMemory(finished,admitted),"invalid native door scene is rejected before load");
    size_t unchanged=RtlSaveSnapshotToMemory(finished,capacity);
    same(expected,admitted,finished,unchanged,"refused scene leaves the complete machine unchanged");
  }
  const char *output=getenv("X3_COOP_DOOR_OUTPUT");char path[1024];bool arrival_saved=false;
  unsigned elapsed=0;
  for(unsigned i=0;i<900;++i){frame(scene_menu_pad(i));elapsed=i+1;
    if(getenv("X3_COOP_TRANSITION_MENU") && (i==0 || i==32 || i==60)){
      X3CoopMenuInfo panel;check(X3CoopRuntimeSceneActive() && !X3CoopMenuRead(&panel) && !(g_ram[0x1f37]&1),"both actual Start requests leave door/transport ownership with the native scene");
      size_t pending=RtlSaveSnapshotToMemory(work,capacity);check(RtlLoadSnapshotFromMemory(work,pending),"actual pending scene with prior Start requests restores without a hidden pause owner");
    }
    if(i==60 && output && *output){
      snprintf(path,sizeof(path),"%s-p%u-%s-mid.sav",output,remote+1,character==X3_COOP_ZERO?"zero":"x");
      check(RtlSaveSnapshot(path),"pending native gate visual fixture saves");
    }
    if(i>8 && !arrival_saved && X3CoopRuntimeSceneActive() && !g_ram[0x1f28] && !g_ram[X3_COOP_BODY+0x6e]){
      arrival_saved=true;
      if(output && *output){
        snprintf(path,sizeof(path),"%s-p%u-%s-arrival.sav",output,remote+1,character==X3_COOP_ZERO?"zero":"x");
        check(RtlSaveSnapshot(path),"native gate arrival visual fixture saves");
      }
    }
    if(!X3CoopRuntimeSceneActive())break;
  }
  check(elapsed<900 && arrival_saved,"door transport waits through the following native miniboss introduction");
  check(!g_ram[0x1f28] && view_read(0x1e5d)==2560 && support_position(remote,5)>2552,
        "native gate finishes its original forced walk and cleanup");
  const X3CoopState *arrived=X3CoopRuntimeState();
  check(!X3CoopRuntimeSceneActive() && arrived->players[0].life==X3_COOP_ALIVE &&
        arrived->players[1].life==X3_COOP_ALIVE,
        "native door transport retains both living participants through cleanup");
  for(unsigned seat=0;seat<2;++seat)if(arrived->players[seat].character==X3_COOP_X)
    for(unsigned part=0;part<3;++part)if(g_ram[0x1fd1]&(2u<<part)){
      const uint8_t *child=arrived->players[seat].auxiliaries+part*32;
      check(child[0] && child[0x10]==0x5d,"authored door arrival reconstructs X's acquired native armor children");
    }
  check((arrived->players[0].body[0x27]&127)==(setup.players[0].body[0x27]&127) &&
        (arrived->players[1].body[0x27]&127)==(setup.players[1].body[0x27]&127),
        "door transport preserves both personal health values");
  check(abs((int)support_position(remote,5)-(int)support_position(remote^1,5))<=16 &&
        abs((int)support_position(remote,8)-(int)support_position(remote^1,8))<=8,
        "transported X and Zero arrive on the same native floor with their different feet offsets");
  if(output && *output){
    snprintf(path,sizeof(path),"%s-p%u-%s-complete.sav",output,remote+1,character==X3_COOP_ZERO?"zero":"x");
    check(RtlSaveSnapshot(path),"completed native gate visual fixture saves");
  }
  size_t finished_size=RtlSaveSnapshotToMemory(finished,capacity);
  check(RtlLoadSnapshotFromMemory(expected,admitted),"admitted native gate snapshot restores");
  for(unsigned i=0;i<elapsed;++i)frame(scene_menu_pad(i));
  size_t replay=RtlSaveSnapshotToMemory(expected,capacity);
  same(finished,finished_size,expected,replay,"native gate forced-walk and cleanup deterministic replay");
  free(finished);
  unsigned before0=support_position(0,5),before1=support_position(1,5);
  for(unsigned i=0;i<15;++i)frame(SNES_PAD_RIGHT);
  check(support_position(0,5)>before0+10 && support_position(1,5)==before1,
        "P1 resumes independent native control after transport while P2 stays parked by its own input");
  size_t moving=RtlSaveSnapshotToMemory(expected,capacity);
  if(!X3CoopRuntimePreflight(expected,moving)){const X3CoopViewState *v=X3CoopViewsState();fprintf(stderr,"released door views stage%u masks%u/%u valid%u scene%u nativeflag%02x\n",v->stage,v->valid,v->camera_valid,X3CoopViewsValidState(v),X3CoopRuntimeSceneActive(),g_ram[0x1f5d]);for(unsigned seat=0;seat<2;++seat)fprintf(stderr,"released door seat%u camera%u,%u targets%u/%u/%u/%u current%u/%u/%u/%u\n",seat,v->camera[seat][0],v->camera[seat][1],v->limits[seat][0],v->limits[seat][1],v->limits[seat][2],v->limits[seat][3],v->current[seat][0],v->current[seat][1],v->current[seat][2],v->current[seat][3]);check(RtlSaveSnapshot("native-released-door-preflight.sav"),"invalid released-door machine saves without a picker");}
  check(X3CoopRuntimePreflight(expected,moving),"moving released door scene passes actor-state preflight");
  check(RtlLoadSnapshotFromMemory(expected,moving),"moving released door scene restores in the current build");
  if(output && *output){
    snprintf(path,sizeof(path),"%s-p%u-%s-released.sav",output,remote+1,character==X3_COOP_ZERO?"zero":"x");
    check(RtlSaveSnapshot(path),"released native gate actors visual fixture saves");
  }
  before0=support_position(0,5);
  for(unsigned i=0;i<15;++i)frame(SNES_PAD_RIGHT<<12);
  check(support_position(1,5)>before1+10 && abs((int)support_position(0,5)-(int)before0)<=2,
        "P2 resumes independent native control after transport without moving P1");
  if(getenv("X3_COOP_TRANSITION_MENU")){
    X3CoopMenuInfo panel;check(!X3CoopMenuRead(&panel) && !(g_ram[0x1f37]&1),"restored door cleanup leaves no hidden menu and both native controls released");
    free(admitted_state);check(RtlLoadSnapshotFromMemory(original,size),"transition-menu gate restores its original route");return;
  }
  /* Isolated lifecycle API events, not authored enemy deaths or a launcher
   * withdrawal handoff: native door progression/cleanup is still exercised. */
  for(unsigned mode=0;mode<8;++mode){
    check(RtlLoadSnapshotFromMemory(admitted_state,admitted),"pending gate retirement approach restores");
    if(mode>=4){
      for(unsigned i=0;i<600 && g_ram[0x1f28];++i)frame(0);
      check(!g_ram[0x1f28] && X3CoopRuntimeSceneActive() && g_ram[X3_COOP_BODY+0x6e],
            "late retirement starts in the following original miniboss introduction");
    }
    X3CoopState retired=*X3CoopRuntimeState();unsigned seat=(mode&3)<2?remote:remote^1;
    check(mode&1?X3CoopWithdraw(&retired,seat):X3CoopFall(&retired,seat),"door participant lifecycle precondition applies");
    retired.current=retired.anchor;
    size_t retirement=RtlSaveSnapshotToMemory(expected,capacity);
    check(X3CoopEncode(&retired,expected+contact_context_offset(expected,retirement),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,retirement),"retired native scene participant restores");
    door_install_player(&retired);
    for(unsigned i=0;i<900 && X3CoopRuntimeSceneActive();++i)frame(0);
    const X3CoopState *result=X3CoopRuntimeState();
    check(!X3CoopRuntimeSceneActive() && !g_ram[0x1f28] && g_ram[0xd1]==4 && g_ram[0xd2]==4,
          "retiring either door participant leaves native progression bright and unfrozen");
    check(result->players[seat].life==(mode&1?X3_COOP_ABSENT:X3_COOP_FALLEN) &&
          result->players[seat^1].life==X3_COOP_ALIVE &&
          (result->players[seat^1].body[0x27]&127)==(setup.players[seat^1].body[0x27]&127),
          "door transfer preserves survivor health and never revives a retired participant");
    check(support_position(seat^1,5)>2552 && view_read(0x1e5d)==2560,
          "surviving participant finishes the original committed native door walk");
  }
  free(admitted_state);
  for(unsigned mode=0;mode<5;++mode){
    check(RtlLoadSnapshotFromMemory(work,prepared),"native gate eligibility approach restores");
    X3CoopState candidates=*X3CoopRuntimeState();unsigned first=candidates.anchor;
    if(mode!=2 && mode!=3){
      candidates.players[first].body[5]=0xf8;candidates.players[first].body[6]=9;
      candidates.players[first].body[8]=0x97;candidates.players[first].body[9]=5;
      if(mode<2)candidates.players[first].body[2]=0x12; /* Retail admission exclusion. */
    }
    if(mode==1)candidates.players[remote].body[2]=0x12;
    if(mode==2)candidates.players[remote].body[0x27]=0;
    if(mode==3)check(X3CoopFall(&candidates,remote),"fallen gate candidate precondition applies");
    memcpy(expected,work,prepared);
    check(X3CoopEncode(&candidates,expected+contact_context_offset(expected,prepared),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,prepared),"gate candidates restore without admission results");
    memcpy(g_ram+X3_COOP_BODY,candidates.players[first].body,X3_COOP_BODY_BYTES);
    if(!mode){
      check(interp_bridge_add_pre_opcode_hook(0x01e0ae,pickup_continuation_probe),"gate predicate continuation probe registers");
      pickup_probe_slot=slot;pickup_probe_pc=0x01e0ae;pickup_probe_seat=remote;pickup_probe_seen=false;
    }
    pickup_native_items(0x00daa0);
    if(!mode)check(pickup_probe_seen,"blocked nearest candidate retries the whole gate predicate with a restorable continuation");
    bool hit=mode==0 || mode==4;
    check(hit?(g_ram[slot+1]==4 && g_ram[0x1f28]):(!g_ram[0x1f28] && (!g_ram[slot] || g_ram[slot+1]==2)),
          "native gate handles excluded, zero-HP, fallen and simultaneously touching candidates");
    check(X3CoopRuntimeState()->anchor==(mode==0?remote:first) &&
          X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,
          "gate miss restores canonical ownership; one actual entrant owns a successful gate");
  }
  for(unsigned mode=0;mode<5;++mode){
    memcpy(expected,work,prepared);expected[door_scene_chunk(expected,prepared)+10]=1;
    check(RtlLoadSnapshotFromMemory(expected,prepared),"both-present gate policy restores");
    X3CoopState candidates=*X3CoopRuntimeState();unsigned other=remote^1;
    if(mode==1){
      candidates.players[other].body[5]=0xe8;candidates.players[other].body[6]=9;
      candidates.players[other].body[8]=0x9b;candidates.players[other].body[9]=5;
    }else if(mode==2 || mode==3){
      check(mode==2?X3CoopFall(&candidates,other):X3CoopWithdraw(&candidates,other),
            "unavailable both-present prerequisite applies");
      candidates.current=candidates.anchor;
    }else if(mode==4)candidates.players[other].body[0x27]=0;
    check(X3CoopEncode(&candidates,expected+contact_context_offset(expected,prepared),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(expected,prepared),"both-present candidates restore without gate results");
    door_install_player(&candidates);pickup_native_items(0x00daa0);
    check(mode?(g_ram[slot+1]==4 && g_ram[0x1f28]):(g_ram[slot+1]==2 && !g_ram[0x1f28]),
          "both-present rule waits only for living available participants");
    if(mode==1){
      check(!(g_ram[0x1f5d]&32) && !g_ram[0x1f37],"full-campaign entrance sensor cannot queue a native Zero exchange");
      for(unsigned i=0;i<900 && X3CoopRuntimeSceneActive();++i)frame(0);
      check(!X3CoopRuntimeSceneActive() && !g_ram[0x1f28] &&
            X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,
            "both-present admission transports both native bodies through cleanup without crushing");
    }
  }
  check(RtlLoadSnapshotFromMemory(original,size),"native gate admission probes restore the starting machine");
}
