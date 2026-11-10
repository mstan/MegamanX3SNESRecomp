/* Optional player-two protection. The native damage paths decide the kind
 * of contact. Fatal rescue requires a living P1 and a safe native floor. */
static bool learner_landing(SafeLanding *landing){
  if(state.players[0].life!=X3_COOP_ALIVE || state.players[1].life!=X3_COOP_ALIVE ||
      scene.phase || refill.active || menu_pending() || stock_exit_pending || g_ram[0x1f4f])return false;
  const uint8_t *owner=state.current==0?g_ram+X3_COOP_BODY:state.players[0].body;
  const uint8_t *learner=state.current==1?g_ram+X3_COOP_BODY:state.players[1].body;
  if(!(owner[0x27]&127) || owner[2]==12 || !(learner[0x27]&127) || learner[2]>0x16 ||
      learner[0x64] || learner[0x6e])return false;
  return find_landing(1,owner,X3CoopViewsForPlayer(g_ram,&state,0,false),landing);
}
static void learner_damage_boundary(CpuState *cpu,uint32_t pc){
  if(!learner_enabled || !X3CoopRuntimeReady() || state.wipe || state.current!=1 ||
      state.players[1].life!=X3_COOP_ALIVE || scene.phase || menu_pending() ||
      g_ram[0x1f4f] || stock_exit_pending || !(g_ram[X3_COOP_BODY+0x27]&127))return;
  unsigned at=pc&0x7fffff,damage,exit;
  if(at==0x04cd6e){
    if(!enemy_actor(cpu->D))return;
    damage=g_ram[cpu->D+0x26];exit=0xcd65; /* Original zero-damage contact. */
  }else{
    damage=g_ram[0x9fe];exit=0xd0bf; /* Long return before the helper pushes P. */
  }
  if(!damage || damage>127)return;
  if(damage==127){
    SafeLanding landing;
    if(!learner_landing(&landing))return;
    learner_rescue=true;
  }
  interp_bridge_pre_opcode_redirect((pc&0xff0000)|exit);
}
static void learner_tick(void){
  if(!learner_rescue)return;
  if(!learner_enabled || state.wipe || state.players[0].life!=X3_COOP_ALIVE ||
      state.players[1].life!=X3_COOP_ALIVE){learner_rescue=false;return;}
  if(!participation_ready())return;
  SafeLanding landing;
  if(!learner_landing(&landing))return;
  X3CoopCapture(&state,g_ram);
  X3CoopViewsAdoptPlayer(g_ram,&state,0,false);
  if(state.current==1)X3CoopProject(&state,g_ram,0);
  land_player(1,&landing);restore_x_armor(1);
  X3CoopProject(&state,g_ram,state.anchor);learner_rescue=false;
  X3CoopViewsCommitNative(g_ram,&state);
}
