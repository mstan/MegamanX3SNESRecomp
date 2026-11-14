/* Native deaths/retry on the authored stage5 spikes. Positions and the camera
 * prepare hazards; life, damage, retry and resource results remain original. */
static void team_hazard_body(uint8_t *b,unsigned x,unsigned y){
  b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);
  b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);
  b[4]=b[7]=b[2]=b[3]=0;memset(b+0x1a,0,6);
}
static void native_team_retry_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *expected,size_t capacity){
  check(interp_bridge_add_pre_opcode_hook(0x00e30a,shared_pit_probe),"team retry observes original last-survivor pit damage");
  bool streaming=getenv("X3_COOP_RETRY_STREAMING")!=NULL;
  for(unsigned roster=0;roster<2;++roster)for(unsigned order=getenv("X3_COOP_TEAM_EXTRA_ONLY")?3:0;order<(streaming?1:6);++order){
    bool stock=order==3,absent=order==4,pit=order==5;
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==5 &&
        !X3CoopRuntimeLearner() && !X3CoopRuntimeStockZero(),"team retry starts in the actual default stage5 roster");
    X3CoopState setup=*X3CoopRuntimeState();unsigned x=setup.players[0].character==X3_COOP_X?0:1;
    uint8_t permanent[3]={g_ram[0x1fd1],g_ram[0x1fd3],g_ram[0x1fd4]},reserves[4];memcpy(reserves,g_ram+0x1fb7,4);
    if(streaming)check((permanent[0]&0x20) && (permanent[2]&1) && setup.players[x].maximum_health==18 && g_ram[0xfa84],"retry starts with actual earned heart/subtank and a natively visited moving support");
    unsigned first=(order==2 || stock || pit)?x^1:x,lives=g_ram[0x1fb4];
    check(setup.players[0].character==roster && lives>0 &&
        setup.players[0].life==X3_COOP_ALIVE && setup.players[1].life==X3_COOP_ALIVE,
        "team hazard prerequisite has both original participants and a native retry resource");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);
    if(stock){
      base[door_scene_chunk(base,n)+16+6]|=1;
      check(RtlLoadSnapshotFromMemory(base,n) && !(g_ram[0x1fb2]&64),"stock retry starts before Zero's real native loss");
      g_ram[0x1fe1]|=1; /* Stock Zero has already been called in this stage. */
    }
    if(absent){
      for(unsigned i=0;i<95;++i)frame((uint32_t)SNES_PAD_SELECT<<((x^1)*12));frame(0);
      check(X3CoopRuntimeState()->players[x^1].life==X3_COOP_ABSENT &&
          X3CoopRuntimeState()->players[x].life==X3_COOP_ALIVE,"real Select withdraws the partner before last-survivor death");
    }
    n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();
    const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
    unsigned original_camera[6];for(unsigned i=0;i<6;++i)original_camera[i]=view_read(vertical[i]);
    for(unsigned seat=0;seat<2;++seat){
      bool hazard=!order || seat==first;unsigned y=setup.players[seat].character==X3_COOP_ZERO?(hazard?1392:1367):(hazard?1400:1375);
      team_hazard_body(setup.players[seat].body,hazard?1680:1568,y);
      if(streaming)setup.players[seat].body[0x88]=0; /* End the approach assistance before real fatal contact. */
    }
    shared_prepare(base,n,work,&setup);view_word(0x1e5d,1456);view_word(0x1e7a,1456);
    for(unsigned i=0;i<sizeof(vertical)/sizeof(*vertical);++i)view_word(vertical[i],1280);
    shared_pit_victim=x;shared_pit_damage=0;
    unsigned death[2]={~0u,~0u},elapsed=0,cut_frame=0;size_t cut_size=0;
    bool moved=false,wipe=false;
    for(;elapsed<3600;++elapsed){
      if(!wipe){cut_size=RtlSaveSnapshotToMemory(base,capacity);cut_frame=elapsed;}
      frame(0);const X3CoopState *s=X3CoopRuntimeState();
      for(unsigned seat=0;seat<2;++seat)if(death[seat]==~0u && s->players[seat].life==X3_COOP_FALLEN)death[seat]=elapsed;
      if(!(g_ram[0x1fb4]==lives || g_ram[0x1fb4]+1==lives))check(false,"one team wipe cannot debit multiple native lives");
      if(order && !absent && !moved && s->players[first].life==X3_COOP_FALLEN){
        check(s->players[first^1].life==X3_COOP_ALIVE && !s->wipe,"first native fatal contact preserves the sole survivor");
        n=RtlSaveSnapshotToMemory(work,capacity);setup=*s;uint8_t *b=setup.players[first^1].body;
        team_hazard_body(b,pit?1216:1680,pit?2703:setup.players[first^1].character==X3_COOP_ZERO?1392:1400);
        shared_prepare(work,n,base,&setup);moved=true;s=X3CoopRuntimeState();
        if(pit){
          view_word(0x1e5d,1000);view_word(0x1e7a,1000);
          for(unsigned i=0;i<6;++i)view_word(vertical[i],original_camera[i]);
        }
      }
      if(s->wipe)wipe=true;
      if(wipe && s->initialized && !s->wipe && s->players[x].life==X3_COOP_ALIVE &&
          (stock?s->players[x^1].life==X3_COOP_ABSENT:s->players[x^1].life==X3_COOP_ALIVE) &&
          X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() &&
          g_ram[0xd1]==4 && g_ram[0xd2]==4)break;
    }
    const X3CoopState *s=X3CoopRuntimeState();
    fprintf(stderr,"native team retry roster%u order%u deaths%u/%u frames%u wipe%u lives%u->%u stage%u\n",
        roster,order,death[0],death[1],elapsed,wipe,lives,g_ram[0x1fb4],g_ram[0x1fae]);
    check(elapsed<3600 && wipe && g_ram[0x1fb4]+1==lives && g_ram[0x1fae]==5,
        "actual team deaths reach one original stage retry with one shared resource debit");
    check(!order?death[0]==death[1]:absent?death[x]!=~0u && death[x^1]==~0u:death[first]<death[first^1],
        "actual native damage follows the requested simultaneous or character-ordered death trigger");
    if(pit)check(shared_pit_damage==1,"last survivor dies through the original native pit call exactly once");
    if(stock)check((g_ram[0x1fb2]&64) && !X3CoopRuntimeRejoin(x^1),
        "native retry retains contact-earned stock Zero loss and refuses his rejoin");
    for(unsigned seat=0;seat<2;++seat){const X3CoopPlayer *p=&s->players[seat];
      check(p->character==(seat^roster) && p->body[0xb6]==(p->character==X3_COOP_ZERO?2:0) &&
          (p->body[0x27]&127)==p->maximum_health && p->maximum_health==setup.players[seat].maximum_health,
          "native retry reconstructs each requested character with its own full resources");
    }
    size_t complete=RtlSaveSnapshotToMemory(expected,capacity);
    if(streaming){
      check(g_ram[0x1fd1]==permanent[0] && g_ram[0x1fd3]==permanent[1] && g_ram[0x1fd4]==permanent[2] && !memcmp(reserves,g_ram+0x1fb7,4),"native retry retains both actually earned permanent rewards and shared reserves");
      check(!g_ram[0xfa84] && !view_has_enemy(1224,1464),"native retry resets the previously visited support's admission and old live object");
      const char *output=getenv("X3_COOP_RETRY_STREAMING_OUTPUT");check(output!=NULL,"retry retains its actual reward/eligibility result");char path[1024];snprintf(path,sizeof(path),"%s-roster%u.sav",output,roster);check(RtlSaveSnapshot(path),"actual reward-preserving native retry saves");
    }
    check(cut_size && RtlLoadSnapshotFromMemory(base,cut_size),"last real frame before the team wipe restores");
    for(unsigned i=cut_frame;i<=elapsed;++i)frame(0);
    n=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,complete,work,n,"fatal contact, full wipe, resource debit and native retry replay the complete machine");
    unsigned driver=stock?x:0,before=coop_x(driver),clock=g_ram[0x9cc];
    for(unsigned f=0;f<24;++f)frame((uint32_t)SNES_PAD_RIGHT<<(driver*12));
    check(coop_x(driver)>before && (uint8_t)(g_ram[0x9cc]-clock)==24 && g_ram[0x1fb4]+1==lives,
        "retried team keeps actual player control, one world tick per frame and a stable single debit");
  }
}
