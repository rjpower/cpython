#!/bin/sh
# The gate ladder of boil/PROCESS.md section 3.
#
# usage: boil/tools/gate.sh LEVEL BUILDDIR [targeted test files...]
#   0  build, python -c pass, smoke test set
#   1  the targeted test files given on the command line
#   2  full suite (release build)
#   3  full suite (debug build), -u cpu test_cext, regen leaves the tree unchanged
#   4  internal benchmark set recorded as boil/bench/results/$GATE_LABEL.json
# Environment: GATE_JOBS (default 16), GATE_NO_BUILD=1 (level 0 skips make),
# GATE_LABEL (level 4).  A lock per build directory serialises builds and
# test runs on it.  A fresh pyc cache prefix is used for every run.
set -eu
level=$1; build=$2; shift 2
REPO=$(cd "$(dirname "$0")/../.." && pwd)
build=$(cd "$build" && pwd)
py="$build/python"
J=${GATE_JOBS:-16}
tmp=$(mktemp -d /tmp/boilgate-XXXXXX)
trap 'rm -rf "$tmp"' EXIT
export PYTHONPYCACHEPREFIX="$tmp/pyc"
lock="$build/.gate.lock"
# test_sys's remote-exec test times out on this machine on every build since
# before the fork (a machine-local failure); it is ignored unless GATE_IGNORE
# overrides it.
IGNORE=${GATE_IGNORE:-test_remote_exec_deleted_static_executable}
run() { flock "$lock" "$@"; }
case $level in
0)
    [ "${GATE_NO_BUILD:-0}" = 1 ] || run make -C "$build" -s -j"$J"
    run "$py" -c pass
    run "$py" -m test -q --timeout 600 -j"$J" -i "$IGNORE" test_grammar test_opcodes test_builtin \
        test_dict test_list test_tuple test_class test_generators test_capi ;;
1)
    [ $# -gt 0 ] || { echo "gate 1 needs test files" >&2; exit 2; }
    run "$py" -m test -q --timeout 600 -j"$J" -i "$IGNORE" "$@" ;;
2)
    run "$py" -m test -q --timeout 900 -j"$J" -i "$IGNORE" ;;
3)
    run "$py" -m test -q --timeout 1200 -j"$J" -i "$IGNORE"
    run "$py" -m test -q -u cpu test_cext
    before=$(git -C "$REPO" status --porcelain)
    run make -C "$build" -s regen-cases clinic
    after=$(git -C "$REPO" status --porcelain)
    [ "$before" = "$after" ] || { echo "regen changed the tree:"; git -C "$REPO" status --short; exit 1; } ;;
4)
    [ -n "${GATE_LABEL:-}" ] || { echo "gate 4 needs GATE_LABEL" >&2; exit 2; }
    [ -x "$REPO/boil/bench/run.py" ] || [ -f "$REPO/boil/bench/run.py" ] || { echo "boil/bench/run.py missing (step B)" >&2; exit 2; }
    mkdir -p "$REPO/boil/bench/results"
    run python3 "$REPO/boil/bench/run.py" --build "$build" --label "$GATE_LABEL" --set all --metric instr,rss \
        -o "$REPO/boil/bench/results/$GATE_LABEL.json" ;;
*)  echo "unknown level $level" >&2; exit 2 ;;
esac
echo "gate $level passed on $build"
