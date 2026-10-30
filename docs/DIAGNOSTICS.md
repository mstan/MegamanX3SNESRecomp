# Reporting problems in Mega Man X3

Diagnostic recording is **enabled by default**, including after an upgrade.
Play normally. After a problem, close the game and ZIP the complete `logs`
folder beside the executable. Send it with your issue list, approximate times
and, when useful, a video. Include `last_run_report.json` and crash reports
if present. No upload happens automatically.

Keep the `.replay`, `.replay.sav`, `.replay.config.ini`, `.replay.mods.toml`
and `.replay.mods.txt` files together: checkpoints alone are not enough.
Recordings include game memory, inputs, build/ROM identities, settings and
active mod options; settings can include local paths. No ROM is included.
A read-only installation falls back to `%TEMP%/MegaManX3SNESRecomp/logs`.

Each launch keeps up to one hour of 10-second segments, with at most 256 MiB
of machine checkpoints (heavy save loading may shorten retained history).
Older sessions remain until you remove them. Disk writes and per-frame hashes
run on the shared background worker; if its bounded queue fills or storage
fails, capture stops and gameplay continues. Run-ahead suspends capture;
disable it when reproducing a report. A hung/crashed final frame may have an
input without a completed checksum and is reported as incomplete.

## Replaying a report

Use the matching release and your own verified USA ROM. Python 3.11+:

```powershell
python tools/replay_diagnostic.py logs/capture.replay --check
python tools/replay_diagnostic.py logs/capture.replay --exe ./MegaManX3SNESRecomp.exe --rom /path/to/mmx3.sfc --work-dir /path/to/new-reproduction
```

The helper copies diagnostics and restores the captured effective mods/settings
in an isolated directory. It never writes the player's original saves/settings.
Use `--headless` for muted, full-speed verification, or omit it to watch.
`--until N` stops after N completed input executions. Replay verifies WRAM,
VRAM, palette and sprite memory each frame and exits nonzero on a mismatch,
invalid checkpoint, wrong ROM/title, or unfinished frame. The first mismatch
is logged and saved as `replay-divergence.sav` in the reproduction directory.

Developers can opt out of automatic capture with `MMX_REPLAY_RECORD=0`;
`MMX_REPLAY_DIR` overrides the capture directory, and
`MMX_REPLAY_HISTORY_MIB` (2 to 256) lowers its checkpoint storage budget.
`logging.ini` can keep session output in a console (`Console = 1`).

## Developer acceptance

Build with `-DMMX_STATE_TESTS=ON`, then run:

```powershell
python tests/test_diagnostic_replay.py --exe build-diagnostics/MegaManX3SNESRecomp.exe --state-exe build-diagnostics/mmx_state_tests.exe --rom /path/to/mmx3.sfc --work-dir /path/to/new-validation
```

This uses isolated disposable saves, SDL dummy video/audio, fresh-process
replay, the shared current-build save/load/rewind checks, default capture,
widescreen settings, adaptive window widths, reset/renderer changes, malformed reports, opt-out,
storage failure and bounded retention. It never edits an owner profile.
