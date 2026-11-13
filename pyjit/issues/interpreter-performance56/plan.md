# Interpreter performance: combined plan

Status: combined architecture reviewed by Astra high in 6036b9a5ce7; source
peer review and foundation qualification active. This is the active successor
to model55. Read the [design](../../design/proposals/interpreter-performance56.md)
and [critical audit](review.md), then the experiments below. The [goal](../../GOAL.md),
[architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) own direction, selected contracts and exact state.

## Outcome

Improve the interpreter across DeltaBlue, generators, coroutines, Richards,
comprehensions and silent logging, then qualify the whole benchmark set.
Preserve Python behavior, explicit ownership, useful observation and the gains
already delivered by canonical PyValues. Select qualified improvements against
immutable golden54 and upstream controls. Do not attribute a roughly43%
generator gap, or the smaller DeltaBlue gap, to frame clearing without evidence.

## One architecture, four workstreams

| Workstream | Required design and implementation |
| --- | --- |
| Values | Distinguish canonical homes, native private results and typed borrows. Carry established canonical/object facts through producers and native interfaces; eliminate redundant conversion and ownership work. |
| Borrowing | Extend the existing borrow-selection and copy-propagation passes with an explicit use/def, effect and lifetime model. Establish where a borrowed view is legal, when it becomes an owner, and what callbacks, local mutation, escape and suspension invalidate. |
| Calls and returns | Preserve exact-call overlap and inline return. Define a common argument, binding, recipient, continuation and retirement contract for ordinary calls, generators and coroutines, with exceptional/observer work paid at reached boundaries. |
| Specialization | Use one checked description of guards, effects, ownership, result facts, cache validity and fallback across lowering and interpreter specializations. Reuse existing adaptive caches; avoid parallel handwritten proof systems. |

PyValue is already canonical throughout runtime homes, containers and rebuilt
extensions. Native numeric results can still be private boxed temporaries until
publication. That boundary is the cleanup target; this is not another value
format rollout. A borrow pass also already exists: the frontend selects safe
borrow candidates and register copy propagation folds many local reads into
consumer operands. Strong snapshots at callback and ownership boundaries remain
necessary unless a stronger lifetime proof replaces them.

## Design gate and sequence

Astra's review preserves the current frame ABI and requires one checked
value/owner/effect contract in existing generator and register passes. It rejects
persistent borrowed homes, another value rollout, a universal runtime call
object and waiting for all static storage proofs before any ordinary-path gain.
Sol peers each concrete proposal before implementation. Root freezes experiment
inputs and records source, generated, native and dynamic evidence separately.

| Lane | Next concrete work | Owner and exit |
| --- | --- | --- |
| Foundation | I VALUE/error-copy plus isolated empty-family repair; qualify full generation and native focused behavior | Root;33 narrow/full cases/metadata/contracts pass at 1413a9; products/build/native pending |
| Early call | Exact-object-tuple CALL_EX probe before owning snapshots, preserving final refresh/nonexact path | Frames Sol; propose/peer, expected mortal-pair delta and actual hot-set frequency |
| Shared facts/transport | Extend existing helper/action facts for one frequent canonical producer; replace its old special proof | IR Sol; immediate consumer, negative proof tests and emitted/native removal |
| Borrowing | Multi-use callback-free folding, then acyclic CFG joins with explicit owning snapshots | IR/frames peers; net surviving-word and DUP/CLOSE reduction after materialization |
| Completion | One remaining binding/recipient or completion duplication under the common ownership contract | Frames Sol; keep overlap, code pins, CALL_WAIT partition and inline return |
| Specialization | One hot arithmetic or attribute/call family consuming shared guard/effect/result facts | GC Sol; cache hit/miss/invalidation proof and fewer hot checks/bridges |
| Cold storage | Independently qualify IRSC2; diagnose bitmap monitor/profile and synthetic debug failure | Root with source peers; full M4 remains a separate lifecycle gate |
| Selection | Compose qualified winners; direct hot6 and all29 versus frozen parent, golden54 and upstream | Root; coherent full diagnostics and cold post-review |

The first call and transport proposals proceed in parallel. Shared facts are a
bounded foundation for broader borrowing/specialization, not a framework project.
Cross-line observer reconstruction and arbitrary allocation require their own
qualified readers/transfers. They do not block safe local optimizations on
initialized-NULL homes. After two revisions with no native removal or measurable
benefit, close that experiment and choose the next high-frequency cost; do not
silently abandon the entire workstream.

Every increment follows propose → peer review → implement → focused test →
post-review → integrate. Sol workers own disjoint implementation and review
lanes. Root owns generation, native builds, merged validation and serialized
timing. Full suites belong at coherent representation gates. A source-reviewed
component is not a qualified runtime or a speedup.

## Carry-forward evidence

Golden54 remains `61015a9452d9069646fb5fede9ff963896833b89`. The latest measured
model55 static checkpoint is 8232dd8adde, with mixed two-pair results against54
and substantial remaining gaps versus upstream. The
[checkpoint report](report.md) owns the exact ratios, ranges and scope.

The model55 bitmap execution-reader candidate builds both roles, but monitoring
and profiling qualification remain open. Its pending IRSC2 retained-transfer
source is reviewed but not generated or native-qualified. Preserve these
components and their evidence while the combined design decides their use.
Full static scratch authority and arbitrary register allocation are unfinished;
they must not become an unexamined prerequisite for every ordinary-path gain.

## Measurement and selection

Use unchanged workloads, matched GCC configurations, fixed affinity/hash and
JIT disabled. Record source/native identity, validity checks and pair ranges.
Measure the actual count and cost of remaining conversions, count operations,
dispatches, call adapters and storage maintenance. Historical flat samples or
instruction ratios are clues, not additive removable cycle budgets.

Each integration reports what work disappeared and its hot-set result. Use
larger confirmation only when the initial evidence warrants it; run all 29 and
matched upstream profiles for final selection. Keep golden54 if no qualified
composition improves the result. Startup RSS and zero-box counts are not gates.

## Documentation cleanup

The goal, active plan, architecture and status now identify this stage.
The checkpoint report owns current comparison evidence; model55's report and
reviewed source remain historical inputs. Four duplicate progress/staging pages
are retired with their exact committed contents retained in git and the stage56
evidence directory. Ten superseded Loom review/inventory/progress mirrors have
been exported with revision metadata and empty thread inventories before
retirement. Their canonical design/review evidence remains in the repository.
