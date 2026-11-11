/* Resume actual interrupted secondary native continuations. The existing
 * rollback blob carries CPU/APU/bridge timing; the retail routines and guest
 * stack supply the continuation. Audio-consumer delivery is host state, so
 * compare the normal canonical netplay prefix plus architectural CPU/time. */
extern int g_interp_apu_driving;
static struct {uint8_t *bytes;size_t capacity,size;uint32_t pc;uint64_t cycles;bool armed;} midpass;
static void midpass_capture(CpuState *cpu,uint32_t pc){
  pc&=0x7fffff;
  if(!midpass.armed || midpass.pc!=pc || X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor)return;
  midpass.armed=false;cpu_mirrors_to_p(cpu);midpass.cycles=cpu->master_cycles;midpass.size=RtlRollbackSaveToMemory(midpass.bytes,midpass.capacity);
  check(midpass.size!=0,"actual pending secondary native continuation saves a complete rollback snapshot");
}
static void native_interrupted_pass_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *expected,uint8_t *actual,size_t capacity){
  const unsigned roots[]={0x048000,0x04b58a,0x04cb31,0x04bfae},cuts[]={0x04800c,0x04b591,0x04cb31,0x04bfae};
  uint8_t *cut=malloc(capacity);check(cut!=NULL,"interrupted native rollback buffer allocates");midpass.bytes=cut;midpass.capacity=capacity;
  for(unsigned which=0;which<4;++which)check(interp_bridge_add_pre_opcode_hook(cuts[which],midpass_capture),"actual native continuation observation registers");
  for(unsigned roster=0;roster<2;++roster)for(unsigned which=0;which<4;++which){
    check(RtlLoadSnapshot(roster?reversed:normal),"interrupted continuation loads its real native roster");for(unsigned i=0;i<12;++i)frame(0);size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();setup.current=setup.anchor=0;
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=384+128*seat,y=setup.players[seat].character==X3_COOP_ZERO?1175:1183;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=b[0x26]=b[0x30]=b[0x88]=b[0x8a]=0;memset(b+0x1a,0,6);}
    memcpy(actual,base,n);check(X3CoopEncode(&setup,actual+contact_context_offset(actual,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(actual,n),"interrupted continuation selects the same installed seat0 world owner");door_install_player(&setup);X3CoopViewsConfigure(true);
    X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;views.valid=views.camera_valid=3;for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=(uint16_t)(256+128*seat);views.camera[seat][1]=1024;views.limits[seat][0]=views.current[seat][0]=256;views.limits[seat][1]=views.current[seat][1]=1024;views.limits[seat][2]=views.current[seat][2]=1024;views.limits[seat][3]=views.current[seat][3]=1024;}X3CoopViewsSetState(&views);view_word(0x1e5d,256);view_word(0x1e7a,256);const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};for(unsigned i=0;i<6;++i)view_word(vertical[i],1024);
    if(which==2)contact_geometry(0xd18,support_position(1,5),support_position(1,8));if(which==3)support_geometry(0xd18,support_position(1,5),support_position(1,8)+(setup.players[1].character==X3_COOP_ZERO?36:28));
    midpass.pc=cuts[which];midpass.size=0;midpass.armed=true;cpu_mirrors_to_p(&g_cpu);unsigned stack=g_cpu.S;g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=which>=2?0xd18:0;g_cpu.P=(uint8_t)((g_cpu.P|0x30)&~0x40);cpu_p_to_mirrors(&g_cpu);cpu_push_jsl_return_frame(&g_cpu);unsigned watermark=g_cpu.S;
    check(interp_bridge_run(&g_cpu,roots[which]),"retail paired native routine finishes its original continuation");midpass.armed=false;fprintf(stderr,"native midpass setup roster%u kind%u size%zu stack%04x/%04x enabled%u scene%u current%u anchor%u emulation%u body%u/%u freeze%02x\n",roster,which,midpass.size,stack,g_cpu.S,X3CoopRuntimeEnabled(),X3CoopRuntimeSceneActive(),X3CoopRuntimeState()->current,X3CoopRuntimeState()->anchor,g_cpu.emulation,g_ram[0x9d9],g_ram[0x9da],g_ram[0x1f2b]);check(midpass.size && g_cpu.S==stack,"real secondary continuation was captured and its guest caller stack balances");
    if(which==2)check(contact_hp(0)==setup.players[0].maximum_health && contact_hp(1)<setup.players[1].maximum_health,"interrupted actual contact hits only the secondary native actor");
    if(which==3){size_t raw=RtlSaveSnapshotToMemory(base,capacity);check((base[raw-X3_COOP_MENU_STATE_BYTES-31]&3)==2,"interrupted actual support classifies only its secondary rider");}
    cpu_mirrors_to_p(&g_cpu);CpuState outcome=g_cpu;size_t end=RtlNetplaySnapshotToMemory(expected,capacity);check(end && outcome.master_cycles>midpass.cycles,"original continuation advances actual guest time after its saved cut");
    g_interp_apu_driving=1;check(RtlRollbackLoadFromMemory(cut,midpass.size) && g_interp_apu_driving==1,"in-flight restore retains an already-active caller APU timing scope");g_interp_apu_driving=0;
    check(RtlRollbackLoadFromMemory(cut,midpass.size),"actual mid-controller/terrain/contact/support snapshot restores");check(g_interp_apu_driving==0,"in-flight restore retains the external caller APU timing scope");check(g_cpu.master_cycles==midpass.cycles && X3CoopRuntimeState()->current==1,"restore resumes the same projected actor and exact saved guest clock");
    check(interp_bridge_run(&g_cpu,cuts[which]),"retail interpreter resumes the actual pending native guest continuation");cpu_mirrors_to_p(&g_cpu);
    check(g_cpu.A==outcome.A && g_cpu.X==outcome.X && g_cpu.Y==outcome.Y && g_cpu.S==outcome.S && g_cpu.S==stack && g_cpu.D==outcome.D && g_cpu.DB==outcome.DB && g_cpu.PB==outcome.PB && g_cpu.P==outcome.P && g_cpu.master_cycles==outcome.master_cycles,"restored secondary pass retains exact native registers, balanced stack and elapsed guest time");
    size_t resumed=RtlNetplaySnapshotToMemory(actual,capacity);fprintf(stderr,"midpass full comparison roster%u kind%u coreBytes%zu cycles%u/%u flagsN%u/%u V%u/%u Z%u/%u C%u/%u\n",roster,which,RtlSnapshotGuestSize(NULL),outcome.cycles,g_cpu.cycles,outcome._flag_N,g_cpu._flag_N,outcome._flag_V,g_cpu._flag_V,outcome._flag_Z,g_cpu._flag_Z,outcome._flag_C,g_cpu._flag_C);if(getenv("X3_COOP_MIDPASS_OUTPUT")){char path[1024];snprintf(path,sizeof(path),"%s-expected.bin",getenv("X3_COOP_MIDPASS_OUTPUT"));FILE *fp=fopen(path,"wb");check(fp!=NULL,"native expected continuation evidence opens");fwrite(expected,1,end,fp);fclose(fp);snprintf(path,sizeof(path),"%s-resumed.bin",getenv("X3_COOP_MIDPASS_OUTPUT"));fp=fopen(path,"wb");check(fp!=NULL,"native resumed continuation evidence opens");fwrite(actual,1,resumed,fp);fclose(fp);}same(expected,RtlNetplaySnapshotHashedBytes(end),actual,RtlNetplaySnapshotHashedBytes(resumed),"interrupted secondary pass reproduces the complete canonical guest/title state");
    fprintf(stderr,"native interrupted pass roster%u kind%u cut%06x bytes%zu elapsed%llu stack%04x\n",roster,which,cuts[which],midpass.size,(unsigned long long)(outcome.master_cycles-midpass.cycles),stack);
  }
  free(cut);midpass.bytes=NULL;midpass.armed=false;
}
