# Next decision: holistic call/frame architecture

Round54 is closed. All five performance options, two revisions and two
compositions are rejected. Only the separate public-constructor correctness
fix is selected at checkpoint54, with explicit qualification limits and no
performance lift. Retain the [report](report.md), [tables](benchmark-tables.md),
[qualification](qualification.md) and immutable controls.

The successor is [round55](../call-model55/plan.md): a completed, independently
reviewed architecture and migration plan, **before implementation**. The
[proposed call model](../../design/proposals/register-call-model55.md) assesses
calls, frames, storage, occupancy/bitmaps, handoff and observation as one
execution model. It remains separate from the as-built designs. The source
[cost audit](call-model55-cost-audit.md) supplies current facts, not a new ABI.

The reviewed plan addresses these connected questions:

1. What do initialized bits, RC ownership, logical liveness and GC roots mean
   at ordinary execution, callbacks, unwind, suspension and observation? Compare
   dynamic occupancy bits, static maps, tags and NULL homes on every producer
   and consumer; do not count only the teardown scan.
2. Which facts can constructors or producers establish once for binding,
   overlapping arguments, destinations and continuation state? Define the
   full transfer/retirement protocol, including first input, same-PC redirects,
   discarded results, allocation failure and escaped frames.
3. Which state must remain local to the executing frame/image, which owners
   are independently pinned, and which cold state can be deferred? Account for
   dependent loads and mapping-bearing construction, not just header bytes.
4. Which repeated operations can a coherent interpreter or compiled region
   remove? Evaluate concrete work and exits; native JIT is one architectural
   option, not an automatic explanation or prescribed answer.
5. What weighted lifecycle census and unchanged-workload evidence would
   distinguish the alternatives? Keep generator resume traffic separate from
   recursive coroutine creation/completion and ordinary calls.

Round55 research and independent post-review are complete; no runtime source
work has started. The [resolution](../call-model55/review-resolution.md) closes
all five initial must-fix findings. Do not reopen measured round54 variants,
change flags or rely on placement to rescue them. A physical owner bitmap is
a complete comparison candidate, not rejected by round54's small experiments.

For a later implementation decision, preserve Python behavior, explicit owners
and observer/error contracts; require measured removed work and account for
new bookkeeping. Keep hot6 plus all29, direct composition evidence and all
qualification limits. Flat profiles are not an exclusive removable cycle budget.
