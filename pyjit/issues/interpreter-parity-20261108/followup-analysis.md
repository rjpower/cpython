# Fresh empty kwargs: bounded follow-up hypothesis

2026-11-08. Source-only review of a possible GIL-only fast path at
`Python/ceval.c:call_transaction_overlap_python`. This is a hypothesis for a
later experiment, not a change or an admission of the current batch.

**Finding:** a narrow primitive could in principle retain the already-checked
callable/code proof when it returns a genuinely fresh, empty kwargs dict
without an escape, callback, exception or frame change. A plain successful
`PyDict_New()` or a freelist hit is **not** that proof. The ordinary path must
keep `PyDict_New()`, `call_record_refresh()` and every final guard. The source
and aggregate counts do not yet justify implementing a second dict-creation
interface.

`PyDict_New()` calls `new_dict(Py_EMPTY_KEYS, NULL, 0, 0)` in
`Objects/dictobject.c`. `new_dict()` first tries `_Py_FREELIST_POP` and, on a
miss, calls `PyObject_GC_New`; `new_dict_impl()` initializes exact-dict fields,
clears the watcher tag and tracks the object in GC. A freelist pop calls
`_Py_NewReference` (`Include/internal/pycore_freelist.h`), which emits a
`PyRefTracer_CREATE` event when a reference tracer is installed
(`Objects/object.c`, `Include/internal/pycore_object.h`). The documented tracer
contract forbids object creation, exception changes and reentrancy, and debug
builds check release-sensitive state, but it does not prove that arbitrary C
tracers preserve this function/code/stack-space selection. An allocation miss
can invoke allocator/GC machinery. Thus a proof-preserving result would need
an explicit no-tracer, nonempty-freelist, GIL-held path whose implementation
performs the same reference renewal, dict initialization, GC tracking and
accounting without an intervening callback. A free-threaded build should
decline to the existing path. These are necessary conditions, not an approved
implementation sketch.

The overlap route pins `code` before kwargs creation. Its existing post-create
refresh and guards re-read the input homes, exact function identity and
standard vectorcall provider, function code, PEP 523 state, executable code,
stack space, defaults and argument offset. If creation can run a callback,
each remains necessary. A proof-preserving hit could omit only checks shown
redundant under its no-callback invariant; it still needs the initial shape,
capacity, owner and code pinning obligations. Until the child is current,
the current source forbids any release, callback or failure after detaching
the parent function home. The fresh dict transfers as an owned value to the
child kwargs slot, and child teardown must release it normally. Reusing one
shared empty dict or suppressing its destruction would change identity,
mutation and watcher-visible lifetime. `dict_dealloc()` can notify watchers,
can be affected by resurrection, and only then returns the exact dict to the
freelist; none of that may be moved across call publication.

On a primitive decline, the parent window must remain owning and untouched,
the code pin must remain valid, and no new exception may be left by a benign
decline. Continue through ordinary `PyDict_New()` and all current final
revalidation. An actual allocation failure must preserve the existing error
and cleanup. A callback-induced failed final guard must release the temporary
dict and pinned code, refresh the record, then follow the existing decline or
error result.
Do not turn a decline into a consuming binder or skip the native fallback.

The retained M1 diagnostic reports ten freelist allocations and ten freelist
frees per loop, with zero general object allocations and zero inline boxes;
the independent route capture reports ten allocating-overlap calls per loop.
These are aggregate object counters, not dict-specific allocation provenance,
so matching counts do not establish that all ten kwargs dicts hit the dict
freelist. They also say nothing about the cost of refresh/guards. Before a
code proposal, obtain dict-specific hit/miss and tracer-gate counts on this
route, inspect the resulting GCC hot path, and test callback mutation,
observer-visible dict identity/lifetime, allocation failure and decline
cleanup. No speedup is estimated here. The current six-commit batch and its
qualification remain unchanged.
