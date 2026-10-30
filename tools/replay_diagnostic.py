#!/usr/bin/env python3
"""Inspect or replay a Mega Man X2/X3 diagnostic in a separate directory (Python 3.11+)."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import tomllib


def inspect(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) < 4 or lines[0] not in ("MMX2REPLAY 1", "MMX3REPLAY 1"):
        raise ValueError("unsupported or truncated replay header")
    frames = checkpoints = 0
    unfinished = False
    for line in lines[4:]:
        fields = line.split()
        if not fields:
            raise ValueError("blank replay record")
        if fields[0] == "S" and len(fields) == 4 and not unfinished:
            checkpoints += 1
        elif fields[0] == "I" and len(fields) == 6 and not unfinished:
            unfinished = True
        elif fields[0] == "H" and len(fields) == 6 and unfinished:
            frames += 1
            unfinished = False
        else:
            raise ValueError(f"invalid record near execution {frames}: {line}")
    print(f"{lines[1]}; {frames} completed executions, {checkpoints} machine checkpoints")
    if unfinished:
        print("Final input has no checkpoint: this is an unfinished/crashed frame.")
    if not path.with_name(path.name + ".sav").is_file():
        raise ValueError("missing matching .replay.sav checkpoint file")
    return lines


def apply_active_set(state, active, catalog):
    """The saved TOML holds offline preferences; a lobby may have overridden them."""
    # Explicitly turn off catalog defaults and offline selections before
    # applying the actual features/options recorded during the match.
    for manifest in sorted(catalog.glob("packages/*/*/manifest.toml")):
        package = tomllib.loads(manifest.read_text(encoding="utf-8"))
        for feature in package.get("feature", []):
            state += (f'\n[[feature]]\npackage_id = {json.dumps(package["id"])}\n'
                      f'id = {json.dumps(feature["id"])}\nenabled = false\n')
    for line in active.splitlines():
        if not line or line == "(none)":
            continue
        match = re.fullmatch(r"([\w.-]+)@([^/ ]+)/([\w.-]+)(.*)", line)
        if not match:
            raise ValueError(f"invalid active mod selection: {line}")
        package, version, feature, options = match.groups()
        state += (f'\n[[package]]\nid = {json.dumps(package)}\nversion = {json.dumps(version)}\n'
                  f'\n[[feature]]\npackage_id = {json.dumps(package)}\n'
                  f'id = {json.dumps(feature)}\nenabled = true\n[feature.values]\n')
        for option in options.split():
            key, sep, value = option.partition("=")
            if not sep or not re.fullmatch(r"[\w.-]+", key):
                raise ValueError(f"invalid recorded option: {option}")
            state += f'{key} = {json.dumps(value)}\n'
    return state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("replay", type=Path)
    parser.add_argument("--check", action="store_true", help="inspect without running the game")
    parser.add_argument("--exe", type=Path, help="matching game executable")
    parser.add_argument("--rom", type=Path, help="your matching USA game ROM")
    parser.add_argument("--work-dir", type=Path, help="new directory for this reproduction")
    parser.add_argument("--headless", action="store_true", help="SDL dummy video/audio, full-speed execution")
    parser.add_argument("--until", type=int, help="stop after this many completed executions (including rollbacks)")
    args = parser.parse_args()
    source = args.replay.resolve(strict=True)
    lines = inspect(source)
    if args.check:
        return 0
    if not args.exe or not args.rom:
        parser.error("replaying requires --exe and --rom")
    exe, rom = args.exe.resolve(strict=True), args.rom.resolve(strict=True)
    if args.work_dir:
        work = args.work_dir.resolve()
        work.mkdir(parents=True, exist_ok=False)
    else:
        work = Path(tempfile.mkdtemp(prefix="mmx-replay-"))
    print(f"Reproduction directory: {work}", flush=True)
    for suffix in ("", ".sav", ".config.ini", ".mods.toml", ".mods.txt"):
        src = source.with_name(source.name + suffix)
        if src.is_file():
            shutil.copy2(src, work / ("capture.replay" + suffix))
    replay = work / "capture.replay"
    if args.until is not None:
        if args.until <= 0:
            parser.error("--until must be positive")
        completed = 0
        for i, line in enumerate(lines):
            if line.startswith("H "):
                completed += 1
                if completed == args.until:
                    replay.write_text("\n".join(lines[:i+1]) + "\n", encoding="utf-8")
                    break
        else:
            parser.error("--until exceeds the number of completed executions")
    config = source.with_name(source.name + ".config.ini")
    if not config.is_file():
        raise ValueError("capture has no configuration sidecar; recreate the recorded settings first")
    shutil.copy2(config, work / "config.ini")
    catalog = work / "mods" / "preloaded"
    shutil.copytree(exe.parent / "mods" / "preloaded" / "packages", catalog / "packages")
    state_path = source.with_name(source.name + ".mods.toml")
    active_path = source.with_name(source.name + ".mods.txt")
    if state_path.is_file() or active_path.is_file():
        state = state_path.read_text(encoding="utf-8") if state_path.is_file() else "format_version = 1\n"
        if active_path.is_file():
            state = apply_active_set(state, active_path.read_text(encoding="utf-8"), catalog)
        tomllib.loads(state)  # Refuse broken TOML before invoking the host.
        (catalog / "state.toml").write_text(state, encoding="utf-8")
    env = os.environ.copy()
    env["MMX_REPLAY_RECORD"] = "0"
    env["SNESRECOMP_RUNAHEAD"] = "0"
    env["MMX_REPLAY_DIR"] = str(work)
    command = [str(exe), "--rom", str(rom), "--config", str(work / "config.ini"),
               "--no-launcher", "--replay-diagnostic", str(replay)]
    if args.headless:
        env["SDL_VIDEODRIVER"] = env["SDL_AUDIODRIVER"] = "dummy"
        env["SNESRECOMP_RUN_FRAMES"] = "1000000"
        env["MMX_REPLAY_FAST"] = "1"
    with (work / "replay.log").open("wb") as log:
        result = subprocess.run(command, cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT)
    print((work / "replay.log").read_text(encoding="utf-8", errors="replace"))
    return result.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        raise SystemExit(str(error)) from error
