# Source finding: cleared generator roots in finalizer ordering

Bounded source analysis, 2026-10-29; no patch, target, profiler, compiler or
new counter instrumentation. Read ce2ac collector source through the owned
writer tree08953c89274 (native writerf5e818b0454). Its collector/generator
files are unchanged from ce2ac. This finding is separate from the active
ROOTS+bytes integration and is not implementation authorization.

The four existing profiling rows establish material collection cost, not
specifically repeated old-live traversal. Coroutine hot counts include56.43M
candidate visits,48.56M collected GC objects and10.88GB freed, with a last-mark
live gauge3.05MB. Candidate visits repeat across collections and include live,
dead and immortal heap objects; collected counts only swept GC-flagged objects
(gc_live:sweep_one1172). Freed bytes include freed blocks more generally.
None is an age histogram or distinct-allocation count. The last live gauge is
not average/maximum live size. Root time is within collection duration; queue
Python calls after collect() are outside that duration. No gain is predicted.

Exact existing no-op predicate in pycore_genobject.h27 is:

```c
Py_TYPE(op)->tp_finalize == PyGen_Type.tp_finalize &&
    FRAME_STATE_FINISHED(((PyGenObject *)op)->gi_frame_state)
```

FRAME_STATE_FINISHED is exactly state==FRAME_CLEARED here (pycore_frame.h57),
not a new guessed numeric interval or a test for every coroutine/generator.
Gen/coro/asyncgen share gen_finalize; its first branch returns for this state
(genobject.c98). gc_live.c865 already applies the helper before queueing:
cleared instances incur no CALL_FINALIZE allocation/resurrection merely for
their no-op finalizer. This existing optimization must not be claimed as new.

The remaining source-derived candidate is earlier ordering-root eligibility:

1. gc_trace.c2184 marks every dead tp_finalize-bearing instance FINALIZE,
   without using the no-op predicate.
2. gc_live.c747 finalizer_order starts a dead-graph DFS at those instances,
   creates stack/visited/order storage, traverses their dead references, clears
   temporary marks and reverses the postorder.
3. Only afterward handle_finalizers813/865 filters finalized/no-op objects out
   of actual calls. Queued calls resurrect their object/dead reachable closure
   at606; cleared no-op generators themselves are already excluded from that.

A possible separate design would narrow the DFS *roots* to objects whose
finalizer can actually run. It must preserve no-op objects as transit nodes
when reached from an active finalizer and preserve active-finalizer precedence
through those nodes. Do not skip gen_traverse globally, remove its code/name/
exception/origin visits, or equate no-op finalization with absence of roots or
referents. Removing independent no-op DFS roots may change unrelated traversal
order; prove the actual referrer-before-referent constraints and cycle rules,
not identical incidental heap-order output.

Weakref callback clearing precedes handle_finalizers. Legacy tp_del cycles,
DEBUG_SAVEALL, destroy hooks, ownership/queue routing, resurrection, OOM
keep-all paths and finalizer-once flags retain their existing contracts.
The later deallocation order (DEALLOC_READS_REFERENTS, then ordinary instances,
then types) and final block-free pass are independent, required invariants;
this candidate does not remove them. Per-dead classification, actual resource
cleanup, type-before-free protection and allocation reclamation remain work
regardless of generations. The repeated live-root/heap passes likewise remain;
the profile's collect/gen_traverse samples alone do not isolate the proposed
DFS-root work. A small proof/design plus existing semantic/negative controls
must precede any patch, not another broad profiling framework.

Inspected file SHA256s in the writer tree:

- Python/gc_live.c:0833204bbf7c316aac1f7a3de48a1ae8548a85a041144b35379a9aa1cd11a83f
- Python/gc_trace.c:a992e22b36eb4c31031cddf1d92101c2cb91e6363de376cd678528648cccaa01
- Objects/genobject.c:9a948aacce82e11f29745b11a1f88d8bf128bc04604fc0dd1927aaac4be639c6
- Include/internal/pycore_genobject.h:4c56a0e45237ba66c9c01cb39944677ead87ffa8c55b3708fa584be8912c6d36
- Include/internal/pycore_frame.h:d11fa9f98b80cd04d657777d5a72eddedfb9905a9da9da7f23f714bb0fdb2fb9
