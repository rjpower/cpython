#!/usr/bin/env bash
set -euo pipefail

source_dir=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/w5
main_dir=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal
build_dir=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/w5-release
artifact_dir=/home/power/code/cpython/.worktrees/python-jit/pyjit/issues/checkpoint-closure
scratch_dir=$(mktemp -d /tmp/closure-binary.XXXXXX)
files=(Python/flowgraph.c Python/compile.c Include/internal/pycore_compile.h)

for file in "${files[@]}"; do
    mkdir -p "$scratch_dir/$(dirname "$file")"
    cp "$source_dir/$file" "$scratch_dir/$file"
done

restore() {
    rc=$?
    trap - EXIT
    set +e
    for file in "${files[@]}"; do
        cp "$scratch_dir/$file" "$source_dir/$file" || rc=1
    done
    make -C "$build_dir" -j6 all > "$artifact_dir/compiler-binary-w5-restore-build.log" 2>&1 || rc=1
    rm -rf "$scratch_dir"
    exit "$rc"
}
trap restore EXIT

for file in "${files[@]}"; do
    cp "$main_dir/$file" "$source_dir/$file"
done
git -C "$source_dir" apply "$artifact_dir/w5-cold-compiler-binary.patch"
make -C "$build_dir" -j6 all > "$artifact_dir/compiler-binary-w5-overlay-build.log" 2>&1
"$build_dir/python" -m test test_compile test_ast > "$artifact_dir/compiler-binary-w5-focused-tests.log" 2>&1
"$build_dir/python" - <<'PY' > "$artifact_dir/compiler-binary-w5-arithmetic-probe.log" 2>&1
import dis
import math

cases = {
    'x = 2*3': 6,
    'x = 2**10': 1024,
    'x = 1<<17': 131072,
    'x = 25%7': 4,
    'x = 0*123456789123456789123456789': 0,
    'x = 3*"abc"': 'abcabcabc',
    'x = (1, 2)*3': (1, 2, 1, 2, 1, 2),
}
for source, expected in cases.items():
    namespace = {}
    exec(compile(source, '<fold-probe>', 'exec'), namespace)
    assert namespace['x'] == expected, source

namespace = {}
exec(compile('x = (-1)**0.5', '<fold-probe>', 'exec'), namespace)
assert abs(namespace['x'] - 1j) < 1e-15

for source in ('x = 2**1000', 'x = 1<<1000', 'x = "x"*4097'):
    code = compile(source, '<refusal-probe>', 'exec')
    assert any(ins.opname == 'BINARY_OP' for ins in dis.get_instructions(code)), source
    namespace = {}
    exec(code, namespace)
    assert namespace['x'] is not None

code = compile('x = (1, 1.0, -0.0, 0.0)', '<cache-probe>', 'exec')
namespace = {}
exec(code, namespace)
assert math.copysign(1.0, namespace['x'][2]) == -1.0
assert math.copysign(1.0, namespace['x'][3]) == 1.0
print('arithmetic, refusal, and cache probes passed')
PY
