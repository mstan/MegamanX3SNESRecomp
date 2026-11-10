#pragma once
#include "x3_coop.h"
#include "snes/saveload.h"

enum {X3_COOP_WORLD_DRAWS=128,X3_COOP_BODY_CHR_BYTES=8192};
typedef struct X3CoopWorldDraw {
  uint16_t address;
  uint8_t owner,body[32];
} X3CoopWorldDraw;
typedef struct X3CoopPresentedFrame {
  X3CoopState players;
  uint16_t camera[2],views[2][2];
  uint8_t body_chr[2][X3_COOP_BODY_CHR_BYTES];
  uint8_t count;
  X3CoopWorldDraw actors[X3_COOP_WORLD_DRAWS];
  uint8_t weapon_energy[18],hud_palette[64];
} X3CoopPresentedFrame;

void X3CoopPresentationReset(void);
void X3CoopPresentationEnroll(const X3CoopState *players,const uint16_t *vram,const uint8_t *rom,size_t size);
bool X3CoopPresentationBodyDma(unsigned seat,const uint8_t *body,const uint8_t *rom,size_t size);
void X3CoopPresentationBegin(const X3CoopState *players,const uint8_t *ram,unsigned x,unsigned y);
void X3CoopPresentationSubmit(const X3CoopState *players,const uint8_t *ram,unsigned address,unsigned owner);
void X3CoopPresentationComplete(void);
void X3CoopPresentationUpload(void);
const X3CoopPresentedFrame *X3CoopPresentationVisible(void);
size_t X3CoopPresentationEncodedSize(void);
void X3CoopPresentationSave(SaveLoadInfo *io);
bool X3CoopPresentationLoad(SaveLoadInfo *io);
bool X3CoopPresentationPreflight(const void *data,size_t size);
