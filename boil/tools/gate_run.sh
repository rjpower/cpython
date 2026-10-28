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
# The signal is not ignored here: an ignored signal is inherited through exec
# and would make the test run itself immune.  sh runs the trap after the
# foreground command, which the same group signal has already ended.
lock=$1; py=$2; shift 2
pat="^$py( |\$)"
sweep() {
    for _ in 1 2 3 4 5; do pkill -TERM -f "$pat" 2>/dev/null || return 0; sleep 1; done
    pkill -KILL -f "$pat" 2>/dev/null; return 0
}
trap 'sweep; exit 143' TERM HUP INT
flock "$lock" "$@"; rc=$?
sweep
exit $rc
