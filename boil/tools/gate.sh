#!/bin/sh
# The gate ladder of boil/PROCESS.md section 3.
#
# usage: boil/tools/gate.sh LEVEL BUILDDIR [targeted test files...]
#   0  build, python -c pass, smoke test set
#   1  the targeted test files given on the command line (regrtest options
#      such as -m PATTERN may follow them)
#   2  full suite (release build)
#   3  full suite (debug build), -u cpu test_cext, regen leaves the tree unchanged,
#      Tools/boil/traverse_audit.py --check
#   4  internal benchmark set recorded as boil/bench/results/$GATE_LABEL.json
# Levels 2/3 accept only the optional regrtest pair --python WORKER_COMMAND,
# for an instrumented worker launcher; neither accepts a narrowed test set.
# Levels 2 and 3 also sum the live collector's root counters over the suite
# and fail on a stack-walk fallback (note 11 section 10).
# Environment: GATE_JOBS (default 16), GATE_NO_BUILD=1 (level 0 skips make),
# GATE_LABEL (level 4), GATE_LIMIT (seconds; the ceiling for one command of
# the level, default by level below).  A lock per build directory serialises
# builds and test runs on it.  A fresh pyc cache prefix is used for every run.
#
# Every command runs under `timeout` in its own process group, so the test
# workers die with the run: at the ceiling, when this script is signalled,
# and when the shell that started this script is killed (the timeout process
# survives it and fires at the ceiling).  Orphaned regrtest workers held
# build directories for hours before this.
# The body is one function so that the shell parses the whole file before
# running anything; editing this file while a gate runs then cannot break
# the running gate.
main() {
set -eu
level=$1; build=$2; shift 2
case $level in
2|3)
    if [ "$#" -ne 0 ]; then
        if [ "$#" -ne 2 ] || [ "$1" != --python ]; then
            echo "gate $level accepts only --python WORKER_COMMAND" >&2
            exit 2
        fi
    fi ;;
esac
REPO=$(cd "$(dirname "$0")/../.." && pwd)
build=$(cd "$build" && pwd)
echo "gate $level starting on $build at $(git -C "$REPO" rev-parse --short HEAD)"
py="$build/python"
J=${GATE_JOBS:-16}
case $level in 0) LIMIT=1200 ;; 1) LIMIT=2400 ;; 2) LIMIT=3600 ;; 3) LIMIT=5400 ;; *) LIMIT=7200 ;; esac
LIMIT=${GATE_LIMIT:-$LIMIT}
# The pyc prefix stays on TMPDIR's filesystem (test_import renames a cached
# pyc into TMPDIR, which fails across devices). Each run removes only its
# own prefix: pruning other runs races with their cleanup and may delete a
# still-running gate's cache.
gatetmp="${TMPDIR:-/tmp}/boilgate-pyc"
mkdir -p "$gatetmp"
tmp=$(mktemp -d "$gatetmp/run-XXXXXX")
child=
cleanup() { [ -n "$child" ] && kill -TERM -- -"$child" 2>/dev/null; rm -rf "$tmp"; }
trap cleanup EXIT
trap 'cleanup; exit 143' INT TERM HUP
export PYTHONPYCACHEPREFIX="$tmp/pyc"
# An ASan build runs with fake stacks off unless the caller says otherwise:
# fake stacks hide escaped locals from the conservative scan (the ASan line
# of record, note 11 section 8.2) and make deep recursion quadratic once a
# size class fills (test_list's test_repr_deep exceeds 240 s with them on,
# 5 s off; 7a rung 1's record has the times).
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_stack_use_after_return=0:detect_leaks=0}"
# The suite's root counters, one line per process (root_stats.py), on the
# branches that have the tool
stats_tool="$REPO/boil/tools/measure/root_stats.py"
export PYTHON_GCROOTSTATS="${PYTHON_GCROOTSTATS:-$tmp/roots.jsonl}"
roots() { [ ! -f "$stats_tool" ] || python3 "$stats_tool" check "$PYTHON_GCROOTSTATS"; }
lock="$build/.gate.lock"
# test_sys's remote-exec test times out on this machine on every build since
# before the fork (a machine-local failure); it is ignored unless GATE_IGNORE
# overrides it.
IGNORE=${GATE_IGNORE:-test_remote_exec_deleted_static_executable}
# timeout makes itself the leader of a new process group and signals the
# whole group at the ceiling; gate_run.sh takes the lock, runs the command
# and sweeps the build's python processes afterwards (regrtest's workers
# live in their own sessions and would survive the group signal).
run() {
    timeout -k 30 "$LIMIT" "$REPO/boil/tools/gate_run.sh" "$lock" "$py" "$@" &
    child=$!
    wait "$child"; rc=$?
    child=
    [ "$rc" -ne 124 ] || echo "gate $level: ceiling of ${LIMIT}s reached" >&2
    return "$rc"
}
case $level in
0)
    [ "${GATE_NO_BUILD:-0}" = 1 ] || run make -C "$build" -s -j"$J"
    run "$py" -c pass
    run "$py" -m test -q --timeout 600 -j"$J" -i "$IGNORE" test_grammar test_opcodes test_builtin \
        test_dict test_list test_tuple test_class test_generators test_capi ;;
1)
    [ $# -gt 0 ] || { echo "gate 1 needs test files" >&2; exit 2; }
    # The caller's arguments go before -i: regrtest's default verdict for a
    # test no pattern names is the negation of the first pattern's kind, so
    # an -i before a -m makes every unmatched test run (filter.py match_test).
    run "$py" -m test -q --timeout 600 -j"$J" "$@" -i "$IGNORE" ;;
2)
    run "$py" -m test -q --timeout 900 -j"$J" -i "$IGNORE" "$@"
    roots ;;
3)
    run "$py" -m test -q --timeout 1200 -j"$J" -i "$IGNORE" "$@"
    roots
    run "$py" -m test -q -u cpu test_cext
    # Status and content: a generated file already modified before the
    # regen keeps the same status line when the regen changes it again.
    before=$(git -C "$REPO" status --porcelain; git -C "$REPO" diff HEAD | sha256sum)
    run make -C "$build" -s regen-cases clinic
    after=$(git -C "$REPO" status --porcelain; git -C "$REPO" diff HEAD | sha256sum)
    [ "$before" = "$after" ] || { echo "regen changed the tree:"; git -C "$REPO" status --short; exit 1; }
    [ ! -f "$REPO/Tools/boil/traverse_audit.py" ] || python3 "$REPO/Tools/boil/traverse_audit.py" --check ;;
4)
    [ -n "${GATE_LABEL:-}" ] || { echo "gate 4 needs GATE_LABEL" >&2; exit 2; }
    [ -x "$REPO/boil/bench/run.py" ] || [ -f "$REPO/boil/bench/run.py" ] || { echo "boil/bench/run.py missing (step B)" >&2; exit 2; }
    mkdir -p "$REPO/boil/bench/results"
    run python3 "$REPO/boil/bench/run.py" --build "$build" --label "$GATE_LABEL" --set all --metric instr,rss \
        -o "$REPO/boil/bench/results/$GATE_LABEL.json" ;;
*)  echo "unknown level $level" >&2; exit 2 ;;
esac
echo "gate $level passed on $build"
}
main "$@"; exit $?
