#!/usr/bin/env python3
"""Apply Mega Man X3's marker-injected widescreen generated-code patches.

X3 retained X2's camera-window idiom. Objects store world X/Y at dp$05/$08
and compare them with the camera at $1E5D/$1E60. This pass widens only:

* pair-confirmed X windows: camera-X read -> add constant -> limit constant;
* camera triggers: camera-X read -> add constant -> compare with dp$05.

The X-axis constants are routed through X3WsObjWinAdd/Limit. Those helpers
return the original value in 4:3 and widen by the live margin plus 32 pixels
in 16:9. Camera-Y reads disarm matching, so vertical behavior stays original.

Injection is idempotent and `--restore` removes every marked line.
"""

import argparse
import glob
import os
import re
import sys


MARKERS = ("/*WS-OBJ-WIN*/",)

RE_FUNC = re.compile(
    r"^RecompReturn (bank_[0-9A-Fa-f]{2}_[0-9A-F]{4})_M\dX\d\(CpuState")
RE_ANCHOR_X = re.compile(
    r"cpu_read16\(cpu, (?:cpu->DB|0x7e), \(uint16\)\(0x1e5d\)\)", re.I)
RE_ANCHOR_Y = re.compile(
    r"cpu_read16\(cpu, (?:cpu->DB|0x7e), \(uint16\)\(0x1e60\)\)", re.I)
RE_CONST = re.compile(r"^(\s*)uint16 (_v\d+) = (0x[0-9a-f]+);\s*$")
RE_DP5 = re.compile(
    r"cpu_read16\(cpu, 0x00, \(uint16\)\(cpu->D \+ 0x0005\)\)")

ADD_CONSTS = {"0x20", "0x40", "0x60"}
LIMIT_CONSTS = {"0x140", "0x180", "0x1c0"}
TRIG_ADDS = {
    "0x20", "0x40", "0x60", "0x80", "0xa0", "0xc0",
    "0x100", "0x110", "0x120", "0x140",
}
ADD_BUDGET = 60
LIMIT_BUDGET = 40


def win_snippet(indent, var, kind, const):
    fn = "X3WsObjWinAdd" if kind == "add" else "X3WsObjWinLimit"
    return (
        f"{indent}/*WS-OBJ-WIN*/ {{ extern uint16 {fn}(uint16); "
        f"{var} = {fn}({const}); }}\n"
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
            state = ("limit", LIMIT_BUDGET, idx, (indent, var, const))
        elif state[0] == "limit" and const in LIMIT_CONSTS:
            add_idx, add_match = state[2], state[3]
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
        if state[0] == "added" and RE_DP5.search(line):
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


def process_file(path, restore, check, verbose):
    with open(path, "r", encoding="utf-8") as source:
        lines = source.readlines()

    had_markers = any(
        any(marker in line for marker in MARKERS) for line in lines)
    if restore:
        if not had_markers:
            return 0, 0
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
        return count, 0

    if had_markers:
        if verbose:
            print(f"{os.path.basename(path)}: already injected, skipping")
        return 0, 0

    out, count, pairs = apply_obj_windows(lines, verbose, os.path.basename(path))
    if count and not check:
        with open(path, "w", encoding="utf-8", newline="\n") as output:
            output.writelines(out)
    if count and verbose:
        print(f"{os.path.basename(path)}: injected {count} site(s)")
    return count, pairs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--gen-dir", default="src/gen")
    parser.add_argument("--restore", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    files = sorted(glob.glob(os.path.join(args.gen_dir, "bank*_v2.c")))
    if not files:
        print(
            f"apply_overrides: no generated banks under {args.gen_dir}",
            file=sys.stderr)
        return 1

    total = 0
    pairs = 0
    for path in files:
        count, file_pairs = process_file(
            path, args.restore, args.check, args.verbose)
        total += count
        pairs += file_pairs

    verb = "stripped" if args.restore else "injected"
    print(f"apply_overrides: {verb} {total} site(s)")
    # At minimum, all variants of the three shared bank-$02 gates must exist.
    # The current full coverage profile yields 26 total injections and 9
    # complete pairs; trigger counts may grow with future gameplay coverage.
    if not args.restore and total not in (0,) and (total < 14 or pairs < 7):
        print(
            "apply_overrides: shared object-window population is incomplete; "
            "verify coverage and emitted shapes before building",
            file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
