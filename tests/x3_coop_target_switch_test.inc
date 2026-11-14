/* Actual class08/01 jump attack and hostile child updates. Only player
 * positions/poses and contact protection are supplied; enemy state is earned. */
static unsigned switch_parent,switch_parent_calls,switch_child_calls[8],switch_births,switch_seen;
static bool switch_probing;
static void switch_target_probe(CpuState *cpu,uint32_t pc){
  if(!switch_probing)return;
  unsigned d=cpu->D;
  if(pc==0x00dd1c && d==switch_parent && g_ram[d]){
    ++switch_parent_calls;const X3CoopState *s=X3CoopRuntimeState();
    unsigned nearest=X3CoopNearest(s,g_ram,view_read(d+5),view_read(d+8));
    if(s->current!=nearest || g_ram[X3_COOP_BODY+0xb6]!=s->players[nearest].character*2)
      fprintf(stderr,"target probe current%u nearest%u anchor%u char%u expected%u enemy%u/%u nativeScene%u/%u\n",s->current,nearest,s->anchor,g_ram[X3_COOP_BODY+0xb6],s->players[nearest].character*2,view_read(d+5),view_read(d+8),g_ram[0x1f22],g_ram[0x1f24]);
    check(s->current==nearest && g_ram[X3_COOP_BODY+0xb6]==s->players[nearest].character*2,"the real in-progress enemy update receives its currently nearest living actor");
    switch_seen=nearest;
  }
  if(pc==0x00dcb5 && d>=0x1318 && d<0x1518 && !((d-0x1318)%64) && g_ram[d]){
    unsigned i=(d-0x1318)/64;++switch_child_calls[i];if(!g_ram[d+1])switch_births|=1u<<i;
  }
}
static void native_target_switch_checks(const char *fixture,uint8_t *work,uint8_t *expected,size_t capacity){
  /* Observe after SEP, independently of runtime-hook registration order. */
  check(interp_bridge_add_pre_opcode_hook(0x00dd1c,switch_target_probe) && interp_bridge_add_pre_opcode_hook(0x00dcb5,switch_target_probe),"target-switch observation registers after actual native target selection");
  uint8_t *seed=malloc(capacity),*future=malloc(capacity);check(seed && future,"target-switch full-machine replay buffers allocate");
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==1,"target switching loads its actual authored jump-attack world");
    unsigned d=0xd18;check(g_ram[d] && g_ram[d+10]==8 && g_ram[d+11]==1 && g_ram[d+1]==2 && g_ram[d+2]==8 && (g_ram[d+0x27]&127),"native enemy is already in its real jump attack before target changes");switch_parent=d;
    size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    if(roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
    setup.current=setup.anchor=0;
    for(unsigned seat=0;seat<2;++seat){setup.players[seat].body[0x88]=255;check(setup.players[seat].life==X3_COOP_ALIVE,"target switching retains two genuinely living participants");}
    team_hazard_body(setup.players[1].body,761,setup.players[1].character==X3_COOP_ZERO?1127:1135);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"target-switch preparation supplies only roster, actor positions and personal protection");door_install_player(&setup);
    X3CoopViewsConfigure(true);X3CoopViewsCommitNative(g_ram,X3CoopRuntimeState());
    X3CoopViewState views=*X3CoopViewsState();
    for(unsigned seat=0;seat<2;++seat){unsigned x=support_position(seat,5);views.camera[seat][0]=(uint16_t)(x>128?x-128:0);}
    X3CoopViewsSetState(&views);n=RtlSaveSnapshotToMemory(seed,capacity);size_t future_size=0;
    for(unsigned replay=0;replay<2;++replay){
      check(RtlLoadSnapshotFromMemory(seed,n),"target-switch sequence restores its unchanged live attack and native children");
      unsigned ai=0,births=0,stable=0,changes=0,previous=2,max_parent=0,max_child=0;bool attack_seen=false;
      for(unsigned f=0;f<240;++f){
        const X3CoopState *s=X3CoopRuntimeState();check(s->current==0,"each actual AI field restores canonical world ownership");
        unsigned ex=view_read(d+5),ey=view_read(d+8),p2x=support_position(1,5),p2y=support_position(1,8);unsigned distance=(unsigned)(abs((int)p2x-(int)ex)+abs((int)p2y-(int)ey));
        bool nearby=attack_seen && (f/8)%2;unsigned px=nearby?ex:(ex>distance+96?ex-distance-96:264);
        team_hazard_body(g_ram+X3_COOP_BODY,px,ey-(s->players[0].character==X3_COOP_ZERO?8:0));g_ram[X3_COOP_BODY+0x6e]=0;g_ram[X3_COOP_BODY+0x88]=255;
        uint8_t children[512];memcpy(children,g_ram+0x1318,sizeof(children));switch_parent_calls=switch_births=0;switch_seen=2;memset(switch_child_calls,0,sizeof(switch_child_calls));
        switch_probing=true;frame(0);switch_probing=false;
        check(switch_parent_calls<=1,"a nearest-target change never advances the same native enemy AI twice");ai+=switch_parent_calls;
        if(switch_seen<2){if(previous<2 && switch_seen!=previous)++changes;previous=switch_seen;}
        unsigned delta=(unsigned)(abs((int)view_read(d+5)-(int)ex)+abs((int)view_read(d+8)-(int)ey));if(delta>max_parent)max_parent=delta;
        if(!g_ram[d] || g_ram[d+10]!=8 || delta>16)fprintf(stderr,"parent step f%u before%u/%u after%u/%u delta%u active%u kind%02x/%02x phase%u/%u/%u AI%u nearest%u births%02x\n",f,ex,ey,view_read(d+5),view_read(d+8),delta,g_ram[d],g_ram[d+10],g_ram[d+11],g_ram[d+1],g_ram[d+2],g_ram[d+3],switch_parent_calls,switch_seen,switch_births);
        check(g_ram[d] && g_ram[d+10]==8 && delta<=16,"the in-progress native parent attack cannot teleport when another actor becomes nearer");
        for(unsigned i=0;i<8;++i){unsigned a=0x1318+i*64;const uint8_t *before=children+i*64;
          check(switch_child_calls[i]<=1,"native hostile child AI runs once per shared field");
          if(switch_births&(1u<<i)){++births;attack_seen=true;fprintf(stderr,"native target-switch child roster%u replay%u f%u slot%04x kind%02x/%02x velocity%04x/%04x\n",roster,replay,f,a,g_ram[a+10],g_ram[a+11],view_read(a+0x1a),view_read(a+0x1c));}
          if(before[0] && g_ram[a] && before[1]==2 && g_ram[a+1]==2 && before[10]==g_ram[a+10] && before[11]==g_ram[a+11] && !(switch_births&(1u<<i))){
            unsigned oldx=before[5]|before[6]<<8,oldy=before[8]|before[9]<<8;
            unsigned step=(unsigned)(abs((int)view_read(a+5)-(int)oldx)+abs((int)view_read(a+8)-(int)oldy));if(step>max_child)max_child=step;
            check(step<=16,"a live native attack child cannot jump to the newly nearer victim");++stable;
          }
        }
        for(unsigned seat=0;seat<2;++seat)check(X3CoopRuntimeState()->players[seat].life==X3_COOP_ALIVE && contact_hp(seat)>0,"both protected native targets remain eligible throughout the attack");
      }
      fprintf(stderr,"native TARGET SWITCH roster%u replay%u frames240 AI%u nearestChanges%u births%u childSteps%u maxParent%u maxChild%u HP%u/%u\n",roster,replay,ai,changes,births,stable,max_parent,max_child,contact_hp(0),contact_hp(1));
      check(ai>=200 && changes>=4 && births && stable>=8,"real target changes occur while native attacks and their children stay active");
      size_t actual=RtlSaveSnapshotToMemory(expected,capacity);if(!replay){memcpy(future,expected,actual);future_size=actual;}else same(future,future_size,expected,actual,"native in-progress attacks, nearest-target changes and hostile children repeat the complete machine");
    }
  }
  free(seed);free(future);
}
