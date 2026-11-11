/* Original08:85DC is copied to7E:25DC. Its native poll retains both ports:
 * A8/AA/AC and AE/B0/B2. Keep P1's physical current/previous words in the
 * existing serialized raw cache while a shared scene lends JOY1 to P2. */
static void primary_input_restore(void){
  putword(g_ram+0xa8,scene.raw[0]);putword(g_ram+0xaa,scene.raw[1]);
  putword(g_ram+0xac,scene.raw[0]&~scene.raw[1]);
}
static void modal_input_project(void){
  if(!enabled || !state.initialized || g_ram[0x1f63] || menu_pending() || X3CoopNativePauseActive(g_ram))return;
  if(!scene.phase && !g_ram[0x1f22] && !g_ram[0x1f4f] && g_ram[0xd1]==4 && g_ram[0xd2]==4)return;
  unsigned owner=scene.phase?scene.owner:state.anchor;
  if(state.players[owner].life!=X3_COOP_ALIVE)owner=state.anchor;
  if(state.players[owner].life!=X3_COOP_ALIVE && !state.wipe)return;
  if(owner)memcpy(g_ram+0xa8,g_ram+0xae,6);
  else primary_input_restore();
}
static void modal_input_boundary(CpuState *cpu,uint32_t pc){
  (void)cpu;(void)pc;modal_input_project();
}
static void native_input_boundary(CpuState *cpu,uint32_t pc){
  if(!enabled || cpu->D)return;
  if((pc&0x7fffff)==0x7e25ef){
    /* Restore physical P1 before the original poll advances its history.
     * Recorded input branches around this instruction. */
    putword(g_ram+0xa8,scene.raw[0]);return;
  }
  if((pc&0x7fffff)!=0x7e2621)return;
  if(g_ram[0x1f63]){
    /* JOY1 belongs to the original demonstration. Track its physical port
     * without changing the recorded buttons or their native action stream. */
    unsigned held=cpu_read16(cpu,0,0x4218);scene.raw[1]=scene.raw[0];
    scene.raw[0]=(uint16_t)((held&15)?0:held);
  }else{
    scene.raw[0]=(uint16_t)word(g_ram+0xa8);scene.raw[1]=(uint16_t)word(g_ram+0xaa);
  }
  modal_input_project();
}
