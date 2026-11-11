/* Isolated native combat: the actual controller/owned-pool loops run normally.
 * Clear shared hostile slots and consume local spawn visits as prerequisites;
 * no attack, charge, capacity, hit or progression result is supplied. */
typedef struct CombatSample {uint8_t command,charge[2],busy[2],controls[12],pose[4],palette[64],aux[X3_COOP_AUX_BYTES],shots[X3_COOP_SHOT_BYTES],hit[3];} CombatSample;

static unsigned combat_buttons(unsigned mode,unsigned seat,unsigned f){
  if(mode==4)return f<216 || f%4==0?SNES_PAD_Y:0;
  if(!mode)return f%4==0?SNES_PAD_Y:0;
  if(mode==1)return f<(seat?216:180)?SNES_PAD_Y:0;
  return seat==(mode==2?0:1)?(f<180?SNES_PAD_Y:0):(f%4==0?SNES_PAD_Y:0);
}
static void combat_read(CombatSample *out,unsigned seat,bool original){
  memset(out,0,sizeof(*out));
  const X3CoopState *s=X3CoopRuntimeState();const X3CoopPlayer *p=&s->players[seat];bool live=original || s->current==seat;
  const uint8_t *body=live?g_ram+X3_COOP_BODY:p->body;out->command=live?g_ram[0x1f1e]:p->shot_command;
  out->pose[0]=body[2];out->pose[1]=body[3];/*04:BCAF consumes the native DMA-request bit; the private atlas consumes the same pose without that shared-VRAM write. */out->pose[2]=body[0x17]&127;out->pose[3]=body[0x59];out->busy[0]=body[0x8f];out->busy[1]=body[0xb7];const unsigned control_offsets[12]={0xac,0x59,0x6e,0x3b,0x33,0x7d,0xaa,0x8e,0x36,0x37,0x38,0x39};for(unsigned i=0;i<12;++i)out->controls[i]=body[control_offsets[i]];memcpy(out->charge,body+0x57,2);memcpy(out->palette,live?g_ram+X3_COOP_PALETTE:p->palette,64);
  memcpy(out->aux,live?g_ram+X3_COOP_AUX:p->auxiliaries,X3_COOP_AUX_BYTES);memcpy(out->shots,live?g_ram+X3_COOP_SHOTS:p->shots,X3_COOP_SHOT_BYTES);
}
static void combat_probe(CombatSample *out,unsigned seat,bool original,uint8_t *scratch,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);check(n!=0,"native weapon hit-window snapshot saves");
  if(!original){X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);}
  X3CoopRuntimeEnable(false);
  unsigned x=view_read(X3_COOP_BODY+5),y=view_read(X3_COOP_BODY+8);
  contact_geometry(0xd18,x+32,y-8);g_ram[0xd18+0x28]=0;g_ram[0xd18+0x27]=127;
  uint16_t stack=g_cpu.S;g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=0xd18;g_cpu.P=(uint8_t)((g_cpu.P|0x30)&~0x40);cpu_p_to_mirrors(&g_cpu);cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x04cb74) && g_cpu.S==stack,"actual native weapon hit-window call balances");
  out->hit[0]=g_ram[0xd18+0x27];out->hit[1]=(uint8_t)g_cpu.A;out->hit[2]=g_ram[0xd18+0x30];
  check(RtlLoadSnapshotFromMemory(scratch,n),"actual weapon hit-window probe restores its unchanged combat timeline");
  if(original)X3CoopRuntimeEnable(false);
}
static void combat_retirement(const uint8_t *base,size_t n,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned phase=0;phase<4;++phase)for(unsigned victim=0;victim<2;++victim){
    check(RtlLoadSnapshotFromMemory(base,n),"combat retirement restores the neutral actual roster");X3CoopState setup=*X3CoopRuntimeState();
    if(phase==3 && setup.players[victim].character!=X3_COOP_X)continue;
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=384+128*seat;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);}
    if(phase<2)setup.players[victim].body[0x27]=2;memcpy(work,base,n);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"separated actors and low victim HP establish a fatal-contact prerequisite");door_install_player(&setup);
    unsigned frames=phase==1 || phase==3?227:phase==2?216:200;for(unsigned f=0;f<frames;++f)frame(phase==3?combat_buttons(4,victim,f)<<(victim*12):combat_buttons(4,0,f)|(combat_buttons(4,1,f)<<12));
    size_t before=RtlSaveSnapshotToMemory(work,capacity);X3CoopState live=*X3CoopRuntimeState();unsigned survivor=victim^1;
    check(live.players[victim].life==X3_COOP_ALIVE && live.players[survivor].life==X3_COOP_ALIVE,"native charge/beam death starts with two living actors");
    if(phase==0 || phase==2)check(live.players[victim].body[0x58]>=4 && live.players[survivor].body[0x58]>=4,"both actors actually charge before fatal damage");
    size_t owner_at=door_scene_chunk(work,before)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;unsigned owners[2]={0,0};
    for(unsigned i=0;i<40;++i)if(work[owner_at+i])++owners[work[owner_at+i]-1];
    if(phase==1)check(owners[0] && owners[1],"both real charged attacks retain shared native child effects before fatal contact");
    if(phase>=2){
      if(phase==3)check(live.players[victim].body[0x7d] || live.players[victim].body[0xaa],"actual charged X beam/children own nonzero native attack counters before withdrawal");
      if(phase==2 && live.players[victim].character==X3_COOP_ZERO)check(live.players[victim].body[0xb7]!=0,"Zero actually arms the native combo before living withdrawal");
      check(X3CoopRuntimeWithdraw(victim),"living actor withdraws with native charge armed");const X3CoopPlayer *p=&X3CoopRuntimeState()->players[victim];
      fprintf(stderr,"withdraw charge victim%u char%u charge%u/%u combo%u AA%u count%u\n",victim,p->character,p->body[0x57],p->body[0x58],p->body[0xb7],p->body[0xaa],p->body[0x7d]);
      check(!p->body[0x35] && !p->body[0x57] && !p->body[0x58] && !p->body[0xb7] && !p->body[0xaa] && !p->body[0x7d],"withdrawal cancels the actual charge/combo and all retired native shot counters");
      check(X3CoopRuntimeRejoin(victim),"charged withdrawal returns on real clear native ground");frame(0);
      p=&X3CoopRuntimeState()->players[victim];check(p->life==X3_COOP_ALIVE && p->body[2]!=0x5c && !p->body[0xb7],"return requires the real native charge/combo again before a saber swing");
      bool fired=false;unsigned firing=0;for(;firing<40 && !fired;++firing){frame((firing%4==0?SNES_PAD_Y:0)<<(victim*12));p=&X3CoopRuntimeState()->players[victim];for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)fired|=p->shots[i]!=0;}fprintf(stderr,"returned native fire phase%u victim%u frames%u recoil%u busy%u enabled%u stock%u world%u\n",phase,victim,firing,p->body[0x59],p->body[0x8f],X3CoopRuntimeEnabled(),X3CoopRuntimeStockZero(),g_ram[0x9cc]);if(!fired){fprintf(stderr,"blocked returned attack char%u state%02x/%02x input%04x/%04x/%04x selected%u hold%u charge%u/%u gate7c%u gate7d%u AA%u B7%u capture%u script%02x global%u\n",p->character,p->body[2],p->body[3],p->body[0x36]|p->body[0x37]<<8,p->body[0x38]|p->body[0x39]<<8,p->body[0x3a]|p->body[0x3b]<<8,p->body[0x33],p->body[0x59],p->body[0x57],p->body[0x58],p->body[0x7c],p->body[0x7d],p->body[0xaa],p->body[0xb7],p->body[0x6e],p->body[0xa3],g_ram[0x1f36]);check(RtlSaveSnapshot("return-fire-failed.sav"),"blocked native return preserves a disposable diagnostic snapshot");}check(fired,"returned actor fires a fresh real native buster without stale allocation locks");
      continue;
    }
    contact_geometry(0xd18,support_position(victim,5),support_position(victim,8));CpuState caller=g_cpu;contact_native_call(0xd18,0x04cb31);
    /* This test injects a call between host frames. Retain its elapsed guest
     * time while restoring the outer frame caller, especially its direct page. */
    uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
    const X3CoopState *s=X3CoopRuntimeState();const X3CoopPlayer *p=&s->players[victim],*other=&s->players[survivor],*old=&live.players[survivor];
    check(p->life==X3_COOP_FALLEN && other->life==X3_COOP_ALIVE && !s->wipe && s->anchor==survivor,"actual native enemy damage retires only the charging/firing victim");
    bool empty=true;for(unsigned i=0;i<sizeof(p->shots);++i)empty&=!p->shots[i];for(unsigned i=0;i<sizeof(p->auxiliaries);++i)empty&=!p->auxiliaries[i];
    check(empty && !p->shot_command,"fatal attack retirement removes all victim-owned native damage and charge sources");
    check(other->body[0x27]==old->body[0x27] && !memcmp(other->body+0x57,old->body+0x57,2) && !memcmp(other->shots,old->shots,sizeof(other->shots)) && !memcmp(other->auxiliaries,old->auxiliaries,sizeof(other->auxiliaries)) && !memcmp(other->palette,old->palette,sizeof(other->palette)),"survivor retains its exact health, native charge, attacks, effects and palette at fatal handoff");
    size_t end=RtlSaveSnapshotToMemory(after,capacity);unsigned retained=0;owner_at=door_scene_chunk(after,end)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;
    for(unsigned i=0;i<40;++i){check(after[owner_at+i]!=victim+1,"dead actor retains no shared native child-effect owner");retained+=after[owner_at+i]==survivor+1;}
    check(retained==owners[survivor],"fatal damage preserves every surviving shared child-effect allocation");
    unsigned tick=g_ram[0x9cc];for(unsigned f=0;f<24;++f)frame((SNES_PAD_RIGHT|SNES_PAD_Y)<<(survivor*12));
    fprintf(stderr,"retirement continuation phase%u victim%u tick%u->%u states%u/%u hp%u/%u display%02x tasks%u/%u freeze%02x/%02x/%02x/%02x/%02x/%02x/%02x position%u,%u\n",phase,victim,tick,g_ram[0x9cc],X3CoopRuntimeState()->players[victim].life,X3CoopRuntimeState()->players[survivor].life,contact_hp(victim),contact_hp(survivor),g_ppu->inidisp,g_ram[0xd1],g_ram[0xd2],g_ram[0x1f25],g_ram[0x1f26],g_ram[0x1f27],g_ram[0x1f28],g_ram[0x1f29],g_ram[0x1f2a],g_ram[0x1f2b],support_position(survivor,5),support_position(survivor,8));
    check((uint8_t)(g_ram[0x9cc]-tick)==24 && X3CoopRuntimeState()->players[survivor].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[victim].life==X3_COOP_FALLEN && !(g_ppu->inidisp&0x80),"survivor continues actual movement/combat and one bright shared world after charged death");
    fprintf(stderr,"native combat retirement phase%u victim%u char%u ownerChildren%u/%u survivorCharge%u world24\n",phase,victim,p->character,owners[0],owners[1],old->body[0x58]);
  }
}
static void combat_select_creator(unsigned seat,uint8_t *scratch,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);X3CoopState s=*X3CoopRuntimeState();s.current=(uint8_t)seat;
  check(X3CoopEncode(&s,scratch+contact_context_offset(scratch,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(scratch,n),"native effect allocator selects its real creator context");door_install_player(&s);
}
static unsigned combat_allocate_effect(unsigned pc,unsigned d){
  CpuState caller=g_cpu;g_cpu.PB=2;g_cpu.DB=0;g_cpu.D=(uint16_t)d;g_cpu.A=0;g_ram[0]=g_ram[1]=0;g_cpu.P=(uint8_t)((g_cpu.P|0x20)&~0x10);cpu_p_to_mirrors(&g_cpu);cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc) && g_cpu.S==caller.S,"real shared-effect constructor/allocator returns with its balanced guest caller");
  unsigned slot=g_cpu.X;uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;return slot;
}
static size_t combat_owners(const uint8_t *bytes,size_t n){return door_scene_chunk(bytes,n)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;}
static void combat_effect_pressure(uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned f=0;f<12;++f)frame((f%4==0?SNES_PAD_Y:0)*0x1001u);
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState live=*X3CoopRuntimeState();
  for(unsigned seat=0;seat<2;++seat){unsigned shots=0;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)shots+=live.players[seat].shots[i]!=0;check(shots>=2,"both native projectile pools contain real live busters during effect pressure");}
  unsigned created[2]={0,0},used=0;for(unsigned i=0;i<40;++i)used+=g_ram[0x1818+i*32]!=0;
  for(unsigned attempt=0;used<40 && attempt<40;++attempt){unsigned seat=attempt&1;combat_select_creator(seat,work,capacity);unsigned slot=combat_allocate_effect(0x02d9a8,X3_COOP_BODY);
    if(slot<0x1818 || slot>=0x1d18 || g_ram[slot+10]!=0x37)fprintf(stderr,"native effect construct seat%u slot%04x bytes%02x/%02x/%02x/%02x AA%u\n",seat,slot,g_ram[slot&0x1ffff],g_ram[(slot+1)&0x1ffff],g_ram[(slot+10)&0x1ffff],g_ram[(slot+11)&0x1ffff],g_ram[X3_COOP_BODY+0xaa]);
    check(slot>=0x1818 && slot<0x1d18 && !((slot-0x1818)%32) && g_ram[slot] && g_ram[slot+10]==0x37,"actual native weapon-child constructor fills a free shared slot");
    size_t size=RtlSaveSnapshotToMemory(after,capacity),owners=combat_owners(after,size);check(after[owners+(slot-0x1818)/32]==seat+1,"allocated native child retains the actual creator rather than the world anchor");++used;++created[seat];}
  check(used==40 && created[0] && created[1],"competing real creators fill all40 native shared-effect slots");
  n=RtlSaveSnapshotToMemory(base,capacity);size_t owners=combat_owners(base,n);uint8_t records[1280];memcpy(records,g_ram+0x1818,sizeof(records));
  check(RtlLoadSnapshotFromMemory(base,n),"full native effect pressure snapshot restores");same(base,n,after,RtlSaveSnapshotToMemory(after,capacity),"full native child ownership and the complete machine restore exactly");
  for(unsigned seat=0;seat<2;++seat){combat_select_creator(seat,work,capacity);unsigned aa=X3CoopRuntimeState()->players[seat].body[0xaa];unsigned slot=combat_allocate_effect(0x02d9a8,X3_COOP_BODY);
    check(slot==0x1d18 && !memcmp(records,g_ram+0x1818,sizeof(records)),"full retail allocator refuses another child without replacing any live effect");
    size_t size=RtlSaveSnapshotToMemory(after,capacity),at=combat_owners(after,size);check(!memcmp(base+owners,after+at,40) && X3CoopRuntimeState()->players[seat].body[0xaa]==aa,"failed native allocation preserves both owners and the creator's count");
    for(unsigned s=0;s<2;++s)check(!memcmp(live.players[s].shots,X3CoopRuntimeState()->players[s].shots,X3_COOP_SHOT_BYTES),"effect-pool exhaustion preserves each complete live projectile and hit-history pool");}
  combat_select_creator(0,work,capacity);check(X3CoopRuntimeWithdraw(0),"one living creator withdraws while both pools are pressured");
  size_t size=RtlSaveSnapshotToMemory(after,capacity),at=combat_owners(after,size);unsigned freed=0;
  for(unsigned i=0;i<40;++i){if(base[owners+i]==1){++freed;check(!g_ram[0x1818+i*32] && !after[at+i],"withdrawal releases only its creator's actual shared child slot");}
    else check(after[at+i]==base[owners+i] && !memcmp(g_ram+0x1818+i*32,records+i*32,32),"other-creator and unowned world effects retain exact native records under withdrawal");}
  check(freed && !memcmp(live.players[1].shots,X3CoopRuntimeState()->players[1].shots,X3_COOP_SHOT_BYTES),"surviving live projectiles and hit history remain exact after competing creator withdrawal");
  for(unsigned i=0;i<freed;++i){unsigned slot=combat_allocate_effect(0x02d9a8,X3_COOP_BODY);check(slot<0x1d18 && slot>=0x1818,"survivor reuses an actual freed native effect slot");}
  size=RtlSaveSnapshotToMemory(after,capacity);at=combat_owners(after,size);for(unsigned i=0;i<40;++i)if(base[owners+i]==1)check(after[at+i]==2,"reused child slots belong to their new real creator");
  /* A native allocator must also discard a former personal tag when an
   * inactive slot is reused by a world caller. Inactivity is a prerequisite,
   * not an assertion about a particular authored effect's cleanup. */
  unsigned slot=0x1818;while(slot<0x1d18 && after[at+(slot-0x1818)/32]!=2)slot+=32;check(slot<0x1d18,"world reuse has a former personal slot");memset(g_ram+slot,0,32);
  unsigned reused=combat_allocate_effect(0x02d859,0xd18);size=RtlSaveSnapshotToMemory(work,capacity);check(reused==slot && !work[combat_owners(work,size)+(slot-0x1818)/32],"unowned native allocation resets the old personal tag on actual slot reuse");
  fprintf(stderr,"native effect pressure used%u created%u/%u retired%u reusedWorld%04x\n",used,created[0],created[1],freed,reused);
}
static void native_concurrent_combat_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *future,size_t capacity){
  enum {FRAMES=420};CombatSample *trace=malloc(2*FRAMES*sizeof(*trace));uint8_t *probe=malloc(capacity);check(probe!=NULL,"native damage-window scratch allocates");check(trace!=NULL,"native combat comparison traces allocate");
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==1,"native concurrent combat loads the actual stage1 roster");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();setup.current=setup.anchor=0;
    for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&setup.players[seat];uint8_t *b=p->body;unsigned x=384,y=p->character==X3_COOP_ZERO?1175:1183;
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x33]=b[0x57]=b[0x58]=b[0x88]=b[0x8a]=0;memset(b+0x38,0,4);memset(p->auxiliaries,0,sizeof(p->auxiliaries));memset(p->shots,0,sizeof(p->shots));p->shot_command=0;p->previous_actions=0;}
    memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"combat position, neutral weapons and empty owned pools restore");door_install_player(&setup);X3CoopViewsConfigure(false);
    memset(g_ram+0xd18,0,0x3c0);memset(g_ram+0x1318,0,0x500);
    for(unsigned col=0;col<255;++col){unsigned first=view_read(0xf800+2*col),end=view_read(0xf802+2*col);if(first<0xfa02 || end>0xfffc || end<first || (end-first)%5)continue;for(unsigned at=first;at<end;at+=5)g_ram[at]=1;}
    view_word(0x1e5d,256);view_word(0x1e7a,256);const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};for(unsigned i=0;i<6;++i)view_word(vertical[i],1024);
    for(unsigned i=0;i<12;++i)frame(0);check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"both native combat actors settle on actual stage1 ground");n=RtlSaveSnapshotToMemory(base,capacity);
    for(unsigned mode=getenv("X3_COOP_COMBAT_COMBO_ONLY")?4:0;mode<((getenv("X3_COOP_COMBAT_RETIREMENT_ONLY") || getenv("X3_COOP_COMBAT_POOL_ONLY"))?0:getenv("X3_COOP_COMBAT_COMBO_ONLY")?5:4);++mode){
      unsigned frames=mode==4?420:240;
      unsigned peaks[2]={0,0},maxcharge[2]={0,0},saber_hits=0,saber_first=FRAMES,saber_last=0;uint64_t actions[2]={0,0},kinds[2]={0,0};
      for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(base,n),"original native firing/charge reference restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;memcpy(work,base,n);check(X3CoopEncode(&solo,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"original native reference selects its actual snapshot actor");door_install_player(&solo);X3CoopRuntimeEnable(false);
        for(unsigned f=0;f<frames;++f){frame(combat_buttons(mode,seat,f));CombatSample *sample=&trace[seat*FRAMES+f];combat_read(sample,seat,true);if(mode==4 && f>=216 && solo.players[seat].character==X3_COOP_ZERO){combat_probe(sample,seat,true,probe,capacity);if(sample->pose[0]==0x5c && (sample->hit[0]&127)<127){++saber_hits;if(f<saber_first)saber_first=f;saber_last=f;}}if(sample->pose[0]<128)actions[seat]|=UINT64_C(1)<<(sample->pose[0]/2);for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)if(sample->shots[j] && sample->shots[j+10]<64)kinds[seat]|=UINT64_C(1)<<sample->shots[j+10];unsigned count=0;for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)count+=sample->shots[j]!=0;if(count>peaks[seat])peaks[seat]=count;if(sample->charge[1]>maxcharge[seat])maxcharge[seat]=sample->charge[1];}}
      check(RtlLoadSnapshotFromMemory(base,n),"concurrent combat restores the matched original prerequisites");unsigned tick=g_ram[0x9cc];
      for(unsigned f=0;f<frames;++f){frame(combat_buttons(mode,0,f)|(combat_buttons(mode,1,f)<<12));for(unsigned seat=0;seat<2;++seat){CombatSample actual;combat_read(&actual,seat,false);if(mode==4 && f>=216 && X3CoopRuntimeState()->players[seat].character==X3_COOP_ZERO)combat_probe(&actual,seat,false,probe,capacity);CombatSample *expected=&trace[seat*FRAMES+f];if(memcmp(expected,&actual,sizeof(actual))){unsigned at=0;while(at<sizeof(actual) && ((uint8_t *)expected)[at]==((uint8_t *)&actual)[at])++at;fprintf(stderr,"combat mismatch roster%u mode%u seat%u frame%u byte%u original%02x paired%02x charge%u/%u command%02x/%02x\n",roster,mode,seat,f,at,((uint8_t *)expected)[at],((uint8_t *)&actual)[at],expected->charge[1],actual.charge[1],expected->command,actual.command);fprintf(stderr,"native control fields AC/59/6E/3B/33/7D/AA/8E/36/37/38/39 original");for(unsigned k=0;k<12;++k)fprintf(stderr," %02x",expected->controls[k]);fprintf(stderr," paired");for(unsigned k=0;k<12;++k)fprintf(stderr," %02x",actual.controls[k]);fprintf(stderr,"\n");check(false,"concurrent native command, charge, palette and all owned pools equal the original actor on every frame");}}}
      check((uint8_t)(g_ram[0x9cc]-tick)==(uint8_t)frames,"concurrent combat advances one native world tick per frame");
      if(!mode)check(peaks[0]>=2 && peaks[1]>=2,"rapid simultaneous firing actually pressures both original native shot limits");else if(mode==1)check(maxcharge[0]>=4 && maxcharge[1]>=4,"staggered releases actually exercise both original native charge progressions");
      fprintf(stderr,"native concurrent combat roster%u mode%u frames%u originalShotPeaks%u/%u originalChargeMax%u/%u\n",roster,mode,frames,peaks[0],peaks[1],maxcharge[0],maxcharge[1]);
      if(mode==4){fprintf(stderr,"native saber damage roster%u activeFrames%u first%u last%u\n",roster,saber_hits,saber_first,saber_last);check(saber_hits>0,"actual native saber swing includes successful damage windows matching the original beside X attacks");}
      if(mode==4)fprintf(stderr,"native full chain roster%u actionMasks%016llx/%016llx shotKindMasks%016llx/%016llx\n",roster,(unsigned long long)actions[0],(unsigned long long)actions[1],(unsigned long long)kinds[0],(unsigned long long)kinds[1]);
      size_t result=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(base,n),"native concurrent combat restores for complete replay");for(unsigned f=0;f<frames;++f)frame(combat_buttons(mode,0,f)|(combat_buttons(mode,1,f)<<12));same(future,result,work,RtlSaveSnapshotToMemory(work,capacity),"concurrent native firing, charge, owned effects and the complete machine replay exactly");
    }
    if(getenv("X3_COOP_COMBAT_RETIREMENT_ONLY"))combat_retirement(base,n,work,future,capacity);
    if(getenv("X3_COOP_COMBAT_POOL_ONLY"))combat_effect_pressure(base,work,future,capacity);
  }
  free(probe);free(trace);
}
