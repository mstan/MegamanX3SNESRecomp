#include "common_cpu_infra.h"
#include "x3_rtl.h"

/* .title drives coverage-artifact naming (tier2_<title>_*.json) — keep it
 * unique per ROM so sibling Mega Man X ports never collide. */
const RtlGameInfo kX3GameInfo = {
  .title = "x3",
  .initialize = NULL,
  .run_frame = &RunOneFrameOfGame,
  .draw_ppu_frame = &X3DrawPpuFrame,
  .save_name_prefix = "save",
  .state_save_extra = &X3StateSaveExtra,
  .state_load_extra = &X3StateLoadExtra,
  .on_state_loaded = &X3OnStateLoaded,
};
