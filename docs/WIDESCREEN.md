# Mega Man X3 16:9 widescreen

## Status

Implemented for Mega Man X3 (USA):

- true 342x224 rendering at 16:9;
- player/weapon HUD anchored to the left and boss HUD anchored to the right;
- exact BG1 and BG2 tiles in newly visible columns, including while scrolling;
- widescreen-aware enemy activation, visibility, drawing, and camera triggers;
- unchanged 4:3 output when the feature is disabled.

The launcher exposes the widescreen option. The embedded default remains
`Widescreen = 0`, so a new install starts in authentic 4:3 until selected.

## Mega Man X2 correspondence

X3 retained the relevant X2 engine structures with relocated code. The X2
implementation and `docs/OAM_SURVEY.md` were used as the structural reference;
every X3 address and live data source below was then verified against X3.

### Background streamers

| Purpose | X3 address |
|---|---:|
| tile composer | `$00:B91B` |
| world/layout derivation | `$00:BC60` |
| BG1 source setup | `$00:BC9E` |
| BG2 source setup | `$00:BCD5` |

The retained level-map inputs are:

| Layer | layout | screen definitions | metatile pointer | world X/Y | BGSC | VRAM map |
|---|---:|---:|---:|---:|---:|---:|
| BG1 | `$7E:E800` | `$7E:2800` | `[$09C5]` | `$1E5D/$1E60` | `$51` | `$5000` |
| BG2 | `$7E:EC00` | `$7E:A600` | `[$09C8]` | `$1E9D/$1EA0` | `$59` | `$5800` |

`X3ConfigureWsBgMargins` reconstructs only gutter tiles from these structures.
Before enabling a layer each frame, it compares 12 reconstructed native-view
samples against VRAM, permits at most one mismatch, and rejects uninitialized
or near-uniform sources. This makes menus and unrelated layer modes fail safe
to authentic PPU wrapping. Native-view tiles are never replaced.

Game-authored gutter writes take priority for 60 frames, matching X2's
stream-transition handling. `SNESRECOMP_WS_BG_MARGINS=0` disables this provider;
`SNESRECOMP_WS_BG_MARGINS_DEBUG=1` logs its validation state.

### HUD

Live X3 OAM on save slot 0 confirms the X2 layout:

- slots 0-5: player health, X=8;
- slots 7-13: weapon health, X=24;
- slots 16-22: boss health, X=232.

The player and weapon bars shift toward the widescreen left edge; the boss bar
shifts toward the right edge. The shift is enabled only when the measured
player-health signature is present (slot 0 tile `$86`, attribute `$34`, plus at
least four matching frame slots). Cutscenes that reuse slots 0-23 therefore
remain unshifted.

### Object activation, visibility, and culling

X3's shared object-window family is the relocated X2 implementation:

| X3 routine | vanilla horizontal test |
|---|---|
| `$02:D58A` activation | `(objX - camX + $40) < $180` |
| `$02:D611` visibility | `(objX - camX + $60) < $1C0` |
| `$02:D636` draw/common AI tail | `(objX - camX + $20) < $140` |

`tools/apply_overrides.py` recognizes generated C structurally: a read of the
X camera anchor `$1E5D`, followed by a complete add/limit pair. A `$1E60` read
disarms the match so vertical windows remain unchanged. It also recognizes the
camera-X/add/`dp+$05` trigger idiom used by per-type wake-up logic.

The matched horizontal windows grow by `margin + 32` on each side. The extra
32 pixels ensure an object activates outside the visible 16:9 edge instead of
popping on its outermost column. The current full native generation contains
26 marked rewrites across banks `$02`, `$03`, `$07`, `$08`, `$13`, and `$3C`.
Regeneration fails loudly if the expected structural surface shrinks.

The helpers return their vanilla constants whenever widescreen is inactive.
`SNESRECOMP_WS_SPAWN=0` independently disables object-window widening.

## Native promotion boundary

Profile promotion originally froze immediately after loading save slot 0.
First-frame return tracing isolated `$00:DD59 -> $03:8DA0`. Routine `$03:8DA0`
calls a generated WRAM helper at `$7E:26A0` which rewrites the guest return
stack and returns directly to the outer caller. A native C call frame cannot
represent that non-local return; it leaked five guest-stack bytes per call.

`force_lle 038DA0` in `recomp/bank00.cfg` keeps that exact executable boundary
on the interpreter tier while its callers remain native. `$00:9133`, which
enters the same stack-programmed WRAM dispatcher at `$7E:26A6` during the
CAPCOM/title transition, is held LLE for the same reason. The directive is
implemented in both Python and Rust analyzers in the paired latest-snesrecomp
worktree. Neither unsafe boundary receives a native dispatch slot.

## Validation

Save slot 0 was loaded through the paused debug bridge and driven right with a
deterministic input sequence.

- The repaired native build advanced beyond the formerly frozen first frame.
- Camera X moved from `$0100` to `$0204` over 180 rightward frames.
- Both BG layers stayed active while scrolling and accumulated hundreds of
  thousands of west/east shadow hits with zero misses.
- Captured 342x224 frames showed continuous jungle/terrain in both gutters.
- Enemy sprites were active and rendering into the east extension.
- Native entry into `$02:D636` was observed during the same scene.
- With widescreen disabled, the pre-change interpreter build and the new
  native build produced an exact 256x224 pixel match after the same save and
  120-frame input sequence: zero differing pixels.

Generated sources are intentionally untracked. Always run
`tools/regen.sh`; it applies the widescreen overrides and checks idempotency.
