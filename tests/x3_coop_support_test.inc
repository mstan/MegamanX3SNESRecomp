/* Isolated real-cartridge solid/carry contracts. The geometry record below
 * deliberately supplies no authored platform AI or spawn coverage. */
static unsigned support_position(unsigned seat,unsigned axis) {
  const X3CoopState *s=X3CoopRuntimeState();
  const uint8_t *p=s->current==seat?g_ram+X3_COOP_BODY:s->players[seat].body;
  return p[axis]|p[axis+1]<<8;
}
static void support_move(unsigned slot,int dx,int dy) {
  unsigned x=g_ram[slot+5]|g_ram[slot+6]<<8,y=g_ram[slot+8]|g_ram[slot+9]<<8;
  g_ram[slot+0x22]=(uint8_t)x;g_ram[slot+0x23]=(uint8_t)(x>>8);
  g_ram[slot+0x24]=(uint8_t)y;g_ram[slot+0x25]=(uint8_t)(y>>8);
  x+=dx;y+=dy;
  g_ram[slot+5]=(uint8_t)x;g_ram[slot+6]=(uint8_t)(x>>8);
  g_ram[slot+8]=(uint8_t)y;g_ram[slot+9]=(uint8_t)(y>>8);
}
static void support_geometry(unsigned slot,unsigned x,unsigned y) {
  contact_geometry(slot,x,y);
  /* Native class37 selects06:C5D4 = {0,0,32,16}; do not approximate
   * its top with the player's narrower, vertically offset hurt box. */
  g_ram[slot+0x20]=0xd4;g_ram[slot+0x21]=0xc5;g_ram[slot+0x2c]=0x80;
}
static void native_support_checks(uint8_t *snapshot,size_t size,uint8_t *modified,uint8_t *extra,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);X3CoopState setup;
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"support precondition decodes");
  setup.players[1].body[5]=setup.players[0].body[5];
  setup.players[1].body[6]=setup.players[0].body[6];
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"two-rider position precondition encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"two-rider native support fixture restores");
  unsigned x=support_position(0,5),y=support_position(0,8)+28;
  support_geometry(0xd18,x,y);
  contact_native_call(0xd18,0x04bfae);
  unsigned before[4]={support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8)};
  support_move(0xd18,5,-3);contact_native_call(0xd18,0x04c07f);
  check(support_position(0,5)==before[0]+5 && support_position(0,8)==before[1]-3 &&
        support_position(1,5)==before[2]+5 && support_position(1,8)==before[3]-3,
        "one native carry moves both classified riders by the same platform delta");
  /* Leave only Zero on the first support, then land X on a different one. */
  unsigned moved=support_position(0,5)+64;
  g_ram[X3_COOP_BODY+5]=(uint8_t)moved;g_ram[X3_COOP_BODY+6]=(uint8_t)(moved>>8);
  contact_native_call(0xd18,0x04bfae);
  unsigned platform_y=g_ram[0xd18+8]|g_ram[0xd18+9]<<8;
  support_geometry(0xd58,moved,platform_y);
  contact_native_call(0xd58,0x04bfae);
  before[0]=support_position(0,5);before[1]=support_position(0,8);
  before[2]=support_position(1,5);before[3]=support_position(1,8);
  support_move(0xd18,2,1);support_move(0xd58,-4,2);
  size_t prepared=RtlSaveSnapshotToMemory(modified,capacity);
  contact_native_call(0xd18,0x04c07f);contact_native_call(0xd58,0x04c07f);
  check(support_position(0,5)==before[0]-4 && support_position(0,8)==before[1]+2 &&
        support_position(1,5)==before[2]+2 && support_position(1,8)==before[3]+1,
        "separate supports keep independent rider histories without cross-carry");
  check(RtlLoadSnapshotFromMemory(modified,prepared),"classified support histories restore");
  contact_native_call(0xd18,0x04c07f);contact_native_call(0xd58,0x04c07f);
  check(support_position(0,5)==before[0]-4 && support_position(1,5)==before[2]+2,
        "restored histories carry the same two actors");
  size_t expected=RtlSaveSnapshotToMemory(extra,capacity);
  modified[prepared-X3_COOP_MENU_STATE_BYTES-31]|=4;
  check(!RtlLoadSnapshotFromMemory(modified,prepared),"invalid support rider bits are refused before load");
  size_t after=RtlSaveSnapshotToMemory(modified,capacity);
  same(extra,expected,modified,after,"invalid rider state leaves the machine unchanged");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"support probes restore their starting machine");
}
static void authored_support_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);X3CoopState setup;
  check(g_ram[0x1fae]==5,"authored support fixture is native stage5");
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"authored support actors decode");
  unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;setup.current=setup.anchor=(uint8_t)x;
  setup.players[zero].body[5]=0xd4;setup.players[zero].body[6]=4; /*1236*/
  setup.players[zero].body[8]=0x8b;setup.players[zero].body[9]=5; /*1419*/
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"authored support positioning encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"authored support positioning restores");
  door_install_player(&setup);
  g_ram[X3_COOP_BODY+5]=0xbc;g_ram[X3_COOP_BODY+6]=4; /*1212*/
  g_ram[X3_COOP_BODY+8]=0x93;g_ram[X3_COOP_BODY+9]=5; /*1427*/
  /* QA teleport into the actual upper-room band. Camera admission/stream
   * frontiers are bypassed here; the support itself is spawned from ROM. */
  const unsigned camera_addresses[]={0x1e5d,0x1e7a,0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
  for(unsigned i=0;i<8;++i) {
    unsigned value=i<2?1096:1280;
    g_ram[camera_addresses[i]]=(uint8_t)value;g_ram[camera_addresses[i]+1]=(uint8_t)(value>>8);
  }
  /* A current independent fixture retains its original private room. Match
   * those cameras to this explicit teleport before asking the union allocator
   * to admit a different room's authored support. */
  if(X3CoopViewsIndependent()){
    X3CoopViewState views=*X3CoopViewsState();
    views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){
      views.camera[seat][0]=1096;views.camera[seat][1]=1280;
      views.limits[seat][0]=views.current[seat][0]=0;
      views.limits[seat][1]=views.current[seat][1]=8192;
      views.limits[seat][2]=views.current[seat][2]=1280;
      views.limits[seat][3]=views.current[seat][3]=1280;
    }
    X3CoopViewsSetState(&views);
  }
  g_ram[0]=0xc0;g_ram[1]=4; /* native spawn column1216 */
  g_ram[2]=0xe0;g_ram[3]=4;g_ram[4]=0x40;g_ram[5]=1; /* top1248,height320 */
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x00ded3),"native upper-room event column spawns");
  check(g_cpu.S==resume.S,"native spawn precondition preserves its guest stack");
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  unsigned d=0;
  for(unsigned a=0xd18;a<0x10d8;a+=64)if(g_ram[a] && g_ram[a+10]==0x26)d=a;
  check(d && (g_ram[d+5]|g_ram[d+6]<<8)==1224 && (g_ram[d+8]|g_ram[d+9]<<8)==1464,
        "ROM event3C:E12C creates its authored class26 support at1224,1464");
  size_t prepared=RtlSaveSnapshotToMemory(snapshot,capacity);
  X3CoopRuntimeEnable(false);
  unsigned clock=g_ram[0x9cc];
  for(unsigned i=0;i<45;++i)frame(0);
  unsigned reference_ticks=(g_ram[0x9cc]-clock)&255;
  uint8_t reference[64];memcpy(reference,g_ram+d,sizeof(reference));
  unsigned reference_y=support_position(x,8);
  fprintf(stderr,"platform baseline: active=%u state=%u/%u xy=%u,%u player=%u,%u latch=%02x ticks=%u phase=%u/%u cameraY=%u\n",
      g_ram[d],g_ram[d+1],g_ram[d+2],g_ram[d+5]|g_ram[d+6]<<8,g_ram[d+8]|g_ram[d+9]<<8,
      support_position(x,5),reference_y,g_ram[d+0x2c],reference_ticks,g_ram[0xd1],g_ram[0xd2],g_ram[0x1e60]|g_ram[0x1e61]<<8);
  int travel=(int)(g_ram[d+8]|g_ram[d+9]<<8)-1464;
  check(g_ram[d] && (travel>=8 || travel<=-8) && (g_ram[d+0x2c]&1),
        "single-seat reference actually lands on and rides the moving support");
  X3CoopRuntimeEnable(true);
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"authored co-op support precondition restores");
  clock=g_ram[0x9cc];
  for(unsigned i=0;i<45;++i)frame(0);
  unsigned ticks=(g_ram[0x9cc]-clock)&255;
  fprintf(stderr,"authored support: ticks=%u/%u y=%u/%u platform=%u/%u\n",ticks,reference_ticks,
      support_position(x,8),support_position(zero,8),g_ram[d+8]|g_ram[d+9]<<8,reference[8]|reference[9]<<8);
  check(ticks==reference_ticks,"authored co-op support preserves native world-update cadence");
  check(!memcmp(reference,g_ram+d,sizeof(reference)),"shared support matches one native platform simulation");
  check(support_position(x,8)==reference_y && support_position(zero,8)+8==reference_y,
        "both actors follow the authored moving support at their native standing heights");
  const char *output=getenv("X3_COOP_PLATFORM_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"authored support visual handoff saves");
  unsigned y;int zero_gap;
  for(unsigned departing=0;departing<2;++departing){
    check(RtlLoadSnapshotFromMemory(snapshot,prepared),"native rider departure restores its original support approach");
    X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)departing;
    X3CoopRuntimeEnable(false);door_install_player(&solo);
    uint16_t native_landing[45][2];bool native_landed=false;
    for(unsigned i=0;i<45;++i){frame(0);native_landing[i][0]=(uint16_t)view_read(X3_COOP_BODY+5);native_landing[i][1]=(uint16_t)view_read(X3_COOP_BODY+8);native_landed=native_landed || (g_ram[d+0x2c]&1);}
    check(native_landed,"original rider reference includes the actual moving-frame landing");
    for(unsigned i=0;i<5;++i)frame(SNES_PAD_LEFT|SNES_PAD_B);
    uint8_t native_velocity[4];memcpy(native_velocity,g_ram+X3_COOP_BODY+0x1a,4);
    unsigned native_x=view_read(X3_COOP_BODY+5),native_y=view_read(X3_COOP_BODY+8);
    check(RtlLoadSnapshotFromMemory(snapshot,prepared),"paired rider departure restores the matched original approach");
    for(unsigned i=0;i<45;++i){frame(0);if(support_position(departing,5)!=native_landing[i][0] || support_position(departing,8)!=native_landing[i][1])check(false,"co-op landing and same-frame carry match the original native rider on every approach/update frame");}
    check(true,"either native rider matches the original landing and same-frame carry throughout the authored support approach");
    for(unsigned i=0;i<5;++i)frame((uint32_t)(SNES_PAD_LEFT|SNES_PAD_B)<<(departing*12));
    const X3CoopState *s=X3CoopRuntimeState();const uint8_t *body=s->current==departing?g_ram+X3_COOP_BODY:s->players[departing].body;
    check(support_position(departing,5)==native_x && support_position(departing,8)==native_y && !memcmp(native_velocity,body+0x1a,4),
        "either rider jumps off with the original native position and horizontal/vertical momentum");
    for(unsigned i=5;i<30;++i)frame((uint32_t)(SNES_PAD_LEFT|SNES_PAD_B)<<(departing*12));
    y=view_read(d+8);unsigned other=departing^1,feet=other==x?21:29,jump_feet=departing==x?21:29;
    int gap=(int)support_position(other,8)+(int)feet-(int)y;
    size_t riding=RtlSaveSnapshotToMemory(modified,capacity);
    check(support_position(departing,8)+jump_feet<y-8 && gap>=0 && gap<=1 &&
        (modified[riding-X3_COOP_MENU_STATE_BYTES-31+(d-0xd18)/64]&3)==(1u<<other),
        "jumping off releases only the departing rider while the partner stays on the native moving support");
  }
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"P2-first authored support setup restores");
  /* X is outside the real38-pixel contact width; both remain in one view. */
  g_ram[X3_COOP_BODY+5]=0x98;g_ram[X3_COOP_BODY+6]=4; /*1176*/
  for(unsigned i=0;i<45;++i)frame(0);
  y=g_ram[d+8]|g_ram[d+9]<<8;
  zero_gap=(int)support_position(zero,8)+29-(int)y;
  check(!memcmp(reference,g_ram+d,sizeof(reference)) && zero_gap>=0 && zero_gap<=1,
        "P2 alone lands, starts and rides the actual support with native movement");
  size_t observed=RtlSaveSnapshotToMemory(modified,capacity);
  check((modified[observed-X3_COOP_MENU_STATE_BYTES-31+(d-0xd18)/64]&3)==(1u<<zero),
        "authored P2-only ride retains no false X rider");
  output=getenv("X3_COOP_PLATFORM_P2_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"P2-only support visual handoff saves");
}

/* Two distinct stage5 ROM records,3C:E12C and3C:E1BB, live in vertically
 * separated rooms. Teleports/camera prerequisites bypass admission traversal. */
static void native_separate_authored_support_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,size_t capacity){
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==5,"separate supports load the actual stage5 roster");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    unsigned platform_x[2]={1224,1480},platform_y[2]={1464,1944},camera_y[2]={1280,1792};
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=platform_x[seat],y=platform_y[seat]-(setup.players[seat].character==X3_COOP_ZERO?45:37);
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x88]=255;b[0x8a]=0;}
    memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"separate authored support approach encodes");door_install_player(&setup);
    X3CoopViewsConfigure(true);X3CoopViewState views=*X3CoopViewsState();views.independent=views.initialized=1;views.stage=5;views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=(uint16_t)(platform_x[seat]-128);views.camera[seat][1]=(uint16_t)camera_y[seat];views.limits[seat][0]=views.current[seat][0]=0;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=(uint16_t)camera_y[seat];views.limits[seat][3]=views.current[seat][3]=(uint16_t)camera_y[seat];}X3CoopViewsSetState(&views);
    view_word(0x1e5d,views.camera[setup.anchor][0]);view_word(0x1e7a,views.camera[setup.anchor][0]);view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6e,0);view_word(0x1e70,8192);const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};for(unsigned k=0;k<6;++k)view_word(vertical[k],camera_y[setup.anchor]);
    for(unsigned seat=0;seat<2;++seat){view_word(0,platform_x[seat]&~31u);view_word(2,camera_y[seat]-32);view_word(4,320);cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
      check(interp_bridge_run(&g_cpu,0x00ded3) && g_cpu.S==resume.S,"actual separated native support column allocates with balanced stack");g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);}
    unsigned slot[2]={0,0};for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x26)for(unsigned seat=0;seat<2;++seat)if(view_read(d+5)==platform_x[seat] && view_read(d+8)==platform_y[seat])slot[seat]=d;
    check(slot[0] && slot[1] && slot[0]!=slot[1],"original stage5 records create two distinct native moving supports");n=RtlSaveSnapshotToMemory(base,capacity);
    uint16_t position[2][45][2];uint8_t record[2][64],child_record[2][32];
    for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(base,n),"separated original rider comparison restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);X3CoopRuntimeEnable(false);g_ram[slot[seat^1]]=0;view_word(0x1e5d,platform_x[seat]-128);view_word(0x1e7a,platform_x[seat]-128);for(unsigned k=0;k<6;++k)view_word(vertical[k],camera_y[seat]);
      for(unsigned f=0;f<45;++f){frame(0);position[seat][f][0]=(uint16_t)view_read(X3_COOP_BODY+5);position[seat][f][1]=(uint16_t)view_read(X3_COOP_BODY+8);}memcpy(record[seat],g_ram+slot[seat],64);unsigned child=view_read(slot[seat]+0x3e);check(child>=0x1800 && child+32<=0x1e00 && g_ram[child] && g_ram[child+10]==0x1e && view_read(child+0x1b)==slot[seat],"each original moving support owns its actual native trail child");memcpy(child_record[seat],g_ram+child,32);
      fprintf(stderr,"separate original roster%u seat%u object%04x active%u state%u/%u variant%02x xy%u,%u rider%u,%u HP%u latch%02x camera%u bounds%u/%u\n",roster,seat,slot[seat],g_ram[slot[seat]],g_ram[slot[seat]+1],g_ram[slot[seat]+2],g_ram[slot[seat]+11],view_read(slot[seat]+5),view_read(slot[seat]+8),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),g_ram[X3_COOP_BODY+0x27]&127,g_ram[slot[seat]+0x2c],view_read(0x1e60),view_read(0x1e6a),view_read(0x1e6c));
      check(g_ram[slot[seat]] && (g_ram[slot[seat]+0x2c]&1) && view_read(slot[seat]+8)!=platform_y[seat],"each original support reference actually moves with its rider");}
    check(RtlLoadSnapshotFromMemory(base,n),"paired separated supports restore their original world");unsigned tick=g_ram[0x9cc];
    for(unsigned f=0;f<45;++f){frame(0);for(unsigned seat=0;seat<2;++seat)if(support_position(seat,5)!=position[seat][f][0] || support_position(seat,8)!=position[seat][f][1])check(false,"separate moving supports retain each original rider position on every native frame");}
    check((uint8_t)(g_ram[0x9cc]-tick)==45,"separate support rooms advance the shared world once");size_t end=RtlSaveSnapshotToMemory(work,capacity);
    for(unsigned seat=0;seat<2;++seat){/*07:E649 stores an allocated shared child slot in3E. With two real supports, slot addresses differ; validate its complete target and parent instead. */unsigned child=view_read(slot[seat]+0x3e);check(child>=0x1800 && child+32<=0x1e00 && g_ram[child] && g_ram[child+10]==0x1e && view_read(child+0x1b)==slot[seat] && !memcmp(child_record[seat],g_ram+child,32),"each separated support retains its original native child behavior and actual parent identity");record[seat][0x3e]=g_ram[slot[seat]+0x3e];record[seat][0x3f]=g_ram[slot[seat]+0x3f];check(!memcmp(record[seat],g_ram+slot[seat],64),"each separated support matches one original native simulation after resolving its allocated child pointer");check((work[end-X3_COOP_MENU_STATE_BYTES-31+(slot[seat]-0xd18)/64]&3)==(1u<<seat),"two authored supports retain independent actual rider histories");}
    fprintf(stderr,"separate authored supports roster%u slots%04x/%04x riderY%u/%u platformY%u/%u\n",roster,slot[0],slot[1],support_position(0,8),support_position(1,8),view_read(slot[0]+8),view_read(slot[1]+8));
  }
}
