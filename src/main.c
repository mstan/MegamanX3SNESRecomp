/* Mega Man X3 identity/adaptation layer for the shared X2/X3 desktop host. */
#define MMX_RTL_HEADER "x3_rtl.h"
#define MMX_DISPLAY_HEADER "x3_display.h"
#define MMX_SPC_HEADER "x3_spc_player.h"

#define MMX_WINDOW_TITLE "Mega Man X3 (Recompiled)"
#define MMX_GAME_NAME "Mega Man X3"
#define MMX_GAME_REGION "(USA)"
#define MMX_LAUNCHER_TITLE "Mega Man X3 \xE2\x80\x94 Launcher"
#define MMX_ROM_CRC32 0xFA0FE671u
#define MMX_ROM_SHA256_BYTES \
  0x65,0xb0,0x32,0x68,0xaf,0xac,0x29,0x63, \
  0x30,0xe8,0xff,0x8d,0x60,0xdd,0x08,0x25, \
  0x87,0x9e,0x13,0xed,0x65,0x8b,0x37,0x71, \
  0x3c,0x03,0x4a,0x3b,0xd0,0x74,0xf1,0xd7
#define MMX_ROM_SHA256_HEX \
  "65b03268afac296330e8ff8d60dd0825879e13ed658b37713c034a3bd074f1d7"
#define MMX_MOD_GAME_ID "megaman-x3-us"
#define MMX_DEBUG_PORT 4384
#define MMX_HAS_BG3_SUB_OVERLAY 0
#define MMX_WIDESCREEN_STATUS_LINES \
  "# is native 4:3. X3's BG gutters, HUD, and object activation/cull\n" \
  "# windows are adapted for 16:9. See docs/WIDESCREEN.md.\n"

#define MmxDisplayViewport X3DisplayViewport
#define MmxDisplay_ComputeFrameWidth X3Display_ComputeFrameWidth
#define MmxDisplay_ComputeViewport X3Display_ComputeViewport
#define MmxDisplay_GetWindowBaseWidth X3Display_GetWindowBaseWidth
#define MmxDisplay_GetWindowBaseHeight X3Display_GetWindowBaseHeight
#define MmxDisplay_SetWidescreenEnabled X3Display_SetWidescreenEnabled
#define MmxDisplay_IsWidescreenEnabled X3Display_IsWidescreenEnabled
#define MmxDisplay_IsWidescreenActive X3Display_IsWidescreenActive
#define MmxDisplay_GetCurrentFrameWidth X3Display_GetCurrentFrameWidth
#define MmxConfigureWsHud X3ConfigureWsHud
#define MmxConfigureWsBgMargins X3ConfigureWsBgMargins
#define kMmxGameInfo kX3GameInfo

#include "desktop/mmx23_host_main.inc"
