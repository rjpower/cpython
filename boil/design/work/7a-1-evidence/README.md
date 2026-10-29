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
