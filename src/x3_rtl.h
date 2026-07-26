#ifndef X3_RTL_H_
#define X3_RTL_H_

#include "common_rtl.h"
#include "common_cpu_infra.h"

struct SaveLoadInfo;

/* Whole-program LLE bring-up driver. See x3_rtl.c. */
void RunOneFrameOfGame(void);
void X3DrawPpuFrame(void);

/* LLE host execution cursor (resume PC + CpuState) — not covered by
 * snes_saveload, which snapshots the unused snes->cpu. */
void X3StateSaveExtra(struct SaveLoadInfo *sli);
void X3StateLoadExtra(struct SaveLoadInfo *sli, uint32_t version);
void X3OnStateLoaded(uint32_t version);

#endif  /* X3_RTL_H_ */
