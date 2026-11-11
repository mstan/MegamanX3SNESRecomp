/* Native stage loader preparation bypasses prior travel. It does not prove
 * the stage-end trigger or reward flow, and retirement here is an API setup. */
static void stage_request(void){
  g_ram[0x1fae]=5;g_ram[0x1faf]=0;g_ram[0x1fb3]=0;
  g_ram[0x1fb5]=g_ram[0x1fb6]=0;g_ram[0x1ebf]=1;
  g_ram[0xd1]=2;g_ram[0xd2]=4;g_ram[0xd3]=0;
}
static void native_stage_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *pending=malloc(capacity);check(pending!=NULL,"stage pending buffer allocates");
  check(RtlLoadSnapshotFromMemory(original,size),"scripted reconstruction probe starts from original machine");
  X3CoopState scripted=*X3CoopRuntimeState();
  g_ram[X3_COOP_BODY+1]=0;
  view_native_call(0x048000,0,true);
  check(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() &&
        !memcmp(&scripted,X3CoopRuntimeState(),sizeof(scripted)),
        "scripted native reconstruction is not a gameplay enrollment boundary");
  for(unsigned reversed=0;reversed<2;++reversed)for(unsigned mode=0;mode<5;++mode)
  for(unsigned owner=0;owner<2;++owner){
    if(mode && owner==((mode-1)&1))continue;
    check(RtlLoadSnapshotFromMemory(original,size),"native stage loader starts from original actors");
    X3CoopState setup=*X3CoopRuntimeState();
    if(reversed){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
    setup.anchor=setup.current=(uint8_t)owner;
    if(mode){unsigned retired=(mode-1)&1;
      check(mode<=2?X3CoopFall(&setup,retired):X3CoopWithdraw(&setup,retired),"stage retirement uses the lifecycle API");}
    memcpy(work,original,size);
    check(X3CoopEncode(&setup,work+contact_context_offset(original,size),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,size),"stage roster/owner precondition restores");
    door_install_player(&setup);stage_request();
    unsigned waited=0;
    do{frame(0);++waited;}while(X3CoopRuntimeReady() && waited<1200);
    check(waited<1200 && X3CoopRuntimeSceneActive(),"real native constructor enters pending stage handoff");
    const X3CoopState before=*X3CoopRuntimeState();
    fprintf(stderr,"pending roster%u mode%u owner%u: native%u/%u HP%u/%u expected%u/%u life%u/%u expected%u/%u\n",
            reversed,mode,owner,before.players[0].body[0xb6],before.players[1].body[0xb6],
            before.players[0].body[0x27],before.players[1].body[0x27],setup.players[0].body[0x27],setup.players[1].body[0x27],
            before.players[0].life,before.players[1].life,setup.players[0].life,setup.players[1].life);
    for(unsigned seat=0;seat<2;++seat)
      check(before.players[seat].body[0xb6]==setup.players[seat].body[0xb6] &&
            before.players[seat].body[0x27]==setup.players[seat].body[0x27] &&
            before.players[seat].maximum_health==setup.players[seat].maximum_health &&
            before.players[seat].life==setup.players[seat].life,
            "pending native constructor cannot overwrite retained character, HP or life");
    size_t middle=RtlSaveSnapshotToMemory(pending,capacity);
    check(!memcmp(&before,X3CoopRuntimeState(),sizeof(before)),"saving pending arrival never captures temporary native X");
    check(RtlLoadSnapshotFromMemory(pending,middle),"pending native arrival snapshot restores");
    size_t restored=RtlSaveSnapshotToMemory(work,capacity);
    same(pending,middle,work,restored,"pending native arrival immediate restore");
    frame(SNES_PAD_RIGHT|(SNES_PAD_B<<12));
    size_t with_input=RtlSaveSnapshotToMemory(expected,capacity),input_scene=door_scene_chunk(expected,with_input);
    check(expected[input_scene+11]==3 && (expected[input_scene+12] || expected[input_scene+13]) &&
        RtlLoadSnapshotFromMemory(expected,with_input),"pending arrival accepts actual native physical-pad history");
    restored=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,with_input,work,restored,"pending arrival restores actual physical-pad history exactly");
    check(RtlLoadSnapshotFromMemory(pending,middle),"pending arrival resumes its original no-input prerequisite");
    size_t scene_at=door_scene_chunk(pending,middle);
    const unsigned fields[]={4,6,7,8,9,11,12,14};
    for(unsigned k=0;k<sizeof(fields)/sizeof(*fields);++k){
      memcpy(work,pending,middle);
      unsigned f=fields[k];work[scene_at+f]=(uint8_t)(f==6?owner^1:f==7?1:f==9?15:f==11?2:1);
      check(!RtlLoadSnapshotFromMemory(work,middle),"malformed pending stage scene is refused");
      restored=RtlSaveSnapshotToMemory(expected,capacity);
      same(pending,middle,expected,restored,"refused pending scene leaves complete machine unchanged");
    }
    memcpy(work,pending,middle);work[scene_at+9]=4;
    check(!RtlLoadSnapshotFromMemory(work,middle),"pending stage must match the saved native stage");
    memcpy(work,pending,middle);work[scene_at+16+4]=1;
    check(!RtlLoadSnapshotFromMemory(work,middle),"pending stage cannot retain an active participation request");
    /* Current native-pass ABI: two 12-byte register records, then a pass
     * byte and padding. Each constructed continuation must be valid without
     * the stage scene, so refusal proves the cross-state invariant. */
    size_t core=contact_context_offset(pending,middle)+X3CoopEncodedSize();
    const unsigned pass_offsets[]={24,50,76,154,184,190};
    for(unsigned k=0;k<sizeof(pass_offsets)/sizeof(*pass_offsets);++k){
      memcpy(work,pending,middle);work[core+pass_offsets[k]]=1;
      unsigned d=k==0?8:k==1?34:k==2?60:0;
      if(d){unsigned body=k==2?0xd18:X3_COOP_BODY;work[core+d]=(uint8_t)body;work[core+d+1]=(uint8_t)(body>>8);}
      if(k==2){work[core+78]=0x31;work[core+79]=0xcb;work[core+80]=(uint8_t)owner;}
      if(k==3){work[core+156]=0x7f;work[core+157]=0xda;work[core+158]=0x9f;work[core+159]=0xda;}
      if(k==4){work[core+162]=(uint8_t)X3_COOP_SHOTS;work[core+163]=(uint8_t)(X3_COOP_SHOTS>>8);}
      if(k==5){work[core+186]=0x18;work[core+187]=0x0d;work[core+191]=(uint8_t)owner;}
      memset(work+scene_at+4,0,12);
      check(X3CoopRuntimePreflight(work,middle),"control continuation is valid without pending stage");
      memcpy(work+scene_at,pending+scene_at,16);
      check(!RtlLoadSnapshotFromMemory(work,middle),"pending stage refuses a borrowed actor continuation");
    }
    restored=RtlSaveSnapshotToMemory(work,capacity);
    same(pending,middle,work,restored,"refused pending continuations leave complete machine unchanged");
    waited=0;
    do{frame(0);++waited;}while(!X3CoopRuntimeReady() && waited<1800);
    check(waited<1800 && g_ram[0xd1]==4 && g_ram[0xd2]==4,"native arrival completes at ordinary gameplay caller");
    for(unsigned i=0;i<120;++i)frame(0);
    const X3CoopState *s=X3CoopRuntimeState();
    check(s->anchor==owner && s->current==owner && !s->wipe,"stage entry preserves living world owner");
    for(unsigned seat=0;seat<2;++seat){
      const X3CoopPlayer *p=&s->players[seat];
      check(p->life==setup.players[seat].life && p->character==setup.players[seat].character,
            "stage entry cannot revive fallen or absent actor or exchange roster");
      if(p->life!=X3_COOP_ALIVE){
        same(&before.players[seat],sizeof(*p),p,sizeof(*p),"retired personal context survives native stage load");
        continue;
      }
      bool zero=p->character==X3_COOP_ZERO;
      unsigned x=p->body[5]|p->body[6]<<8,y=p->body[8]|p->body[9]<<8;
      fprintf(stderr,"stage roster%u mode%u owner%u seat%u native%u hp%u/max%u at%u,%u pending%u\n",
              reversed,mode,owner,seat,p->body[0xb6],p->body[0x27],p->maximum_health,x,y,waited);
      check(p->body[0xb6]==(zero?2:0) && (p->body[0x27]&127)==(zero?28:16) &&
            p->maximum_health==(zero?28:16),"stage constructor binds each living native character and health");
      check(x==384 && y==(zero?2711:2719),"native stage entry supplies new position and character standing height");
    }
    check((g_ppu->inidisp&0x8f)==15,"surviving native entry retains full brightness");
    unsigned at=coop_x(owner);
    for(unsigned i=0;i<20;++i)frame(SNES_PAD_RIGHT<<(12*owner));
    check(coop_x(owner)>at,"stage survivor accepts real native movement after arrival");
    size_t complete=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(pending,middle),"pending stage restores for full replay");
    for(unsigned i=0;i<waited+120;++i)frame(0);
    for(unsigned i=0;i<20;++i)frame(SNES_PAD_RIGHT<<(12*owner));
    restored=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,complete,work,restored,"pending stage replays complete constructor, lifecycle and movement");
    check(RtlLoadSnapshotFromMemory(expected,complete),"rebound stage snapshot restores");
    restored=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,complete,work,restored,"rebound stage immediate restore");
  }
  free(pending);check(RtlLoadSnapshotFromMemory(original,size),"stage probes restore their starting machine");
}
