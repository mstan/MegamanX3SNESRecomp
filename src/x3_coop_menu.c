#include "x3_coop_menu.h"
#include "x3_display.h"
#include "snes/ppu.h"
#include <string.h>

static bool native_reference;
static Ppu *native_picture;
static uint32_t native_pixels[256*224];
void X3CoopMenuNativeReference(bool enabled){native_reference=enabled;}
void X3CoopMenuDraw(const Ppu *ppu,uint8_t *pixels,size_t pitch,unsigned width,int seat){
  X3CoopMenuInfo menu;
  if(native_reference || !ppu || !pixels || width<256 || pitch<width*4 || !X3CoopMenuRead(&menu))return;
  bool split=X3Display_LocalSplit() && seat<0 && width>=512;
  unsigned count=split?2:1,panel=width/count;
  if(!native_picture)native_picture=ppu_init();
  if(!native_picture)return;
  for(unsigned col=0;col<count;++col){
    unsigned who=split?col:seat>=0?(unsigned)seat:menu.owner;
    if(!X3CoopMenuNativePicture(who,native_picture)){
      for(unsigned y=0;y<224;++y)memset(pixels+(size_t)y*pitch+col*panel*4,0,panel*4);
      continue;
    }
    memcpy(native_picture->brightnessMult,ppu->brightnessMult,sizeof(native_picture->brightnessMult));
    memcpy(native_picture->brightnessMultHalf,ppu->brightnessMultHalf,sizeof(native_picture->brightnessMultHalf));
    native_picture->renderBuffer=(uint8_t *)native_pixels;native_picture->renderPitch=256*4;
    native_picture->evenFrame=ppu->evenFrame;
    for(unsigned line=0;line<=224;++line)PpuDrawDetachedLine(native_picture,(int)line);
    unsigned left=col*panel+(panel-256)/2;
    for(unsigned y=0;y<224;++y)memcpy(pixels+(size_t)y*pitch+left*4,native_pixels+y*256,256*4);
  }
}
