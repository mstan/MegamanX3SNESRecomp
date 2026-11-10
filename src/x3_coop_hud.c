#include "x3_coop_hud.h"
#include "x3_coop_present.h"
#include "x3_coop_runtime.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include <string.h>
extern Snes *g_snes;
extern uint8_t g_ram[0x20000];
static unsigned layout;
void X3CoopHudSetLayout(unsigned value){if(value<=X3_COOP_HUD_NATIVE_REFERENCE)layout=value;}
unsigned X3CoopHudLayout(void){return layout;}
static unsigned word(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static void sprite(const Ppu *p,uint16_t *out,int x,int y,unsigned tile,
                   const uint16_t colors[16],const uint8_t *badge,bool inverted){
  for(unsigned dy=0;dy<16;++dy)for(unsigned dx=0;dx<16;++dx){
    int px=x+(int)dx,py=y+(int)dy;
    if(px<0 || px>=X3_COOP_HUD_WIDTH || py<0 || py>=X3_COOP_HUD_HEIGHT)continue;
    unsigned sy=inverted?15-dy:dy,color=0,shift=7-(dx&7);
    if(badge){
      unsigned at=((sy/8)*2+dx/8)*32+(sy&7)*2;
      for(unsigned plane=0;plane<4;++plane)color|=((badge[at+(plane/2)*16+plane%2]>>shift)&1)<<plane;
    }else{
      unsigned number=(tile+sy/8*16+dx/8)&255;
      unsigned at=(PPU_objTileAdr1(p)+number*16+(sy&7))&32767;
      unsigned lo=p->vram[at],hi=p->vram[(at+8)&32767];
      color=((lo>>shift)&1)|(((lo>>(shift+8))&1)<<1)|(((hi>>shift)&1)<<2)|(((hi>>(shift+8))&1)<<3);
    }
    if(color)out[py*X3_COOP_HUD_WIDTH+px]=(uint16_t)(0x8000|colors[color]);
  }
}
static bool read_rom(const uint8_t *rom,size_t size,unsigned address,unsigned count,unsigned *value){
  size_t at=((address>>16)&127)*32768+(address&32767);
  if(!rom || (address&65535)<32768 || at>size || count>size-at)return false;
  *value=0;for(unsigned i=0;i<count;++i)*value|=(unsigned)rom[at+i]<<(8*i);return true;
}
static bool weapon_badge(const uint8_t *rom,size_t size,unsigned selection,uint8_t out[128]){
  /* Native04:DCC5 /00:872F uses the original DMA list at06:97AD.
   * Only copy the two HUD transfers, never another view's live CHR. */
  unsigned list;if(!read_rom(rom,size,0x0697ad+selection+0x7e,2,&list))return false;
  unsigned found=0;
  for(unsigned i=0;i<64;++i,list+=7){
    unsigned count,dest,source;
    if(!read_rom(rom,size,0x060000|list,2,&count))return false;
    if(count&1)break;
    if(!read_rom(rom,size,0x060002+list,2,&dest) || !read_rom(rom,size,0x060004+list,3,&source))return false;
    for(unsigned row=0;row<2;++row){unsigned target=0x6ac0+row*0x100;
      if(target<dest || (target-dest)*2+64>count)continue;
      for(unsigned n=0;n<64;++n){unsigned b;
        if(!read_rom(rom,size,source+(target-dest)*2+n,1,&b))return false;
        out[row*64+n]=(uint8_t)b;
      }
      found|=1u<<row;
    }
  }
  return found==3;
}
static void meter(const Ppu *p,uint16_t *pixels,int x,unsigned value,unsigned maximum,
                  const uint16_t colors[16],bool inverted){
  if(maximum>32)maximum=32;
  if(value>maximum)value=maximum;
  if(inverted)value=maximum-value;
  typedef struct HudStrip {int y;unsigned tile;} HudStrip;
  HudStrip strips[10];unsigned count=0;
  int top=64,remaining=(int)value;
  while(remaining>0){remaining-=8;int y=top-(remaining<0?remaining*2:0);
    strips[count++]=(HudStrip){y,0x80};top=y-16;}
  remaining=(int)maximum-(int)value;
  do{remaining-=8;int y=top-(remaining<0?remaining*2:0);
    strips[count++]=(HudStrip){y,0x82};top=y-16;
  }while(remaining>=0);
  strips[count++]=(HudStrip){top,0x84};
  for(int i=(int)count-1;i>=0;--i){unsigned tile=strips[i].tile;int y=strips[i].y;
    if(inverted){y=80-y;if(tile==0x80)tile=0x82;else if(tile==0x82)tile=0x80;}
    sprite(p,pixels,x,y,tile,colors,NULL,inverted);
  }
}
void X3CoopHudPlayer(const Ppu *p,const X3CoopPresentedFrame *frame,unsigned seat,
                     const uint8_t *rom,size_t size,bool inverted,uint16_t *pixels){
  memset(pixels,0,X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT*sizeof(*pixels));
  if(!p || !frame || seat>1)return;
  const X3CoopPlayer *player=&frame->players.players[seat];
  if(player->life==X3_COOP_ABSENT)return;
  uint16_t colors[32];for(unsigned i=0;i<32;++i)colors[i]=(uint16_t)(word(frame->hud_palette+i*2)&32767);
  uint8_t badge[128];const uint8_t *icon=NULL;
  if(rom && size>=0x160e20){
    unsigned source=player->character==X3_COOP_ZERO?0x160d20:0x160c20;
    memcpy(badge,rom+source,64);memcpy(badge+64,rom+source+0xc0,64);icon=badge;
  }
  unsigned hp=player->life==X3_COOP_FALLEN?0:player->body[0x27]&127;
  meter(p,pixels,0,hp,player->maximum_health,colors,inverted);
  int icon_y=inverted?0:80;
  sprite(p,pixels,0,icon_y,0x86,colors,icon,inverted);
  if(inverted && icon){
    /* Mirror the native frame while keeping Zero's symbol upright. */
    for(unsigned y=3;y<=13;++y)for(unsigned x=2;x<14;++x){
      if(y==13 && (x==2 || x==13))continue;
      unsigned at=((y/8)*2+x/8)*32+(y&7)*2,shift=7-(x&7),color=0;
      for(unsigned plane=0;plane<4;++plane)color|=((icon[at+(plane/2)*16+plane%2]>>shift)&1)<<plane;
      pixels[y*X3_COOP_HUD_WIDTH+x]=(uint16_t)(0x8000|colors[color]);
    }
  }
  unsigned charge=player->body[0x58];
  if(player->life==X3_COOP_ALIVE && player->body[0x57]>=20 && charge>=4 && charge<=10 && !(charge&1)){
    /* Original04:ABF9/AC41/ACA1/AD04 publishes these charge levels. Use
     * the owning actor's submitted native color; Zero gets no weapon bar. */
    uint16_t color=(uint16_t)(word(player->palette+18)&32767);if(!color)color=colors[1];
    for(unsigned pip=0;pip<(charge-2)/2;++pip)for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x)
      pixels[(icon_y+2+pip*3+y)*X3_COOP_HUD_WIDTH+12+x]=(uint16_t)(0x8000|color);
  }
  unsigned selection=player->body[0x33];
  if(player->character==X3_COOP_X && selection>=2 && selection<=18 && !(selection&1) &&
     (frame->weapon_energy[selection-1]&0x40) && weapon_badge(rom,size,selection,badge)){
    meter(p,pixels,16,frame->weapon_energy[selection-1]&31,28,colors+16,false);
    sprite(p,pixels,16,80,0xac,colors+16,badge,false);
  }
}

static uint16_t shared_pixels[2][X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT];
static PpuExtraObject shared_objects[3];
static uint32_t captured_hud[256*224];
static PpuOverlayCapture previous_capture;
static uint8_t *previous_surface;
static uint32_t previous_pitch;
static bool overlay_borrowed;
void X3CoopHudNativeBegin(Ppu *p){
  if(!p || !g_snes || !g_snes->cart || !X3CoopRuntimeReady() || layout==X3_COOP_HUD_NATIVE_REFERENCE ||
     g_ram[0x1f37] || p->oam[0]!=0x5008 || p->oam[1]!=0x3486 || (p->highOam[0]&1))return;
  const X3CoopPresentedFrame *frame=X3CoopPresentationVisible();
  if(!frame->players.initialized)return;
  /* Filter original HUD pixels after native OBJ evaluation. OAM, hardware
   * sprite quotas/flags and the complete simulation remain untouched. */
  previous_capture=p->overlayCaptures[kPpuOverlaySource_Obj];
  previous_surface=p->overlayRenderBuffer[kPpuOverlaySource_Obj];
  previous_pitch=p->overlayRenderPitch[kPpuOverlaySource_Obj];overlay_borrowed=true;
  if(!PpuBindOverlaySurface(p,kPpuOverlaySource_Obj,(uint8_t *)captured_hud,256*4) ||
     !PpuSetOverlayCapture(p,kPpuOverlaySource_Obj,8-(int)p->extraLeftCur,0,32,96,kPpuOverlayFlag_RemoveFromGame) ||
     !PpuSetOverlayOamRange(p,0,16)){X3CoopHudNativeRestore(p);return;}
  unsigned count=0;
  if(p->extraObjectCount)shared_objects[count++]=p->extraObjects[0];
  for(unsigned seat=0;seat<2;++seat){const X3CoopPlayer *player=&frame->players.players[seat];
    if(player->life==X3_COOP_ABSENT)continue;
    bool inverted=layout==X3_COOP_HUD_COMPACT && player->character==X3_COOP_ZERO;
    X3CoopHudPlayer(p,frame,seat,g_snes->cart->rom,g_snes->cart->romImageSize,inverted,shared_pixels[seat]);
    shared_objects[count++]=(PpuExtraObject){.x=8-(int)p->extraLeftCur+(layout==X3_COOP_HUD_HORIZONTAL?(int)seat*32:0),
      .y=inverted?100:0,.width=X3_COOP_HUD_WIDTH,.height=X3_COOP_HUD_HEIGHT,.stride=X3_COOP_HUD_WIDTH,
      .priority=3,.oamSlot=0,.order=seat+1,.pixels=shared_pixels[seat]};
  }
  PpuSetExtraObjects(p,shared_objects,count);
}
void X3CoopHudNativeRestore(Ppu *p){
  if(!overlay_borrowed || !p)return;
  p->overlayCaptures[kPpuOverlaySource_Obj]=previous_capture;
  p->overlayRenderBuffer[kPpuOverlaySource_Obj]=previous_surface;
  p->overlayRenderPitch[kPpuOverlaySource_Obj]=previous_pitch;overlay_borrowed=false;
}
