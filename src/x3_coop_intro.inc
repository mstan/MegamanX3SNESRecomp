/* The original intro reuses its single player body for story Zero, then X.
 * Transfer that native construction to the matching existing seat. The
 * captured participant remains alive, parked and hidden until actual rescue. */
static bool intro_capture_actor(unsigned d){
  return !g_ram[0x1fae] && enemy_actor(d) && g_ram[d+10]==0x62 &&
      !g_ram[d+11] && g_ram[d+1]<4 &&
      (!scene.phase || scene.kind==SCENE_INTRO_CAPTURE);
}
static bool intro_constructing(void){
  return scene.phase && (scene.kind==SCENE_INTRO_ZERO_CONSTRUCT || scene.kind==SCENE_INTRO_X_CONSTRUCT);
}
static void intro_input_boundary(CpuState *cpu,uint32_t pc){
  (void)cpu;(void)pc;
  if(!enabled || !state.initialized || !scene.phase || scene.kind<SCENE_INTRO_CAPTURE)return;
  uint16_t held[2]={(uint16_t)native_pad(0),(uint16_t)native_pad(1)};
  unsigned owner=state.players[scene.owner].life==X3_COOP_ALIVE?scene.owner:state.anchor;
  /* Sample at the original native pad mapper, even while the world task is
   * frozen. Keep both real host pads intact and maintain each seat's history
   * across character/script ownership changes. */
  if(!g_ram[0x1f63]){
    putword(g_ram+0xa8,held[owner]);putword(g_ram+0xaa,scene.raw[owner]);
    putword(g_ram+0xac,held[owner]&~scene.raw[owner]);
  }
  memcpy(scene.raw,held,sizeof(held));
}
static void intro_capture_begin(unsigned d){
  if(scene.phase || !intro_capture_actor(d) || g_ram[X3_COOP_BODY+2]!=0x66 ||
      state.players[state.current].character!=X3_COOP_X ||
      state.players[state.current].life!=X3_COOP_ALIVE)return;
  X3CoopViewsAdoptPlayer(g_ram,&state,state.current,false);
  state.anchor=state.current;target.seat=state.current;
  scene_begin(d,SCENE_INTRO_CAPTURE);
}
static void intro_change_owner(unsigned seat,unsigned kind){
  /* Do not project the old body over the original initializer's inputs.
   * The outgoing participant was captured before construction started. */
  state.current=state.anchor=(uint8_t)seat;
  scene.owner=(uint8_t)seat;scene.kind=(uint8_t)kind;scene.phase=2;scene.tick=0;
  if(target.active)target.seat=(uint8_t)seat;
}
static void intro_helper_handoff(void){
  if(!scene.phase || scene.kind!=SCENE_INTRO_ZERO ||
     state.players[scene.owner].life==X3_COOP_ALIVE ||
     state.players[state.anchor].life!=X3_COOP_ALIVE ||
     state.players[state.anchor].character!=X3_COOP_X)return;
  /* Finish the balanced native call before releasing the living captive.
   * Do this at death settlement: the next native camera/pit check must not
   * clamp his old capture position into the rescue room's lower void. */
  unsigned survivor=state.anchor;
  scene_cancel_transients(survivor);scene_place(survivor,state.players[scene.owner].body);
  scene.owner=(uint8_t)survivor;scene.phase=2;scene.tick=0;
  scene_install(survivor);
}
static void intro_boundary(CpuState *cpu,uint32_t pc){
  (void)cpu;
  if(!enabled || !state.initialized || !scene.phase || g_ram[0x1fae])return;
  unsigned x=state.players[0].character==X3_COOP_X?0:1,zero=x^1;
  switch(pc&0x7fffff){
    case 0x03ac57:
      if(scene.kind!=SCENE_INTRO_CAPTURE)return;
      /* The first constructor calls already replace the native X palette
       * and graphics. Retain X before those calls, not after their return. */
      X3CoopCapture(&state,g_ram);
      /* A host frame may end between individual initializer instructions.
       * Retain both original contexts until the new selector/health exist. */
      scene.kind=SCENE_INTRO_ZERO_CONSTRUCT;scene.phase=2;scene.tick=0;
      break;
    case 0x03ac8c:
      if(scene.kind!=SCENE_INTRO_ZERO_CONSTRUCT)break;
      if(state.players[zero].life==X3_COOP_ALIVE){
        /* Native story construction must not refill an existing partner. */
        g_ram[X3_COOP_BODY+0x27]=state.players[zero].body[0x27];
        g_ram[0x1fd2]=state.players[zero].maximum_health;
        intro_change_owner(zero,SCENE_INTRO_ZERO);
      }else{
        /* The native story helper cannot enroll an unavailable participant.
         * Release living X into the original helper's entry instead, keeping
         * Zero absent/fallen and X's own combat, health and identity. */
        intro_change_owner(x,SCENE_INTRO_ZERO);scene_cancel_transients(x);
        scene_place(x,g_ram+X3_COOP_BODY);scene_install(x);
      }
      break;
    case 0x03ad70:
      if(scene.kind==SCENE_INTRO_ZERO && state.players[x].life!=X3_COOP_ALIVE){
        /* The original exchange needs its temporary X body to finish. Keep
         * the real Zero context while native WRAM belongs to that cinematic;
         * the fallen/absent X is never enrolled or captured from it. */
        X3CoopCapture(&state,g_ram);scene_cancel_transients(zero);
        intro_change_owner(zero,SCENE_INTRO_RETIRED_X);
      }
      break;
    case 0x03ad7f:
      if(scene.kind!=SCENE_INTRO_ZERO)return;
      X3CoopCapture(&state,g_ram);
      scene.kind=SCENE_INTRO_X_CONSTRUCT;scene.phase=2;scene.tick=0;
      break;
    case 0x03ad87:
      if(scene.kind==SCENE_INTRO_X_CONSTRUCT){
        g_ram[X3_COOP_BODY+0x27]=state.players[x].body[0x27];
        g_ram[0x1fd2]=state.players[x].maximum_health;
        intro_change_owner(x,SCENE_INTRO_RETURN);
      }
      break;
    case 0x03aed9:
      if(intro_cinematic()){
        /* Native palette/lock cleanup has returned; the NPC retires next.
         * Adopt its finished floor using Zero's retained body/resources. */
        scene_place(zero,g_ram+X3_COOP_BODY);
        intro_change_owner(zero,SCENE_INTRO_RETURN);scene_install(zero);
      }
      break;
  }
}
