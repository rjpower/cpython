#!/bin/sh
# Run the plan's pyperformance subset against an interpreter that has no ssl.
#
# pyperformance creates its venv with the target interpreter's pip, which
# cannot reach PyPI without ssl. So the venv is created first with the host
# uv at the exact path pyperformance derives from the interpreter, seeded with
# the package set recorded in boil/tools/pyperf-requirements.txt. pyperformance
# then finds it ("trying common venv first") and runs.
#
# usage: boil/tools/pyperf_run.sh <name> <interpreter> [pyperformance run args]
#   writes boil/bench/<name>.json and boil/bench/run-<name>.log
set -eu
REPO=$(cd "$(dirname "$0")/../.." && pwd)
name=$1; python=$(realpath "$2"); shift 2
BENCHES=nbody,spectral_norm,float,richards,raytrace,deltablue,chaos,go,nqueens,fannkuch,unpack_sequence,generators,json_dumps,json_loads,regex_compile,pickle,unpickle,xml_etree,comprehensions,coroutines,deepcopy,logging
root="$REPO/build/pyperf-venvs/$name"
mkdir -p "$root"
cd "$root"
venv=$(uv tool run pyperformance venv show -p "$python" 2>/dev/null | grep -o 'venv/cpython[^ ]*' | head -1)
if [ -z "$venv" ]; then echo "could not determine venv path" >&2; exit 1; fi
if [ ! -x "$root/$venv/bin/python" ]; then
    uv venv --python "$python" --seed "$root/$venv"
    uv pip install --python "$root/$venv/bin/python" -r "$REPO/boil/tools/pyperf-requirements.txt"
fi
uv tool run pyperformance run -p "$python" --benchmarks "$BENCHES" \
    -o "$REPO/boil/bench/$name.json" "$@" > "$REPO/boil/bench/run-$name.log" 2>&1
