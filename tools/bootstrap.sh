#!/usr/bin/env bash
# Initialize and verify every dependency needed by this checkout.
#
# Two supported layouts:
#
#   submodule  — a standalone clone. snesrecomp/ and recomp-ui/ are real
#                submodule checkouts pinned by gitlink. The pin is enforced.
#
#   junction   — a local multi-game working tree, where snesrecomp/ and
#                recomp-ui/ are Windows junctions (or symlinks) into shared
#                checkouts so every game builds against ONE engine. The
#                gitlink pin is advisory here by design: the shared engine
#                moves ahead of any single game's pin, and that is the point
#                of the layout. The pin is reported, not enforced.
#
# Run: bash tools/bootstrap.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! git -C "$ROOT" rev-parse --show-toplevel >/dev/null 2>&1; then
  echo "bootstrap.sh: $ROOT is not a Git checkout" >&2
  exit 1
fi

# A junction/symlink at either dependency means the shared working-tree layout.
LINKED=0
for dep in snesrecomp recomp-ui; do
  if [ -L "$ROOT/$dep" ]; then LINKED=1; fi
done

if [ "$LINKED" -eq 1 ]; then
  echo "=== Shared working-tree layout (junctioned dependencies) ==="
  for dep in snesrecomp recomp-ui; do
    if [ ! -d "$ROOT/$dep" ]; then
      echo "bootstrap.sh: $dep is missing at $ROOT/$dep" >&2
      exit 1
    fi
    kind="directory"
    [ -L "$ROOT/$dep" ] && kind="junction"
    head="$(git -C "$ROOT/$dep" rev-parse --short HEAD 2>/dev/null || echo '<not a git checkout>')"
    pin="$(git -C "$ROOT" ls-files --stage -- "$dep" | awk '$1 == "160000" { print substr($2,1,7) }')"
    printf '  %-11s %-9s head=%s pin=%s\n' "$dep" "$kind" "$head" "${pin:-<none>}"
    if [ -n "$pin" ] && [ "$head" != "$pin" ]; then
      printf '  %-11s note: shared checkout is ahead of / differs from this game'\''s pin (expected in this layout)\n' ""
    fi
  done
else
  echo "=== Synchronizing submodule URLs ==="
  git -C "$ROOT" submodule sync --recursive

  echo "=== Initializing pinned submodules ==="
  git -C "$ROOT" submodule update --init --recursive

  expected="$(git -C "$ROOT" ls-files --stage -- snesrecomp | awk '$1 == "160000" { print $2 }')"
  actual="$(git -C "$ROOT/snesrecomp" rev-parse HEAD 2>/dev/null || true)"
  if [ -z "$expected" ] || [ "$actual" != "$expected" ]; then
    echo "bootstrap.sh: snesrecomp is at ${actual:-<missing>}, expected ${expected:-<missing gitlink>}" >&2
    exit 1
  fi

  status="$(git -C "$ROOT" submodule status --recursive)"
  if printf '%s\n' "$status" | grep -Eq '^[+-U]'; then
    echo "bootstrap.sh: one or more submodules are missing or at the wrong revision:" >&2
    printf '%s\n' "$status" >&2
    exit 1
  fi
fi

# Layout-independent: the files the build and regen pipeline actually consume.
for f in snesrecomp/runner/runner.cmake \
         snesrecomp/tools/v2_emit.py \
         recomp-ui/recomp_ui.cmake; do
  if [ ! -f "$ROOT/$f" ]; then
    echo "bootstrap.sh: required file missing: $f" >&2
    echo "bootstrap.sh: the dependency checkout is incomplete" >&2
    exit 1
  fi
done

engine_head="$(git -C "$ROOT/snesrecomp" rev-parse --short HEAD 2>/dev/null || echo '<unknown>')"
printf '\nReady: snesrecomp %s and recomp-ui are initialized.\n' "$engine_head"
printf 'Next: stage your legally obtained ROM as mmx3.sfc, then run\n'
printf '      bash tools/regen.sh --no-tests\n'
