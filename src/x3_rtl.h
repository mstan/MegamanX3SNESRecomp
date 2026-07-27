#ifndef X3_RTL_H_
#define X3_RTL_H_

#include "common_rtl.h"
#include "common_cpu_infra.h"

struct SaveLoadInfo;

/* Whole-program LLE bring-up driver. See x3_rtl.c. */
void RunOneFrameOfGame(void);
void X3DrawPpuFrame(void);

/* Per-frame 16:9 HUD anchoring. Slot map inherited from X2 and NOT
 * surveyed on X3 (provenance note in x3_rtl.c); inert unless widescreen
 * is active AND the health bar signature is present in OAM. */
void X3ConfigureWsHud(void);

/* Per-frame exact BG1/BG2 gutter fill, self-gated by native VRAM parity. */
void X3ConfigureWsBgMargins(void);

/* LLE host execution cursor (resume PC + CpuState) — not covered by
 * snes_saveload, which snapshots the unused snes->cpu. */
void X3StateSaveExtra(struct SaveLoadInfo *sli);
void X3StateLoadExtra(struct SaveLoadInfo *sli, uint32_t version);
void X3OnStateLoaded(uint32_t version);

#endif  /* X3_RTL_H_ */
