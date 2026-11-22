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
static bool intro_opening_zero(void){
  return !stock_zero && !state.wipe && !g_ram[0x1fae] &&
      g_ram[0xcc8] && g_ram[0xcca]==0x10 && g_ram[0xcd2]==1;
}
static void intro_opening_join(void){
  if(!intro_opening_zero() || g_ram[0xd04])return;
  unsigned zero=state.players[0].character==X3_COOP_ZERO?0:1;
  uint8_t *body=state.players[zero].body;
  putword(body+5,word(g_ram+0xccd));putword(body+8,word(g_ram+0xcd0));
  putword(body+0x22,word(body+5));putword(body+0x24,word(body+8));
  body[4]=body[7]=0;
  g_ram[0xcc8]=0;
}
static void intro_hold_zero_at_capture(void){
  /* The native story requires X at Mac's approach. Let Zero step away from
   * the boundary or continue alone if the team no longer has a living X. */
  const unsigned boundary_x=0x1100;
  if(g_ram[0x1fae] || scene.phase || state.players[state.current].character!=X3_COOP_ZERO ||
     state.players[state.current^1].life!=X3_COOP_ALIVE)return;
  uint8_t *body=g_ram+X3_COOP_BODY;
  if(word(body+0x22)<=boundary_x && word(body+5)>boundary_x){
    putword(body+5,boundary_x);body[4]=0;putword(body+0x1a,0);
  }
}
static void intro_capture_begin(unsigned d){
  const uint8_t *body=g_ram+X3_COOP_BODY;
  bool walk_queued=(body[0x6e]&128) && word(body+0xb0)==0x1140 && g_ram[0x1f4f];
  if(scene.phase || !intro_capture_actor(d) || (!walk_queued && body[2]!=0x66) ||
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
static bool intro_keep_partner(void){
  return !stock_zero && !scene.stage && scene.kind==SCENE_INTRO_RETURN &&
      state.players[scene.owner].character==X3_COOP_X &&
      state.players[scene.owner].life==X3_COOP_ALIVE &&
      state.players[scene.owner^1].life==X3_COOP_ALIVE;
}
static void intro_boundary(CpuState *cpu,uint32_t pc){
  if(!enabled || g_ram[0x1fae])return;
  unsigned at=pc&0x7fffff;
  if(!state.initialized && intro_opening_zero()){
    if(at==0x03a9b4 && cpu->D==0xcc8 && g_ram[0xd04]<=1){
      /* Keep the NPC's standing draw while the native ready sequence runs. */
      g_ram[0xd04]=0;
      interp_bridge_pre_opcode_redirect((pc&0x800000)|0x03a9b8);
    }else if(at==0x04974d && cpu->D==X3_COOP_BODY && !g_ram[0xd04]){
      /* The dialogue and its closing timer finished. Treat Zero as done
       * here, then exchange the parked NPC for the player at enrollment. */
      interp_bridge_pre_opcode_redirect((pc&0x800000)|0x049752);
    }
  }
  if(!state.initialized)return;
  unsigned x=state.players[0].character==X3_COOP_X?0:1,zero=x^1;
  /* A prior-fallen X lets native Zero reach rescue without the earlier
   * capture scene. The actual exchange still needs its temporary X body. */
  if((pc&0x7fffff)==0x03ad70 && !scene.phase && cpu->D==0x0cc8 &&
     state.players[x].life!=X3_COOP_ALIVE && state.players[zero].life==X3_COOP_ALIVE &&
     state.current==zero && state.anchor==zero && g_ram[X3_COOP_BODY+0xb6]==2){
    scene_begin(cpu->D,SCENE_NATIVE_CINEMATIC);
    /* Original Zero's exit consumes the capture/exchange token. A survivor
     * that skipped capture has not acquired it, so own it for this exchange
     * and let the original cleanup release it instead of underflowing. */
    if(!g_ram[0x1f45])++g_ram[0x1f45];
  }
  if(!scene.phase)return;
  switch(at){
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
        intro_change_owner(zero,SCENE_NATIVE_CINEMATIC);
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
    case 0x03ae95:
      if(cpu->D==0xcc8 && intro_keep_partner()){
        /* The dialogue has finished. Exchange the standing story NPC for
         * its retained player, then execute the original lock/health cleanup
         * directly. The solo takeoff sound, movement and FX are unnecessary. */
        const uint8_t *npc=g_ram+0xcc8;
        uint8_t *body=state.players[zero].body;
        scene_place(zero,npc);
        body[0x11]=(uint8_t)((body[0x11]&~64)|(npc[0x11]&64));
        body[0x17]=npc[0x17];
        X3CoopPresentationBodyDma(zero,body,g_snes->cart->rom,g_snes->cart->romImageSize);
        interp_bridge_pre_opcode_redirect((pc&0x800000)|0x03aec2);
      }
      break;
    case 0x03aed9:
      if(native_cinematic()){
        /* Native palette/lock cleanup has returned; the NPC retires next.
         * Adopt its finished floor using Zero's retained body/resources. */
        scene_place(zero,g_ram+X3_COOP_BODY);
        intro_change_owner(zero,SCENE_INTRO_RETURN);scene_install(zero);
      }
      break;
  }
}
