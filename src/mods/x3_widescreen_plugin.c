#include "mod_runtime.h"
#include "x3_display.h"

/*
 * The surveyed renderer/HUD/BG/object-window implementation remains in the
 * game and engine. This plugin moves only its player-facing activation into
 * the package catalog.
 */
static void x3_widescreen_reset(void) {
  X3Display_SetWidescreenEnabled(false);
}

static void x3_widescreen_activate(void) {
  X3Display_SetWidescreenEnabled(true);
}

SNES_MOD_CONSTRUCTOR(x3_register_widescreen_plugin) {
  (void)snes_mod_register_reset_callback(x3_widescreen_reset);
  (void)snes_mod_register_activation_plugin(
      "megaman-x3.widescreen", x3_widescreen_activate);
}
