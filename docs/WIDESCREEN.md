# 16:9 widescreen — Mega Man X3

## Status: NOT IMPLEMENTED. Ground floor is faithful 4:3.

The shipped default is native 256x224. `Widescreen` and `NoSpriteLimits` are both
**0** in the embedded default inside `src/main.c` (that string, not the repo-root
`config.ini`, is what a fresh run writes and reads), and the launcher toggle is
hidden (`gi.widescreen_supported = 0`).

An earlier iteration shipped `Widescreen = 1` while every hook was inert — 16:9 on
with nothing adapted to it. Do not flip any of this back on until the four parts
below are done and the 4:3 regression gate passes.

## Mega Man X 1 is the reference — the SHAPES port, the ADDRESSES do not

`MegamanXRecomp` has a working, surveyed 16:9. Its hooks live in
`src/mmx_rtl.c` and are injected into the *generated* C by
`tools/apply_overrides.py`, which pattern-matches emitted code at specific
ROM-derived sites and rewrites a value or a flag in place:

```
/*WS-CULL*/  { cpu->_flag_C = MmxWsCullVerdictX((uint16)(_v12)); }
/*WS-OAM*/   { _v7 = MmxWsOamRightLimit(_v7); }
```

Every MMX1 site is an MMX1 fact: `bank_02_806E` (enemy cull), `bank_82_80B4`
(shot cull), `bank_00_DC36`/`DCDB` (spawn scan + record walk), `bank_00_D76A`
(metasprite X gate), `bank_82_B964` (enemy activation), `bank_03_FDD3` (camera
line triggers), and WRAM `$00D1`/`$00D2` (gameplay gate), `$1E4D` (scan anchor),
`$0BAD` (camera X). **None of those transfer.** Survey Mega Man X3's own routines
first; reuse the formulas below, never the constants.

## The four parts

### 1. Background scrolling on EVERY layer

Per layer, and it must be every layer the game uses, not just the surveyed one:

* The added margin columns must be **populated before they are shown**, or
  first-visit margins display stale or wrapped tiles. MMX1 does this by reading
  the game's own retained level map to seed the margin (`WsShadowPrefillTile`).
* **Per-line PPU scroll registers are the authority for pixel phase — never a
  WRAM camera mirror.** The mirror is off by one against the PPU and produces the
  "feature sliced in half at the margin" artefact. This bit MMX1 once and is
  worth not repeating.
* HDMA-driven per-scanline effects must continue across the margins.
* Periodic/parallax layers should **fold** by their proven period
  (`WsShadowSetPeriodicFold`) rather than serving stale history.
* Stage-trigger lead must not exceed the margin, or CHR paging garbles.

Renderer-side surface already available, no per-game code needed to *call* it:
`PpuSetExtraSpace`, `PpuSetWidescreenBg3Widen`, `PpuSetWidescreenLineEnhancer`,
`WsShadowFrame` / `WsShadowSetWorld` / `WsShadowSetPeriodicFold` /
`WsShadowPrefillTile`.

### 2. HUD anchored to 16:9 bounds

MMX1 reserves OAM slots 0-15 for HUD sprites and shifts them outward with one
renderer call, gated on real gameplay:

```c
bool in_stage = g_ws_active && <game-state discriminator>;
PpuSetWsHudOamShift(g_ppu, in_stage ? 16 : 0);
```

Two requirements:
* Gate on a **verified game-state discriminator**, not on an HDMA-enable mirror.
  MMX1 originally gated partly on `$00C3` (an HDMAEN mirror) and the HUD snapped
  back to native placement during any effect that toggled HDMA channels.
* Menus, intros and mode-7 scenes must keep native placement.

Survey needed: which OAM slots Mega Man X3 uses for HUD, and which WRAM byte
reliably means "in live stage gameplay".

### 3. Enemy spawning respects 16:9 bounds

Spawn scanning is anchored to a camera column. Widen the anchor so enemies enter
the world before the widescreen edge reveals them:

```
right anchor:  v + (margin + 32)
left  anchor:  max(0, v - (margin + 32))
```

The `+32` matters: an anchor of exactly the margin lands spawns on the outermost
*visible* wide column, i.e. visible pop-in.

**The dual-pass trick is the important part.** Widening the anchor for everything
makes stage controllers, camera staging and minibosses fire early. MMX1 runs the
record walk twice: the widened pass admits **only ordinary enemy records**, then a
second pass at the **unmodified 4:3 anchor** admits everything else, so
progression-critical records keep authentic timing. Per-record flags make an
already-created enemy a no-op in the native pass.

```c
int WsSpawnRecordAllowed(uint16 dpage, uint8 type);   /* wide pass: type only */
void WsSpawnRunNativePass(CpuState *cpu);             /* balanced synthetic JSR */
```

The native pass must be a **balanced** call that preserves all guest registers and
cycle accounting (`cpu_dispatch_call_pc`, save/restore `CpuState`), or it corrupts
the stack.

Survey needed: the spawn-scan routine, the record-descriptor type nibble, and
which types are ordinary enemies versus controllers.

### 4. Enemy culling respects 16:9 bounds

The mirror of (3) — cull keyed to the native edge deletes things still visible in
the margins. Widen the scroll-off verdict symmetrically:

```
vanilla:  carry = (objX - camX + 0x40) >= 0x180          /* keep cam-64..+320 */
widened:  carry = (v + margin) >= (0x180 + 2*margin)
```

Projectiles use the same shape with the game's own tighter base window (MMX1:
`0x20` / `0x140`). Then the OAM emitter, which is a separate gate and easy to
miss:

* the metasprite X **reject limit** must be widened (`vanilla_limit + margin`);
* the reject **compare** must be replaced, because it is a single *unsigned*
  test — negative screen X wraps high and always rejects, so sprites still vanish
  at the native left edge no matter how far the limit is widened:

```c
uint16 WsOamXReject(uint16 x_plus_16, uint16 widened_limit) {
  if (x_plus_16 < widened_limit) return 0;               /* right window */
  if (m && x_plus_16 >= (uint16)(0u - (uint16)m)) return 0; /* left margin */
  return 1;
}
```

Also widen per-enemy **activation distance** for large objects, or a big sprite's
controller only wakes when its centre reaches the widened edge and its outer tiles
pop in.

## Non-negotiables

* **Camera, collision, AI, RNG and save-state data unchanged.** 16:9 is
  presentation plus spawn/cull bounds. If it alters simulation, it is wrong.
* **4:3 must stay bit-identical with the enhancement off.** That is the
  regression gate: capture frames at `Widescreen = 0` before and after, and diff.
  The engine has `PPU frame-diff` tooling for exactly this.
* Every widening gets its own env kill-switch (MMX1: `SNESRECOMP_WS_SPAWN`,
  `SNESRECOMP_WS_STAGE`) so a misbehaving part can fall back to authentic 4:3
  independently, with the rest still active.

## Survey plan — what to measure, with what

Nothing here can be written without Mega Man X3's addresses. All of these are
always-on rings; query them, do not arm-then-run:

| question | tool |
|---|---|
| which routines write OAM, and the HUD slot range | `oam_write_get`, `oam_render_get` |
| the live gameplay-state discriminator | `read_ram`, `set_wram_watch` across mode changes |
| camera X location in WRAM | `trace_wram` while scrolling |
| which code culls/spawns | `SNESRECOMP_WRITE_WATCH` on an object slot, then the reported function |
| per-layer scroll authority | `get_ppu_state` (`hScroll`/`vScroll`) vs the WRAM mirror |

Reaching live gameplay requires a human at the controls — the standing rule is
that gameplay verdicts are the owner's, not the agent's.

## Turning it on

Only after 1-4 are done and the 4:3 gate passes:
1. `gi.widescreen_supported = 1` in `src/main.c`;
2. optionally flip the embedded default's `Widescreen` to 1;
3. record what was surveyed **in this file**, so the next person knows what is
   proven versus assumed.
