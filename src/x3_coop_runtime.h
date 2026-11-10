#pragma once
#include "x3_coop.h"
#include "cpu_state.h"
struct SaveLoadInfo;
/* Development actor qualification only. No launcher package is advertised
 * until world contact, lifecycle and presentation integration qualify. */
void X3CoopRuntimeEnable(bool enabled);
bool X3CoopRuntimeEnabled(void);
void X3CoopRuntimeFrame(void);
const X3CoopState *X3CoopRuntimeState(void);
void X3CoopRuntimeReset(void);
void X3CoopRuntimeSave(struct SaveLoadInfo *sli);
bool X3CoopRuntimeLoad(struct SaveLoadInfo *sli);
bool X3CoopRuntimePreflight(const void *data, size_t size);
