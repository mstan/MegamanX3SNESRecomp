#!/usr/bin/env python3
"""Keep X3's paired actor, contact and owned-pool boundaries observable."""
import argparse
from pathlib import Path
import re

MARKER = '/*X3-COOP*/'
ENTRY = re.compile(r'RecompReturn bank_(00|01|02|03|04|13|3D|3F)_([0-9A-F]{4})_M[01]X[01]\(CpuState \*cpu\) \{')
TARGETS = {0: {0xda3b,0xda4e,0xda7f,0xdaa0,0xdac2,0xdaf0,0xdafa,0xdb07,0xdb2c,0xdb9f,0xdbc1,0xdc06,0xdc23,0xdc59,0xdc7b,0xdca9,0xdcb3,0xdcc0,0xdce2,0xdd10,0xdd1a,0xde3a,0xded3,0xdf9f,0xdfc6,0xe02b,0xe088,0xe0a7,0xe49e},
           1: {0xcede,0xd820,0xd989,0xd9e7,0xdd07,0xe0ae,0xec15,0xecec},
           2: {0xd859,0xd8de,0xd58a,0xd5ab,0xd5da,0xd611,0xd636,0xd65b,0xd67a,0xd68f,0xd6a4,0xd6b9,0xd6ce,0xd6e3,0xda99,0xdab8,0xddd3,0xe13f},
           3: {0xddf3,0xa5ec,0x8f70,0xac57,0xad70,0xaeb6,0xb2a7,0xb33d,0xc392,0xd352},
           4: {0x8000,0x81d0,0xb58a,0xbca2,0xbfae,0xc07f,0xcb31,0xcb74,0xcc5c,0xcd6a,0xd06a,0xd06f,0xd1ca,0xd1ef,0xd8d5},
           0x13: {0xc000,0xc145,0xc214,0xc5bd},
           0x3d: {0xff1d},
           0x3f: {0xe268}}
LEARNER_TARGETS = {0x04cd6a,0x04d06a,0x04d06f}

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
            gate = 'X3CoopRuntimeSceneActive' if pending==0x00e49e else 'X3CoopRuntimeLearner' if pending in LEARNER_TARGETS else 'X3CoopRuntimeEnabled'
            output.append('  ' + MARKER + f' {{ extern bool {gate}(void); if ({gate}()) {{ RecompReturn r = interp_tier_dispatch_bank_miss(cpu, 0x{pending:06x}u, _entry_s, _hrv); RecompStackPop(); return r; }} }}\n')
            count += 1
            pending = False
    if pending:
        raise ValueError('Missing X3 controller paired prologue')
    if not count:
        output.pop(0)
    return ''.join(output), count

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gen-dir', type=Path, required=True)
    args = parser.parse_args()
    count = 0
    for path in args.gen_dir.glob('bank*.c'):
        source = path.read_text(encoding='utf-8')
        updated, n = apply(source)
        if updated != source and (n or MARKER in source):
            path.write_text(updated, encoding='utf-8', newline='\n')
        count += n
    if not count:
        raise ValueError('X3 controller entry was not emitted')
    print(f'X3 co-op: {count} native controller entries routed')

if __name__ == '__main__':
    main()
