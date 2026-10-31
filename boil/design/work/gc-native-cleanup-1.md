# Native suspended-generator cleanup: source checkpoint

2026-10-30. **Source only; no build or native test has run.** Branch
`boil/gc-native-cleanup` starts at primary
`9c3c0d57424668714ace4afff25fb0fe970f8254`. This is the bounded implementation of
[the approved handoff](gc-native-cleanup-next.md), not a new cleanup protocol.
The five code/test files are `Objects/genobject.c`,
`Include/internal/pycore_genobject.h`, `Python/gc_live.c`,
`Lib/test/test_gc.py`, and `Lib/test/test_generators.py`.

## Contract and source proof

`gen_close_clears_frame` shares the existing public `close()` proof: a RESUME
instruction with the original depth-one operand and no unwind tools. Public
close retains the same sequence. Collector admission additionally requires an
exact ordinary suspended generator, generator-owned storage, and no escaped
frame. Created, running, delegated, coroutine, async-generator and escaped-frame
cases retain the old path.

`native_cleanup_ready` makes a whole-pass choice. Any eligible other finalizer,
legacy destructor, first-pass deallocator, destroy hook, registered weakref,
memoryview/managed buffer, queued work, debug/callback observer, tracer/profiler,
monitoring tool or custom eval-frame hook rejects the cohort. The supported
domain must contain only the main interpreter. Actual finalizer eligibility is
checked dynamically after the death-kind prefilter; finalized and no-op
generators are not made pending again.

Admission changes no object or graph. `order_dead_first` preparation and a
second complete admission check precede commit. On an admission mismatch the
temporary order is discarded and the unchanged slow preparation runs. A
preparation failure retains the original no-sweep rollback. There is no
fallback after clearing the first frame.

The admitted route has an empty batch. `route_calls` therefore returns from
`queue_splice` without allocating. The test-only deletion barrier cannot find a
foreign deleting interpreter in the admitted one-interpreter domain.
`unpin_domain` uses the existing HEAD lock with `_Py_LOCK_DONT_DETACH`; its body
only changes pin counters and notifies the pin event. No Python is called.
An explicit `Py_GIL_DISABLED` admission rejection selects the old path on an
unsupported free-threaded build. This proof is for the supported GIL build,
not arbitrary custom RAW allocator callbacks.

Sweep saves the current exception, sets the existing `gc.sweeping` guard, and
clears every still-eligible generator before ordinary deallocation. The new
precise entry roots its argument with the existing one-value span, marks actual
finalization complete, sets FRAME_CLEARED and calls `gen_clear_frame`.
`frame_obj == NULL` excludes `take_ownership`; clearing then consists of frame
slot/function/local deletion, last-profiled-frame assignments, unlinking
`previous`, and exception-state deletion. The critical-section macros do not
acquire a lock in this GIL configuration. The existing sweep guards reject
object allocation or Python execution. Normal generator deallocation still
clears names/code and normal sweep frees/accounting owns the blocks.

### Required integration before final qualification

Selective-death is deliberately **not cherry-picked while unlanded**. This
base's WEAKREFS category denotes type capability, so rejecting that aggregate
would disable the intended generator cohort. The current draft uses actual
weakref/header and exact buffer checks. After selective-death lands, merge
primary, reject its authoritative actual DEL/FIRST/HOOK/BUFFER/WEAKREFS kinds
in aggregate, and remove those redundant per-object capability/header checks.
Retain the FINALIZE prefilter and dynamic finalizer/readiness revalidation.
Freeze and review that actual composition before final native qualification.

## Observable checks

Five new methods exercise behavior rather than preparation counters:

- `TracingCollectorTests.test_finalizer_observes_suspended_generator`: an owner
  finalizer resumes/resurrects its generator and preserves mutable payload data;
  eventual generator close and owner notification remain once-only.
- `TracingCollectorTests.test_weakref_callback_observes_suspended_generator`:
  another object's callback uses a plain generator weakref, resumes it and
  preserves the result; callback delivery and eventual clearing remain correct.
- `TracingCollectorTests.test_suspended_generator_payloads_do_not_accumulate`:
  four discarded batches of suspended generators retain no plain payloads
  after explicit collection. Its positive cohort has no finalizers or weakrefs
  that would deliberately disable admission.
- `GeneratorDeallocTest.test_collect_preserves_delegation_and_finally`: nested
  delegation retains inner-before-outer finally effects and usable data.
- `GeneratorDeallocTest.test_escaped_frame_preserves_mutable_payload`: escaped
  frame locals remain usable after generator death.

The inspection tests preserve this fork's existing referrer-first delivery
behavior; they do not assert that Python specifies an order for cyclic
finalizers. The child controls reuse `_queue_child` and its actual-mode
assertions with stress disabled. No raw-address fixture or new hook is
introduced. Existing
`test_finalizer_order_through_closed_generator`, finalizer-work OOM recovery,
buffer readiness, generator close/throw and monitoring `test_gen_unwind`
remain relevant. The first feedback proposal is these five methods once in
O1/Py_DEBUG actual kill/N0, followed on success by whole `test_gc` and
`test_generators`, plus affected `test_coroutines`, `test_asyncgen` and
`test_monitoring` under the coordinator's exact selection.

No new copied runtime negative is claimed at this checkpoint. Omitting the
weakref gate can execute callbacks inside sweep and is not a safe missing-event
oracle. The behavioral controls test observable admission boundaries; any
additional copied negative needs a separately reviewed, non-crashing causal
recipe rather than weakening the sweep guard.

## Verification and qualification boundary

The source packet is under ignored `build/gc-native-cleanup-source-v1/`.
Host checks are the existing C root checker on both edited C translation units,
AST parsing of the edited Python modules, compilation of the three new child
script literals, and `git diff --check`. Their exact results and source hashes
are frozen in that packet. An earlier unsaved root-check invocation reported
the new entry's argument crossing the admission call without a span; adding
the ordinary one-value span resolved it, without an effect waiver.

The new `Py_GC_PRECISE` entry triggers the existing PROCESS section 3 and
`gate.sh` contract: linked inline/outlined-callee audits and applicable default,
ASan and tail-call gate0 coverage, in addition to ordinary GCC/O1 affected
behavior and root-mode checks. No marker is removed to avoid these gates, and
ordinary GCC success alone cannot qualify the new entry on other backends.
The coordinator may compose these gates with the next coherent candidate;
this record does not request a redundant backend full suite.

Builds and targets remain held for independent source review, exact launch
review and machine admission. Root's existing selective-death qualification
owns the current native/runtime window. Timing and admission effectiveness are
later questions: measure the conservative cohort and actual elapsed/shared
cost change with the existing bounded comparison, without a speedup promise
or widening the semantics to make a benchmark enter the cohort.

## Reviewed deallocator cleanup follow-up

Source checkpoint `32d4e339c2d` preserves the original five-file change. The
separate follow-up removes only `gen_dealloc`'s redundant TRACK →
`PyObject_CallFinalizerFromDealloc` → UNTRACK ceremony after the initial
UNTRACK and weakref clearing. The exported function in `Objects/object.c`
already returns zero before its unreachable reference-counted body; that body
is deleted while the exported signature, comment and return value stay intact.
All native/frame/origin/name/code cleanup and normal block freeing remain.
No other callers or resource families are changed. This source simplification
has no claimed speedup and needs no test mirroring unreachable statements.
