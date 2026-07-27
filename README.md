# MegaManX3SNESRecomp

Static recompilation of **Mega Man X3 (USA)** built on
[snesrecomp](https://github.com/mstan/snesrecomp), sharing the
[recomp-ui](https://github.com/mstan/recomp-ui) pre-boot launcher with the
other SNES recomp ports.

You must supply your own legally-obtained ROM. Nothing ROM-derived is
committed here.

## ROM identity

| | |
|---|---|
| file | `mmx3.sfc` at the repo root |
| size | headerless, verified against the digest below |
| CRC32 | `0xFA0FE671` |
| SHA-256 | `65b03268afac296330e8ff8d60dd0825879e13ed658b37713c034a3bd074f1d7` |

A 512-byte SMC copier header is stripped automatically by both the recompiler
and the launcher, so headered dumps verify against the same digest.

## Status

**Boots and runs live gameplay.** The opening story scenes and save-slot 0
gameplay have been verified through the debug surface. The native coverage
manifest emits 5,082 exact variants; unsupported control-flow boundaries fall
back to the interpreter.

16:9 widescreen is implemented as a default-disabled built-in Mod. It includes
anchored HUD elements, exact BG1/BG2 gutter tiles from X3's retained level
maps, and widened enemy activation/visibility/draw windows. Enable it from the
launcher's **Mods** page; see
[`docs/WIDESCREEN.md`](docs/WIDESCREEN.md) for addresses and validation.

Still open: broad stage-by-stage gameplay/oracle coverage and audio
verification.

This project was stood up from the MegamanXRecomp host layer; every Mega Man
X 1 specific finding (the fiber task scheduler, the BG2 widescreen shadow, the
MMX1 WRAM stage gates) was deliberately *not* carried over, because none of it
is known to hold for Mega Man X3.

What that means concretely:

* **Execution is LLE-first.** `src/x3_rtl.c` boots the real RESET vector
  through the interpreter bridge, injects NMI only once the game arms NMITIMEN,
  and services IRQs as they latch. AOT coverage from `recomp/*.cfg` is layered
  on as an optimization; anything not covered runs the real ROM bytes.
* **`recomp/bank00.cfg` remains the hand-written seed** (`auto_vectors` +
  `tier_down_stubs`), augmented by profile-manifest roots and explicit
  interpreter-only boundaries.
* **The interrupt handlers live in WRAM.** The ROM's native NMI/IRQ vectors
  point at `$00:1FEF` / `$00:1FF3` (the low WRAM mirror), where boot installs
  `JML` trampolines into a driver block copied out of ROM bank `$08`. That is
  the same class as SMW's `$7F8000` RAM routine, so the engine's `ram_routine`
  guarded-AOT mechanism is the eventual path to compiling it; today it runs
  interpreted.
* **An IRQ is delivered only when `P.I` is clear.** `g_snes->inIrq` is the
  hardware IRQ *line*, which stays asserted until `$4211` is read; the 65816
  only takes it while interrupts are enabled. Mega Man X3 plants `NOP / BRA self`
  at its WRAM IRQ entry as the *un-armed* state and patches it when it actually
  wants the raster IRQ — so delivering an IRQ without checking `P.I` parks the
  guest in that spin loop for the rest of the host frame. Symptom: interpreter
  step-cap bails every frame and `master_cycles` running tens of frames ahead
  of the deadline, with the screen stuck in forced blank.

### Cx4 — firmware required

Mega Man X3 carries Capcom's **Cx4** (a Hitachi HG51B S169 DSP; internal header
chipset `$F3`, `$FFBF` sub-type `$10`). The engine emulates it at the
**instruction level** (`snesrecomp/runner/src/snes/cx4.c`, from ares, ISC) --
the faithful LLE floor. Without it the game spins forever on the Cx4 status
register at `$7F5E` and never lifts forced blank.

**You must supply `cx4.rom`** (exactly 3072 bytes) at the repo root, next to the
executable, or via `$SNESRECOMP_CX4_ROM`. That is the chip's internal
reciprocal table; it is not in the game ROM, it is Capcom/Hitachi data, and this
project does not redistribute it. Missing firmware is reported loudly -- it does
not fail silently.

Read `docs/CX4.md` before debugging anything Cx4-shaped.

## Debugging

The trace build carries a TCP debug server on port 4384 plus the
always-on observability rings. Probes QUERY the rings; they never arm a
recorder and then run a workload.

```bash
python tools/dbgprobe.py ppu        # live PPU state
python tools/dbgprobe.py offrails   # out-of-range cart access ring
python tools/dbgprobe.py cx4        # Cx4 command ring + unknown-command count
python tools/dbgprobe.py shot x.bmp # screenshot through the debug surface
```

## Build

```bash
git submodule update --init --recursive
cp /path/to/your/dump mmx3.sfc
bash tools/regen.sh --no-tests      # emit src/gen/*.c + recomp/funcs.h
cmake -S . -B build -G Ninja -DSNESRECOMP_ENABLE_TRACE=ON
cmake --build build
```

`-DSNESRECOMP_ENABLE_TRACE=ON` builds the TCP debug server and the always-on
observability rings — the bring-up configuration. Omit it for a release build.

This title opts into snesrecomp's package loader. The build preloads a
default-disabled Mega Man X3 Widescreen feature under `mods/packages`; users
may also install data-only `.snesmod` archives from the launcher's Mods page.
