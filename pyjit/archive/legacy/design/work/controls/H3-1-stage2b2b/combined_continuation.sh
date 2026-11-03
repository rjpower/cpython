#!/bin/sh
set -eu
repo=$(pwd)
export UV_CACHE_DIR=/home/power/.cache/uv
uv run --no-project -- timeout -k 30 600 boil/tools/gate_run.sh build/default/.gate.lock "$repo/build/default/python" uv run --no-project --python /usr/bin/python3 -- python build/H3-1-stage2b2b/run_ast_final.py default -- -m test > build/H3-1-stage2b2b/final-ast-default.log 2>&1
uv run --no-project -- timeout -k 30 1200 boil/tools/gate_run.sh build/debug/.gate.lock "$repo/build/debug/python" sh -c 'make -C build/debug -s -j8' h3-build -m test > build/H3-1-stage2b2b/combined-build-debug.log 2>&1
uv run --no-project -- timeout -k 30 180 boil/tools/gate_run.sh build/debug/.gate.lock "$repo/build/debug/python" uv run --no-project --python /usr/bin/python3 -- python build/H3-1-stage2b2b/run_frozen_probe.py debug eligibility-combined eligibility_probe.py -- -m test > build/H3-1-stage2b2b/eligibility-combined-debug-outer.log 2>&1
GATE_NO_BUILD=1 GATE_JOBS=2 GATE_LIMIT=1200 uv run --no-project -- boil/tools/gate.sh 0 build/debug > build/H3-1-stage2b2b/combined-smoke-debug.log 2>&1
GATE_JOBS=2 GATE_LIMIT=1200 uv run --no-project -- boil/tools/gate.sh 1 build/debug test_generated_cases test_dis test_ir test__opcode test_peepholer test_compile test_monitoring test_code test_marshal test_sys_settrace test_sys_setprofile > build/H3-1-stage2b2b/combined-targeted-debug.log 2>&1
for pair in 'allocation-final run_allocation_probe_final.py' 'synthetic-final synthetic_lines_final.py' 'no-monitor-final no_monitoring_flag.py' 'global-oom-final global_oom.py' 'storage-controls-final2 storage_controls_final_v2.py'; do
    set -- $pair
    uv run --no-project -- timeout -k 30 600 boil/tools/gate_run.sh build/debug/.gate.lock "$repo/build/debug/python" uv run --no-project --python /usr/bin/python3 -- python build/H3-1-stage2b2b/run_frozen_probe.py debug "$1" "$2" -- -m test > "build/H3-1-stage2b2b/$1-debug-outer.log" 2>&1
done
uv run --no-project -- timeout -k 30 600 boil/tools/gate_run.sh build/debug/.gate.lock "$repo/build/debug/python" uv run --no-project --python /usr/bin/python3 -- python build/H3-1-stage2b2b/run_ast_final.py debug -- -m test > build/H3-1-stage2b2b/final-ast-debug.log 2>&1
PYTHON_GCVERIFY=kill PYTHON_GCROOTSTATS="$repo/build/H3-1-stage2b2b/roots-combined.jsonl" GATE_JOBS=2 GATE_LIMIT=1200 uv run --no-project -- boil/tools/gate.sh 1 build/debug test_generated_cases test_dis test_ir test__opcode test_peepholer test_compile test_monitoring test_code test_marshal test_sys_settrace test_sys_setprofile --python "$repo/build/debug/python $repo/build/H3-1-stage2b2b/assert-kill-worker.py" --fail-env-changed > build/H3-1-stage2b2b/combined-kill-debug.log 2>&1
