#!/usr/bin/env bash
# Regen pipeline driver for MegaManX3SNESRecomp.
#
# Regenerates src/gen/*.c from recomp/*.cfg over a verified Mega Man X3 (USA)
# ROM, then syncs recomp/funcs.h.
#
# ROM: mmx3.sfc at the repo root (headered .smc dumps are accepted — the loader
# strips a 512-byte copier header).
#
# Flags:
#   --no-tests             skip the framework test suite (default: run it).
#   --strict-idempotent    regenerate into a temp dir and require byte-identical
#                          output.
#   -h | --help            this message.
#
# Run from anywhere — paths resolve relative to this script's location.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RUN_TESTS=1
STRICT_IDEMPOTENT=0
for arg in "$@"; do
  case "$arg" in
    --no-tests) RUN_TESTS=0 ;;
    --strict-idempotent) STRICT_IDEMPOTENT=1 ;;
    -h|--help)  sed -n '2,/^set -euo/p' "$0" | sed -n '/^# /p' | sed 's/^# //'; exit 0 ;;
    *) echo "regen.sh: unknown flag: $arg (try --help)" >&2; exit 2 ;;
  esac
done

cd "$ROOT"

ROM="mmx3.sfc"
SNESRECOMP_ROOT="${SNESRECOMP_ROOT:-snesrecomp}"
TESTS="$SNESRECOMP_ROOT/tests/run_tests.py"

PYTHON="${PYTHON:-$(command -v python3 || command -v python || true)}"
if [ -z "$PYTHON" ]; then
  echo "regen.sh: no python3/python interpreter found on PATH" >&2
  exit 1
fi

if [ ! -f "$ROM" ]; then
  echo "regen.sh: $ROM not found — stage the verified Mega Man X3 (USA) ROM at the repo root." >&2
  exit 1
fi

if [ ! -f "$SNESRECOMP_ROOT/tools/v2_emit.py" ]; then
  echo "regen.sh: snesrecomp is not initialized (missing $SNESRECOMP_ROOT/tools/v2_emit.py)." >&2
  exit 1
fi

step() { echo; echo "=== $* ==="; }

ANALYSIS_BACKEND="${SNESRECOMP_ANALYSIS_BACKEND:-native}"
case "$ANALYSIS_BACKEND" in
  native|python|auto) ;;
  *) echo "regen.sh: invalid SNESRECOMP_ANALYSIS_BACKEND: $ANALYSIS_BACKEND" >&2; exit 2 ;;
esac

if [ "$ANALYSIS_BACKEND" = native ]; then
  step "Building native analyzer"
  "$PYTHON" "$SNESRECOMP_ROOT/tools/build_native_analyzer.py"
fi

step "Regenerating banks from $ROM"
# --cfg-roots is the static-coverage policy: every declared `func` seeds the
# analysis closure so the proven surface is materialized as AOT; the
# interpreter is the failsafe for the unprovable remainder, never the plan.
"$PYTHON" "$SNESRECOMP_ROOT/tools/v2_emit.py" --rom "$ROM" \
    --cfg-dir recomp --out-dir src/gen --cfg-roots \
    --analysis-backend "$ANALYSIS_BACKEND"

step "Syncing funcs.h"
"$PYTHON" "$SNESRECOMP_ROOT/tools/v2_sync_funcs_h.py" --cfg-dir recomp \
    --out recomp/funcs.h

if [ "$STRICT_IDEMPOTENT" -eq 1 ]; then
  step "Idempotency check: regen into temp dir + byte-compare"
  TMP_GEN="$(mktemp -d)"
  trap 'rm -rf "$TMP_GEN"' EXIT
  "$PYTHON" "$SNESRECOMP_ROOT/tools/v2_emit.py" --rom "$ROM" \
      --cfg-dir recomp --out-dir "$TMP_GEN" --cfg-roots \
      --analysis-backend "$ANALYSIS_BACKEND"
  : > "$TMP_GEN/.gitkeep"
  "$PYTHON" "$SNESRECOMP_ROOT/tools/v2_compare_output.py" \
      --expected src/gen --actual "$TMP_GEN"
fi

if [ "$RUN_TESTS" -eq 1 ]; then
  step "Framework tests"
  "$PYTHON" "$TESTS"
fi

step "Done"
