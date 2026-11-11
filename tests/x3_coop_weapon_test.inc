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
    for(unsigned index=first;index<=last;++index)for(unsigned mode=getenv("X3_COOP_WEAPON_MODE_FIRST")?(unsigned)atoi(getenv("X3_COOP_WEAPON_MODE_FIRST")):0;mode<modes;++mode){
      check(RtlLoadSnapshotFromMemory(base,n),"native weapon case restores the same real arena");unsigned address=0x1fbc+(index-1)*2;g_ram[address]=(uint8_t)(mode==2?0x40:0x5c);
      frame(SNES_PAD_R<<(x*12));for(unsigned f=0;f<4;++f)frame(0);
      check(X3CoopRuntimeState()->players[x].body[0x33]==index*2 && !X3CoopRuntimeState()->players[x^1].body[0x33],"actual native input selects only the acquired X weapon and leaves Zero's selection untouched");
      size_t sn=RtlSaveSnapshotToMemory(start,capacity);unsigned frames=mode==1?420:240,ran=0,damage=0,spent=0,charge=0,moving=0;
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
