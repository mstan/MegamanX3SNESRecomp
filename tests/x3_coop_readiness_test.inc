/* Native offscreen allocation precedes enemy construction. Freeze only the
 * shared NPC scheduler while preparing that actual pending constructor. */
static void native_readiness_checks(const char *normal,const char *reversed,uint8_t *work,uint8_t *after,size_t capacity){
  uint8_t *seed=malloc(capacity),*future=malloc(capacity);check(seed && future,"native readiness replay buffers allocate");
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==1 && !g_ram[0xfa16],"readiness starts with a genuinely unvisited stage1 enemy in either roster");
    size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);setup.current=setup.anchor=0;
    campaign_position_only(&setup.players[1],736,setup.players[1].character==X3_COOP_ZERO?1127:1135);
    for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x88]=0;
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"readiness supplies actor positions without changing enemy initialization or collision flags");door_install_player(&setup);
    X3CoopViewsConfigure(true);X3CoopViewsCommitNative(g_ram,X3CoopRuntimeState());X3CoopViewState views=*X3CoopViewsState();views.camera[1][0]=608;views.camera[1][1]=1024;views.camera_valid=0;X3CoopViewsSetState(&views);
    g_ram[0x1f27]=1;for(unsigned f=0;f<3;++f)frame(0);
    unsigned d=0;for(unsigned a=0xd18;a<0x10d8;a+=64)if(g_ram[a] && g_ram[a+10]==8 && g_ram[a+11]==1 && view_read(a+5)==736)d=a;
    fprintf(stderr,"native READINESS pending roster%u slot%04x primary%u HP%u ready%02x cameras%u/%u\n",roster,d,g_ram[d+1],g_ram[d+0x27],g_ram[d+14],X3CoopViewsState()->camera[0][0],X3CoopViewsState()->camera[1][0]);
    check(d && !g_ram[d+1] && !g_ram[d+0x27] && !g_ram[d+14] && g_ram[0xfa16],"the real offscreen allocator admits an unconstructed and collision-ineligible enemy");
    /* Earn shots only after the actual private views and pending constructor
     * exist. Earlier pre-teleport attacks left their real camera region. */
    for(unsigned f=0;f<48;++f){
      frame(f%4==0?SNES_PAD_Y*0x1001u:0);unsigned active=0;
      for(unsigned seat=0;seat<2;++seat){const X3CoopState *s=X3CoopRuntimeState();const uint8_t *shots=s->current==seat?g_ram+X3_COOP_SHOTS:s->players[seat].shots;
        for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)if(shots[i])active|=1u<<seat;}
      if(active==3)break;
    }
    n=RtlSaveSnapshotToMemory(seed,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)if(setup.players[seat].shots[i]){
      uint8_t *shot=setup.players[seat].shots+i;shot[5]=g_ram[d+5];shot[6]=g_ram[d+6];shot[8]=g_ram[d+8];shot[9]=g_ram[d+9];
    }
    check(X3CoopEncode(&setup,seed+contact_context_offset(seed,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(seed,n),
        "input-created projectiles receive the actual pending enemy collision geometry without a hit or readiness result");door_install_player(&setup);
    for(unsigned seat=0;seat<2;++seat){unsigned shots=0;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)shots+=setup.players[seat].shots[i]!=0;fprintf(stderr,"native READINESS actual shots roster%u seat%u count%u\n",roster,seat,shots);check(shots,"both native players have actual input-created live projectiles before the readiness contact checks");}
    for(unsigned pc=0;pc<2;++pc){unsigned root=pc?0x04cb74:0x04cb31;
      for(unsigned seat=0;seat<2;++seat){
        check(RtlLoadSnapshotFromMemory(seed,n),"original readiness reference restores the actual pending constructor");X3CoopState solo=setup;X3CoopRuntimeEnable(false);check(X3CoopProject(&solo,g_ram,seat),"original readiness reference projects its real native actor");
        CombatSample before,reference;combat_read(&before,seat,true);uint8_t object[64];memcpy(object,g_ram+d,64);contact_native_call(d,root);combat_read(&reference,seat,true);
        check(!(g_cpu.A&255) && !memcmp(&before,&reference,sizeof(before)) && !memcmp(object,g_ram+d,64),"original native contact refuses the unready enemy without consuming a personal attack");
      }
      check(RtlLoadSnapshotFromMemory(seed,n),"paired readiness restores the same unready native object");CombatSample before[2],now;for(unsigned seat=0;seat<2;++seat)combat_read(before+seat,seat,false);uint8_t object[64];memcpy(object,g_ram+d,64);contact_native_call(d,root);
      check(!(g_cpu.A&255) && !memcmp(object,g_ram+d,64),"paired contact cannot turn the unready offscreen enemy into another collision opportunity");
      for(unsigned seat=0;seat<2;++seat){combat_read(&now,seat,false);check(!memcmp(before+seat,&now,sizeof(now)),"both actors retain their complete original attack and body state after unready contact");}
    }
    /* The negative checks retain the live attacks exactly. Move only their
     * collision approach away before the separate body-readiness positive
     * check, so an immediate buster kill cannot hide constructor readiness. */
    check(RtlLoadSnapshotFromMemory(seed,n),"positive readiness restores the unconsumed native attacks");
    setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)if(setup.players[seat].shots[i]){
      uint8_t *shot=setup.players[seat].shots+i;shot[5]=0x80;shot[6]=1;
    }
    check(X3CoopEncode(&setup,seed+contact_context_offset(seed,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(seed,n),
        "positive body readiness supplies projectile separation without changing the pending enemy or either actor");door_install_player(&setup);
    size_t finish=0;for(unsigned replay=0;replay<2;++replay){
      check(RtlLoadSnapshotFromMemory(seed,n),"native readiness activation restores its pending constructor and live shots");g_ram[0x1f27]=0;unsigned hp=contact_hp(1),ready=0;
      for(unsigned f=0;f<8;++f){frame(0);if(g_ram[d] && g_ram[d+1]==2 && (g_ram[d+0x27]&127) && g_ram[d+14])ready=f+1;}
      fprintf(stderr,"native READINESS active roster%u replay%u ready%u phase%u HP%u flags%02x targetHP%u/%u\n",roster,replay,ready,g_ram[d+1],g_ram[d+0x27],g_ram[d+14],hp,contact_hp(1));
      check(ready && contact_hp(1)<hp && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"the real constructor enables native collision and actual P2 damage only after scheduler release");
      size_t actual=RtlSaveSnapshotToMemory(after,capacity);if(!replay){memcpy(future,after,actual);finish=actual;}else same(future,finish,after,actual,"native offscreen construction, contact readiness and personal attacks repeat the complete machine");
    }
  }
  free(seed);free(future);
}
