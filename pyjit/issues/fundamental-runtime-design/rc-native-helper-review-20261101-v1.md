# Independent RC first-feedback helper review

2026-11-01. **ACCEPT host readiness for one separately activated first-feedback
batch.** No concrete helper blocker remains. This accepts preparation, not an RC
build, runtime adoption, verifier qualification or performance result.

Reviewed source `d7df12b1d81722927e71d9734c70eeafdccfee73` and frozen helpers at
`build/worktrees/rc-successor/build/rc-first-feedback-v1`. Exact SHA256 pins:

| Artifact | SHA256 |
| --- | --- |
| plan.json | c0b2da6cf99266ed40a98b9346126b236909ddaea10d2142d7109ba799a33f44 |
| launch-held.json | 6b8247420b1fdc0f5dcda52cbdbd800e079f5e0506f30fe820c4090282a1cac5 |
| ready.json | c5873f4466b7460b9a521d0f209a3d13db68ef666208ea950cd3d9e44e364dd8 |
| helper.diff | 54df3ffda56ff67f3e214db85942a4e57b0f3de4d2c037ddb158c2cc90b854bb |

All 7115 plan inputs and 54 readiness artifacts match. The source remains clean
and root activation is absent. The accepted source review is pinned separately;
this review concerns the actual helper changes, not a repeat of ownership
projection review.

Four literal rows request fresh GCC debug/O1 build, debug focus, fresh GCC
release/O3 build and release focus. Both images use the declared fixed Linux,
GIL, mimalloc, computed-dispatch, historical boxed-shapes configuration with
`--with-shapes`; no option search is proposed. Build validation requires actual
configuration macros, exact configure arguments, the RC SOABI and frozen
toolchain/products. The restored core has no tracing-owner macro, call-effect
audit or linked precise/inline stamps; helpers correctly stop asserting those
unavailable interfaces. Normal fresh-build handling accepts empty/lock-only
directories, including the empty debug directory created by the host timeout
control.

All 39 exact selected method bodies and their method/class decorators match the
source-bound roster. The four new ownership/cache cases accompany compiler,
local/frame, call, generator/coroutine, cyclic GC, callback/resurrection and
weakref cases. Specialization is enabled in the fixed GIL source;
`requires_refcounting` is active, `support.Py_GC` is false, and ContainerNoGC is
provided by the required _testcapi source. Zero expected outer skips is coherent
for this configuration. Missing modules, actual skips or missing/extra IDs fail
validation. Inner test subprocesses receive no invented worker or root credit.

The wrapper uses real historical `gc_verify`, `gc_verify_count` and
`gc_verify_holes` APIs. Source confirms holes are a list of
`(type_name, address, refcount, referrer)` tuples and count is an integer.
`PYTHON_GCVERIFY=0`, environment enabled and no gcverify X-option request the
diagnostic OFF state, including debug builds whose default differs. This is
requested-policy evidence, not a fabricated actual mode getter. The wrapper
never calls gc_verify. Counts and holes remain observations, with no zero-hole
veto or verifier acceptance claim. No current tracing rootstats/stress API is
assumed.

The ordinary driver and worker record actual PIDs, source/config/CAPI identities
and both phase observations. Scheduled worker configuration and PID must match
returned results and the authoritative live unittest result; exact method IDs,
errors/failures/skips and module origins are checked. The worker shell preserves
interpreter arguments and calls its sibling wrapper. Existing observer and
authority bodies remain reused. Nonzero child exit, early dependency import
failure or missing state cannot produce successful row verification or permit
the supervisor to continue.

Independent replay of the actual host controls passed all 15 negative checks,
configuration/authority/contract positives, the synthetic wrapper, and partial
raw-output survival with cleanup on timeout. Additional reviewer controls
created a real TemporaryDirectory and exclusive output file in **each actual
root, affected, debug and release evidence directory**, verified exclusive
creation rejects a collision, then removed the temporary files. An actual
compiled-wrapper execution with _testcapi unavailable propagated
ModuleNotFoundError before any run_module call. Frozen helper bytes were
unchanged by these checks.

The inherited supervisor retains per-image and global serialization locks,
one-attempt outputs, first-failure stop, a separately activated **45-minute
inclusive** deadline, positive downward-clamped timeouts, 210-second reserve,
stream files opened before waiting, and actual post-gate process/lock cleanup.
Post-run source/config/tool/product checks remain mandatory. Root must observe
successful preactivation checks before writing activation; no activation or
native authority follows from this review alone.

Independent receipt: `build/rc-native-helper-review-20261101-v1/receipt.json`,
SHA256 `0637777f9ba08cce2a3253f8b4596b8184c844770876a0201226debe90fc524d`.
The same directory retains the exact host replay, method closure and added
reviewer checks. No compiler, target interpreter, workload, counter probe or
native test ran during this review. Canonical configure regeneration, fresh
native behavior and broader RC qualification remain outstanding.
