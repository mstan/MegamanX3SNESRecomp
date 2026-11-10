#include "x3_coop_menu.h"
#include "x3_coop_runtime.h"
#include "snes/ppu.h"
#include <stdio.h>
extern uint8_t g_ram[0x20000];
static bool native_reference;
void X3CoopMenuNativeReference(bool enabled){native_reference=enabled;}
/* The original pause font is already installed in BG1 VRAM. Reuse its
 * letters; its limited alphabet lacks Q/Z. Small missing glyphs and numerals
 * below are drawn directly, without allocating guest tiles or OAM slots. */
static unsigned glyph(const Ppu *p,unsigned ch,unsigned y){
  static const uint8_t letters[26]={0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x82,0x83,0,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0};
  static const uint8_t digits[10][7]={
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}};
  if(ch>='0' && ch<='9')return y<7?digits[ch-'0'][y]<<1:0;
  if(ch=='/')return 1u<<y;
  if(ch=='-')return y==3?60:0;
  if(ch=='>')return y<4?1u<<(5-y):y<7?1u<<(y-1):0;
  if(ch=='Q'){static const uint8_t rows[8]={60,66,66,66,82,74,60,2};return rows[y];}
  if(ch=='Z'){static const uint8_t rows[8]={126,2,4,8,16,32,126,0};return rows[y];}
  if(ch<'A' || ch>'Z')return 0;
  unsigned tile=letters[ch-'A'],bits=0;
  unsigned at=(0x1000+tile*16+y)&32767;
  unsigned lo=p->vram[at],hi=p->vram[(at+8)&32767];
  for(unsigned x=0;x<8;++x){unsigned shift=7-x;
    unsigned color=((lo>>shift)&1)|(((lo>>(shift+8))&1)<<1)|(((hi>>shift)&1)<<2)|(((hi>>(shift+8))&1)<<3);
    /* Native glyph ink is11..13;14 is its shadow and15 its opaque UI
     * background. Treating every nonzero texel as ink makes solid blocks. */
    if(color>=11 && color<=13)bits|=1u<<(7-x);
  }
  return bits;
}
static uint32_t tint(const Ppu *p,unsigned rgb){
  uint32_t out=0;
  for(unsigned c=0;c<3;++c){unsigned v=(rgb>>(c*8))&255;
    out|=(unsigned)p->brightnessMult[v>>3]<<(c*8);}
  return out;
}
static void box(uint8_t *pixels,size_t pitch,unsigned width,int x,int y,int w,int h,uint32_t color){
  for(int row=y;row<y+h && row<224;++row){if(row<0)continue;
    uint32_t *dst=(uint32_t*)(pixels+(size_t)row*pitch);
    for(int col=x;col<x+w && col<(int)width;++col)if(col>=0)dst[col]=color;}
}
static void text(const Ppu *p,uint8_t *pixels,size_t pitch,unsigned width,int x,int y,const char *s,uint32_t color){
  for(;*s;++s,x+=8)for(unsigned row=0;row<8;++row){unsigned bits=glyph(p,(unsigned char)*s,row);
    for(unsigned col=0;col<8;++col)if(bits&(128u>>col))box(pixels,pitch,width,x+(int)col,y+(int)row,1,1,color);}
}
static const char *const choices[16]={"X BUSTER","ACID BURST","PARASITIC BOMB","TRIAD THUNDER","SPINNING BLADE","RAY SPLASHER","GRAVITY WELL","FROST SHIELD","TORNADO FANG","HYPER CANNON","SUBTANK 1","SUBTANK 2","SUBTANK 3","SUBTANK 4","EXIT STAGE","RESUME"};
void X3CoopMenuDraw(const Ppu *p,uint8_t *pixels,size_t pitch,unsigned width,int seat){
  X3CoopMenuInfo menu;
  if(native_reference || !p || !pixels || width<128 || pitch<width*4 || !X3CoopMenuRead(&menu))return;
  const X3CoopState *s=X3CoopRuntimeState();
  uint32_t background=tint(p,0x081526),edge=tint(p,0x284462),white=tint(p,0xe0efff),gray=tint(p,0x8291a4),yellow=tint(p,0xffdc6b);
  box(pixels,pitch,width,0,0,(int)width,224,background);
  unsigned count=seat<0?2:1,panel=width/count;
  for(unsigned col=0;col<count;++col){unsigned who=seat<0?col:(unsigned)seat;
    if(who>1)continue;const X3CoopPlayer *actor=&s->players[who];int left=(int)(col*panel)+8;
    uint32_t own=tint(p,actor->character==X3_COOP_X?0x70c8ff:0xff8880);
    char line[32];snprintf(line,sizeof(line),"P%u %s",who+1,actor->character==X3_COOP_X?"X":"ZERO");
    box(pixels,pitch,width,left-2,10,(int)panel-12,2,edge);text(p,pixels,pitch,width,left,18,line,own);
    unsigned hp=actor->life==X3_COOP_FALLEN?0:actor->body[0x27]&127;
    snprintf(line,sizeof(line),"HP %u/%u",hp,actor->maximum_health);text(p,pixels,pitch,width,left,32,line,white);
    if(actor->life!=X3_COOP_ALIVE){text(p,pixels,pitch,width,left,62,actor->life==X3_COOP_FALLEN?"FALLEN":"WITHDRAWN",gray);continue;}
    unsigned selected=menu.selection[who];
    text(p,pixels,pitch,width,left,48,actor->character==X3_COOP_X?choices[selected/2]:"Z BUSTER",own);
    unsigned first=menu.cursor[who]<3?0:menu.cursor[who]>12?10:menu.cursor[who]-3;
    if(actor->character==X3_COOP_ZERO)first=10;
    for(unsigned row=0;row<6 && first+row<16;++row){unsigned index=first+row;
      bool usable=index>=14 || (index>=10?(g_ram[0x1fb7+index-10]&128)!=0:
          actor->character==X3_COOP_X && (!index || (g_ram[0x1fbb+index*2-1]&64)));
      uint32_t color=usable?white:gray;int y=68+(int)row*13;
      if(index==menu.cursor[who]){text(p,pixels,pitch,width,left,y,">",yellow);color=yellow;}
      /* Native compact panels have room for fourteen glyphs per row. */
      char label[15];snprintf(label,sizeof(label),"%.14s",choices[index]);
      text(p,pixels,pitch,width,left+8,y,label,color);
    }
    snprintf(line,sizeof(line),"T %u %u %u %u",g_ram[0x1fb7]&31,g_ram[0x1fb8]&31,g_ram[0x1fb9]&31,g_ram[0x1fba]&31);
    text(p,pixels,pitch,width,left,154,line,white);
    text(p,pixels,pitch,width,left,169,menu.healing[who]?"HEALING":"FIRE TO USE",menu.healing[who]?yellow:gray);
    text(p,pixels,pitch,width,left,184,"START RESUMES",gray);
    if(menu.modal){text(p,pixels,pitch,width,left,200,menu.modal==who+1?"EXIT? FIRE YES":"PENDING EXIT",yellow);
      if(menu.modal==who+1)text(p,pixels,pitch,width,left,212,"JUMP CANCEL",gray);}
  }
  if(count==2)box(pixels,pitch,width,(int)panel-1,10,2,204,edge);
}
