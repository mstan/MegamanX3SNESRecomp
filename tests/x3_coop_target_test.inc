/* Run the authored class08/variant1 enemy from the stage-1 fixture. Position
 * the actors and expire its real idle timer; do not set facing/proximity or
 * attack results. Compare one complete native update with a single-seat run. */
static void target_world_call(void) {
  uint16_t stack=g_cpu.S;
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x00dcc0),"native shared enemy loop returns");
  check(g_cpu.S==stack,"target borrowing preserves the world caller's stack");
}
static void native_target_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);
  X3CoopState setup;
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"authored target fixture decodes");
  check(setup.players[0].life==X3_COOP_ALIVE && setup.players[1].life==X3_COOP_ALIVE,
        "authored target fixture retains both living players");
  unsigned d=0xd18,x=g_ram[d+5]|g_ram[d+6]<<8;
  check(g_ram[d] && g_ram[d+10]==8 && g_ram[d+11]==1,"fixture contains its real stage enemy");
  setup.players[1].body[5]=(uint8_t)(x+32);setup.players[1].body[6]=(uint8_t)((x+32)>>8);
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"nearby P2 position encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"nearby P2 target fixture restores");
  g_ram[X3_COOP_BODY+5]=(uint8_t)(x-160);g_ram[X3_COOP_BODY+6]=(uint8_t)((x-160)>>8);
  g_ram[d+2]=0;g_ram[d+3]=0;g_ram[d+0x35]=1;
  size_t prepared=RtlSaveSnapshotToMemory(snapshot,capacity);
  check(prepared!=0,"target comparison precondition saves");
  uint8_t expected[0x3c0];
  X3CoopRuntimeEnable(false);
  memcpy(g_ram+X3_COOP_BODY,setup.players[1].body,X3_COOP_BODY_BYTES);
  target_world_call();
  check((g_ram[d+0x11]&64) && g_ram[d+0x36]==1 && !g_ram[d+0x35],
        "single-seat native reference faces right and detects its nearby player");
  memcpy(expected,g_ram+0xd18,sizeof(expected));
  X3CoopRuntimeEnable(true);
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"co-op matched target precondition restores");
  X3CoopRuntimeFrame();target_world_call();
  check((g_ram[d+0x11]&64) && g_ram[d+0x36]==1,
        "native enemy faces and detects nearer P2 instead of distant X");
  check(!memcmp(expected,g_ram+0xd18,sizeof(expected)),
        "co-op enemy records match exactly one native AI update");
  check(X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,
        "world targeting restores canonical ownership after the native update");
}
