/* Two cartridge pause contexts share one paused world. The native UI's
 * drawing memory is private to each character; no gameplay pool is replayed. */
enum { NATIVE_MENU_PICTURE=PPU_SAVESTATE_REGS_SIZE+PPU_SAVESTATE_MEM_SIZE };
typedef struct NativeMenuView {
  uint8_t ui[64],globals[28],palette[512],objects[224],tables[512],window[16];
  uint8_t picture[NATIVE_MENU_PICTURE];
} NativeMenuView;
typedef struct NativeMenus {
  uint8_t magic[4],valid,resume,primed,init_step;
  NativeMenuView view[2];
  uint8_t cx4_before[3072],background[2048],background_ready;
} NativeMenus;
static NativeMenus native_menus;
size_t X3CoopMenuNativeEncodedSize(void){return sizeof(NativeMenus);}
static void native_menus_reset(void){
  memset(&native_menus,0,sizeof(native_menus));memcpy(native_menus.magic,"X3U1",4);
}
static void native_menu_picture_save(uint8_t *bytes){
  Ppu *p=g_snes->ppu;
  memcpy(bytes,&p->inidisp,PPU_SAVESTATE_REGS_SIZE);
  memcpy(bytes+PPU_SAVESTATE_REGS_SIZE,p->cgram,PPU_SAVESTATE_MEM_SIZE);
}
static void native_menu_picture_load(const uint8_t *bytes){
  Ppu *p=g_snes->ppu;
  memcpy(&p->inidisp,bytes,PPU_SAVESTATE_REGS_SIZE);
  memcpy(p->cgram,bytes+PPU_SAVESTATE_REGS_SIZE,PPU_SAVESTATE_MEM_SIZE);
  ++p->vramWriteCount;++p->oamWriteCount;
}
/* Native7E:227F consumes the inline tile uploads built at0600. Use the
 * same PPU ports for each private menu before the next context owns them. */
static bool native_menu_uploads(void){
  unsigned end=g_ram[0xa5],at=0;Ppu *p=g_snes->ppu;
  while(at<end){
    if(end-at<4)return false;
    unsigned count=g_ram[0x603+at];
    if(!count || count>end-at-4)return false;
    ppu_write(p,0x15,g_ram[0x600+at]);
    ppu_write(p,0x16,g_ram[0x601+at]);ppu_write(p,0x17,g_ram[0x602+at]);at+=4;
    for(unsigned i=0;i<count;++i)ppu_write(p,(uint8_t)(0x18+(i&1)),g_ram[0x600+at+i]);
    at+=count;
  }
  g_ram[0xa5]=0;return true;
}
static void native_menu_context_save(unsigned seat){
  NativeMenuView *v=&native_menus.view[seat];
  memcpy(v->ui,g_ram+0x1ed8,sizeof(v->ui));
  memcpy(v->globals,g_ram+0xb4,sizeof(v->globals));
  memcpy(v->palette,g_ram+0x300,sizeof(v->palette));
  memcpy(v->objects,g_ram+0x1818,sizeof(v->objects));
  memcpy(v->tables,g_ram+0x1c640,sizeof(v->tables));
  memcpy(v->window,g_ram+0x952,sizeof(v->window));
  native_menu_picture_save(v->picture);
}
static void native_menu_context_load(unsigned seat){
  const NativeMenuView *v=&native_menus.view[seat];
  memcpy(g_ram+0x1ed8,v->ui,sizeof(v->ui));
  memcpy(g_ram+0xb4,v->globals,sizeof(v->globals));
  memcpy(g_ram+0x300,v->palette,sizeof(v->palette));
  memcpy(g_ram+0x1818,v->objects,sizeof(v->objects));
  memcpy(g_ram+0x1c640,v->tables,sizeof(v->tables));
  memcpy(g_ram+0x952,v->window,sizeof(v->window));
  native_menu_picture_load(v->picture);
}
static bool native_menu_chr(NativeMenuView *view,unsigned character){
  const uint8_t *rom=g_snes->cart->rom;size_t size=g_snes->cart->romImageSize;
  unsigned key=rom[0x31f8f+(character==X3_COOP_ZERO)],at=0x30000+word(rom+0x317ad+key)-32768;
  uint8_t *vram=view->picture+PPU_SAVESTATE_REGS_SIZE+0x420;
  for(unsigned i=0;i<64;++i,at+=7){
    if(at+7>size)return false;
    unsigned count=word(rom+at);if(count&1)return true;
    unsigned dest=word(rom+at+2)*2,source=word(rom+at+4)|(unsigned)rom[at+6]<<16;
    size_t offset=((source>>16)&127)*32768+(source&32767);
    if(dest>65536 || count>65536-dest || offset>size || count>size-offset)return false;
    memcpy(vram+dest,rom+offset,count);
  }
  return false;
}
/* Native00:B47A's character-icon uploads normally wait for the background
 * decompressor and NMI DMA queue. A detached menu owns neither queue; use the
 * existing exact cartridge decoder and original count/VRAM upload records. */
static bool native_menu_graphics(NativeMenuView *view,unsigned key){
  const uint8_t *rom=g_snes->cart->rom;size_t size=g_snes->cart->romImageSize;
  if(0x373c3+key+2>size)return false;
  size_t at=0x30000+word(rom+0x373c3+key)-32768;
  uint8_t decoded[X3_RESOURCE_FILE_MAX];
  uint8_t *vram=view->picture+PPU_SAVESTATE_REGS_SIZE+0x420;
  for(unsigned files=0;files<64;++files){
    if(at>=size)return false;unsigned file=rom[at++];if(file==255)return true;
    size_t length=0,used=0;
    if(!X3ResourceDecode(rom,size,file,decoded,sizeof(decoded),&length))return false;
    for(unsigned segment=0;segment<64;++segment){
      if(at+4>size)return false;unsigned count=word(rom+at),target=word(rom+at+2);at+=4;
      unsigned dest=(target&32767)*2;
      if(!count || dest+count>65536 || used>length || count>length-used)return false;
      memcpy(vram+dest,decoded+used,count);used+=count;
      if(target&32768)break;if(segment==63)return false;
    }
  }
  return false;
}
static void native_menu_prepare(void){
  unsigned owner=menu.owner;
  native_menu_context_save(owner);native_menus.valid=native_menus.primed=(uint8_t)(1u<<owner);
  unsigned other=owner^1;
  NativeMenuView *v=&native_menus.view[other];*v=native_menus.view[owner];
  if(!native_menus.background_ready){enabled=false;return;}
  unsigned map=(PPU_bgTilemapAdr(g_snes->ppu,1)&0x7fff)*2;
  memcpy(v->picture+PPU_SAVESTATE_REGS_SIZE+0x420+map,native_menus.background,sizeof(native_menus.background));
  unsigned selected=state.players[other].character==X3_COOP_X?state.players[other].body[0x33]:0;
  v->ui[1]=2;v->ui[2]=v->ui[3]=v->ui[0x34]=v->ui[0x37]=v->ui[0x38]=0;
  v->ui[0x0a]=v->ui[0x35]=(uint8_t)selected;memset(v->ui+0x2a,0,4);
  if(v->ui[0x39]!=4)v->ui[0x39]=(uint8_t)(state.players[other].character==X3_COOP_ZERO?2:0);
  for(unsigned i=0;i<sizeof(v->objects);i+=32)v->objects[i+1]=0;
  uint16_t colors[256];for(unsigned i=0;i<256;++i)colors[i]=(uint16_t)word(v->palette+i*2);
  unsigned palette=state.players[other].character==X3_COOP_ZERO?0xd0:0x100+selected;
  if(!X3ResourcePalette(g_snes->cart->rom,g_snes->cart->romImageSize,palette,0,colors) ||
     !native_menu_chr(v,state.players[other].character) ||
     !native_menu_graphics(v,v->ui[0x39]==4?0x96:state.players[other].character==X3_COOP_ZERO?0x92:0x94)){enabled=false;return;}
  if(state.players[other].character==X3_COOP_X && (!selected || selected==18) &&
     (g_ram[0x1fd7]&0xf0)==0xf0 &&
     !X3ResourcePalette(g_snes->cart->rom,g_snes->cart->romImageSize,0x1b8,0,colors)){enabled=false;return;}
  for(unsigned i=0;i<256;++i)putword(v->palette+i*2,colors[i]);
  memcpy(v->picture+PPU_SAVESTATE_REGS_SIZE,v->palette,512);
  native_menus.valid|=(uint8_t)(1u<<other);
}
/* The same native builders used by00:CB7B, followed by the two entry draws.
 * Their ROM layouts and character data remain authoritative. */
static const uint16_t native_menu_builders[]={0xd113,0xd1eb,0xd451,0xd23c,0xd160,0xd485,0xd521,0xd415,0xcd6e,0xcd6e};
static void native_menu_build_step(CpuState *cpu){
  if(!native_menus.init_step)restore(cpu,&menu.entry);
  /* Each finished RTS has consumed C5D8's return. Reuse that same frame
   * while retaining the preceding builder's result registers. */
  cpu->S=menu.entry.s;cpu->D=native_menus.init_step>=8?0x1ed8:0;
  if(native_menus.init_step==6)cpu->A=(uint16_t)((cpu->A&0xff00)|0x30);
  if(native_menus.init_step==7)cpu->A=(uint16_t)((cpu->A&0xff00)|0x3c);
  interp_bridge_pre_opcode_redirect(native_menu_builders[native_menus.init_step]);
}
static void native_menu_pass(CpuState *cpu,unsigned seat){
  native_menu_context_load(seat);
  if(state.players[seat].life==X3_COOP_ALIVE)X3CoopProject(&state,g_ram,seat);
  else X3CoopProjectRetired(&state,g_ram,seat);
  menu.op=4;menu.seat=(uint8_t)seat;
  unsigned held=menu.held[seat],edge=menu.edge[seat];menu.edge[seat]=0;
  if(menu.heal[0][1] || menu.heal[1][1]){held&=~0x1000u;edge&=~0x1000u;}
  putword(g_ram+0xa10,held);putword(g_ram+0xa12,edge);
  /* Native UI writes own a detached picture, so hardware sleep does not
   * serialize the two users or expose a partially updated native menu. */
  g_snes->ppu->inidisp|=128;
  if(!(native_menus.primed&(1u<<seat))){menu.op=5;native_menus.init_step=0;native_menu_build_step(cpu);}
}
static void native_menu_finish_picture(unsigned seat){
  NativeMenuView *v=&native_menus.view[seat];
  native_menu_context_save(seat);
  SnesRenderRegisters registers=SnesRenderRegistersLoad(v->picture);
  registers.inidisp=g_ram[0xb4]&15;
  registers.hScroll[0]=(uint16_t)word(g_ram+0xb5);registers.vScroll[0]=(uint16_t)word(g_ram+0xb7);
  registers.hScroll[1]=(uint16_t)word(g_ram+0xb9);registers.vScroll[1]=(uint16_t)word(g_ram+0xbb);
  memcpy(v->picture,&registers,sizeof(registers));
  memcpy(v->picture+PPU_SAVESTATE_REGS_SIZE,v->palette,512);
  cx4_sync(g_snes->cart->cx4,g_cpu.master_cycles);
  const uint8_t *oam=cx4_ram_ptr(g_snes->cart->cx4,0x6000);
  if(oam)memcpy(v->picture+PPU_SAVESTATE_REGS_SIZE+512,oam,544);
}
bool X3CoopMenuNativePicture(unsigned seat,Ppu *out){
  if(!out || seat>1 || menu.phase<2 || !(native_menus.primed&(1u<<seat)))return false;
  const uint8_t *bytes=native_menus.view[seat].picture;
  memcpy(&out->inidisp,bytes,PPU_SAVESTATE_REGS_SIZE);
  memcpy(out->cgram,bytes+PPU_SAVESTATE_REGS_SIZE,PPU_SAVESTATE_MEM_SIZE);
  ++out->vramWriteCount;++out->oamWriteCount;
  if(menu.phase==3)out->inidisp=g_snes->ppu->inidisp;
  return true;
}
static bool native_menus_valid(const NativeMenus *v,const PairedMenu *m,const X3CoopState *s){
  if(memcmp(v->magic,"X3U1",4) || v->background_ready>1 || v->valid>3 || v->resume>1 || (v->primed&~v->valid) || v->init_step>=sizeof(native_menu_builders)/sizeof(*native_menu_builders))return false;
  if(!m->phase)return !v->valid && !v->resume && !v->primed && !v->init_step && !v->background_ready;
  if(m->phase<2)return !v->valid;
  if(v->valid!=3 || !v->background_ready)return false;
  if(m->op==5 && (v->primed&(1u<<m->seat)))return false;
  if(m->op==4 && !(v->primed&(1u<<m->seat)))return false;
  if(m->op!=5 && v->init_step)return false;
  for(unsigned seat=0;seat<2;++seat)if(v->valid&(1u<<seat)){
    const uint8_t *ui=v->view[seat].ui,*healing=ui+0x2a;
    if(healing[1]!=m->heal[seat][1] ||
       (healing[1] && memcmp(healing,m->heal[seat],4)))return false;
    if((ui[1]!=2 && ui[1]!=4) ||
       ui[2]>16 || (ui[2]&1) || ui[0xa]>30 || (ui[0xa]&1))return false;
  }
  return true;
}
