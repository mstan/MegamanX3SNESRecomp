#pragma once
#include "x3_coop.h"
#include "snes/ppu.h"
struct X3CoopPresentedFrame;
enum {X3_COOP_HUD_WIDTH=32,X3_COOP_HUD_HEIGHT=112};
enum {X3_COOP_HUD_COMPACT,X3_COOP_HUD_HORIZONTAL,X3_COOP_HUD_NATIVE_REFERENCE};
void X3CoopHudSetLayout(unsigned layout);
unsigned X3CoopHudLayout(void);
void X3CoopHudPlayer(const Ppu *ppu,const struct X3CoopPresentedFrame *frame,unsigned seat,
                     const uint8_t *rom,size_t size,bool inverted,
                     uint16_t pixels[X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT]);
void X3CoopHudNativeBegin(Ppu *ppu);
void X3CoopHudNativeRestore(Ppu *ppu);
