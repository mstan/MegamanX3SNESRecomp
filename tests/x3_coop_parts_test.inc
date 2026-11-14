/* Authored Maoh head and two shielded arm records. Native input creates
 * attacks; collision positioning and health/protection assist difficulty. */
static unsigned parts_target,parts_reflections,parts_contacts;
static bool parts_observing;
static void parts_response_probe(CpuState *cpu,uint32_t pc){
  if(!parts_observing || cpu->D!=parts_target)return;
  if(pc==0x04cf13){++parts_reflections;return;}
  if(parts_target==0xd18)return;
  ++parts_contacts;
  /* Arms move during their original update. Position the already input-created
   * attacks at the actual contact boundary, after that movement; no hit,
   * damage, reflection, cooldown or readiness value is supplied. */
  for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)if(g_ram[X3_COOP_SHOTS+i]){
    memcpy(g_ram+X3_COOP_SHOTS+i+5,g_ram+parts_target+5,2);
    memcpy(g_ram+X3_COOP_SHOTS+i+8,g_ram+parts_target+8,2);
  }
}
static void parts_native_hit(unsigned d){
  CpuState caller=g_cpu;contact_native_call(d,0x04cb74);
  uint64_t master=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;
  g_cpu=caller;g_cpu.master_cycles=master;g_cpu.cycles=cycles;
}
static void native_maoh_parts_checks(const char *fixture,uint8_t *work,uint8_t *after,size_t capacity){
  const char *prepared_output=getenv("X3_COOP_PARTS_PREPARE_OUTPUT");
  if(prepared_output && *prepared_output){
    check(RtlLoadSnapshot(fixture) && !g_ram[0x1fae] && !g_ram[0x1fd3] &&
        !X3CoopRuntimeSceneActive(),"multipart preparation continues an actually earned native rescue");
    unsigned f=0;
    bool native_only=getenv("X3_COOP_PARTS_NATIVE_ROUTE")!=NULL;
    if(native_only){
      X3CoopState solo=*X3CoopRuntimeState();solo.current=solo.anchor=solo.players[0].character==X3_COOP_X?0:1;
      door_install_player(&solo);X3CoopViewsConfigure(false);X3CoopRuntimeEnable(false);
    }
    for(f=0;f<2400;++f){
      unsigned keys=SNES_PAD_RIGHT|(f%24<6?SNES_PAD_B:0);
      frame(g_ram[0x1f18]!=6 && !X3CoopRuntimeSceneActive() && !g_ram[0x1f22]?keys*(native_only?1u:0x1001u):0);
      if(g_ram[0xd18] && g_ram[0xd22]==0x50 && g_ram[0xd19]==2 && g_ram[0xd1a]>=8 && g_ram[0xd1a]<16 &&
          (g_ram[0xd3f]&127)==32 && g_ram[0xd58] && g_ram[0xd80]==13 &&
          !X3CoopRuntimeSceneActive() && !g_ram[0x1f22] && !g_ram[0x1f25] && !g_ram[0x1f26] && !g_ram[0x1f27])break;
    }
    fprintf(stderr,"native PARTS current preparation checkpoint%u f%u actors%u,%u/%u,%u root%02x/%u HP%u arms%02x/%02x\n",g_ram[0x1fb5],f,support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8),g_ram[0xd18+10],g_ram[0xd18+1],g_ram[0xd18+0x27],g_ram[0xd58+10],g_ram[0xd98+10]);
    check(RtlSaveSnapshot(prepared_output),"current native multipart preparation retains its actual constructor result without a picker");check(f<2400,"the authored Maoh constructor and shielded arms actually initialize after the native route");fixture=prepared_output;
    if(native_only)return;
  }
  check(interp_bridge_add_pre_opcode_hook(0x04cf13,parts_response_probe) &&
      interp_bridge_add_pre_opcode_hook(0x04cb74,parts_response_probe),"armored part observation uses the original contact and negative-damage response branches");
  uint8_t *armed=malloc(capacity),*cut=malloc(capacity),*future=malloc(capacity);check(armed && cut && future,"native multipart comparison futures allocate");
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(fixture) && !g_ram[0x1fae] && g_ram[0xd18+10]==0x50 && g_ram[0xd58+10]==0x50 && g_ram[0xd98+10]==0x50,"multipart checks load the actual native intro boss and its constructed arm records");
    size_t n=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    if(roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}setup.current=setup.anchor=0;
    for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&setup.players[seat];team_hazard_body(p->body,6880+32*seat,360-(p->character==X3_COOP_ZERO?8:0));p->body[0x27]=p->maximum_health;p->body[0x33]=p->body[0x57]=p->body[0x58]=p->body[0x8a]=0;p->body[0x88]=255;memset(p->body+0x38,0,4);p->previous_actions=0;}
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"multipart approach installs only the requested roster, position and native attack prerequisites");door_install_player(&setup);X3CoopViewsConfigure(false);
    n=RtlSaveSnapshotToMemory(cut,capacity);
    for(unsigned part=0;part<3;++part){
      check(RtlLoadSnapshotFromMemory(cut,n),"each actual native part restores its pre-attack world");
      unsigned target=0xd18+part*64,wait=0;
      if(part)for(;wait<600;++wait){
        if(g_ram[target+1]==2 && g_ram[target+3]>=4 && g_ram[target+2]!=0x14 && (g_ram[target+14]&1))break;
        if(!(wait%120))for(unsigned seat=0;seat<2;++seat)weapon_protect(seat,255,work,capacity);
        frame(0);
      }
      check(wait<600,"the actual arm reaches its original contact-enabled state before firing");
      for(unsigned f=0;f<3;++f)frame(SNES_PAD_LEFT*0x1001u);
      for(unsigned f=0;f<3;++f)frame(f?0:SNES_PAD_Y*0x1001u);
      size_t attack=RtlSaveSnapshotToMemory(armed,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
      for(unsigned seat=0;seat<2;++seat){unsigned active=0;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)active+=setup.players[seat].shots[i]!=0;
        fprintf(stderr,"native PARTS input roster%u part%u seat%u shots%u wait%u phase%u/%u\n",roster,part,seat,active,wait,g_ram[target+2],g_ram[target+3]);
        check(active,"both real native buster pools exist before multipart collision positioning");}
      for(unsigned first=0;first<2;++first){
      check(RtlLoadSnapshotFromMemory(armed,attack),"native part comparison restores the same input-created attacks");unsigned d=0xd18+part*64,tx=view_read(d+5),ty=view_read(d+8),hp=g_ram[d+0x27]&127;
      check(g_ram[d] && g_ram[d+1]==2 && g_ram[d+14] && g_ram[d+0x28]==(part?13:16),"the authored head and arm retain their original vulnerable or shielded damage row");
      X3CoopState s=*X3CoopRuntimeState();X3CoopCapture(&s,g_ram);s.current=s.anchor=(uint8_t)first;
      for(unsigned seat=0;seat<2;++seat){campaign_position_only(&s.players[seat],tx+(seat==first?20:160),ty+32);for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)if(s.players[seat].shots[i]){uint8_t *b=s.players[seat].shots+i;b[5]=(uint8_t)tx;b[6]=(uint8_t)(tx>>8);b[8]=(uint8_t)ty;b[9]=(uint8_t)(ty>>8);}}
      size_t cn=RtlSaveSnapshotToMemory(work,capacity);check(X3CoopEncode(&s,work+contact_context_offset(work,cn),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,cn),"actual native projectiles receive collision geometry without a hit, reflection or part-result write");door_install_player(&s);cn=RtlSaveSnapshotToMemory(after,capacity);
      parts_target=d;parts_reflections=parts_contacts=0;parts_observing=true;X3CoopRuntimeEnable(false);if(part)parts_native_hit(d);else for(unsigned f=0;f<2;++f)frame(0);parts_observing=false;
      if(part)check(parts_contacts,"the naturally contact-enabled original arm executes its real collision routine");
      uint8_t native_part[64],native_shots[X3_COOP_SHOT_BYTES],native_hp[3];memcpy(native_part,g_ram+d,64);memcpy(native_shots,g_ram+X3_COOP_SHOTS,sizeof(native_shots));for(unsigned i=0;i<3;++i)native_hp[i]=g_ram[0xd18+i*64+0x27];unsigned reflected=parts_reflections;
      fprintf(stderr,"native PARTS original roster%u part%u first%u HP%u/%u reflect%u phase%u/%u\n",roster,part,first,hp,g_ram[d+0x27]&127,reflected,g_ram[d+1],g_ram[d+2]);
      check(part?(reflected && (g_ram[d+0x27]&127)==hp):((g_ram[d+0x27]&127)<hp),"the actual original part earns either a shield reflection or native head damage");
      check(RtlLoadSnapshotFromMemory(after,cn),"paired multipart contact restores the identical native prerequisites");parts_reflections=0;parts_observing=true;if(part)parts_native_hit(d);else for(unsigned f=0;f<2;++f)frame(0);parts_observing=false;
      check(!memcmp(native_part,g_ram+d,64),"the contacted native part retains its complete original response beside the competing actor");
      for(unsigned i=0;i<3;++i)check(g_ram[0xd18+i*64+0x27]==native_hp[i],"a second native attack pool cannot bypass armor or damage another boss part");
      check(!memcmp(native_shots,X3CoopRuntimeState()->players[first].shots,sizeof(native_shots)),"the original successful attack retains its full native projectile and reflection history");
      if(part){
        check(parts_reflections==reflected,"the first native shield reflection cannot become a second actor contact opportunity");
        check(!memcmp(s.players[first^1].shots,X3CoopRuntimeState()->players[first^1].shots,X3_COOP_SHOT_BYTES),
            "a reflected first native attack leaves the competing actor's complete live projectile pool unchanged");
      }
      for(unsigned f=0;f<64;++f)frame(0);size_t finish=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(after,cn),"native multipart result restores before the complete repeated future");parts_observing=true;if(part)parts_native_hit(d);else for(unsigned f=0;f<2;++f)frame(0);parts_observing=false;for(unsigned f=0;f<64;++f)frame(0);same(future,finish,work,RtlSaveSnapshotToMemory(work,capacity),"authored multipart attacks, shield responses and both owned pools repeat the complete machine");
      fprintf(stderr,"native PARTS PASS roster%u part%u first%u reflect%u\n",roster,part,first,parts_reflections);
      }
    }
  }
  free(armed);free(cut);free(future);
}
