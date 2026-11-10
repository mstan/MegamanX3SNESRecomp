#pragma once
#include "x3_coop.h"
#include "cpu_state.h"
struct SaveLoadInfo;
/* Native actors, world borrowing and saved scene continuations. */
void X3CoopRuntimeEnable(bool enabled);
bool X3CoopRuntimeEnabled(void);
bool X3CoopRuntimeReady(void);
/* Participation requests are admitted only at a balanced gameplay boundary.
 * Return requires clear native terrain beside the living participant. */
bool X3CoopRuntimeWithdraw(unsigned seat);
bool X3CoopRuntimeRejoin(unsigned seat);
void X3CoopRuntimeSetP1Character(unsigned character);
void X3CoopRuntimeSetDoorPolicy(bool both_present);
void X3CoopRuntimeSetStockZero(bool stock);
bool X3CoopRuntimeStockZero(void);
bool X3CoopRuntimeZeroAvailable(void);
bool X3CoopRuntimeSceneActive(void);
void X3CoopRuntimeScenePresentation(X3CoopState *copy);
void X3CoopRuntimeFrame(void);
const X3CoopState *X3CoopRuntimeState(void);
void X3CoopRuntimeReset(void);
void X3CoopRuntimeSave(struct SaveLoadInfo *sli);
bool X3CoopRuntimeLoad(struct SaveLoadInfo *sli);
bool X3CoopRuntimePreflight(const void *data, size_t size);
