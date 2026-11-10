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
  modified[prepared-31]|=4;
  check(!RtlLoadSnapshotFromMemory(modified,prepared),"invalid support rider bits are refused before load");
  size_t after=RtlSaveSnapshotToMemory(modified,capacity);
  same(extra,expected,modified,after,"invalid rider state leaves the machine unchanged");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"support probes restore their starting machine");
}
static void authored_support_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);X3CoopState setup;
  check(g_ram[0x1fae]==5,"authored support fixture is native stage5");
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"authored support actors decode");
  setup.players[1].body[5]=0xd4;setup.players[1].body[6]=4; /*1236*/
  setup.players[1].body[8]=0x8b;setup.players[1].body[9]=5; /*1419*/
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"authored support positioning encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"authored support positioning restores");
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
  unsigned reference_y=support_position(0,8);
  fprintf(stderr,"platform baseline: active=%u state=%u/%u xy=%u,%u player=%u,%u latch=%02x ticks=%u phase=%u/%u cameraY=%u\n",
      g_ram[d],g_ram[d+1],g_ram[d+2],g_ram[d+5]|g_ram[d+6]<<8,g_ram[d+8]|g_ram[d+9]<<8,
      support_position(0,5),reference_y,g_ram[d+0x2c],reference_ticks,g_ram[0xd1],g_ram[0xd2],g_ram[0x1e60]|g_ram[0x1e61]<<8);
  int travel=(int)(g_ram[d+8]|g_ram[d+9]<<8)-1464;
  check(g_ram[d] && (travel>=8 || travel<=-8) && (g_ram[d+0x2c]&1),
        "single-seat reference actually lands on and rides the moving support");
  X3CoopRuntimeEnable(true);
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"authored co-op support precondition restores");
  clock=g_ram[0x9cc];
  for(unsigned i=0;i<45;++i)frame(0);
  unsigned ticks=(g_ram[0x9cc]-clock)&255;
  fprintf(stderr,"authored support: ticks=%u/%u y=%u/%u platform=%u/%u\n",ticks,reference_ticks,
      support_position(0,8),support_position(1,8),g_ram[d+8]|g_ram[d+9]<<8,reference[8]|reference[9]<<8);
  check(ticks==reference_ticks,"authored co-op support preserves native world-update cadence");
  check(!memcmp(reference,g_ram+d,sizeof(reference)),"shared support matches one native platform simulation");
  check(support_position(0,8)==reference_y && support_position(1,8)+8==reference_y,
        "both actors follow the authored moving support at their native standing heights");
  const char *output=getenv("X3_COOP_PLATFORM_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"authored support visual handoff saves");
  for(unsigned i=0;i<30;++i)frame(SNES_PAD_LEFT|SNES_PAD_B);
  unsigned y=g_ram[d+8]|g_ram[d+9]<<8;
  fprintf(stderr,"platform leave: X=%u,%u Zero=%u,%u platform=%u phase=%u/%u hitbox=%02x%02x\n",
      support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8),y,
      g_ram[0xd1],g_ram[0xd2],g_ram[d+0x21],g_ram[d+0x20]);
  /* Native BFAE resolves penetration minus one pixel; fractional support
   * motion can leave either zero or one pixel of contact at this boundary. */
  int zero_gap=(int)support_position(1,8)+29-(int)y;
  size_t riding=RtlSaveSnapshotToMemory(modified,capacity);
  check(support_position(0,8)+21<y-8 && zero_gap>=0 && zero_gap<=1 &&
        (modified[riding-31+(d-0xd18)/64]&3)==2,
        "X can jump off while Zero keeps riding the authored support");
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"P2-first authored support setup restores");
  /* X is outside the real38-pixel contact width; both remain in one view. */
  g_ram[X3_COOP_BODY+5]=0x98;g_ram[X3_COOP_BODY+6]=4; /*1176*/
  for(unsigned i=0;i<45;++i)frame(0);
  y=g_ram[d+8]|g_ram[d+9]<<8;
  zero_gap=(int)support_position(1,8)+29-(int)y;
  check(!memcmp(reference,g_ram+d,sizeof(reference)) && zero_gap>=0 && zero_gap<=1,
        "P2 alone lands, starts and rides the actual support with native movement");
  size_t observed=RtlSaveSnapshotToMemory(modified,capacity);
  check((modified[observed-31+(d-0xd18)/64]&3)==2,
        "authored P2-only ride retains no false X rider");
  output=getenv("X3_COOP_PLATFORM_P2_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"P2-only support visual handoff saves");
}
