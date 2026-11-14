# Interpreter performance: combined plan

Status: combined architecture reviewed by Astra high in 6036b9a5ce7; source
implementation, peer review and merged qualification active. This is the active successor
to model55. Read the [design](../../design/proposals/interpreter-performance56.md)
and [critical audit](review.md), then the experiments below. The [goal](../../GOAL.md),
[architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) own direction, selected contracts and exact state.

## Outcome

Improve the interpreter across DeltaBlue, generators, coroutines, Richards,
comprehensions and silent logging, then qualify the whole benchmark set.
Preserve Python behavior, explicit ownership, useful observation and the gains
already delivered by canonical PyValues. Select qualified improvements against
immutable golden54 and upstream controls. Do not attribute the generator or
DeltaBlue gap to frame clearing without evidence.

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
| Foundation | Qualify the VALUE replay correction and direct completion composition | Root; I8d0e core52 passes per role plus 5+27; earlier completion84 passes per role |
| Early call | [Exact-tuple preparation](call-ex-tuple.md) and cold call-observer geometry integrated | Root; 12 transaction methods pass per role, native arm bypasses acquisitions; hot six has zero sites, so no broad gain claim |
| Shared facts/transport | [Canonical list/tuple transport](value-transport.md) replaces the old list body proof | IR Sol/root; e61a022 integrated; actual products reviewed,154-method integration passes per role at 82e857; native canonicalization removal confirmed; inline accessor rejected after six-pair coroutine regression; final full qualification active |
| Borrowing | Close initial callback-free COPY-web experiment at zero candidates; reassess broader materialization/CFG opportunities against measured work | Frames/IR; 21 corpus sources, 426 CFGs; no speculative pass is implemented |
| Completion | Qualify [direct native terminal entries](completion.md); fix shared generator-entry recipient admission | Frames Sol/root; exact-coroutine transfer/owning-output proof integrated and154-method gate passes per role; native scratch arm removes helper/refcount pair, costs+238B text/+16B stack |
| Specialization | Execute complete homogeneous [polymorphic SHAPE rows](polymorphic-slot.md) under one checked read contract | GC Sol/root; actual hits confirmed; current composition154-method passes per role, Richards improves; broad comparison/confirmation complete at82e857; current qualification and selection pending |
| Cold storage | Independently qualify IRSC2; diagnose bitmap monitor/profile and synthetic debug failure | Root with source peers; full M4 remains a separate lifecycle gate |
| Selection | Compose qualified winners; direct hot six and all 29 versus frozen parent, golden54 and upstream | Root; ac4c integrates qualification repairs; both 117/79, focused gate active before new full runs and exact-head timing; cold source/doc reviews GO |

The first call, transport and specialization increments are composed. Shared facts are a
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

Golden54 remains `61015a9452d9069646fb5fede9ff963896833b89`. Latest integration
82e857384f8 gives Richards .720826/golden, generators .934380 and DeltaBlue
.960183 in two pairs; coroutine/logging range across parity. Earlier f291
coroutine/logging regressions are retained in the report. The earlier broad/PMU
checkpoint6a00c6a08d4 has all 29/upstream mean1.161978. No new variant is selected.
The [checkpoint report](report.md) owns exact-head ratios, ranges and scope.

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
retirement. All ten are now retired; 12 current mirrors retain the goal → plan →
architecture/status chain. Canonical design/review evidence remains in git.


## Followup priorities

Complete current qualification and selection before another source experiment.
The independent cold review recommends: centralize compiler fact invalidation
and meet operations; remove redundant native-call refresh only if compiled
work survives; migrate the next measured numeric producer into shared facts;
count terminal completion before further ABI work; and profile final call/
transport paths before widening PIC selection. The terminal helper is only
1.66% of current generator self samples, while SEND_GEN and delegated yield
remain frequent. Prioritize the common resume/recipient/retirement path over
another terminal-only adapter. These are proposals, with measured removal and
regression gates; none implies a universal runtime call object or new frame ABI.


## Current qualification closure

| Lane | Owner | Gate |
| --- | --- | --- |
| Logical absence | IR/GC; source0dcc→Idab1 | Unavailable reads reject across public constructors; full-extent/dirty retirement and existing error authority remain correct |
| Fixture alignment | Frames/GC; ac4c integrated | Current magic/table pins, exact call roles, real unknown producers and frame layout; semantic witnesses remain intact |
| Generator source checks | IR with independent peer | Resolve seven failing generated-case methods, including optional observer metadata for minimal analyses; no expected-output waiver |
| Final integration | Root | Focused checks, coherent full release/debug, then exact-head benchmark/control/PMU and selection |

The first release full gate is red and incomplete debug is interrupted, not
qualified. Six preexisting debugger-jump errors remain explicit; only the
existing GDB pretty-print class is quarantined. Golden54 stays selected.
