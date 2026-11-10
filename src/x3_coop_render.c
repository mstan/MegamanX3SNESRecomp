/* Native X3 Zero sprite data, read from the validated runtime cartridge.
 * Layout/CHR traversal follows the existing X1 extract_zero.py implementation.
 * No extracted graphics are stored in the repository or release package. */
#include "x3_coop_render.h"
#include "x3_coop_runtime.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include <string.h>
extern Ppu *g_ppu;
extern Snes *g_snes;
extern uint8_t g_ram[0x20000];
static uint8_t chr[8192];
static uint16_t pixels[128*128];
static PpuExtraObject object;
static uint16_t guest_palette[32];
static bool palette_borrowed;
static unsigned word(const uint8_t *p) {return p[0]|p[1]<<8;}
static const uint8_t *rom(unsigned a,size_t n) {
  size_t off=((a>>16)&127)*32768+(a&32767);
  if((a&65535)<32768 || off>g_snes->cart->romImageSize || n>g_snes->cart->romImageSize-off)return NULL;
  return g_snes->cart->rom+off;
}
static unsigned pointer(const uint8_t *p) {return word(p)|p[2]<<16;}
static bool draw_pose(const uint8_t *body) {
  unsigned group=body[0x16],pose=body[0x17]&127;
  unsigned dma=group==0x4a?0x85d6a8:group==0x4b?0x85db47:0;
  if(!dma || pose>=(group==0x4a?117:21))return false;
  const uint8_t *p=rom(dma+2*pose,2);if(!p)return false;
  unsigned transfer=(dma&0xff0000)|((dma+word(p))&65535);
  for(unsigned i=0;i<32;++i,transfer+=6) {
    p=rom(transfer,6);if(!p)return false;
    if(!p[0])break;
    unsigned target=word(p+4),dest=((target&32767)-0x6000)*2,count=p[0]*16;
    const uint8_t *source=rom(pointer(p+1),count);
    if(!source || dest>sizeof(chr) || count>sizeof(chr)-dest)return false;
    memcpy(chr+dest,source,count);
    if(target&0x8000)break;
    if(i==31)return false;
  }
  p=rom(0x8d8000+group*3,3);if(!p)return false;
  p=rom(pointer(p)+pose*3,3);if(!p)return false;
  p=rom(pointer(p),1);if(!p || *p>64)return false;
  unsigned count=*p;
  /* Re-fetch the complete list so bounds are checked before reading pieces. */
  const uint8_t *directory=rom(0x8d8000+group*3,3);
  const uint8_t *entry=rom(pointer(directory)+pose*3,3);
  p=rom(pointer(entry),1+count*4);if(!p)return false;
  uint16_t colors[256];memcpy(colors,g_ppu->cgram,sizeof(colors));
  const X3CoopState *s=X3CoopRuntimeState();
  const uint8_t *palette=s->players[s->anchor^1].palette;
  for(unsigned j=0;j<32;++j)colors[0x80+j]=(uint16_t)(0x8000|word(palette+2*j));
  memset(pixels,0,sizeof(pixels));
  bool flip=(body[0x11]&0x40)!=0;
  for(unsigned i=count;i>0;--i) {
    const uint8_t *piece=p+1+(i-1)*4;
    unsigned flags=piece[0],tile=piece[3],size=(flags&32)?16:8;
    int x=(int8_t)piece[1],y=(int8_t)piece[2];
    for(unsigned dy=0;dy<size;++dy)for(unsigned dx=0;dx<size;++dx) {
      unsigned tx=(flags&64)?size-1-dx:dx,ty=(flags&128)?size-1-dy:dy;
      unsigned t=(((tile>>4)+ty/8)&15)*16+((tile+tx/8)&15);
      unsigned off=t*32+(ty&7)*2,shift=7-(tx&7),color=0;
      for(unsigned plane=0;plane<4;++plane)color|=((chr[off+plane/2*16+plane%2]>>shift)&1)<<plane;
      int px=64+x+(int)dx,py=64+y+(int)dy;if(flip)px=127-px;
      unsigned palette=((flags>>1)&7)|1;
      if(color && px>=0 && px<128 && py>=0 && py<128)pixels[py*128+px]=colors[128+palette*16+color];
    }
  }
  return true;
}
void X3CoopRender(void) {
  PpuSetExtraObjects(g_ppu,NULL,0);
  const X3CoopState *s=X3CoopRuntimeState();
  palette_borrowed=false;
  if(X3CoopRuntimeEnabled() && s->initialized) {
    /* NMI can land inside a borrowed actor pass. Draw with the anchor's
     * personal ramps, then restore the exact emulated CGRAM after rendering.
     * Presentation never becomes a simulation write or a rollback input. */
    memcpy(guest_palette,g_ppu->cgram+0x80,sizeof(guest_palette));
    const uint8_t *palette=s->players[s->anchor].palette;
    for(unsigned i=0;i<32;++i)g_ppu->cgram[0x80+i]=(uint16_t)(word(palette+2*i)&32767);
    palette_borrowed=true;
  }
  unsigned partner=s->anchor^1;
  if(!X3CoopRuntimeEnabled() || !s->initialized || s->players[partner].life!=X3_COOP_ALIVE)return;
  const uint8_t *body=s->players[partner].body;
  if(!body[0xe] || !draw_pose(body))return;
  object=(PpuExtraObject){
    .x=(int16_t)(word(body+5)-word(g_ram+0x1e5d))-64,
    .y=(int16_t)(word(body+8)-word(g_ram+0x1e60))-64,
    .width=128,.height=128,.stride=128,
    .priority=(body[0x11]>>4)&3,.oamSlot=108,.order=1,.pixels=pixels};
  PpuSetExtraObjects(g_ppu,&object,1);
}
void X3CoopRenderRestorePalette(void) {
  if(palette_borrowed)memcpy(g_ppu->cgram+0x80,guest_palette,sizeof(guest_palette));
  palette_borrowed=false;
}
