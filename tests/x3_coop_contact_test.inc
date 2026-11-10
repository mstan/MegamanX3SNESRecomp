/* Contract probes against the cartridge's real collision routines. These use
 * isolated geometry fixtures, not an assertion of stage/AI coverage. */
static size_t contact_context_offset(const uint8_t *snapshot,size_t size) {
  for(size_t i=size-4;i>0;--i)if(!memcmp(snapshot+i,"X3C2",4))return i;
  check(0,"actor context is present");return 0;
}
static void contact_geometry(unsigned slot,unsigned x,unsigned y) {
  /* Both records use the same native geometry-table contract. Enemy motion,
   * AI, spawning and retirement are deliberately not manufactured here. */
  memcpy(g_ram+slot,g_ram+X3_COOP_BODY,64);
  g_ram[slot]=1;g_ram[slot+0xe]=1;g_ram[slot+0x26]=2;g_ram[slot+0x27]=8;
  g_ram[slot+5]=(uint8_t)x;g_ram[slot+6]=(uint8_t)(x>>8);
  g_ram[slot+8]=(uint8_t)y;g_ram[slot+9]=(uint8_t)(y>>8);
  g_ram[slot+0x30]=0;g_ram[0x1f1d]=0;
}
static void contact_native_call(unsigned slot,unsigned pc) {
  X3CoopRuntimeFrame();
  uint16_t stack=g_cpu.S;
  g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=(uint16_t)slot;
  g_cpu.P=(uint8_t)((g_cpu.P|0x30)&~0x40);cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc),"native contact returns");
  check(g_cpu.S==stack,"paired contact preserves the real guest stack");
}
static unsigned contact_hp(unsigned seat) {
  const X3CoopState *s=X3CoopRuntimeState();
  return (s->current==seat?g_ram[X3_COOP_BODY+0x27]:s->players[seat].body[0x27])&127;
}
static void native_contact_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);
  check(offset+X3CoopEncodedSize()<=size,"actor chunk fits snapshot");
  X3CoopState setup;
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"native fixture context decodes");
  unsigned primary_x=setup.players[0].body[5]|setup.players[0].body[6]<<8;
  unsigned partner_x=setup.players[1].body[5]|setup.players[1].body[6]<<8;
  unsigned y=setup.players[1].body[8]|setup.players[1].body[9]<<8;
  check(primary_x!=partner_x,"native fixture separates both bodies");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"isolated P2 contact fixture restores");
  contact_geometry(0xd18,partner_x,y);contact_native_call(0xd18,0x04cb31);
  check((g_cpu.A&255)!=0 && contact_hp(0)==16 && contact_hp(1)==14,
        "native enemy body contact reaches P2 without damaging distant X");

  memcpy(modified,snapshot,size);
  setup.players[1].body[5]=setup.players[0].body[5];
  setup.players[1].body[6]=setup.players[0].body[6];
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"overlap fixture encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"overlapping bodies restore");
  contact_geometry(0xd18,primary_x,y);contact_native_call(0xd18,0x04cb31);
  check(contact_hp(0)==14 && contact_hp(1)==14,"solid enemy contact evaluates each eligible body once");

  check(RtlLoadSnapshotFromMemory(modified,size),"hostile-shot overlap fixture restores");
  contact_geometry(0x1318,primary_x,y);contact_native_call(0x1318,0x04cb31);
  check(contact_hp(0)==14 && contact_hp(1)==16,"consumed hostile shot does not damage both players");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"native projectile-update fixture restores");
  for(int i=0;i<3;++i)frame(SNES_PAD_Y<<12);
  const uint8_t *shot=X3CoopRuntimeState()->players[1].shots;
  unsigned index=0;
  while(index<X3_COOP_SHOT_BYTES && !shot[index])index+=64;
  check(index<X3_COOP_SHOT_BYTES,"native partner creates a buster projectile");
  unsigned before=shot[index+5]|shot[index+6]<<8;
  for(int i=0;i<3;++i)frame(0);
  shot=X3CoopRuntimeState()->players[1].shots;
  check(shot[index] && (unsigned)(shot[index+5]|shot[index+6]<<8)!=before,
        "partner projectile advances through its native updater");
  contact_geometry(0xd18,shot[index+5]|shot[index+6]<<8,shot[index+8]|shot[index+9]<<8);
  g_ram[0xd18+0x28]=0;
  contact_native_call(0xd18,0x04cb74);
  check((g_ram[0xd18+0x27]&127)<8,"partner projectile damages shared native enemy");
  check(RtlSaveSnapshotToMemory(modified,capacity)!=0,"updated native pools remain snapshot-capable");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"contact probes restore their starting machine");
}
