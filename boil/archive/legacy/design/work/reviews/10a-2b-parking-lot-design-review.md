# ROOTS2b Park ownership prerequisite design review

Verdict: **IMPLEMENT_PARK_PREREQUISITE**, 2026-10-29. This clears the bounded
design, not an implementation or landing. Review was read-only: no native
execution, compiler/build/test/probe command or runtime edit was performed.

Reviewed source is frozen ROOTS
`8ce9c57c662bf3a027d01cd0a8bfc45d3d4fb89b`. Evidence paths below are relative
to that worktree's `build/takeover-20261029/threadpool-retention/`:

| Artifact | SHA256 |
|---|---|
| `parking-lot-design.md` | `1cbc5217262cf7cd88306a3cec01e619f2d89666291f5f8a90033b8f0313a394` |
| `parking-lot-disassembly.txt` | `f8e187b94d4c4f37e6fdbb3b54a27e28baed064a619fd09b6c4b0d8886df1866` |
| `debug-cfi.txt` | `97391c3231b33410151dc0e6e744d9f39c9f52d5b6638b794847f6ecb1a5a86c` |
| Original/diagnostic `plan.json` | `18ffce6548598517c93eb76e014c601fa4e477ffabdee7898b4dc6499ecdcbc6` |

I checked these hashes, all ten original failure receipts and the separate
trace failure. The original representative fork-thread selector fails its
unchanged weakref oracle in every selection. The trace records all three
survivors directly at Park's `rsp+0` word, with zero dropped hits. At captured
PC `0x36b2fa`, disassembly and CFI give `rsp=rbp-0xa0`, `CFA=rbp+16`, hence a
176-byte extent. That lowest word is unused padding, below the lowest stored
local at `rbp-0x98`. This supports classifying the function by its ownership;
it does not justify clearing padding or changing walk bounds. Stack-hit IDs
16–19 are not proof of the exact original three collections. Post-comparison
Python frame observations remain supplementary.

`Python/parking_lot.c:324–384` owns raw synchronization state and borrows
`addr`, `expected` and `park_arg`. There is no independent Python payload in
its wait entry. Detach/attach remain collecting or reentrant calls for the
checker; no effect-table exemption is needed. The seven direct callers close
as follows, using source lines at frozen `8ce`:

| Caller | Data held across Park |
|---|---|
| `Python/lock.c:119`, mutex | Borrowed mutex, scalar expected value and caller-stack deadline/handoff record; `mutex_unpark` changes only scalar/raw state. |
| `Python/lock.c:320`, event | Borrowed event and scalar expected value; no park payload. |
| `Python/lock.c:380`, once flag | Borrowed flag and scalar expected value; initializer callback is outside Park. Caller retains its callback argument. |
| `Python/lock.c:488`, rwmutex | Borrowed mutex and scalar expected value; no park payload. |
| `Python/pystate.c:2054`, attach wait | Runtime-owned thread state and scalar expected value, with `detach=0`. |
| `Python/pylifecycle.c:2301`, finalization guard | Runtime-owned interpreter and scalar guard count; no park payload. |
| `Modules/_queuemodule.c:418`, SimpleQueue.get | Caller-owned writable `PyObject *item` output slot, initialized NULL at line 417. |

The SimpleQueue handoff is the important object case. The attached producer's
`maybe_handoff_item` writes `Py_NewRef(data->item)` into the opaque
`get_impl` caller slot without allocation (`_queuemodule.c:270–279`), before
bucket unlock and semaphore wakeup (`parking_lot.c:398–407`). Park's pinned
record persists through GIL acquisition and is invalidated only after
attachment (`pystate.c:2089–2103`). The walk passes beyond precise Park into
the opaque caller, retaining caller-owned saved registers as well
(`gc_walk.c:941–976`). The published item therefore remains in scanned caller
storage while the consumer is unattached. After attachment the live caller
continues to own it through return. This proof does not depend on scanning
the `void *park_arg` value as a Python object.

The smallest production scope is Park's `Py_GC_PRECISE` annotation and a
comment, plus the borrowed-storage/output-slot obligation in
`Include/internal/pycore_parking_lot.h`. All callers remain opaque. Future
3a conversion of `get_impl` must explicitly anchor its required inputs and
register the writable output slot before Park, keeping registration through
reattachment and return/error cleanup. This review grants no such conversion.

The bounded native fixture belongs in `Modules/_testinternalcapi/gc_walk.c`
with its Python test in `Lib/test/test_gc.py`. Its frozen implementation
must satisfy these requirements:

1. Notify readiness while the consumer holds the GIL, then immediately enter
   actual Park with `detach=1`; no intervening detaching helper or second
   Park may replace the measured capture. The producer cannot regain the
   GIL until this Park releases it.
2. Use allocation-free scan/frame callbacks. Both walks must identify the
   same valid, pinned capture starting in actual Park, classify Park precise,
   cover the opaque caller's exact slot with its expected value, and report
   no fallback. The unpark callback writes the second sentinel without
   collecting; the producer retains the GIL through the second walk.
3. Label that second observation **payload published, unpark issued,
   consumer unattached**. It does not prove the consumer has executed the
   semaphore return or reached `take_gil`.
4. Old Park classification must fail the precise-frame assertion. Making
   only the test caller precise must fail exact-slot coverage. Always
   unpark/join and restore temporary range registration, including on
   negative-control failure. Matching a sentinel elsewhere is insufficient.

This structural omitted-root control is appropriate to the classification
change; it does not prove target-object GC survival. A separate real-object
handoff fixture remains optional and unclaimed. Existing detached-register
modes 0/1/2 and the unchanged three thread-family lifetime selectors provide
the complementary caller-register and reclamation checks. Preserve their
original assertions and the old failures, then bind candidate results to
new source/native/helper identities. Actual section classification, inline
audit, source checker and restoration evidence remain implementation gates.

The coordinator subsequently authorized an ordinary primary `64ab` merge and
this exact bounded implementation/control scope. That resulting source has
not been reviewed here. New qualification plans are required after the source
change; original `8ce` evidence and historical verifier/import hashes stay
preserved. Full debug failure, unrun kill/stress profiles and global O1 policy
are not cleared by this design verdict. No Queue2 work is included.
