typedef struct LadderSample {uint16_t x,y,vx,vy,cx,cy,limits[4];uint8_t action,sub,hp,ground;} LadderSample;
static unsigned ladder_keys(unsigned f){return f<120?SNES_PAD_UP:f<210?SNES_PAD_DOWN:SNES_PAD_RIGHT|(f<218?SNES_PAD_B:0);}
static uint32_t ladder_inputs(unsigned seat,unsigned f,bool paired){uint32_t input=(uint32_t)ladder_keys(f)<<(paired?seat*12:0);return input|(paired && f<30?(uint32_t)SNES_PAD_UP<<((seat^1)*12):0);}
static LadderSample ladder_sample(unsigned seat,bool paired){
  const uint8_t *body=vehicle_player(seat);LadderSample s={0};s.x=body[5]|body[6]<<8;s.y=body[8]|body[9]<<8;s.vx=body[0x1a]|body[0x1b]<<8;s.vy=body[0x1c]|body[0x1d]<<8;s.action=body[2];s.sub=body[3];s.hp=body[0x27];s.ground=body[0x2b];
  if(paired){X3CoopView view=X3CoopViewsForPlayer(g_ram,X3CoopRuntimeState(),seat,false);s.cx=(uint16_t)view.x;s.cy=(uint16_t)view.y;memcpy(s.limits,X3CoopViewsState()->current[seat],sizeof(s.limits));}
  else{s.cx=(uint16_t)view_read(0x1e5d);s.cy=(uint16_t)view_read(0x1e60);for(unsigned i=0;i<4;++i)s.limits[i]=(uint16_t)view_read(0x1e66+2*i);}
  return s;
}
/* Native stage2 ladder columns come from the live authored collision map.
 * Route/camera positioning is a prerequisite, never a supplied climb result. */
static void native_ladder_checks(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshot(fixture),"native ladder route restores its current-build starting actors");
  if(g_ram[0x1fae]!=2){stage_request();g_ram[0x1fae]=2;unsigned elapsed=0;for(;elapsed<3600;++elapsed){frame(0);if(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4 && g_ram[0x1fae]==2)break;}check(elapsed<3600,"actual native loader enters the free stage2 start");for(unsigned f=0;f<120;++f)frame(0);}
  check(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0x1fae]==2 && !g_ram[0x1f3f] && !g_ram[0x1f22],"ladder route has no inherited arena or scene lock");
  const char *output=getenv("X3_COOP_LADDER_STAGE_OUTPUT");if(output && *output)check(RtlSaveSnapshot(output),"free native stage2 preparation saves directly through the test interface");
  unsigned seat=getenv("X3_COOP_LADDER_SEAT")?(unsigned)atoi(getenv("X3_COOP_LADDER_SEAT")):0;check(seat<2,"actual ladder seat is valid");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();if(getenv("X3_COOP_LADDER_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}setup.current=setup.anchor=(uint8_t)(seat^1);
  for(unsigned s=0;s<2;++s){team_hazard_body(setup.players[s].body,s?2392:1480,s?660:835);setup.players[s].body[0x88]=255;setup.players[s].body[0x8a]=0;}
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"authored ladder approaches supply only normal body positions and protection");door_install_player(&setup);
  X3CoopViewsConfigure(true);X3CoopViewState views=*X3CoopViewsState();views.independent=views.initialized=1;views.stage=2;views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=(uint16_t)(s?2264:1352);views.camera[s][1]=(uint16_t)(s?512:640);views.limits[s][0]=views.current[s][0]=0;views.limits[s][1]=views.current[s][1]=8192;views.limits[s][2]=views.current[s][2]=512;views.limits[s][3]=views.current[s][3]=768;}X3CoopViewsSetState(&views);vehicle_install_view(&views,setup.anchor);
  /* Walking through1408 normally admits all four native region sensors.
   * The teleport bypasses that frontier, so admit their real event column. */
  X3CoopViewsConfigure(false);CpuState caller=g_cpu;view_word(0,1408);view_word(2,608);view_word(4,320);g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);check(interp_bridge_run(&g_cpu,0x00ded3) && g_cpu.S==caller.S,"actual native event column admits the ladder room's region-controller prerequisites");uint64_t master=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=master;g_cpu.cycles=cycles;X3CoopViewsSetState(&views);
  unsigned sensors=0;for(unsigned d=0x1d18;d<0x1e18;d+=16)if(g_ram[d] && !g_ram[d+10] && g_ram[d+11]<4)sensors|=1u<<g_ram[d+11];check(sensors==15,"all four authored3C:D557..D56C native region sensors are present before either camera comparison");n=RtlSaveSnapshotToMemory(base,capacity);
  LadderSample native[240];
  for(unsigned paired=0;paired<2;++paired){
    check(RtlLoadSnapshotFromMemory(base,n),"native/co-op ladder sequence starts from the same complete positioned stage");
    if(!paired){X3CoopState solo=*X3CoopRuntimeState();solo.anchor=solo.current=(uint8_t)seat;memcpy(work,base,n);check(X3CoopEncode(&solo,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"original reference selects the actual tested native character");door_install_player(&solo);X3CoopViewsConfigure(false);vehicle_install_view(&views,seat);X3CoopRuntimeEnable(false);}
    unsigned start=support_position(seat,8),top=start,down=0,clock=g_ram[0x9cc],held_x=0,held_y=0;bool climbing=false,descending=false,detached=false;LadderSample before_jump={0};
    for(unsigned f=0;f<240;++f){frame(ladder_inputs(seat,f,paired!=0));LadderSample now=ladder_sample(seat,paired!=0);
      if(f<120){if(now.y<top)top=now.y;climbing|=now.action==0x2a;}
      else if(f<210){if(now.y>down)down=now.y;descending|=now.action==0x2a;}
      if(f==209)before_jump=now;if(f>=210)detached|=now.action!=0x2a && now.x>before_jump.x+8;
      if(paired){if(memcmp(&native[f],&now,sizeof(now))){fprintf(stderr,"native LADDER MISMATCH seat%u char%u f%u native%u,%u v%04x/%04x action%u/%u view%u,%u bounds%u/%u paired%u,%u v%04x/%04x action%u/%u view%u,%u bounds%u/%u\n",seat,setup.players[seat].character,f,native[f].x,native[f].y,native[f].vx,native[f].vy,native[f].action,native[f].sub,native[f].cx,native[f].cy,native[f].limits[2],native[f].limits[3],now.x,now.y,now.vx,now.vy,now.action,now.sub,now.cx,now.cy,now.limits[2],now.limits[3]);fprintf(stderr,"LADDER DETAIL native HP%u ground%02x xbounds%u/%u paired HP%u ground%02x xbounds%u/%u size%u\n",native[f].hp,native[f].ground,native[f].limits[0],native[f].limits[1],now.hp,now.ground,now.limits[0],now.limits[1],(unsigned)sizeof(now));check(false,"native ladder movement, pose, velocity and complete private camera stay identical on every frame");}
        if(f==30){held_x=support_position(seat^1,5);held_y=support_position(seat^1,8);}
      }else native[f]=now;
    }
    fprintf(stderr,"native LADDER QUALIFY paired%u seat%u char%u start%u top%u down%u climb%u descend%u detach%u viewY%u->%u\n",paired,seat,setup.players[seat].character,start,top,down,climbing,descending,detached,native[0].cy,native[119].cy);
    check(climbing && descending && top+64<start && down>top+64 && detached && contact_hp(seat),"real native Up/Down traverse the authored ladder and Right/Jump detach through its native exit");
    if(!seat)check(before_jump.action==0x2a,"the long authored ladder exercises actual Jump from mid-climb");
    check((uint8_t)(g_ram[0x9cc]-clock)==240 && native[0].cy!=native[119].cy,"native ladder travel scrolls through its authored vertical band with one world update per frame");
    if(paired){check(support_position(seat^1,5)==held_x && support_position(seat^1,8)==held_y && vehicle_player(seat^1)[2]==0x2a,"the other actual ladder rider holds its own rung under the tested actor's opposite directions and jump");size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"actual ladder future allocates");memcpy(future,work,done);check(RtlLoadSnapshotFromMemory(base,n),"actual two-rider ladder sequence restores before replay");for(unsigned f=0;f<240;++f)frame(ladder_inputs(seat,f,true));same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"both ladder actors, native room cameras and the whole machine replay identically");free(future);}
  }

}
