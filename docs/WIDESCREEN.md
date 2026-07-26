# 16:9 widescreen — Mega Man X3

## Ground floor is faithful 4:3. This is an opt-in enhancement.

The shipped default is native 256x224. `Widescreen` and `NoSpriteLimits` are
both **0** in the default config (see the embedded default in `src/main.c`, which
is what actually writes `config.ini` on first run), and the launcher's widescreen
toggle is **hidden** (`gi.widescreen_supported = 0` in `src/main.c`).

Do not turn any of that on until the work below is done. A previous iteration
shipped `Widescreen = 1` while every widescreen hook was inert — 16:9 was on with
nothing adapted to it, which is worse than not offering it.

## What "done" requires

16:9 is wanted, but only done properly. All four of these, not a subset:

### 1. Background scrolling on EVERY layer
Each BG layer has to scroll correctly across the widened viewport, not just the
one that happened to be surveyed. Per-layer, verify:
- the tilemap is populated for the added margin columns before they are shown
  (first-visit margins must not display stale or wrapped tiles);
- per-line scroll registers are the authority for pixel phase, NOT a WRAM camera
  mirror — a mirror is off by one against the PPU and produces the "feature cut
  in half at the margin" artefact;
- layers driven by HDMA keep their per-scanline effects across the margins;
- parallax/periodic layers fold correctly rather than repeating a stale window.

### 2. HUD anchored to 16:9 bounds
Health/weapon/boss bars are OBJ sprites positioned against the native 256px
frame. They must anchor to the widened edges instead of floating inward, and only
during real gameplay — menus, intros and mode-7 scenes must keep native placement.
Gate on a verified game-state discriminator, not on an HDMA-enable mirror.

### 3. Enemy spawning respects 16:9 bounds
Enemies must spawn while still off the WIDENED screen, or they pop into
existence inside the added margins. Spawn triggers keyed to the native frame edge
have to be widened by the same margin. Keep stage/room controllers on authentic
4:3 timing so progression and RNG are unchanged.

### 4. Enemy culling respects 16:9 bounds
The mirror of (3): despawn logic keyed to the native edge deletes enemies that are
still visible in the margins. Shots, effects and OAM write limits all need the
same treatment — miss one and objects blink out mid-margin.

## Non-negotiables

- **Camera, collision, AI, RNG and save-state data must be unchanged.** 16:9 is a
  presentation and spawn/cull-bounds change. If it alters simulation, it is wrong.
- **4:3 must remain bit-identical with the enhancement off.** That is the
  regression gate: capture frames at `Widescreen = 0` before and after any
  widescreen work and diff them.
- Stage-trigger lead must not exceed the margin, or CHR paging garbles.

## Prior art to draw on, carefully

Mega Man X 1 has a surveyed, shipping 16:9 (`MegamanXRecomp`): CULL / SPAWN /
STAGE / SHADOW hooks, a BG2 layer path, HUD anchoring and margin prefill. The
mechanism generalises; **the addresses do not.** MMX1's version reads its retained
level map at `$EC00`/`$A600` and gates on `$00D1`/`$00D2` — all MMX1 facts. Those
were deliberately NOT carried into this project. Survey Mega Man X3's own streamer,
camera and spawn tables first; reuse the shapes, never the constants.

## Turning it on

Only after 1-4 are done and the 4:3 regression gate passes:
1. `gi.widescreen_supported = 1` in `src/main.c`;
2. flip the embedded default if 16:9 should be the out-of-box experience
   (`Widescreen = 1` there) — otherwise leave the default faithful and let the
   launcher toggle expose it;
3. record what was surveyed in this file so the next person knows what is proven.
