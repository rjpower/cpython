#!/bin/sh
set -eu
repo=$(pwd)
export UV_CACHE_DIR=/home/power/.cache/uv
for build in default debug; do
    if [ "$build" = debug ]; then
        uv run --no-project -- timeout -k 30 1200 boil/tools/gate_run.sh "build/$build/.gate.lock" "$repo/build/$build/python" sh -c 'make -C "$1" -s -j8' h3-build "build/$build" -m test > "build/H3-1-stage2b2b/combined-build-$build.log" 2>&1
    fi
    GATE_NO_BUILD=1 GATE_JOBS=2 GATE_LIMIT=1200 uv run --no-project -- boil/tools/gate.sh 0 "build/$build" > "build/H3-1-stage2b2b/combined-smoke-$build.log" 2>&1
    GATE_JOBS=2 GATE_LIMIT=1200 uv run --no-project -- boil/tools/gate.sh 1 "build/$build" test_generated_cases test_dis test_ir test__opcode test_peepholer test_compile test_monitoring test_code test_marshal test_sys_settrace test_sys_setprofile > "build/H3-1-stage2b2b/combined-targeted-$build.log" 2>&1
    uv run --no-project -- sh build/H3-1-stage2b2b/final_controls.sh "$build"
    uv run --no-project -- timeout -k 30 600 boil/tools/gate_run.sh "build/$build/.gate.lock" "$repo/build/$build/python" uv run --no-project --python /usr/bin/python3 -- python build/H3-1-stage2b2b/run_ast_final.py "$build" -- -m test > "build/H3-1-stage2b2b/final-ast-$build.log" 2>&1
    if [ "$build" = debug ]; then
        uv run --no-project -- timeout -k 30 180 boil/tools/gate_run.sh "build/$build/.gate.lock" "$repo/build/$build/python" uv run --no-project --python /usr/bin/python3 -- python build/H3-1-stage2b2b/run_frozen_probe.py "$build" eligibility-combined eligibility_probe.py -- -m test > "build/H3-1-stage2b2b/eligibility-combined-$build-outer.log" 2>&1
    fi
done
