# Rung-1 landing review, third repair: evidence

The production source is merge `36562302832` plus shared getter
`0f8a019b846` and the queue-test fixup carrying this directory. The exact
source blobs checked before this commit are:

- `Lib/test/test_gc.py`: `8388e2fd1ccd56eb84781f83c1e45cd718e878c9`
- `Modules/_testinternalcapi.c`: `77ca789d73a365ecf34dfe7d1787b8abac3e0b86`
- `Python/gc_live.c`: `b940a0acf1827a338fc6635f82a47967c69b5480`

`diagnostic-controls.patch` applies only to that production gc_live.c.
It temporarily suppresses queue readiness or queued-call roots, and adds
native marked-domain events plus a 250 ms delay between the pin wakeup
and reacquiring the head lock. It is an experiment, never shipped code.
The source was restored byte-for-byte before positive gates; SHA256 of
both production and the saved source is
`b7093831a059b6a7f213190c989870fa5a492b95a8bcb9afffd5669083182488`.

`control_driver.py` executes the committed tests with one local negative
change per run. Its forwarding control reconstructs the old helper from
`0f8a019b846`, whose accessor is present but forwarding is still unfixed.
Duplicate/drop controls change reported tokens while preserving totals.
Readiness/root switches are armed inside the tested child after imports,
so startup does not supply the failure. Run with the diagnostic build:

```
PYTHON_GCVERIFY=kill uv run --no-project -- build/debug/python -B \
    boil/design/work/7a-1-evidence/control_driver.py CONTROL
```

Expected exit status is 1 for forwarding, duplicate_pin, duplicate_alloc,
no_ready and no_roots. Full local logs live in `build/queue-review3/`.
The root control detects rediscovery/requeue: 100 queued nodes but only
50 delivered unique finalizers. It does not produce a kill trap or prove
that every possible payload edge is traced. A first version applied to
the pin test passed through alternate roots; its output is retained as
`control-no_roots-pin-alternate-root.log`. The positive payload fixture
also validates heap contents after two C-only inline collections before
the first drain.

`trace_pin.py` reconstructs the reviewed pin body at `a0338fd7634` and
records collection-start/stop interpreter lists and routing counters.
The native patch records the actual pinned mark domains. The delayed
wakeup retains the owner in the domain through the next mark; it never
bypasses the pin recheck. `domain-events.txt` records representative
normal and delayed events. No stack-retention cause is inferred from
these cases.

`trace_mp.py` is a regrtest worker wrapper. It adds diagnostics only when
the existing teardown emits a dangling-thread warning: alive/active/limbo
state, Python stack, recorded conservative stack hits, Python referrers,
and the pending finalizer registry. A successful run without a warning
is a non-reproduction, not a cause or a fix.


The feeder investigation made three bounded forkserver-process runs (183
passing tests and 7 skips each), with no warning, including two during
concurrent suite load. These are non-reproductions. A controlled schedule
then identifies a real completion race: queue collection has sent the
sentinel, but the feeder is still closing its pipes when teardown checks
for dangling threads. The historical warning has no such captured state;
its exact cause remains unknown.

`feeder_completion.py` reproduces that race with a one-second Python
switch interval. `old` reconstructs only BaseMixin.tearDownClass from
exact commit `970e14edbde`; it exits 1 after the warning, while the actual
current method exits 0 after waiting for completion. `blocked` holds the
real feeder in reader.close after the sentinel, shortens this control's
wait budget to 0.1 seconds, and proves the live-thread warning survives;
it then releases and joins the feeder. `feeder-events.txt` preserves all
three observed event streams on the merged debug source. Run each in a
fresh child from the repository root:

```
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project -- \
    build/debug/python -B boil/design/work/7a-1-evidence/feeder_completion.py MODE
```

The source fix waits only for newly created QueueFeederThread objects,
using one shared monotonic deadline; the existing dangling-thread warning
and stopped-thread collection/recheck remain. No exclusion is introduced.
The first observer-triggering experiment exposed a missing
print_warning.orig_stderr attribute in the diagnostic wrapper. The
wrapper now preserves it; the failure is retained in build storage. The
three earlier warning-free suite runs did not reach that broken path.
One later attempt expired while waiting for the busy debug build lock;
it ran no control and supplies no test result.

`assert_kill_worker.py` is the roots lane's actual-state assertion,
unchanged; its shell adapter uses this worktree's debug build and
preserves interpreter flags. BOIL_MODE_EVIDENCE selects a queue-owned
JSONL log. This provides per-worker (2, 0) evidence for the final plain
kill suite. The -E control must fail with actual state (0, 0) before
running the requested worker module.

The later stress audit found that gc_alloc_in_reftracer reset the test_gc
worker's global interval to zero. Shared prerequisite 59790b545bc was
independently integrated as 02bcd2f6132 and merged here at 5576cdbd755.
Earlier environment-requested stress counts do not establish uniform
actual worker coverage. The new child prelude asserts gc_get_stress()
before the test body; the two exact-injection fixtures still disable
automatic stress afterward, as already stated above.

`control_driver.py stress_forwarding` removes only PYTHON_GCSTRESS from
the forwarding loop. Its test deliberately removes the parent verifier
environment override: otherwise that kwarg would itself disable -I and
could hide missing stress forwarding. The control fails with actual child
stress 0 instead of 1000; no setter repairs the state. The positive test
checks inherited 1000 and an explicit-off override.

`assert_stress_worker.py` and its portable shell adapter require actual
1000 at worker entry and in finally after runpy, recording the true test
list/filter, profile, PID, verifier extras, flags and source/config hashes.
They preserve a failing worker's nonzero result and contain no setter.
The -E control fails at 0; the portable positive control passes the two
getter/reftracer tests with one matching 1000 start/end pair. Full stress
verification will use this adapter with BOIL_STRESS_RUN and
BOIL_STRESS_EVIDENCE selecting the run and queue-owned JSONL destination.
