/* Reload the genuinely earned stage1 heart through native constructors, then
 * use actual P2 movement to visit, retire and backtrack to the floor enemy. */
static void native_left_activation(const char *normal,const char *reversed,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *seed=malloc(capacity),*future=malloc(capacity);check(seed && future,"native left-route full-future buffers allocate");
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==1 && (g_ram[0x1fd4]&1),"left route loads an actually completed native heart acquisition");
    unsigned earned=g_ram[0x1fd4];
    for(unsigned transition=0;transition<2;++transition){
      unsigned stage=transition?1:5;X3CoopViewsConfigure(false);stage_request();g_ram[0x1fae]=(uint8_t)stage;bool pending=false;unsigned arrived=0;
      for(;arrived<1800;++arrived){frame(0);pending|=X3CoopRuntimeSceneActive();if(pending && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0x1fae]==stage && g_ram[0xd1]==4 && g_ram[0xd2]==4 && !g_ram[0x1f22] && !g_ram[0x1f24] && !g_ram[0x1f4f])break;}
      fprintf(stderr,"native LEFT entry roster%u transition%u requested%u arrived%u pending%u stage%u mode%u/%u ready%u scene%u heart%02x->%02x locks%u/%u/%u\n",roster,transition,stage,arrived,pending,g_ram[0x1fae],g_ram[0xd1],g_ram[0xd2],X3CoopRuntimeReady(),X3CoopRuntimeSceneActive(),earned,g_ram[0x1fd4],g_ram[0x1f22],g_ram[0x1f24],g_ram[0x1f4f]);
      check(arrived<1800 && g_ram[0x1fd4]==earned,"native stage entry rebuilds playable terrain while retaining the actually earned heart");
    }
    for(unsigned f=0;f<120;++f)frame(0);
    size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);setup.current=setup.anchor=0;
    for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x88]=255;
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"left route selects its waiting world leader and supplies only personal contact protection");door_install_player(&setup);X3CoopViewsConfigure(true);X3CoopViewsCommitNative(g_ram,X3CoopRuntimeState());
    unsigned approached=0;bool visited=false;
    for(;approached<720 && support_position(1,5)<1000;++approached){
      frame((uint32_t)(SNES_PAD_RIGHT|(approached%32<12?SNES_PAD_B:0)|(approached%64<32?SNES_PAD_A:0))<<12);
      for(unsigned slot=0xd18;slot<0x10d8;slot+=64)if(g_ram[slot] && g_ram[slot+10]==8 && g_ram[slot+11]==1 && view_read(slot+5)==736 && g_ram[slot+1]==2 && (g_ram[slot+0x27]&127))visited=true;
    }
    check(approached<720 && visited,"actual P2 right movement first visits the authored enemy before backtracking");
    n=RtlSaveSnapshotToMemory(work,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);team_hazard_body(setup.players[1].body,support_position(1,5),support_position(1,8));
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"backtracking preparation stops existing movement without changing position, terrain or eligibility");door_install_player(&setup);
    for(unsigned f=0;f<160;++f)frame(0);
    fprintf(stderr,"native LEFT preparation roster%u approach%u P1%u,%u P2%u,%u camera%u limits%u/%u flag%u enemy%u\n",roster,approached,support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8),X3CoopViewsState()->camera[1][0],X3CoopViewsState()->current[1][0],X3CoopViewsState()->current[1][1],g_ram[0xfa16],view_has_enemy(736,1136));
    check(!g_ram[0xfa16] && !view_has_enemy(736,1136) && X3CoopViewsState()->camera[1][0]>800 && X3CoopViewsState()->current[1][0]<736,"native culling makes the previously visited enemy eligible beyond P2's current left frontier");
    n=RtlSaveSnapshotToMemory(seed,capacity);size_t future_size=0;
    for(unsigned replay=0;replay<2;++replay){
      check(RtlLoadSnapshotFromMemory(seed,n),"native left approach restores its unchanged consumed-event world");
      unsigned still=support_position(0,5),start=support_position(1,5),camera=X3CoopViewsState()->camera[1][0],clock=g_ram[0x9cc],first=0,frames=0,hp=0;bool initialized=false;
      for(unsigned f=0;f<320;++f){
        frame((uint32_t)(SNES_PAD_LEFT|(f%32<12?SNES_PAD_B:0)|(f%64<32?SNES_PAD_A:0))<<12);++frames;
        unsigned copies=0;for(unsigned slot=0xd18;slot<0x10d8;slot+=64)if(g_ram[slot] && g_ram[slot+10]==8 && g_ram[slot+11]==1 && view_read(slot+5)==736){
          ++copies;if(!first)first=frames;hp=g_ram[slot+0x27]&127;initialized|=g_ram[slot+1]==2 && hp>0;
        }
        if(copies>1)check(0,"a left frontier cannot duplicate its actual native enemy");
        if(!((g_ram[0x1fd4]&1) && !g_ram[0x1f4f] && !g_ram[0x1f45] && !X3CoopRuntimeSceneActive()))check(0,"P2 leading left cannot replay the consumed heart acquisition");
        if(first && initialized && frames>=first+4 && support_position(1,5)<950)break;
      }
      fprintf(stderr,"native LEFT admission roster%u replay%u first%u frames%u P1%u/%u P2%u->%u camera%u->%u enemyHP%u initialized%u clock%u heart%02x HP%u/%u\n",roster,replay,first,frames,still,support_position(0,5),start,support_position(1,5),camera,X3CoopViewsState()->camera[1][0],hp,initialized,(uint8_t)(g_ram[0x9cc]-clock),g_ram[0x1fd4],contact_hp(0),contact_hp(1));
      check(first>1 && initialized && hp && g_ram[0xfa16] && support_position(0,5)==still && support_position(1,5)<start && X3CoopViewsState()->camera[1][0]<camera && (uint8_t)(g_ram[0x9cc]-clock)==(uint8_t)frames,"actual P2 left controls admit and update the eligible native enemy with stationary P1 and one shared world tick");
      for(unsigned seat=0;seat<2;++seat)check(X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE && contact_hp(seat)>0,"both real characters remain living through the left-frontier approach");
      size_t actual=RtlSaveSnapshotToMemory(expected,capacity);if(!replay){memcpy(future,expected,actual);future_size=actual;native_permanent_streaming(11,5856,1096,1,work,expected,capacity);}else same(future,future_size,expected,actual,"native P2 left movement and authored enemy admission repeat the entire consumed-event machine");
    }
  }
  free(seed);free(future);
}
