# Peer review: borrowed frame views proposal

2026-11-08. Independent source review of
[borrowed-frame-views-proposal.md](borrowed-frame-views-proposal.md) against
candidate `0267d428808` and pinned upstream `3a440b6`. **REVISE; approve a
bounded design investigation, not implementation.** The proposal correctly
separates the eight-byte heap format from frame ownership, rejects a raw
borrow in today's owning home, and leaves representation open. It does not yet
give a complete owner graph or an experiment that discriminates this mechanism
on its motivating call route. This review makes no correctness, throughput or
parity claim. The current six-commit batch's qualification and final performance
decision remain ahead of this packet.

## Must fix before choosing an implementation

1. **Specify the owner graph and transfer state for each supported slot.** Name
   the owner of the evaluated *object* at load, in the caller window, during
   each fallible call-preparation step, after child publication, during return
   and on unwind. State when the parent local remains owning, when
   `f_overwritten_fast_locals` takes over, and how a source overwritten by
   another route is protected. The child view must name the original object;
   source-slot identity alone is insufficient. Distinguish the nonowning
   borrowed edge seen by traversal from counted owner edges used by cyclic GC.
   Show an owner reachable throughout the child's use, including
   local replacement and observer callbacks. Candidate
   `Objects/frameobject.c` already preserves an overwritten mortal local, but
   `Python/frame.c` clear and `Include/internal/pycore_interpframe.h` frame
   copy/return paths still assume every frame word owns its mortal reference.
   State which direct writes, clear/detach paths and frame-owner transitions
   can occur in the selected route; reject or promote before any unproved one.

2. **Make promotion a complete transaction.** Inventory every frame copy,
   exposure, retention and suspension site, including `Python/frame.c`,
   `Objects/frameobject.c`, `Objects/genobject.c`, `RETURN_GENERATOR`, generator
   resumption, and native/JIT adapters. Candidate `_PyFrame_Copy` moves words
   and clears source slots; it cannot move a view if the anchor stays in a
   parent frame. State exactly where all live views become canonical owners
   *before* changing linkage or releasing an anchor. If allocation or later
   preparation fails after any promotion, define rollback and exception
   preservation for the partially promoted frame. If an operation cannot
   fail, say why and identify the first subsequent fallible operation. A
   frame object may be created without detaching the running frame, so
   exposure, escape and copying need separate rules. Keep destructor order
   through parent/child retirement and line or monitoring boundaries, with a
   concrete raising-finalizer case. Merely saying that unsupported routes
   “promote or decline” leaves the most dangerous edges undecided.

3. **Prove the view cannot enter an owning slot or generic operation.** For a
   private tag, enumerate the permitted frame-word domain and all tests,
   closes, duplicates, identity comparisons, debug checks, GC visits and
   generated handlers that must classify it. Bit 0 currently means a
   no-count immortal in frame homes, and `_PyGC_VisitFrameStack` rechecks all
   home words; `_PyGC_VisitValue` skips `visit_decref` only for an explicitly
   no-count edge. For side metadata, specify how its role follows a slot
   through overlap, frame copy, compaction, tracing and unwind, and how stale
   metadata is detected. Both choices need an exhaustive consumer census and
   an explicit conversion before any heap/container/cell write. Preserve the
   canonical eight-byte value in all heap homes. Do not pick the encoding on
   source-diff size or the fact that `PyValue_Is` masks the existing bit.

4. **Resolve call fallback and native ownership before transfer.** The
   candidate's `call_transaction_overlap_python` allocates kwargs and
   revalidates after callbacks; `call_transaction_push_python_fallback` copies
   owning arguments, and `call_transaction_native` invokes value/native
   providers while the window keeps its owners. Specify a single view-to-owner
   conversion point and the failure path for every decline from a proposed
   borrowed call. Neither a native borrow scope nor a live parent local
   substitutes for an owning window if that path may detach, steal, retain or
   retire it. Give a checked generator effect/terminal contract so an
   unforeseen callback, provider change, instrumentation or PEP 523 route
   cannot reach generic cleanup with a borrowed slot. Preserve current PC,
   error indicator and ordered releases when preparation or promotion fails.

5. **Make the pilot answer the motivating question.** The proposed exact
   positional child is a useful *lifetime* proof only if a real retained
   `LOAD_FAST_BORROW` temporary flows into it. The current compiler folds
   eligible single-reader local loads (`Python/regcfg_passes.c`), and the exact
   call already uses `_PyFrame_PushOverlapping` for ordinary thread frames
   (`Python/bytecodes.c`). A tiny `f(x)` may therefore exercise no avoidable
   owner at all. Before coding, capture warmed IR and route counts for a
   chosen pilot that demonstrably retains the load, and an owner-delta or
   refcount trace attributable to that one argument. Then provide the bridge
   to the actual `logging_silent` route: its ten message snapshots go through
   `CALL_PY_GENERAL` and an allocating `CO_VARKEYWORDS` overlap with possible
   callbacks. Identify the callback, local replacement and fallback conditions
   required to carry the view through that route. If they cannot be bounded,
   the exact-call pilot is a correctness exercise, not evidence for pursuing
   this throughput mechanism.

## Decision evidence for a revised packet

Keep a bounded pilot only if a source-level owner graph covers each state and
an explicit producer/consumer census yields no unchecked owner assumption.
The diagnostic should report eligible, declined, promoted and fallback calls
by route; attributable message-object increments/decrements; actual GC counted
edges; and generated release GCC for the accepted path. Compare the new
metadata, branches, spills and code size with the owner pair it removes.
Focused release/debug checks should cover an overwritten caller local,
identity under tracing, cyclic objects, retained frames, generator/coroutine
and native rejection or promotion, partial promotion failure, exception unwind,
and destructor order. For the later logging route, use fresh balanced timing
only after its callback proof and the current batch's performance verdict.
Reject the mechanism if the pilot has no retained owner pair, if the logging
bridge needs an unbounded list of exceptions, if canonical heap or native
ownership weakens, or if the emitted accepted path adds comparable work.
