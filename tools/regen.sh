#!/usr/bin/env bash
# Regen pipeline driver for MegaManX3SNESRecomp.
#
# Regenerates src/gen/*.c from recomp/*.cfg over a verified Mega Man X3 (USA)
# ROM, then syncs recomp/funcs.h.
#
# ROM: mmx3.sfc at the repo root (headered .smc dumps are accepted — the loader
# strips a 512-byte copier header).
#
# AOT coverage: if recomp/tier2_coverage.json exists it is passed as an
# optional-root profile (see the block below). Grow coverage with the
# feedback loop in snesrecomp/docs/MULTI_TIER.md:
#   run the game -> `tier2_dump` on the debug port -> audit the manifest with
#   snesrecomp/tools/tier2_ingest.py -> paste the vetted directives into
#   recomp/*.cfg -> re-run this script. Repeat to fixpoint.
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

# Tier-2 coverage profile: clean interpreter-observed targets become optional
# AOT roots. It only influences materialization -- it never authorizes
# behavior, changes decoding, or removes the LLE fallback. The manifest is
# produced by the runner (`tier2_dump` on the debug port, or on exit) and
# audited through snesrecomp/tools/tier2_ingest.py before anything is trusted.
#
# Passed conditionally: v2_emit hard-errors on a missing manifest, and X3 has
# to be able to regen before the first profiling run has ever happened.
PROFILE_MANIFEST="recomp/tier2_coverage.json"
emit_extra=()
if [ -f "$PROFILE_MANIFEST" ]; then
  emit_extra+=(--profile-manifest "$PROFILE_MANIFEST")
  echo "regen.sh: using coverage profile $PROFILE_MANIFEST"
else
  echo "regen.sh: no $PROFILE_MANIFEST yet - AOT roots come from cfg + vectors only."
  echo "regen.sh: to build one, run the game and issue 'tier2_dump' on the debug port."
fi

step "Regenerating banks from $ROM"
# --cfg-roots is the static-coverage policy: every declared `func` seeds the
# analysis closure so the proven surface is materialized as AOT; the
# interpreter is the failsafe for the unprovable remainder, never the plan.
"$PYTHON" "$SNESRECOMP_ROOT/tools/v2_emit.py" --rom "$ROM" \
    --cfg-dir recomp --out-dir src/gen --cfg-roots \
    --analysis-backend "$ANALYSIS_BACKEND" \
    "${emit_extra[@]+"${emit_extra[@]}"}"

step "Applying widescreen gen-code overrides"
"$PYTHON" tools/apply_overrides.py --gen-dir src/gen -v

step "Syncing funcs.h"
"$PYTHON" "$SNESRECOMP_ROOT/tools/v2_sync_funcs_h.py" --cfg-dir recomp \
    --out recomp/funcs.h

if [ "$STRICT_IDEMPOTENT" -eq 1 ]; then
  step "Idempotency check: regen into temp dir + byte-compare"
  TMP_GEN="$(mktemp -d)"
  trap 'rm -rf "$TMP_GEN"' EXIT
  "$PYTHON" "$SNESRECOMP_ROOT/tools/v2_emit.py" --rom "$ROM" \
      --cfg-dir recomp --out-dir "$TMP_GEN" --cfg-roots \
      --analysis-backend "$ANALYSIS_BACKEND" \
      "${emit_extra[@]+"${emit_extra[@]}"}"
  "$PYTHON" tools/apply_overrides.py --gen-dir "$TMP_GEN"
  : > "$TMP_GEN/.gitkeep"
  "$PYTHON" "$SNESRECOMP_ROOT/tools/v2_compare_output.py" \
      --expected src/gen --actual "$TMP_GEN"
fi

if [ "$RUN_TESTS" -eq 1 ]; then
  step "Framework tests"
  "$PYTHON" "$TESTS"
fi

step "Done"
