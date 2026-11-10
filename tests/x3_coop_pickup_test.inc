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
/* Native dropped-weapon-energy controller: allocation/RNG and route traversal
 * are bypassed, but native eligibility, overlap, refill and cleanup are real. */
static void native_weapon_pickup_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  unsigned zero=health_pickup_approach(original,size,work,20),x=zero^1,slot=0;
  X3CoopState setup=*X3CoopRuntimeState();
  setup.anchor=setup.current=(uint8_t)zero;
  setup.players[x].body[5]=0x38;setup.players[x].body[6]=7;
  setup.players[x].body[8]=0x9b;setup.players[x].body[9]=5; /*1848,1435*/
  setup.players[x].body[0x33]=2; /* First native special weapon. */
  size_t offset=contact_context_offset(original,size);
  X3CoopState initial;check(X3CoopDecode(&initial,original+offset,X3CoopEncodedSize()),"native safe Zero location decodes");
  memcpy(setup.players[zero].body+5,initial.players[zero].body+5,5);
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),
        "Zero-led remote X weapon-energy approach restores");
  /* This fixture changes the leading actor without a native death/door
   * handoff. Prepare its whole projected context, including its real palette. */
  memcpy(g_ram+X3_COOP_BODY,setup.players[zero].body,X3_COOP_BODY_BYTES);
  memcpy(g_ram+X3_COOP_AUX,setup.players[zero].auxiliaries,X3_COOP_AUX_BYTES);
  memcpy(g_ram+X3_COOP_SHOTS,setup.players[zero].shots,X3_COOP_SHOT_BYTES);
  memcpy(g_ram+X3_COOP_PALETTE,setup.players[zero].palette,X3_COOP_PALETTE_BYTES);
  g_ram[0x1f1e]=setup.players[zero].shot_command;g_ram[0x1fd2]=28;g_ram[0xa1]=1;
  X3CoopViewsConfigure(true);X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=s==x?1720:256;views.camera[s][1]=s==x?1280:1024;
    views.limits[s][1]=views.current[s][1]=8192;views.limits[s][2]=views.current[s][2]=views.camera[s][1];
    views.limits[s][3]=views.current[s][3]=views.camera[s][1];}
  X3CoopViewsSetState(&views);
  for(unsigned d=0x1518;d<0x1818;d+=48)if(!g_ram[d]){slot=d;break;}
  check(slot!=0,"native weapon-energy fixture has an unused item slot");
  memset(g_ram+slot,0,48);g_ram[slot]=1;g_ram[slot+10]=1;g_ram[slot+11]=128;
  view_word(slot+5,1848);view_word(slot+8,1448);
  memset(g_ram+0x1fbb,0,18);g_ram[0x1fbc]=0x48; /* Unlocked,8/28 energy. */
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  X3CoopRuntimeEnable(false);memcpy(g_ram+X3_COOP_BODY,setup.players[x].body,X3_COOP_BODY_BYTES);
  g_ram[0x1fd2]=16;
  /* Only the original single-body reference needs X's native camera. The
   * paired machine retains the actual leading Zero's upper-room camera. */
  view_word(0x1e5d,1720);view_word(0x1e7a,1720);view_word(0x1e60,1280);view_word(0x1e7c,1280);
  view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6a,1280);view_word(0x1e6c,1280);
  view_word(0x1e6e,0);view_word(0x1e70,8192);view_word(0x1e78,1280);view_word(0x1e7e,1280);
  for(unsigned i=0;i<12 && !g_ram[0x1f28];++i)pickup_native_items(0x00daa0);
  if(!g_ram[0x1f28])fprintf(stderr,"native weapon reference: item=%u/%u/%u active=%u xy=%u,%u selector=%u energy=%02x geom=%04x\n",
      g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],g_ram[slot],view_read(slot+5),view_read(slot+8),g_ram[0xa0b],g_ram[0x1fbc],view_read(slot+0x20));
  check(g_ram[slot+2]==6 && g_ram[0x1f28],"single-body native X really starts weapon-energy refill");
  uint8_t native_item[48];memcpy(native_item,g_ram+slot,sizeof(native_item));
  for(unsigned mode=0;mode<3;++mode){
    X3CoopState probe=setup;
    if(mode==0){
      probe.players[x].body[5]=0;probe.players[x].body[6]=1;
      probe.players[x].body[8]=0x20;probe.players[x].body[9]=4;
    }else check(mode==1?X3CoopFall(&probe,x):X3CoopWithdraw(&probe,x),
               "weapon pickup ineligible-X lifecycle precondition applies");
    probe.players[zero].body[5]=0x38;probe.players[zero].body[6]=7;
    probe.players[zero].body[8]=0x9b;probe.players[zero].body[9]=5;
    memcpy(expected,work,prepared);
    check(X3CoopEncode(&probe,expected+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(expected,prepared),
          "Zero-only weapon-energy contact precondition restores");
    memcpy(g_ram+X3_COOP_BODY,probe.players[zero].body,X3_COOP_BODY_BYTES);
    X3CoopViewState nearby=views;nearby.camera[zero][0]=1720;nearby.camera[zero][1]=1280;
    nearby.limits[zero][2]=nearby.current[zero][2]=nearby.limits[zero][3]=nearby.current[zero][3]=1280;
    X3CoopViewsSetState(&nearby);
    for(unsigned i=0;i<12;++i)pickup_native_items(0x00daa0);
    check(pickup_single_overlap(slot,&probe.players[zero]),"native Zero hitbox actually overlaps the rejected weapon-energy item");
    if(!g_ram[slot] || g_ram[slot+2]==6 || g_ram[0x1f28] || g_ram[0x1fbc]!=0x48)
      fprintf(stderr,"rejected weapon mode%u: active=%u item=%u/%u/%u freeze=%u energy=%02x\n",
          mode,g_ram[slot],g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],g_ram[0x1f28],g_ram[0x1fbc]);
    check(g_ram[slot] && g_ram[slot+2]!=6 && !g_ram[0x1f28] && g_ram[0x1fbc]==0x48,
          "Zero cannot collect weapon energy for a distant fallen or withdrawn X");
    check(X3CoopRuntimeState()->players[x].life==probe.players[x].life,
          "weapon energy does not revive or rejoin an ineligible X");
  }
  check(RtlLoadSnapshotFromMemory(work,prepared),"Zero-led weapon-energy comparison restores");
  for(unsigned i=0;i<12 && !g_ram[0x1f28];++i)pickup_native_items(0x00daa0);
  if(!g_ram[0x1f28])fprintf(stderr,"weapon pickup: seat%u anchor%u item=%u/%u/%u energy=%02x\n",
      x,X3CoopRuntimeState()->anchor,g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],g_ram[0x1fbc]);
  check(g_ram[slot+2]==6 && g_ram[0x1f28],"remote X collects native weapon energy while Zero leads the world");
  check(!memcmp(native_item,g_ram+slot,sizeof(native_item)),"remote weapon-energy claim matches one native X item update");
  size_t pending=RtlSaveSnapshotToMemory(work,capacity);
  const char *output=getenv("X3_COOP_WEAPON_PICKUP_OUTPUT");
  if(output && *output){
    /* Teleport/anchor preparation changed core state without advancing the
     * native presentation queue. Let it catch up before a visual handoff. */
    for(unsigned i=0;i<4;++i)frame(0);
    check(g_ram[0x1f28] && RtlSaveSnapshot(output),"native pending weapon-energy visual fixture saves");
    check(RtlLoadSnapshotFromMemory(work,pending),"visual preparation restores the original pending refill");
  }
  setup=*X3CoopRuntimeState();check(X3CoopWithdraw(&setup,x),"pending weapon-energy recipient withdraws through the lifecycle API");
  memcpy(expected,work,pending);
  check(X3CoopEncode(&setup,expected+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(expected,pending),
        "withdrawn weapon-energy recipient restores");
  for(unsigned i=0;i<12;++i)frame(0);
  bool cancelled=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)cancelled=cancelled && !g_ram[a];
  check(cancelled && !g_ram[0x1f4f] && (g_ram[0x1fbc]&63)==8 &&
        X3CoopRuntimeState()->players[x].life==X3_COOP_ABSENT,
        "retired weapon-energy cleanup releases the freeze without energy or revival");
  check(RtlLoadSnapshotFromMemory(work,pending),"native weapon-energy refill resumes from its real pending owner");
  unsigned positions[2][2];for(unsigned s=0;s<2;++s){positions[s][0]=support_position(s,5);positions[s][1]=support_position(s,8);}
  bool parked=true;
  for(unsigned i=0;i<8;++i){frame(SNES_PAD_RIGHT|(SNES_PAD_RIGHT<<12));
    for(unsigned s=0;s<2;++s)parked=parked && support_position(s,5)==positions[s][0] && support_position(s,8)==positions[s][1];}
  check(parked,"weapon-energy freeze parks the noncollector while native refill continues");
  for(unsigned i=0;i<40;++i)frame(0);
  bool released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
  if(!released || g_ram[0x1f4f] || g_ram[0x1fbc]!=0xd0)fprintf(stderr,
      "weapon complete: energy=%02x freeze=%u/%u item=%u/%u/%u count=%u owner%u selections=%u/%u phase=%u/%u\n",
      g_ram[0x1fbc],g_ram[0x1f28],g_ram[0x1f4f],g_ram[slot+1],g_ram[slot+2],g_ram[slot+3],g_ram[slot+0x27],
      X3CoopRuntimeState()->current,X3CoopRuntimeState()->players[x].body[0x33],
      X3CoopRuntimeState()->players[zero].body[0x33],g_ram[0xd1],g_ram[0xd2]);
  /* Native D5D3 ORs C000 into the updated8->16 energy word. */
  check(released && !g_ram[0x1f4f] && g_ram[0x1fbc]==0xd0,
        "remote X receives exactly eight native energy units and releases the shared freeze");
  bool isolated=true;for(unsigned a=0x1fbb;a<0x1fcd;++a)if(a!=0x1fbc && g_ram[a])isolated=false;
  check(isolated,"selected native energy refill leaves every other weapon inventory unchanged");
  check(contact_hp(x)==16 && contact_hp(zero)==20 && X3CoopRuntimeState()->players[x].body[0x33]==2 &&
        !X3CoopRuntimeState()->players[zero].body[0x33],"weapon pickup preserves personal HP and Zero has no special selection");
  size_t complete=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(work,pending),"pending remote weapon-energy refill restores");
  for(unsigned i=0;i<8;++i)frame(SNES_PAD_RIGHT|(SNES_PAD_RIGHT<<12));
  for(unsigned i=0;i<40;++i)frame(0);
  size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,complete,work,repeated,"remote weapon-energy refill replays to the identical complete machine");
  check(RtlLoadSnapshotFromMemory(original,size),"native weapon-energy probes restore their starting machine");
}
/* Two actual native drop controllers in one pool pass, with different refill
 * lengths. Drop allocation and route traversal are explicit QA preconditions. */
static void competing_pickup_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity,unsigned mode){
  bool weapon=mode>=2,reverse=mode&1;
  unsigned zero=health_pickup_approach(original,size,work,20),x=zero^1,slots[2]={0};
  X3CoopState setup=*X3CoopRuntimeState();size_t offset=contact_context_offset(original,size);
  for(unsigned s=0;s<2;++s){
    unsigned px=s==x?1816:1848;
    setup.players[s].body[5]=(uint8_t)px;setup.players[s].body[6]=(uint8_t)(px>>8);
    setup.players[s].body[8]=0x5a;setup.players[s].body[9]=5; /*1370, clear of both items*/
  }
  setup.players[x].body[0x27]=8;
  setup.players[x].body[0x33]=(uint8_t)(weapon?2:0);
  memcpy(work,original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),
        "two-collector damaged-body approach restores");
  memcpy(g_ram+X3_COOP_BODY,setup.players[x].body,X3_COOP_BODY_BYTES);
  X3CoopViewsConfigure(true);X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned s=0;s<2;++s){views.camera[s][0]=1720;views.camera[s][1]=1280;
    views.limits[s][1]=views.current[s][1]=8192;views.limits[s][2]=views.current[s][2]=1280;
    views.limits[s][3]=views.current[s][3]=1280;}
  X3CoopViewsSetState(&views);
  view_word(0x1e5d,1720);view_word(0x1e7a,1720);view_word(0x1e60,1280);view_word(0x1e7c,1280);
  view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6a,1280);view_word(0x1e6c,1280);
  view_word(0x1e6e,0);view_word(0x1e70,8192);view_word(0x1e78,1280);view_word(0x1e7e,1280);
  for(unsigned d=0x1518,n=0;d<0x1818 && n<2;d+=48)if(!g_ram[d])slots[n++]=d;
  check(slots[0] && slots[1],"competing native drops have two unused item slots");
  memset(g_ram+0x1fbb,0,18);if(weapon)g_ram[0x1fbc]=0x48;
  for(unsigned i=0;i<2;++i){unsigned d=slots[i],which=i^reverse;
    memset(g_ram+d,0,48);g_ram[d]=1;g_ram[d+10]=(uint8_t)(weapon && !which?1:2);
    g_ram[d+11]=(uint8_t)(which?128:129);view_word(d+5,which?1848:1816);view_word(d+8,1448);}
  for(unsigned i=0;i<12 && (g_ram[slots[0]+2]!=2 || g_ram[slots[1]+2]!=2);++i)pickup_native_items(0x00daa0);
  check(g_ram[slots[0]+2]==2 && g_ram[slots[1]+2]==2 && !g_ram[0x1f28],
        "both native drops finish landing before either collector touches them");
  setup=*X3CoopRuntimeState();
  for(unsigned s=0;s<2;++s){setup.players[s].body[8]=0x9b;setup.players[s].body[9]=5;}
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,prepared),
        "both native collectors reach their separate drops in the same pool pass");
  memcpy(g_ram+X3_COOP_BODY,setup.players[x].body,X3_COOP_BODY_BYTES);
  pickup_native_items(0x00daa0);
  unsigned simultaneous=(g_ram[slots[0]+2]==6)+(g_ram[slots[1]+2]==6);
  size_t pending=RtlSaveSnapshotToMemory(work,capacity);
  bool premature=false;
  for(unsigned i=0;i<96;++i){frame(0);
    for(unsigned k=0;k<2;++k)if(g_ram[slots[k]] && g_ram[slots[k]+1]==2 &&
        g_ram[slots[k]+2]==6 && g_ram[slots[k]+3]==2 && !g_ram[0x1f28])premature=true;}
  fprintf(stderr,"competing pickup mode%u: claims=%u premature=%u HP=%u/%u energy=%02x states=%u/%u,%u/%u freeze=%u\n",
      mode,simultaneous,premature,contact_hp(x),contact_hp(zero),g_ram[0x1fbc],g_ram[slots[0]+1],g_ram[slots[0]+2],
      g_ram[slots[1]+1],g_ram[slots[1]+2],g_ram[0x1f28]);
  check(simultaneous==1 && !premature,"one native refill owns the shared freeze until its cleanup finishes");
  check(contact_hp(x)==(weapon?8:10) && contact_hp(zero)==28 && g_ram[0x1fbc]==(weapon?0xca:0),
        "two distinct native resource drops refill each original collector exactly once");
  bool released=true;for(unsigned a=0x1f25;a<=0x1f2b;++a)released=released && !g_ram[a];
  check(released && !g_ram[0x1f4f],"both differently sized refills finish without a stale shared freeze");
  size_t complete=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(work,pending),"pending competing-item sequence restores");
  for(unsigned i=0;i<96;++i)frame(0);
  size_t repeated=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,complete,work,repeated,"queued native resource pickups replay to the identical complete machine");
  check(RtlLoadSnapshotFromMemory(original,size),"competing native pickup probes restore their starting machine");
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
