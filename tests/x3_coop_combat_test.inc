/* Isolated native combat: the actual controller/owned-pool loops run normally.
 * Clear shared hostile slots and consume local spawn visits as prerequisites;
 * no attack, charge, capacity, hit or progression result is supplied. */
typedef struct CombatSample {uint8_t command,charge[2],palette[64],aux[X3_COOP_AUX_BYTES],shots[X3_COOP_SHOT_BYTES];} CombatSample;
static unsigned combat_buttons(unsigned mode,unsigned seat,unsigned f){
  if(!mode)return f%4==0?SNES_PAD_Y:0;
  if(mode==1)return f<(seat?216:180)?SNES_PAD_Y:0;
  return seat==(mode==2?0:1)?(f<180?SNES_PAD_Y:0):(f%4==0?SNES_PAD_Y:0);
}
static void combat_read(CombatSample *out,unsigned seat,bool original){
  const X3CoopState *s=X3CoopRuntimeState();const X3CoopPlayer *p=&s->players[seat];bool live=original || s->current==seat;
  const uint8_t *body=live?g_ram+X3_COOP_BODY:p->body;out->command=live?g_ram[0x1f1e]:p->shot_command;
  memcpy(out->charge,body+0x57,2);memcpy(out->palette,live?g_ram+X3_COOP_PALETTE:p->palette,64);
  memcpy(out->aux,live?g_ram+X3_COOP_AUX:p->auxiliaries,X3_COOP_AUX_BYTES);memcpy(out->shots,live?g_ram+X3_COOP_SHOTS:p->shots,X3_COOP_SHOT_BYTES);
}
static void native_concurrent_combat_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *future,size_t capacity){
  enum {FRAMES=240};CombatSample *trace=malloc(2*FRAMES*sizeof(*trace));check(trace!=NULL,"native combat comparison traces allocate");
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
    for(unsigned mode=0;mode<4;++mode){
      unsigned peaks[2]={0,0},maxcharge[2]={0,0};
      for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(base,n),"original native firing/charge reference restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);X3CoopRuntimeEnable(false);
        for(unsigned f=0;f<FRAMES;++f){frame(combat_buttons(mode,seat,f));CombatSample *sample=&trace[seat*FRAMES+f];combat_read(sample,seat,true);unsigned count=0;for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)count+=sample->shots[j]!=0;if(count>peaks[seat])peaks[seat]=count;if(sample->charge[1]>maxcharge[seat])maxcharge[seat]=sample->charge[1];}}
      check(RtlLoadSnapshotFromMemory(base,n),"concurrent combat restores the matched original prerequisites");unsigned tick=g_ram[0x9cc];
      for(unsigned f=0;f<FRAMES;++f){frame(combat_buttons(mode,0,f)|(combat_buttons(mode,1,f)<<12));for(unsigned seat=0;seat<2;++seat){CombatSample actual;combat_read(&actual,seat,false);CombatSample *expected=&trace[seat*FRAMES+f];if(memcmp(expected,&actual,sizeof(actual))){unsigned at=0;while(at<sizeof(actual) && ((uint8_t *)expected)[at]==((uint8_t *)&actual)[at])++at;fprintf(stderr,"combat mismatch roster%u mode%u seat%u frame%u byte%u original%02x paired%02x charge%u/%u command%02x/%02x\n",roster,mode,seat,f,at,((uint8_t *)expected)[at],((uint8_t *)&actual)[at],expected->charge[1],actual.charge[1],expected->command,actual.command);check(false,"concurrent native command, charge, palette and all owned pools equal the original actor on every frame");}}}
      check((uint8_t)(g_ram[0x9cc]-tick)==(uint8_t)FRAMES,"concurrent combat advances one native world tick per frame");
      if(!mode)check(peaks[0]>=2 && peaks[1]>=2,"rapid simultaneous firing actually pressures both original native shot limits");else if(mode==1)check(maxcharge[0]>=4 && maxcharge[1]>=4,"staggered releases actually exercise both original native charge progressions");
      fprintf(stderr,"native concurrent combat roster%u mode%u frames%u originalShotPeaks%u/%u originalChargeMax%u/%u\n",roster,mode,FRAMES,peaks[0],peaks[1],maxcharge[0],maxcharge[1]);
      size_t result=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(base,n),"native concurrent combat restores for complete replay");for(unsigned f=0;f<FRAMES;++f)frame(combat_buttons(mode,0,f)|(combat_buttons(mode,1,f)<<12));same(future,result,work,RtlSaveSnapshotToMemory(work,capacity),"concurrent native firing, charge, owned effects and the complete machine replay exactly");
    }
  }
  free(trace);
}
