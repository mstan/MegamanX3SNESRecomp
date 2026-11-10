/* Native event admission and camera contracts. Teleports prepare positions;
 * the retail allocator, culling predicates and camera arithmetic decide results. */
#include "../src/x3_coop_view.h"
typedef struct ViewCodecProbe {SaveLoadInfo io;uint8_t bytes[131072];size_t used;bool load;} ViewCodecProbe;
static ViewCodecProbe view_probe;
static unsigned view_probe_action;
static unsigned view_read(unsigned address);
static void view_probe_io(SaveLoadInfo *io,void *data,size_t size) {
  ViewCodecProbe *p=(ViewCodecProbe *)io;
  if(size>sizeof(p->bytes)-p->used)check(0,"interrupted spawn codec overflow");
  if(p->load)memcpy(data,p->bytes+p->used,size);else memcpy(p->bytes+p->used,data,size);
  p->used+=size;
}
static void independent_route_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity) {
  const X3CoopViewState *v=X3CoopViewsState();
  check(X3CoopViewsIndependent() && v->camera_valid==3 && v->camera[1][0]>v->camera[0][0]+256,
        "normal P2 approach advances its own camera beyond the stationary anchor view");
  bool distant_enemy=false;
  for(unsigned d=0xd18;d<0x10d8;d+=64)
    if(g_ram[d] && view_read(d+5)>view_read(0x1e5d)+288)distant_enemy=true;
  check(distant_enemy,"normal P2-only route retains an authored enemy outside the anchor's retirement rectangle");
  unsigned tick=g_ram[0x9cc],anchor_x=view_read(0x1e5d);
  for(unsigned i=0;i<30;++i)frame(0);
  check((uint8_t)(g_ram[0x9cc]-tick)==30,"two independent views advance the native world once per frame");
  check(view_read(0x1e5d)==anchor_x,"P2's separate region does not drag the stationary anchor camera");
  size_t expected_size=RtlSaveSnapshotToMemory(expected,capacity);
  check(RtlLoadSnapshotFromMemory(original,size),"independent route snapshot restores");
  for(unsigned i=0;i<30;++i)frame(0);
  size_t actual_size=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,expected_size,work,actual_size,"independent camera, visit history and shared world deterministic replay");
}
static void view_probe_boundary(CpuState *cpu,uint32_t pc) {
  (void)cpu;(void)pc;
  if(!view_probe_action)return;
  unsigned action=view_probe_action;view_probe_action=0;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=action==2;
  if(action==1) {
    X3CoopRuntimeSave(&view_probe.io);
    check(X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),
          "real interrupted native allocation passes state preflight");
  } else check(X3CoopRuntimeLoad(&view_probe.io),"pending union columns and visit history restore inside native allocation");
}
static void view_word(unsigned address,unsigned value) {
  g_ram[address]=(uint8_t)value;g_ram[address+1]=(uint8_t)(value>>8);
}
static unsigned view_read(unsigned address) {return g_ram[address]|g_ram[address+1]<<8;}
static void view_native_call(unsigned pc,unsigned d,bool long_call) {
  uint16_t stack=g_cpu.S;
  g_cpu.PB=(uint8_t)(pc>>16);g_cpu.DB=0;g_cpu.D=(uint16_t)d;
  g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);
  if(long_call)cpu_push_jsl_return_frame(&g_cpu);else cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc),"native independent-world boundary returns");
  check(g_cpu.S==stack,"independent-world boundary preserves the native caller stack");
}
static bool view_has_enemy(unsigned x,unsigned y) {
  for(unsigned d=0xd18;d<0x10d8;d+=64)
    if(g_ram[d] && view_read(d+5)==x && view_read(d+8)==y)return true;
  return false;
}
static void native_view_checks(uint8_t *original,size_t original_size,uint8_t *work,uint8_t *expected,size_t capacity) {
  size_t offset=contact_context_offset(original,original_size);X3CoopState setup;
  check(X3CoopDecode(&setup,original+offset,X3CoopEncodedSize()),"independent-view setup decodes");
  setup.players[1].body[5]=0x30;setup.players[1].body[6]=7; /*1840*/
  setup.players[1].body[8]=0x70;setup.players[1].body[9]=4; /*1136*/
  memcpy(work,original,original_size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()),"separated P2 precondition encodes");
  check(RtlLoadSnapshotFromMemory(work,original_size),"separated P2 precondition restores");
  X3CoopViewsConfigure(true);X3CoopRuntimeFrame();
  view_word(X3_COOP_BODY+5,736);view_word(X3_COOP_BODY+8,1136);
  view_word(0x1e5d,608);view_word(0x1e60,1024);
  X3CoopViewState views={0};views.independent=1;views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  for(unsigned seat=0;seat<2;++seat) {
    views.limits[seat][1]=views.current[seat][1]=8192;
    views.limits[seat][3]=views.current[seat][3]=8192;
    views.camera[seat][0]=(uint16_t)(seat?1728:608);views.camera[seat][1]=1024;
  }
  X3CoopViewsSetState(&views);
  memset(g_ram+0xd18,0,0x3c0);
  for(unsigned col=0;col<256;++col) {
    unsigned first=view_read(0xf800+col*2),end=view_read(0xf802+col*2);
    if(first<0xfa02 || end>0xfffc || end<first || (end-first)%5)continue;
    for(unsigned flag=first;flag<end;flag+=5)g_ram[flag]=0;
  }
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  X3CoopViewState edge=views;edge.camera[0][0]=256;edge.camera[1][0]=473;
  X3CoopViewsSetState(&edge);view_word(0x1e5d,256);
  view_native_call(0x00de3a,0,false);
  check(!view_has_enemy(736,1136) && !X3CoopViewsSeen(0xfa16),
        "outer retention margin cannot consume an event before its native frontier admits it");
  check(RtlLoadSnapshotFromMemory(work,prepared),"frontier boundary probe restores");
  check(interp_bridge_add_pre_opcode_hook(0x00df4c,view_probe_boundary),"interrupted allocation probe installs");
  view_probe_action=1;
  view_native_call(0x00de3a,0,false);
  check(!view_probe_action,"allocation probe observed a real native visit write");
  check(view_has_enemy(736,1136) && view_has_enemy(1840,1056),
        "retail event allocator admits actual stage enemies near both independent cameras");
  check(!view_has_enemy(1104,1198) && !view_has_enemy(1232,1184),
        "empty gap between independent views does not admit its authored enemies");
  uint8_t enemies[0x3c0];memcpy(enemies,g_ram+0xd18,sizeof(enemies));
  view_native_call(0x00de3a,0,false);
  check(!memcmp(enemies,g_ram+0xd18,sizeof(enemies)),"stationary independent views do not allocate duplicate enemies");
  unsigned partner_slot=0;
  for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d] && view_read(d+5)==1840){partner_slot=d;break;}
  check(partner_slot!=0,"P2-only authored enemy has a real native slot");
  view_native_call(0x02d58a,partner_slot,true);
  cpu_mirrors_to_p(&g_cpu);check(!(g_cpu.P&1),"native culling retains an enemy visible only to P2");
  view_word(partner_slot+5,1104);view_word(partner_slot+8,1198);
  view_native_call(0x02d58a,partner_slot,true);
  cpu_mirrors_to_p(&g_cpu);check(g_cpu.P&1,"native culling rejects an enemy in the gap between views");
  check(RtlLoadSnapshotFromMemory(work,prepared),"independent activation precondition restores");
  view_probe_action=2;
  view_native_call(0x00de3a,0,false);
  check(!view_probe_action,"restored allocation reaches the same native continuation");
  check(!memcmp(enemies,g_ram+0xd18,sizeof(enemies)),"restored union activation reproduces native allocation order");

  /* Real region lists include a stage alias and non-monotonic record pointers. */
  X3CoopState local=*X3CoopRuntimeState();uint8_t room_ram[0x20000];memcpy(room_ram,g_ram,sizeof(room_ram));
  local.current=local.anchor=0;local.players[0].body[0x27]=local.players[1].body[0x27]=16;
  const unsigned stages[]={0,1,5,11,13,14};
  for(unsigned k=0;k<sizeof(stages)/sizeof(*stages);++k) {
    room_ram[0x1fae]=(uint8_t)stages[k];room_ram[0x1f63]=0;
    unsigned first=(stages[k]==0?0xe5ba:stages[k]==1?0xe1ef:stages[k]==5?0xe976:stages[k]==13?0xf152:0xefab);
    const uint8_t *rec=g_snes->cart->rom+0x18000+(first&32767);
    unsigned x=(rec[2]|rec[3]<<8)+1,y=(rec[6]|rec[7]<<8)+1;
    room_ram[X3_COOP_BODY+5]=(uint8_t)x;room_ram[X3_COOP_BODY+6]=(uint8_t)(x>>8);
    room_ram[X3_COOP_BODY+8]=(uint8_t)y;room_ram[X3_COOP_BODY+9]=(uint8_t)(y>>8);
    room_ram[X3_COOP_BODY+0x27]=16;
    X3CoopViewsReset();X3CoopViewsUpdate(room_ram,&local,false,false);
    const X3CoopViewState *v=X3CoopViewsState();
    check(v->stage==stages[k] && (v->valid&1),"native region parser admits its actual stage list");
    if(stages[k]==1)check(v->limits[0][0]==256 && v->limits[0][1]==1024 && v->limits[0][2]==1024 && v->limits[0][3]==1024,
                           "stage1 room applies its four original camera-limit commands");
    if(stages[k]==13)check(v->limits[0][0]==1536 && v->limits[0][2]==3328,"stage13 list survives the later stage11 alias");
    if(stages[k]==14)check(v->limits[0][0]==768 && v->limits[0][2]==1024,"stage14 uses the original stage11 region alias");
  }
  uint8_t encoded[X3_COOP_VIEW_STATE_BYTES];X3CoopViewsSave(encoded);encoded[0]=2;
  check(!X3CoopViewsLoad(encoded,sizeof(encoded),true),"invalid independent simulation profile is refused");
  check(RtlLoadSnapshotFromMemory(original,original_size),"native camera comparison starts from the original scene");
  X3CoopViewsConfigure(true);X3CoopRuntimeFrame();
  setup=*X3CoopRuntimeState();
  unsigned px=view_read(X3_COOP_BODY+5)+128;
  setup.players[1].body[5]=(uint8_t)px;setup.players[1].body[6]=(uint8_t)(px>>8);
  setup.players[1].body[0x22]=(uint8_t)(px-4);setup.players[1].body[0x23]=(uint8_t)((px-4)>>8);
  X3CoopViewsUpdate(g_ram,&setup,false,false);
  views=*X3CoopViewsState();views.camera_valid=3;
  for(unsigned seat=0;seat<2;++seat) {
    for(unsigned i=0;i<4;++i)views.current[seat][i]=(uint16_t)view_read(0x1e66+i*2);
    views.camera[seat][0]=(uint16_t)view_read(0x1e5d);views.camera[seat][1]=(uint16_t)view_read(0x1e60);
  }
  X3CoopViewsSetState(&views);prepared=RtlSaveSnapshotToMemory(work,capacity);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()),"camera comparison P2 position encodes");
  check(RtlLoadSnapshotFromMemory(work,prepared),"matched native camera setup restores");
  X3CoopRuntimeEnable(false);memcpy(g_ram+X3_COOP_BODY,setup.players[1].body,X3_COOP_BODY_BYTES);
  view_native_call(0x00dfc6,0,false);
  unsigned reference[6]={view_read(0x1e5d),view_read(0x1e60)};
  for(unsigned i=0;i<4;++i)reference[i+2]=view_read(0x1e66+i*2);
  X3CoopRuntimeEnable(true);check(RtlLoadSnapshotFromMemory(work,prepared),"paired camera comparison setup restores");
  view_native_call(0x00dfc6,0,false);
  X3CoopViewsEndFrame(g_ram,X3CoopRuntimeState(),false,false);
  const X3CoopViewState *result=X3CoopViewsState();
  check(result->camera[1][0]==reference[0] && result->camera[1][1]==reference[1],
        "P2 camera follows the original native camera coordinates");
  for(unsigned i=0;i<4;++i)check(result->current[1][i]==reference[i+2],"P2 native camera-limit pan matches the single-seat reference");
  check(RtlLoadSnapshotFromMemory(work,prepared),"P2 pit precondition restores");
  setup=*X3CoopRuntimeState();unsigned pit=view_read(0x1e6c)+300;
  setup.players[1].body[8]=(uint8_t)pit;setup.players[1].body[9]=(uint8_t)(pit>>8);
  setup.players[1].body[0x30]=0;setup.players[1].body[0x27]=16;
  size_t pit_size=RtlSaveSnapshotToMemory(expected,capacity);
  check(X3CoopEncode(&setup,expected+offset,X3CoopEncodedSize()),"below-floor partner position encodes");
  check(RtlLoadSnapshotFromMemory(expected,pit_size),"below-floor partner position restores");
  views=*X3CoopViewsState();views.camera[1][1]=views.current[1][3];X3CoopViewsSetState(&views);
  view_native_call(0x00dfc6,0,false);
  check(X3CoopRuntimeState()->players[1].life==X3_COOP_FALLEN &&
        X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && !X3CoopRuntimeState()->wipe,
        "P2's own native camera pit damage leaves the living partner in control");
  check(g_ram[0xd1]==4 && g_ram[0xd2]==4,"P2 camera pit does not select the team fade/retry phase");
  check(RtlLoadSnapshotFromMemory(work,prepared),"anchor pit precondition restores");
  setup=*X3CoopRuntimeState();px=view_read(0x1e5d)+384;
  setup.players[1].body[5]=(uint8_t)px;setup.players[1].body[6]=(uint8_t)(px>>8);
  pit_size=RtlSaveSnapshotToMemory(expected,capacity);
  check(X3CoopEncode(&setup,expected+offset,X3CoopEncodedSize()),"separated survivor position encodes");
  check(RtlLoadSnapshotFromMemory(expected,pit_size),"separated survivor position restores");
  views=*X3CoopViewsState();views.camera[1][0]=(uint16_t)(px-128);X3CoopViewsSetState(&views);
  view_word(X3_COOP_BODY+8,pit);view_word(0x1e60,view_read(0x1e6c));g_ram[X3_COOP_BODY+0x30]=0;
  view_native_call(0x00dfc6,0,false);
  check(X3CoopRuntimeState()->players[0].life==X3_COOP_FALLEN &&
        X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE && X3CoopRuntimeState()->anchor==1,
        "native anchor pit hands the world to the separated living actor");
  check(view_read(0x1e5d)==px-128,"anchor pit adopts the survivor's own camera instead of the corpse's view");
  check(RtlLoadSnapshotFromMemory(original,original_size),"independent-view probes restore their starting machine");
}
