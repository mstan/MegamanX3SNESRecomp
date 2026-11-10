#!/usr/bin/env python3
"""Keep X3's balanced native controller boundary observable in development co-op."""
import argparse
from pathlib import Path
import re

MARKER = '/*X3-COOP*/'
ENTRY = re.compile(r'RecompReturn bank_04_(8000|B58A)_M[01]X[01]\(CpuState \*cpu\) \{')

def apply(source):
    output = [MARKER + ' #include "snes/interp_bridge.h"\n']
    pending = False
    count = 0
    for line in source.splitlines(keepends=True):
        if MARKER in line:
            continue
        output.append(line)
        match=ENTRY.match(line)
        if match:
            pending = int(match[1],16)
        if pending and 'g_cpu_entry_s[g_recomp_stack_top - 1] = _entry_s;' in line:
            output.append('  ' + MARKER + f' {{ extern bool X3CoopRuntimeEnabled(void); if (X3CoopRuntimeEnabled()) {{ RecompReturn r = interp_tier_dispatch_bank_miss(cpu, 0x04{pending:04x}u, _entry_s, _hrv); RecompStackPop(); return r; }} }}\n')
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
