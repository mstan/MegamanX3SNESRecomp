#!/usr/bin/env python3
"""Probe the running MegaManX3SNESRecomp trace build (debug server port 4381).

Every subcommand QUERIES an always-on ring or live state. Nothing here arms a
recorder then runs a workload â€” the rings record continuously from process
start, so probes read backward in history.

Usage:
  python tools/dbgprobe.py ping
  python tools/dbgprobe.py shot [out.bmp]     # screenshot via the debug surface
  python tools/dbgprobe.py ppu                # live PPU register state
  python tools/dbgprobe.py cpu                # live 65816 / CpuState
  python tools/dbgprobe.py offrails           # out-of-range cart access ring
  python tools/dbgprobe.py cx4 [n]            # Cx4 command ring + unknown count
  python tools/dbgprobe.py interp             # interpreter-tier stats
  python tools/dbgprobe.py dispatch           # runtime indirect-dispatch ring
  python tools/dbgprobe.py regs [n]           # last n hardware-register writes
  python tools/dbgprobe.py raw "<command>"    # any raw server command
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'snesrecomp', 'tools'))
from sneslib.client import DebugClient  # noqa: E402

PORT = int(os.environ.get('DBG_PORT', '4384'))


def main():
    args = sys.argv[1:]
    cmd = args[0] if args else 'ping'
    c = DebugClient(PORT, name=f'x3:{PORT}')

    if cmd == 'ping':
        print(c.query_raw('ping'))
    elif cmd == 'shot':
        out = args[1] if len(args) > 1 else os.path.join(HERE, '..', '_shot.bmp')
        print(c.query_raw(f'screenshot {os.path.abspath(out)}'))
    elif cmd == 'ppu':
        print(c.query_raw('get_ppu_state'))
    elif cmd == 'cpu':
        print(c.query_raw('get_v2_cpu'))
    elif cmd == 'offrails':
        print(c.query_raw('offrails_get'))
    elif cmd == 'cx4':
        n = args[1] if len(args) > 1 else '32'
        print(c.query_raw(f'cx4_state {n}'))
    elif cmd == 'interp':
        print(c.query_raw('interp_stats'))
    elif cmd == 'dispatch':
        print(c.query_raw('dispatch_log_get'))
    elif cmd == 'regs':
        n = args[1] if len(args) > 1 else '64'
        print(c.query_raw(f'get_reg_trace {n}'))
    elif cmd == 'raw':
        print(c.query_raw(args[1]))
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())


