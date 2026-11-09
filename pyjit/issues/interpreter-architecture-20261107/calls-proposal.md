# Call transaction: staged implementation proposal

2026-11-07, baseline `7780e32e0e2`. Pending independent review and coordinator
baseline freeze; no runtime edits yet. Runtime file ownership: `Python/ceval.c`
and narrowly related call tests. Shared headers, generators and builds belong
to the coordinator. Each stage is separately reviewed, integrated and screened.

## 1. Remove write-only record state

Delete `_PyCallTransaction.effects`, its `.effects` initializer, and its five
writes in the method preparation, EX preparation, monitoring, native call and
Python fallback paths. In `call_record_expand_method`, preserve the helper call,
its status conversion and setting `prepared`; only delete the conditional
whose sole body writes the dead field. The generated handler's `call_state` and
`CALL_EFFECTS` are separate, live protocol: the cases generator uses them to
choose fallback rather than replay a side effect after a guard failure. Keep
those bits, definitions, producers, guards, `prepared`, `monitored`, periodic
state and all refreshes. Do not infer a performance improvement from deletion.

Acceptance: source search for all record field reads/writes and references;
release compile; targeted call, instrumentation/monitoring, method/EX and
error/reentry tests. Verify generated output and `CALL_EFFECTS` uses remain
unchanged. Full suites belong at a later integrated batch point.

## 2. Isolate the consuming provider

Split the current `call_transaction_push_python` immediately after
`call_transaction_overlap_python` returns 1. A private `Py_NO_INLINE` fallback
helper receives the same transaction/output pointers and contains the existing
slow provider body, including `small[16]`, unchanged. The wrapper returns
`-1` or `0` directly for failure/published child and calls the helper only for
1. Preserve `call->effects` only if stage 1 is not integrated; otherwise omit
its dead write. Keep all other functions and their sequence unchanged.

The parent window remains owning throughout fallible preparation and consuming
binding. On success the slow provider detaches the original window, publishes
child, retires values in the same reverse order and performs the same
refresh/cleanup on failure. A decline before detachment still returns 1. The
wrapper must not interpret an error as a decline or replay expansion or
monitor events. No call ABI, opcode, frame layout, generated product, fallback
ownership or GIL-disabled behavior changes. Review that `Py_NO_INLINE` gives
a real separate symbol and does not inadvertently force an extra successful
common-path call: inline decisions remain a GCC result, not a source promise.

The retained GCC ELF `8c621301…` places a 248-byte allocation and stack canary
before the overlap guards; its annotated logging profile also samples header
initialization, revalidation, NULL-tail and varargs work, so moving the array
is a bounded hypothesis, not an explanation for all cost. Inspect changed
common and declined disassembly, canary, frame size, calls, spills and code
size. The outlined fallback may cost more for declined shapes; compare the
matched screen including logging_silent, deltablue and non-call-heavy controls.
The coordinator owns each shared build and integration measurement.

Existing call statistics `frames_pushed` and `inlined_py_calls` do not separate
common overlap accept, decline reasons or bound argument shapes, and sampled
IPs are not invocation counts. No existing trustworthy shape denominator was
identified. Ask the parallel inventory worker for its evidence; if deciding
between further call mechanisms depends on it, propose separately reviewed
`Py_STATS`-only counters with explicit totals and no release-path traffic. The
mechanical boundary itself needs no new permanent counter.

## 3. Known empty-varargs value, separate decision

The common path currently constructs the immortal empty tuple through
`PyValue_FromObjectBorrow` plus `PyValue_DUP`, causing type and immortality
checks in the retained GCC sequence. The existing
`_PyValue_STATIC_IMMORTAL(op)` initializer builds the same owned tag (address
plus `Py_TAG_REFCNT`), and `_Py_SINGLETON(tuple_empty)` has `PyTuple_Type` and
`_PyVarObject_HEAD_INIT`, whose GIL and free-threaded initializers both mark
static immortality. Exact tuple is neither exact int nor float, so canonical
conversion never inlines it; duplication of an immortal tagged value takes no
count. After review, use the existing initializer for a local `PyValue` in
this guarded empty-varargs branch and assign that owner to the child slot;
no bespoke bit construction or new shared API. The common path currently
excludes free-threaded execution; verify initializer assumptions on both build
modes rather than generalizing that path.

Keep separate from the fallback split so changed GCC work and any measurement
can be attributed. Focus tests on repeated empty varargs, tuple identity,
retained/suspended generator and exposed frames, GC, finalizers and argument
errors; run debug and release via coordinator. Check the generated assembly for
removed conversion checks and inspect for new work. No speedup is presumed.
If the static initializer has a hidden restriction on automatic storage or its
ownership proof is disputed, drop this stage until an existing typed API can
express the value; do not expand header ownership without coordinator review.

## Scope and stop rule

Do not simplify guard state merely because it looks repeated. Any extra change
must identify the observation point, source proof and refreshed values after
allocation or callback, receive independent review and be measured separately.
First finish the record cleanup and the reviewed provider boundary. Preserve
unrelated runtime work and all designated ctypes/LRU holds.
