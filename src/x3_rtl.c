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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_cpu_infra.h"
#include "cpu_state.h"
#include "cpu_trace.h"
#include "snes/cart.h"
#include "snes/interp_bridge.h"
#include "snes/snes.h"
#include "snes/dma.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include "snes/ws_shadow.h"
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
static void x3_patch_ws_interp_obj_windows(void);

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
    const char *e = getenv("SNESRECOMP_X3_BOOTLOG");
    v = (e && e[0] == '0') ? 0 : 1;
  }
  return v;
}

void RunOneFrameOfGame(void) {
  /* Generated bodies route viewport constants through X3WsObjWin*.
   * Interpreter fallbacks fetch the original ROM bytes, so mirror those
   * dynamic constants into the private runtime ROM copy each frame. */
  x3_patch_ws_interp_obj_windows();

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
  kX3HudSlotFirst = 0,        /* HP / weapon / boss all live in 0..23 */
  kX3HudSlotCount = 24,       /* slot 24 onward is actors            */
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

/* Call once per frame from the host's frame-prep, after g_ws_extra is known. */
void X3ConfigureWsHud(void) {
  extern bool g_ws_active;
  if (!g_ppu) return;
  /* X3's HUD is OAM-only; BG3 is gameplay dialogue and must stay centered. */
  PpuSetWidescreenHudSplit(g_ppu, 0, 0, 0);
  if (!g_ws_active || !x3_ws_hud_present()) {
    PpuSetWsHudOamShiftRange(g_ppu, 0, 0);   /* off = authentic placement */
    return;
  }
  PpuSetWsHudOamBand(g_ppu, kX3HudBandHeight,
                     kX3HudLeftEnd, kX3HudRightStart);
  PpuSetWsHudOamShiftRange(g_ppu, kX3HudSlotFirst,
                           kX3HudSlotCount);
}

/* ── 16:9 BG margins: exact fill from X3's level structures ──────────────
 *
 * X3 retained X2's two-layer column streamer. The corresponding X3 routines
 * are $00:B91B (composer), $00:BC60 (address/layout derivation), and
 * $00:BC9E/$00:BCD5 (BG1/BG2 source setup). Their external data layout is
 * unchanged:
 *
 *          layout     screenDefs   metatile ptr    world anchor
 *   BG1    $7E:E800   $7E:2800     24-bit [$09C5]  $1E5D / $1E60
 *   BG2    $7E:EC00   $7E:A600     24-bit [$09C8]  $1E9D / $1EA0
 *
 * Live validation on save slot 0 reproduced all sampled native VRAM entries
 * on both layers before this provider was enabled. As in X2, the native view
 * is never modified; only gutter lookups are replaced, and only after a
 * per-frame native-view self-check proves that the scene uses this layout.
 */
typedef struct X3BgStream {
  uint16_t layoutBase;
  uint16_t screenDefs;
  uint16_t mtPtrAddr;
  uint16_t worldXAddr;
  uint16_t worldYAddr;
  uint8_t bgsc;
  uint16_t mapBaseWord;
} X3BgStream;

static const X3BgStream kX3BgStreams[2] = {
    {0xE800, 0x2800, 0x09C5, 0x1E5D, 0x1E60, 0x51, 0x5000},
    {0xEC00, 0xA600, 0x09C8, 0x1E9D, 0x1EA0, 0x59, 0x5800},
};

static uint16_t x3_wram16(uint32_t addr) {
  return (uint16_t)(g_ram[addr] | (g_ram[addr + 1] << 8));
}

static uint16_t x3_bg_world_tile(const X3BgStream *s, uint32_t px,
                                 uint32_t py) {
  uint32_t li = (((py >> 8) & 0x1F) << 5) | ((px >> 8) & 0x1F);
  uint8_t sid = g_ram[s->layoutBase + li];
  uint16_t ba = (uint16_t)(s->screenDefs + sid * 512u +
                           ((py >> 4) & 0xF) * 0x20u +
                           ((px >> 4) & 0xF) * 2u);
  uint16_t block =
      (uint16_t)(g_ram[ba] | (g_ram[(uint16_t)(ba + 1)] << 8));
  uint16_t mt_addr = x3_wram16(s->mtPtrAddr);
  uint8_t mt_bank = g_ram[s->mtPtrAddr + 2];
  /* Match the guest's fixed-bank, 16-bit pointer wrap at $00:B9C0. */
  uint16_t a =
      (uint16_t)(mt_addr + block * 8u + (((py >> 3) & 1u) << 2) +
                 (((px >> 3) & 1u) << 1));
  return (uint16_t)(cart_read(g_snes->cart, mt_bank, a) |
                    (cart_read(g_snes->cart, mt_bank,
                               (uint16_t)(a + 1)) << 8));
}

static uint16_t x3_bg_vram_word(const X3BgStream *s, uint32_t px,
                                uint32_t py) {
  uint32_t tx = px >> 3, ty = py >> 3;
  return (uint16_t)(s->mapBaseWord + ((tx >> 5) & 1u) * 0x400u +
                    (ty & 0x1F) * 0x20u + (tx & 0x1F));
}

static bool x3_bg_stream_valid(const X3BgStream *s, int32_t wx, int32_t wy) {
  if (wx < 0 || wy < 0)
    return false;
  /* Boot/menu frames can already have the gameplay BG mode/map bases while
   * the stream-source cluster is still zero. Reject before cart_read so an
   * uninitialized pointer cannot create a false off-rails diagnostic. */
  if (x3_wram16(s->mtPtrAddr) < 0x8000)
    return false;
  int miss = 0, modal = 0;
  uint16_t modal_tile = 0;
  uint16_t got_v[12];
  for (int i = 0; i < 12; i++) {
    uint32_t px = (uint32_t)wx + 10u + (uint32_t)i * 20u;
    uint32_t py = (uint32_t)wy + 12u + (uint32_t)(i % 6) * 36u;
    uint16_t want = x3_bg_world_tile(s, px, py);
    uint16_t got = g_ppu->vram[x3_bg_vram_word(s, px, py) & 0x7FFF];
    got_v[i] = got;
    if (want != got)
      miss++;
  }
  if (miss > 1)
    return false;
  /* A near-uniform sample cannot prove that a layer is level data. It may
   * instead be a dynamic object layer, which authentic map wrap preserves. */
  for (int i = 0; i < 12; i++) {
    int same = 0;
    for (int j = 0; j < 12; j++)
      same += (got_v[j] == got_v[i]);
    if (same > modal) {
      modal = same;
      modal_tile = got_v[i];
    }
  }
  if (modal <= 9)
    return true;

  /*
   * A sparse level screen can legitimately make the quick sample almost
   * uniform.  This happens throughout the slot-8 vertical fall: BG1 is
   * mostly transparent until the terrain scrolls up, so the old diversity
   * gate disabled exact gutters for roughly 35 frames and exposed wrapped
   * stale rows at the right edge.
   *
   * Preserve the object-layer safeguard by looking for positive evidence,
   * rather than merely accepting a uniform match.  Scan the native view for
   * six source tiles that differ from the modal native entry and require
   * each one to reproduce VRAM.  A repurposed object layer whose level
   * source is blank finds no proof; one whose live objects disagree with
   * that source fails the comparisons.  A genuine sparse level map proves
   * itself as soon as its off-air terrain entries agree.
   */
  int proof = 0, proof_miss = 0;
  for (uint32_t y = 4; y < 224; y += 8) {
    for (uint32_t x = 4; x < 256; x += 8) {
      uint32_t px = (uint32_t)wx + x;
      uint32_t py = (uint32_t)wy + y;
      uint16_t want = x3_bg_world_tile(s, px, py);
      if (want == modal_tile)
        continue;
      uint16_t got =
          g_ppu->vram[x3_bg_vram_word(s, px, py) & 0x7FFF];
      if (want == got) {
        if (++proof >= 6)
          return true;
      } else if (++proof_miss > 1) {
        return false;
      }
    }
  }
  return false;
}

void X3ConfigureWsBgMargins(void) {
  static int s_enabled = -1, s_debug = -1;
  static bool s_was_active;
  if (s_enabled < 0) {
    const char *e = getenv("SNESRECOMP_WS_BG_MARGINS");
    s_enabled = (e && e[0] == '0') ? 0 : 1;
    e = getenv("SNESRECOMP_WS_BG_MARGINS_DEBUG");
    s_debug = (e && e[0] && e[0] != '0') ? 1 : 0;
  }

  bool scene_ok = s_enabled && g_ws_active && g_ppu &&
                  (g_ppu->bgmode & 0x37) == 1 &&
                  g_ppu->bgXsc[0] == kX3BgStreams[0].bgsc &&
                  g_ppu->bgXsc[1] == kX3BgStreams[1].bgsc;
  if (s_debug) {
    static unsigned s_calls;
    if ((s_calls++ % 120) == 0) {
      fprintf(stderr,
              "[x3_ws_bg] call=%u en=%d ws=%d bgmode=%u bgXsc=%02X/%02X "
              "scene_ok=%d extra=%d\n",
              s_calls, s_enabled, (int)g_ws_active,
              g_ppu ? g_ppu->bgmode : 0xFF,
              g_ppu ? g_ppu->bgXsc[0] : 0xFF,
              g_ppu ? g_ppu->bgXsc[1] : 0xFF,
              (int)scene_ok, g_ws_extra);
    }
  }

  bool ok[2] = {false, false};
  int32_t wxp[2] = {0, 0}, wyp[2] = {0, 0};
  bool any = false;
  if (scene_ok) {
    for (int l = 0; l < 2; l++) {
      const X3BgStream *s = &kX3BgStreams[l];
      uint16_t h = (uint16_t)(g_ppu->hScroll[l] & 0x3FF);
      uint16_t v = (uint16_t)(g_ppu->vScroll[l] & 0x3FF);
      int32_t wx = (int32_t)x3_wram16(s->worldXAddr);
      int32_t wy = (int32_t)x3_wram16(s->worldYAddr);
      int32_t dh = (int32_t)((uint16_t)(h - wx) & 0x3FF);
      int32_t dv = (int32_t)((uint16_t)(v - wy) & 0x3FF);
      if (dh >= 512) dh -= 1024;
      if (dv >= 512) dv -= 1024;
      wx += dh;
      wy += dv;
      if (!x3_bg_stream_valid(s, wx, wy)) {
        if (s_debug) {
          static unsigned s_fail[2];
          if ((s_fail[l]++ % 120) == 0)
            fprintf(stderr,
                    "[x3_ws_bg] L%d validation fail #%u wx=%d wy=%d "
                    "h=%u v=%u anchor=(%u,%u)\n",
                    l, s_fail[l], wx, wy, h, v,
                    x3_wram16(s->worldXAddr),
                    x3_wram16(s->worldYAddr));
        }
        continue;
      }
      ok[l] = true;
      any = true;
      wxp[l] = wx;
      wyp[l] = wy;
      WsShadowSetWorld(l, (uint32_t)wx, (uint32_t)wy);
      WsShadowSetBlankTile(l, -1);
      WsShadowSetRespectGameWrites(l, 60);
    }
  }

  if (!any) {
    if (s_was_active)
      WsShadowReset();
    s_was_active = false;
    WsShadowFrame(g_ppu);
    return;
  }
  s_was_active = true;
  WsShadowFrame(g_ppu);

  int margin = (g_ws_extra + 7) & ~7;
  for (int l = 0; l < 2; l++) {
    if (!ok[l])
      continue;
    const X3BgStream *s = &kX3BgStreams[l];
    const int32_t tx_rng[2][2] = {
        {(wxp[l] - margin) >> 3, (wxp[l] - 1) >> 3},
        {(wxp[l] + 256) >> 3, (wxp[l] + 255 + margin) >> 3},
    };
    int32_t ty0 = (wyp[l] - 256) >> 3;
    int32_t ty1 = (wyp[l] + 491) >> 3;
    if (ty0 < 0)
      ty0 = 0;
    for (int r = 0; r < 2; r++) {
      for (int32_t tx = tx_rng[r][0]; tx <= tx_rng[r][1]; tx++) {
        if (tx < 0)
          continue;
        for (int32_t ty = ty0; ty <= ty1; ty++) {
          WsShadowForceTile(l, (uint32_t)tx, (uint32_t)ty,
                            x3_bg_world_tile(s, (uint32_t)tx << 3,
                                             (uint32_t)ty << 3));
        }
      }
    }
  }
}

/* X3's shared object-window family is the X2 implementation moved to bank
 * $02: $D58A activation, $D611 visibility, and $D636 draw. Generated-code
 * overrides route only structurally confirmed camera-X add/limit pairs (and
 * camera-X/dp$05 triggers) through these helpers. Y windows are untouched. */
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

uint16 X3WsObjWinAdd(uint16 base) {
  return (uint16)(base + x3_ws_spawn_margin());
}

uint16 X3WsObjWinLimit(uint16 base) {
  return (uint16)(base + 2 * x3_ws_spawn_margin());
}

/* $00:DE3A is X3's copy of X2's 32-pixel dynamic record streamer.
 * Keep its probes beyond the visible margin plus wake slack. The two sides
 * round in opposite directions because the right base is already camera+$100
 * while the left sweep starts at camera-$20. */
static uint16_t x3_ws_spawn_stream_left_extra(void) {
  int margin = x3_ws_spawn_margin();
  return margin ? (uint16_t)((margin + 31) & ~31) : 0;
}

static uint16_t x3_ws_spawn_stream_right_extra(void) {
  int margin = x3_ws_spawn_margin();
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
  c.magic = kX3LleSaveMagic;
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
}

void X3StateLoadExtra(SaveLoadInfo *sli, uint32_t version) {
  (void)version;
  X3LleSaveChunk c;
  memset(&c, 0, sizeof(c));
  sli->func(sli, &c, sizeof(c));
  if (c.magic != kX3LleSaveMagic) {
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
  fprintf(stderr, "[x3_rtl] LLE load extra resume=$%06X master=%llu\n",
          (unsigned)s_lle_resume_pc,
          (unsigned long long)g_cpu.master_cycles);
}

void X3OnStateLoaded(uint32_t version) {
  (void)version;
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
