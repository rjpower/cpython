#!/usr/bin/env bash
# Coordinator-only landing check. Fail on the actual command status.
set -euo pipefail

tag=${1:?usage: landing.sh TAG [TEST ...]}
shift
runtime_build=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build
source_tree=$runtime_build/worktrees/pyvalue-rc-universal
logs=$runtime_build/pyjit-link-wave1/checkpoint-closure/$tag
mkdir -p "$logs"
exec > >(tee "$logs/landing.log") 2>&1
printf 'Source: '
git -C "$source_tree" log -1 --oneline
git -C "$source_tree" diff --stat HEAD
export UV_CACHE_DIR=/home/power/.cache/uv

run_stage() {
    local name=$1
    shift
    printf '%s: started\n' "$name"
    if "$@" > "$logs/$name.log" 2>&1; then
        printf '%s: PASS\n' "$name"
    else
        local rc=$?
        tail -60 "$logs/$name.log"
        printf '%s: FAIL (%s)\n' "$name" "$rc"
        return "$rc"
    fi
}

debug=$runtime_build/pyvalue-rc-repr-debug
run_stage regen make -C "$debug" -j6 -s regen-ast clinic regen-cases
run_stage lint make -C "$debug" check-value-api
for variant in debug release stats; do
    run_stage "$variant-build" make -C "$runtime_build/pyvalue-rc-repr-$variant" -j6 -s
done
run_stage limited-abi make -C "$debug" -j6 -s regen-limited-abi
run_stage frozenmain uv run --no-project --python /usr/bin/python3 -- \
    "$debug/python" "$source_tree/Programs/freeze_test_frozenmain.py" \
    "$source_tree/Programs/test_frozenmain.h"
if ! git -C "$source_tree" diff --quiet HEAD -- Programs/test_frozenmain.h; then
    run_stage frozenmain-build make -C "$debug" -j6 -s
fi
tests=()
if [[ ${PYJIT_LANDING_FULL:-0} != 1 ]]; then
    mapfile -t tests < <(printf '%s\n' \
        test_ir test_dis test_code test_compile test_import test_capi \
        test_value_slots test_tools test_sys test_embed test_generators \
        test_coroutines test_asyncio.test_tasks test_asyncio.test_futures \
        "$@" | sort -u)
fi
for variant in release debug; do
    build=$runtime_build/pyvalue-rc-repr-$variant
    run_stage "$variant-tests" uv run --no-project --python /usr/bin/python3 -- \
        "$build/python" -m test -j6 "${tests[@]}"
    rg '^(Total tests:|Result:)' "$logs/$variant-tests.log"
done
git -C "$source_tree" diff --check
printf 'Landing check passed. Source changes remain for coordinator commit.\n'
