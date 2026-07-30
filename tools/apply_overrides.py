#!/usr/bin/env python3
"""Apply Mega Man X3's marker-injected widescreen generated-code patches.

X3 retained X2's camera-window idiom. Objects store world X/Y at dp$05/$08
and compare them with the camera at $1E5D/$1E60. This pass widens only:

* pair-confirmed X windows: camera-X read -> add constant -> limit constant;
* camera triggers: camera-X read -> add constant -> compare with dp$05.
* $00:DE3A's four dynamic record-stream frontiers, routed through guarded
  helpers that retain the authentic bounds by default.

The X-axis constants are routed through X3WsObjWinAdd/Limit. Those helpers
return the original value in 4:3 and widen by the live margin plus 32 pixels
in 16:9. Camera-Y reads disarm matching, so vertical behavior stays original.
The record-stream helpers widen only with the diagnostic
SNESRECOMP_WS_STREAM=1 opt-in because the records also control progression.

src/x3_rtl.c applies equivalent, signature-gated private-ROM rewrites for
interpreter and full-LLE execution. Injection is idempotent and `--restore`
removes every marked line.
"""

import argparse
import glob
import os
import re
import sys


OBJ_MARKER = "/*WS-OBJ-WIN*/"
STREAM_MARKER = "/*WS-SPAWN-STREAM*/"
DISPATCH_MARKER = "/*WS-SPAWN-DISPATCH*/"
MARKERS = (OBJ_MARKER, STREAM_MARKER, DISPATCH_MARKER)

RE_FUNC = re.compile(
    r"^RecompReturn (bank_[0-9A-Fa-f]{2}_[0-9A-F]{4})_M\dX\d\(CpuState")
RE_ANCHOR_X = re.compile(
    r"cpu_read16\(cpu, (?:cpu->DB|0x7e), \(uint16\)\(0x1e5d\)\)", re.I)
RE_ANCHOR_Y = re.compile(
    r"cpu_read16\(cpu, (?:cpu->DB|0x7e), \(uint16\)\(0x1e60\)\)", re.I)
RE_CONST = re.compile(r"^(\s*)uint16 (_v\d+) = (0x[0-9a-f]+);\s*$")
RE_DP5 = re.compile(
    r"cpu_read16\(cpu, 0x00, \(uint16\)\(cpu->D \+ 0x0005\)\)")
RE_CMP_TEMP = re.compile(r"\buint32 _tc\d+_\d+ =")
CENTERED_PAIRS = {"bank_02_DAE7": ("0x80", "0x1c0")}
CENTERED_FUNCS = set(CENTERED_PAIRS)

# Per-type copies retain the same symmetric formula with a few additional
# native paddings. Keep emitted bodies on the dynamic-margin path; x3_rtl.c
# mirrors the same constants for interpreter-only bodies.
ADD_CONSTS = {"0x20", "0x38", "0x40", "0x60", "0x80"}
LIMIT_CONSTS = {"0x140", "0x170", "0x180", "0x1c0", "0x200"}
TRIG_ADDS = {
    "0x20", "0x40", "0x60", "0x80", "0xa0", "0xc0",
    "0x100", "0x110", "0x120", "0x140",
}
ADD_BUDGET = 60
LIMIT_BUDGET = 40
CENTERED_LIMIT_BUDGET = 180
EXPECTED_SITES = 30
EXPECTED_PAIRS = 12
EXPECTED_STREAM_SITES = 4
EXPECTED_DISPATCH_SITES = 0


def win_snippet(indent, var, kind, const):
    fn = "X3WsObjWinAdd" if kind == "add" else "X3WsObjWinLimit"
    return (
        f"{indent}{OBJ_MARKER} {{ extern uint16 {fn}(uint16); "
        f"{var} = {fn}({const}); }}\n"
    )


def spawn_stream_snippet(indent, var, kind):
    helpers = {
        "left": "X3WsSpawnStreamLeft",
        "right": "X3WsSpawnStreamRightAdd",
        "grid_pad": "X3WsSpawnStreamGridPad",
        "columns": "X3WsSpawnStreamColumns",
    }
    fn = helpers[kind]
    return (
        f"{indent}{STREAM_MARKER} {{ extern uint16 {fn}(uint16); "
        f"{var} = {fn}({var}); }}\n"
    )


def apply_obj_windows(lines, verbose, fname):
    """Find complete horizontal window pairs, then camera-X triggers."""
    inject = {}
    cur_fn = None
    state = None
    pairs = 0

    for idx, line in enumerate(lines):
        match = RE_FUNC.match(line)
        if match:
            cur_fn = match.group(1)
            state = None
        elif line.startswith("RecompReturn "):
            cur_fn = None
            state = None
        if cur_fn is None:
            continue
        if RE_ANCHOR_X.search(line):
            state = ("add", ADD_BUDGET)
            continue
        if RE_ANCHOR_Y.search(line):
            state = None
            continue
        if state is None:
            continue
        ttl = state[1] - 1
        if ttl <= 0:
            state = None
            continue
        state = (state[0], ttl) + state[2:]
        match = RE_CONST.match(line)
        if not match:
            continue
        indent, var, const = match.groups()
        if state[0] == "add" and const in ADD_CONSTS:
            limit_budget = (CENTERED_LIMIT_BUDGET
                            if cur_fn in CENTERED_FUNCS else LIMIT_BUDGET)
            state = ("limit", limit_budget, idx, (indent, var, const))
        elif state[0] == "limit" and const in LIMIT_CONSTS:
            add_idx, add_match = state[2], state[3]
            add_value = int(add_match[2], 16)
            symmetric = int(const, 16) == 0x100 + 2 * add_value
            centered = CENTERED_PAIRS.get(cur_fn) == (add_match[2], const)
            if not symmetric and not centered:
                continue
            inject[add_idx] = (
                add_match[0], add_match[1], "add", add_match[2], cur_fn)
            inject[idx] = (indent, var, "limit", const, cur_fn)
            pairs += 1
            state = None

    cur_fn = None
    state = None
    for idx, line in enumerate(lines):
        match = RE_FUNC.match(line)
        if match:
            cur_fn = match.group(1)
            state = None
        elif line.startswith("RecompReturn "):
            cur_fn = None
            state = None
        if cur_fn is None:
            continue
        if RE_ANCHOR_X.search(line):
            state = ("armed", 40)
            continue
        if RE_ANCHOR_Y.search(line):
            state = None
            continue
        if state is None:
            continue
        ttl = state[-1] - 1
        if ttl <= 0:
            state = None
            continue
        state = state[:-1] + (ttl,)
        const_match = RE_CONST.match(line)
        if (const_match and state[0] == "armed" and
                const_match.group(3) in TRIG_ADDS and idx not in inject):
            state = ("added", idx, const_match.groups(), 30)
            continue
        # A dp+$05 read can be SBC/LDA as well as CMP. Only a generated CMP
        # has the subtraction temporary immediately after the read.
        is_cmp = any(RE_CMP_TEMP.search(follow)
                     for follow in lines[idx + 1:idx + 6])
        if state[0] == "added" and RE_DP5.search(line) and is_cmp:
            add_idx, (indent, var, const) = state[1], state[2]
            if add_idx not in inject:
                inject[add_idx] = (
                    indent, var, "add", const, cur_fn + " [trigger]")
            state = None

    out = []
    for idx, line in enumerate(lines):
        out.append(line)
        if idx in inject:
            indent, var, kind, const, fn = inject[idx]
            out.append(win_snippet(indent, var, kind, const))
            if verbose:
                print(f"  WS-OBJ-WIN {kind} {const} in {fn} ({fname})")
    return out, len(inject), pairs


def apply_spawn_stream(lines, verbose, fname):
    """Route X3's DE3A record grid through guarded boundary helpers."""
    inject = {}
    cur_fn = None
    block = None
    grid_pad_seen = False
    assign = re.compile(r"^(\s*)uint16 (_v\d+) =")
    label = re.compile(r"^\s*L_(DE5A|DE70|DEB2)_M0X0:")

    for idx, line in enumerate(lines):
        match = RE_FUNC.match(line)
        if match:
            cur_fn = match.group(1)
            block = None
        elif line.startswith("RecompReturn "):
            cur_fn = None
            block = None
        if cur_fn != "bank_00_DE3A":
            continue

        match = label.match(line)
        if match:
            block = match.group(1)
            grid_pad_seen = False
            continue

        if block == "DE5A" and RE_ANCHOR_X.search(line):
            match = assign.match(line)
            if match:
                inject[idx] = (match.group(1), match.group(2), "left")
                block = None
            continue

        match = RE_CONST.match(line)
        if not match:
            continue
        indent, var, const = match.groups()
        if block == "DE70" and const == "0x100":
            inject[idx] = (indent, var, "right")
            block = None
        elif block == "DEB2" and not grid_pad_seen and const == "0x20":
            inject[idx] = (indent, var, "grid_pad")
            grid_pad_seen = True
        elif block == "DEB2" and grid_pad_seen and const == "0xa":
            inject[idx] = (indent, var, "columns")
            block = None

    out = []
    for idx, line in enumerate(lines):
        out.append(line)
        if idx in inject:
            indent, var, kind = inject[idx]
            out.append(spawn_stream_snippet(indent, var, kind))
            if verbose:
                print(
                    f"  WS-SPAWN-STREAM {kind} in bank_00_DE3A ({fname})")
    return out, len(inject)


def apply_spawn_dispatch(lines, verbose, fname):
    """Strip the superseded dispatch experiment through MARKERS cleanup."""
    return lines, 0


def process_file(path, restore, check, verbose):
    with open(path, "r", encoding="utf-8") as source:
        lines = source.readlines()

    old_markers = sum(
        any(marker in line for marker in MARKERS) for line in lines)
    if restore:
        if not old_markers:
            return 0, 0, 0, 0, 0, False
        kept = [
            line for line in lines
            if not any(marker in line for marker in MARKERS)
        ]
        if not check:
            with open(path, "w", encoding="utf-8", newline="\n") as output:
                output.writelines(kept)
        count = len(lines) - len(kept)
        if verbose:
            print(f"{os.path.basename(path)}: stripped {count} injected line(s)")
        return count, 0, 0, 0, 0, False

    clean = [
        line for line in lines
        if not any(marker in line for marker in MARKERS)
    ]
    out, count, pairs = apply_obj_windows(
        clean, verbose, os.path.basename(path))
    out, stream_sites = apply_spawn_stream(
        out, verbose, os.path.basename(path))
    out, dispatch_sites = apply_spawn_dispatch(
        out, verbose, os.path.basename(path))
    matches = lines == out
    stale = old_markers > 0 and not matches
    if not matches and not check:
        with open(path, "w", encoding="utf-8", newline="\n") as output:
            output.writelines(out)
    changed = max(
        0, count + stream_sites + dispatch_sites - old_markers)
    if verbose:
        if matches:
            print(f"{os.path.basename(path)}: injections verified")
        elif check:
            print(f"{os.path.basename(path)}: would normalize injections")
        else:
            print(f"{os.path.basename(path)}: normalized injections")
    return changed, count, pairs, stream_sites, dispatch_sites, stale


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--gen-dir", default="src/gen")
    parser.add_argument("--restore", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    files = sorted(glob.glob(os.path.join(args.gen_dir, "bank*_v2.c")))
    dispatch = os.path.join(args.gen_dir, "dispatch_v2.c")
    if os.path.isfile(dispatch):
        files.append(dispatch)
    if not files:
        print(
            f"apply_overrides: no generated banks under {args.gen_dir}",
            file=sys.stderr)
        return 1

    total = 0
    sites = 0
    pairs = 0
    stream_sites = 0
    dispatch_sites = 0
    stale_files = []
    for path in files:
        (changed, file_sites, file_pairs, file_stream, file_dispatch,
         stale) = process_file(
            path, args.restore, args.check, args.verbose)
        total += changed
        sites += file_sites
        pairs += file_pairs
        stream_sites += file_stream
        dispatch_sites += file_dispatch
        if stale:
            stale_files.append(path)

    verb = "stripped" if args.restore else "injected"
    print(f"apply_overrides: {verb} {total} site(s)")
    # Recompute the expected marked output from clean generated code every
    # time. This validates shape and placement, not merely marker totals.
    if (not args.restore and
            (sites != EXPECTED_SITES or pairs != EXPECTED_PAIRS or
             stream_sites != EXPECTED_STREAM_SITES or
             dispatch_sites != EXPECTED_DISPATCH_SITES)):
        print(
            f"apply_overrides: expected {EXPECTED_SITES} object sites/"
            f"{EXPECTED_PAIRS} pairs, {EXPECTED_STREAM_SITES} stream sites, "
            f"and {EXPECTED_DISPATCH_SITES} dispatch sites; got "
            f"{sites}/{pairs}, {stream_sites}, and {dispatch_sites}; "
            "verify coverage and emitted shapes before building",
            file=sys.stderr)
        return 1
    if args.check and stale_files:
        print("apply_overrides: marked files do not match a clean structural "
              "reinjection: " + ", ".join(stale_files), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
