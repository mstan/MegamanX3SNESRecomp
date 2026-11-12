/* Use the real station and original action2C piloting path. Only route,
 * acquired Chimera and personal protection are supplied as prerequisites. */
static const uint8_t *vehicle_player(unsigned seat){
  const X3CoopState *s=X3CoopRuntimeState();return s->current==seat?g_ram+X3_COOP_BODY:s->players[seat].body;
}
static void vehicle_place(unsigned x,unsigned xx,unsigned xy,unsigned zx,unsigned zy,uint8_t *base,uint8_t *work,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
  team_hazard_body(setup.players[x].body,xx,xy);team_hazard_body(setup.players[x^1].body,zx,zy);
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"empty-armor contact approach supplies only normal actor positions/velocity");door_install_player(&setup);
  X3CoopViewState views=*X3CoopViewsState();views.valid=views.camera_valid=3;
  for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=1536;views.camera[seat][1]=256;views.limits[seat][0]=views.current[seat][0]=1280;views.limits[seat][1]=views.current[seat][1]=2048;views.limits[seat][2]=views.current[seat][2]=256;views.limits[seat][3]=views.current[seat][3]=512;}X3CoopViewsSetState(&views);
  view_word(0x1e5d,1536);view_word(0x1e7a,1536);view_word(0x1e60,256);view_word(0x1e7c,256);
  const unsigned limits[]={0x1e66,0x1e68,0x1e6a,0x1e6c,0x1e6e,0x1e70,0x1e78,0x1e7e},upper[]={1280,2048,256,512,1280,2048,256,512};for(unsigned i=0;i<8;++i)view_word(limits[i],upper[i]);
}
static void vehicle_extended(unsigned x,bool paired,uint8_t *base,uint8_t *work,size_t capacity){
  size_t cut=RtlSaveSnapshotToMemory(base,capacity);unsigned other=support_position(x^1,5);
  uint32_t right=(uint32_t)SNES_PAD_RIGHT<<(paired?x*12:0);
  for(unsigned j=0;j<30;++j)frame(right);
  uint8_t armor[80];memcpy(armor,g_ram+0xcc8,sizeof(armor));
  check(RtlLoadSnapshotFromMemory(base,cut),"actual piloting cut restores");
  uint32_t both=right|(paired?(uint32_t)SNES_PAD_LEFT<<((x^1)*12):0);
  for(unsigned j=0;j<30;++j)frame(both);
  check(!memcmp(armor,g_ram+0xcc8,sizeof(armor)),"the free actor's opposite input cannot steer or duplicate the shared armor update");
  if(paired)check(support_position(x^1,5)<other && vehicle_player(x^1)[2]!=0x2c,"the free actor really moves independently and never becomes a second pilot");
  size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"vehicle future allocates");memcpy(future,work,done);
  check(RtlLoadSnapshotFromMemory(base,cut),"active pilot restores for full replay");
  for(unsigned j=0;j<30;++j)frame(both);
  same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"actual paired piloting and free movement replay the whole machine");free(future);
  bool exited=false;
  for(unsigned j=0;j<90;++j){unsigned keys=SNES_PAD_RIGHT|(j<8?SNES_PAD_UP|SNES_PAD_B:0);frame((uint32_t)keys<<(paired?x*12:0));
    if(vehicle_player(x)[2]!=0x2c && !(g_ram[0xcd3]&64)){exited=true;fprintf(stderr,"native vehicle EXIT EMPTY paired%u frame%u X%u,%u action%u armor%u,%u flags%u\n",paired,j,support_position(x,5),support_position(x,8),vehicle_player(x)[2],view_read(0xccd),view_read(0xcd0),g_ram[0xcd3]);break;}
    if(j%15==0)fprintf(stderr,"native vehicle EXIT paired%u f%u X%u,%u action%u armor%u,%u state%u/%u flags%u\n",paired,j,support_position(x,5),support_position(x,8),vehicle_player(x)[2],view_read(0xccd),view_read(0xcd0),g_ram[0xcc9],g_ram[0xcca],g_ram[0xcd3]);
  }
  check(exited && g_ram[0xcc8] && !(g_ram[0xcd3]&64) && vehicle_player(x)[2]!=0x2c,"real Up and jump leave a live empty armor without retaining a pilot");
  if(!paired)return;
  unsigned ax=view_read(0xccd),ay=view_read(0xcd0);
  vehicle_place(x,ax+112,ay+19,ax,ay-80,base,work,capacity);
  for(unsigned j=0;j<90;++j)frame(0);
  fprintf(stderr,"native vehicle ZERO CONTACT X%u,%u action%u Zero%u,%u action%u armor%u,%u flags%u\n",support_position(x,5),support_position(x,8),vehicle_player(x)[2],support_position(x^1,5),support_position(x^1,8),vehicle_player(x^1)[2],view_read(0xccd),view_read(0xcd0),g_ram[0xcd3]);
  check(g_ram[0xcc8] && !(g_ram[0xcd3]&64) && vehicle_player(x)[2]!=0x2c && vehicle_player(x^1)[2]!=0x2c,"Zero's real falling/contact cannot borrow the distant X's native boarding eligibility");
  ax=view_read(0xccd);ay=view_read(0xcd0);
  vehicle_place(x,ax,ay-80,ax+112,ay+19,base,work,capacity);
  bool reboarded=false;
  for(unsigned j=0;j<120;++j){frame(0);if(vehicle_player(x)[2]==0x2c && (g_ram[0xcd3]&64)){reboarded=true;break;}}
  fprintf(stderr,"native vehicle REBOARD X%u,%u action%u Zero%u,%u action%u armor%u,%u flags%u\n",support_position(x,5),support_position(x,8),vehicle_player(x)[2],support_position(x^1,5),support_position(x^1,8),vehicle_player(x^1)[2],view_read(0xccd),view_read(0xcd0),g_ram[0xcd3]);
  check(reboarded && vehicle_player(x^1)[2]!=0x2c,"X's actual falling/contact reboards the same empty armor with only one native pilot");
}
static unsigned vehicle_trace_count;
static void vehicle_trace(CpuState *cpu,uint32_t pc){
  if(cpu->D!=0xcc8 || vehicle_trace_count>=24)return;
  static unsigned previous;
  if(!g_ram[0xcc8] && !previous)return;
  previous=g_ram[0xcc8];++vehicle_trace_count;
  const X3CoopState *s=X3CoopRuntimeState();unsigned x=s->players[0].character==X3_COOP_X?0:1;
  X3CoopView v=X3CoopViewsForPlayer(g_ram,s,x,false);
  fprintf(stderr,"vehicle callback %06x RA%u/%u/%u class%u flags%u at%u,%u current%u Xview%d,%d freeze%u\n",pc,g_ram[0xcc8],g_ram[0xcc9],g_ram[0xcca],g_ram[0xcd2],g_ram[0xcd3],view_read(0xccd),view_read(0xcd0),s->current,v.x,v.y,g_ram[0x1f22]);
}
static void native_vehicle_reference(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==1,"vehicle reference restores its real station room");
  unsigned station=0;for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x2f && view_read(d+5)==1712 && view_read(d+8)==432)station=d;
  check(station!=0,"vehicle uses the actual authored station at1712,432");
  bool paired=getenv("X3_COOP_VEHICLE_PAIRED")!=NULL,summoned=false,boarded=false;
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
  if(getenv("X3_COOP_VEHICLE_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  unsigned x=setup.players[0].character==X3_COOP_X?0:1;
  setup.current=setup.anchor=(uint8_t)(paired?x^1:x);
  team_hazard_body(setup.players[x].body,1712,400);setup.players[x].body[0x88]=255;
  if(paired){team_hazard_body(setup.players[x^1].body,384,1175);setup.players[x^1].body[0x88]=255;}
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"native X receives only position/protection and real roster prerequisites");
  door_install_player(&setup);X3CoopRuntimeEnable(paired);g_ram[0x1fd7]|=1;
  view_word(0x1e5d,paired?256:1532);view_word(0x1e7a,paired?256:1532);view_word(0x1e60,paired?1024:304);view_word(0x1e7c,paired?1024:304);
  const unsigned limits[]={0x1e66,0x1e68,0x1e6a,0x1e6c,0x1e6e,0x1e70,0x1e78,0x1e7e};
  const unsigned upper[]={1280,2048,256,512,1280,2048,256,512},lower[]={0,1024,1024,1024,0,1024,1024,1024};
  for(unsigned i=0;i<8;++i)view_word(limits[i],paired?lower[i]:upper[i]);
  X3CoopViewsConfigure(paired);
  if(paired){X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){bool u=seat==x;views.camera[seat][0]=u?1532:256;views.camera[seat][1]=u?304:1024;
      views.limits[seat][0]=views.current[seat][0]=u?1280:0;views.limits[seat][1]=views.current[seat][1]=u?2048:1024;
      views.limits[seat][2]=views.current[seat][2]=u?256:1024;views.limits[seat][3]=views.current[seat][3]=u?512:1024;}X3CoopViewsSetState(&views);}
  if(getenv("X3_COOP_VEHICLE_TRACE")){interp_bridge_add_pre_opcode_hook(0x00da48,vehicle_trace);interp_bridge_add_pre_opcode_hook(0x00da4b,vehicle_trace);interp_bridge_add_pre_opcode_hook(0x00da6b,vehicle_trace);interp_bridge_add_pre_opcode_hook(0x00da6e,vehicle_trace);}
  for(unsigned f=0;f<540;++f){
    unsigned keys=f>=30 && f<90?SNES_PAD_DOWN:f>=90 && f%24<8?(f<300?SNES_PAD_Y:f<420?SNES_PAD_B:SNES_PAD_A):0;
    frame((uint32_t)keys<<(paired?x*12:0));const uint8_t *player=vehicle_player(x);
    if(f%60==0)fprintf(stderr,"native vehicle paired%u seat%u f%u X%u,%u action%u/%u station%u/%u/%u HP%u menu%u RA%u/%u/%u freeze%u/%u/%u/%u\n",paired,x,f,support_position(x,5),support_position(x,8),player[2],player[3],g_ram[station+1],g_ram[station+2],g_ram[station+3],g_ram[station+0x27],g_ram[0x1f3f],g_ram[0xcc8],g_ram[0xcc9],g_ram[0xccb],g_ram[0x1f22],g_ram[0x1f25],g_ram[0x1f27],g_ram[0x1f2b]);
    if(paired && f==180 && !g_ram[station+2])break;
    if(g_ram[0xcc8])summoned=true;
    if(g_ram[0xcc8] && player[2]==0x2c && !g_ram[0x1f3f]){
      boarded=true;unsigned before=view_read(0xccd),other=support_position(x^1,5);
      for(unsigned j=0;j<30;++j)frame((uint32_t)SNES_PAD_RIGHT<<(paired?x*12:0));
      fprintf(stderr,"native vehicle BOARDED paired%u seat%u f%u armorX%u->%u player%u,%u action%u other%u->%u RA%u/%u/%u\n",paired,x,f,before,view_read(0xccd),support_position(x,5),support_position(x,8),vehicle_player(x)[2],other,support_position(x^1,5),g_ram[0xcc8],g_ram[0xcc9],g_ram[0xccb]);
      check(view_read(0xccd)>before && vehicle_player(x)[2]==0x2c,"native riding action moves the actual armor under its owner's real Right input");
      if(paired)check(support_position(x^1,5)==other,"the other actor's neutral input does not move the actual pilot or vehicle");
      const char *output=getenv("X3_COOP_VEHICLE_OUTPUT");if(output && *output)check(RtlSaveSnapshot(output),"actual current-build native pilot fixture saves through the test interface");
      if(getenv("X3_COOP_VEHICLE_EXTENDED"))vehicle_extended(x,paired,base,work,capacity);break;
    }
  }
  fprintf(stderr,"native vehicle reference paired%u summoned%u boarded%u\n",paired,summoned,boarded);
  check(summoned && boarded,"real station interaction summons the armor and native contact earns boarding");
}
