#include "../src/x3_coop_present.h"
/* Isolated real-cartridge solid/carry contracts. The geometry record below
 * deliberately supplies no authored platform AI or spawn coverage. */
static unsigned support_position(unsigned seat,unsigned axis) {
  const X3CoopState *s=X3CoopRuntimeState();
  const uint8_t *p=s->current==seat?g_ram+X3_COOP_BODY:s->players[seat].body;
  return p[axis]|p[axis+1]<<8;
}
static void support_move(unsigned slot,int dx,int dy) {
  unsigned x=g_ram[slot+5]|g_ram[slot+6]<<8,y=g_ram[slot+8]|g_ram[slot+9]<<8;
  g_ram[slot+0x22]=(uint8_t)x;g_ram[slot+0x23]=(uint8_t)(x>>8);
  g_ram[slot+0x24]=(uint8_t)y;g_ram[slot+0x25]=(uint8_t)(y>>8);
  x+=dx;y+=dy;
  g_ram[slot+5]=(uint8_t)x;g_ram[slot+6]=(uint8_t)(x>>8);
  g_ram[slot+8]=(uint8_t)y;g_ram[slot+9]=(uint8_t)(y>>8);
}
static void support_geometry(unsigned slot,unsigned x,unsigned y) {
  contact_geometry(slot,x,y);
  /* Native class37 selects06:C5D4 = {0,0,32,16}; do not approximate
   * its top with the player's narrower, vertically offset hurt box. */
  g_ram[slot+0x20]=0xd4;g_ram[slot+0x21]=0xc5;g_ram[slot+0x2c]=0x80;
}
static void native_support_checks(uint8_t *snapshot,size_t size,uint8_t *modified,uint8_t *extra,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);X3CoopState setup;
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"support precondition decodes");
  setup.players[1].body[5]=setup.players[0].body[5];
  setup.players[1].body[6]=setup.players[0].body[6];
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"two-rider position precondition encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"two-rider native support fixture restores");
  unsigned x=support_position(0,5),y=support_position(0,8)+28;
  support_geometry(0xd18,x,y);
  contact_native_call(0xd18,0x04bfae);
  unsigned before[4]={support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8)};
  support_move(0xd18,5,-3);contact_native_call(0xd18,0x04c07f);
  check(support_position(0,5)==before[0]+5 && support_position(0,8)==before[1]-3 &&
        support_position(1,5)==before[2]+5 && support_position(1,8)==before[3]-3,
        "one native carry moves both classified riders by the same platform delta");
  /* Leave only Zero on the first support, then land X on a different one. */
  unsigned moved=support_position(0,5)+64;
  g_ram[X3_COOP_BODY+5]=(uint8_t)moved;g_ram[X3_COOP_BODY+6]=(uint8_t)(moved>>8);
  contact_native_call(0xd18,0x04bfae);
  unsigned platform_y=g_ram[0xd18+8]|g_ram[0xd18+9]<<8;
  support_geometry(0xd58,moved,platform_y);
  contact_native_call(0xd58,0x04bfae);
  before[0]=support_position(0,5);before[1]=support_position(0,8);
  before[2]=support_position(1,5);before[3]=support_position(1,8);
  support_move(0xd18,2,1);support_move(0xd58,-4,2);
  size_t prepared=RtlSaveSnapshotToMemory(modified,capacity);
  contact_native_call(0xd18,0x04c07f);contact_native_call(0xd58,0x04c07f);
  check(support_position(0,5)==before[0]-4 && support_position(0,8)==before[1]+2 &&
        support_position(1,5)==before[2]+2 && support_position(1,8)==before[3]+1,
        "separate supports keep independent rider histories without cross-carry");
  check(RtlLoadSnapshotFromMemory(modified,prepared),"classified support histories restore");
  contact_native_call(0xd18,0x04c07f);contact_native_call(0xd58,0x04c07f);
  check(support_position(0,5)==before[0]-4 && support_position(1,5)==before[2]+2,
        "restored histories carry the same two actors");
  size_t expected=RtlSaveSnapshotToMemory(extra,capacity);
  modified[contact_menu_offset(modified,prepared)-31]|=16;
  check(!RtlLoadSnapshotFromMemory(modified,prepared),"unknown native attachment bits are refused before load");
  size_t after=RtlSaveSnapshotToMemory(modified,capacity);
  same(extra,expected,modified,after,"invalid attachment state leaves the machine unchanged");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"support probes restore their starting machine");
}
static void native_support_turn_checks(uint8_t *base,size_t n,unsigned d,uint8_t *work,size_t capacity){
  enum {count=1050};uint8_t *objects=malloc(count*64);uint16_t (*positions)[count][2]=malloc(2*count*2*sizeof(uint16_t));check(objects && positions,"actual support turn reference allocates");
  frame(0);check(g_ram[d] && g_ram[d+10]==0x37 && g_ram[d+1]==2,"actual lift constructor completes before its native activation prerequisite");n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState armed;check(X3CoopDecode(&armed,base+contact_context_offset(base,n),X3CoopEncodedSize()),"lift activation retains the actual current actors");unsigned caster=armed.players[0].character==X3_COOP_X?0:1;
  for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&armed.players[seat];uint8_t *b=p->body;b[0x88]=255;b[2]=b[3]=0;memset(b+0x1a,0,6);memset(b+0x36,0,6);memcpy(b+0x22,b+5,2);memcpy(b+0x24,b+8,2);p->previous_actions=0;}
  check(X3CoopEncode(&armed,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"lift activation supplies explicit personal damage protection");door_install_player(&armed);g_ram[0x1fd1]|=2;for(unsigned i=0;i<9;++i)g_ram[0x1fbc+i*2]=0;g_ram[0x1fc6]=0xdc;
  for(unsigned f=0;f<12;++f)frame(0);frame((uint32_t)SNES_PAD_R<<(caster*12));for(unsigned f=0;f<4;++f)frame(0);
  check(X3CoopRuntimeState()->players[caster].body[0x33]==12,"native cycling selects the acquired Gravity Beetle weapon slot at its native doubled index");bool original_activation=getenv("X3_COOP_PLATFORM_ORIGINAL")!=NULL;if(original_activation)X3CoopRuntimeEnable(false);unsigned start_y=view_read(d+8),charged=0,peak=0,release=180;bool activated=false,ready=false,actual_charge=false;
  for(;charged<1200;++charged){const uint8_t *previous=original_activation || X3CoopRuntimeState()->current==caster?g_ram+X3_COOP_BODY:X3CoopRuntimeState()->players[caster].body;if(charged>=180 && release==charged && (previous[0x35] || previous[0x7d] || previous[0xaa]))++release;frame((uint32_t)(charged<release?SNES_PAD_Y:0)<<(original_activation?0:caster*12));activated|=g_ram[d] && g_ram[d+10]==0x37 && view_read(d+8)<start_y;const uint8_t *body=X3CoopRuntimeState()->current==caster?g_ram+X3_COOP_BODY:X3CoopRuntimeState()->players[caster].body;if(body[0x58]>peak)peak=body[0x58];if(charged==179 || charged==180 || charged==181 || charged==190)fprintf(stderr,"lift cast original%u f%u xy%u,%u actorState%u/%u charge%u/%u readiness%u/%u special%u action%u latch%02x flags%02x parts%02x freeze%u/%u/%u\n",original_activation,charged,support_position(caster,5),support_position(caster,8),body[1],body[2],body[0x57],body[0x58],body[0x8e],body[0x8f],body[0xb8],body[0x59],g_ram[d+0x2c],g_ram[0x1fff],g_ram[0x1fd1],g_ram[0x1f25],g_ram[0x1f27],g_ram[0x1f29]);
    bool live=false;const X3CoopPlayer *p=&X3CoopRuntimeState()->players[caster];const uint8_t *shots=X3CoopRuntimeState()->current==caster?g_ram+X3_COOP_SHOTS:p->shots,*aux=X3CoopRuntimeState()->current==caster?g_ram+X3_COOP_AUX:p->auxiliaries;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64){live|=shots[i]!=0;actual_charge|=shots[i] && shots[i+10]==21;}for(unsigned i=96;i<X3_COOP_AUX_BYTES;i+=32)live|=aux[i]!=0;
    if(charged>=release && actual_charge && activated && !live){ready=true;break;}
  }
  fprintf(stderr,"native lift activation chargeFrames%u peak%u activated%u ready%u source%u/%u/%u Y%u HP%u energy%02x\n",charged,peak,activated,ready,g_ram[d],g_ram[d+1],g_ram[d+2],view_read(d+8),g_ram[d+0x27],g_ram[0x1fc6]);if(!activated || !ready)check(RtlSaveSnapshot("lift-activation-miss.sav"),"unqualified native lift activation retains the actual running state for visual inspection");check(actual_charge && activated && ready && g_ram[d] && g_ram[d+10]==0x37,"actual charged native weapon activates the authored lift and retires before the rider comparison");n=RtlSaveSnapshotToMemory(base,capacity);
  if(original_activation){free(objects);free(positions);return;}
  /* Keep the bodies produced by the actual activation/ride. Resetting a
   * still scripted caster to an airborne standing pose is not a valid rider. */
  X3CoopState carried;check(X3CoopDecode(&carried,base+contact_context_offset(base,n),X3CoopEncodedSize()),"support stop decodes both actual carried bodies");door_install_player(&carried);
  for(unsigned seat=0;seat<2;++seat){const uint8_t *body=carried.players[seat].body;fprintf(stderr,"actual lift rider seat%u xy%u,%u action%u/%u grounded%02x busy%u\n",seat,body[5]|body[6]<<8,body[8]|body[9]<<8,body[2],body[3],body[0x2b],body[0xb8]);}
  /*03:EA48 gives this authored lift its native512..1024 room. */
  view_word(0x1e6a,512);view_word(0x1e78,512);view_word(0x1e6c,1024);view_word(0x1e7e,1024);
  if(X3CoopViewsIndependent()){X3CoopViewState views=*X3CoopViewsState();for(unsigned seat=0;seat<2;++seat){views.limits[seat][2]=views.current[seat][2]=512;views.limits[seat][3]=views.current[seat][3]=1024;}X3CoopViewsSetState(&views);}n=RtlSaveSnapshotToMemory(base,capacity);
  uint8_t clocks[count];unsigned turn=0,resumed=0,frames=count;
  for(unsigned seat=0;seat<2;++seat){
    check(RtlLoadSnapshotFromMemory(base,n),"support endpoint reference restores the actual native approach");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);X3CoopRuntimeEnable(false);unsigned previous=view_read(d+8);
    fprintf(stderr,"original support start seat%u xy%u,%u source%u,%u\n",seat,view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),view_read(d+5),view_read(d+8));for(unsigned f=0;f<frames;++f){native_gameplay_update(0,true);positions[seat][f][0]=(uint16_t)view_read(X3_COOP_BODY+5);positions[seat][f][1]=(uint16_t)view_read(X3_COOP_BODY+8);
      if(!g_ram[d] || g_ram[d+10]!=0x37){fprintf(stderr,"original support retirement seat%u f%u source%u state%u/%u sourceY%u rider%u,%u action%u camera%u,%u limits%u/%u targets%u/%u phase%u/%u HP%u\n",seat,f,g_ram[d],g_ram[d+1],g_ram[d+2],view_read(d+8),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),g_ram[X3_COOP_BODY+2],view_read(0x1e5d),view_read(0x1e60),view_read(0x1e6a),view_read(0x1e6c),view_read(0x1e78),view_read(0x1e7e),g_ram[0xd1],g_ram[0xd2],g_ram[X3_COOP_BODY+0x27]&127);check(false,"authored reference support remains live through its endpoint");}
      if(!seat){clocks[f]=g_ram[0x9cc];memcpy(objects+f*64,g_ram+d,64);if(!turn && view_read(d+8)+192<=view_read(d+0x36)){turn=f+1;fprintf(stderr,"original support endpoint f%u Y%u riderY%u HP%u\n",turn,view_read(d+8),view_read(X3_COOP_BODY+8),g_ram[X3_COOP_BODY+0x27]&127);}if(turn && !resumed && view_read(d+8)>previous){resumed=f+1;fprintf(stderr,"original support reversal f%u Y%u riderY%u HP%u\n",resumed,view_read(d+8),view_read(X3_COOP_BODY+8),g_ram[X3_COOP_BODY+0x27]&127);}if(resumed && f+1>=resumed+24)frames=f+1;}
      previous=view_read(d+8);
    }
  }
  check(turn && resumed>=turn+60,"original support reaches its actual upper endpoint, waits sixty frames and reverses under native control");
  check(RtlLoadSnapshotFromMemory(base,n),"paired endpoint restores the same actual support and two riders");unsigned clock=g_ram[0x9cc];
  for(unsigned f=0;f<frames;++f){native_gameplay_update(0,false);if(memcmp(objects+f*64,g_ram+d,64)){unsigned difference=0;while(difference<64 && objects[f*64+difference]==g_ram[d+difference])++difference;fprintf(stderr,"support turn mismatch frame%u byte%u expected%02x actual%02x clocks%u/%u nativeY%u pairedY%u state%u/%u\n",f,difference,objects[f*64+difference],g_ram[d+difference],clocks[f],g_ram[0x9cc],objects[f*64+8]|objects[f*64+9]<<8,view_read(d+8),g_ram[d+1],g_ram[d+2]);check(false,"shared support endpoint and reversal match exactly one original native source update on every frame");}
    if(f>=8){check((g_ram[d+0x2c]&1)!=0,"actual support retains native top contact during the stop and reversed motion");for(unsigned seat=0;seat<2;++seat){unsigned height=X3CoopRuntimeState()->players[seat].character==X3_COOP_ZERO?37:29;check(contact_hp(seat)>0 && support_position(seat,8)+height==view_read(d+8),"both living riders remain on the native support surface during the stop and reversal");}}
    for(unsigned seat=0;seat<2;++seat)if(support_position(seat,5)!=positions[seat][f][0] || support_position(seat,8)!=positions[seat][f][1]){fprintf(stderr,"support turn rider mismatch frame%u seat%u original%u,%u paired%u,%u\n",f,seat,positions[seat][f][0],positions[seat][f][1],support_position(seat,5),support_position(seat,8));check(false,"either rider retains original carry through stop, waiting and reversal");}
  }
  check((uint8_t)(g_ram[0x9cc]-clock)==(uint8_t)frames,"endpoint and reversal advance the world once per frame");
  fprintf(stderr,"actual support turn roster%u endpoint%u reverse%u finalY%u riderY%u/%u frames%u\n",X3CoopRuntimeState()->players[0].character,turn,resumed,view_read(d+8),support_position(0,8),support_position(1,8),frames);
  free(objects);free(positions);(void)work;(void)capacity;
}
static void authored_support_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  bool journey=getenv("X3_COOP_PLATFORM_TURN")!=NULL;unsigned platform_x=journey?1080:1224,platform_y=journey?896:1464,camera_y=journey?768:1280,camera_x=platform_x-128,platform_class=journey?0x37:0x26;
  size_t offset=contact_context_offset(snapshot,size);X3CoopState setup;
  check(g_ram[0x1fae]==5,"authored support fixture is native stage5");
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"authored support actors decode");
  unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;setup.current=setup.anchor=(uint8_t)x;
  unsigned zx=platform_x+12,zy=platform_y-45;setup.players[zero].body[5]=(uint8_t)zx;setup.players[zero].body[6]=(uint8_t)(zx>>8);
  setup.players[zero].body[8]=(uint8_t)zy;setup.players[zero].body[9]=(uint8_t)(zy>>8);
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"authored support positioning encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"authored support positioning restores");
  door_install_player(&setup);
  view_word(X3_COOP_BODY+5,platform_x-12);view_word(X3_COOP_BODY+8,platform_y-37);
  /* QA teleport into the actual upper-room band. Camera admission/stream
   * frontiers are bypassed here; the support itself is spawned from ROM. */
  const unsigned camera_addresses[]={0x1e5d,0x1e7a,0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};
  for(unsigned i=0;i<8;++i) {
    unsigned value=i<2?camera_x:camera_y;
    g_ram[camera_addresses[i]]=(uint8_t)value;g_ram[camera_addresses[i]+1]=(uint8_t)(value>>8);
  }
  /* A current independent fixture retains its original private room. Match
   * those cameras to this explicit teleport before asking the union allocator
   * to admit a different room's authored support. */
  if(X3CoopViewsIndependent()){
    X3CoopViewState views=*X3CoopViewsState();
    views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){
      views.camera[seat][0]=(uint16_t)camera_x;views.camera[seat][1]=(uint16_t)camera_y;
      views.limits[seat][0]=views.current[seat][0]=0;
      views.limits[seat][1]=views.current[seat][1]=8192;
      views.limits[seat][2]=views.current[seat][2]=(uint16_t)camera_y;
      views.limits[seat][3]=views.current[seat][3]=(uint16_t)camera_y;
    }
    X3CoopViewsSetState(&views);
  }
  view_word(0,platform_x&~31u);view_word(2,camera_y-32);view_word(4,320);
  cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x00ded3),"native upper-room event column spawns");
  check(g_cpu.S==resume.S,"native spawn precondition preserves its guest stack");
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  unsigned d=0;
  for(unsigned a=0xd18;a<0x10d8;a+=64)if(g_ram[a] && g_ram[a+10]==platform_class)d=a;
  check(d && view_read(d+5)==platform_x && view_read(d+8)==platform_y,
        "actual ROM event creates its authored class26 support at the selected native position");
  size_t prepared=RtlSaveSnapshotToMemory(snapshot,capacity);
  if(getenv("X3_COOP_PLATFORM_TURN")){native_support_turn_checks(snapshot,prepared,d,modified,capacity);return;}
  X3CoopRuntimeEnable(false);
  unsigned clock=g_ram[0x9cc];
  for(unsigned i=0;i<45;++i)frame(0);
  unsigned reference_ticks=(g_ram[0x9cc]-clock)&255;
  uint8_t reference[64];memcpy(reference,g_ram+d,sizeof(reference));
  unsigned reference_y=support_position(x,8);
  fprintf(stderr,"platform baseline: active=%u state=%u/%u xy=%u,%u player=%u,%u latch=%02x ticks=%u phase=%u/%u cameraY=%u\n",
      g_ram[d],g_ram[d+1],g_ram[d+2],g_ram[d+5]|g_ram[d+6]<<8,g_ram[d+8]|g_ram[d+9]<<8,
      support_position(x,5),reference_y,g_ram[d+0x2c],reference_ticks,g_ram[0xd1],g_ram[0xd2],g_ram[0x1e60]|g_ram[0x1e61]<<8);
  int travel=(int)(g_ram[d+8]|g_ram[d+9]<<8)-1464;
  check(g_ram[d] && (travel>=8 || travel<=-8) && (g_ram[d+0x2c]&1),
        "single-seat reference actually lands on and rides the moving support");
  X3CoopRuntimeEnable(true);
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"authored co-op support precondition restores");
  clock=g_ram[0x9cc];
  for(unsigned i=0;i<45;++i)frame(0);
  unsigned ticks=(g_ram[0x9cc]-clock)&255;
  fprintf(stderr,"authored support: ticks=%u/%u y=%u/%u platform=%u/%u\n",ticks,reference_ticks,
      support_position(x,8),support_position(zero,8),g_ram[d+8]|g_ram[d+9]<<8,reference[8]|reference[9]<<8);
  check(ticks==reference_ticks,"authored co-op support preserves native world-update cadence");
  check(!memcmp(reference,g_ram+d,sizeof(reference)),"shared support matches one native platform simulation");
  check(support_position(x,8)==reference_y && support_position(zero,8)+8==reference_y,
        "both actors follow the authored moving support at their native standing heights");
  if(getenv("X3_COOP_DIRTY_RETRY_SUPPORT")){native_dirty_team_retry(false,snapshot,modified,capacity);return;}
  if(getenv("X3_COOP_DIFFICULT_SUPPORT")){native_return_on_support(capacity);return;}
  const char *output=getenv("X3_COOP_PLATFORM_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"authored support visual handoff saves");
  unsigned y;int zero_gap;
  for(unsigned departing=0;departing<2;++departing){
    check(RtlLoadSnapshotFromMemory(snapshot,prepared),"native rider departure restores its original support approach");
    X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)departing;
    X3CoopRuntimeEnable(false);door_install_player(&solo);
    uint16_t native_landing[45][2];bool native_landed=false;
    for(unsigned i=0;i<45;++i){frame(0);native_landing[i][0]=(uint16_t)view_read(X3_COOP_BODY+5);native_landing[i][1]=(uint16_t)view_read(X3_COOP_BODY+8);native_landed=native_landed || (g_ram[d+0x2c]&1);}
    check(native_landed,"original rider reference includes the actual moving-frame landing");
    for(unsigned i=0;i<5;++i)frame(SNES_PAD_LEFT|SNES_PAD_B);
    uint8_t native_velocity[4];memcpy(native_velocity,g_ram+X3_COOP_BODY+0x1a,4);
    unsigned native_x=view_read(X3_COOP_BODY+5),native_y=view_read(X3_COOP_BODY+8);
    check(RtlLoadSnapshotFromMemory(snapshot,prepared),"paired rider departure restores the matched original approach");
    for(unsigned i=0;i<45;++i){frame(0);if(support_position(departing,5)!=native_landing[i][0] || support_position(departing,8)!=native_landing[i][1])check(false,"co-op landing and same-frame carry match the original native rider on every approach/update frame");}
    check(true,"either native rider matches the original landing and same-frame carry throughout the authored support approach");
    for(unsigned i=0;i<5;++i)frame((uint32_t)(SNES_PAD_LEFT|SNES_PAD_B)<<(departing*12));
    const X3CoopState *s=X3CoopRuntimeState();const uint8_t *body=s->current==departing?g_ram+X3_COOP_BODY:s->players[departing].body;
    check(support_position(departing,5)==native_x && support_position(departing,8)==native_y && !memcmp(native_velocity,body+0x1a,4),
        "either rider jumps off with the original native position and horizontal/vertical momentum");
    for(unsigned i=5;i<30;++i)frame((uint32_t)(SNES_PAD_LEFT|SNES_PAD_B)<<(departing*12));
    y=view_read(d+8);unsigned other=departing^1,feet=other==x?21:29,jump_feet=departing==x?21:29;
    int gap=(int)support_position(other,8)+(int)feet-(int)y;
    size_t riding=RtlSaveSnapshotToMemory(modified,capacity);
    check(support_position(departing,8)+jump_feet<y-8 && gap>=0 && gap<=1 &&
        (modified[contact_menu_offset(modified,riding)-31+(d-0xd18)/64]&3)==(1u<<other),
        "jumping off releases only the departing rider while the partner stays on the native moving support");
  }
  check(RtlLoadSnapshotFromMemory(snapshot,prepared),"P2-first authored support setup restores");
  /* X is outside the real38-pixel contact width; both remain in one view. */
  g_ram[X3_COOP_BODY+5]=0x98;g_ram[X3_COOP_BODY+6]=4; /*1176*/
  for(unsigned i=0;i<45;++i)frame(0);
  y=g_ram[d+8]|g_ram[d+9]<<8;
  zero_gap=(int)support_position(zero,8)+29-(int)y;
  check(!memcmp(reference,g_ram+d,sizeof(reference)) && zero_gap>=0 && zero_gap<=1,
        "P2 alone lands, starts and rides the actual support with native movement");
  size_t observed=RtlSaveSnapshotToMemory(modified,capacity);
  check((modified[contact_menu_offset(modified,observed)-31+(d-0xd18)/64]&3)==(1u<<zero),
        "authored P2-only ride retains no false X rider");
  output=getenv("X3_COOP_PLATFORM_P2_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"P2-only support visual handoff saves");
}

/* Two distinct stage5 ROM records,3C:E12C and3C:E1BB, live in vertically
 * separated rooms. Teleports/camera prerequisites bypass admission traversal. */
static void native_separate_authored_support_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *replay,size_t capacity){
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal) && g_ram[0x1fae]==5,"separate supports load the actual stage5 roster");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
    unsigned platform_x[2]={1224,1480},platform_y[2]={1464,1944},camera_y[2]={1280,1792};
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=platform_x[seat],y=platform_y[seat]-(setup.players[seat].character==X3_COOP_ZERO?45:37);
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x88]=255;b[0x8a]=0;}
    memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"separate authored support approach encodes");door_install_player(&setup);
    X3CoopViewsConfigure(true);X3CoopViewState views=*X3CoopViewsState();views.independent=views.initialized=1;views.stage=5;views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=(uint16_t)(platform_x[seat]-128);views.camera[seat][1]=(uint16_t)camera_y[seat];views.limits[seat][0]=views.current[seat][0]=0;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=(uint16_t)camera_y[seat];views.limits[seat][3]=views.current[seat][3]=(uint16_t)camera_y[seat];}X3CoopViewsSetState(&views);
    view_word(0x1e5d,views.camera[setup.anchor][0]);view_word(0x1e7a,views.camera[setup.anchor][0]);view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6e,0);view_word(0x1e70,8192);const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};for(unsigned k=0;k<6;++k)view_word(vertical[k],camera_y[setup.anchor]);
    for(unsigned seat=0;seat<2;++seat){view_word(0,platform_x[seat]&~31u);view_word(2,camera_y[seat]-32);view_word(4,320);cpu_mirrors_to_p(&g_cpu);CpuState resume=g_cpu;g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
      check(interp_bridge_run(&g_cpu,0x00ded3) && g_cpu.S==resume.S,"actual separated native support column allocates with balanced stack");g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;g_cpu.PB=resume.PB;g_cpu.DB=resume.DB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);}
    unsigned slot[2]={0,0};for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x26)for(unsigned seat=0;seat<2;++seat)if(view_read(d+5)==platform_x[seat] && view_read(d+8)==platform_y[seat])slot[seat]=d;
    check(slot[0] && slot[1] && slot[0]!=slot[1],"original stage5 records create two distinct native moving supports");n=RtlSaveSnapshotToMemory(base,capacity);
    uint16_t position[2][45][2];uint8_t record[2][64],child_record[2][32];
    for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(base,n),"separated original rider comparison restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);X3CoopRuntimeEnable(false);g_ram[slot[seat^1]]=0;view_word(0x1e5d,platform_x[seat]-128);view_word(0x1e7a,platform_x[seat]-128);for(unsigned k=0;k<6;++k)view_word(vertical[k],camera_y[seat]);
      for(unsigned f=0;f<45;++f){frame(0);position[seat][f][0]=(uint16_t)view_read(X3_COOP_BODY+5);position[seat][f][1]=(uint16_t)view_read(X3_COOP_BODY+8);}memcpy(record[seat],g_ram+slot[seat],64);unsigned child=view_read(slot[seat]+0x3e);check(child>=0x1800 && child+32<=0x1e00 && g_ram[child] && g_ram[child+10]==0x1e && view_read(child+0x1b)==slot[seat],"each original moving support owns its actual native trail child");memcpy(child_record[seat],g_ram+child,32);
      fprintf(stderr,"separate original roster%u seat%u object%04x active%u state%u/%u variant%02x xy%u,%u rider%u,%u HP%u latch%02x camera%u bounds%u/%u\n",roster,seat,slot[seat],g_ram[slot[seat]],g_ram[slot[seat]+1],g_ram[slot[seat]+2],g_ram[slot[seat]+11],view_read(slot[seat]+5),view_read(slot[seat]+8),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),g_ram[X3_COOP_BODY+0x27]&127,g_ram[slot[seat]+0x2c],view_read(0x1e60),view_read(0x1e6a),view_read(0x1e6c));
      check(g_ram[slot[seat]] && (g_ram[slot[seat]+0x2c]&1) && view_read(slot[seat]+8)!=platform_y[seat],"each original support reference actually moves with its rider");}
    check(RtlLoadSnapshotFromMemory(base,n),"paired separated supports restore their original world");unsigned tick=g_ram[0x9cc];
    for(unsigned f=0;f<45;++f){frame(0);for(unsigned seat=0;seat<2;++seat)if(support_position(seat,5)!=position[seat][f][0] || support_position(seat,8)!=position[seat][f][1])check(false,"separate moving supports retain each original rider position on every native frame");}
    check((uint8_t)(g_ram[0x9cc]-tick)==45,"separate support rooms advance the shared world once");size_t end=RtlSaveSnapshotToMemory(work,capacity);
    for(unsigned seat=0;seat<2;++seat){/*07:E649 stores an allocated shared child slot in3E. With two real supports, slot addresses differ; validate its complete target and parent instead. */unsigned child=view_read(slot[seat]+0x3e);check(child>=0x1800 && child+32<=0x1e00 && g_ram[child] && g_ram[child+10]==0x1e && view_read(child+0x1b)==slot[seat] && !memcmp(child_record[seat],g_ram+child,32),"each separated support retains its original native child behavior and actual parent identity");record[seat][0x3e]=g_ram[slot[seat]+0x3e];record[seat][0x3f]=g_ram[slot[seat]+0x3f];check(!memcmp(record[seat],g_ram+slot[seat],64),"each separated support matches one original native simulation after resolving its allocated child pointer");check((work[contact_menu_offset(work,end)-31+(slot[seat]-0xd18)/64]&3)==(1u<<seat),"two authored supports retain independent actual rider histories");}
    fprintf(stderr,"separate authored supports roster%u slots%04x/%04x riderY%u/%u platformY%u/%u\n",roster,slot[0],slot[1],support_position(0,8),support_position(1,8),view_read(slot[0]+8),view_read(slot[1]+8));
    size_t active=RtlSaveSnapshotToMemory(base,capacity);unsigned input=SNES_PAD_LEFT|SNES_PAD_B|(SNES_PAD_RIGHT<<12);
    for(unsigned f=0;f<24;++f)frame(input);size_t result=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual support departure retains both living actors");
    check(RtlLoadSnapshotFromMemory(base,active),"actual two-rider moving support snapshot restores");same(base,active,replay,RtlSaveSnapshotToMemory(replay,capacity),"restored authored support riders, native children, views and the complete machine match immediately");
    for(unsigned f=0;f<24;++f)frame(input);same(work,result,replay,RtlSaveSnapshotToMemory(replay,capacity),"active authored support restore replays both departures and ownership histories exactly");
  }
}

/* Actual jungle records3C:EC96/EC9E create class1F solid enemies. Position
 * prerequisites bypass travel; native allocation, landing and weapons decide. */
/* Compare the actual surviving private view against voluntary absence at the
 * identical live-contact cut. Only corpse/death presentation may differ. */
static void native_jungle_survivor_visual(const uint8_t *cut,size_t n,unsigned victim,unsigned x,unsigned foe,uint8_t *work,size_t capacity){
  enum {samples=8,body_bytes=64*80*4,world_bytes=48*80*4};const unsigned phases[samples]={0,7,15,31,63,95,127,179};unsigned survivor=victim^1;
  size_t now=RtlSaveSnapshotToMemory(work,capacity);uint8_t *saved=malloc(now),*reference=malloc(samples*(body_bytes+world_bytes));check(saved && reference,"survivor private-view reference allocates");memcpy(saved,work,now);unsigned coordinates[samples][4];
  for(unsigned death=0;death<2;++death){
    check(RtlLoadSnapshotFromMemory(cut,n),"corpse comparison restores the identical real hostile contact and earned support");if(!death)check(X3CoopRuntimeWithdraw(victim),"living withdrawal establishes the same survivor without a corpse");
    unsigned sample=0;for(unsigned f=0;f<180;++f){frame(0);
      /* Fatal contact is already earned. Move only the living hostile away
       * in both branches so later targeting cannot obscure the color check. */
      if(!f){view_word(foe+5,4096);view_word(foe+0x22,4096);view_word(foe+8,4096);view_word(foe+0x24,4096);}
      if(sample>=samples || f!=phases[sample])continue;
      check(X3CoopRuntimeState()->players[victim].life==(death?X3_COOP_FALLEN:X3_COOP_ABSENT) && X3CoopRuntimeState()->players[survivor].life==X3_COOP_ALIVE,"actual corpse/absence comparison retains its intended survivor");
      X3Display_SetPresentationSeat((int)survivor);uint64_t clock=g_cpu.master_cycles;memcpy(work,g_ram,0x20000);CaptureSimulationFrame(1);check(X3Display_RenderedPrivateFrame() && g_ppu->renderPitch>=256*4 && clock==g_cpu.master_cycles && !memcmp(work,g_ram,0x20000),"actual survivor pixel capture uses the private compositor without changing guest RAM or time");
      const X3CoopPresentedFrame *v=X3CoopPresentationVisible();unsigned position[4]={support_position(survivor,5),support_position(survivor,8),v->views[survivor][0],v->views[survivor][1]};int cx=(int)position[0]-(int)position[2],cy=(int)position[1]-(int)position[3];if(cx<32)cx=32;if(cx>224)cx=224;if(cy<48)cy=48;if(cy>192)cy=192;
      uint8_t pixels[body_bytes+world_bytes];const uint8_t *screen=(const uint8_t *)g_ppu->renderBuffer;for(unsigned row=0;row<80;++row){memcpy(pixels+row*64*4,screen+(cy-48+row)*g_ppu->renderPitch+(cx-32)*4,64*4);memcpy(pixels+body_bytes+row*48*4,screen+row*g_ppu->renderPitch+(survivor==x?0:208)*4,48*4);}
      if(!death){memcpy(reference+sample*sizeof(pixels),pixels,sizeof(pixels));memcpy(coordinates[sample],position,sizeof(position));}
      else{check(!memcmp(coordinates[sample],position,sizeof(position)),"corpse and absence references retain the same actual survivor body and private camera");check(!memcmp(reference+sample*sizeof(pixels),pixels,sizeof(pixels)),"actual corpse cannot change the survivor body colors, HUD or remote background pixels at any sampled native death phase");}
      const char *output=getenv("X3_COOP_JUNGLE_VISUAL_OUTPUT");if(output && *output){char file[1024];snprintf(file,sizeof(file),"%s-x%u-victim%u-%s-%u.ppm",output,x,victim,death?"corpse":"absent",f);WritePpm(file,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);}
      X3Display_SetPresentationSeat(-1);++sample;
    }
    check(sample==samples,"survivor colors cover the actual initial, intermediate and completed native death phases");
  }
  check(RtlLoadSnapshotFromMemory(saved,now),"pixel comparison returns to the actual fatal-contact outcome for the remaining native checks");free(saved);free(reference);fprintf(stderr,"native jungle CORPSE VISUAL roster%u victim%u samples%u exact private body/background matches\n",x,victim,samples);
}
static void native_jungle_rider_lifecycle(unsigned x,const unsigned slot[2],uint8_t *base,uint8_t *work,size_t capacity){
  size_t cut=RtlSaveSnapshotToMemory(base,capacity);uint8_t *start=malloc(cut);check(start!=NULL,"actual jungle riders cut allocates");memcpy(start,base,cut);
  for(unsigned seat=0;seat<2;++seat){
    check(RtlLoadSnapshotFromMemory(start,cut),"each actual support withdrawal starts with both earned rider histories");unsigned own=seat==x?0:1,other=own^1;uint8_t source[64];memcpy(source,g_ram+slot[other],64);unsigned hp=contact_hp(seat^1);
    check(X3CoopRuntimeWithdraw(seat),"the actual jungle rider can withdraw through the public lifecycle API");size_t n=RtlSaveSnapshotToMemory(work,capacity),at=contact_menu_offset(work,n)-31;
    check(!(work[at+(slot[own]-0xd18)/64]&(1u<<seat)) && (work[at+(slot[other]-0xd18)/64]&3)==(1u<<(seat^1)) && !memcmp(source,g_ram+slot[other],64) && contact_hp(seat^1)==hp,"rider withdrawal clears only that rider while retaining the other real support, contact and health");
  }
  check(RtlLoadSnapshotFromMemory(start,cut),"real fatal support contact restores the two actual riders");
  X3CoopViewState views=*X3CoopViewsState();X3CoopViewsConfigure(false);CpuState caller=g_cpu;view_word(0,2368);view_word(2,608);view_word(4,320);g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);check(interp_bridge_run(&g_cpu,0x00ded3) && g_cpu.S==caller.S,"authored jungle3C:ED92 enemy08 allocates from its real column");X3CoopViewsSetState(&views);
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);check(interp_bridge_run(&g_cpu,0x00dcc0) && g_cpu.S==caller.S,"actual jungle hostile initializes through native DB06 constructors");uint64_t master=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=master;g_cpu.cycles=cycles;X3CoopViewsSetState(&views);
  unsigned foe=0;for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==8 && view_read(d+5)==2376)foe=d;check(foe && g_ram[foe+0x26] && g_ram[foe+0x26]<128,"the authored jungle foe has native nonzero contact damage");
  size_t admitted=RtlSaveSnapshotToMemory(base,capacity);uint8_t *live=malloc(admitted);check(live!=NULL,"actual hostile and rider cut allocates");memcpy(live,base,admitted);
  for(unsigned victim=0;victim<2;++victim){
    check(RtlLoadSnapshotFromMemory(live,admitted),"either fatal rider restores the same real hostile and native supports");X3CoopState low=*X3CoopRuntimeState();low.players[victim].body[0x27]=1;low.players[victim].body[0x88]=low.players[victim].body[0x8a]=0;
    memcpy(base,live,admitted);check(X3CoopEncode(&low,base+contact_context_offset(base,admitted),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,admitted),"fatal rider prerequisite supplies only low HP and expired protection");door_install_player(&low);view_word(foe+5,support_position(victim,5));view_word(foe+0x22,support_position(victim,5));view_word(foe+8,support_position(victim,8));view_word(foe+0x24,support_position(victim,8));size_t n=RtlSaveSnapshotToMemory(base,capacity);unsigned frames=0,own=victim==x?0:1,other=own^1,hp=contact_hp(victim^1);
    for(;frames<60 && X3CoopRuntimeState()->players[victim].life==X3_COOP_ALIVE;++frames)frame(0);
    size_t done=RtlSaveSnapshotToMemory(work,capacity),at=contact_menu_offset(work,done)-31;fprintf(stderr,"native jungle FATAL roster%u seat%u frames%u bits%02x/%02x life%u/%u sourceHP%u/%u\n",x,victim,frames,work[at+(slot[0]-0xd18)/64],work[at+(slot[1]-0xd18)/64],X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life,g_ram[slot[0]+0x27],g_ram[slot[1]+0x27]);
    check(frames<60 && X3CoopRuntimeState()->players[victim].life==X3_COOP_FALLEN && X3CoopRuntimeState()->players[victim^1].life==X3_COOP_ALIVE && !(work[at+(slot[own]-0xd18)/64]&(1u<<victim)) && (work[at+(slot[other]-0xd18)/64]&3)==(1u<<(victim^1)) && contact_hp(victim^1)==hp,"actual fatal contact clears only the dead support rider and preserves the living rider's earned contact");
    uint8_t *future=malloc(done);check(future!=NULL,"actual fatal rider future allocates");memcpy(future,work,done);check(RtlLoadSnapshotFromMemory(base,n),"actual pre-fatal rider and hostile contact restore");for(unsigned j=0;j<frames;++j)frame(0);same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"authored fatal contact and rider ownership replay the complete machine");free(future);
    if(getenv("X3_COOP_JUNGLE_CORPSE_VISUAL"))native_jungle_survivor_visual(base,n,victim,x,foe,work,capacity);
    unsigned before=support_position(victim^1,5),clock=g_ram[0x9cc];for(unsigned j=0;j<96;++j){frame(0);check((g_ppu->inidisp&0x8f)==15,"rider death leaves the surviving support view bright throughout cleanup");}
    check((uint8_t)(g_ram[0x9cc]-clock)==96 && support_position(victim^1,5)!=before && g_ram[slot[other]] && (g_ram[slot[other]+0x2c]&1) && contact_hp(victim^1)==hp,"the surviving native support actually carries its living rider after the other rider's death");
  }
  free(live);check(RtlLoadSnapshotFromMemory(start,cut),"actual support destruction restores both living riders after lifecycle branches");free(start);
}
static void native_destructible_support_checks(const char *fixture,uint8_t *base,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==8,"destructible support restores the current authored jungle stage");bool paired=getenv("X3_COOP_JUNGLE_ORIGINAL")==NULL,visual=getenv("X3_COOP_JUNGLE_CORPSE_VISUAL")!=NULL;unsigned second=visual?880:760;size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();
  if(getenv("X3_COOP_JUNGLE_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  unsigned x=setup.players[0].character==X3_COOP_X?0:1;setup.current=setup.anchor=(uint8_t)x;
  for(unsigned seat=0;seat<2;++seat){unsigned px=seat==x?640:second,py=seat==x?800:760;uint8_t *b=setup.players[seat].body;b[5]=b[0x22]=(uint8_t)px;b[6]=b[0x23]=(uint8_t)(px>>8);b[8]=b[0x24]=(uint8_t)py;b[9]=b[0x25]=(uint8_t)(py>>8);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x88]=255;b[0x8a]=0;}
  check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"actual jungle support approach supplies only living body positions and protection");door_install_player(&setup);X3CoopRuntimeEnable(paired);
  X3CoopViewsConfigure(paired);X3CoopViewState views=*X3CoopViewsState();views.independent=(uint8_t)paired;views.initialized=1;views.stage=8;views.valid=views.camera_valid=3;
  for(unsigned seat=0;seat<2;++seat){views.camera[seat][0]=(uint16_t)(seat==x?512:second-128);views.camera[seat][1]=640;views.limits[seat][0]=views.current[seat][0]=0;views.limits[seat][1]=views.current[seat][1]=8192;views.limits[seat][2]=views.current[seat][2]=512;views.limits[seat][3]=views.current[seat][3]=768;}X3CoopViewsSetState(&views);
  view_word(0x1e5d,views.camera[x][0]);view_word(0x1e7a,views.camera[x][0]);view_word(0x1e60,640);view_word(0x1e7c,640);view_word(0x1e66,0);view_word(0x1e68,8192);view_word(0x1e6a,512);view_word(0x1e6c,768);view_word(0x1e6e,0);view_word(0x1e70,8192);view_word(0x1e78,512);view_word(0x1e7e,768);
  CpuState caller=g_cpu;for(unsigned col=640;col<=880;col+=240){view_word(0,col&~31u);view_word(2,608);view_word(4,320);g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);check(interp_bridge_run(&g_cpu,0x00ded3) && g_cpu.S==caller.S,"native jungle support column allocates with its actual DB06");}
  g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);check(interp_bridge_run(&g_cpu,0x00dcc0) && g_cpu.S==caller.S,"native constructors initialize both authored solid enemies");uint64_t master=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=master;g_cpu.cycles=cycles;
  unsigned slot[2]={0,0};for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x1f){if(view_read(d+5)==640)slot[0]=d;if(view_read(d+5)==880)slot[1]=d;}
  check(slot[0] && slot[1] && slot[0]!=slot[1],"actual two jungle records initialize distinct destructible supports");
  /* Keep the shot target inside X's original projectile retirement range. */
  view_word(slot[1]+5,second);view_word(slot[1]+0x22,second);
  for(unsigned f=0;f<30;++f)frame(0);
  n=RtlSaveSnapshotToMemory(base,capacity);size_t riders_at=contact_menu_offset(base,n)-31;
  fprintf(stderr,"native jungle RIDERS X%u,%u Zero%u,%u bits%02x/%02x sourceY%u/%u HP%u/%u\n",support_position(x,5),support_position(x,8),support_position(x^1,5),support_position(x^1,8),base[riders_at+(slot[0]-0xd18)/64],base[riders_at+(slot[1]-0xd18)/64],view_read(slot[0]+8),view_read(slot[1]+8),g_ram[slot[0]+0x27],g_ram[slot[1]+0x27]);
  if(paired)check((base[riders_at+(slot[0]-0xd18)/64]&3)==(1u<<x) && (base[riders_at+(slot[1]-0xd18)/64]&3)==(1u<<(x^1)),"actual falling/contact establishes only the real rider on each jungle support");
  if(paired && getenv("X3_COOP_JUNGLE_LIFECYCLE"))native_jungle_rider_lifecycle(x,slot,base,work,capacity);
  if(visual)return; /* Original distant placements isolate corpse presentation. */
  g_ram[slot[1]+0x27]=1;n=RtlSaveSnapshotToMemory(base,capacity);unsigned initial=g_ram[slot[1]+0x27];
  for(unsigned f=0;f<60;++f)frame(0);check(g_ram[slot[1]] && (g_ram[slot[1]+0x27]&127)==initial,"no-fire control retains the low-health authored support");
  check(RtlLoadSnapshotFromMemory(base,n),"actual armed support approach restores before native Fire");X3CoopRuntimeEnable(paired);unsigned killed=0;uint8_t health[120];
  for(unsigned f=0;f<120;++f){frame((uint32_t)((f%12<4?SNES_PAD_Y:0)|(f==0?SNES_PAD_RIGHT:0))<<(paired?x*12:0));health[f]=g_ram[slot[1]]?g_ram[slot[1]+0x27]&127:0;if(!health[f] && !killed)killed=f+1;}
  fprintf(stderr,"native jungle WEAPON paired%u roster%u firstLethal%u trace",paired,x,killed);for(unsigned f=0;f<120;++f)fprintf(stderr,"%02x",health[f]);fprintf(stderr,"\n");

  size_t done=RtlSaveSnapshotToMemory(work,capacity);riders_at=contact_menu_offset(work,done)-31;fprintf(stderr,"native jungle DESTROYED killed%u active%u histories%02x/%02x life%u/%u\n",killed,g_ram[slot[1]],work[riders_at+(slot[0]-0xd18)/64],work[riders_at+(slot[1]-0xd18)/64],X3CoopRuntimeState()->players[0].life,X3CoopRuntimeState()->players[1].life);
  check(killed && !g_ram[slot[1]],"real native Fire destroys the actual low-health solid jungle enemy");
  if(paired)check(!work[riders_at+(slot[1]-0xd18)/64] && (work[riders_at+(slot[0]-0xd18)/64]&3)==(1u<<x) && g_ram[slot[0]] && g_ram[slot[0]+0x27]==18 && (g_ram[slot[0]+0x2c]&1) && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual support destruction releases only its obsolete rider history and retains the separate living support and rider");
  uint8_t *future=malloc(done);check(future!=NULL,"actual support destruction future allocates");memcpy(future,work,done);check(RtlLoadSnapshotFromMemory(base,n),"actual support restores before the real lethal shot");X3CoopRuntimeEnable(paired);
  for(unsigned f=0;f<120;++f)frame((uint32_t)((f%12<4?SNES_PAD_Y:0)|(f==0?SNES_PAD_RIGHT:0))<<(paired?x*12:0));same(future,done,work,RtlSaveSnapshotToMemory(work,capacity),"actual support destruction and retained unrelated rider replay the complete machine");free(future);


}

/* Authored factory record3C:F038, enemy31. This is a rising lift, not a
 * factory lift. Its free rider crosses native camera rows after96 fields. */
typedef struct FactoryLiftSample {uint8_t support[64],body[X3_COOP_BODY_BYTES];} FactoryLiftSample;
static FactoryLiftSample factory_lift_sample(unsigned slot,unsigned seat,bool paired){
  FactoryLiftSample out;memcpy(out.support,g_ram+slot,sizeof(out.support));
  const X3CoopState *s=X3CoopRuntimeState();const uint8_t *body=!paired || s->current==seat?g_ram+X3_COOP_BODY:s->players[seat].body;
  memcpy(out.body,body,sizeof(out.body));out.body[0x17]&=127;return out;
}
static void native_factory_lift_view_checks(const char *fixture,uint8_t *base,uint8_t *work,uint8_t *expected,size_t capacity){
  enum{fields=720};
  for(unsigned roster=0;roster<2;++roster)for(unsigned rider=0;rider<2;++rider){
    check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==9 && (g_ram[0x1fc0]&64),"factory lift restores an actual factory and its authored campaign prerequisite");
    X3CoopRuntimeFrame();check(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive(),"factory lift starts in a registered free native co-op world");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    if(roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
    setup.current=setup.anchor=(uint8_t)rider;
    for(unsigned seat=0;seat<2;++seat){check(setup.players[seat].life==X3_COOP_ALIVE,"factory lift starts with a living free pair");setup.players[seat].body[0x88]=255;}
    /* The route is bypassed. Read its three original ROM region records
     * before positioning the initial view. No lift/rider state or future
     * camera position is supplied. The continuation is compared to native. */
    const unsigned approach[3][2]={{928,1480},{928,1696},{1152,1616}};
    for(unsigned point=0;point<3;++point){
      team_hazard_body(setup.players[rider].body,approach[point][0],approach[point][1]);
      check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"factory lift approach positions the actor in an authored region record");door_install_player(&setup);X3CoopViewsUpdate(g_ram,&setup,false,false);
      n=RtlSaveSnapshotToMemory(base,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    }
    team_hazard_body(setup.players[rider].body,1152,1616);
    check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"factory lift changes only roster, personal protection and an airborne approach");door_install_player(&setup);X3CoopViewsConfigure(true);
    X3CoopViewState views=*X3CoopViewsState();views.valid=views.camera_valid=3;
    X3CoopViewsUpdate(g_ram,&setup,false,false);views=*X3CoopViewsState();
    fprintf(stderr,"native LIFT INITIAL REGION targets%u/%u/%u/%u\n",views.limits[rider][0],views.limits[rider][1],views.limits[rider][2],views.limits[rider][3]);
    check(views.limits[rider][2]==768 && views.limits[rider][3]==1536,"the original bypassed-route region records define the lift camera range");
    views.camera[rider][0]=1024;views.camera[rider][1]=1456;
    memcpy(views.current[rider],views.limits[rider],sizeof(views.current[rider]));
    X3CoopViewsSetState(&views);vehicle_install_view(&views,rider);
    CpuState caller=g_cpu;view_word(0,1088);view_word(2,1504);view_word(4,320);g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P&=(uint8_t)~0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
    check(interp_bridge_run(&g_cpu,0x00ded3) && g_cpu.S==caller.S,"native factory event column allocates the actual factory lift");
    g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0;g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
    check(interp_bridge_run(&g_cpu,0x00dcc0) && g_cpu.S==caller.S,"original constructors initialize the authored platform and its actual child");
    uint64_t master=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=master;g_cpu.cycles=cycles;
    unsigned slot=0;for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && g_ram[d+10]==0x31 && !g_ram[d+11])slot=d;
    fprintf(stderr,"native FACTORY LIFT SETUP roster%u rider%u slot%04x XY%u,%u state%u/%u latch%02x\n",roster,rider,slot,view_read(slot+5),view_read(slot+8),g_ram[slot+1],g_ram[slot+2],g_ram[slot+0x2c]);
    check(slot && g_ram[slot+1]==2 && !g_ram[slot+2] && view_read(slot+5)==1152 && view_read(slot+8)==1696,"native class31 is waiting for a rider at its original position before the landing");
    /* Let the original OAM upload publish the newly admitted support before
     * this camera-handoff cut. Initial admission itself is not the criterion. */
    for(unsigned f=0;f<3;++f)frame(0);
    setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);views=*X3CoopViewsState();
    bool admitted=false;const X3CoopPresentedFrame *first_draw=X3CoopPresentationVisible();
    for(unsigned i=0;i<first_draw->count;++i)if(first_draw->actors[i].address==slot)admitted=true;
    check(admitted && !g_ram[slot+2],"the native upload shows the waiting support while the rider is still airborne");
    n=RtlSaveSnapshotToMemory(base,capacity);FactoryLiftSample native[fields];unsigned trigger=0,rise=0,transitions=0,previous=0;uint16_t native_camera[fields];unsigned native_updates=0;
    X3CoopRuntimeEnable(false);X3CoopViewsConfigure(false);door_install_player(&setup);vehicle_install_view(&views,rider);
    for(unsigned f=0;f<fields;++f){unsigned tick=g_ram[0x9cc];frame(0);unsigned delta=(uint8_t)(g_ram[0x9cc]-tick);check(delta==1,"the original free lift world advances once on each observed field");native_updates+=delta;native[f]=factory_lift_sample(slot,rider,false);native_camera[f]=(uint16_t)view_read(0x1e60);
      if(!trigger && g_ram[slot+2]==2)trigger=f+1;
      if(g_ram[slot+2]==4 && previous!=4){++transitions;if(!rise)rise=f+1;}previous=g_ram[slot+2];
    }
    fprintf(stderr,"native FACTORY LIFT ORIGINAL roster%u rider%u char%u trigger%u rise%u transitions%u Y%u->%u HP%u action%u\n",roster,rider,setup.players[rider].character,trigger,rise,transitions,1696,view_read(slot+8),g_ram[X3_COOP_BODY+0x27],g_ram[X3_COOP_BODY+2]);
    check(trigger && rise==trigger+96 && transitions==1 && view_read(slot+8)<1000 && g_ram[X3_COOP_BODY+0x27],"actual original landing triggers exactly one96-field delayed rise and retains its free native rider");
    FactoryLiftSample native_control[8];unsigned initial_x=view_read(X3_COOP_BODY+5),initial_y=view_read(X3_COOP_BODY+8);bool native_moved=false;
    for(unsigned f=0;f<8;++f){frame(SNES_PAD_RIGHT|SNES_PAD_B);native_control[f]=factory_lift_sample(slot,rider,false);native_moved|=view_read(X3_COOP_BODY+5)!=initial_x || view_read(X3_COOP_BODY+8)!=initial_y;}
    check(native_moved && g_ram[X3_COOP_BODY+2]<0x1c,"the original rider resumes actual lateral or jump movement at the native lift endpoint");
    size_t future=0;unsigned free_x=setup.players[rider^1].body[5]|setup.players[rider^1].body[6]<<8;
    for(unsigned run=0;run<2;++run){check(RtlLoadSnapshotFromMemory(base,n),"paired factory lift restores the same native pre-landing cut");
      X3CoopState paired=*X3CoopRuntimeState();paired.current=paired.anchor=(uint8_t)(rider^1);
      memcpy(work,base,n);check(X3CoopEncode(&paired,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"the free partner retains the canonical world while the real lift rider owns the distant camera");door_install_player(&paired);vehicle_install_view(&views,rider^1);
      unsigned clock=g_ram[0x9cc],visible_fields=0,updates=0;int first_camera=-1,last_camera=-1;
      X3Display_SetPresentationSeat((int)rider);
      for(unsigned f=0;f<fields;++f){unsigned tick=g_ram[0x9cc];frame(f<12?(uint32_t)SNES_PAD_RIGHT<<((rider^1)*12):0);unsigned delta=(uint8_t)(g_ram[0x9cc]-tick);check(delta==1,"the paired lift world advances once on each observed field");updates+=delta;FactoryLiftSample now=factory_lift_sample(slot,rider,true);
        if(memcmp(&native[f],&now,sizeof(now))){unsigned at=0;while(at<sizeof(now) && ((uint8_t *)&native[f])[at]==((uint8_t *)&now)[at])++at;fprintf(stderr,"native FACTORY LIFT MISMATCH roster%u rider%u f%u byte%u original%02x paired%02x\n",roster,rider,f,at,((uint8_t *)&native[f])[at],((uint8_t *)&now)[at]);check(RtlSaveSnapshot("native-factory-lift-mismatch.sav"),"actual factory-lift mismatch preserves its native machine");check(false,"the shared factory lift and actual rider match their original complete records on each native field");}
        const X3CoopViewState *v=X3CoopViewsState();if(!f)first_camera=v->camera[rider][1];last_camera=v->camera[rider][1];
        if(v->camera[rider][1]!=native_camera[f]){fprintf(stderr,"native LIFT CAMERA MISMATCH roster%u rider%u f%u original%u paired%u limits%u/%u\n",roster,rider,f,native_camera[f],v->camera[rider][1],v->current[rider][2],v->current[rider][3]);check(false,"the actual noncanonical rider camera follows the same native ascent on every field");}
        const X3CoopPresentedFrame *draw=X3CoopPresentationVisible();bool drawn=false;for(unsigned i=0;i<draw->count;++i)if(draw->actors[i].address==slot)drawn=true;
        if(drawn && X3Display_RenderedPrivateFrame())++visible_fields;
        const char *image=getenv("X3_COOP_FACTORY_LIFT_IMAGE");
        if(image && *image && !roster && !rider && !run && (f==4 || f==150 || f==350 || f==650)){
          char path[1024];snprintf(path,sizeof(path),"%s-f%u.ppm",image,f);WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256+g_ppu->extraLeftRight*2,224,0,false);
        }
      }
      fprintf(stderr,"native LIFT CAMERA roster%u rider%u run%u camera%d->%d privateVisible%u/%u sourceY%u freeX%u\n",roster,rider,run,first_camera,last_camera,visible_fields,fields,view_read(slot+8),support_position(rider^1,5));
      check(first_camera-last_camera>=512 && visible_fields==fields,"the real moving lift remains in its rider's private render across multiple native camera rows");
      X3Display_SetPresentationSeat(-1);
      check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && support_position(rider^1,5)>free_x && updates==native_updates && updates==fields && (uint8_t)(g_ram[0x9cc]-clock)==(uint8_t)fields,"the free partner moves independently while one shared rise advances once per native world field");
      for(unsigned f=0;f<8;++f){frame((uint32_t)(SNES_PAD_RIGHT|SNES_PAD_B)<<(rider*12));FactoryLiftSample now=factory_lift_sample(slot,rider,true);same(&native_control[f],sizeof(now),&now,sizeof(now),"the actual noncanonical rider resumes control with the same complete native body and lift update");}
      size_t done=RtlSaveSnapshotToMemory(work,capacity);if(!run){future=done;memcpy(expected,work,done);}else same(expected,future,work,done,"actual landing, countdown, ascent, independent cameras and complete machine replay exactly");
    }
  }
}
