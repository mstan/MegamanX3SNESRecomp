#!/usr/bin/env python3
"""Drive Mega Man X3 through the TCP debug server and capture screenshots.

Input-injection discipline, which exists because getting it wrong wastes a
session:

  * Injection LATCHES across interrupts. Every press is followed by an explicit
    release, and the script clears the controller on exit even on exception.
  * Never blind-loop a button hoping something happens. Each step states what it
    expects, and `--verify` captures a screenshot after every step so the actual
    pixels decide what happened rather than a frame counter.
  * Screens are identified by measured pixel statistics (distinct colour count,
    mean luminance, non-black fraction), not by assuming a fixed frame number --
    boot timing is not reproducible.

Usage:
  python tools/navigate.py probe                     # where are we now?
  python tools/navigate.py press start --frames 8    # one press+release
  python tools/navigate.py script intro-to-title     # a named sequence
  python tools/navigate.py shot out.png
"""
from __future__ import annotations

import argparse
import os
import socket
import sys
import time

PORT = int(os.environ.get('DBG_PORT', '4384'))
HOST = '127.0.0.1'


class Dbg:
    def __init__(self, port=PORT):
        self.s = socket.create_connection((HOST, port), 10)
        self.s.settimeout(15)

    def q(self, cmd: str) -> str:
        self.s.sendall((cmd + '\n').encode())
        buf = b''
        while b'\n' not in buf:
            chunk = self.s.recv(1 << 20)
            if not chunk:
                raise ConnectionError('debug server closed')
            buf += chunk
        return buf.split(b'\n')[0].decode()

    def frame(self) -> int:
        r = self.q('ping')
        return int(r.split('"frame":')[1].rstrip('}'))

    def wait_ready(self, timeout=60.0):
        """Block until the emulator is actually presenting.

        The trace build allocates ~730 MB of observability rings at startup, so
        the debug server answers `ping` for several seconds before there is a
        render buffer. Probing in that window returns
        'render buffer unavailable', which is a NOT-READY answer -- not a
        verdict about the screen. Distinguishing the two is the difference
        between a real finding and a wasted conclusion."""
        t0 = time.time()
        while time.time() - t0 < timeout:
            try:
                if self.frame() > 0 and '"error"' not in self.q('get_ppu_state'):
                    return
            except Exception:
                pass
            time.sleep(0.25)
        raise TimeoutError(f'emulator not presenting after {timeout}s')

    def wait_frames(self, n: int, timeout=20.0):
        """Wait for n emulated frames to elapse. Polls the frame counter rather
        than sleeping a wall-clock guess -- the trace build runs below realtime
        and a fixed sleep would be either flaky or needlessly slow."""
        start = self.frame()
        t0 = time.time()
        while self.frame() - start < n:
            if time.time() - t0 > timeout:
                raise TimeoutError(
                    f'only {self.frame() - start}/{n} frames advanced in '
                    f'{timeout}s -- is the emulator wedged?')
            time.sleep(0.02)

    def press(self, buttons: str, hold=8, gap=10):
        """Press, hold for `hold` frames, release, then settle for `gap`."""
        self.q(f'set_controller {buttons}')
        self.wait_frames(hold)
        self.q('clear_controller')
        self.wait_frames(gap)

    def clear(self):
        try:
            self.q('clear_controller')
        except Exception:
            pass

    def shot(self, path: str) -> dict:
        path = os.path.abspath(path)
        bmp = path[:-4] + '.bmp' if path.lower().endswith('.png') else path
        r = self.q(f'screenshot {bmp}')
        if '"ok":true' not in r:
            raise RuntimeError(f'screenshot failed: {r}')
        return {'bmp': bmp, 'raw': r}

    def close(self):
        self.clear()
        try:
            self.s.close()
        except Exception:
            pass


def describe(bmp: str) -> dict:
    """Measured pixel statistics -- the basis for any claim about the screen."""
    try:
        from PIL import Image
    except ImportError:
        return {'error': 'PIL not available'}
    im = Image.open(bmp).convert('RGB')
    px = list(im.getdata())
    n = len(px)
    colors = len(set(px))
    lum = [0.299 * r + 0.587 * g + 0.114 * b for r, g, b in px]
    nonblack = sum(1 for v in lum if v > 8)
    return {
        'size': im.size,
        'colors': colors,
        'mean_lum': round(sum(lum) / n, 2),
        'nonblack_pct': round(100.0 * nonblack / n, 1),
    }


def to_png(bmp: str, scale=2):
    from PIL import Image
    im = Image.open(bmp).convert('RGB')
    out = bmp[:-4] + '.png'
    im.resize((im.width * scale, im.height * scale), Image.NEAREST).save(out)
    return out


SCRIPTS = {
    # X3's intro is a multi-scene text cutscene. START should skip scenes; the
    # sequence is deliberately short and verified between presses rather than
    # mashing blindly. Timings are inherited from X2 and NOT yet tuned for X3 --
    # run with --verify and adjust from the measured screens.
    'intro-to-title': [('start', 6, 40)] * 6,
    'title-to-play': [('start', 6, 60), ('start', 6, 60)],
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('action', choices=['probe', 'press', 'script', 'shot'])
    ap.add_argument('arg', nargs='?')
    ap.add_argument('--frames', type=int, default=8)
    ap.add_argument('--gap', type=int, default=12)
    ap.add_argument('--verify', action='store_true',
                    help='screenshot + describe after every step')
    ap.add_argument('--out', default='_nav')
    args = ap.parse_args()

    d = Dbg()
    try:
        d.wait_ready()
        if args.action == 'probe':
            print('frame :', d.frame())
            print('ppu   :', d.q('get_ppu_state')[:160])
            print('cx4   :', d.q('cx4_state 1')[:150])
            r = d.shot(f'{args.out}_probe.bmp')
            print('shot  :', r['raw'])
            print('pixels:', describe(r['bmp']))
            print('png   :', to_png(r['bmp']))

        elif args.action == 'press':
            if not args.arg:
                print('need a button name'); return 2
            before = d.frame()
            d.press(args.arg, args.frames, args.gap)
            print(f'pressed {args.arg}: frame {before} -> {d.frame()}')
            if args.verify:
                r = d.shot(f'{args.out}_press.bmp')
                print('pixels:', describe(r['bmp']), '->', to_png(r['bmp']))

        elif args.action == 'script':
            steps = SCRIPTS.get(args.arg)
            if not steps:
                print(f'unknown script; have: {", ".join(SCRIPTS)}'); return 2
            for i, (btn, hold, gap) in enumerate(steps):
                d.press(btn, hold, gap)
                line = f'step {i + 1}/{len(steps)} {btn:6s} frame={d.frame()}'
                if args.verify:
                    r = d.shot(f'{args.out}_s{i + 1}.bmp')
                    line += f'  {describe(r["bmp"])}'
                print(line, flush=True)

        elif args.action == 'shot':
            r = d.shot(args.arg or f'{args.out}.bmp')
            print(r['raw'])
            print('pixels:', describe(r['bmp']))
            print('png   :', to_png(r['bmp']))
        return 0
    finally:
        d.close()          # never leave a button latched


if __name__ == '__main__':
    sys.exit(main())
