#include "../src/x3_coop_menu.h"
static uint32_t menu_pad(unsigned seat,unsigned action){
  for(unsigned bit=0;bit<12;++bit)if(X3CoopActions((uint16_t)(1u<<bit),g_ram+0xffc0)&action)
    return 1u<<(bit+seat*12);
  check(0,"native menu action has a physical pad mapping");return 0;
}
static void menu_press(unsigned seat,unsigned action){frame(menu_pad(seat,action));frame(0);frame(0);}
static X3CoopMenuInfo menu_read(void){X3CoopMenuInfo v;check(X3CoopMenuRead(&v),"paired menu remains active");return v;}
static void menu_pixels(uint8_t *out){
  for(unsigned y=0;y<224;++y)memcpy(out+(size_t)y*256*4,g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch,256*4);
}
static void menu_open(unsigned seat){
  menu_press(seat,0x1000);for(unsigned i=0;i<90;++i)frame(0);
  X3CoopMenuInfo v=menu_read();check(v.owner==seat && (g_ram[0x1f37]&1),"actual requesting player's Start owns the one native pause");
}
static void menu_native_subtank(unsigned pc,unsigned tank){
  uint16_t stack=g_cpu.S;g_cpu.PB=0;g_cpu.DB=6;g_cpu.D=0x1ed8;g_cpu.X=(uint16_t)tank;
  g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc) && g_cpu.S==stack,"original subtank operation returns with its real stack");
}
static size_t menu_chunk(const uint8_t *data,size_t size){return contact_menu_offset(data,size);}
static bool menu_probe_armed,menu_probe_seen;
static bool menu_exit_world_seen;
static void menu_exit_world_probe(CpuState *cpu,uint32_t pc){
  if((pc&0x7fffff)==0x00cb76 && cpu->D==0 && (g_ram[0x1f37]&0x81)==0x81 &&
      X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor)menu_exit_world_seen=true;
}
static unsigned menu_compactions;
static void menu_compaction_probe(CpuState *cpu,uint32_t pc){
  (void)cpu;if((pc&0x7fffff)==0x00ceaf)++menu_compactions;
}
static void menu_heal_continuation_probe(CpuState *cpu,uint32_t pc){
  if(!menu_probe_armed || cpu->D!=0x1ed8 || (pc&0x7fffff)!=0x00c909)return;
  menu_probe_armed=false;menu_probe_seen=true;
  view_probe.io.func=view_probe_io;view_probe.used=0;view_probe.load=false;
  X3CoopRuntimeSave(&view_probe.io);size_t marker=menu_chunk(view_probe.bytes,view_probe.used);
  check(view_probe.bytes[marker+23]==4 && X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),
        "interrupted native healing captures a valid borrowed actor and real return continuation");
  uint8_t p=view_probe.bytes[marker+57];view_probe.bytes[marker+57]&=(uint8_t)~0x30;
  check(!X3CoopRuntimePreflight(view_probe.bytes,view_probe.used),"native healing refuses the wrong saved CPU operand widths");
  view_probe.bytes[marker+57]=p;view_probe.used=0;view_probe.load=true;
  check(X3CoopRuntimeLoad(&view_probe.io),"native healing continuation restores in place before the original pulse");
}
static void menu_map_wrap_probe(CpuState *cpu,uint32_t pc){
  (void)cpu;(void)pc;ppu_write(g_ppu,0x08,(uint8_t)(g_ppu->bgXsc[1]|0x80));
}
static void native_paired_menu_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *paused=malloc(capacity),*prepared=malloc(capacity),*image=malloc(512u*224u*4u),*native_reference=malloc(512u*224u*4u);
  check(paused && prepared && image && native_reference,"paired menu reference buffers allocate");
  if(getenv("X3_COOP_MENU_ADDRESS_WRAP"))
    check(interp_bridge_add_pre_opcode_hook(0x00cc58,menu_map_wrap_probe),"native menu tests the PPU tilemap address's ignored high bit");
  for(unsigned roster=0;roster<2;++roster)for(unsigned owner=0;owner<2;++owner){
    if(roster){const char *path=getenv("X3_COOP_PAIRED_MENU_REVERSED_STATE");
      check(path && *path && RtlLoadSnapshot(path),"paired menu reversed roster uses its real native construction");}
    else check(RtlLoadSnapshotFromMemory(original,size),"paired menu starts from native gameplay");
    X3CoopState setup=*X3CoopRuntimeState();unsigned x=setup.players[0].character==X3_COOP_X?0:1,z=x^1;
    unsigned winner=owner^1,partner=owner;
    for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x27]=1;
    unsigned initial=getenv("X3_COOP_PAIRED_MENU_WEAPON")?2:0,chosen=initial?0:2;
    setup.players[x].body[0x33]=(uint8_t)initial;
    size_t n=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,n),"personal low-health menu prerequisite restores");
    door_install_player(&setup);g_ram[0x1fbc]=0x5c;
    g_ram[0x1fb7]=0x8e;g_ram[0x1fb8]=0x83;g_ram[0x1fb9]=g_ram[0x1fba]=0x80;
    if(getenv("X3_COOP_PAIRED_MENU_GOLD")){g_ram[0x1fd1]|=15;g_ram[0x1fd7]|=0xf0;}
    size_t base=RtlSaveSnapshotToMemory(prepared,capacity);
    frame(SNES_PAD_R<<(x*12));for(unsigned i=0;i<4;++i)frame(0);
    X3CoopMenuInfo weapon_menu;
    check(!X3CoopMenuRead(&weapon_menu) && X3CoopRuntimeState()->players[x].body[0x33]==chosen,
          "actual native weapon-cycle input selects X's weapon without opening pause");
    check(RtlLoadSnapshotFromMemory(prepared,base),"Start admission restores the stage before native weapon cycling");
    if(getenv("X3_COOP_PAIRED_MENU_PIXELS")){
      for(unsigned seat=0;seat<2;++seat){
        check(RtlLoadSnapshotFromMemory(prepared,base),"original screen reference restores its own native gameplay");
        X3CoopState solo=*X3CoopRuntimeState();check(X3CoopProject(&solo,g_ram,seat),"original screen projects the actual character");
        X3CoopRuntimeEnable(false);frame(SNES_PAD_START);for(unsigned i=0;i<92;++i)frame(0);
        g_rtl_game_info->draw_ppu_frame();
        for(unsigned y=0;y<224;++y)memcpy(native_reference+(seat*224u+y)*256u*4u,
            g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch+g_ppu->extraLeftRight*4u,256u*4u);
      }
      check(RtlLoadSnapshotFromMemory(prepared,base),"paired native screens restore their original reference prerequisite");
    }
    menu_open(owner);check(X3CoopRuntimeEnabled(),"either player's native menu keeps co-op scheduling enabled");
    const char *output=getenv("X3_COOP_PAIRED_MENU_OUTPUT");
    if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-r%u-p%u-active.sav",output,roster,owner+1);
      check(RtlSaveSnapshot(path),"actual paired native menu saves for host presentation inspection");}
    if(getenv("X3_COOP_PAIRED_MENU_PIXELS")){
      X3Display_SetLocalSplit(true);X3CoopMenuDraw(g_ppu,image,512u*4u,512,-1);
      if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-r%u-p%u-split.ppm",output,roster,owner+1);
        WritePpm(path,image,512u*4u,512,224,0,false);}
      for(unsigned seat=0;seat<2;++seat){
        unsigned differences=0;
        for(unsigned y=0;y<224;++y)for(unsigned col=0;col<256;++col)
          differences+=memcmp(image+(y*512u+seat*256u+col)*4u,native_reference+((seat*224u+y)*256u+col)*4u,3)!=0;
        fprintf(stderr,"native screen comparison roster%u owner%u seat%u differences%u\n",roster,owner,seat,differences);
        if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-r%u-p%u-reference%u.ppm",output,roster,owner+1,seat+1);
          WritePpm(path,native_reference+seat*256u*224u*4u,256u*4u,256,224,0,false);}
        check(!differences,"each native split screen matches the original character's complete pause image");
      }
    }
    if(getenv("X3_COOP_PAIRED_MENU_PAGES")){
      size_t page_base=RtlSaveSnapshotToMemory(paused,capacity);
      for(unsigned seat=0;seat<2;++seat){
        check(RtlLoadSnapshotFromMemory(prepared,base),"native equipment reference starts from the same game");
        X3CoopState solo=*X3CoopRuntimeState();check(X3CoopProject(&solo,g_ram,seat),"native equipment reference owns its actual character");
        X3CoopRuntimeEnable(false);frame(SNES_PAD_START);for(unsigned i=0;i<92;++i)frame(0);
        menu_press(0,0x0010);for(unsigned i=0;i<90;++i)frame(0);
        check(g_ram[0x1eda]==8 && !g_ram[0x1f0c],"original native input actually opens and settles the equipment page");
        fprintf(stderr,"native page original roster%u owner%u seat%u state%u/%u busy%u\n",roster,owner,seat,g_ram[0x1eda],g_ram[0x1edb],g_ram[0x1f0c]);
        g_rtl_game_info->draw_ppu_frame();
        for(unsigned y=0;y<224;++y)memcpy(native_reference+(seat*224u+y)*256u*4u,
            g_ppu->renderBuffer+(size_t)y*g_ppu->renderPitch+g_ppu->extraLeftRight*4u,256u*4u);
      }
      check(RtlLoadSnapshotFromMemory(paused,page_base),"native paired equipment restores its actual paused machine");
      frame(menu_pad(0,0x0010)|menu_pad(1,0x0010));frame(0);frame(0);for(unsigned i=0;i<90;++i)frame(0);
      check(g_ram[0x1eda]==8 && !g_ram[0x1f0c],"paired native input actually opens and settles the equipment page");
      X3Display_SetLocalSplit(true);X3CoopMenuDraw(g_ppu,image,512u*4u,512,-1);
      if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-r%u-p%u-page-split.ppm",output,roster,owner+1);WritePpm(path,image,512u*4u,512,224,0,false);}
      for(unsigned seat=0;seat<2;++seat){
        unsigned differences=0;
        for(unsigned y=0;y<224;++y)for(unsigned col=0;col<256;++col)
          differences+=memcmp(image+(y*512u+seat*256u+col)*4u,native_reference+((seat*224u+y)*256u+col)*4u,3)!=0;
        fprintf(stderr,"native page comparison roster%u owner%u seat%u differences%u\n",roster,owner,seat,differences);
        if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-r%u-p%u-page-reference%u.ppm",output,roster,owner+1,seat+1);WritePpm(path,native_reference+seat*256u*224u*4u,256u*4u,256,224,0,false);}
        check(!differences,"each native equipment page matches the actual original character's full image");
      }
      n=RtlSaveSnapshotToMemory(expected,capacity);check(RtlLoadSnapshotFromMemory(expected,n),"native equipment page snapshot restores");
      size_t restored=RtlSaveSnapshotToMemory(work,capacity);same(expected,n,work,restored,"native equipment page preserves the complete machine immediately");
      check(RtlLoadSnapshotFromMemory(paused,page_base),"native equipment qualification restores the paired controls prerequisite");
    }
    uint8_t shared[0xb00],palettes[2][64];memcpy(shared,g_ram+0xd18,sizeof(shared));
    unsigned positions[2][2];for(unsigned seat=0;seat<2;++seat){
      memcpy(palettes[seat],X3CoopRuntimeState()->players[seat].palette,64);
      for(unsigned axis=0;axis<2;++axis)positions[seat][axis]=support_position(seat,axis?8:5);}
    frame(menu_pad(x,0x0400)|menu_pad(z,0x0400));frame(0);frame(0);
    X3CoopMenuInfo after=menu_read();
    fprintf(stderr,"native paired navigation r%u owner%u cursor%u/%u selection%u/%u\n",roster,owner,after.cursor[0],after.cursor[1],after.selection[0],after.selection[1]);
    check(after.cursor[x]==chosen/2 && after.cursor[z]==0 && after.selection[x]==chosen && !after.selection[z],
          "both native seats navigate independently while Zero cannot select X's acquired weapons");
    check(!memcmp(shared,g_ram+0xd18,sizeof(shared)),"native navigation leaves shared enemies, hostile shots and items frozen");
    for(unsigned seat=0;seat<2;++seat){check(!memcmp(palettes[seat],X3CoopRuntimeState()->players[seat].palette,64),"native navigation preserves private gameplay palettes");
      for(unsigned axis=0;axis<2;++axis)check(support_position(seat,axis?8:5)==positions[seat][axis],"native navigation cannot move either paused body");}
    frame(menu_pad(x,0x0100)|menu_pad(z,0x0100));frame(0);frame(0);after=menu_read();
    check(after.cursor[0]==10 && after.cursor[1]==10,"both original screens reach acquired tank0 through native Right");
    size_t pause_size=RtlSaveSnapshotToMemory(paused,capacity);
    X3CoopState reference_actor=*X3CoopRuntimeState();
    check(X3CoopProject(&reference_actor,g_ram,winner),"original tank reference installs the actual race winner");
    X3CoopRuntimeEnable(false);g_ram[X3_COOP_BODY+0x27]=1;g_ram[0x1fd2]=setup.players[winner].maximum_health;
    memset(g_ram+0x1f02,0,4);menu_native_subtank(0x00c8b2,0);
    uint8_t reference[57][3];
    for(unsigned tick=0;tick<=56;++tick){
      reference[tick][0]=g_ram[X3_COOP_BODY+0x27];reference[tick][1]=g_ram[0x1fb7];reference[tick][2]=g_ram[0x1f03];
      if(tick<56)menu_native_subtank(0x00c901,0);
    }
    check(RtlLoadSnapshotFromMemory(paused,pause_size),"shared tank race restores the exact paused prerequisite");
    frame(menu_pad(0,0x4000)|menu_pad(1,0x4000));after=menu_read();
    fprintf(stderr,"native tank race r%u owner%u winner%u healing%u/%u hp%u/%u tank%02x\n",roster,owner,winner,after.healing[0],after.healing[1],contact_hp(0),contact_hp(1),g_ram[0x1fb7]);
    check(after.healing[winner] && !after.healing[partner],"simultaneous native tank requests give exactly one deterministic owner");
    check(contact_hp(winner)==reference[0][0] && g_ram[0x1fb7]==reference[0][1],"native full tank starts without an immediate duplicate heal or spend");
    size_t healing_size=RtlSaveSnapshotToMemory(paused,capacity);
    check(interp_bridge_add_pre_opcode_hook(0x00c909,menu_heal_continuation_probe),"native healing continuation probe registers");
    menu_probe_armed=true;menu_probe_seen=false;
    for(unsigned tick=1;tick<=56;++tick){frame(0);
      if(contact_hp(winner)!=reference[tick][0] || g_ram[0x1fb7]!=reference[tick][1])
        fprintf(stderr,"native tank mismatch tick%u hp%u/%u reserve%02x/%02x\n",tick,contact_hp(winner),reference[tick][0],g_ram[0x1fb7],reference[tick][1]);
      check(contact_hp(winner)==reference[tick][0] && g_ram[0x1fb7]==reference[tick][1],
            "paired subtank health and reserve spending match every original native healing tick");}
    after=menu_read();check(menu_probe_seen,"shared healing replay exercises an interrupted original native pulse");
    check(!after.healing[0] && !after.healing[1] && g_ram[0x1fb7]==reference[56][1],
        "original full tank finishes, repacks remaining reserves and releases its shared lock");
    size_t done=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(paused,healing_size),"active shared healing restores for full replay");
    for(unsigned tick=0;tick<56;++tick)frame(0);
    n=RtlSaveSnapshotToMemory(work,capacity);same(expected,done,work,n,"native paired healing and the complete machine replay identically");
    unsigned reserve=g_ram[0x1fb7];menu_press(winner,0x4000);after=menu_read();
    check(!after.healing[winner] && g_ram[0x1fb7]==reserve,
          "full personal HP refuses an acquired nonempty tank without spending shared reserve");
    check(RtlLoadSnapshotFromMemory(paused,healing_size),"resume coordination restores active healing");
    frame(menu_pad(partner,0x1000)|menu_pad(partner,0x0100));frame(0);after=menu_read();
    check(after.healing[winner] && after.cursor[partner]==11,"healing refuses Start while the other original screen navigates independently");
    frame(menu_pad(partner,0x4000));after=menu_read();
    check(after.healing[0] && after.healing[1],"different shared tanks heal both living actors with separate locks");
    check(interp_bridge_add_pre_opcode_hook(0x00ceaf,menu_compaction_probe),"native reserve repacking probe registers");
    menu_compactions=0;for(unsigned tick=0;tick<60;++tick)frame(0);
    check(menu_compactions==1 && contact_hp(partner)==4 && (g_ram[0x1fb7]&31)==0 && (g_ram[0x1fb8]&31)==0,
          "overlapping native full and partial heals spend their own reserves before one shared repacking");
    frame(menu_pad(0,0x4000)|menu_pad(1,0x4000));frame(0);after=menu_read();
    check(!after.healing[0] && !after.healing[1] && contact_hp(partner)==4 && (g_ram[0x1fb7]&31)==0,
          "empty native tanks cannot refill, heal a partner or underflow reserves");
    n=RtlSaveSnapshotToMemory(expected,capacity);size_t marker=menu_chunk(expected,n);
    static const unsigned offsets[]={14,20,21,22,23,24,25,26,27,28,29,30,31,34,35,36,37,122,127};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(*offsets);++i){
      memcpy(work,expected,n);work[marker+offsets[i]]=255;
      check(!RtlLoadSnapshotFromMemory(work,n),"malformed native paired menu rejects before machine mutation");
      size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);same(expected,n,work,unchanged,"refused native menu preserves the complete machine");}
    static const unsigned native_offsets[]={0,4,5,6,7,9,10,18,51};
    for(unsigned i=0;i<sizeof(native_offsets)/sizeof(*native_offsets);++i){
      memcpy(work,expected,n);work[marker+128+native_offsets[i]]=255;
      check(!RtlLoadSnapshotFromMemory(work,n),"malformed cartridge context rejects before machine mutation");
      size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);same(expected,n,work,unchanged,"refused cartridge context preserves the complete machine");}
    size_t ram_at;check(RtlSnapshotGuestLayout(NULL,&ram_at)>0,"native paired pause finds snapshot RAM");
    memcpy(work,expected,n);work[ram_at+0x1f37]=0x81;
    check(!RtlLoadSnapshotFromMemory(work,n),"interactive native screens cannot restore over the Escape world phase");
    same(expected,n,work,RtlSaveSnapshotToMemory(work,capacity),"refused Escape phase preserves the complete machine");
    g_rtl_game_info->draw_ppu_frame();menu_pixels(image);
    n=RtlSaveSnapshotToMemory(paused,capacity);check(RtlLoadSnapshotFromMemory(paused,n),"native paired output restores");
    g_rtl_game_info->draw_ppu_frame();menu_pixels(work);same(image,256u*224u*4u,work,256u*224u*4u,"native paired screen pixels restore exactly");
    frame(menu_pad(0,0x1000)|menu_pad(1,0x1000));for(unsigned i=0;i<120;++i)frame(0);
    check(!(g_ram[0x1f37]&1) && !X3CoopMenuRead(&after) && X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,
          "simultaneous Start requests complete one native exit and restore world ownership");
    check(X3CoopRuntimeState()->players[x].body[0x33]==chosen && X3CoopRuntimeState()->players[z].body[0x33]==0,
          "native exit commits X's selected weapon without overwriting Zero's inventory");
    unsigned start=coop_x(z);for(unsigned i=0;i<24;++i)frame(SNES_PAD_RIGHT<<(z*12));
    check(coop_x(z)>start,"Zero retains native control after another player's equipment commit");
    menu_open(owner);n=RtlSaveSnapshotToMemory(paused,capacity);size_t reference_draw=0;
    for(unsigned view=0;view<4;++view){
      check(RtlLoadSnapshotFromMemory(paused,n),"native presentation starts from identical pre-draw machine");
      X3Display_SetLocalSplit(view==1);X3Display_SetPresentationSeat(view<2?-1:(int)view-2);
      g_rtl_game_info->draw_ppu_frame();size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);
      if(!view){reference_draw=unchanged;memcpy(expected,work,unchanged);}
      else same(expected,reference_draw,work,unchanged,"native shared/split/selected presentation never changes simulation");}
    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
    /* Original Exit uses Start and the cartridge's original eligibility. */
    for(unsigned seat=0;seat<2;++seat)for(unsigned attempts=0;menu_read().cursor[seat]!=14 && attempts<4;++attempts)
      menu_press(seat,0x0100);
    check(menu_read().cursor[0]==14 && menu_read().cursor[1]==14,"both screens reach the original Exit entry");
    unsigned stage=g_ram[0x1fae],flag=g_snes->cart->rom[0x33cba+stage];
    check(stage>=1 && stage<=8 && flag<18,"prepared stage retains the original Escape eligibility table");
    g_ram[0x1fbc+flag]&=(uint8_t)~64;menu_press(owner,0x1000);after=menu_read();
    check((g_ram[0x1f37]&1) && !after.modal,"original Escape refusal keeps both native menus usable");
    check(interp_bridge_add_pre_opcode_hook(0x00cb76,menu_exit_world_probe),"native Escape world draw probe registers");
    menu_exit_world_seen=false;g_ram[0x1fbc+flag]|=64;
    frame(menu_pad(0,0x1000)|menu_pad(1,0x1000));bool exit_task=false;
    for(unsigned i=0;i<240;++i){frame(0);exit_task|=g_ram[0xd2]==0x0a && g_ram[0x1fd5]==0x0a;}
    check(exit_task && menu_exit_world_seen && !X3CoopMenuRead(&after) && !g_ram[0x1f37] && X3CoopRuntimeEnabled(),
          "simultaneous native Exit reaches the one restored world draw and original stage-exit task");
    fprintf(stderr,"native paired menu QUALIFIED roster%u owner%u\n",roster,owner);
    check(RtlLoadSnapshotFromMemory(prepared,base),"native paired menu restores its original stage prerequisite");
  }
  const char *survivor=getenv("X3_COOP_PAIRED_MENU_SURVIVOR_STATE");
  if(survivor && *survivor){
    check(RtlLoadSnapshot(survivor),"paired menu starts from a real contact-death survivor");
    unsigned living=X3CoopRuntimeState()->anchor,fallen=living^1;
    check(X3CoopRuntimeState()->players[fallen].life==X3_COOP_FALLEN,"native survivor fixture has a fallen partner");
    g_ram[0x1fb7]=0x8e;menu_press(fallen,0x1000);X3CoopMenuInfo info;
    check(!X3CoopMenuRead(&info) && !(g_ram[0x1f37]&1),"a fallen actor cannot claim the shared pause");
    menu_open(living);info=menu_read();unsigned cursor=info.cursor[fallen];
    Ppu *fallen_picture=ppu_init();check(fallen_picture && X3CoopMenuNativePicture(fallen,fallen_picture),
        "fallen partner retains its own constructed native screen in split presentation");
    ppu_free(fallen_picture);
    X3Display_SetLocalSplit(true);X3CoopMenuDraw(g_ppu,image,512u*4u,512,-1);
    const char *output=getenv("X3_COOP_PAIRED_MENU_OUTPUT");
    if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-survivor-split.ppm",output);WritePpm(path,image,512u*4u,512,224,0,false);}

    menu_press(fallen,0x0400);menu_press(fallen,0x4000);info=menu_read();
    check(info.cursor[fallen]==cursor && !info.healing[fallen] && g_ram[0x1fb7]==0x8e,
          "a fallen partner cannot navigate or spend a shared tank");
    size_t n=RtlSaveSnapshotToMemory(paused,capacity);
    check(RtlLoadSnapshotFromMemory(paused,n),"paused survivor retains the fallen partner after restore");
    menu_press(living,0x1000);for(unsigned i=0;i<120;++i)frame(0);
    check(!X3CoopMenuRead(&info) && !(g_ram[0x1f37]&1) &&
          X3CoopRuntimeState()->players[fallen].life==X3_COOP_FALLEN,
          "native pause and resume do not revive the contact-death partner");
  }
  free(paused);free(prepared);free(image);free(native_reference);check(RtlLoadSnapshotFromMemory(original,size),"paired menu checks restore original gameplay");
}
