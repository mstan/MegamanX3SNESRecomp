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
  check(RtlLoadSnapshotFromMemory(base,cut),"actual piloting cut restores");X3CoopRuntimeEnable(paired);
  uint32_t both=right|(paired?(uint32_t)SNES_PAD_LEFT<<((x^1)*12):0);
  for(unsigned j=0;j<30;++j)frame(both);
  check(!memcmp(armor,g_ram+0xcc8,sizeof(armor)),"the free actor's opposite input cannot steer or duplicate the shared armor update");
  if(paired)check(support_position(x^1,5)<other && vehicle_player(x^1)[2]!=0x2c,"the free actor really moves independently and never becomes a second pilot");
  size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"vehicle future allocates");memcpy(future,work,done);
  check(RtlLoadSnapshotFromMemory(base,cut),"active pilot restores for full replay");X3CoopRuntimeEnable(paired);
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
static void vehicle_capture(bool paired,uint8_t *work,const char *phase){
  const char *output=getenv("X3_COOP_VEHICLE_IMAGE");if(!output || !*output)return;
  if(paired){const X3CoopPresentedFrame *v=X3CoopPresentationVisible();check(v->views[0][0]!=v->views[1][0] || v->views[0][1]!=v->views[1][1],"native playable armor HUD mode retains both distinct actual views");}
  memcpy(work,g_ram,0x20000);uint64_t clock=g_cpu.master_cycles;
  for(unsigned seat=0;seat<(paired?2u:1u);++seat){
    X3Display_SetPresentationSeat(paired?(int)seat:-1);CaptureSimulationFrame(1);
    if(paired)check(X3Display_RenderedPrivateFrame(),"actual pilot image uses the private compositor");
    char path[1024];snprintf(path,sizeof(path),"%s-%s-seat%u.ppm",output,phase,seat);
    WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);
  }
  X3Display_SetPresentationSeat(-1);
  check(clock==g_cpu.master_cycles && !memcmp(work,g_ram,0x20000),"actual vehicle image leaves the shared simulation unchanged");
}
static unsigned vehicle_action_buttons(unsigned phase,unsigned frame_index,unsigned variant){
  return phase?(frame_index>=12 && frame_index<60?SNES_PAD_Y:0):(frame_index<(variant>=2?60u:8u)?SNES_PAD_B:0);
}
static void vehicle_actions(unsigned x,bool paired,unsigned variant,uint8_t *base,uint8_t *work,size_t capacity){
  bool landed=false;for(unsigned j=0;j<180;++j){frame(0);if(((g_ram[0xcf3]|g_ram[0xcf4])&4) && g_ram[0xcca]==2 && g_ram[0xccb]==1){landed=true;break;}}
  check(landed && vehicle_player(x)[2]==0x2c,"actual vehicle settles on native ground before jump/fire controls");
  size_t cut=RtlSaveSnapshotToMemory(base,capacity);
  for(unsigned phase=0;phase<2;++phase){
    check(RtlLoadSnapshotFromMemory(base,cut),"vehicle action starts from the restored actual pilot");X3CoopRuntimeEnable(paired);
    unsigned before=view_read(0xcd0),minimum=before,missiles=0;bool attack=false,timely=true;
    for(unsigned j=0;j<90;++j){frame((uint32_t)vehicle_action_buttons(phase,j,variant)<<(paired?x*12:0));unsigned y=view_read(0xcd0);if(y<minimum)minimum=y;
      unsigned expected=X3CoopActions(vehicle_action_buttons(phase,j,variant),g_ram+0xffc0);timely&=view_read(0xcfe)==expected;
      if(variant==3 && !phase && j<48 && !(j%8))fprintf(stderr,"native Frog jump detail paired%u f%u action%u/%u xy%u,%u vy%04x flags%02x input%04x/%04x Xinput%04x/%04x\n",paired,j,g_ram[0xcca],g_ram[0xccb],view_read(0xccd),y,view_read(0xce4),g_ram[0xd14],view_read(0xcfe),view_read(0xd02),vehicle_player(x)[0x36]|vehicle_player(x)[0x37]<<8,vehicle_player(x)[0x3a]|vehicle_player(x)[0x3b]<<8);
      unsigned action=g_ram[0xcca];attack|=variant<2?(action==6 || action==12):action==28;if(g_ram[0xd15]>missiles)missiles=g_ram[0xd15];}
    fprintf(stderr,"native vehicle ACTION paired%u variant%u phase%u y%u->min%u attack%u missiles%u armor%u action%u pilot%u\n",paired,variant,phase,before,minimum,attack,missiles,g_ram[0xcc8],g_ram[0xcca],vehicle_player(x)[2]);
    check(timely,"native vehicle sees its pilot's current-frame physical port without an added frame of latency");
    check(g_ram[0xcc8] && vehicle_player(x)[2]==0x2c,"jump/fire retains the one original pilot and vehicle");
    if(!phase)check(minimum<before,"real native Jump raises the piloted armor");
    else check(attack && (variant<2 || missiles>=2),"real native Fire enters the original punch or allocates the paired vehicle missiles");
    size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"vehicle action future allocates");memcpy(future,work,done);
    vehicle_capture(paired,work,phase?"fire":"jump");
    check(RtlLoadSnapshotFromMemory(base,cut),"vehicle action restores for full replay");X3CoopRuntimeEnable(paired);
    for(unsigned j=0;j<90;++j)frame((uint32_t)vehicle_action_buttons(phase,j,variant)<<(paired?x*12:0));
    same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"native jump/fire replays the complete machine");free(future);
  }
  check(RtlLoadSnapshotFromMemory(base,cut),"qualified vehicle controls restore the active native pilot");X3CoopRuntimeEnable(paired);
}
static void vehicle_damage(unsigned x,bool paired,uint8_t *base,uint8_t *work,size_t capacity){
  /* Admit the original3C:D1F4 class08 at1840,1056, initialize it natively,
   * then teleport that living authored foe to the armor for actual contact. */
  uint8_t camera[64];memcpy(camera,g_ram+0x1e58,64);X3CoopViewState views=*X3CoopViewsState();CpuState caller=g_cpu;
  X3CoopViewsConfigure(false);view_word(0x1e5d,1824);view_word(0x1e7a,1856);view_word(0x1e60,1024);view_word(0x1e7c,1024);
  view_native_call(0x00de3a,0,false);
  uint16_t stack=g_cpu.S;g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x00dcc0) && g_cpu.S==stack,"original enemy constructor runs with its actual DB06 table bank and balanced stack");
  unsigned foe=0;for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==8 && (g_ram[d+0x27]&127))foe=d;
  memcpy(g_ram+0x1e58,camera,64);X3CoopViewsSetState(&views);
  uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
  if(!foe)for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d])fprintf(stderr,"vehicle foe precondition slot%04x type%u state%u/%u HP%u xy%u,%u flag%u\n",d,g_ram[d+10],g_ram[d+1],g_ram[d+2],g_ram[d+0x27],view_read(d+5),view_read(d+8),g_ram[0xfa66]);
  check(foe!=0,"actual authored class08 initializes from its original stage1 column");
  unsigned hp=g_ram[0xcef]&127,damage=g_ram[foe+0x26],xhp=contact_hp(x),zhp=contact_hp(x^1);
  view_word(foe+5,view_read(0xccd));view_word(foe+8,view_read(0xcd0));
  fprintf(stderr,"native vehicle DAMAGE READY paired%u variant%u foe%04x state%u/%u hp%u damage%u armorHP%u\n",paired,g_ram[0xd13],foe,g_ram[foe+1],g_ram[foe+2],g_ram[foe+0x27],damage,hp);
  size_t cut=RtlSaveSnapshotToMemory(base,capacity);check(RtlLoadSnapshotFromMemory(base,cut),"both contact executions start from the identical restored native preparation");X3CoopRuntimeEnable(paired);unsigned hit=0;
  for(unsigned j=0;j<60;++j){frame(0);if((g_ram[0xcef]&127)<hp){hit=j+1;break;}}
  fprintf(stderr,"native vehicle DAMAGE paired%u variant%u hit%u armorHP%u->%u XHP%u->%u ZeroHP%u->%u cooldown%u\n",paired,g_ram[0xd13],hit,hp,g_ram[0xcef]&127,xhp,contact_hp(x),zhp,contact_hp(x^1),g_ram[0xcf8]);
  check(hit && damage && damage<128 && (g_ram[0xcef]&127)==hp-damage,"one actual native enemy contact subtracts the original damage once from armor");
  check(contact_hp(x)==xhp && (!paired || contact_hp(x^1)==zhp),"armor absorbs the hit without changing either personal health");
  for(unsigned j=0;j<3;++j)frame(0);
  size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"native vehicle damage future allocates");memcpy(future,work,done);vehicle_capture(paired,work,"hurt");
  check(RtlLoadSnapshotFromMemory(base,cut),"pre-contact native armor restores");X3CoopRuntimeEnable(paired);for(unsigned j=0;j<hit+3;++j)frame(0);
  same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"actual armor contact replays the complete machine");free(future);
  if(getenv("X3_COOP_VEHICLE_WEAPON_HIT")){
    check(RtlLoadSnapshotFromMemory(base,cut),"vehicle weapon target restores the authored live enemy");X3CoopRuntimeEnable(paired);
    view_word(foe+5,view_read(0xccd)-56);view_word(foe+8,view_read(0xcd0)-16);
    size_t prepared=RtlSaveSnapshotToMemory(work,capacity);uint8_t *start=malloc(prepared);check(start!=NULL,"vehicle weapon approach allocates");memcpy(start,work,prepared);
    unsigned health=g_ram[foe+0x27]&127,neutral=health;
    check(RtlLoadSnapshotFromMemory(start,prepared),"no-fire control restores the same actual target");X3CoopRuntimeEnable(paired);
    for(unsigned j=0;j<120;++j){frame(0);unsigned hp=g_ram[foe] && g_ram[foe+10]==8?g_ram[foe+0x27]&127:0;if(hp<neutral)neutral=hp;}
    check(neutral==health,"stationary armor without Fire does not damage this actual weapon target");
    check(RtlLoadSnapshotFromMemory(start,prepared),"real vehicle Fire restores the identical live target");X3CoopRuntimeEnable(paired);
    unsigned minimum=health;uint8_t samples[120];
    for(unsigned j=0;j<120;++j){frame((uint32_t)(j<48?SNES_PAD_Y:0)<<(paired?x*12:0));unsigned hp=g_ram[foe] && g_ram[foe+10]==8?g_ram[foe+0x27]&127:0;samples[j]=(uint8_t)hp;if(hp<minimum)minimum=hp;}
    fprintf(stderr,"native vehicle WEAPON HIT paired%u variant%u neutral%u targetHP%u->%u trace",paired,g_ram[0xd13],neutral,health,minimum);for(unsigned j=0;j<120;++j)fprintf(stderr,"%02x",samples[j]);fprintf(stderr,"\n");
    check(minimum<health,"the actual native punch or vehicle missile damages the authored enemy only after real Fire");
    size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"vehicle weapon hit future allocates");memcpy(future,work,done);vehicle_capture(paired,work,"weapon-hit");
    check(RtlLoadSnapshotFromMemory(start,prepared),"native vehicle weapon hit restores for replay");X3CoopRuntimeEnable(paired);
    for(unsigned j=0;j<120;++j)frame((uint32_t)(j<48?SNES_PAD_Y:0)<<(paired?x*12:0));
    same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"actual vehicle weapon damage replays the complete machine");free(future);free(start);
  }
  check(RtlLoadSnapshotFromMemory(base,cut),"low-health armor branch restores before contact");X3CoopRuntimeEnable(paired);g_ram[0xcef]=1;
  bool fatal_pilot=getenv("X3_COOP_VEHICLE_FATAL_PILOT")!=NULL;
  if(fatal_pilot){check(paired,"native rider death requires its living free partner");size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState low=*X3CoopRuntimeState();low.players[x].body[0x27]=1;check(X3CoopEncode(&low,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"rider fatal contact supplies only one personal HP while actual armor is still occupied");door_install_player(&low);cut=RtlSaveSnapshotToMemory(base,capacity);}

  bool destroyed=false,released=false;unsigned variant=g_ram[0xd13];
  for(unsigned j=0;j<640;++j){frame(0);if(!(g_ram[0xcef]&127))destroyed=true;if(destroyed && vehicle_player(x)[2]!=0x2c)released=true;if(j%120==0)fprintf(stderr,"native vehicle DESTRUCTION STEP paired%u f%u RA%u/%u/%u sub%u HP%u flags%u hitflags%u timer%u Xaction%u mode%u\n",paired,j,g_ram[0xcc8],g_ram[0xcc9],g_ram[0xcca],g_ram[0xccb],g_ram[0xcef],g_ram[0xcd3],g_ram[0xd14],g_ram[0xd0f],vehicle_player(x)[2],g_ram[0x1f22]);if(destroyed && !g_ram[0xcc8] && (!fatal_pilot || X3CoopRuntimeState()->players[x].life==X3_COOP_FALLEN))break;}
  fprintf(stderr,"native vehicle DESTRUCTION paired%u variant%u destroyed%u released%u RA%u/%u hp%u Xaction%u XHP%u ZeroHP%u life%u/%u\n",paired,variant,destroyed,released,g_ram[0xcc8],g_ram[0xcc9],g_ram[0xcef],vehicle_player(x)[2],contact_hp(x),contact_hp(x^1),X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life);
  check(destroyed && released && !g_ram[0xcc8],"actual low-health armor hit destroys the vehicle and releases the native pilot");
  if(fatal_pilot){
    check(X3CoopRuntimeState()->players[x].life==X3_COOP_FALLEN && X3CoopRuntimeState()->players[x^1].life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe && !g_ram[0x1f22],"actual enemy contact kills only the released low-health rider and leaves no vehicle HUD or scene lock");
    size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"native rider death future allocates");memcpy(future,work,done);
    check(RtlLoadSnapshotFromMemory(base,cut),"native low-health pilot and occupied armor restore before actual contact");
    for(unsigned j=0;j<640;++j){frame(0);if(!g_ram[0xcc8] && X3CoopRuntimeState()->players[x].life==X3_COOP_FALLEN)break;}
    same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"actual vehicle destruction, release and rider death replay the complete machine");free(future);
    unsigned before=support_position(x^1,5),clock=g_ram[0x9cc];for(unsigned j=0;j<24;++j)frame((uint32_t)SNES_PAD_RIGHT<<((x^1)*12));
    check(support_position(x^1,5)>before && (uint8_t)(g_ram[0x9cc]-clock)==24 && (g_ppu->inidisp&0x8f)==15,"the actual free survivor moves in a bright world with one native world update after rider death");
    check(!X3CoopRuntimeRejoin(x) && X3CoopRuntimeState()->players[x].life==X3_COOP_FALLEN,"destroyed armor cannot return the actually fallen pilot");
  }
  vehicle_capture(paired,work,"empty");
}
static void vehicle_install_view(const X3CoopViewState *views,unsigned seat){
  view_word(0x1e5d,views->camera[seat][0]);view_word(0x1e7a,views->camera[seat][0]);view_word(0x1e60,views->camera[seat][1]);view_word(0x1e7c,views->camera[seat][1]);
  const unsigned targets[]={0x1e6e,0x1e70,0x1e78,0x1e7e};for(unsigned i=0;i<4;++i){view_word(0x1e66+2*i,views->current[seat][i]);view_word(targets[i],views->limits[seat][i]);}
}
static void vehicle_withdrawal(unsigned x,uint8_t *base,uint8_t *work,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
  team_hazard_body(setup.players[x^1].body,1856,407);
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"vehicle withdrawal places the living free partner in the actual armor room");door_install_player(&setup);
  X3CoopViewState views=*X3CoopViewsState();memcpy(views.limits[x^1],views.limits[x],sizeof(views.limits[x]));memcpy(views.current[x^1],views.current[x],sizeof(views.current[x]));memcpy(views.camera[x^1],views.camera[x],sizeof(views.camera[x]));X3CoopViewsSetState(&views);vehicle_install_view(&views,setup.anchor);
  bool settled=false;for(unsigned j=0;j<180;++j){frame(0);if(g_ram[0xcca]==2 && g_ram[0xccb]==1){settled=true;break;}}
  check(settled && vehicle_player(x)[2]==0x2c,"native vehicle settles before armed withdrawal");
  n=RtlSaveSnapshotToMemory(base,capacity);check(!X3CoopRuntimeWithdraw(x),"a pilot cannot withdraw before its native vehicle exit");same(base,n,work,RtlSaveSnapshotToMemory(work,capacity),"refused pilot withdrawal preserves the whole machine");
  bool fired=false;for(unsigned j=0;j<60;++j){frame((uint32_t)SNES_PAD_Y<<(x*12));if(g_ram[0xd15]){fired=true;break;}}
  check(fired && g_ram[0xd15]==2,"real Fire allocates two native armor missiles before exit");
  bool exited=false;for(unsigned j=0;j<90;++j){frame((uint32_t)(j%4<2?SNES_PAD_UP|SNES_PAD_B:0)<<(x*12));if(j%8==0)fprintf(stderr,"native vehicle EXIT AFTER FIRE f%u action%u/%u missiles%u\n",j,g_ram[0xcca],g_ram[0xccb],g_ram[0xd15]);if(vehicle_player(x)[2]!=0x2c && !(g_ram[0xcd3]&64)){exited=true;break;}}
  fprintf(stderr,"native vehicle WITHDRAW READY seat%u variant%u exit%u action%u lock%u mode%u missiles%u\n",x,g_ram[0xd13],exited,vehicle_player(x)[2],vehicle_player(x)[0x64],g_ram[0x1f22],g_ram[0xd15]);
  check(exited && g_ram[0xd15]==2,"native Up and Jump release X while its two real missiles remain live");
  n=RtlSaveSnapshotToMemory(base,capacity);uint8_t armor[80],missiles[sizeof(setup.players[0].shots)];memcpy(armor,g_ram+0xcc8,sizeof(armor));memcpy(missiles,X3CoopRuntimeState()->players[x].shots,sizeof(missiles));
  check(X3CoopRuntimeWithdraw(x^1),"the free partner can withdraw while X's actual armor missiles remain live");
  check(!memcmp(armor,g_ram+0xcc8,sizeof(armor)) && !memcmp(missiles,g_ram+X3_COOP_SHOTS,sizeof(missiles)),"withdrawing Zero preserves every shared armor byte and X's live native missiles");
  size_t only_x=RtlSaveSnapshotToMemory(work,capacity);uint8_t *unchanged=malloc(only_x);check(unchanged!=NULL,"last-pilot guard future allocates");memcpy(unchanged,work,only_x);check(!X3CoopRuntimeWithdraw(x),"an exited pilot cannot withdraw the last living actor");same(unchanged,only_x,work,RtlSaveSnapshotToMemory(work,capacity),"last-player refusal keeps live armor missiles unchanged");free(unchanged);
  check(RtlLoadSnapshotFromMemory(base,n),"both living participants restore with actual exited missiles");
  uint8_t other[sizeof(setup.players[0].shots)];memcpy(other,X3CoopRuntimeState()->players[x^1].shots,sizeof(other));unsigned hp=g_ram[0xcef],zhp=contact_hp(x^1);
  check(X3CoopRuntimeWithdraw(x),"a living X can withdraw immediately after the real native exit");
  unsigned live=0;for(unsigned i=0;i<sizeof(setup.players[0].shots);++i)live|=X3CoopRuntimeState()->players[x].shots[i];
  fprintf(stderr,"native vehicle WITHDRAW RESULT seat%u variant%u missiles%u live%u armor%u HP%u life%u/%u\n",x,g_ram[0xd13],g_ram[0xd15],live,g_ram[0xcc8],g_ram[0xcef],X3CoopRuntimeState()->players[x].life,X3CoopRuntimeState()->players[x^1].life);
  check(!live && !g_ram[0xd15],"cancelling the retired pilot's real missiles also releases native armor missile capacity");
  check(g_ram[0xcc8] && g_ram[0xcef]==hp && !(g_ram[0xcd3]&64) && contact_hp(x^1)==zhp && !memcmp(other,X3CoopRuntimeState()->players[x^1].shots,sizeof(other)),"withdrawal keeps the empty armor and the free actor's health and shots intact");
  for(unsigned j=0;j<24;++j)frame((uint32_t)SNES_PAD_LEFT<<((x^1)*12));
  size_t done=RtlSaveSnapshotToMemory(work,capacity);uint8_t *future=malloc(done);check(future!=NULL,"vehicle withdrawal future allocates");memcpy(future,work,done);
  check(RtlLoadSnapshotFromMemory(base,n) && X3CoopRuntimeWithdraw(x),"armed native-exit withdrawal restores and repeats");for(unsigned j=0;j<24;++j)frame((uint32_t)SNES_PAD_LEFT<<((x^1)*12));same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"armed vehicle withdrawal and survivor motion replay the complete machine");free(future);
  /* The station corridor need not contain the conservative return clearance.
   * Teleport only the free body and empty armor to the already-qualified
   * native lower plateau, then require real physics, safe return and boarding. */
  n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();team_hazard_body(setup.players[x^1].body,384,1175);
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"return approach positions only the living partner on the native lower plateau");door_install_player(&setup);
  view_word(0xccd,496);view_word(0xcea,496);view_word(0xcd0,1160);view_word(0xcec,1160);
  views=*X3CoopViewsState();for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=256;views.camera[seat][1]=1024;views.limits[seat][0]=views.current[seat][0]=0;views.limits[seat][1]=views.current[seat][1]=1024;views.limits[seat][2]=views.current[seat][2]=views.limits[seat][3]=views.current[seat][3]=1024;}X3CoopViewsSetState(&views);vehicle_install_view(&views,setup.anchor);
  for(unsigned j=0;j<30;++j)frame(0);
  fprintf(stderr,"native vehicle RETURN READY Zero%u,%u action%u mode%u armor%u,%u active%u HP%u\n",support_position(x^1,5),support_position(x^1,8),vehicle_player(x^1)[2],g_ram[0x1f22],view_read(0xccd),view_read(0xcd0),g_ram[0xcc8],g_ram[0xcef]);
  check(g_ram[0xcc8] && g_ram[0xcef]==hp && !g_ram[0xd15],"the empty armor survives native updates while X is absent without stale capacity");
  check(X3CoopRuntimeRejoin(x),"the living withdrawn pilot returns to genuinely safe native ground");
  unsigned ax=view_read(0xccd),ay=view_read(0xcd0);n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();team_hazard_body(setup.players[x].body,ax,ay-80);
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"returned X approaches above the retained native armor using only position and velocity");door_install_player(&setup);
  bool boarded=false;for(unsigned j=0;j<180;++j){frame(0);if(vehicle_player(x)[2]==0x2c && g_ram[0xcca]==2 && g_ram[0xccb]==1){boarded=true;break;}}
  check(boarded && !g_ram[0xd15],"the returned living X naturally boards the retained armor with empty missile capacity");
  bool refired=false;for(unsigned j=0;j<60;++j){frame((uint32_t)SNES_PAD_Y<<(x*12));if(g_ram[0xd15]){refired=true;break;}}
  check(refired && g_ram[0xd15]==2,"the returned pilot actually fires a fresh native missile pair after cancellation");

}
static void native_vehicle_reference(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==1,"vehicle reference restores its real station room");
  unsigned station=0;for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x2f && view_read(d+5)==1712 && view_read(d+8)==432)station=d;
  check(station!=0,"vehicle uses the actual authored station at1712,432");
  bool paired=getenv("X3_COOP_VEHICLE_PAIRED")!=NULL,summoned=false,boarded=false;
  unsigned variant=getenv("X3_COOP_VEHICLE_VARIANT")?(unsigned)atoi(getenv("X3_COOP_VEHICLE_VARIANT")):0;check(variant<4,"native requested armor variant is valid");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
  if(getenv("X3_COOP_VEHICLE_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  unsigned x=setup.players[0].character==X3_COOP_X?0:1;
  setup.current=setup.anchor=(uint8_t)(paired?x^1:x);
  team_hazard_body(setup.players[x].body,1712,400);setup.players[x].body[0x88]=255;
  if(paired){team_hazard_body(setup.players[x^1].body,384,1175);setup.players[x^1].body[0x88]=255;}
  memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"native X receives only position/protection and real roster prerequisites");
  door_install_player(&setup);X3CoopRuntimeEnable(paired);g_ram[0x1fd7]|=getenv("X3_COOP_VEHICLE_VARIANT")?15:1;
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
    if(getenv("X3_COOP_VEHICLE_VARIANT") && g_ram[station+2]==8)keys=g_ram[station+0x34]<variant?(f%2?SNES_PAD_DOWN:0):(f%24<8?SNES_PAD_Y:0);
    uint32_t controls=(uint32_t)keys<<(paired?x*12:0);
    if(paired && getenv("X3_COOP_VEHICLE_PARTNER_START") && (f==150 || f==151))controls|=(uint32_t)SNES_PAD_START<<((x^1)*12);
    frame(controls);const uint8_t *player=vehicle_player(x);
    if(paired && getenv("X3_COOP_VEHICLE_PARTNER_START"))check(!X3CoopNativePauseActive(g_ram),"partner Start cannot replace the station modal owner");
    if(f%60==0)fprintf(stderr,"native vehicle paired%u seat%u f%u X%u,%u action%u/%u station%u/%u/%u HP%u menu%u RA%u/%u/%u freeze%u/%u/%u/%u\n",paired,x,f,support_position(x,5),support_position(x,8),player[2],player[3],g_ram[station+1],g_ram[station+2],g_ram[station+3],g_ram[station+0x27],g_ram[0x1f3f],g_ram[0xcc8],g_ram[0xcc9],g_ram[0xccb],g_ram[0x1f22],g_ram[0x1f25],g_ram[0x1f27],g_ram[0x1f2b]);
    if(paired && f==180 && !g_ram[station+2])break;
    if(g_ram[0xcc8])summoned=true;
    if(g_ram[0xcc8] && player[2]==0x2c && !g_ram[0x1f3f]){
      boarded=true;check(g_ram[0xd13]==variant,"native station selects the acquired requested armor variant");unsigned before=view_read(0xccd),other=support_position(x^1,5);
      for(unsigned j=0;j<30;++j)frame((uint32_t)SNES_PAD_RIGHT<<(paired?x*12:0));
      fprintf(stderr,"native vehicle BOARDED paired%u seat%u f%u armorX%u->%u player%u,%u action%u other%u->%u RA%u/%u/%u\n",paired,x,f,before,view_read(0xccd),support_position(x,5),support_position(x,8),vehicle_player(x)[2],other,support_position(x^1,5),g_ram[0xcc8],g_ram[0xcc9],g_ram[0xccb]);
      check(view_read(0xccd)>before && vehicle_player(x)[2]==0x2c,"native riding action moves the actual armor under its owner's real Right input");
      if(paired)check(support_position(x^1,5)==other,"the other actor's neutral input does not move the actual pilot or vehicle");
      if(variant==3){bool landed=false;for(unsigned j=0;j<180;++j){frame(0);if((g_ram[0xcf3]|g_ram[0xcf4])&4){landed=true;fprintf(stderr,"native Frog settles after%u frames at%u,%u state%u/%u\n",j+1,view_read(0xccd),view_read(0xcd0),g_ram[0xcca],g_ram[0xccb]);break;}}check(landed,"native Frog hop lands before the opposite-direction ground request");}
      unsigned turned=view_read(0xccd);for(unsigned j=0;j<30;++j)frame((uint32_t)SNES_PAD_LEFT<<(paired?x*12:0));
      check(view_read(0xccd)<turned && vehicle_player(x)[2]==0x2c,"native pilot Left turns and moves the actual armor back into the room");
      fprintf(stderr,"native vehicle VARIANT%u group%u pose%u HP%u flags%u\n",g_ram[0xd13],g_ram[0xcde],g_ram[0xcdf],g_ram[0xcef],g_ram[0xcd3]);
      vehicle_capture(paired,work,"pilot");
      const char *output=getenv("X3_COOP_VEHICLE_OUTPUT");if(output && *output)check(RtlSaveSnapshot(output),"actual current-build native pilot fixture saves through the test interface");
      if(getenv("X3_COOP_VEHICLE_WITHDRAWAL")){check(paired && variant>=2,"armed vehicle withdrawal requires a native missile variant in co-op");vehicle_withdrawal(x,base,work,capacity);break;}
      if(getenv("X3_COOP_VEHICLE_ACTIONS"))vehicle_actions(x,paired,variant,base,work,capacity);
      if(getenv("X3_COOP_VEHICLE_DAMAGE"))vehicle_damage(x,paired,base,work,capacity);
      if(getenv("X3_COOP_VEHICLE_EXTENDED"))vehicle_extended(x,paired,base,work,capacity);break;
    }
  }
  fprintf(stderr,"native vehicle reference paired%u summoned%u boarded%u\n",paired,summoned,boarded);
  check(summoned && boarded,"real station interaction summons the armor and native contact earns boarding");
}
