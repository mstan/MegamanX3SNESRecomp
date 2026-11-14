/* Earn the item first. Position/camera assistance skips travel between regions;
 * native camera entry, culling, event admission and item updates own the result. */
static void streaming_position(unsigned px,unsigned py,unsigned leader,uint8_t *work,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();
  X3CoopCapture(&setup,g_ram);setup.current=setup.anchor=(uint8_t)leader;
  for(unsigned seat=0;seat<2;++seat){
    uint8_t *b=setup.players[seat].body;
    campaign_position_only(&setup.players[seat],px+seat*32,py-(setup.players[seat].character==X3_COOP_ZERO?8:0));
    b[2]=b[3]=b[4]=b[7]=b[0x6e]=0;memset(b+0x1a,0,6);b[0x88]=255;
  }
  check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"streaming visit prepares only positions, normal poses and contact protection");
  door_install_player(&setup);X3CoopViewsConfigure(true);
  X3CoopViewState views=*X3CoopViewsState();views.valid=3;views.camera_valid=0;
  views.initialized=1;views.stage=g_ram[0x1fae];
  for(unsigned seat=0;seat<2;++seat){
    views.camera[seat][0]=(uint16_t)(px+seat*32-128);views.camera[seat][1]=(uint16_t)(py&~255u);
    for(unsigned axis=0;axis<2;++axis){views.limits[seat][axis*2]=views.current[seat][axis*2]=0;views.limits[seat][axis*2+1]=views.current[seat][axis*2+1]=8192;}
  }
  X3CoopViewsSetState(&views);vehicle_install_view(&views,leader);
  for(unsigned tick=0;tick<4;++tick)frame(0);
  check(!X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4 && !g_ram[0x1f28],"native streaming visit remains active gameplay without a new reward freeze");
}
static void native_permanent_streaming(unsigned kind,unsigned px,unsigned py,unsigned variant,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *seed=malloc(capacity),*future=malloc(capacity);check(seed && future,"earned-item streaming replay buffers allocate");
  size_t n=RtlSaveSnapshotToMemory(seed,capacity),future_size=0;
  unsigned flag=kind==11?0x1fd4:0x1fd1;uint8_t rewards[3]={g_ram[0x1fd1],g_ram[0x1fd3],g_ram[0x1fd4]},tanks[4];memcpy(tanks,g_ram+0x1fb7,4);
  unsigned hp[2]={contact_hp(0),contact_hp(1)},maximum[2]={X3CoopRuntimeState()->players[0].maximum_health,X3CoopRuntimeState()->players[1].maximum_health};
  check((g_ram[flag]&variant) && !g_ram[0x1f28],"streaming begins with the actually completed native permanent award");
  for(unsigned replay=0;replay<2;++replay){
    check(RtlLoadSnapshotFromMemory(seed,n),"streaming restores the same earned native item and actor capacities");
    for(unsigned leader=0;leader<2;++leader)for(unsigned visiting=0;visiting<2;++visiting){
      streaming_position(visiting?px-96:px-608,py-40,leader,work,capacity);
      bool contains=X3CoopViewsContains(g_ram,X3CoopRuntimeState(),px,py,0,256,0,224,0,false);
      check(contains==(bool)visiting,"both actual private views leave and re-enter the collected item's authored region");
      for(unsigned slot=0x1518;slot<0x1818;slot+=48)
        check(!(g_ram[slot] && g_ram[slot+10]==kind && view_read(slot+5)==px && abs((int)view_read(slot+8)-(int)py)<16),"native streaming cannot recreate the collected authored item");
      check(g_ram[0x1fd1]==rewards[0] && g_ram[0x1fd3]==rewards[1] && g_ram[0x1fd4]==rewards[2] && !memcmp(tanks,g_ram+0x1fb7,4),"alternating region ownership preserves earned campaign flags and tank reserves without a second award");
      for(unsigned seat=0;seat<2;++seat)check(X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE && contact_hp(seat)==hp[seat] && X3CoopRuntimeState()->players[seat].maximum_health==maximum[seat],"streaming preserves each living actor's original HP and earned capacity");
      fprintf(stderr,"native permanent streaming kind%u roster%u replay%u leader%u visiting%u cameras%u,%u/%u,%u reward%02x HP%u/%u maximum%u/%u\n",kind,X3CoopRuntimeState()->players[0].character,replay,leader,visiting,X3CoopViewsState()->camera[0][0],X3CoopViewsState()->camera[0][1],X3CoopViewsState()->camera[1][0],X3CoopViewsState()->camera[1][1],g_ram[flag],contact_hp(0),contact_hp(1),X3CoopRuntimeState()->players[0].maximum_health,X3CoopRuntimeState()->players[1].maximum_health);
    }
    size_t actual=RtlSaveSnapshotToMemory(expected,capacity);
    if(!replay){memcpy(future,expected,actual);future_size=actual;}
    else same(future,future_size,expected,actual,"alternating native streaming visits repeat the entire earned-item machine future");
  }
  check(RtlLoadSnapshotFromMemory(seed,n),"streaming probes restore the actual completed native acquisition");free(seed);free(future);
}
static void native_reward_retry_prepare(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *expected,size_t capacity){
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==1 && (g_ram[0x1fd4]&1),"retry preparation loads the actually earned current-build heart");
    unsigned x=X3CoopRuntimeState()->players[0].character==X3_COOP_X?0:1;
    check(X3CoopRuntimeState()->players[x].maximum_health==18,"earned heart supplies native X's real increased capacity");
    X3CoopViewsConfigure(false);stage_request();bool pending=false;unsigned frames=0;
    for(;frames<1800;++frames){
      frame(0);pending|=X3CoopRuntimeSceneActive();
      if(pending && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0x1fae]==5 && g_ram[0xd1]==4 && g_ram[0xd2]==4 && !g_ram[0x1f22] && !g_ram[0x1f24] && !g_ram[0x1f4f])break;
    }
    check(frames<1800 && (g_ram[0x1fd4]&1) && X3CoopRuntimeState()->players[x].maximum_health==18,"actual native stage constructor retains the earned heart without supplying any reward flag");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);
    native_persistent_pickup_checks(base,n,work,expected,capacity,5,2944,1912,0x20,false);
    fprintf(stderr,"native reward retry preparation roster%u stage5arrival%u heart%02x maximum%u\n",roster,frames,g_ram[0x1fd4],X3CoopRuntimeState()->players[x].maximum_health);
  }
}
static void native_one_shot_revisit(uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *seed=malloc(capacity),*future=malloc(capacity);check(seed && future,"one-shot native event replay buffers allocate");
  unsigned px=capsule_x(),py=capsule_y(),flags=capsule_flags(),mask=capsule_mask();unsigned owned_mask=mask&0xf0?mask<<8:mask;
  check((flags&owned_mask)==owned_mask && !X3CoopRuntimeSceneActive(),"one-shot revisit starts after the actual native upgrade, recorded demonstration and scene release");
  size_t n=RtlSaveSnapshotToMemory(seed,capacity),future_size=0;unsigned x=X3CoopRuntimeState()->players[0].character==X3_COOP_X?0:1;
  unsigned hp[2]={contact_hp(0),contact_hp(1)},maximum[2]={X3CoopRuntimeState()->players[0].maximum_health,X3CoopRuntimeState()->players[1].maximum_health};
  for(unsigned replay=0;replay<2;++replay){
    check(RtlLoadSnapshotFromMemory(seed,n),"one-shot revisit restores the same earned capsule scene");
    for(unsigned leader=0;leader<2;++leader)for(unsigned visiting=0;visiting<2;++visiting){
      streaming_position(visiting?px-16:px-528,py-16,leader,work,capacity);
      bool contains=X3CoopViewsContains(g_ram,X3CoopRuntimeState(),px,py,0,256,0,224,0,false);check(contains==(bool)visiting,"one-shot event frontier leaves and re-enters through each actual view owner");
      unsigned capsules=0;for(unsigned slot=0xd18;slot<0x10d8;slot+=64)if(g_ram[slot] && g_ram[slot+10]==0x4d && view_read(slot+5)==px && view_read(slot+8)==py){
        ++capsules;check(g_ram[slot+2]==8 && g_ram[slot+3]>=16,"a retained native capsule can only remain in its completed one-shot state");
      }
      check(capsules<=1 && !g_ram[0x1f4f] && !g_ram[0x1f45] && !g_ram[0x1f63] && capsule_flags()==flags,"alternating frontiers cannot duplicate the capsule, restart dialogue or replay its native upgrade");
      if(visiting)check(abs((int)support_position(x,5)-(int)px)<32 && abs((int)support_position(x,8)-(int)py)<32,"native X actually revisits the capsule opening rectangle after earning its upgrade");
      for(unsigned seat=0;seat<2;++seat)check(X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE && contact_hp(seat)==hp[seat] && X3CoopRuntimeState()->players[seat].maximum_health==maximum[seat],"one-shot revisits preserve both living native characters and personal resources");
      fprintf(stderr,"native one-shot revisit stage%u roster%u replay%u leader%u visiting%u count%u flags%04x mask%02x X%u,%u native%u/%u/%u\n",g_ram[0x1fae],X3CoopRuntimeState()->players[0].character,replay,leader,visiting,capsules,capsule_flags(),mask,support_position(x,5),support_position(x,8),g_ram[0x1f4f],g_ram[0x1f45],g_ram[0x1f63]);
    }
    size_t actual=RtlSaveSnapshotToMemory(expected,capacity);if(!replay){memcpy(future,expected,actual);future_size=actual;}else same(future,future_size,expected,actual,"native one-shot event ownership and frontier revisits repeat the entire machine");
  }
  check(RtlLoadSnapshotFromMemory(seed,n),"one-shot probes restore the genuinely completed capsule");free(seed);free(future);
}
