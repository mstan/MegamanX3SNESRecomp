#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
/* Authored native camera locks shared by the solo and private renderers.
 * A zero world span uses the native field's two outer wall columns. */
typedef struct X3FixedArena {
  uint8_t stage,checkpoint,bounds[10];
  bool moving_top;
  uint16_t world_left,world_right;
} X3FixedArena;
static const X3FixedArena *X3FixedArenaFor(const uint8_t *ram){
  static const X3FixedArena arenas[]={
      {10,255,{0x00,0x16,0x00,0x06,0x00,0x06,0x00,0x16,0x00,0x16},false,0,0}, /* Press */
      {10,255,{0x00,0x16,0x00,0x07,0x00,0x07,0x00,0x16,0x00,0x16},false,0,0}, /* Godkarmachine */
      {12,255,{0x00,0x0a,0x00,0x04,0x00,0x04,0x00,0x0a,0x00,0x0a},false,0,0}, /* Doppler */
      {13,255,{0x00,0x07,0x00,0x0a,0x00,0x0a,0x00,0x07,0x00,0x07},false,0,0}, /* Sigma */
      /* Lab2's water room is512px wide and its ceiling scrolls during the
       * introduction/fight. Preserve real room art in the gutters; reflect
       * only beyond world4352..4864, whose PPU map phase is256..768. */
      {11,2,{0x00,0x12,0x00,0x00,0x00,0x02,0x00,0x11,0x00,0x12},true,256,768},
  };
  for(size_t i=0;i<sizeof(arenas)/sizeof(arenas[0]);++i){
    const X3FixedArena *a=&arenas[i];
    if(ram[0x1fae]==a->stage && (a->checkpoint==255 || ram[0x1fb5]==a->checkpoint) &&
       !memcmp(ram+0x1e68,a->bounds,2) && !memcmp(ram+0x1e6c,a->bounds+4,6) &&
       (a->moving_top || !memcmp(ram+0x1e6a,a->bounds+2,2)))return a;
  }
  return NULL;
}
