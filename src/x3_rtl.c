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
#include <string.h>

#include "common_cpu_infra.h"
#include "cpu_state.h"
#include "cpu_trace.h"
#include "snes/interp_bridge.h"
#include "snes/snes.h"
#include "snes/dma.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
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
 * PROVENANCE (2026-07-26): this slot map is INHERITED FROM MEGA MAN X2 and has
 * NOT been surveyed on Mega Man X3. X3 has no docs/OAM_SURVEY.md; every
 * constant below (tile 0x86, attr 0x34/0x36, X 8/24/232, the 0..23 band) is an
 * X2 measurement assumed to carry over because the two games share a HUD
 * layout. That assumption is UNVERIFIED here.
 *
 * It is safe to ship unverified only because it fails closed: if any constant
 * is wrong for X3 the signature simply never matches, x3_ws_hud_present()
 * returns 0, and the HUD keeps its authentic 4:3 placement. Widescreen is also
 * off by default for this title. Do NOT promote X3 widescreen on the strength
 * of this block -- run the survey first and write docs/OAM_SURVEY.md.
 *
 * Inherited X2 slot map:
 *
 *   HP bar      slots  0-5   screen X 8     attr 0x34   anchors LEFT
 *   weapon bar  slots  7-13  screen X 24    attr 0x36   anchors LEFT
 *   boss bar    slots 16-22  screen X 232   attr 0x34   anchors RIGHT
 *   actors      slots 24+
 *
 * The bars anchor to OPPOSITE edges, so they cannot be shifted as one block.
 * PpuAdjustWidescreenHudOamX already handles that: it pushes sprites left of
 * wsHudLeftEnd outward by extraLeftCur and sprites at/after wsHudRightStart
 * outward by extraRightCur. So this only has to configure it correctly.
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
   * of the stage. (That tolerance is an X2 finding -- requiring all of 0-5
   * there evaluated 5/6 and disabled the shift nearly everywhere. Carried over
   * with the rest of the inherited map; not re-measured on X3.) */
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
  if (!g_ws_active || !x3_ws_hud_present()) {
    PpuSetWsHudOamShiftRange(g_ppu, 0, 0);   /* off = authentic placement */
    PpuSetWidescreenHudSplit(g_ppu, 0, 0, 0);
    return;
  }
  PpuSetWidescreenHudSplit(g_ppu, kX3HudBandHeight,
                           kX3HudLeftEnd, kX3HudRightStart);
  PpuSetWsHudOamShiftRange(g_ppu, kX3HudSlotFirst,
                           kX3HudSlotCount);
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
