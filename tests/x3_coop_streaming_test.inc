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
