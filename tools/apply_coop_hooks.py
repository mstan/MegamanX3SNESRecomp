#!/usr/bin/env python3
"""Keep X3's paired actor, contact and owned-pool boundaries observable."""
import argparse
from pathlib import Path
import re

MARKER = '/*X3-COOP*/'
ENTRY = re.compile(r'RecompReturn bank_(00|02|03|04)_([0-9A-F]{4})_M[01]X[01]\(CpuState \*cpu\) \{')
TARGETS = {0: {0xda7f,0xdaa0,0xdac2,0xdaf0,0xdafa,0xdb9f,0xdbc1,0xdc06,0xdc23,0xdc59,0xdc7b,0xdca9,0xdcb3,0xdcc0,0xdce2,0xdd10,0xdd1a},
           2: {0xd58a,0xd5da,0xd611,0xd636,0xd65b,0xd67a,0xd68f,0xd6a4,0xd6b9,0xd6ce,0xd6e3,0xda99,0xdab8,0xe13f},
           3: {0x8f70},
           4: {0x8000,0x81d0,0xb58a,0xbfae,0xc07f,0xcb31,0xcb74}}

def apply(source):
    output = [MARKER + ' #include "snes/interp_bridge.h"\n']
    pending = False
    count = 0
    for line in source.splitlines(keepends=True):
        if MARKER in line:
            continue
        output.append(line)
        match=ENTRY.match(line)
        if match and int(match[2],16) in TARGETS.get(int(match[1],16),set()):
            pending = int(match[1],16)<<16|int(match[2],16)
        if pending and 'g_cpu_entry_s[g_recomp_stack_top - 1] = _entry_s;' in line:
            output.append('  ' + MARKER + f' {{ extern bool X3CoopRuntimeEnabled(void); if (X3CoopRuntimeEnabled()) {{ RecompReturn r = interp_tier_dispatch_bank_miss(cpu, 0x{pending:06x}u, _entry_s, _hrv); RecompStackPop(); return r; }} }}\n')
            count += 1
            pending = False
    if pending:
        raise ValueError('Missing X3 controller paired prologue')
    return ''.join(output), count

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gen-dir', type=Path, required=True)
    args = parser.parse_args()
    count = 0
    for path in args.gen_dir.glob('bank*.c'):
        source = path.read_text(encoding='utf-8')
        updated, n = apply(source)
        if n and updated != source:
            path.write_text(updated, encoding='utf-8', newline='\n')
        count += n
    if not count:
        raise ValueError('X3 controller entry was not emitted')
    print(f'X3 co-op: {count} native controller entries routed')

if __name__ == '__main__':
    main()
