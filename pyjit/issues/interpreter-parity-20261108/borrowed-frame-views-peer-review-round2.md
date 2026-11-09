# Peer review, round 2: borrowed frame views

2026-11-08. Source review of the revised
[proposal](borrowed-frame-views-proposal.md), following the preserved
[first review](borrowed-frame-views-peer-review.md). Candidate source is
`0267d428808`; pinned upstream is `3a440b6`. **Approve the next diagnostic
and representation-decision packet. Do not approve implementation yet.** The
revised proposal now has an explicit L/W/C/T owner model, separates counted GC
edges from views, places a no-fail promotion before copying or owning fallback,
and requires an actually retained mortal load for the pilot. These are useful
design advances, not executed proof or a speed claim. The current interpreter
batch's qualification and final performance decision remain separate.

## What the revision resolves

- The L/W/C/T table names the evaluated object and its support owner through
  load, window, child and retirement. It correctly treats proxy overwrite as
  transfer of support from the caller local to
  `f_overwritten_fast_locals`, whose insertion precedes closing the old local
  in `Objects/frameobject.c`. The proposed child edge contributes no
  `visit_decref` count; an escaped owned copy would contribute a count.
- Candidate `_PyFrame_Copy` is a move that clears source slots. Requiring
  promotion while the anchor is alive, before that move, addresses the
  central frame-retention hazard. On the GIL path `_Py_INCREF_MORTAL` is a
  nonallocating increment, so monotonic promotion with ordinary owning-slot
  unwind after it is a plausible error model. Generator and coroutine routes
  are excluded from the pilot, with later promotion required before copying.
- The proposed fallback boundary keeps the current owning transactions for
  native calls and the consuming binder. The wrapper/observer gate is placed
  before any replacement wrapper runs; a guard only at the final call would
  be too late.
- The pilot must first show a retained `LOAD_FAST_BORROW` and an attributable
  owner pair for a mortal object. The separate, conditional
  `CO_VARKEYWORDS` bridge acknowledges that the pilot does not exercise the
  allocating `logging_silent` route. Checking `MESSAGE` immortality is
  necessary before claiming that route has an owner pair to remove.

## Required before implementation

1. **Close dynamic guard failures after view creation.** The proposal says
   all route exclusions are preflighted, but also puts function/code/space
   checks in the consuming call prefix and calls a post-view decline an
   internal contract failure. In current `Python/bytecodes.c`, exact-call
   version and stack-space checks can decline on normal inputs. The selected
   producer/consumer design must either move every ordinary decline check
   before the view producer, or promote at each later guard exit and resume
   the existing owning fallback. A legal cache miss or recursion/space
   decline cannot become a fatal assertion. Show the generated control-flow
   edges, including instrumentation replacement before observer execution.

2. **Finish the consumer census for the chosen representation.** The table
   identifies the right source families, but it expressly defers the complete
   frame-slot and generated-handler scan. It is a migration checklist, not
   yet a closed proof. Choose a tag or side metadata only after the scan
   records every reachable read, write, move, clear, GC visit, frame exposure,
   copy and native/JIT handoff, plus the rejection or conversion at each
   boundary. In particular, candidate `Python/frame.c:clear_slots` and
   `Python/gc.c:_PyGC_VisitFrameStack` currently read words as owning homes;
   neither can receive a view through an unclassified path. No heap home may
   acquire the view representation.

3. **Specify promotion's exact word operation.** For side metadata, the
   untagged mortal pointer can take a count before clearing its role. For a
   private no-count tag, `PyValue_DUP` on the tagged *view word* would take no
   count under current `pycore_value.h`; promotion must reconstruct or untag
   the captured mortal object, take exactly one count, publish the canonical
   owning word, then clear the role without an observable half-state. State
   how exact numeric values that canonicalize inline are excluded or handled
   and how a pointer/anchor mismatch is detected. This is a representation
   choice to settle before code, not a reason to prefer either variant now.

The next packet should capture warmed pilot and logging IR, the actual
`MESSAGE` owner status, route-specific eligible/declined/promoted counts,
attributable argument refcount deltas, and the complete consumer scan. It
should select one representation with its producer, transfer, promotion,
traversal, clear and wrapper gates, or reject the mechanism if the pilot has
no owner pair or the logging bridge is not bounded. Only then can focused
release/debug tests and emitted-code review evaluate an implementation.
