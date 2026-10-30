/* Mega Man X3 — LLE-first bring-up runtime.
 *
 * The faithful floor per PRINCIPLES.md: boot the real RESET vector through the
 * interpreter bridge, inject NMI only once the game arms NMITIMEN, service
 * IRQs as they latch, and cap each host frame at one NTSC frame of master
 * clocks so a boot clear-loop cannot monopolize the bridge.
 *
 * AOT coverage is an optimization layered on top: whatever recomp/*.cfg
 * declares gets emitted into src/gen and the bridge calls it; everything else
 * runs the real ROM bytes on the interpreter tier. There is deliberately no
 * host-side task scheduler here — Mega Man X 1's fiber scheduler is an MMX1
 * finding and must not be assumed for this title.
 */
#include "x3_rtl.h"
#include "mmx_replay.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_cpu_infra.h"
#include "cpu_state.h"
#include "cpu_trace.h"
#include "snes/cart.h"
#include "snes/cx4.h"
#include "snes/interp_bridge.h"
#include "snes/snes.h"
#include "snes/dma.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include "snes/ws_shadow.h"
#include "snes/mmx23_background.h"
#include "types.h"
#include "widescreen.h"

extern Snes *g_snes;
extern Dma *g_dma;
extern Ppu *g_ppu;
extern uint8_t g_ram[0x20000];

/* NTSC: 1364 master clocks/scanline x 262 scanlines/frame. */
enum { kX3MasterClocksPerFrame = 1364u * 262u };

static bool s_lle_did_reset = false;
static uint32_t s_lle_resume_pc = 0;
static unsigned s_lle_host_frames = 0;
static bool s_lle_extra_loaded = false;
static bool s_execution_loaded;
static void x3_patch_ws_interp_obj_windows(void);
static void x3_patch_hunter_landing(void);

/* The original HG51B program computes signed screen X, then writes its low
 * byte at02:80E6. Retain that value for presentation, and attach it only when
 * DMA actually commits the corresponding XY word to PPU OAM. Neither the
 * microcode nor guest OAM/camera/collision/progression is changed. */
static int16_t s_cx4_oam_x[128];
static uint8_t s_cx4_oam_valid[16];
static bool s_cx4_oam_supported;

static void x3_cx4_ram_written(void *context, uint32_t instruction,
                              uint16_t address, unsigned byte, uint32_t value) {
  (void)context;
  if (address >= 512 || (address & 3)) return;
  const unsigned slot = address >> 2;
  s_cx4_oam_valid[slot >> 3] &= ~(1u << (slot & 7));
  if (instruction == 0x0280e6 && byte == 0) {
    int x = (int)(value & 0xffff);
    if (x >= 32768) x -= 65536;
    s_cx4_oam_x[slot] = (int16_t)x;
    s_cx4_oam_valid[slot >> 3] |= 1u << (slot & 7);
  }
}

static void x3_oam_xy_uploaded(uint8_t bank, uint16_t y_address, uint8_t slot) {
  if (!g_snes || !g_snes->cart || !s_cx4_oam_supported ||
      !cart_is_cx4_window(g_snes->cart, bank, y_address)) return;
  /* Accept both RAM mirrors, but never IO, a shifted byte pair, or a source
   * whose current byte no longer matches the captured full coordinate. */
  const unsigned offset = y_address & 0xfff;
  if (offset >= 512 || (offset & 3) != 1) return;
  const unsigned source_slot = offset >> 2;
  if (!(s_cx4_oam_valid[source_slot >> 3] & (1u << (source_slot & 7)))) return;
  const int16_t x = s_cx4_oam_x[source_slot];
  if ((g_ppu->oam[slot * 2] & 255) == (x & 255))
    PpuWsSetOamFullX(g_ppu, slot, x);
}

static void x3_install_oam_observers(void) {
  static const uint8_t signature[] = {0x60,0x80,0x68,0x80,0x66,0xe0,0x0c,0xe0,0x00,0xec};
  Cart *cart = g_snes ? g_snes->cart : NULL;
  s_cx4_oam_supported = cart && cart->cx4 && cart->romSize >= 0x100e8 &&
      !memcmp(cart->rom + 0x100de, signature, sizeof(signature));
  if (cart && cart->cx4)
    cx4_set_ram_write_observer(cart->cx4,
        s_cx4_oam_supported ? x3_cx4_ram_written : NULL, NULL);
  dma_set_oam_xy_notify_hook(s_cx4_oam_supported ? x3_oam_xy_uploaded : NULL);
}

/* Read a 16-bit CPU vector out of bank $00's vector table. */
static uint32_t x3_read_vector_pc24(uint16_t vec_addr) {
  uint8 lo = cpu_read8(&g_cpu, 0x00, vec_addr);
  uint8 hi = cpu_read8(&g_cpu, 0x00, (uint16)(vec_addr + 1));
  return ((uint32_t)hi << 8) | lo; /* bank $00 */
}

static void x3_run_interrupt(uint16_t vec_addr, uint64_t deadline) {
  cpu_push_interrupt_frame(&g_cpu);
  interp_bridge_set_master_deadline(deadline);
  (void)interp_bridge_run_interrupt(&g_cpu,
                                    x3_read_vector_pc24(vec_addr));
  interp_bridge_set_master_deadline(0);
}

static void x3_run_nmi(uint64_t deadline) {
  const uint16 s_entry = g_cpu.S;
  cpu_push_interrupt_frame(&g_cpu);
  interp_bridge_set_master_deadline(deadline);
  const int ok = interp_bridge_run_interrupt(&g_cpu,
                                            x3_read_vector_pc24(0xFFEA));
  interp_bridge_set_master_deadline(0);
  if (!ok) {
    static unsigned reports;
    if (reports < 8) {
      reports++;
      fprintf(stderr, "[x3_rtl] NMI bail S=$%04X (restore $%04X)\n",
              (unsigned)g_cpu.S, (unsigned)s_entry);
    }
    g_cpu.S = s_entry;
  }
}

static int x3_boot_log_enabled(void) {
  static int v = -1;
  if (v < 0) {
    /* Opt-IN. This used to default on, so release builds wrote a boot/heartbeat
     * line to stderr every 60 frames for the whole session. */
    const char *e = getenv("SNESRECOMP_X3_BOOTLOG");
    v = (e && e[0] && e[0] != '0') ? 1 : 0;
  }
  return v;
}

void RunOneFrameOfGame(void) {
  x3_install_oam_observers();
  /* Generated bodies route viewport constants through X3WsObjWin*.
   * Interpreter fallbacks fetch the original ROM bytes, so mirror those
   * dynamic constants into the private runtime ROM copy each frame. */
  x3_patch_ws_interp_obj_windows();
  x3_patch_hunter_landing();

  if (!s_lle_did_reset) {
    cpu_state_init(&g_cpu, g_ram);
    s_lle_resume_pc = x3_read_vector_pc24(0xFFFC);
    fprintf(stderr, "[x3_rtl] LLE boot from RESET vector $%06X\n",
            (unsigned)s_lle_resume_pc);
    s_lle_did_reset = true;
    s_lle_host_frames = 0;
  }

  const uint64_t frame_end =
      g_cpu.master_cycles + (uint64_t)kX3MasterClocksPerFrame;

  /* Hardware NMI is gated by NMITIMEN — firing before the game arms it
   * corrupts the SEI boot window. */
  if (s_lle_host_frames > 0 && g_snes->nmiEnabled) {
    g_snes->inNmi = true;
    x3_run_nmi(frame_end);
  }

  /* Slice cap: a backstop, not the loop's normal exit. The exit is the master
   * deadline. Sized so the deadline is reachable even when each slice is short
   * (the quiescence detector needs ~64 instructions to trip, so a slice is
   * never tiny), and a hit is reported rather than silently throttling. */
  enum { kMaxSlices = 8192 };
  int slices = 0;
  while (g_cpu.master_cycles < frame_end && slices < kMaxSlices) {
    slices++;
    interp_bridge_set_master_deadline(frame_end);
    (void)interp_bridge_run_until_quiescent(&g_cpu, s_lle_resume_pc);
    interp_bridge_set_master_deadline(0);
    s_lle_resume_pc = interp_bridge_lle_resume_pc();

    /* Deliver an IRQ only when the guest has interrupts ENABLED. The latch in
     * g_snes->inIrq is the hardware IRQ *line*, which stays asserted until
     * $4211 is read or H/V IRQ is disabled; the 65816 only takes it while
     * P.I is clear. Skipping this check delivers interrupts the hardware would
     * not: Mega Man X3 installs `NOP / BRA self` at its WRAM IRQ entry as the
     * un-armed state and patches it when it actually wants the raster IRQ, so
     * an early delivery parks the guest in that spin loop for the rest of the
     * host frame (visible as interp step-cap bails plus master_cycles running
     * tens of frames ahead of the deadline). */
    if (g_snes->inIrq && !g_cpu._flag_I) {
      x3_run_interrupt(0xFFEE, frame_end);
      continue;
    }
    /* WAI means "sleep until the next interrupt" — end the host frame so the
     * leading NMI on the next RtlRunFrame wakes it one vblank later. */
    if (interp_bridge_lle_took_wai())
      break;
    /* Otherwise the bridge returned because its quiescence detector saw a poll
     * loop, NOT because the frame is over. Keep slicing until the master
     * deadline: the guest may be polling something that only changes as master
     * cycles advance (H/V counter, DMA-done, an APU port), and while NMI is
     * disabled nothing else will move it along. Breaking out here handed the
     * guest ~0.5% of a frame per host frame — a ~200x slowdown that looks like
     * "the interpreter is slow" and is not. */
  }
  if (slices >= kMaxSlices) {
    static unsigned reports;
    if (reports < 8) {
      reports++;
      fprintf(stderr,
              "[x3_rtl] slice cap hit (%d) at master=%llu — guest advanced "
              "%llu of %u cycles this frame\n",
              slices, (unsigned long long)g_cpu.master_cycles,
              (unsigned long long)(g_cpu.master_cycles + (uint64_t)kX3MasterClocksPerFrame - frame_end),
              (unsigned)kX3MasterClocksPerFrame);
    }
  }

  s_lle_host_frames++;

  if (x3_boot_log_enabled() &&
      (s_lle_host_frames <= 10 || (s_lle_host_frames % 60) == 0)) {
    const uint8 inidisp = g_ppu ? g_ppu->inidisp : 0;
    fprintf(stderr,
            "[x3_rtl] host_frame=%u resume=$%06X master=%llu slices=%d "
            "nmiEn=%d irqEn=%d/%d inidisp=$%02X hdmaen=$%02X\n",
            s_lle_host_frames, (unsigned)s_lle_resume_pc,
            (unsigned long long)g_cpu.master_cycles, slices,
            (int)g_snes->nmiEnabled, (int)g_snes->vIrqEnabled,
            (int)g_snes->hIrqEnabled, (unsigned)inidisp,
            (unsigned)g_snesrecomp_last_hdmaen);
  }
}


/* ── 16:9 HUD anchoring ───────────────────────────────────────────────────
 *
 * PROVENANCE (2026-07-26): X2 supplied the candidate slot map; live X3 OAM on
 * save slots 0 and 4 then confirmed the health/weapon/boss layout and its
 * signature:
 *
 *   HP bar      slots  0-5   screen X 8     attr 0x34   anchors LEFT
 *   weapon bar  slots  7-13  screen X 24    attr 0x36   anchors LEFT
 *   boss bar    slots 16-22  screen X 232   attr 0x34   anchors RIGHT
 *   actors      slots 24+
 *
 * The bars anchor to OPPOSITE edges, so they cannot be shifted as one block.
 * PpuAdjustWidescreenHudOamX pushes sprites left of wsHudLeftEnd outward by
 * extraLeftCur and sprites at/after wsHudRightStart outward by extraRightCur.
 *
 * X3's gameplay dialogue is BG3, including rows inside the HUD's Y band.
 * Therefore the OAM anchor band must be configured independently: applying
 * the BG3 HUD split moves the first two text columns toward the left gutter
 * while leaving the rest centered, visibly truncating "(BIT)", "Welcome",
 * and "I'm". BG3 remains authentic/centered here.
 *
 * The layout is symmetric -- HP's left edge is 8px from the left, and the boss
 * bar at X=232 is 16px wide so its right edge is 8px from the right -- so both
 * sides move by the same margin and stay symmetric at any width.
 */
enum {
  kX3HudSlotFirst = 0,        /* HP / weapon occupy 0..15 */
  kX3HudBandHeight = 96,      /* measured HUD Y extent is 0..80       */
  kX3HudLeftEnd = 64,         /* HP X=8 and weapon X=24 are below this */
  kX3HudRightStart = 192,     /* boss X=232 is at/above this          */
};

/* True when the health bar's measured signature is present in live OAM.
 *
 * Cutscenes reuse slots 0-23 for actors, so the shift MUST NOT be applied
 * whenever those slots merely happen to be populated. Gating on the HUD's own
 * fingerprint avoids inventing a WRAM game-state byte and fails safe: no
 * signature, no shift, authentic placement. Requires only the HP bar -- the
 * weapon bar is absent until a special weapon is equipped, and the boss bar
 * only exists during a fight. */
static int x3_ws_hud_present(void) {
  if (!g_ppu) return 0;

  /* Slot 0 is the bar's "X" icon: tile 0x86, palette 0x34, hard against the
   * left edge. Distinctive enough that a cutscene actor will not impersonate it. */
  const unsigned icon_x = g_ppu->oam[0] & 0xFFu;
  const unsigned icon_y = g_ppu->oam[0] >> 8;
  const unsigned icon_tile = g_ppu->oam[1] & 0xFFu;
  const unsigned icon_attr = g_ppu->oam[1] >> 8;
  const unsigned icon_xhi = g_ppu->highOam[0] & 1u;
  if (icon_x != 8u || icon_xhi || icon_attr != 0x34u ||
      icon_tile != 0x86u || icon_y >= kX3HudBandHeight)
    return 0;

  /* Corroborate with the bar frame, slots 0-4. Do NOT demand an exact count:
   * the bar's length varies with max health and slot 5 parks at Y=224 in most
   * of the stage. Save slot 0 measured the same 5/6 live signature as X2, so
   * demanding all six would incorrectly disable the shift during gameplay. */
  unsigned frame_slots = 0;
  for (unsigned slot = 0; slot <= 4; slot++) {
    const unsigned w = slot * 2u;
    const unsigned x = g_ppu->oam[w] & 0xFFu;
    const unsigned y = g_ppu->oam[w] >> 8;
    const unsigned attr = g_ppu->oam[w + 1] >> 8;
    const unsigned xhi = (g_ppu->highOam[w >> 3] >> (w & 7)) & 1u;
    if (x == 8u && !xhi && attr == 0x34u && y < kX3HudBandHeight)
      frame_slots++;
  }
  return frame_slots >= 4u;
}

static bool x3_ws_boss_hud_present(void) {
  unsigned matches=0;
  for (unsigned slot=16; slot<23; ++slot) {
    unsigned w=slot*2;
    unsigned x=g_ppu->oam[w]&255, y=g_ppu->oam[w]>>8;
    unsigned attr=g_ppu->oam[w+1]>>8;
    unsigned xhi=(g_ppu->highOam[slot>>2]>>((slot&3)*2))&1;
    matches += x==232 && !xhi && attr==0x34 && y<kX3HudBandHeight;
  }
  return matches>=4;
}

/* Call once per frame from the host's frame-prep, after g_ws_extra is known. */
static void x3_configure_chamber_view(void) {
  /* The final-stage entrance starts at world256. Its left neighbour is
   * unused map data, not room art. Reuse X1's presentation anchor to spend
   * the widescreen allowance inside this authored room. Leave the guest's
   * camera, scroll limits, OAM and sequence comparisons untouched.
   * Locked boss/escape views keep their separate arena policies. */
  if (!g_ws_active || (g_ws_extra > 128 && !s_cx4_oam_supported) || g_ram[0xd1] != 4 ||
      g_ram[0x1fae] != 13 || (g_ppu->bgmode & 7) != 1 ||
      g_ppu->bgXsc[0] != 0x51 || g_ppu->bgXsc[1] != 0x59)
    return;
  const int camera = g_ram[0x1e5d] | g_ram[0x1e5e] << 8;
  const int left_bound = g_ram[0x1e6e] | g_ram[0x1e6f] << 8;
  const int right_bound = g_ram[0x1e68] | g_ram[0x1e69] << 8;
  const int top = g_ram[0x1e6a] | g_ram[0x1e6b] << 8;
  if (left_bound != 256 || right_bound != 1536 || top != 3584 ||
      camera < left_bound - 16 || camera > right_bound + 16)
    return;
  const int cx = camera < left_bound ? left_bound :
                 camera > right_bound ? right_bound : camera;
  int left_available = cx - left_bound;
  /* BG2 starts at map phase0 in this chamber and advances at half speed.
   * Spending BG1's entire left allowance exposes BG2's unused predecessor
   * cells while moving away from the entrance. Slide within both layers'
   * authored art. The accepted <=512 policy retains its existing origin. */
  const int bg2_left_available = g_ppu->hScroll[1] & 1023;
  if (g_ws_extra > 128 && left_available > bg2_left_available)
    left_available = bg2_left_available;
  int left = left_available;
  if (left > g_ws_extra) left = g_ws_extra;
  int right = right_bound - cx;
  if (right > 2 * g_ws_extra - left) right = 2 * g_ws_extra - left;
  left = left_available;
  if (left > 2 * g_ws_extra - right) left = 2 * g_ws_extra - right;
  PpuSetWidescreenViewAnchor(g_ppu, true);
  PpuWsEnableOamFullX(g_ppu, g_ws_extra > 128);
  PpuSetExtraSideSpace(g_ppu, left, right, 0);
  /* Dialogue is authored in screen coordinates. Keep its complete native
   * sheet at the physical centre while only the world view slides. */
  const int shift = g_ws_extra - g_ppu->extraFrameLeft;
  const PpuWsElasticSeg dialogue = {shift, shift + 256, 0, 256};
  PpuSetWidescreenLayerElasticBand(g_ppu, 2, 0, 224, &dialogue, 1);
}

void X3ConfigureWsHud(void) {
  extern bool g_ws_active;
  if (!g_ppu) return;
  x3_install_oam_observers();
  x3_configure_chamber_view();
  /* X3's HUD is OAM-only; BG3 is gameplay dialogue and must stay centered. */
  PpuSetWidescreenHudSplit(g_ppu, 0, 0, 0);
  if (!g_ws_active || !x3_ws_hud_present()) {
    PpuSetWsHudOamShiftRange2(g_ppu, 0, 0);
    PpuSetWsHudOamShiftRange(g_ppu, 0, 0);   /* off = authentic placement */
    return;
  }
  PpuSetWsHudOamBand(g_ppu, kX3HudBandHeight,
                     kX3HudLeftEnd, kX3HudRightStart);
  PpuSetWsHudOamShiftRange(g_ppu, kX3HudSlotFirst, 16);
  PpuSetWsHudOamShiftRange2(g_ppu, 16, x3_ws_boss_hud_present() ? 7 : 0);
}

/* Maoh paints BG2 characters0x300..0x3ff over a reused circular map. Other
 * characters outside the native view are stale level tiles, not boss art.
 * Filter the renderer's live tile so moving/destructible body pieces retain
 * their exact map attributes and never depend on cached terrain history. */
static uint16_t x3_maoh_margin_tile(void *context, uint16_t address,
                                   uint16_t tile) {
  (void)context;
  (void)address;
  return (tile & 0x3ff) >= 0x300 ? tile : 0;
}

static void x3_configure_fortress_travel(void) {
  if (!g_ws_active || !g_ppu || g_ram[0xd1] != 4 || g_ram[0x1fae] != 13 ||
      (g_ppu->bgmode & 7) != 1 || g_ppu->bgXsc[0] != 0x51 ||
      g_ppu->bgXsc[1] != 0x59)
    return;
  const int camera_y = g_ram[0x1e60] | g_ram[0x1e61] << 8;
  const int left = g_ram[0x1e6e] | g_ram[0x1e6f] << 8;
  const int right = g_ram[0x1e68] | g_ram[0x1e69] << 8;
  const int top = g_ram[0x1e6a] | g_ram[0x1e6b] << 8;
  const int bottom = g_ram[0x1e6c] | g_ram[0x1e6d] << 8;
  /* The entrance anchor and closed boss arenas already own their framing.
   * Verify the loaded fortress layout before interpreting screen0 as void. */
  if (camera_y < 2560 || camera_y > 3584 ||
      (left == 256 && right == 1536 && top == 3584) ||
      (left == right && top == bottom) ||
      g_ram[0xe800 + 10 * 32 + 7] != 10 ||
      g_ram[0xe800 + 14 * 32 + 1] != 17)
    return;
  const int camera_x = g_ram[0x1e5d] | g_ram[0x1e5e] << 8;
  int dx = (g_ppu->hScroll[0] - camera_x) & 511;
  int dy = (g_ppu->vScroll[0] - camera_y) & 255;
  if (dx >= 256) dx -= 512;
  if (dy >= 128) dy -= 256;
  const int world_x = camera_x + dx, world_y = camera_y + dy;
  /* At most two256px source rows intersect the native224px frame. Their
   * nonzero screen spans describe the authored canvas, including intentional
   * transparent holes inside it. Never infer room bounds from opaque pixels.
   * Continue only the outermost wall pixel, avoiding reflected doors/actors.
   * BG2 remains its live circular canvas (its logical Y may be negative). */
  for (unsigned slot = 0; slot < 2; ++slot) {
    const int row = (world_y >> 8) + slot;
    if (row < 10 || row > 14) continue;
    int y0 = row * 256 - world_y, y1 = y0 + 256;
    if (y0 < 0) y0 = 0;
    if (y1 > 224) y1 = 224;
    if (y0 >= y1) continue;
    int first = 32, last = -1;
    for (int x = 0; x < 32; ++x) {
      if (!g_ram[0xe800 + row * 32 + x]) continue;
      if (first == 32) first = x;
      last = x;
    }
    if (last < first) continue;
    int a = first * 256 - world_x + g_ppu->hScroll[0];
    int b = (last + 1) * 256 - world_x + g_ppu->hScroll[0];
    /* Keep authentic quake/scroll overhang. World policies use the hardware
     * phase; a distant negative bound is outside the widest supported view. */
    if (a > g_ppu->hScroll[0]) a = g_ppu->hScroll[0];
    if (a < 0) a = 0;
    if (b < g_ppu->hScroll[0] + 256) b = g_ppu->hScroll[0] + 256;
    PpuSetWidescreenLayerWorldRepeatBandSlot(g_ppu,slot,0,y0,y1,a,b,1);
    PpuSetWidescreenLayerWorldClipBandSlot(g_ppu,slot,1,y0,y1,0,a,b);
    PpuSetWidescreenLayerWorldClipBandSlot(g_ppu,slot,4,y0,y1,0,a,b);
  }
}

/* Exact background source for the widescreen margins. */
void X3ConfigureWsBgMargins(void) {
  Mmx23ConfigureWsBgMargins(g_snes, g_ppu, g_ram, g_ws_active);
  x3_configure_fortress_travel();
  /* Keep Maoh's genuine offscreen body inside its512px canvas while BG1
   * supplies the roof. Never reflect the boss or expose stale map
   * cells beside it. Enter the policy when the original camera reaches the
   * combat roof, including the introduction before the left bound locks. */
  static const uint8_t maoh_vertical_bounds[4] = {0x00,0x01,0x00,0x01};
  const unsigned camera_x = g_ram[0x1e5d] | g_ram[0x1e5e] << 8;
  const unsigned min_x = g_ram[0x1e6e] | g_ram[0x1e6f] << 8;
  if (g_ws_active && g_ppu && g_ram[0xd1] == 4 && g_ram[0x1fae] == 0 &&
      !memcmp(g_ram+0x1e6a,maoh_vertical_bounds,sizeof maoh_vertical_bounds) &&
      g_ram[0x1e70] == 0x00 && g_ram[0x1e71] == 0x1b &&
      min_x >= 6656 && min_x <= 6784 && camera_x >= 6784 && camera_x <= 6912 &&
      (g_ppu->bgmode & 7) == 1 && g_ppu->bgXsc[0] == 0x51 &&
      g_ppu->bgXsc[1] == 0x59) {
    WsShadowUseFilteredVram(1,g_ppu,x3_maoh_margin_tile,NULL);
    PpuSetWidescreenLayerWorldClipBand(g_ppu,1,0,224,1,512,1024);
    /* This scene uses BG3 for its complete repeating skyline rather than
     * the normal dialogue sheet. Its loaded map supplies the distant city. */
    PpuSetWidescreenBg3Widen(g_ppu,1);
    /* The roof ends at world7168. Its combat section starts at6784;
     * reflecting the preceding entrance would invent another boss door.
     * Repeat the32px edge strips only outside the authored roof. Reflecting
     * the whole combat area would copy Maoh's real floor damage as well. */
    PpuSetWidescreenLayerWorldRepeatBand(g_ppu,0,0,224,640,1024,32);
  }
  /* Keep authored fortress arenas' native pixels intact and continue their wall
   * edges into the gutters. The BG2 Kaiser head is a circular object canvas,
   * so it must stay within the room rather than echo beyond the side walls.
   * These presentation fields reset each frame; the escape shaft keeps its
   * normal streaming policy as soon as the original bounds unlock. */
  static const struct {
    uint8_t stage;
    uint8_t checkpoint; /* 255 = any */
    uint8_t bounds[10];
    bool moving_top;
    uint16_t world_left, world_right; /* BG1's unwrapped PPU map phase */
  } fixed_arenas[] = {
      {10,255,{0x00,0x16,0x00,0x06,0x00,0x06,0x00,0x16,0x00,0x16},false,0,0}, /* Press */
      {10,255,{0x00,0x16,0x00,0x07,0x00,0x07,0x00,0x16,0x00,0x16},false,0,0}, /* Godkarmachine */
      {12,255,{0x00,0x0a,0x00,0x04,0x00,0x04,0x00,0x0a,0x00,0x0a},false,0,0}, /* Doppler */
      {13,255,{0x00,0x07,0x00,0x0a,0x00,0x0a,0x00,0x07,0x00,0x07},false,0,0}, /* Sigma */
      /* Lab2's water room is512px wide and its ceiling scrolls during the
       * introduction/fight. Preserve real room art in the gutters; reflect
       * only beyond world4352..4864, whose PPU map phase is256..768. */
      {11,2,{0x00,0x12,0x00,0x00,0x00,0x02,0x00,0x11,0x00,0x12},true,256,768},
  };
  int arena = -1;
  for (size_t i = 0; i < sizeof(fixed_arenas)/sizeof(fixed_arenas[0]); ++i) {
    if (g_ram[0x1fae] == fixed_arenas[i].stage &&
        (fixed_arenas[i].checkpoint == 255 ||
         g_ram[0x1fb5] == fixed_arenas[i].checkpoint) &&
        !memcmp(g_ram+0x1e68,fixed_arenas[i].bounds,2) &&
        !memcmp(g_ram+0x1e6c,fixed_arenas[i].bounds+4,6) &&
        (fixed_arenas[i].moving_top ||
         !memcmp(g_ram+0x1e6a,fixed_arenas[i].bounds+2,2))) {
      arena = (int)i;
      break;
    }
  }
  if (g_ws_active && g_ppu && arena >= 0 &&
      g_ram[0xd1] == 4 && (g_ppu->bgmode & 7) == 1 &&
      (g_ppu->bgXsc[0] & 0xfc) == 0x50 && (g_ppu->bgXsc[1] & 0xfc) == 0x58) {
    if (fixed_arenas[arena].world_right > fixed_arenas[arena].world_left) {
      int left = fixed_arenas[arena].world_left;
      int right = fixed_arenas[arena].world_right;
      const int scroll = g_ppu->hScroll[0];
      /* An entirely in-room viewport already has genuine art everywhere. */
      if (scroll-(int)g_ppu->extraLeftCur >= left &&
          scroll+256+(int)g_ppu->extraRightCur <= right)
        return;
      /* BG2 contains the boss on a circular canvas. Keep the legitimate body
       * in the room's offscreen columns, but reject its wrapped copies. */
      PpuSetWidescreenLayerWorldClipBand(g_ppu,1,0,224,0,
                                        (uint16_t)left,(uint16_t)right);
      /* A native camera shake may briefly expose a few columns beyond the
       * authored wall. Retain that authentic view; only synthesize overflow.
       * Use the actual PPU scroll, including its raster/quake phase. */
      if (left > scroll) left = scroll;
      if (right < scroll+256) right = scroll+256;
      PpuSetWidescreenLayerWorldMirrorBand(g_ppu,0,0,224,
                                          (uint16_t)left,(uint16_t)right);
    } else {
      PpuSetWidescreenLayerElasticSplitBandSlot(g_ppu,0,0,0,224,0,1,255,256);
      PpuSetWidescreenLayerClamp(g_ppu,2);
    }
  }
}

/* X3's shared object-window family is the X2 implementation moved to bank
 * $02: $D58A activation, $D611 visibility, and $D636 draw. Generated-code
 * overrides route only structurally confirmed camera-X add/limit pairs (and
 * camera-X/dp$05 triggers) through these helpers. Y windows are untouched. */
/* Boss Rush stores unrelated arenas beside each other. Match X2's native
 * framing for isolated rematches; the hub retains its complete wide layout.
 * Hub teleports select checkpoints 3..8. Checkpoint 2 is the ordinary
 * corridor after the hub and keeps its scrolling widescreen presentation. */
bool X3UseNativeFrame(void) {
  /* The opening epilogue is an authored256px composition. Its background
   * canvases repeat the sun and factory when extended. The subsequent
   * scrolling cast (ending phase2) keeps adaptive widescreen. */
  return (g_ram[0xd1] == 6 && g_ram[0xd2] == 0) ||
         (g_ram[0x1fae] == 12 && g_ram[0xd1] == 4 &&
          g_ram[0x1fb5] >= 3 && g_ram[0x1fb5] <= 8);
}

static int x3_ws_spawn_margin(void) {
  static int s_enabled = -1;
  if (s_enabled < 0) {
    const char *e = getenv("SNESRECOMP_WS_SPAWN");
    s_enabled = (e && e[0] == '0') ? 0 : 1;
  }
  if (!s_enabled || !g_ws_active)
    return 0;
  return g_ws_extra + 32;
}

/* Resident type $04 owns parallax/scroll configuration, not enemy AI.
 * Its native cull window also owns its lifetime: keeping a previous room's
 * controller alive lets its Y-only crossing overwrite the current room.
 * In the factory lift this changes BG2 mode $10 to $02 at $03:F854. */
static bool x3_ws_native_controller(uint16_t dp) {
  return dp >= 0x1818 && dp < 0x1e18 &&
         ((dp - 0x1818) & 0x1f) == 0 && g_ram[dp + 0x0a] == 0x04;
}

uint16 X3WsObjWinAdd(uint16 base) {
  return (uint16)(base + (x3_ws_native_controller(g_cpu.D) ? 0 : x3_ws_spawn_margin()));
}

uint16 X3WsObjWinLimit(uint16 base) {
  return (uint16)(base + 2 * (x3_ws_native_controller(g_cpu.D) ? 0 : x3_ws_spawn_margin()));
}

/* Bit/Byte delay terrain collision until cameraY+$98/$78. A vertical
 * approach can leave the camera below the authored room origin, so that
 * first collision check happens after their feet have passed through the
 * floor. Use the original spawn record's world Y for that threshold. Keep
 * the original falling physics, terrain collision and encounter flags. */
uint16 X3HunterLandingCameraY(uint16 camera) {
  uint16_t dp = g_cpu.D;
  if (dp < 0xd18 || dp >= 0x1518 || ((dp - 0xd18) & 63)) return camera;
  uint8_t id = g_ram[dp + 10];
  if (id != 0x46 && id != 0x47) return camera;
  uint32_t first = g_ram[0xf800] | (g_ram[0xf801] << 8);
  uint32_t end = g_ram[0xfa00] | (g_ram[0xfa01] << 8);
  if (first != 0xfa02 || end < first || (end-first)%5) return camera;
  for (uint32_t p = first; p + 4 < end; p += 5) {
    uint16_t addr = g_ram[p+3] | (g_ram[p+4] << 8);
    if (addr < 0x8000 || addr > 0xfff9) continue;
    if ((cart_read(g_snes->cart,0x3c,addr) & 15) != 3 ||
        cart_read(g_snes->cart,0x3c,addr+3) != id) continue;
    uint16_t y = g_ram[p+1] | (g_ram[p+2] << 8);
    uint16_t x = cart_read(g_snes->cart,0x3c,addr+5) |
                 ((cart_read(g_snes->cart,0x3c,addr+6) & 31) << 8);
    uint16_t object_x = g_ram[dp+5] | (g_ram[dp+6] << 8);
    /* Both hunters must begin checking by recordY-8 (40px above the
     * authored floor). Bit's stock $98 gate is 32px later than Byte's $78
     * gate, so compensate that operand too; collision chooses the landing. */
    unsigned bias = id == 0x46 ? 160 : 128;
    if (abs((int)x-(int)object_x) <= 512 && y >= bias) return y - bias;
  }
  return camera;
}

static uint8_t *s_hunter_landing[2];
static bool s_hunter_landing_scanned;
static void x3_hunter_landing_hook(CpuState *cpu, uint32_t pc24) {
  unsigned index = pc24 == 0x07907d ? 0 : 1;
  if (!s_hunter_landing[index] || cpu->m_flag) return;
  uint16_t camera = cpu_read16(cpu,cpu->DB,0x1e60);
  uint16_t origin = X3HunterLandingCameraY(camera);
  /* Complete the proven 16-bit LDA absolute with the substituted operand.
   * Preserve its five slow-bus cycles, flags and original WRAM read. Keeping
   * ROM unchanged also avoids mutable instruction operands across state loads. */
  cpu_write_a_m(cpu,origin);
  cpu->_flag_Z = origin == 0;
  cpu->_flag_N = (origin & 0x8000) != 0;
  cpu_mirrors_to_p(cpu);
  cpu->cycles += 5;
  cpu->master_cycles += 5 * 8; /* Both signature sites are slow LoROM banks. */
  interp_bridge_pre_opcode_redirect(pc24 + 3);
}
static void x3_patch_hunter_landing(void) {
  static const uint32_t pcs[2] = {0x07907d,0x3cc588};
  if (!s_hunter_landing_scanned && g_snes && g_snes->cart && g_snes->cart->rom) {
    s_hunter_landing_scanned = true;
    for (unsigned i=0;i<2;i++) {
      uint32_t offset = ((pcs[i] >> 16) * 0x8000) + (pcs[i] & 0x7fff);
      if (offset < 2 || offset + 6 > g_snes->cart->romSize) continue;
      uint8_t *p = g_snes->cart->rom + offset;
      uint16_t add = i ? 0x78 : 0x98;
      if (p[-2]==0xc2 && p[-1]==0x21 && p[0]==0xad && p[1]==0x60 &&
          p[2]==0x1e && p[3]==0x69 && p[4]==add && p[5]==0) {
        s_hunter_landing[i] = p;
        interp_bridge_add_pre_opcode_hook(pcs[i],x3_hunter_landing_hook);
      } else fprintf(stderr,"[x3] hunter landing signature mismatch at $%06X\n",pcs[i]);
    }
  }
}

/* $00:DE3A streams scripted level records, not just ordinary actors. Widening
 * it can allocate progression records before X crosses the authentic playfield
 * boundary (the intro-stage ladder is one fatal example). Keep the vanilla
 * 4:3 frontier by default. SNESRECOMP_WS_STREAM=1 is a diagnostic opt-in for
 * the old widened behavior; resident object windows remain widescreen-aware. */
static int x3_ws_spawn_stream_enabled(void) {
  static int s_enabled = -1;
  if (s_enabled < 0) {
    const char *e = getenv("SNESRECOMP_WS_STREAM");
    s_enabled = e && e[0] == '1';
  }
  return s_enabled;
}

static uint16_t x3_ws_spawn_stream_left_extra(void) {
  int margin = x3_ws_spawn_stream_enabled() ? x3_ws_spawn_margin() : 0;
  return margin ? (uint16_t)((margin + 31) & ~31) : 0;
}

static uint16_t x3_ws_spawn_stream_right_extra(void) {
  int margin = x3_ws_spawn_stream_enabled() ? x3_ws_spawn_margin() : 0;
  return margin ? (uint16_t)((margin + 31) & ~31) : 0;
}

uint16 X3WsSpawnStreamLeft(uint16 camera) {
  return (uint16)(camera - x3_ws_spawn_stream_left_extra());
}

uint16 X3WsSpawnStreamRightAdd(uint16 base) {
  return (uint16)(base + x3_ws_spawn_stream_right_extra());
}

uint16 X3WsSpawnStreamGridPad(uint16 base) {
  uint16_t extra = x3_ws_spawn_stream_left_extra();
  return extra ? extra : base;
}

uint16 X3WsSpawnStreamColumns(uint16 base) {
  uint16_t left = x3_ws_spawn_stream_left_extra();
  uint16_t right = x3_ws_spawn_stream_right_extra();
  if (!left)
    return base;
  return (uint16)(9 + (left >> 5) + (right >> 5));
}

/* Private-ROM parity for interpreter and full-LLE DE3A execution. This is the
 * exact X3 relocation of X2's streamer. The equal-length SBC/NOP/JMP rewrite
 * rejoins at DE77's existing STA $00 with the original 12-cycle timing. */
typedef struct X3WsSpawnStreamRom {
  uint8_t *left_arm;
  uint8_t *right_operand;
  uint8_t *grid_operand;
  uint8_t *columns_operand;
} X3WsSpawnStreamRom;

static X3WsSpawnStreamRom s_x3_ws_stream_rom;
static bool s_x3_ws_stream_scanned;
static bool s_x3_ws_stream_valid;

static void x3_scan_ws_spawn_stream(void) {
  if (!g_snes || !g_snes->cart || !g_snes->cart->rom)
    return;

  uint8_t *rom = g_snes->cart->rom;
  size_t size = g_snes->cart->romSize;
  uint8_t *body = cart_getRomPtr(g_snes->cart, 0x00, 0xDE3A);
  static const uint8_t kEntry[] = {
      0x8B, 0x0B, 0x08, 0xC2, 0x30, 0xA9, 0x00, 0x00,
      0x5B, 0xAD, 0x7A, 0x1E, 0xCD, 0x5D, 0x1E, 0x30,
      0x16,
  };
  static const uint8_t kLeftState[] = {
      0xAD, 0x5D, 0x1E, 0x29, 0xE0, 0xFF, 0xC5, 0x00,
      0xF0, 0x30,
      0xAD, 0x5D, 0x1E, 0x85, 0x00, 0x80, 0x18,
  };
  static const uint8_t kRight[] = {
      0xAD, 0x5D, 0x1E, 0x18, 0x69, 0x00, 0x01, 0x85,
      0x00,
  };
  static const uint8_t kGrid[] = {
      0xAD, 0x5D, 0x1E, 0x38, 0xE9, 0x20, 0x00, 0x85,
      0x00, 0xA9, 0x0A, 0x00, 0x85, 0x06,
  };
  static const uint8_t kWalkerMask[] = {
      0xA5, 0x00, 0x29, 0xE0, 0xFF,
  };
  size_t body_offset = body ? (size_t)(body - rom) : size;
  bool valid =
      body && body_offset <= size && size - body_offset >= 0xA7 &&
      memcmp(body, kEntry, sizeof(kEntry)) == 0 &&
      memcmp(body + 0x16, kLeftState, sizeof(kLeftState)) == 0 &&
      memcmp(body + 0x36, kRight, sizeof(kRight)) == 0 &&
      memcmp(body + 0x78, kGrid, sizeof(kGrid)) == 0 &&
      memcmp(body + 0xA2, kWalkerMask, sizeof(kWalkerMask)) == 0;
  s_x3_ws_stream_scanned = true;
  s_x3_ws_stream_valid = valid;
  if (!valid) {
    fprintf(stderr,
            "[x3_ws] $00:DE3A spawn-stream signature mismatch; "
            "leaving its interpreter bytes unchanged\n");
    return;
  }
  s_x3_ws_stream_rom.left_arm = body + 0x20;
  s_x3_ws_stream_rom.right_operand = body + 0x3B;
  s_x3_ws_stream_rom.grid_operand = body + 0x7D;
  s_x3_ws_stream_rom.columns_operand = body + 0x82;

  const char *debug = getenv("SNESRECOMP_WS_SPAWN_DEBUG");
  if (debug && debug[0] != '0')
    fprintf(stderr, "[x3_ws] interpreter spawn-stream signature: exact\n");
}

static void x3_patch_ws_interp_spawn_stream(void) {
  if (!s_x3_ws_stream_scanned)
    x3_scan_ws_spawn_stream();
  if (!s_x3_ws_stream_scanned || !s_x3_ws_stream_valid)
    return;

  static const uint8_t kLeftOriginal[] = {
      0xAD, 0x5D, 0x1E, 0x85, 0x00, 0x80, 0x18,
  };
  uint16_t left = x3_ws_spawn_stream_left_extra();
  uint16_t right = x3_ws_spawn_stream_right_extra();
  if (!left) {
    memcpy(s_x3_ws_stream_rom.left_arm, kLeftOriginal,
           sizeof(kLeftOriginal));
  } else {
    uint16_t operand = (uint16_t)(left - 1);
    uint8_t patched[] = {
        0xE9, (uint8_t)operand, (uint8_t)(operand >> 8),
        0xEA, 0x4C, 0x77, 0xDE,
    };
    memcpy(s_x3_ws_stream_rom.left_arm, patched, sizeof(patched));
  }

  uint16_t right_add = (uint16_t)(0x100 + right);
  uint16_t grid_pad = left ? left : 0x20;
  uint16_t columns = X3WsSpawnStreamColumns(0x0A);
  s_x3_ws_stream_rom.right_operand[0] = (uint8_t)right_add;
  s_x3_ws_stream_rom.right_operand[1] = (uint8_t)(right_add >> 8);
  s_x3_ws_stream_rom.grid_operand[0] = (uint8_t)grid_pad;
  s_x3_ws_stream_rom.grid_operand[1] = (uint8_t)(grid_pad >> 8);
  s_x3_ws_stream_rom.columns_operand[0] = (uint8_t)columns;
  s_x3_ws_stream_rom.columns_operand[1] = (uint8_t)(columns >> 8);
}

/* X3 is LLE-first: non-emitted M/X variants, and some complete routines, run
 * directly from ROM. Generated-code injection alone therefore leaves whole
 * object classes at native-width activation. Scan the private cart ROM copy
 * for the same pair-confirmed horizontal windows and dp+$05 camera triggers,
 * then rewrite only their 16-bit immediate operands. The on-disk ROM is never
 * changed, and a zero margin restores every original value. */
typedef struct X3WsRomImm {
  uint8_t *operand;
  uint16_t base;
  uint8_t margin_scale;
} X3WsRomImm;

enum { kX3WsRomImmMax = 64, kX3WsRomImmExpected = 24 };
static X3WsRomImm s_x3_ws_rom_imms[kX3WsRomImmMax];
static int s_x3_ws_rom_imm_count;
static bool s_x3_ws_rom_scanned;
static bool s_x3_ws_rom_valid;

static uint16_t x3_ws_rom_u16(const uint8_t *p) {
  return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}

static bool x3_ws_rom_anchor_at(const uint8_t *p) {
  return (p[0] == 0xAD && p[1] == 0x5D && p[2] == 0x1E) ||
         (p[0] == 0xAD && p[1] == 0x60 && p[2] == 0x1E);
}

static bool x3_ws_trigger_add(uint16_t value) {
  switch (value) {
  case 0x20:
  case 0x40:
  case 0x60:
  case 0x80:
  case 0xA0:
  case 0xC0:
  case 0x100:
  case 0x110:
  case 0x120:
  case 0x140:
    return true;
  default:
    return false;
  }
}

static void x3_ws_add_rom_imm(uint8_t *operand, uint16_t base,
                              uint8_t margin_scale) {
  for (int i = 0; i < s_x3_ws_rom_imm_count; i++) {
    if (s_x3_ws_rom_imms[i].operand == operand)
      return;
  }
  if (s_x3_ws_rom_imm_count >= kX3WsRomImmMax) {
    static bool warned;
    if (!warned) {
      warned = true;
      fprintf(stderr, "[x3_ws] interpreter window-site table full\n");
    }
    return;
  }
  X3WsRomImm *site = &s_x3_ws_rom_imms[s_x3_ws_rom_imm_count++];
  site->operand = operand;
  site->base = base;
  site->margin_scale = margin_scale;
}

static void x3_scan_ws_rom_imms(void) {
  if (!g_snes || !g_snes->cart || !g_snes->cart->rom)
    return;

  uint8_t *rom = g_snes->cart->rom;
  size_t size = g_snes->cart->romSize;
  for (size_t i = 3; i + 64 < size; i++) {
    uint8_t *p = rom + i;

    /* Standard symmetric window:
     *   [SEC] LDA dp+$05 / [SEC] SBC $1E5D / CLC / ADC #add /
     *   CMP #($100 + 2*add), followed by the matching Y-axis check. */
    if (p[0] == 0xED && p[1] == 0x5D && p[2] == 0x1E &&
        p[3] == 0x18 && p[4] == 0x69 && p[7] == 0xC9) {
      bool object_x = (p[-2] == 0xA5 && p[-1] == 0x05) ||
                      (p[-3] == 0xA5 && p[-2] == 0x05 &&
                       p[-1] == 0x38);
      uint16_t add = x3_ws_rom_u16(p + 5);
      uint16_t limit = x3_ws_rom_u16(p + 8);
      if (object_x && add <= 0x140 &&
          limit == (uint16_t)(0x100 + 2 * add)) {
        x3_ws_add_rom_imm(p + 5, add, 1);
        x3_ws_add_rom_imm(p + 8, limit, 2);
      }
    }

    /* Bounding-box form used by $03:DDF3. */
    if (p[0] == 0xAD && p[1] == 0x5D && p[2] == 0x1E &&
        p[3] == 0x38 && p[4] == 0xE9 &&
        p[7] == 0x85 && p[8] == 0x08 &&
        p[9] == 0x18 && p[10] == 0x69 &&
        p[13] == 0x85 && p[14] == 0x06) {
      uint16_t add = x3_ws_rom_u16(p + 5);
      uint16_t limit = x3_ws_rom_u16(p + 11);
      if (add <= 0x140 && limit == (uint16_t)(0x100 + 2 * add)) {
        x3_ws_add_rom_imm(p + 5, add, 1);
        x3_ws_add_rom_imm(p + 11, limit, 2);
      }
    }

    /* Symmetric distance form:
     * camera+K - objectX + bias < limit. Both the leading offset and the
     * final limit must grow; changing K alone merely shifts the window. */
    if (p[0] == 0xAD && p[1] == 0x5D && p[2] == 0x1E &&
        p[3] == 0x69 && p[6] == 0x38 &&
        p[7] == 0xE5 && p[8] == 0x05 &&
        p[9] == 0x18 && p[10] == 0x69 && p[13] == 0xC9) {
      uint16_t add = x3_ws_rom_u16(p + 4);
      uint16_t limit = x3_ws_rom_u16(p + 14);
      if (add == 0x80 && limit == 0x1C0) {
        x3_ws_add_rom_imm(p + 4, add, 1);
        x3_ws_add_rom_imm(p + 14, limit, 2);
      }
    }

    /* Per-type wake/attack line: LDA $1E5D, ADC #K, then CMP dp+$05. */
    if (p[0] == 0xAD && p[1] == 0x5D && p[2] == 0x1E) {
      bool found = false;
      for (size_t j = i + 3; j < i + 48 && !found; j++) {
        if (x3_ws_rom_anchor_at(rom + j))
          break;
        if (rom[j] != 0x69)
          continue;
        uint16_t add = x3_ws_rom_u16(rom + j + 1);
        if (!x3_ws_trigger_add(add))
          continue;
        for (size_t k = j + 3; k < j + 33 && k < i + 56; k++) {
          if (x3_ws_rom_anchor_at(rom + k))
            break;
          if (rom[k] == 0xC5 && rom[k + 1] == 0x05) {
            x3_ws_add_rom_imm(rom + j + 1, add, 1);
            found = true;
            break;
          }
        }
      }
    }
  }

  s_x3_ws_rom_scanned = true;
  s_x3_ws_rom_valid = s_x3_ws_rom_imm_count == kX3WsRomImmExpected;
  if (!s_x3_ws_rom_valid) {
    fprintf(stderr,
            "[x3_ws] expected %d interpreter viewport immediates, found %d; "
            "leaving ROM constants unchanged because the layout may not match "
            "the supported revision\n",
            kX3WsRomImmExpected, s_x3_ws_rom_imm_count);
  }
  const char *debug = getenv("SNESRECOMP_WS_SPAWN_DEBUG");
  if (debug && debug[0] != '0') {
    fprintf(stderr, "[x3_ws] interpreter viewport immediate census: %d\n",
            s_x3_ws_rom_imm_count);
    for (int i = 0; i < s_x3_ws_rom_imm_count; i++) {
      const X3WsRomImm *site = &s_x3_ws_rom_imms[i];
      size_t offset = (size_t)(site->operand - rom);
      fprintf(stderr, "[x3_ws]   $%02X:%04X base=$%04X scale=%u\n",
              (unsigned)(offset >> 15),
              (unsigned)(0x8000 + (offset & 0x7FFF)),
              (unsigned)site->base, (unsigned)site->margin_scale);
    }
  }
}

/* Match the generated helpers when the shared activation/cull routine is
 * interpreted. Only its two X immediates change; the original guest body,
 * stack, flags, Y checks and record deletion still execute normally. */
static void x3_ws_activation_hook(CpuState *cpu, uint32_t pc24) {
  (void)pc24;
  if (!s_x3_ws_rom_valid) return;
  unsigned margin = x3_ws_native_controller(cpu->D) ? 0 : x3_ws_spawn_margin();
  for (int i = 0; i < s_x3_ws_rom_imm_count; ++i) {
    X3WsRomImm *site = &s_x3_ws_rom_imms[i];
    size_t offset = (size_t)(site->operand - g_snes->cart->rom);
    if (offset < 0x1558a || offset >= 0x15611) continue;
    uint16_t value = (uint16_t)(site->base + site->margin_scale * margin);
    site->operand[0] = (uint8_t)value;
    site->operand[1] = (uint8_t)(value >> 8);
  }
}

static void x3_patch_ws_interp_obj_windows(void) {
  if (!s_x3_ws_rom_scanned)
    x3_scan_ws_rom_imms();
  if (s_x3_ws_rom_scanned && s_x3_ws_rom_valid) {
    uint16_t margin = (uint16_t)x3_ws_spawn_margin();
    for (int i = 0; i < s_x3_ws_rom_imm_count; i++) {
      X3WsRomImm *site = &s_x3_ws_rom_imms[i];
      uint16_t value =
          (uint16_t)(site->base + site->margin_scale * margin);
      site->operand[0] = (uint8_t)value;
      site->operand[1] = (uint8_t)(value >> 8);
    }
  }
  if (s_x3_ws_rom_valid) {
    interp_bridge_add_pre_opcode_hook(0x02d58a, x3_ws_activation_hook);
    /* Also restore the correct operand context for a snapshot resumed inside
     * the helper; such a resume need not cross its entry hook again. */
    x3_ws_activation_hook(&g_cpu, 0x02d58a);
  }
  x3_patch_ws_interp_spawn_stream();
}

void X3DrawPpuFrame(void) {
  /* Presentation only. IRQs are serviced inside RunOneFrameOfGame while the
   * bridge advances the beam — never mutate g_cpu here. */
  SimpleHdma hdma_chans[8];
  dma_startDma(g_dma, g_snesrecomp_last_hdmaen, true);
  for (int ch = 0; ch < 8; ch++)
    SimpleHdma_Init(&hdma_chans[ch], &g_dma->channel[ch]);

  for (int line = 0; line <= 224; line++) {
    ppu_runLine(g_ppu, line);
    for (int ch = 0; ch < 8; ch++)
      SimpleHdma_DoLine(&hdma_chans[ch]);
  }
}

/* ── save-state extras: the LLE host cursor ───────────────────────────── */
enum { kX3LleSaveMagic = 0x78334C4Cu }; /* 'x','3',"LL" */

typedef struct X3LleSaveChunk {
  uint32_t magic;
  uint32_t resume_pc24;
  uint16_t A, X, Y, S, D;
  uint8_t DB, PB, P, m_flag, x_flag, emulation;
  uint8_t flag_N, flag_V, flag_Z, flag_C, flag_I, flag_D;
  uint64_t cycles;
  uint64_t master_cycles;
  uint32_t host_frames;
} X3LleSaveChunk;

void X3StateSaveExtra(SaveLoadInfo *sli) {
  X3LleSaveChunk c;
  memset(&c, 0, sizeof(c));
  c.magic = kX3LleSaveMagic + 2;
  c.resume_pc24 = s_lle_resume_pc;
  c.A = g_cpu.A; c.X = g_cpu.X; c.Y = g_cpu.Y; c.S = g_cpu.S; c.D = g_cpu.D;
  c.DB = g_cpu.DB; c.PB = g_cpu.PB; c.P = g_cpu.P;
  c.m_flag = g_cpu.m_flag; c.x_flag = g_cpu.x_flag;
  c.emulation = g_cpu.emulation;
  c.flag_N = g_cpu._flag_N; c.flag_V = g_cpu._flag_V;
  c.flag_Z = g_cpu._flag_Z; c.flag_C = g_cpu._flag_C;
  c.flag_I = g_cpu._flag_I; c.flag_D = g_cpu._flag_D;
  c.cycles = g_cpu.cycles;
  c.master_cycles = g_cpu.master_cycles;
  c.host_frames = (uint32_t)s_lle_host_frames;
  sli->func(sli, &c, sizeof(c));
  RtlSaveExecutionState(sli);
  sli->func(sli, s_cx4_oam_x, sizeof(s_cx4_oam_x));
  sli->func(sli, s_cx4_oam_valid, sizeof(s_cx4_oam_valid));
  sli->func(sli, g_ppu->wsOamFullX, sizeof(g_ppu->wsOamFullX));
  sli->func(sli, g_ppu->wsOamFullXValid, sizeof(g_ppu->wsOamFullXValid));
}

void X3StateLoadExtra(SaveLoadInfo *sli, uint32_t version) {
  (void)version;
  X3LleSaveChunk c;
  memset(&c, 0, sizeof(c));
  sli->func(sli, &c, sizeof(c));
  s_execution_loaded = false;
  if (c.magic < kX3LleSaveMagic || c.magic > kX3LleSaveMagic + 2) {
    fprintf(stderr,
            "[x3_rtl] save extra: bad magic $%08X — ignoring LLE chunk\n",
            (unsigned)c.magic);
    s_lle_extra_loaded = false;
    return;
  }
  g_cpu.A = c.A; g_cpu.X = c.X; g_cpu.Y = c.Y; g_cpu.S = c.S; g_cpu.D = c.D;
  g_cpu.DB = c.DB; g_cpu.PB = c.PB; g_cpu.P = c.P;
  g_cpu.m_flag = c.m_flag; g_cpu.x_flag = c.x_flag;
  g_cpu.emulation = c.emulation;
  g_cpu._flag_N = c.flag_N; g_cpu._flag_V = c.flag_V;
  g_cpu._flag_Z = c.flag_Z; g_cpu._flag_C = c.flag_C;
  g_cpu._flag_I = c.flag_I; g_cpu._flag_D = c.flag_D;
  g_cpu.host_return_valid = 0;
  g_cpu.cycles = c.cycles;
  g_cpu.master_cycles = c.master_cycles;
  g_cpu.ram = g_ram;
  s_lle_resume_pc = c.resume_pc24 & 0xFFFFFFu;
  s_lle_host_frames = c.host_frames ? c.host_frames : 1u;
  s_lle_extra_loaded = true;
  if (c.magic >= kX3LleSaveMagic + 1)
    s_execution_loaded = RtlLoadExecutionState(sli);
  memset(s_cx4_oam_valid, 0, sizeof(s_cx4_oam_valid));
  memset(g_ppu->wsOamFullXValid, 0, sizeof(g_ppu->wsOamFullXValid));
  if (c.magic == kX3LleSaveMagic + 2) {
    sli->func(sli, s_cx4_oam_x, sizeof(s_cx4_oam_x));
    sli->func(sli, s_cx4_oam_valid, sizeof(s_cx4_oam_valid));
    sli->func(sli, g_ppu->wsOamFullX, sizeof(g_ppu->wsOamFullX));
    sli->func(sli, g_ppu->wsOamFullXValid, sizeof(g_ppu->wsOamFullXValid));
  }
  fprintf(stderr, "[x3_rtl] LLE load extra resume=$%06X master=%llu\n",
          (unsigned)s_lle_resume_pc,
          (unsigned long long)g_cpu.master_cycles);
}

void X3OnStateLoaded(uint32_t version) {
  MmxReplayStateLoaded();
  (void)version;
  /* Renderer history belongs to the abandoned timeline, not this snapshot. */
  WsShadowReset();
  if (s_execution_loaded) RtlApplyExecutionState();
  s_execution_loaded = false;
  if (s_lle_extra_loaded) {
    s_lle_did_reset = true; /* resume mid-game, skip cold RESET */
  } else {
    /* Snapshot carries WRAM/PPU but no LLE cursor — cold boot rather than
     * splice mid-game WRAM onto a RESET PC. */
    s_lle_did_reset = false;
    s_lle_resume_pc = 0;
    s_lle_host_frames = 0;
    fprintf(stderr,
            "[x3_rtl] state loaded without LLE chunk — cold booting\n");
  }
  s_lle_extra_loaded = false;
}
