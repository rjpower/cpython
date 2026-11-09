# Cold architecture review after Native32

This is a source review of runtime commit `488e85fa3a8`, not a new native or
performance qualification. The retained `c1424be11db4` PMU and cycle profiles
in [sixth-performance-review.md](sixth-performance-review.md) predate the
canonical-home duplication and Native32 changes. They locate historically hot
work; they do not assign a cost or improvement to the current commit.

**Judgment.** The normal register interpreter has a coherent owner model.
Compiler temporaries and named locals are canonical `PyValue` homes; the word
format declares input/output roles and effects in `pycore_ir.h`; the generator
produces matching dispatch and cold constructor metadata. A call window
remains caller-owned through preparation; overlapping Python calls transfer its
owners into a complete child before any callback-capable retirement
(`call_transaction_overlap_python`, `call_bind_known_python` in `Python/ceval.c`).
`_RETURN_VALUE` makes the parent current before clearing the child, and the
return slot is either published, replaced, or discarded through
`_PyFrame_SetReturnValue`. The distinct C-held result and retired-owner phases
in `ceval_macros.h` are necessary when an observer can redirect execution.
These are legible boundaries, not separate competing frame models. The current
code is substantial because callbacks, destructors, folded locals and exposed
frames can reenter it; hiding those phases behind a generic ownership API would
make the proof harder to inspect.

The optional debugger/source transfer model is a different scope. Native32's
observer outcome is a declared edge fact, with physical publication separate
from old-owner provenance (`_PyIR_ObservedHomes` in
`pycore_ir_outcomes.h`). `Objects/frameobject.c` checks a bounded stopped-state
recipe; it does not grant arbitrary jumps through active context managers or
prove every SOURCE_POST/yield transfer. Such crossings are debugger features,
not a requirement for ordinary Python call, return or storage semantics.

1. **Profile current call shapes before shortening the guarded call path.** The
   generated `CALL_PY_GENERAL` handler checks PEP 523, function version and
   recursion, then tries `call_bind_known_python`; a miss runs the complete
   transaction. The known binder already specializes positional/default and
   virtual empty-kwargs layouts, so it should not be described as missing.
   Retained `c142` logging samples located 11.54% of cycles in
   `CALL_PY_EXACT_ARGS` and 10.64% in `CALL_PY_GENERAL`, including 4.43% in the
   known binder; these figures cannot be carried forward as current budgets.
   Count current hit/miss reasons and argument layouts under a fixed identity,
   then compare generated assembly for the dominant shape before removing a
   guard, allocation or dispatch. The likely tradeoff is more specialized code
   and instruction-cache pressure for fewer operations on a proven common
   call shape. Keep the generic transaction for reentrant providers and misses.

2. **Keep the overlapping-child commit invariant in one reviewable place.**
   `call_transaction_overlap_python` and `call_bind_known_python` each detach
   function home 0, push an overlapping child, install defaults/variadic
   bindings, set `return_slot`/`return_pc`, and make the child current. They have
   different preparation guards, so a shared binder would be too broad; the
   repeated final commit is the narrower cleanup target. A small inlined commit
   helper or an equivalent source-level factoring is worthwhile only if it
   preserves the no-callback interval and does not add work to the hot known
   path. Review generated assembly and test a reentrant kwargs allocation,
   default-owner transfer, child-init failure, local/discard return and a
   destructor during cleanup. This is chiefly a correctness-maintenance gain;
   no throughput gain follows from the source alone.

I would not start a new float or publication rewrite from the old profile.
`PyValue_FloatOperandValues` already has a direct two-inline-float first branch,
and the four float constructors plus both local-load producers already use
checked canonical final stores. A current profile can reopen those paths if it
shows an actual remaining conversion or dispatch cost. The reviewed cold
NULL-only debugger transfer facts may improve optional jump coverage, but they
do not address normal interpreter throughput and should remain off its hot path.

## Priorities after the broad screen

The clean two-pair golden screen at `488e85fa3a8` puts generators at `1.07753`,
coroutines at `1.07886`, and nbody at `1.05754` candidate/golden; its original
unpack, float, deepcopy-memo and pickle losses have closed. These are elapsed
results against golden, not current PMU evidence and not an explanation of any
source change. If the coordinator's four-pair repeat confirms them, investigate
generator/coroutine suspension and frame costs first, nbody second, and the
call-shape cleanup above only after a current logging profile justifies it.
The [current PMU plan](current-pmu-plan-after32.md) keeps counters per workload
separate from sample locations and retains the sealed FIFO timing boundary.
