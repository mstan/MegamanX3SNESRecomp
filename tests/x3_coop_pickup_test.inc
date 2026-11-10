/* Real stage1 health item; teleports/HP seed the approach, never the claim,
 * refill count, freeze, completion or retirement results. */
static unsigned pickup_probe_slot;
static unsigned pickup_probe_pc=0x00dafa,pickup_probe_seat=2;
static bool pickup_probe_seen;
static void pickup_continuation_probe(CpuState *cpu,uint32_t pc){
  if((pc&0x7fffff)!=pickup_probe_pc || !pickup_probe_slot || cpu->D!=pickup_probe_slot ||
     (pickup_probe_seat<2 && X3CoopRuntimeState()->current!=pickup_probe_seat))return;
  pickup_probe_slot=0;pickup_probe_seen=true;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);
  check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),
        "interrupted native pickup actor borrow passes snapshot preflight");
  view_probe.used=0;view_probe.load=true;
  check(X3CoopRuntimeLoad(&view_probe.io),"native pickup actor continuation restores in place");
}
static void pickup_native_items(unsigned pc){
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc) && g_cpu.S==resume.S,"native item loop returns with its real stack");
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
}
static void pickup_native_item_loop(void){pickup_native_items(0x00dac2);}
static bool pickup_single_overlap(unsigned slot,const X3CoopPlayer *player){
  bool enabled=X3CoopRuntimeEnabled();X3CoopRuntimeEnable(false);
  memcpy(g_ram+X3_COOP_BODY,player->body,sizeof(player->body));
  uint16_t stack=g_cpu.S;g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=(uint16_t)slot;g_cpu.X=X3_COOP_BODY;
  g_cpu.P=(uint8_t)((g_cpu.P|0x20)&~0x10);cpu_p_to_mirrors(&g_cpu);cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x04cc5c) && g_cpu.S==stack,"single-body native pickup overlap returns with its actual stack");
  bool hit=g_cpu._flag_C;X3CoopRuntimeEnable(enabled);return hit;
}
static unsigned health_pickup_approach(uint8_t *original,size_t size,uint8_t *work,unsigned health){
  check(RtlLoadSnapshotFromMemory(original,size),"health pickup setup restores");
  X3CoopState setup=*X3CoopRuntimeState();
  unsigned seat=setup.players[0].character==X3_COOP_ZERO?0:1;
  setup.players[seat].body[5]=0x38;setup.players[seat].body[6]=7; /*1848*/
  setup.players[seat].body[8]=0x8c;setup.players[seat].body[9]=5; /*1420*/
  setup.players[seat].body[0x27]=(uint8_t)health;
  memset(setup.players[seat].body+0x1a,0,6);
  memcpy(work,original,size);size_t offset=contact_context_offset(original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),
        "private damaged-body approach restores");
  X3CoopViewsConfigure(true);
  /* The QA teleport bypasses the route and camera-admission history. Seed
   * the actual item's lower-room band; native admission/contact/refill remain
   * untouched. Camera traversal is a separate acceptance requirement. */
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){
    views.camera[s][0]=(uint16_t)(s==seat?1720:256);
    views.camera[s][1]=(uint16_t)(s==seat?1280:1024);
    views.limits[s][1]=views.current[s][1]=8192;
    views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];
  }
  X3CoopViewsSetState(&views);
  return seat;
}
static void pickup_contact_fallback_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  unsigned zero=health_pickup_approach(original,size,work,20),x=zero^1;
  size_t offset=contact_context_offset(original,size);
  X3CoopState setup=*X3CoopRuntimeState();setup.players[zero].body[8]=0x5a; /*1370, above the item*/
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),"native pickup constructor approach restores");
  /* Restore the private camera setup after loading the original machine. */
  X3CoopViewsConfigure(true);X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=s==zero?1720:256;views.camera[s][1]=s==zero?1280:1024;
    views.limits[s][1]=views.current[s][1]=8192;views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];}
  X3CoopViewsSetState(&views);frame(0);
  pickup_native_items(0x00daa0);pickup_native_items(0x00daa0);
  unsigned slot=0;
  for(unsigned d=0x1518;d<0x1818;d+=48)if(g_ram[d] && g_ram[d+10]==2 && (g_ram[d+5]|g_ram[d+6]<<8)==1848)slot=d;
  check(slot && g_ram[slot+2]!=6,"original authored health item initializes before any collection");
  for(unsigned i=0;i<8 && g_ram[slot+2]!=2;++i)pickup_native_items(0x00daa0);
  check(g_ram[slot+2]==2 && !g_ram[0x1f28],"native pickup finishes its landing before the contact comparison");
  setup=*X3CoopRuntimeState();
  for(unsigned s=0;s<2;++s){setup.players[s].body[5]=0x38;setup.players[s].body[6]=7;}
  setup.players[x].body[8]=0x92;setup.players[x].body[9]=5; /*1426: nearer center, misses*/
  setup.players[zero].body[8]=0x90;setup.players[zero].body[9]=5; /*1424: taller offset body, hits*/
  setup.players[x].body[0x27]=8;
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,prepared),"unequal native pickup geometry restores");
  memcpy(g_ram+X3_COOP_BODY,setup.players[x].body,X3_COOP_BODY_BYTES);
  for(unsigned s=0;s<2;++s){views.camera[s][0]=1720;views.camera[s][1]=1280;
    views.limits[s][2]=views.current[s][2]=1280;views.limits[s][3]=views.current[s][3]=1280;}
  X3CoopViewsSetState(&views);
  view_word(0x1e5d,1720);view_word(0x1e7a,1720);view_word(0x1e60,1280);view_word(0x1e7c,1280);
  view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6a,1280);view_word(0x1e6c,1280);
  view_word(0x1e6e,0);view_word(0x1e70,8192);view_word(0x1e78,1280);view_word(0x1e7e,1280);
  prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(!pickup_single_overlap(slot,&setup.players[x]),"native X hitbox misses despite the closer body origin");
  check(pickup_single_overlap(slot,&setup.players[zero]),"native Zero hitbox touches the same authored item");
  check(RtlLoadSnapshotFromMemory(work,prepared),"native contact fallback comparison restores");
  check(X3CoopNearest(X3CoopRuntimeState(),g_ram,1848,1448)==x,"closer-origin policy selects the actual native miss");
  X3CoopRuntimeEnable(false);pickup_native_items(0x00daa0);
  check(g_ram[slot+2]!=6 && !g_ram[0x1f28],"nearest-only native X item update leaves the valid Zero contact unclaimed");
  check(RtlLoadSnapshotFromMemory(work,prepared),"native single-body comparison restores");
  X3CoopRuntimeEnable(false);memcpy(g_ram+X3_COOP_BODY,setup.players[zero].body,X3_COOP_BODY_BYTES);
  g_ram[0x1fd2]=28;pickup_native_items(0x00daa0);
  uint8_t native_item[48];memcpy(native_item,g_ram+slot,sizeof(native_item));
  if(g_ram[slot+2]!=6 || !g_ram[0x1f28])fprintf(stderr,"native pickup ref: state=%u/%u/%u xy=%u,%u HP=%u bodyState=%u geom=%04x block=%u freeze=%u\n",
      g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],view_read(slot+5),view_read(slot+8),g_ram[X3_COOP_BODY+0x27],
      g_ram[X3_COOP_BODY+2],view_read(X3_COOP_BODY+0x20),g_ram[0x1f1d],g_ram[0x1f28]);
  check(g_ram[slot+2]==6 && g_ram[0x1f28],"native single-body Zero reference really claims the item");
  check(RtlLoadSnapshotFromMemory(work,prepared),"paired item update starts from the matched native precondition");
  check(interp_bridge_add_pre_opcode_hook(0x04cc5c,pickup_continuation_probe),"pickup fallback continuation probe registers");
  pickup_probe_slot=slot;pickup_probe_pc=0x04cc5c;pickup_probe_seat=zero;pickup_probe_seen=false;
  pickup_native_items(0x00daa0);
  check(g_ram[slot+2]==6 && g_ram[0x1f28],"native item contact retries the other eligible hitbox after a true miss");
  check(pickup_probe_seen,"native second pickup-contact continuation is observed and restored");
  check(!memcmp(native_item,g_ram+slot,sizeof(native_item)),"paired collection matches exactly one original item movement/refill update");
  size_t collected=RtlSaveSnapshotToMemory(expected,capacity),marker=0;
  for(size_t i=0;i+8<collected;++i)if(!memcmp(expected+i,"X3R1",4)){marker=i;break;}
  check(marker && expected[marker+6]==zero && expected[marker+7]==1,"successful second contact owns the one native refill");
  for(unsigned mode=0;mode<5;++mode){
    check(X3CoopDecode(&setup,work+offset,X3CoopEncodedSize()),"native pickup eligibility fixture decodes");
    if(mode==0)setup.players[zero].body[0x27]=0;
    if(mode==1)check(X3CoopFall(&setup,zero),"fallen pickup-candidate lifecycle precondition applies");
    if(mode==2)setup.players[x].body[0x20]=setup.players[x].body[0x21]=0;
    if(mode==3)setup.players[x].body[8]=0x9b; /*1435, both actual bodies touch*/
    memcpy(expected,work,prepared);
    check(X3CoopEncode(&setup,expected+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(expected,prepared),
          "native pickup eligibility fixture restores");
    memcpy(g_ram+X3_COOP_BODY,setup.players[x].body,X3_COOP_BODY_BYTES);
    if(mode==4)g_ram[slot+0x20]=g_ram[slot+0x21]=0;
    pickup_native_items(0x00daa0);
    check(X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,"native pickup retry restores canonical world ownership");
    if(mode<2 || mode==4){
      check(g_ram[slot+2]!=6 && !g_ram[0x1f28],"zero-HP/fallen candidates and missing item geometry cannot claim a pickup");
      if(mode<2)check(X3CoopRuntimeState()->players[zero].life==X3_COOP_FALLEN,
                     "ineligible pickup candidates are not revived by contact fallback");
    }else{
      check(g_ram[slot+2]==6 && g_ram[0x1f28],"eligible native pickup contact starts one refill");
      size_t n=RtlSaveSnapshotToMemory(expected,capacity);marker=0;
      for(size_t i=0;i+8<n;++i)if(!memcmp(expected+i,"X3R1",4)){marker=i;break;}
      check(marker && expected[marker+6]==(mode==2?zero:x) && expected[marker+7]==1,
            "early player-geometry miss and simultaneous contacts retain the single successful collector");
      if(mode==3){
        unsigned positions[2][2];for(unsigned s=0;s<2;++s){positions[s][0]=support_position(s,5);positions[s][1]=support_position(s,8);}
        bool parked=true;
        for(unsigned i=0;i<8;++i){frame(SNES_PAD_RIGHT|(SNES_PAD_RIGHT<<12));
          for(unsigned s=0;s<2;++s)parked=parked && support_position(s,5)==positions[s][0] && support_position(s,8)==positions[s][1];}
        check(parked,"anchor-owned native refill parks the other actor during collision freeze");
        for(unsigned i=0;i<40;++i)frame(0);
        bool released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
        check(released && contact_hp(x)==16 && contact_hp(zero)==20,
              "two touching bodies consume one health pickup and heal only native X");
      }
    }
  }
  check(RtlLoadSnapshotFromMemory(original,size),"pickup fallback probes restore their starting machine");
}
/* Native dropped-life controller checks. The allocator/drop RNG itself is
 * bypassed; no collection result or life award is supplied by this fixture. */
static void native_shared_life_checks(uint8_t *original,size_t size,uint8_t *work,size_t capacity){
  for(unsigned mode=0;mode<4;++mode){
    unsigned zero=health_pickup_approach(original,size,work,20),x=zero^1,slot=0;
    for(unsigned d=0x1518;d<0x1818;d+=48)if(!g_ram[d]){slot=d;break;}
    check(slot!=0,"native life fixture has an unused shared item slot");
    memset(g_ram+slot,0,48);g_ram[slot]=1;g_ram[slot+10]=(uint8_t)(mode==3?4:3);
    g_ram[slot+11]=(uint8_t)(mode?128:0);
    view_word(slot+5,1848);view_word(slot+8,mode?1448:1420);
    g_ram[0x1fb4]=(uint8_t)(mode==2?9:2);
    if(mode==1){
      /* Both native bodies overlap the same drop. */
      view_word(X3_COOP_BODY+5,1848);view_word(X3_COOP_BODY+8,1435);
    }
    for(unsigned i=0;i<12;++i)pickup_native_items(0x00daa0);
    if(g_ram[slot] && g_ram[slot+1]!=4)fprintf(stderr,"life mode%u: seat%u lives=%u item=%u/%u/%u xy=%u,%u geom=%04x\n",
        mode,zero,g_ram[0x1fb4],g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],view_read(slot+5),view_read(slot+8),view_read(slot+0x20));
    check(g_ram[0x1fb4]==(mode==2?9:3),"either native collector grants exactly one shared team life with the original cap");
    check(!g_ram[slot] || g_ram[slot+1]==4,"native life pickup is consumed even at the shared life cap");
    check(contact_hp(zero)==20 && contact_hp(x)==16,"life pickup preserves both actors' personal health");
    check(!g_ram[0x1f28],"life pickup leaves no health-refill freeze");
  }
  check(RtlLoadSnapshotFromMemory(original,size),"native shared-life probes restore their starting machine");(void)capacity;
}
static void authored_health_pickup_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  unsigned seat=health_pickup_approach(original,size,work,20),other=seat^1;
  size_t offset=contact_context_offset(original,size);X3CoopState setup;
  unsigned slot=0;bool frozen=false;size_t middle=0;
  for(unsigned i=0;i<60;++i){
    frame(0);
    for(unsigned d=0x1518;d<0x1818;d+=48)
      if(g_ram[d] && g_ram[d+10]==2 && (g_ram[d+5]|g_ram[d+6]<<8)==1848)slot=d;
    if(slot && g_ram[0x1f28]){frozen=true;middle=RtlSaveSnapshotToMemory(work,capacity);break;}
  }
  if(!frozen)fprintf(stderr,"pickup: slot=%04x phase=%u/%u recipient%u xy=%u,%u HP=%u itemstate=%u/%u camera=%u,%u\n",
      slot,g_ram[0xd1],g_ram[0xd2],seat,coop_x(seat),support_position(seat,8),contact_hp(seat),
      slot?g_ram[slot+2]:0,slot?g_ram[slot+3]:0,X3CoopViewsState()->camera[seat][0],X3CoopViewsState()->camera[seat][1]);
  check(slot && frozen && middle,"authored class02 health pickup starts native refill for the remote actor");
  check((g_ram[slot]&128) && g_ram[slot+2]==6 && g_ram[slot+3]==2,
        "original item owns its native frozen refill state");
  uint8_t *pending=malloc(middle);check(pending!=NULL,"pending refill snapshot buffer allocates");memcpy(pending,work,middle);
  size_t marker=0;for(size_t i=0;i+8<middle;++i)if(!memcmp(pending+i,"X3R1",4)){marker=i;break;}
  check(marker && pending[marker+6]==seat && pending[marker+7]==1 &&
        (pending[marker+4]|pending[marker+5]<<8)==slot,"pending native refill retains its recipient and real item slot");
  const char *output=getenv("X3_COOP_HEALTH_PICKUP_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"native pending refill visual fixture saves");
  unsigned positions[2][2];for(unsigned s=0;s<2;++s){positions[s][0]=support_position(s,5);positions[s][1]=support_position(s,8);}
  bool parked=true;
  for(unsigned i=0;i<8;++i){
    frame((SNES_PAD_RIGHT|SNES_PAD_B)|((SNES_PAD_RIGHT|SNES_PAD_B)<<12));
    parked=parked && g_ram[0x1f28];
    for(unsigned s=0;s<2;++s)parked=parked && support_position(s,5)==positions[s][0] && support_position(s,8)==positions[s][1];
  }
  check(parked,"native refill parks both actors despite movement/jump input while collision is frozen");
  check(RtlLoadSnapshotFromMemory(pending,middle),"native refill movement probe restores");
  for(unsigned i=0;i<48;++i)frame(0);
  bool released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
  check(released && !g_ram[0x1f4f] && g_ram[0xd1]==4 && g_ram[0xd2]==4 && (g_ppu->inidisp&0x8f)==15,
        "remote native refill releases every world freeze without fade or stage exit");
  check(contact_hp(seat)==28 && contact_hp(other)==16,
        "one authored pickup heals only its actual recipient to native Zero28HP");
  size_t completed=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(pending,middle),"pending native refill restores");
  for(unsigned i=0;i<48;++i)frame(0);
  size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,completed,work,repeated,"pending native refill replays to the identical complete machine");
  pending[marker+6]=2;
  check(!RtlLoadSnapshotFromMemory(pending,middle),"invalid native refill recipient is refused before load");
  repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,completed,work,repeated,"refused refill recipient leaves the machine unchanged");pending[marker+6]=(uint8_t)seat;
  check(RtlLoadSnapshotFromMemory(pending,middle),"pending modal-owner probe restores");
  g_ram[0x1f22]=1;
  for(unsigned i=0;i<8;++i)pickup_native_item_loop();
  check(contact_hp(seat)==22 && contact_hp(other)==16,
        "pending item retains its real recipient while a shared scene owns control");

  /* Isolated lifecycle cancellation probes; these do not claim a completed
   * launcher withdrawal or an authored enemy-death route during a freeze. */
  for(unsigned fallen=0;fallen<2;++fallen){
    check(RtlLoadSnapshotFromMemory(pending,middle),"pending retired-recipient probe restores");
    setup=*X3CoopRuntimeState();unsigned retained=setup.players[seat].body[0x27]&127;
    check(fallen?X3CoopFall(&setup,seat):X3CoopWithdraw(&setup,seat),"pending recipient retires through the lifecycle API");
    memcpy(work,pending,middle);
    check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,middle),
          "retired pending-refill context restores");
    check(interp_bridge_add_pre_opcode_hook(0x00dafa,pickup_continuation_probe),"retired-refill continuation probe registers");
    pickup_probe_slot=slot;pickup_probe_pc=0x00dafa;pickup_probe_seat=seat;pickup_probe_seen=false;
    for(unsigned i=0;i<12;++i)frame(0);
    check(pickup_probe_seen,"native cleanup actually borrows the retired recipient before returning to the survivor");
    released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
    check(released && !g_ram[0x1f4f],"retired-recipient native cleanup releases the world freeze");
    check(X3CoopRuntimeState()->players[seat].life==(fallen?X3_COOP_FALLEN:X3_COOP_ABSENT) &&
          (X3CoopRuntimeState()->players[seat].body[0x27]&127)==retained && contact_hp(other)==16,
          "cleanup neither revives nor heals the retired actor or unrelated survivor");
  }
  health_pickup_approach(original,size,work,28);
  g_ram[0x1fb7]=0x80;g_ram[0x1fb8]=g_ram[0x1fb9]=g_ram[0x1fba]=0;
  for(unsigned i=0;i<48;++i)frame(0);
  check(g_ram[0x1fb7]==0x82 && !g_ram[0x1fb8] && !g_ram[0x1fb9] && !g_ram[0x1fba],
        "full-health Zero banks one pickup into the shared native subtank with the original amount");
  check(contact_hp(seat)==28 && contact_hp(other)==16,"shared banking does not change either personal health value");
  free(pending);check(RtlLoadSnapshotFromMemory(original,size),"authored pickup probes restore their starting machine");
}
