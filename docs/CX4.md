# Cx4 coprocessor — Mega Man X3

Mega Man X3 carries Capcom's **Cx4**: a Hitachi **HG51B S169** DSP. Internal
header chipset `$F3` (ROM + coprocessor, coprocessor id `$F` "custom") with the
`$FFBF` sub-type byte `$10`. Only two commercial titles use it — X2 and X3.

## Status: LLE floor, permissively licensed

The engine emulates the DSP at the **instruction level**
(`snesrecomp/runner/src/snes/cx4.c`, ported from ares, ISC-licensed). The chip
fetches and executes Mega Man X3's own Cx4 program out of cartridge ROM — this is
the faithful floor, not a host-side reimplementation of what that program
computes. There is therefore no "unrecognised command" surface: whatever the
game's Cx4 code does, the core runs it.

Full engine-side notes: `snesrecomp/runner/src/snes/CX4_NOTES.md`.

## You must supply the firmware

The HG51B S169 has a 1024-entry x 24-bit internal data ROM — a
reciprocal/division table — which is **not** part of the game ROM. Without it
`RDROM` reads zeros and every division the Cx4 program performs is wrong.

Put **`cx4.rom`** (exactly 3072 bytes) at the repo root, or next to the
executable, or point `$SNESRECOMP_CX4_ROM` at it. It is Capcom/Hitachi data, so
this project does not redistribute it and `.gitignore` refuses it.

Measured on X2's boot self-test: the Cx4 program reads **all 1024** entries. The
firmware is genuinely required, not optional.

Needing real firmware is the expected cost of an LLE floor. A future HLE layer
could remove it — but only as a gated optimization on top, with the faithful
path still forceable, and authored from this core's observed behavior.

## Why the Cx4 blocks boot at all (measured, not assumed)

The measurement below is **X2's**; X3 shares the same boot-time self-test and
was blocked identically, but these counts are X2's.

Before the engine had any Cx4, X2 booted, ran to about host frame 17, then
stopped. The always-on off-rails ring named the cause exactly:

```
$ python tools/dbgprobe.py offrails
{"count":2,"buckets":[
  {"tag":"cart_readLorom","first_frame":2,"last_frame":17,
   "first_hint":"0x00006000","last_hint":"0x0000621F","hit_count":8704},
  {"tag":"RomPtr-invalid","first_frame":17,"last_frame":2514,
   "first_hint":"0x00007F5E","last_hint":"0x00007F5E","hit_count":82433}]}
```

8704 accesses to the Cx4 data-RAM window, then **82,433 reads of `$7F5E`** —
the Cx4 status register — with nothing else happening. The game ran its Cx4
self-test and then spun forever waiting for a device that always answered `0`.
Screen stayed at `inidisp=$80` (forced blank), 100% black.

## CPU-visible map (Mapping 0)

```
$00-$3F,$80-$BF:$6000-$6BFF   3 KB DSP data RAM (mirror at $7000-$7BFF)
$00-$3F,$80-$BF:$6C00-$6FFF   IO window        (mirror at $7C00-$7FFF), within which:
    $7F40-$7F47   DMA source / length / target  (writing target byte 2 starts it)
    $7F48-$7F4F   program cache: page, base, locks, entry PB/PC
                  $7F4F ALSO STARTS THE DSP when halted -- the value is an
                  entry PROGRAM COUNTER, not a command id
    $7F50-$7F5F   wait states, IRQ enable, ROM config, suspend, STATUS
    $7F60-$7F7F   32 vector bytes
    $7F80-$7FAF   R0-R15, three bytes each ($7FC0-$7FEF mirrors it)
$00-$3F,$80-$BF:$8000-$FFFF   ordinary LoROM
```

The DSP runs at 20 MHz against the SNES's ~21.477 MHz master clock, so it is
advanced through `cart_sync_coprocessors` before anything observes its state.
It can also assert the CPU IRQ line when its program halts.

## Debugging

```
$ python tools/dbgprobe.py cx4
{"runs":2,"insns":21704,"rdrom_hits":1024,"firmware":1,"locked":0,"irq":0,
  "ring":[{"seq":0,"pb":"0x000e","pc":"0x5c","base":"0x028000"},
          {"seq":1,"pb":"0x000e","pc":"0x89","base":"0x028000"}]}
```

| field | meaning when it looks wrong |
|---|---|
| `firmware` | `0` => `cx4.rom` missing; every `RDROM` result is zeros |
| `rdrom_hits` | `>0` with `firmware:0` => the missing blob is corrupting results |
| `insns` | `0` => the DSP never ran at all |
| `locked` | `1` => the core wedged its bus (same-space DMA); needs a reset |
| `ring` | DSP program starts: entry `pb`/`pc` and cache `base` |
