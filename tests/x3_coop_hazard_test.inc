/* Actual stage5 spike row1632..1759/y1472; the other actor stands on the
 * adjacent authored plateau at1568/y1392. No damage/death result is supplied. */
static unsigned spike_victim,spike_damage,spike_controller,spike_terrain,spike_entry_tick;
static bool spike_controller_required,spike_controller_independent,spike_entry_living;
static void spike_damage_probe(CpuState *cpu,uint32_t pc){
  if(cpu->D!=X3_COOP_BODY || X3CoopRuntimeState()->current!=spike_victim)return;
  if(pc==0x048008){if(spike_controller_required){spike_entry_living=(g_ram[0x9ff]&127)!=0;spike_entry_tick=g_ram[0x9cc];}return;}
  if(pc==0x048084){if(!spike_controller_required || !spike_entry_living || spike_entry_tick!=g_ram[0x9cc] || g_ram[0x9da]!=0x0c || (g_ram[0x9ff]&127))return;}
  else if((g_ram[0x9ff]&127) || g_ram[0x9fe]!=127)return;
  ++spike_damage;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"actual pending lethal-terrain state passes runtime preflight");
  /* Runtime wire: controller entry/result registers are12 bytes each. */
  spike_controller|=view_probe.bytes[X3CoopEncodedSize()+24];
  spike_terrain|=view_probe.bytes[X3CoopEncodedSize()+26+24]!=0;
  if(spike_controller_independent && spike_controller && X3CoopRuntimeState()->current!=X3CoopRuntimeState()->anchor){
    size_t vi=0;for(size_t i=X3CoopEncodedSize();i+8<view_probe.used;++i)if(!memcmp(view_probe.bytes+i,"X3J2",4)){vi=i+8;break;}
    size_t at=vi+X3_COOP_VIEW_STATE_BYTES+67+24;check(vi && at<view_probe.used && view_probe.bytes[at]==3,"secondary in-controller fatal retains its actual independent personal-camera projection");
    fprintf(stderr,"native INDEPENDENT CONTROLLER FATAL seat%u pass%u personal%u tick%u\n",spike_victim,spike_controller,view_probe.bytes[at],g_ram[0x9cc]);
  }
}
static void native_spike_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,size_t capacity){
  uint8_t *replay=malloc(capacity);check(replay!=NULL,"fatal-contact replay buffer allocates");
  spike_controller_required=getenv("X3_COOP_CONTROLLER_SPIKE")!=NULL;
  spike_controller_independent=spike_controller_required && getenv("X3_COOP_CONTROLLER_SPIKE_INDEPENDENT");
  /* Default terrain contact runs after the controllers. Observe B5A9 after
   * the real collision, before B5AB projects the other body's terrain pass.
   * The ordinary AOT damage helper is not interpreted with learner off. */
  check(interp_bridge_add_pre_opcode_hook(0x048008,spike_damage_probe) &&
        interp_bridge_add_pre_opcode_hook(0x048084,spike_damage_probe) &&
        interp_bridge_add_pre_opcode_hook(0x04b5a9,spike_damage_probe) &&
        interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"native lethal-terrain and pit observations register");
  for(unsigned roster=0;roster<2;++roster)for(unsigned victim=0;victim<2;++victim){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==5 &&
        X3CoopRuntimeState()->players[0].character==roster && !X3CoopRuntimeLearner() &&
        !X3CoopRuntimeStockZero(),"spike route uses actual stage5 and the requested default roster");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat){
      uint8_t *b=setup.players[seat].body;unsigned x=spike_controller_required?1128:seat==victim?1680:1568;
      unsigned y=spike_controller_required?(setup.players[seat].character==X3_COOP_ZERO?663:671):seat==victim?1400:setup.players[seat].character==X3_COOP_ZERO?1367:1375;
      if(spike_controller_required){b[0x30]=b[0x88]=b[0x8a]=0;}
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);
      b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);
      b[4]=b[7]=b[2]=b[3]=0;memset(b+0x1a,0,6);
    }
    shared_prepare(base,n,work,&setup);view_word(0x1e5d,spike_controller_required?1032:1456);view_word(0x1e7a,spike_controller_required?1032:1456);
    /* Match the actual upper-room camera band to this QA teleport. Keeping
     * the lower entry room's bounds makes native B59B push both bodies down
     *16 pixels per frame, burying the otherwise safe partner in its floor. */
    const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
    for(unsigned i=0;i<sizeof(vertical)/sizeof(*vertical);++i)view_word(vertical[i],spike_controller_required?(i<2?576:i==2 || i==4?512:1024):1280);
    if(spike_controller_independent){
      X3CoopViewsConfigure(true);X3CoopViewState v=*X3CoopViewsState();v.independent=v.initialized=1;v.stage=5;v.valid=v.camera_valid=3;
      for(unsigned seat=0;seat<2;++seat){v.camera[seat][0]=1032;v.camera[seat][1]=576;v.current[seat][0]=v.limits[seat][0]=0;v.current[seat][1]=v.limits[seat][1]=8192;v.current[seat][2]=v.limits[seat][2]=512;v.current[seat][3]=v.limits[seat][3]=1024;}X3CoopViewsSetState(&v);vehicle_install_view(&v,setup.anchor);
    }
    if(spike_controller_required){
      for(unsigned f=0;f<12;++f)frame(0);check(campaign_body(victim)[0x2b]&4,"controller-fatal approach first earns actual native ground contact");
      for(unsigned f=0;f<8;++f)frame((uint32_t)SNES_PAD_B<<(victim*12));unsigned falling=0;
      for(;falling<90;++falling){const uint8_t *b=campaign_body(victim);if(b[2]==8 && (int16_t)(b[0x1c]|b[0x1d]<<8)<0 && !(b[0x2b]&4))break;frame(0);}
      check(falling<90,"real physical Jump and gravity earn the downward native controller before spike positioning");
      n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
      /* Earn actual wall-slide on3B above the authored3F column. The
       * open right face is at1040; no wall/contact/action flag is supplied. */
      campaign_position_only(&setup.players[victim],1046,setup.players[victim].character==X3_COOP_X?697:693);
      check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"solid-wall preparation changes only the already falling actor's position");door_install_player(&setup);
      unsigned wall=0;for(;wall<8 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE && (campaign_body(victim)[2]!=0x12 || campaign_body(victim)[3]!=2);++wall)frame((uint32_t)SNES_PAD_LEFT<<(victim*12));
      check(RtlSaveSnapshot("native-controller-wallslide-prerequisite.sav"),"the actual native wall-slide prerequisite retains its machine");
      fprintf(stderr,"native WALL SLIDE victim%u char%u frames%u action%u/%u HP%u flags%02x/%02x XY%u,%u\n",victim,X3CoopRuntimeState()->players[victim].character,wall,campaign_body(victim)[2],campaign_body(victim)[3],contact_hp(victim),campaign_body(victim)[0x2b],campaign_body(victim)[0x2c],support_position(victim,5),support_position(victim,8));
      check(wall<8 && contact_hp(victim) && campaign_body(victim)[2]==0x12 && campaign_body(victim)[3]==2,"real directional input earns native wall-slide on the solid section before any lethal position");
      n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
      campaign_position_only(&setup.players[victim],support_position(victim,5),setup.players[victim].character==X3_COOP_X?777:773);
      check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"wall-slide continues down the authored column using position-only preparation");door_install_player(&setup);
    }
    if(spike_controller_required)check(RtlSaveSnapshot("native-controller-side-spike-approach.sav"),"the earned falling side approach retains its complete machine");
    spike_victim=shared_pit_victim=victim;spike_damage=spike_controller=spike_terrain=shared_pit_damage=0;
    spike_entry_living=false;uint32_t approach=spike_controller_required?(uint32_t)SNES_PAD_LEFT<<(victim*12):0;
    unsigned elapsed=0;size_t contact_size=0;
    for(;elapsed<180 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE;++elapsed){
      contact_size=RtlSaveSnapshotToMemory(base,capacity);frame(approach);
    }
    const X3CoopState *s=X3CoopRuntimeState();
    fprintf(stderr,"authored spike roster%u seat%u frames%u fatalSelections%u controller%u terrain%u pit%u life%u/%u xy%u/%u\n",
        roster,victim,elapsed,spike_damage,spike_controller,spike_terrain,shared_pit_damage,
        s->players[0].life,s->players[1].life,coop_x(victim),support_position(victim,8));
    if(spike_controller_required)check(RtlSaveSnapshot("native-controller-side-spike-outcome.sav"),"the actual side-spike result retains its complete native machine");
    if(spike_controller_required)check(spike_controller && !spike_terrain,"native authored spike fatal must occur inside its actual active controller, outside the post-enemy terrain pass");
    check(elapsed<180 && spike_damage==1 && !shared_pit_damage && s->players[victim].life==X3_COOP_FALLEN &&
        s->players[victim^1].life==X3_COOP_ALIVE && s->anchor==(victim^1) && !s->wipe,
        "actual spike contact kills only the actor above the authored row and retains the free partner");
    size_t fatal_size=RtlSaveSnapshotToMemory(work,capacity);
    CaptureSimulationFrame(1);size_t pixel_size=(size_t)g_ppu->renderPitch*224;
    uint8_t *pixels=malloc(pixel_size);check(pixels!=NULL,"actual fatal-output buffer allocates");
    memcpy(pixels,g_ppu->renderBuffer,pixel_size);
    check(contact_size && RtlLoadSnapshotFromMemory(base,contact_size),"last actual frame before spike contact restores");
    frame(approach);size_t replay_size=RtlSaveSnapshotToMemory(replay,capacity);
    same(work,fatal_size,replay,replay_size,"fatal spike contact replays the complete machine and survivor handoff");
    CaptureSimulationFrame(1);
    check(!memcmp(pixels,g_ppu->renderBuffer,pixel_size),"actual displayed fatal-contact pixels replay identically");
    free(pixels);
    check(RtlLoadSnapshotFromMemory(work,fatal_size),"actual fatal-contact outcome restores");
    replay_size=RtlSaveSnapshotToMemory(replay,capacity);
    same(work,fatal_size,replay,replay_size,"actual fallen and survivor contexts restore immediately");
    if(spike_controller_required){
      uint8_t *future=malloc(capacity);check(future!=NULL,"controller-fatal survivor whole future allocates");unsigned lives=g_ram[0x1fb4];
      for(unsigned run=0;run<2;++run){check(RtlLoadSnapshotFromMemory(work,fatal_size),"the actual in-controller fatal restores before its entire survivor future");unsigned before=coop_x(victim^1),clock=g_ram[0x9cc];
        for(unsigned f=0;f<24;++f)frame((uint32_t)SNES_PAD_LEFT<<((victim^1)*12));check(coop_x(victim^1)<before,"actual controller-fatal survivor retains physical movement");
        for(unsigned f=0;f<120;++f)frame(0);for(unsigned a=0x1f25;a<=0x1f2b;++a)check(!g_ram[a],"fatal selection inside a running actor cannot freeze shared world pools");
        check((uint8_t)(g_ram[0x9cc]-clock)==144 && g_ram[0x1fb4]==lives && X3CoopRuntimeState()->players[victim].life==X3_COOP_FALLEN && X3CoopRuntimeState()->players[victim^1].life==X3_COOP_ALIVE && g_ram[0xd1]==4 && g_ram[0xd2]==4,"actual in-controller death leaves one continuing world and a stable native survivor");
        size_t done=RtlSaveSnapshotToMemory(base,capacity);if(!run)memcpy(future,base,done);else same(future,done,base,done,"actual in-controller fatal and complete unfrozen survivor future replay the whole machine exactly");
      }free(future);continue;
    }
    if(getenv("X3_COOP_TRANSITION_MENU")){
      X3CoopPlayer fallen=X3CoopRuntimeState()->players[victim];X3CoopMenuInfo panel;
      menu_press(victim,0x1000);check(!X3CoopMenuRead(&panel) && !(g_ram[0x1f37]&1),"an actual fallen actor cannot own native pause");
      menu_open(victim^1);size_t paused=RtlSaveSnapshotToMemory(base,capacity);
      check(RtlLoadSnapshotFromMemory(base,paused),"survivor-owned active native pause restores after actual fatal contact");
      frame(menu_pad(victim,0x1000)|menu_pad(victim,0x4000));frame(0);frame(0);panel=menu_read();
      check(panel.owner==(victim^1) && !panel.healing[victim] && !memcmp(&fallen,&X3CoopRuntimeState()->players[victim],sizeof(fallen)),"fallen Start/Fire cannot steal a restored panel, heal or change the retired context");
      menu_press(victim^1,0x1000);for(unsigned f=0;f<120;++f)frame(0);
      check(!X3CoopMenuRead(&panel) && !(g_ram[0x1f37]&1) && !memcmp(&fallen,&X3CoopRuntimeState()->players[victim],sizeof(fallen)) && X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,"survivor resumes one restored pause with balanced ownership and no revival");
    }
    unsigned before=coop_x(victim^1);
    for(unsigned f=0;f<24;++f)frame((uint32_t)SNES_PAD_LEFT<<((victim^1)*12));
    fprintf(stderr,"spike survivor roster%u victim%u x%u->%u y%u display%02x life%u world%u/%u\n",
        roster,victim,before,coop_x(victim^1),support_position(victim^1,8),g_ppu->inidisp,
        X3CoopRuntimeState()->players[victim^1].life,g_ram[0xd1],g_ram[0xd2]);
    check(coop_x(victim^1)<before && (g_ppu->inidisp&0x8f)==15,
        "spike survivor keeps actual movement and bright gameplay");
    for(unsigned a=0x1f25;a<=0x1f2b;++a)check(!g_ram[a],"fatal terrain leaves shared world pools unfrozen");
  }
  free(replay);
}

/* Actual stage1 lower and upper rooms,768px apart vertically. The class05
 * bridge remains active; the terminal fall starts below that real support. */
static bool independent_projection_observe;
static unsigned independent_projection_seen;
static void independent_projection_probe(CpuState *cpu,uint32_t pc){
  (void)pc;if(!independent_projection_observe || cpu->D!=X3_COOP_BODY)return;
  const X3CoopState *s=X3CoopRuntimeState();if(s->current==s->anchor)return;
  const X3CoopViewState *v=X3CoopViewsState();unsigned seat=s->current;
  if(view_read(0x1e60)!=v->camera[seat][1] || view_read(0x1e6a)!=v->current[seat][2])check(false,"secondary native terrain sees its complete actual private camera");
  if(independent_projection_seen&(1u<<seat))return;independent_projection_seen|=1u<<seat;
  check(true,"secondary native terrain sees its complete actual private camera");
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;X3CoopRuntimeSave(&view_probe.io);
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"active personal camera projection passes runtime preflight");
  size_t vi=0;for(size_t i=X3CoopEncodedSize();i+8<view_probe.used;++i)if(!memcmp(view_probe.bytes+i,"X3J2",4)){vi=i+8;break;}
  check(vi && vi+X3_COOP_VIEW_STATE_BYTES+67+24<view_probe.used,"active personal projection locates its serialized view/bounds fields");
  size_t pass_at=vi+X3_COOP_VIEW_STATE_BYTES+67+24;
  check(view_probe.bytes[pass_at]==3,"actual secondary pass retains a personal camera backup");
  view_probe.bytes[pass_at]=4;check(!X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"unknown personal camera pass is rejected");view_probe.bytes[pass_at]=3;
  view_probe.bytes[vi]=0;check(!X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"personal projection without independent simulation is rejected");view_probe.bytes[vi]=1;
  size_t controller_at=X3CoopEncodedSize()+24,terrain_at=controller_at+26;
  uint8_t c=view_probe.bytes[controller_at],t=view_probe.bytes[terrain_at];view_probe.bytes[controller_at]=view_probe.bytes[terrain_at]=0;
  check(!X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"orphaned personal camera backup is rejected without a secondary controller or terrain pass");
  view_probe.bytes[controller_at]=c;view_probe.bytes[terrain_at]=t;
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"malformed personal-view probes preserve the valid pending state");
  view_probe.used=0;view_probe.load=true;check(X3CoopRuntimeLoad(&view_probe.io),"active personal camera projection restores in place");
}
static void native_independent_pit_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,size_t capacity){
  independent_projection_observe=true;independent_projection_seen=0;
  check(interp_bridge_add_pre_opcode_hook(0x04b591,independent_projection_probe) && interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"separated-room pits observe native camera projection and fatal damage");
  for(unsigned roster=0;roster<2;++roster)for(unsigned victim=0;victim<2;++victim){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==1 && !X3CoopRuntimeLearner(),"independent pit loads the actual default stage1 roster");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;
      unsigned x=seat==victim?1024:1696,y=seat==victim?1183:431;if(setup.players[seat].character==X3_COOP_ZERO)y-=8;
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);
      b[4]=b[7]=b[2]=b[3]=0;b[0x88]=255;b[0x8a]=0;memset(b+0x1a,0,6);
    }
    shared_prepare(base,n,work,&setup);X3CoopViewsConfigure(true);
    X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){unsigned cy=seat==victim?1024:256;
      views.camera[seat][0]=(uint16_t)(seat==victim?896:1536);views.camera[seat][1]=(uint16_t)cy;
      views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=(uint16_t)cy;
      views.limits[seat][3]=views.current[seat][3]=(uint16_t)cy;
    }
    X3CoopViewsSetState(&views);
    /* This QA teleport also sets the native anchor's actual camera band. */
    unsigned anchor=setup.anchor;view_word(0x1e5d,views.camera[anchor][0]);view_word(0x1e7a,views.camera[anchor][0]);
    view_word(0x1e66,views.current[anchor][0]);view_word(0x1e68,views.current[anchor][1]);view_word(0x1e6e,views.limits[anchor][0]);view_word(0x1e70,views.limits[anchor][1]);
    const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
    for(unsigned i=0;i<6;++i)view_word(vertical[i],views.camera[anchor][1]);
    for(unsigned i=0;i<60;++i)frame(0);const X3CoopState *s=X3CoopRuntimeState();views=*X3CoopViewsState();
    fprintf(stderr,"independent room roster%u lowerseat%u cameras%u/%u bodies%u/%u lives%u/%u\n",roster,victim,views.camera[0][1],views.camera[1][1],support_position(0,8),support_position(1,8),s->players[0].life,s->players[1].life);
    check(s->players[0].life==X3_COOP_ALIVE && s->players[1].life==X3_COOP_ALIVE && !s->wipe &&
        views.camera[victim][1]>views.camera[victim^1][1]+512 && support_position(victim,8)>views.camera[victim^1][1]+224,
        "living lower-room actor cannot fall through the upper partner camera's bottom");
    if(!roster && !victim){
      size_t reference_size=RtlSaveSnapshotToMemory(base,capacity);X3CoopState reference=*s;uint8_t *body=reference.players[0].body;
      body[5]=body[0x22]=0x40;body[6]=body[0x23]=4; /* x1088, actual approach above the support. */
      memcpy(work,base,reference_size);check(X3CoopEncode(&reference,work+contact_context_offset(work,reference_size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,reference_size),"native support reference restores the same actual world objects");
      door_install_player(&reference);X3CoopRuntimeEnable(false);bool boarded=false;
      for(unsigned i=0;i<228;++i){frame(i<48?SNES_PAD_RIGHT:0);boarded=boarded || (g_ram[0xd62]==5 && (g_ram[0xd84]&1) && view_read(0x9e0)<1183);if(!(g_ram[0x9ff]&127))break;}
      fprintf(stderr,"original class05 support reference boarded%u player%u,%u HP%u object%u,%u latch%02x\n",boarded,view_read(0x9dd),view_read(0x9e0),g_ram[0x9ff]&127,view_read(0xd5d),view_read(0xd60),g_ram[0xd84]);
      check(boarded && (g_ram[0x9ff]&127),"the original single-player support also carries this approach instead of selecting pit damage");
      check(RtlLoadSnapshotFromMemory(base,reference_size),"original support comparison restores the complete paired approach");s=X3CoopRuntimeState();
    }
    n=RtlSaveSnapshotToMemory(base,capacity);setup=*s;uint8_t *b=setup.players[victim].body;
    unsigned x=1152,y=setup.players[victim].character==X3_COOP_ZERO?1232:1240;
    b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);
    b[4]=b[7]=b[2]=b[3]=0;memset(b+0x1a,0,6);memcpy(work,base,n);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"lower actor enters the actual abyss below its live native support without supplying a death result");
    door_install_player(&setup);views=*X3CoopViewsState();views.camera[victim][0]=896;X3CoopViewsSetState(&views);
    shared_pit_victim=victim;shared_pit_damage=0;unsigned walked=0,falling=0;
    for(;walked<64 && support_position(victim,5)<1152 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE;++walked)frame((uint32_t)SNES_PAD_RIGHT<<(victim*12));
    for(;falling<180 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE;++falling)frame(0);
    const char *failure=getenv("X3_COOP_INDEPENDENT_PIT_FAILURE_OUTPUT");
    if(falling==180 && failure && *failure)check(RtlSaveSnapshot(failure),"actual stalled independent pit saves for focused inspection");
    s=X3CoopRuntimeState();fprintf(stderr,"independent pit roster%u victim%u walk%u fall%u fatal%u life%u/%u camera%u/%u\n",roster,victim,walked,falling,shared_pit_damage,s->players[0].life,s->players[1].life,X3CoopViewsState()->camera[0][1],X3CoopViewsState()->camera[1][1]);
    const X3CoopViewState *ending=X3CoopViewsState();
    fprintf(stderr,"independent pit bounds body%u/%u xy%u,%u action%u current%u,%u,%u,%u target%u,%u,%u,%u worldtop/bottom%u/%u\n",
      s->players[victim].body[0x27],s->players[victim^1].body[0x27],support_position(victim,5),support_position(victim,8),s->players[victim].body[2],
      ending->current[victim][0],ending->current[victim][1],ending->current[victim][2],ending->current[victim][3],
      ending->limits[victim][0],ending->limits[victim][1],ending->limits[victim][2],ending->limits[victim][3],view_read(0x1e6a),view_read(0x1e6c));
    check(walked<64 && falling<180 && shared_pit_damage==1 && s->players[victim].life==X3_COOP_FALLEN && s->players[victim^1].life==X3_COOP_ALIVE && !s->wipe && s->anchor==(victim^1),
        "true authored stage1 pit kills only its entering actor and preserves the separated upper-room survivor");
    unsigned before=support_position(victim^1,5);for(unsigned i=0;i<24;++i)frame((uint32_t)SNES_PAD_RIGHT<<((victim^1)*12));
    fprintf(stderr,"independent survivor roster%u victim%u x%u->%u y%u HP%u action%u world%u/%u brightness%02x\n",roster,victim,before,support_position(victim^1,5),support_position(victim^1,8),contact_hp(victim^1),s->players[victim^1].body[2],g_ram[0xd1],g_ram[0xd2],g_ppu->inidisp);
    check(support_position(victim^1,5)>before && (g_ppu->inidisp&0x8f)==15 && g_ram[0xd1]==4 && g_ram[0xd2]==4,"separated pit survivor retains real native movement and bright gameplay");
  }
  check(independent_projection_seen,"separated-room test exercised a restorable native personal camera projection");
  independent_projection_observe=false;
}

static void native_last_true_pit_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,size_t capacity){
  check(interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"last-survivor true pit observes native fatal calls");
  for(unsigned roster=0;roster<2;++roster)for(unsigned last=0;last<2;++last){
    unsigned first=last^1;true_pit_prepare(roster?reversed:normal,first,base,work,capacity);unsigned lives=g_ram[0x1fb4];
    shared_pit_victim=first;shared_pit_damage=0;
    for(unsigned i=0;i<38;++i)frame((uint32_t)SNES_PAD_RIGHT<<(first*12));
    for(unsigned i=0;i<180 && X3CoopRuntimeState()->players[first].life==X3_COOP_ALIVE;++i)frame(0);
    check(shared_pit_damage==1 && X3CoopRuntimeState()->players[first].life==X3_COOP_FALLEN && X3CoopRuntimeState()->players[last].life==X3_COOP_ALIVE,"first real stage1 pit leaves the last survivor in control");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();uint8_t *b=setup.players[last].body;
    unsigned x=1088,y=setup.players[last].character==X3_COOP_ZERO?1175:1183;
    b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);
    b[4]=b[7]=b[2]=b[3]=0;memset(b+0x1a,0,6);shared_prepare(base,n,work,&setup);
    shared_pit_victim=last;shared_pit_damage=0;bool wiped=false;
    for(unsigned i=0;i<38;++i)frame((uint32_t)SNES_PAD_RIGHT<<(last*12));unsigned elapsed=0;
    for(;elapsed<3600;++elapsed){frame(0);const X3CoopState *s=X3CoopRuntimeState();if(s->wipe)wiped=true;
      if(!(g_ram[0x1fb4]==lives || g_ram[0x1fb4]+1==lives))check(false,"true last-survivor pit cannot debit multiple lives");
      if(wiped && !s->wipe && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4 && g_ram[0x9da]<=0x16 && !g_ram[0x1f22])break;
    }
    fprintf(stderr,"true last pit roster%u last%u fatal%u wait%u wiped%u life%u/%u\n",roster,last,shared_pit_damage,elapsed,wiped,lives,g_ram[0x1fb4]);
    check(shared_pit_damage==1 && elapsed<3600 && wiped && g_ram[0x1fb4]+1==lives && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual last-survivor pit completes one native full-team retry with one debit");
    unsigned before=support_position(last,5);for(unsigned i=0;i<24;++i)frame((uint32_t)SNES_PAD_RIGHT<<(last*12));check(support_position(last,5)>before,"true pit retry restores actual survivor-seat control");
  }
}
