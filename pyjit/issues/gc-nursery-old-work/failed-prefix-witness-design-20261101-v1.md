# Real failed-prefix inventory cleanup witness

2026-11-01. **Design only; held for independent review.** Source base is isolated
`c43c8374dbd56ca7862be49a31820ded6ac5a9d9`, whose source capsule is
`e1f3354da23d38d6cdff3146b1c40417f61a43537f2bc2967dfcb67154ad0cad`.
The earlier helper preparation, its extraction failure and
`build/object-map-feedback-preparation/fault4-design.md` in that worktree stay
preserved and unready. This document does not resume that expired grant.

The iterator is a transitional cost-removal hypothesis, not acceptance of the
larger allocator/collector architecture. No source patch, compiler, target,
probe or timing ran for this design. The overall 04:18:11 UTC boundary remains.

## Hypothesis and missing proof

An actual early failure of the new map-page inventory can leave MARKED flags
outside its visited prefix. The cold allocator-block clear must remove those
flags before the failed collection returns. The current address-only local
oracle never marks objects. Existing trace fault 2 sets `w.failed` but continues
the walk and clears marks itself. Neither proves this new cold path works.

The positive witness must observe all three facts on one real collection:

1. The new inventory processed a real prefix and cleared at least one actual
   MARKED object through its existing classifier.
2. After forced early termination, an independent allocator block walk sees
   strictly positive remaining MARKED objects.
3. After the existing cold clear, the same independent walk sees exactly zero.

A missing prefix or remaining-mark witness is a failure, not a skip or a reason
to retry with a larger heap. The next successful collection and death recovery
are separate obligations, not substitutes for these direct observations.

## Exact fault hook

Extend the existing private `_PyGC_TraceFailAlloc` stage selector and
`_testinternalcapi.gc_fail_trace_alloc` validator from 0–3 to 0–4. Stage 4 means
**abrupt real inventory-prefix failure**, not a literal allocator NULL. Keep
0 cancellation, 1 ledger merge, 2 inventory failure-with-continued-clearing and
3 protected-heap/domain preflight semantics and call sites unchanged.

In `live_walk_page` (`gc_trace.c:2418` at c43), nominate only a normal object
death-inventory walk: `w->res != NULL`, `!w->pages`, `!w->seed_old`,
`!w->promote`, `!w->failed`, `!w->t->oom`, and no object-list/oracle mode.
The ordinary path still calls `live_map_area(..., live_walk_block, w)` as now.
Only while private stage 4 is armed does that page use a small diagnostic
visitor around the **same unchanged** `live_walk_block`.

The wrapper observes an object's MARKED bit before delegation, then checks that
delegation succeeded, incremented `res.nobjects` and cleared that bit. Map
membership and canonical block geometry have already been validated by
`live_map_area` before this wrapper reads the header. Use a stack-only context
holding `w` and a checked integer count; it allocates nothing and registers no
root. Its callbacks are synchronous, and no context escapes the page walk.

After a successful whole page with at least one observed marked-object clear,
consume `trace_fail_alloc(4)`, record that positive count and an invocation-local
trigger in `w`, set `w.failed`, and return false. If a page has no marked clear,
continue to the next page; do not consume the control. Thus “first page” means
the first eligible page that actually clears a mark, not merely a page callback
or a growth of `nobjects`. A geometry/real OOM failure before that event does
not receive stage-4 credit.

MEM page setup, old-boundary seeding, promotion, the address-comparison oracle,
the diagnostic counter and the cold clear cannot consume stage 4. No policy
setter, alternative enumeration mode, normal per-object probe or mark/age/root
change is introduced. The only normal-page addition is the existing-style
private fault-selector branch. Mark the new wrappers/helpers precise as their
actual root behavior requires; retain linked-frame/root auditing without an
opaque exception or audit waiver.

## Independent remaining-mark observation

In `live_walk_heaps`, its existing `rc < 0 && w->res != NULL` cold branch remains
the sole cleanup site. On the invocation-local stage-4 trigger only:

- Count remaining MARKED objects **before** its existing clear call.
- Execute the existing `live_visit_heaps(..., true, live_walk_block, &clear)`
  with `clear.failed = 1`, unchanged.
- Count remaining MARKED objects **after** it.

The count visitor is a different callback that never calls `live_map_area`,
classifies a death, marks, traverses references or clears flags. Route it through
`live_visit_heaps` with `blocks=true`, a fresh object-route context and the same
protected domain, attached/exited threads, heap tags and debug offset. The
allocator supplies actually in-use blocks. Skip its area callbacks. Verify
header extent and the initialized-object map bit before reading `ob_flags`;
raw/unpublished blocks are not headers to inspect. No `Py_TYPE` dereference is
needed. The allocator true-block visitor reconstructs its own free map using
fixed stack storage (`Objects/mimalloc/heap.c:599–695`), independently of the new
map-bit enumeration.

Both count walks and the clear occur under the collection's existing head lock,
without Python reentry or object allocation. Use checked `Py_ssize_t` counts.
A callback/route/overflow failure makes that count unavailable (`-1`), never
zero. Diagnostic failure must still run the real cold clear and preserve the
collection's original failure result. The cold clear's route return is not a
success witness: its `failed=1` context deliberately makes the shared helper
return failure even after enumeration. The separate count contexts must have
their own completion checks.

The outer failed-mark path continues to undo listed DEAD flags, discard the
partial inventory and return failure; no survivor promotion, death dispatch or
successful collection accounting is inferred. Existing debt/retry semantics
remain unchanged. Do not promise rollback of unrelated earlier mutations that
the existing collector does not roll back.

## Diagnostic state and OOM semantics

Keep only process-private scalar evidence beside the existing process-private
fault selector: `triggered`, `prefix_marks_cleared`, `pre_remaining`,
`post_remaining`. No object/interpreter pointers or diagnostic roots are held.
The invocation-local trigger in `w` prevents a later unrelated failed walk from
being nominated. The supported collector runs with the GIL; this does not add
a concurrency contract for unsupported collector domains.

Arming stage 4 resets the evidence to `false, 0, -1, -1` before returning.
The nominated event consumes the selector exactly once and records the prefix
count. Each completed count publishes its actual nonnegative value; incomplete
counts retain -1. Selecting/cancelling 0–3 does not erase a completed record,
so checking `gc_fail_trace_alloc(0) == 0` cannot destroy the witness. Rearming 4
starts a new record. If a genuine earlier OOM prevents the hook, it stays
untriggered, with unavailable counts; the still-armed selector must be explicitly
cancelled by the test's cleanup. There is no fabricated observation on OOM.

Add one private read-only CAPI getter returning these four named fields. It
captures all scalars before its first Python allocation. Constructing that
result may raise MemoryError normally; it cannot clear or replace the stored
evidence. A subsequent successful getter may retrieve the same snapshot. There
is no allocation between the pre/post count observations themselves. Provide
the existing-style unsupported-build stub; supported feedback expects execution,
not a new blanket skip.

## Positive and recovery case

Add one NurseryTests method,
`test_object_map_inventory_clears_failed_prefix`, with one queued child body.
This changes the local object-map roster from four methods/six children to five
methods/seven children; the full source-derived roster must be regenerated under
a new preparation grant. Existing method IDs, faults and negative bodies remain.

Use the existing mode-checked `_queue_child`/`run_code` prelude. First run OFF/N0,
seed 0, on fresh O1 debug and O3 products; any later verifier matrix is an explicit
source-derived row. Disable automatic GC before setup and fault arming.
Create a fixed 8,192-node live payload in a helper: each ordinary weakrefable
node owns a self-cycle, and a rooted list owns every node. Retain weakrefs
separately, not extra strong owners. The helper's loop locals must be gone before
collection. This is a fixed bounded multi-page fixture, not heap growth until
a desired sample appears.

First show the address-only inventory oracle does not consume stage 4: arm it,
run the existing fault-free address comparison over a nominated live object,
then cancel and require the previous selector to be 4 and the evidence
untriggered. Rearm 4, run one explicit major, snapshot the read-only evidence
and require selector consumed, triggered true, `prefix_marks_cleared > 0`,
`pre_remaining > 0`, `post_remaining == 0`. Retain exact values in the child
artifact. Any -1 or absent trigger fails by name.

Check the rooted list and every self-edge remain intact. With faults cancelled,
run one normal major and require actual successful-major progress using the
existing counter API. Drop the payload outside the checking helper's active
locals, collect once more and require all nominated weakrefs cleared. Keep weak
callbacks/finalizer timing out of this fixture; existing lifecycle cases cover
those contracts. Unexpected retention fails and is preserved; do not silently
repeat collection. Restore the prior enable state and cancel the fault in the
ordinary positive case's cleanup.

## Causal source negative and finite preparation budget

Nominate a temporary direct child of the accepted witness candidate with exactly
one runtime hunk: remove the existing cold clear call from the nominated wrapper
branch, retaining both independent count walks, fault hook and all diagnostics.
No runtime flag or permanent alternate collector is added. Candidate and
negative must have the same test/auxiliary body, O1 configuration and origins,
but fresh independently bound images. Never overwrite either sealed image.

The auxiliary measures the same prefix/pre/post facts and emits them before its
decision. Candidate must have pre>0/post0. The negative must have the same real
trigger and prefix>0, pre>0 and post>0; its exact `post_remaining == 0` oracle
must fail. This is expected-failure evidence, not a passing method/full-suite
credit. A crash, missing trigger, invalid count, import failure or a different
assertion is not the nominated result.

Do not run another GC or interpreter finalization after a bad post-count merely
to obtain a clean exit: stale mark state is the defect under test. The negative
auxiliary's reviewed runner must serialize the typed record and nominated
assertion outcome, flush raw files, then use a direct process exit. It must keep
actual PID/start/return and mode/root witnesses and label the absent normal
teardown explicitly. No synthetic outer-suite success token is permitted.
This exit protocol belongs only to the causal auxiliary, not the ordinary
positive/recovery method. Freeze its literal body and expected rc before running.

Proposed future grants, not authority from this document: at most 30 minutes
for the focused four runtime/test files plus issue evidence, 10 minutes exact
source review, and 15 minutes a new held helper capsule. The fresh O1 positive,
O1 no-clear negative and O3 positive build/feedback batch must fit one downward-
clamped 45-minute window and leave time for independent outcome review and
qualification before 04:18:11 UTC. Source/feedback stop on the first unexpected
failure. Do not add samples, rebuild a failing arm, resume the expired helper
draft, or weaken the direct positive/negative conditions without a new bounded
decision. If this cannot fit, defer it explicitly.
