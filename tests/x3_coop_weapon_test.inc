/* Matched native X weapon rules on the earned Buffalo fixture. Enemy AI is
 * frozen with its retail freeze flag; health/cooldown prerequisites isolate
 * damage amounts. Native cooldown admission and boss AI are separate gates. */
static unsigned weapon_buttons(unsigned mode,unsigned f){return mode==1?(f<180?SNES_PAD_Y:0):(f%8==0?SNES_PAD_Y:0);}
static void weapon_frame(unsigned input){
  g_ram[0x1f27]=1;g_ram[0xd3f]=127;g_ram[0xd48]=0;
  frame(input);CpuState caller=g_cpu;contact_native_call(0xd18,0x04cb74);
  uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
}
static void weapon_sample(CombatSample *out,unsigned seat,bool solo){combat_read(out,seat,solo);out->hit[0]=(uint8_t)(127-(g_ram[0xd3f]&127));out->hit[1]=g_ram[0xd48];out->hit[2]=g_ram[0xd19];}
static void weapon_protect(unsigned x,unsigned protection,uint8_t *scratch,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);X3CoopState s=*X3CoopRuntimeState();
  for(unsigned seat=0;seat<2;++seat)if(s.players[seat].life==X3_COOP_ALIVE)s.players[seat].body[0x88]=(uint8_t)(seat==x?protection:255);
  check(X3CoopEncode(&s,scratch+contact_context_offset(scratch,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(scratch,n),"weapon retirement protects only living participants as a test prerequisite");door_install_player(&s);
}
static void weapon_retire_frame(unsigned input){
  view_word(0xd1d,5800);view_word(0xd3a,5800);view_word(0xd20,911);view_word(0xd3c,911);g_ram[0xd3f]=127;frame(input);
}
static unsigned weapon_sources(unsigned x,const uint8_t *bytes,size_t n){
  const X3CoopPlayer *p=&X3CoopRuntimeState()->players[x];unsigned sources=0;
  for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)sources+=p->shots[i]!=0;
  for(unsigned i=96;i<X3_COOP_AUX_BYTES;i+=32)sources+=p->auxiliaries[i]!=0;
  size_t owners=combat_owners(bytes,n);for(unsigned i=0;i<40;++i)sources+=bytes[owners+i]==x+1 && g_ram[0x1818+i*32]!=0;
  return sources;
}
static void weapon_assert_retired(unsigned seat,uint8_t *scratch,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);const X3CoopPlayer *p=&X3CoopRuntimeState()->players[seat];
  bool empty=!p->shot_command;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;++i)empty&=!p->shots[i];for(unsigned i=96;i<X3_COOP_AUX_BYTES;++i)empty&=!p->auxiliaries[i];
  check(empty && !weapon_sources(seat,scratch,n),"retired native attack retains no private damage source or shared creator effect");
}
static void weapon_fatal_contact(unsigned seat,uint8_t *scratch,size_t capacity){
  weapon_protect(seat,0,scratch,capacity);weapon_retire_frame(0);weapon_retire_frame(0);
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);X3CoopState s=*X3CoopRuntimeState();s.players[seat].body[0x27]=2;
  check(X3CoopEncode(&s,scratch+contact_context_offset(scratch,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(scratch,n),"native fatal weapon contact supplies only low victim HP after explicit protection expires");door_install_player(&s);
  uint8_t original[64];memcpy(original,g_ram+0xd58,64);unsigned protection=g_ram[0x1f1d];contact_geometry(0xd58,support_position(seat,5),support_position(seat,8));g_ram[0x1f1d]=(uint8_t)protection;CpuState caller=g_cpu;
  contact_native_call(0xd58,0x04cb31);uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;memcpy(g_ram+0xd58,original,64);
  if(X3CoopRuntimeState()->players[seat].life!=X3_COOP_FALLEN && !contact_hp(seat))weapon_retire_frame(0);
  check(X3CoopRuntimeState()->players[seat].life==X3_COOP_FALLEN,"actual native contact kills the weapon creator rather than supplying a lifecycle result");
}
static void weapon_transition(const uint8_t *cut,size_t cn,unsigned x,unsigned index,unsigned mode,const char *kind,uint8_t *work,uint8_t *after,size_t capacity){
  check(RtlLoadSnapshotFromMemory(cut,cn),"weapon lifecycle restores the complete actual cast before public withdrawal");
  unsigned lives=g_ram[0x1fb4],checkpoint=g_ram[0x1fb5],survivor=x^1;
  if(strcmp(kind,"stage")){
    weapon_fatal_contact(x,work,capacity);check(X3CoopRuntimeState()->players[survivor].life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe,"actual weapon death leaves its partner living in the same world");
    weapon_assert_retired(x,after,capacity);unsigned clock=g_ram[0x9cc];for(unsigned f=0;f<24;++f)weapon_retire_frame(0);
    check((uint8_t)(g_ram[0x9cc]-clock)==24 && X3CoopRuntimeState()->players[x].life==X3_COOP_FALLEN,"dead weapon creator stays retired while the survivor advances24 real world frames");weapon_assert_retired(x,after,capacity);
    if(!strcmp(kind,"death")){fprintf(stderr,"native weapon death PASS index%u mode%u seat%u\n",index,mode,x);return;}
    check(!strcmp(kind,"retry"),"weapon lifecycle selector names a known native transition");weapon_fatal_contact(survivor,work,capacity);check(X3CoopRuntimeState()->wipe,"the second actual native contact starts the real team wipe");
  }else stage_request(); /* Bypass prior travel, not the stage constructor/cleanup being tested. */
  bool pending=false;unsigned elapsed=0;for(;elapsed<3600;++elapsed){frame(0);const X3CoopState *s=X3CoopRuntimeState();pending|=!X3CoopRuntimeReady() || s->wipe || X3CoopRuntimeSceneActive();
    if(pending && s->initialized && !s->wipe && X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4 && (!strcmp(kind,"stage")?g_ram[0x1fae]==5:g_ram[0x1fb4]+1==lives))break;
  }
  fprintf(stderr,"native weapon transition index%u mode%u seat%u kind%s elapsed%u lives%u/%u stage%u\n",index,mode,x,kind,elapsed,lives,g_ram[0x1fb4],g_ram[0x1fae]);
  check(elapsed<3600 && pending,"actual native stage/retry transition completes after discarding the old attacks");
  check(!strcmp(kind,"stage")?g_ram[0x1fb4]==lives:g_ram[0x1fae]==2 && g_ram[0x1fb5]==checkpoint && g_ram[0x1fb4]+1==lives,"native weapon stage/retry retains the intended campaign resources and checkpoint");
  for(unsigned seat=0;seat<2;++seat){const X3CoopPlayer *p=&X3CoopRuntimeState()->players[seat];weapon_assert_retired(seat,after,capacity);
    check(p->life==X3_COOP_ALIVE && (p->body[0x27]&127)==p->maximum_health && !p->body[0x35] && !p->body[0x7d] && !p->body[0x89] && !p->body[0xaa] && !p->body[0x8e] && !p->body[0x8f] && !p->body[0xb7],"fresh native bodies retain full resources without old weapon counts, readiness or attack protection");}
  for(unsigned f=0;f<16;++f)frame((f%8==0?SNES_PAD_Y:0)*0x1001u);
  for(unsigned seat=0;seat<2;++seat){const X3CoopPlayer *p=&X3CoopRuntimeState()->players[seat];bool fired=false;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)fired|=p->shots[i]!=0;check(fired,"each reconstructed actor actually fires a fresh native buster after weapon stage/retry cleanup");}
  fprintf(stderr,"native weapon %s PASS index%u mode%u seat%u\n",kind,index,mode,x);
}
static void native_weapon_withdrawal(const uint8_t *start,size_t sn,unsigned x,unsigned index,unsigned mode,uint8_t *work,uint8_t *after,size_t capacity){
  check(RtlLoadSnapshotFromMemory(start,sn),"live native weapon retirement restores its actual selected arena");g_ram[0x1f27]=0;
  unsigned address=0x1fbc+(index-1)*2,frames=0,sources=0;bool retired=false,observed=false;X3CoopPlayer before={0},other={0};unsigned reserve=0;size_t cut_size=0;
  for(;frames<720;++frames){
    if(frames%120==0)weapon_protect(x,255,work,capacity);
    unsigned buttons=mode==1?(frames<180?SNES_PAD_Y:0):(frames==0?SNES_PAD_Y:0);weapon_retire_frame(buttons<<(x*12));
    check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"isolated actual weapon cast retains two living participants");
    if((g_ram[address]&31)==28 || (mode==1 && frames<180))continue;
    size_t n=RtlSaveSnapshotToMemory(work,capacity);cut_size=n;sources=weapon_sources(x,work,n);observed|=sources!=0;
    before=X3CoopRuntimeState()->players[x];other=X3CoopRuntimeState()->players[x^1];reserve=g_ram[address];
    const char *output=getenv("X3_COOP_WEAPON_PREPARE_GUI");if(output && *output){check(RtlSaveSnapshot(output),"actual native weapon cast exports its current-build GUI fixture");return;}
    if(X3CoopRuntimeWithdraw(x)){retired=true;break;}
  }
  fprintf(stderr,"native weapon withdrawal index%u mode%u seat%u wait%u observed%u live%u counters35/7d/89/aa/b7=%u/%u/%u/%u/%u\n",index,mode,x,frames,observed,sources,before.body[0x35],before.body[0x7d],before.body[0x89],before.body[0xaa],before.body[0xb7]);
  check(retired,"actual native weapon cast reaches safe public withdrawal without an infinite attack/freeze lock");
  const char *kind=getenv("X3_COOP_WEAPON_RETIREMENT");if(strcmp(kind,"withdrawal")){weapon_transition(work,cut_size,x,index,mode,kind,after,work,capacity);return;}
  const X3CoopPlayer *p=&X3CoopRuntimeState()->players[x],*survivor=&X3CoopRuntimeState()->players[x^1];
  check(p->life==X3_COOP_ABSENT && survivor->life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe,"weapon retirement withdraws only its living creator");
  bool empty=true;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;++i)empty&=!p->shots[i];for(unsigned i=0;i<X3_COOP_AUX_BYTES;++i)empty&=!p->auxiliaries[i];
  check(empty && !p->shot_command && !p->body[0x35] && !p->body[0x7d] && !p->body[0x89] && !p->body[0xaa] && !p->body[0xb7],"discarded native weapon pools retain no projectile/orbit/charge/combo counts");
  check(!memcmp(survivor->shots,other.shots,sizeof(other.shots)) && !memcmp(survivor->auxiliaries,other.auxiliaries,sizeof(other.auxiliaries)) && survivor->body[0x27]==other.body[0x27],"weapon withdrawal preserves the other participant's complete live sources and health");
  size_t n=RtlSaveSnapshotToMemory(after,capacity),owners=combat_owners(after,n);for(unsigned i=0;i<40;++i)check(after[owners+i]!=x+1,"withdrawn weapon creator retains no shared child tag");
  check(X3CoopRuntimeRejoin(x),"weapon creator returns to actual clear arena ground");p=&X3CoopRuntimeState()->players[x];
  check(p->body[0x33]==before.body[0x33] && p->body[0x27]==before.body[0x27] && p->maximum_health==before.maximum_health && g_ram[address]==reserve,"actual weapon return retains native selection, health and unspent energy");
  weapon_protect(x,0,work,capacity);weapon_retire_frame(0);weapon_retire_frame(0);
  n=RtlSaveSnapshotToMemory(work,capacity);check(!weapon_sources(x,work,n) && g_ram[address]==reserve,"idle return cannot emit a queued attack or spend energy after its original sources were discarded");
  unsigned health=contact_hp(x);view_word(0xd1d,support_position(x,5));view_word(0xd20,support_position(x,8)-16);CpuState caller=g_cpu;contact_native_call(0xd18,0x04cb31);uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
  check(contact_hp(x)<health && X3CoopRuntimeState()->players[x].life==X3_COOP_ALIVE,"returned idle actor has no weapon-derived stale invulnerability after explicit test protection expires");
  check(RtlLoadSnapshotFromMemory(work,n),"fresh weapon cast restores the identical idle return before the separate damage probe");weapon_protect(x,255,after,capacity);for(unsigned f=0;f<64;++f)weapon_retire_frame(0);g_ram[address]=0x5c;
  if(X3CoopRuntimeState()->players[x].body[0x33]!=index*2){weapon_retire_frame(SNES_PAD_R<<(x*12));for(unsigned f=0;f<4;++f)weapon_retire_frame(0);}
  unsigned fresh=0;for(;fresh<420 && (g_ram[address]&31)==28;++fresh){if(fresh%120==0)weapon_protect(x,255,work,capacity);weapon_retire_frame((mode==1?(fresh<180?SNES_PAD_Y:0):(fresh%8==0?SNES_PAD_Y:0))<<(x*12));}
  if((g_ram[address]&31)==28){const X3CoopPlayer *blocked=&X3CoopRuntimeState()->players[x];fprintf(stderr,"blocked native weapon index%u mode%u seat%u state%02x/%02x/%02x HP%u selected%u energy%02x inputs%04x/%04x/%04x world%u\n",index,mode,x,blocked->body[1],blocked->body[2],blocked->body[3],blocked->body[0x27],blocked->body[0x33],g_ram[address],blocked->body[0x36]|blocked->body[0x37]<<8,blocked->body[0x38]|blocked->body[0x39]<<8,blocked->body[0x3a]|blocked->body[0x3b]<<8,g_ram[0x9cc]);for(unsigned i=0;i<X3_COOP_BODY_BYTES;++i)if(before.body[i]!=blocked->body[i])fprintf(stderr,"blocked body%02x %02x->%02x\n",i,before.body[i],blocked->body[i]);check(RtlSaveSnapshot("weapon-retire-blocked.sav"),"blocked weapon retains its actual diagnostic machine");}
  check((g_ram[address]&31)<28 && X3CoopRuntimeState()->players[x].life==X3_COOP_ALIVE,"returned actor actually casts the same native weapon again without stale locks");
  fprintf(stderr,"native weapon retirement PASS index%u mode%u seat%u live%u fresh%u\n",index,mode,x,sources,fresh);
}
static void native_boss_cooldown_checks(const uint8_t *base,size_t bn,uint8_t *work,uint8_t *after,size_t capacity){
  uint8_t *cut=malloc(capacity);check(cut!=NULL,"native concurrent boss comparison buffer allocates");
  for(unsigned mode=0;mode<2;++mode)for(unsigned leader=0;leader<2;++leader){
    check(RtlLoadSnapshotFromMemory(base,bn),"concurrent boss attacks restore the same real Buffalo encounter");
    for(unsigned f=0;f<3;++f)frame(SNES_PAD_LEFT*0x1001u);weapon_protect(0,255,work,capacity);
    unsigned born=mode?227:3;for(unsigned f=0;f<born;++f)frame((mode?combat_buttons(4,0,f):(f==0?SNES_PAD_Y:0))*0x1001u);
    size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState live=*X3CoopRuntimeState();live.current=live.anchor=(uint8_t)leader;
    unsigned native_damage_row=g_ram[0xd40],bx=support_position(leader,5)-16;view_word(0xd1d,bx);view_word(0xd3a,bx);
    uint8_t *distant=live.players[leader^1].body;distant[5]=distant[0x22]=0x38;distant[6]=distant[0x23]=0x18; /*6200: make the original first-contact actor the nearest native AI target. */
    for(unsigned seat=0;seat<2;++seat){unsigned shots=0;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)if(live.players[seat].shots[i]){uint8_t *shot=live.players[seat].shots+i;++shots;shot[5]=(uint8_t)bx;shot[6]=(uint8_t)(bx>>8);shot[8]=g_ram[0xd20];shot[9]=g_ram[0xd21];}
      check(shots>0,"both participants actually create native projectiles before concurrent boss contact");}
    /* Save the boss positioning with the actual owned pools in the same full machine. */
    view_word(0xd1d,bx);view_word(0xd3a,bx);g_ram[0xd3f]=127;g_ram[0xd4f]=0;g_ram[0x1f27]=0;
    n=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&live,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"actual native projectiles receive only collision positioning and contact priority prerequisites");door_install_player(&live);
    n=RtlSaveSnapshotToMemory(cut,capacity);X3CoopRuntimeEnable(false);for(unsigned f=0;f<2;++f)frame(0);
    uint8_t native_boss[64],native_shots[X3_COOP_SHOT_BYTES];memcpy(native_boss,g_ram+0xd18,64);memcpy(native_shots,g_ram+X3_COOP_SHOTS,sizeof(native_shots));
    unsigned damage=127-(g_ram[0xd3f]&127),cooldown=g_ram[0xd4f];fprintf(stderr,"original boss contact mode%u leader%u damage%u cooldown%u HP%u state%u/%u\n",mode,leader,damage,cooldown,g_ram[0xd3f],g_ram[0xd19],g_ram[0xd1a]);check(damage>0 && cooldown>0,"matched original live boss updates accept an attack and earn their real hit cooldown");
    check(RtlLoadSnapshotFromMemory(cut,n),"simultaneous paired collision restores the identical original prerequisites");for(unsigned f=0;f<2;++f)frame(0);
    check((g_ram[0xd3f]&127)==(native_boss[0x27]&127) && g_ram[0xd4f]==native_boss[0x37],"two actual attack pools produce the original successful boss damage and cooldown over the same native world frames");
    RtlSaveSnapshotToMemory(work,capacity);const X3CoopState *state=X3CoopRuntimeState();
    check(!memcmp(native_shots,state->players[leader].shots,sizeof(native_shots)),"the actual first native attack retains its original full projectile and hit-history pool beside a competing actor");
    unsigned health=g_ram[0xd3f]&127,clock=g_ram[0x9cc],frames=0;
    while(g_ram[0xd4f]>0 && frames<240){unsigned previous=g_ram[0xd4f];frame(0);++frames;check((g_ram[0xd3f]&127)==health && g_ram[0xd4f]+1==previous,"actual live boss AI advances its hit cooldown once per world frame without accepting a second actor hit");}
    check(frames<240 && !g_ram[0xd4f] && g_ram[0xd40]==native_damage_row && (uint8_t)(g_ram[0x9cc]-clock)==frames,"native boss cooldown completion retains one shared world tick per frame");
    size_t future=RtlSaveSnapshotToMemory(after,capacity);check(RtlLoadSnapshotFromMemory(cut,n),"concurrent boss contact restores for full native replay");for(unsigned f=0;f<frames+2;++f)frame(0);
    same(after,future,work,RtlSaveSnapshotToMemory(work,capacity),"concurrent native boss damage, cooldown and both owned pools replay the whole machine exactly");
    fprintf(stderr,"native boss cooldown PASS mode%u leader%u character%u damage%u cooldown%u realFrames%u\n",mode,leader,live.players[leader].character,damage,cooldown,frames+2);
  }
  free(cut);
}
static void native_buffalo_ice_contact(unsigned slot,unsigned roster,uint8_t *work,size_t capacity){
  uint8_t *born=malloc(capacity),*cut=malloc(capacity),*future=malloc(capacity);check(born && cut && future,"actual native ice contact comparison buffers allocate");
  check(slot==0x1318 && g_ram[slot] && g_ram[slot+10]==0x20 && !g_ram[slot+0x36],"actual emitted Buffalo ice uses its own hostile vector08:9CE6 and moving variant");
  g_ram[0x1f27]=1;for(unsigned f=0;f<3;++f)frame(0);check(g_ram[slot] && g_ram[slot+1]==2,"real native ice constructor reaches its ordinary active update before collision positioning");
  size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();setup.current=setup.anchor=0;
  unsigned x=view_read(slot+5),y=view_read(slot+8)+16;
  for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=b[0x30]=b[0x88]=0;b[0x27]=setup.players[seat].maximum_health;memset(b+0x1a,0,6);}
  check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"both vulnerable actors approach the same real ice using only geometry/protection prerequisites");door_install_player(&setup);n=RtlSaveSnapshotToMemory(born,capacity);
  const char *output=getenv("X3_COOP_HOSTILE_PREPARE_GUI");if(output && *output){check(RtlSaveSnapshot(output),"actual persistent ice collision exports its vulnerable current-build GUI setup");free(born);free(cut);free(future);return;}
  unsigned expected[2];uint8_t native_source[2][64];
  for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(born,n),"single-actor original ice contact restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);X3CoopRuntimeEnable(false);frame(0);expected[seat]=g_ram[X3_COOP_BODY+0x27]&127;memcpy(native_source[seat],g_ram+slot,64);
    fprintf(stderr,"original ice contact roster%u seat%u char%u HP%u/%u source%u/%u/%u xy%u,%u hero%u,%u\n",roster,seat,solo.players[seat].character,expected[seat],solo.players[seat].maximum_health,g_ram[slot],g_ram[slot+1],g_ram[slot+2],view_read(slot+5),view_read(slot+8),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8));
    check(expected[seat]<solo.players[seat].maximum_health && g_ram[slot] && g_ram[slot+1]==2,"actual original ice damages its vulnerable actor and remains a persistent live source");}
  for(unsigned leader=0;leader<2;++leader){check(RtlLoadSnapshotFromMemory(born,n),"paired persistent ice contact restores both vulnerable bodies");setup=*X3CoopRuntimeState();setup.current=setup.anchor=(uint8_t)leader;
    check(X3CoopEncode(&setup,born+contact_context_offset(born,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(born,n),"either first-contact seat owns the same native persistent hazard update");door_install_player(&setup);size_t cn=RtlSaveSnapshotToMemory(cut,capacity);frame(0);
    fprintf(stderr,"paired ice contact roster%u leader%u HP%u/%u expected%u/%u source%u/%u/%u\n",roster,leader,contact_hp(0),contact_hp(1),expected[0],expected[1],g_ram[slot],g_ram[slot+1],g_ram[slot+2]);
    check(contact_hp(0)==expected[0] && contact_hp(1)==expected[1],"a persistent native ice source evaluates both eligible actors on its one actual collision frame");
    check(!memcmp(native_source[leader],g_ram+slot,64),"two persistent ice contacts retain exactly one original native source update");
    unsigned clock=g_ram[0x9cc];for(unsigned f=0;f<24;++f)frame(0);check((uint8_t)(g_ram[0x9cc]-clock)==24 && contact_hp(0)==expected[0] && contact_hp(1)==expected[1],"each actual ice victim retains independent native hurt protection during24 subsequent world ticks");
    size_t fn=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(cut,cn),"actual persistent ice collision restores for replay");for(unsigned f=0;f<25;++f)frame(0);same(future,fn,work,RtlSaveSnapshotToMemory(work,capacity),"actual persistent ice contact and both native hurt histories replay the complete machine");
    fprintf(stderr,"native Buffalo persistent ice PASS roster%u leader%u HP%u/%u\n",roster,leader,expected[0],expected[1]);
  }
  free(born);free(cut);free(future);
}
static void native_consumed_hostile_fixture(unsigned roster,uint8_t *work,size_t capacity){
  const char *fixture=getenv("X3_COOP_CONSUMED_STATE");check(fixture && RtlLoadSnapshot(fixture) && g_ram[0x1fae]==1 && g_ram[0xd22]==8 && g_ram[0xd23]==1,"consumed-shot setup loads the actual stage1 authored shooter");
  size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();if(roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}setup.current=setup.anchor=0;
  unsigned enemy_x=view_read(0xd1d);for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&setup.players[seat];uint8_t *b=p->body;unsigned x=seat?enemy_x+32:enemy_x-160;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[2]=b[3]=b[4]=b[7]=b[0x30]=0;b[0x27]=p->maximum_health;b[0x88]=255;memset(b+0x1a,0,6);memset(p->shots,0,sizeof(p->shots));memset(p->auxiliaries,0,sizeof(p->auxiliaries));p->shot_command=0;}
  check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"shooter setup supplies only safe roster/position/protection and empty player attacks");door_install_player(&setup);g_ram[0xd1a]=0;g_ram[0xd1b]=2;g_ram[0xd4d]=1;g_ram[0x1f27]=0;memset(g_ram+0x1318,0,0x200);
  unsigned slot=0,elapsed=0;for(;elapsed<600 && !slot;++elapsed){if(elapsed%120==0)weapon_protect(0,255,work,capacity);frame(0);for(unsigned d=0x1318;d<0x1518;d+=64)if(g_ram[d]){slot=d;break;}}
  check(slot!=0,"original authored shooter emits an actual hostile through native AI and animation");fprintf(stderr,"actual shooter emitted roster%u f%u slot%04x class%02x variant%02x state%u/%u/%u XY%u,%u\n",roster,elapsed,slot,g_ram[slot+10],g_ram[slot+11],g_ram[slot],g_ram[slot+1],g_ram[slot+2],view_read(slot+5),view_read(slot+8));
  g_ram[0x1f27]=1;for(unsigned f=0;f<2;++f)frame(0);check(g_ram[slot] && g_ram[slot+1]==2,"actual hostile constructor reaches its native active update before body contact");
  uint8_t *born=malloc(capacity),*cut=malloc(capacity),*future=malloc(capacity);check(born && cut && future,"actual consumed hostile comparison buffers allocate");n=RtlSaveSnapshotToMemory(work,capacity);setup=*X3CoopRuntimeState();unsigned x=view_read(slot+5),y=view_read(slot+8)+1;
  for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=b[0x30]=b[0x88]=0;b[0x27]=setup.players[seat].maximum_health;memset(b+0x1a,0,6);}
  check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"two vulnerable actors approach the actual emitted hostile using geometry and expired protection only");door_install_player(&setup);n=RtlSaveSnapshotToMemory(born,capacity);
  unsigned expected[2];uint8_t source[2][64];for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(born,n),"original single-actor hostile contact restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);X3CoopRuntimeEnable(false);frame(0);expected[seat]=g_ram[X3_COOP_BODY+0x27]&127;memcpy(source[seat],g_ram+slot,64);
    fprintf(stderr,"original emitted hostile roster%u seat%u char%u HP%u/%u source%u/%u/%u\n",roster,seat,solo.players[seat].character,expected[seat],solo.players[seat].maximum_health,g_ram[slot],g_ram[slot+1],g_ram[slot+2]);if(expected[seat]==solo.players[seat].maximum_health){fprintf(stderr,"hostile miss geometry sourceXY%u,%u heroXY%u,%u sourceRect%04x heroRect%04x flags%02x/%02x protect%u/%u/%u global%u\n",view_read(slot+5),view_read(slot+8),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),view_read(slot+0x20),view_read(X3_COOP_BODY+0x20),g_ram[slot+0x0e],g_ram[X3_COOP_BODY+0x0e],g_ram[X3_COOP_BODY+0x30],g_ram[X3_COOP_BODY+0x88],g_ram[X3_COOP_BODY+0x8a],g_ram[0x1f1d]);check(RtlSaveSnapshot("hostile-contact-miss.sav"),"missed hostile keeps its actual diagnostic machine");}check(expected[seat]<solo.players[seat].maximum_health && (!g_ram[slot] || g_ram[slot+1]!=2),"actual original hostile damages one actor and enters its real consumed/retired state");}
  for(unsigned leader=0;leader<2;++leader){check(RtlLoadSnapshotFromMemory(born,n),"paired consumed-hostile contact restores");setup=*X3CoopRuntimeState();setup.current=setup.anchor=(uint8_t)leader;check(X3CoopEncode(&setup,born+contact_context_offset(born,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(born,n),"either first actor owns the same actual single-use hostile update");door_install_player(&setup);size_t cn=RtlSaveSnapshotToMemory(cut,capacity);frame(0);
    check(contact_hp(leader)==expected[leader] && contact_hp(leader^1)==setup.players[leader^1].maximum_health && !memcmp(source[leader],g_ram+slot,64),"a consumed actual hostile hits only its original first victim and executes exactly one native source response");
    for(unsigned f=0;f<24;++f)frame(0);check(contact_hp(leader)==expected[leader] && contact_hp(leader^1)==setup.players[leader^1].maximum_health,"consumed hostile cannot become a delayed second-actor hit during its remaining native lifetime");size_t fn=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(cut,cn),"consumed hostile restores before its actual collision");for(unsigned f=0;f<25;++f)frame(0);same(future,fn,work,RtlSaveSnapshotToMemory(work,capacity),"actual hostile consumption and protected partner replay the complete machine");fprintf(stderr,"native consumed hostile PASS roster%u leader%u HP%u/%u\n",roster,leader,contact_hp(0),contact_hp(1));}
  free(born);free(cut);free(future);
}
static void native_hostile_update(void){
  X3CoopRuntimeFrame();CpuState caller=g_cpu;g_cpu.PB=0;g_cpu.DB=6;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x00dc59) && g_cpu.S==caller.S,"original hostile loop updates once with a balanced native caller");uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
}
static void native_hostile_constructor_checks(unsigned roster,uint8_t *work,size_t capacity){
  uint8_t *cut=malloc(capacity);check(cut!=NULL,"native hostile constructor comparison buffer allocates");
  for(unsigned leader=0;leader<2;++leader){const char *prefix=getenv("X3_COOP_HOSTILE_OUTPUT");char filename[1024];check(prefix && snprintf(filename,sizeof(filename),"%s-roster%u.sav",prefix,roster)>0 && RtlLoadSnapshot(filename),"actual boss-emitted uninitialized projectile restores before its native constructor");
    unsigned slot=0x1318,nearby_seat=leader^1;check(g_ram[slot] && !g_ram[slot+1] && g_ram[slot+10]==0x20 && !g_ram[slot+0x27],"real emitted projectile has not yet initialized its native health or targeting");
    size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();setup.current=setup.anchor=(uint8_t)leader;unsigned source_x=view_read(slot+5);
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=seat==nearby_seat?source_x+32:source_x-160;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[4]=0;b[0x88]=255;memset(b+0x1a,0,6);}
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"a different living actor approaches the actual projectile before its constructor, without supplied aim or velocity");door_install_player(&setup);g_ram[0x1f27]=1;n=RtlSaveSnapshotToMemory(cut,capacity);
    X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)nearby_seat;door_install_player(&solo);X3CoopRuntimeEnable(false);native_hostile_update();uint8_t expected[64];memcpy(expected,g_ram+slot,64);unsigned native_vx=view_read(slot+0x1a);check(g_ram[slot+1]==2 && (g_ram[slot+0x27]&127),"original projectile constructor earns native readiness and aims from its real nearby actor");
    check(RtlLoadSnapshotFromMemory(cut,n),"paired projectile constructor restores the identical real uninitialized source");native_hostile_update();fprintf(stderr,"native hostile constructor roster%u anchor%u nearby_seat%u expectedVx%d actualVx%d HP%u initialized%u\n",roster,leader,nearby_seat,(int16_t)native_vx,(int16_t)view_read(slot+0x1a),g_ram[slot+0x27],g_ram[slot+1]);
    check(!memcmp(expected,g_ram+slot,64),"newly emitted hostile constructor uses the same living nearby actor and full native source state as original");
    size_t fn=RtlSaveSnapshotToMemory(work,capacity);check(RtlLoadSnapshotFromMemory(cut,n),"actual hostile constructor restores before its ownership selection");native_hostile_update();same(work,fn,cut,RtlSaveSnapshotToMemory(cut,capacity),"actual hostile constructor targeting and elapsed native time replay the entire machine exactly");
  }
  free(cut);
}
/* Prepare one real boss-emitted hostile source for the consumption/capture checks.
 * Native AI chooses its own attack; protection only keeps the live roster ready. */
static void native_buffalo_hostile_fixture(const uint8_t *base,size_t bn,unsigned roster,uint8_t *work,size_t capacity){
  if(getenv("X3_COOP_CONSUMED_STATE")){native_consumed_hostile_fixture(roster,work,capacity);return;}
  if(getenv("X3_COOP_HOSTILE_CONSTRUCTOR")){native_hostile_constructor_checks(roster,work,capacity);return;}
  if(getenv("X3_COOP_HOSTILE_CONTACT")){const char *prefix=getenv("X3_COOP_HOSTILE_OUTPUT");char filename[1024];check(prefix && snprintf(filename,sizeof(filename),"%s-roster%u.sav",prefix,roster)>0 && RtlLoadSnapshot(filename),"actual previously emitted hostile fixture loads through the test interface");native_buffalo_ice_contact(0x1318,roster,work,capacity);return;}
  check(RtlLoadSnapshotFromMemory(base,bn),"actual hostile-source setup restores the earned Buffalo arena");g_ram[0x1f27]=0;unsigned slot=0,elapsed=0;uint8_t active[14];for(unsigned i=0;i<14;++i)active[i]=g_ram[0xd58+i*64]!=0;
  for(;elapsed<1800 && !slot;++elapsed){
    if(elapsed%120==0)weapon_protect(0,255,work,capacity);frame(0);
    for(unsigned d=0x1318;d<0x1518;d+=64)if(g_ram[d]){slot=d;break;}
    if(!slot)for(unsigned i=0;i<14;++i){unsigned d=0xd58+i*64;if(g_ram[d] && !active[i]){slot=d;break;}active[i]=g_ram[d]!=0;}
    if(elapsed%120==0)fprintf(stderr,"actual Buffalo hostile wait roster%u f%u phase%u/%u/%u XY%u,%u HP%u players%u/%u\n",roster,elapsed,g_ram[0xd19],g_ram[0xd1a],g_ram[0xd1b],view_read(0xd1d),view_read(0xd20),g_ram[0xd3f]&127,contact_hp(0),contact_hp(1));
  }
  check(slot!=0,"actual native arena allocates a hostile projectile or new actor candidate without supplied spawn/result flags");
  fprintf(stderr,"actual Buffalo emitted roster%u f%u slot%04x class%02x variant%02x state%u/%u/%u XY%u,%u\n",roster,elapsed,slot,g_ram[slot+10],g_ram[slot+11],g_ram[slot],g_ram[slot+1],g_ram[slot+2],view_read(slot+5),view_read(slot+8));
  const char *prefix=getenv("X3_COOP_HOSTILE_OUTPUT");check(prefix && *prefix,"hostile-source preparation has an explicit private output prefix");char filename[1024];int used=snprintf(filename,sizeof(filename),"%s-roster%u.sav",prefix,roster);check(used>0 && (size_t)used<sizeof(filename) && RtlSaveSnapshot(filename),"actual emitted hostile source saves through the existing test interface");
}
static void native_acquired_weapon_checks(const char *fixture,uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  enum {FRAMES=420};CombatSample *trace=malloc(FRAMES*sizeof(*trace));uint8_t energy[FRAMES];uint8_t *start=malloc(capacity);check(trace && start,"native acquired-weapon comparison buffers allocate");
  unsigned first=getenv("X3_COOP_WEAPON_FIRST")?(unsigned)atoi(getenv("X3_COOP_WEAPON_FIRST")):1,last=getenv("X3_COOP_WEAPON_LAST")?(unsigned)atoi(getenv("X3_COOP_WEAPON_LAST")):9,modes=getenv("X3_COOP_WEAPON_MODES")?(unsigned)atoi(getenv("X3_COOP_WEAPON_MODES")):3;
  check(first>=1 && last<=9 && first<=last && modes>=1 && modes<=3,"native weapon selection bounds are valid");
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==2 && g_ram[0xd22]==0x52 && g_ram[0xd3f]==32,"acquired weapons load the actual earned Buffalo encounter");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();if(roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}setup.current=setup.anchor=0;
    for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&setup.players[seat];uint8_t *b=p->body;unsigned x=6010,y=p->character==X3_COOP_ZERO?919:927;
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=b[0x30]=b[0x33]=b[0x35]=b[0x57]=b[0x58]=b[0x5a]=b[0x5b]=b[0x7d]=b[0xaa]=b[0xb7]=0;b[0x27]=p->maximum_health;b[0x88]=255;memset(b+0x1a,0,6);memset(b+0x36,0,6);memset(p->auxiliaries,0,sizeof(p->auxiliaries));memset(p->shots,0,sizeof(p->shots));p->shot_command=0;p->previous_actions=0;}
    check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"actual weapon roster and safe arena positions restore");door_install_player(&setup);X3CoopViewsConfigure(false);
    unsigned x=setup.players[0].character==X3_COOP_X?0:1;g_ram[0x1fd1]|=2;g_ram[0x1f27]=1;
    for(unsigned i=0;i<9;++i)g_ram[0x1fbc+i*2]=0;for(unsigned i=0;i<12;++i)frame(i<3?SNES_PAD_LEFT<<(x*12):0);
    check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && g_ram[0xd19]==2,"both native actors and the real frozen encounter remain ready");
    n=RtlSaveSnapshotToMemory(base,capacity);frame(SNES_PAD_R<<(x*12));for(unsigned f=0;f<4;++f)frame(0);
    check(X3CoopRuntimeState()->players[x].body[0x33]==0,"native cycling refuses every unearned special weapon");
    if(getenv("X3_COOP_HOSTILE_SCOUT")){native_buffalo_hostile_fixture(base,n,roster,work,capacity);continue;}
    if(getenv("X3_COOP_BOSS_COOLDOWN")){native_boss_cooldown_checks(base,n,work,after,capacity);continue;}
    for(unsigned index=first;index<=last;++index)for(unsigned mode=getenv("X3_COOP_WEAPON_MODE_FIRST")?(unsigned)atoi(getenv("X3_COOP_WEAPON_MODE_FIRST")):0;mode<modes;++mode){
      check(RtlLoadSnapshotFromMemory(base,n),"native weapon case restores the same real arena");unsigned address=0x1fbc+(index-1)*2;g_ram[address]=(uint8_t)(mode==2?0x40:0x5c);
      frame(SNES_PAD_R<<(x*12));for(unsigned f=0;f<4;++f)frame(0);
      check(X3CoopRuntimeState()->players[x].body[0x33]==index*2 && !X3CoopRuntimeState()->players[x^1].body[0x33],"actual native input selects only the acquired X weapon and leaves Zero's selection untouched");
      size_t sn=RtlSaveSnapshotToMemory(start,capacity);unsigned frames=mode==1?420:240,ran=0,damage=0,spent=0,charge=0,moving=0;
      if(getenv("X3_COOP_WEAPON_RETIREMENT")){native_weapon_withdrawal(start,sn,x,index,mode,work,after,capacity);continue;}
      memcpy(work,start,sn);X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)x;check(X3CoopEncode(&solo,work+contact_context_offset(work,sn),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,sn),"single X weapon reference selects the same actual native body");door_install_player(&solo);X3CoopRuntimeEnable(false);
      for(unsigned f=0;f<frames;++f){weapon_frame(weapon_buttons(mode,f));weapon_sample(&trace[f],x,true);energy[f]=g_ram[address];if(trace[f].charge[1]>charge)charge=trace[f].charge[1];if(f)for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)if(trace[f].shots[j] && trace[f-1].shots[j] && memcmp(trace[f].shots+j+5,trace[f-1].shots+j+5,5))++moving;damage+=trace[f].hit[0];ran=f+1;if(!(g_ram[0xd3f]&127))break;}
      spent=(mode==2?0:28)-(g_ram[address]&31);fprintf(stderr,"native original weapon roster%u index%u mode%u frames%u spent%u damage%u peakCharge%u movingSamples%u\n",roster,index,mode,ran,spent,damage,charge,moving);
      if(index==1 && mode!=2)check(moving>0,"actual native Acid Burst projectiles move while only enemy AI is frozen");
      if(mode==1 && index<=8)check(charge>=4,"native charged special actually reaches its original charge threshold before release");
      check(mode==2?!(g_ram[address]&31):spent>0,"native weapon actually casts and spends energy, or retains the empty native meter");
      check(RtlLoadSnapshotFromMemory(start,sn),"paired acquired weapon restores the identical actual prerequisites");
      for(unsigned f=0;f<ran;++f){weapon_frame(weapon_buttons(mode,f)<<(x*12));CombatSample actual;weapon_sample(&actual,x,false);
        if(memcmp(&trace[f],&actual,sizeof(actual)) || energy[f]!=g_ram[address]){unsigned at=0;while(at<sizeof(actual) && ((uint8_t*)&trace[f])[at]==((uint8_t*)&actual)[at])++at;fprintf(stderr,"native weapon mismatch roster%u index%u mode%u frame%u byte%u expected%02x actual%02x energy%02x/%02x damage%u/%u\n",roster,index,mode,f,at,at<sizeof(actual)?((uint8_t*)&trace[f])[at]:0,at<sizeof(actual)?((uint8_t*)&actual)[at]:0,energy[f],g_ram[address],trace[f].hit[0],actual.hit[0]);check(false,"native acquired weapon commands, charge, energy, effects, damage and complete owned pools match original X on every frame");}}
      check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"acquired native weapon casting retains both living participants");
      size_t future=RtlSaveSnapshotToMemory(work,capacity);check(RtlLoadSnapshotFromMemory(start,sn),"acquired weapon restores for full-machine replay");for(unsigned f=0;f<ran;++f)weapon_frame(weapon_buttons(mode,f)<<(x*12));same(work,future,after,RtlSaveSnapshotToMemory(after,capacity),"actual native acquired-weapon use and the full machine replay exactly");
      fprintf(stderr,"native paired weapon PASS roster%u index%u mode%u frames%u spent%u damage%u\n",roster,index,mode,ran,spent,damage);
    }
  }
  free(trace);free(start);
}
