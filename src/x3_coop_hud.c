#include "x3_coop_hud.h"
#include <string.h>
static unsigned word(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static void sprite(const Ppu *p,uint16_t *out,int x,int y,unsigned tile,
                   const uint16_t colors[16],const uint8_t *badge){
  for(unsigned dy=0;dy<16;++dy)for(unsigned dx=0;dx<16;++dx){
    int px=x+(int)dx,py=y+(int)dy;
    if(px<0 || px>=X3_COOP_HUD_WIDTH || py<0 || py>=X3_COOP_HUD_HEIGHT)continue;
    unsigned color=0,shift=7-(dx&7);
    if(badge){
      unsigned at=((dy/8)*2+dx/8)*32+(dy&7)*2;
      for(unsigned plane=0;plane<4;++plane)color|=((badge[at+(plane/2)*16+plane%2]>>shift)&1)<<plane;
    }else{
      unsigned number=(tile+dy/8*16+dx/8)&255;
      unsigned at=(PPU_objTileAdr1(p)+number*16+(dy&7))&32767;
      unsigned lo=p->vram[at],hi=p->vram[(at+8)&32767];
      color=((lo>>shift)&1)|(((lo>>(shift+8))&1)<<1)|(((hi>>shift)&1)<<2)|(((hi>>(shift+8))&1)<<3);
    }
    if(color)out[py*X3_COOP_HUD_WIDTH+px]=(uint16_t)(0x8000|colors[color]);
  }
}
void X3CoopHudHealth(const Ppu *p,const X3CoopPlayer *player,unsigned maximum,
                     const uint8_t *rom,size_t size,uint16_t *pixels){
  memset(pixels,0,X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT*sizeof(*pixels));
  if(!p || !player || player->life==X3_COOP_ABSENT)return;
  uint16_t colors[16];memcpy(colors,p->cgram+0xa0,sizeof(colors));
  uint8_t badge[128];const uint8_t *icon=NULL;
  if(rom && size>=0x160e20 && size>=0x63100){
    /* Original84:DCE6 selects list5C: two native64-byte badge transfers.
     * 81:804A loads its16 colors from8C:B0E0 into native HUD paletteA0. */
    unsigned source=player->character==X3_COOP_ZERO?0x160d20:0x160c20;
    memcpy(badge,rom+source,64);memcpy(badge+64,rom+source+0xc0,64);icon=badge;
    if(player->character==X3_COOP_ZERO)
      for(unsigned i=0;i<16;++i)colors[i]=(uint16_t)word(rom+0x630e0+i*2);
  }
  if(maximum>32)maximum=32;
  unsigned value=player->life==X3_COOP_FALLEN?0:player->body[0x27]&127;
  if(value>maximum)value=maximum;
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
  for(int i=(int)count-1;i>=0;--i)sprite(p,pixels,0,strips[i].y,strips[i].tile,colors,NULL);
  sprite(p,pixels,0,80,0x86,colors,icon);
}
