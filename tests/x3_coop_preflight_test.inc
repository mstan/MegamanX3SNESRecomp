/* Current initialized snapshots must reject malformed personal/pass/view/menu
 * input before mutating any guest or title state. Exact supported wire fields
 * are used; this is not a fuzzer or an old-state migration contract. */
static void native_transactional_snapshot_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && X3CoopRuntimeState()->initialized,"transactional snapshot loads its actual initialized roster");size_t n=RtlSaveSnapshotToMemory(base,capacity);
    size_t actor=contact_context_offset(base,n),stride=(X3CoopEncodedSize()-8)/2,scene=door_scene_chunk(base,n),views=scene+16+8,bounds=views+X3_COOP_VIEW_STATE_BYTES+67,presentation=0;
    for(size_t i=bounds+114;i+8<n;++i)if(!memcmp(base+i,"X3P5",4)){presentation=i;break;}
    check(presentation && actor+X3CoopEncodedSize()<scene && n>=X3_COOP_MENU_STATE_BYTES+31,"current transactional snapshot locates its real serialized chunks");
    struct InvalidField {const char *name;size_t at;uint8_t value;} cases[]={
      {"invalid current seat",actor+5,2},{"invalid anchor",actor+6,2},
      {"invalid lifecycle",actor+8+stride-5,3},{"duplicate characters",actor+8+stride+stride-4,base[actor+8+stride-4]},
      {"invalid maximum health",actor+8+stride-1,0},
      {"invalid controller pass",actor+X3CoopEncodedSize()+24,3},{"invalid terrain pass",actor+X3CoopEncodedSize()+26+24,3},
      {"invalid scene owner",scene+6,2},{"invalid scene kind",scene+11,255},{"invalid physical history",scene+12,(uint8_t)(base[scene+12]|1)},
      {"invalid participation profile",scene+16+6,128},
      {"invalid independent policy",views,2},{"invalid view mask",views+1,4},{"invalid camera coordinate",views+6,0x21},
      {"invalid spawn count",views+X3_COOP_VIEW_STATE_BYTES+64,65},{"unknown camera pass",bounds+24,4},{"orphan personal camera backup",bounds+24,3},
      {"invalid presentation identity",presentation+6,2},{"invalid rider bits",n-X3_COOP_MENU_STATE_BYTES-31,4},{"invalid menu header",n-X3_COOP_MENU_STATE_BYTES,255}
    };
    for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);++i){memcpy(work,base,n);work[cases[i].at]=cases[i].value;bool accepted=RtlLoadSnapshotFromMemory(work,n);if(accepted)fprintf(stderr,"malformed snapshot accepted roster%u case%s offset%zu\n",roster,cases[i].name,cases[i].at);check(!accepted,"malformed current actor/pass/view/scene/presentation/menu input is refused before load");size_t an=RtlSaveSnapshotToMemory(after,capacity);same(base,n,after,an,"malformed current input leaves the complete machine unchanged");}
    size_t cuts[]={actor+4,actor+8+stride/2,views+150,n-1};for(unsigned i=0;i<sizeof(cuts)/sizeof(*cuts);++i){check(!RtlLoadSnapshotFromMemory(base,cuts[i]),"truncated current actor/view/menu chunk is refused before load");same(base,n,after,RtlSaveSnapshotToMemory(after,capacity),"truncated current input leaves the complete machine unchanged");}
    fprintf(stderr,"transactional snapshot roster%u malformed%u truncations%u machineBytes%zu\n",roster,(unsigned)(sizeof(cases)/sizeof(*cases)),(unsigned)(sizeof(cuts)/sizeof(*cuts)),n);
  }
}
