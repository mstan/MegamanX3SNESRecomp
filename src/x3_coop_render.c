/* Native X3 player sprite data, read from the validated runtime cartridge.
 * Layout/CHR traversal follows the existing X1 extract_zero.py implementation.
 * No extracted graphics are stored in the repository or release package. */
#include "x3_coop_render.h"
#include "x3_coop_runtime.h"
#include "x3_coop_present.h"
#include "x3_coop_hud.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include <string.h>
extern Ppu *g_ppu;
extern Snes *g_snes;
extern uint8_t g_ram[0x20000];
static uint16_t pixels[2][128*128];
static PpuExtraObject objects[2];
static uint16_t guest_palette[32];
static bool palette_borrowed;
static unsigned word(const uint8_t *p) {return p[0]|p[1]<<8;}
static const uint8_t *rom(unsigned a,size_t n) {
  size_t off=((a>>16)&127)*32768+(a&32767);
  if((a&65535)<32768 || off>g_snes->cart->romImageSize || n>g_snes->cart->romImageSize-off)return NULL;
  return g_snes->cart->rom+off;
}
static unsigned pointer(const uint8_t *p) {return word(p)|p[2]<<16;}
static bool draw_pose(unsigned seat,const uint8_t *body) {
  const uint8_t *atlas=X3CoopPresentationVisible()->body_chr[seat];uint16_t *output=pixels[seat];
  unsigned group=body[0x16],pose=body[0x17]&127;
  const uint8_t *p=rom(0x8d8000+group*3,3);if(!p)return false;
  p=rom(pointer(p)+pose*3,3);if(!p)return false;
  p=rom(pointer(p),1);if(!p || *p>64)return false;
  unsigned count=*p;
  /* Re-fetch the complete list so bounds are checked before reading pieces. */
  const uint8_t *directory=rom(0x8d8000+group*3,3);
  const uint8_t *entry=rom(pointer(directory)+pose*3,3);
  p=rom(pointer(entry),1+count*4);if(!p)return false;
  uint16_t colors[256];memcpy(colors,g_ppu->cgram,sizeof(colors));
  const X3CoopState *s=&X3CoopPresentationVisible()->players;
  const uint8_t *palette=s->players[seat].palette;
  for(unsigned j=0;j<32;++j)colors[0x80+j]=(uint16_t)(0x8000|word(palette+2*j));
  memset(output,0,sizeof(pixels[seat]));
  bool flip=(body[0x11]&0x40)!=0,vflip=(body[0x11]&0x80)!=0;
  for(unsigned i=count;i>0;--i) {
    const uint8_t *piece=p+1+(i-1)*4;
    unsigned flags=piece[0],tile=piece[3],size=(flags&32)?16:8;
    int x=(int8_t)piece[1],y=(int8_t)piece[2];
    for(unsigned dy=0;dy<size;++dy)for(unsigned dx=0;dx<size;++dx) {
      unsigned tx=(flags&64)?size-1-dx:dx,ty=(flags&128)?size-1-dy:dy;
      unsigned t=(((tile>>4)+ty/8)&15)*16+((tile+tx/8)&15);
      unsigned off=t*32+(ty&7)*2,shift=7-(tx&7),color=0;
      for(unsigned plane=0;plane<4;++plane)color|=((atlas[off+plane/2*16+plane%2]>>shift)&1)<<plane;
      int px=64+x+(int)dx,py=64+y+(int)dy;if(flip)px=127-px;if(vflip)py=127-py;
      unsigned palette=((flags|body[0x11])>>1)&7;
      if(color && px>=0 && px<128 && py>=0 && py<128)output[py*128+px]=colors[128+palette*16+color];
    }
  }
  return true;
}
const PpuExtraObject *X3CoopRenderPlayer(unsigned seat,int camera_x,int camera_y) {
  const X3CoopState *s=&X3CoopPresentationVisible()->players;
  if(seat>1 || !g_snes || !g_ppu || !X3CoopRuntimeReady() || !s->initialized || s->players[seat].life!=X3_COOP_ALIVE)return NULL;
  const uint8_t *body=s->players[seat].body;
  if(!body[0xe] || !draw_pose(seat,body))return NULL;
  objects[seat]=(PpuExtraObject){
    .x=(int16_t)(word(body+5)-camera_x)-64,
    .y=(int16_t)(word(body+8)-camera_y)-64,
    .width=128,.height=128,.stride=128,
    .priority=(body[0x11]>>4)&3,.oamSlot=108,.order=1,.pixels=pixels[seat]};
  return &objects[seat];
}
void X3CoopRender(void) {
  PpuSetExtraObjects(g_ppu,NULL,0);
  const X3CoopPresentedFrame *frame=X3CoopPresentationVisible();
  const X3CoopState *s=&frame->players;
  palette_borrowed=false;
  if(g_ram[0x1f37]&1)return; /* Native pause owns its portrait palette and objects. */
  if(X3CoopRuntimeReady() && s->initialized) {
    /* NMI can land inside a borrowed actor pass. Draw with the anchor's
     * personal ramps, then restore the exact emulated CGRAM after rendering.
     * Presentation never becomes a simulation write or a rollback input. */
    memcpy(guest_palette,g_ppu->cgram+0x80,sizeof(guest_palette));
    const uint8_t *palette=s->players[s->anchor].palette;
    for(unsigned i=0;i<32;++i)g_ppu->cgram[0x80+i]=(uint16_t)(word(palette+2*i)&32767);
    palette_borrowed=true;
  }
  unsigned partner=s->anchor^1;
  if(!X3CoopRuntimeReady() || !s->initialized || s->players[partner].life!=X3_COOP_ALIVE)return;
  const PpuExtraObject *object=X3CoopRenderPlayer(partner,frame->camera[0],frame->camera[1]);
  if(object)PpuSetExtraObjects(g_ppu,object,1);
}
void X3CoopRenderRestorePalette(void) {
  X3CoopHudNativeRestore(g_ppu);
  if(palette_borrowed)memcpy(g_ppu->cgram+0x80,guest_palette,sizeof(guest_palette));
  palette_borrowed=false;
}
