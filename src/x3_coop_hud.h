#pragma once
#include "x3_coop.h"
#include "snes/ppu.h"
enum {X3_COOP_HUD_WIDTH=32,X3_COOP_HUD_HEIGHT=96};
void X3CoopHudHealth(const Ppu *ppu,const X3CoopPlayer *player,unsigned maximum,
                     const uint8_t *rom,size_t size,uint16_t pixels[X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT]);
