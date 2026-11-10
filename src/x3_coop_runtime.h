#pragma once
#include "x3_coop.h"
#include "cpu_state.h"
struct SaveLoadInfo;
/* Native actors, world borrowing and saved scene continuations. */
void X3CoopRuntimeEnable(bool enabled);
bool X3CoopRuntimeEnabled(void);
void X3CoopRuntimeSetP1Character(unsigned character);
void X3CoopRuntimeSetDoorPolicy(bool both_present);
bool X3CoopRuntimeSceneActive(void);
void X3CoopRuntimeScenePresentation(X3CoopState *copy);
void X3CoopRuntimeFrame(void);
const X3CoopState *X3CoopRuntimeState(void);
void X3CoopRuntimeReset(void);
void X3CoopRuntimeSave(struct SaveLoadInfo *sli);
bool X3CoopRuntimeLoad(struct SaveLoadInfo *sli);
bool X3CoopRuntimePreflight(const void *data, size_t size);
