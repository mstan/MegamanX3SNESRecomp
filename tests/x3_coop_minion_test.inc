/* Continue an actually marked Hornet homing flight through either real
 * participant's death. The native parent and earned child binding are intact. */
static unsigned minion_source,minion_parent,minion_marked,minion_calls;
static bool minion_probing;
static void minion_target_probe(CpuState *cpu,uint32_t pc){
  (void)pc;if(!minion_probing || cpu->D!=minion_source || !g_ram[cpu->D] || g_ram[cpu->D+1]!=2 || g_ram[cpu->D+0x37]!=2)return;
  ++minion_calls;const X3CoopState *s=X3CoopRuntimeState();
  check(view_read(cpu->D+0x38)==minion_parent,"the live native homing child retains its real parent across survivor handoff");
  check(s->current==minion_marked && g_ram[X3_COOP_BODY+0xb6]==s->players[minion_marked].character*2,"the marked native homing flight cannot inherit the free survivor as its old victim");
}
static void native_marked_minion_survivor(const uint8_t *seed,size_t n,unsigned boss,unsigned source,unsigned marker,unsigned marked,uint8_t *work,size_t capacity){
  check(interp_bridge_add_pre_opcode_hook(0x00dcb5,minion_target_probe),"marked minion ownership observes the real post-selection native dispatch");
  uint8_t *cut=malloc(capacity),*future=malloc(capacity);check(cut && future,"marked minion survivor futures allocate");
  for(unsigned dead=0;dead<2;++dead){
    check(RtlLoadSnapshotFromMemory(seed,n) && g_ram[source] && g_ram[source+0x37]==2 && view_read(source+0x38)==boss && g_ram[marker+0x37]==4,"survivor test retains its actually earned marker, homing flight and common native parent");
    X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;b[0x27]=(uint8_t)(seat==dead?1:setup.players[seat].maximum_health);b[0x88]=(uint8_t)(seat==dead?0:255);b[0x30]=b[0x8a]=b[0x6b]=0;}
    campaign_position_only(&setup.players[dead],view_read(boss+5),view_read(boss+8));campaign_position_only(&setup.players[dead^1],view_read(source+5)+32,view_read(source+8));
    size_t prepared=RtlSaveSnapshotToMemory(work,capacity);check(X3CoopEncode(&setup,work+contact_context_offset(work,prepared),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,prepared),"minion handoff supplies only contact position, low victim HP and survivor protection");door_install_player(&setup);prepared=RtlSaveSnapshotToMemory(cut,capacity);size_t finish=0;
    for(unsigned replay=0;replay<2;++replay){
      check(RtlLoadSnapshotFromMemory(cut,prepared),"marked minion survivor restores before the original fatal contact");CpuState caller=g_cpu;contact_native_call(boss,0x04cb31);uint64_t clock=g_cpu.master_cycles,coprocessor=g_cpu.coprocessor_master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=clock;g_cpu.coprocessor_master_cycles=coprocessor;g_cpu.cycles=cycles;
      check(X3CoopRuntimeState()->players[dead].life==X3_COOP_FALLEN && X3CoopRuntimeState()->anchor==(dead^1) && g_ram[source] && g_ram[boss] && (g_ram[boss+0x27]&127),"actual live-parent body damage kills the selected actor while its marked minion remains in progress");
      unsigned observed=0,before=support_position(dead^1,5);bool retired=false;minion_source=source;minion_parent=boss;minion_marked=marked;
      for(unsigned f=0;f<240;++f){unsigned tick=g_ram[0x9cc];minion_calls=0;minion_probing=!retired;frame(f<24?(uint32_t)(before>8064?SNES_PAD_LEFT:SNES_PAD_RIGHT)<<((dead^1)*12):0);minion_probing=false;
        check(minion_calls<=1 && (uint8_t)(g_ram[0x9cc]-tick)==1,"marked minion and survivor share exactly one native world update per field");observed+=minion_calls;
        if(!g_ram[source] || g_ram[source+1]>=4 || g_ram[source+10]!=0x21 || g_ram[source+0x37]!=2)retired=true;
        check(X3CoopRuntimeState()->players[dead].life==X3_COOP_FALLEN && X3CoopRuntimeState()->players[dead^1].life==X3_COOP_ALIVE && X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor && g_ram[boss] && (g_ram[boss+0x27]&127),"the living encounter retains the fallen actor and canonical survivor without stale child ownership freezing the world");
      }
      fprintf(stderr,"native MARKED MINION SURVIVOR roster%u marked%u dead%u replay%u observed%u retired%u moved%d parentHP%u\n",setup.players[0].character,marked,dead,replay,observed,retired,(int)support_position(dead^1,5)-(int)before,g_ram[boss+0x27]&127);
      check(observed && (retired || observed>=8) && support_position(dead^1,5)!=before,"actual marked minion updates retain their earned target while the free survivor really moves");
      size_t actual=RtlSaveSnapshotToMemory(work,capacity);if(!replay){memcpy(future,work,actual);finish=actual;}else same(future,finish,work,actual,"marked minion, real parent, fallen target and survivor repeat the complete machine");
    }
  }
  free(cut);free(future);
}
