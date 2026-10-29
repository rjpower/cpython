#!/bin/sh
# gate_run.sh LOCK PY CMD...: runs CMD under the build directory's lock, then
# kills every process still running PY.  gate.sh runs this under `timeout`,
# which signals the process group at the ceiling; regrtest starts each worker
# in a new session (libregrtest/worker.py), so the group signal does not reach
# the workers and they survived their run.  Holding the lock means every
# process running this build's python belongs to this run, so sweeping by
# executable is exact.  The sweep also runs when the shell that started
# gate.sh is killed, because this process outlives it.
#
# A test run (CMD contains "-m test") also takes one of GATE_SLOTS (default
# 2) machine-wide slots, /tmp/boilgate-slot.N, before the build lock: three
# agents' suites at once put the load average at 35 on 2026-10-28 and two
# failures were load artefacts (a 600 s timeout, a test deleting another
# run's perf maps).  The slot is taken before the build lock on every path,
# so a run never waits for a build lock while another waits for its slot.
#
# The signal is not ignored here: an ignored signal is inherited through exec
# and would make the test run itself immune.  sh runs the trap after the
# foreground command, which the same group signal has already ended.
lock=$1; py=$2; shift 2
locked=0
pat="^$py( |\$)"
sweep() {
    for _ in 1 2 3 4 5; do pkill -TERM -f "$pat" 2>/dev/null || return 0; sleep 1; done
    pkill -KILL -f "$pat" 2>/dev/null; return 0
}
trap 'if [ "$locked" = 1 ]; then sweep; fi; exit 143' TERM HUP INT
case " $* " in
*" -m test "*)
    slots=${GATE_SLOTS:-2}
    while :; do
        i=1
        while [ "$i" -le "$slots" ]; do
            # fd 9 holds the slot for the rest of this process
            exec 9>"${TMPDIR:-/tmp}/boilgate-slot.$i"
            if flock -n 9; then break 2; fi
            exec 9>&-
            i=$((i + 1))
        done
        sleep 5
    done ;;
esac
# Keep the build lock through cleanup: releasing it after CMD lets the
# next gate start Python processes that this gate's sweep would kill.
exec 8>"$lock"
flock 8 || exit $?
locked=1
"$@"; rc=$?
sweep
exit $rc
