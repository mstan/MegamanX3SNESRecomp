/* Shared tracking changes only the original camera target. Native dead zones,
 * follow speed, authored clamps and the original world update stay in charge. */
static bool shared_camera_ready(void){
  return !X3CoopViewsIndependent() && partner_ready() && !view_scene() && !menu_pending() &&
      !refill.active && !g_ram[0x1f37] && !g_ram[0x1f4f] && !(g_ram[0x1f5d]&128) &&
      g_ram[0xd1]==4 && g_ram[0xd2]==4;
}
enum { SHARED_GAP_X=192,SHARED_GAP_Y=160 };
static unsigned shared_pit_bottom(void){
  unsigned current=word(g_ram+0x1e6c),target=word(g_ram+0x1e7e);
  return (current>target?current:target)+224;
}
static bool shared_beyond_pit(const uint8_t *body){
  /* Native E2F8 uses a signed 16-bit difference, including negative Y at
   * the top of the map. An unsigned coordinate is not an enormous fall. */
  return (int16_t)(word(body+8)-32-shared_pit_bottom())>=0;
}
static unsigned shared_leader(unsigned axis){
  if(learner_enabled)return 0;
  const uint8_t *a=state.current==0?g_ram+X3_COOP_BODY:state.players[0].body;
  const uint8_t *b=state.current==1?g_ram+X3_COOP_BODY:state.players[1].body;
  int p0=(int16_t)word(a+axis),p1=(int16_t)word(b+axis),sign=p0<=p1?1:-1;
  unsigned previous=axis==5?0x22:0x24;
  int out0=sign*((int16_t)word(a+previous)-p0),out1=sign*(p1-(int16_t)word(b+previous));
  if(out0!=out1)return out1>out0?1:0;
  /* At rest keep the actor already closest to the current native view. */
  unsigned zone=axis==5?0x1e84:0x1e80;
  int center=(int16_t)word(g_ram+(axis==5?0x1e5d:0x1e60))+(int)(word(g_ram+zone)+word(g_ram+zone+2))/2;
  int d0=p0-center,d1=p1-center;if(d0<0)d0=-d0;if(d1<0)d1=-d1;
  return d0==d1?state.anchor:d1<d0?1:0;
}
static void shared_camera_boundary(CpuState *cpu,uint32_t pc){
  if(cpu->D!=0x1e58 || (cpu->P&0x20) || !X3CoopRuntimeReady() || X3CoopViewsIndependent())return;
  unsigned at=pc&0xffff;
  if(at==0xe2e2 || at==0xe2f1){
    /* A moving viewport is not a pit. Use the authored lower limit, including
     * the native pan's current bound, for either projected actor. */
    if(!view_bounds.native.pass)return;
    unsigned current=word(g_ram+0x1e6c),bottom=shared_pit_bottom();
    if(at==0xe2f1){if(word(g_ram)<bottom)putword(g_ram,bottom);return;}
    /* Retail reaches the pit path only when the camera itself tries to go
     * past the bottom. A partner can reach a real pit before the shared
     * midpoint gets there; enter the original health/damage path once. */
    if((int16_t)(current-word(g_ram+0x1e60))>=0 && (g_ram[X3_COOP_BODY+0x27]&127) &&
        shared_beyond_pit(g_ram+X3_COOP_BODY))
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0xe2fd);
    return;
  }
  if(state.current!=state.anchor || !shared_camera_ready())return;
  unsigned axis=(at==0xe0aa || at==0xe0b5)?8:5;
  const uint8_t *partner=state.players[state.anchor^1].body;
  if(!(g_ram[X3_COOP_BODY+0x27]&127) || !(partner[0x27]&127) ||
     g_ram[X3_COOP_BODY+2]==12 || partner[2]==12)return;
  int anchor=(int16_t)word(g_ram+X3_COOP_BODY+axis),other=(int16_t)word(partner+axis);
  /* Round toward the world actor so a one-pixel subpixel phase difference
   * cannot alternate which character dictates the shared position. */
  int focus=anchor+(other-anchor)/2,gap=anchor-other;if(gap<0)gap=-gap;
  if(learner_enabled)focus=state.anchor==0?anchor:other;
  else if(gap>(axis==5?SHARED_GAP_X:SHARED_GAP_Y)){
    /* If no safe floor has been found yet, keep one real actor in view
     * instead of centering the empty space between the two bodies. */
    int leader=shared_leader(axis)==state.anchor?anchor:other;
    int leash=axis==5?80:48;
    if(focus<leader-leash)focus=leader-leash;
    if(focus>leader+leash)focus=leader+leash;
  }
  cpu->A=(uint16_t)focus;
  cpu->_flag_N=(cpu->A&0x8000)!=0;cpu->_flag_Z=cpu->A==0;
  cpu->P=(uint8_t)((cpu->P&~0x82)|(cpu->_flag_N?128:0)|(cpu->_flag_Z?2:0));
}
