#pragma once
#include "x3_coop.h"

typedef struct X3CoopView {int x,y;unsigned seat;} X3CoopView;
/* Simulation policy. A display-seat change never touches it. */
void X3CoopViewsConfigure(bool independent);
bool X3CoopViewsIndependent(void);
void X3CoopViewsSetRom(const uint8_t *rom,size_t size);

typedef struct X3CoopViewState {
  uint16_t limits[2][4],current[2][4],camera[2][2];
  uint8_t independent,valid,camera_valid,stage,initialized;
  uint8_t seen[256];
} X3CoopViewState;
void X3CoopViewsReset(void);
const X3CoopViewState *X3CoopViewsState(void);
bool X3CoopViewsValidState(const X3CoopViewState *state);
void X3CoopViewsSetState(const X3CoopViewState *state);
void X3CoopViewsUpdate(const uint8_t *ram,const X3CoopState *players,bool scene,bool menu);
void X3CoopViewsEndFrame(const uint8_t *ram,const X3CoopState *players,bool scene,bool menu);
X3CoopView X3CoopViewsForPlayer(const uint8_t *ram,const X3CoopState *players,unsigned seat,bool scene);
/* The unclamped follow result feeds the retail body-bound/pit predicate. */
X3CoopView X3CoopViewsFollowPlayer(const uint8_t *ram,const X3CoopState *players,unsigned seat);
void X3CoopViewsPlayerLimits(const uint8_t *ram,const X3CoopState *players,unsigned seat,int limits[4]);
void X3CoopViewsAdoptPlayer(uint8_t *ram,const X3CoopState *players,unsigned seat,bool scene);
/* Native door/encounter commitments may temporarily cross their two raw
 * limits. Store the same effective clamped interval in private views. */
void X3CoopViewsCommitNative(const uint8_t *ram,const X3CoopState *players);
bool X3CoopViewsContains(const uint8_t *ram,const X3CoopState *players,
    int x,int y,int left,int right,int top,int bottom,int margin,bool scene);
bool X3CoopViewsSeen(unsigned flag);
void X3CoopViewsMark(unsigned flag,bool seen);
enum {X3_COOP_VIEW_STATE_BYTES=5+40+256};
void X3CoopViewsSave(void *out);
bool X3CoopViewsLoad(const void *bytes,size_t size,bool apply);
