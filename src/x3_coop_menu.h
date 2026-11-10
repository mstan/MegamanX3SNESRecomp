#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct Ppu;
enum { X3_COOP_MENU_STATE_BYTES=128 };
/* Read-only presentation of the serialized simulation menu. */
typedef struct X3CoopMenuInfo {
  uint8_t owner,modal,cursor[2],selection[2],healing[2];
} X3CoopMenuInfo;
bool X3CoopMenuRead(X3CoopMenuInfo *out);
/* Presentation-only native reference for pixel qualification. */
void X3CoopMenuNativeReference(bool enabled);
void X3CoopMenuDraw(const struct Ppu *ppu,uint8_t *pixels,size_t pitch,unsigned width,int seat);
