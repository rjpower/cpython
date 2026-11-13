# Interpreter performance: combined plan

Status: combined design review in progress, requested 2026-11-13. Astra runs at
high effort in Loom session `koiua7b4`; implementation follows review and
coordinator integration. This is the active successor to the model55 migration
plan. The [goal](../../GOAL.md), [architecture](../../design/runtime-architecture.md)
and [status](../../STATUS.md) are the entry path. The
[handoff](../../HANDOFF.md) records source/build identities and operational state.

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

Astra will produce one combined design and critical source review, with Mermaid
execution/ownership diagrams, implementation dependencies and falsifiable cost
hypotheses. Root will integrate it into this plan and the architecture chain.
The dependency order is provisional until that review:

1. Freeze the source, native controls and current evidence; finish documentation
   cleanup and classify the outstanding model55 components.
2. Define the shared value/ownership/effect contracts and land bounded ordinary
   value-transport improvements that do not require the entire frame migration.
3. Extend borrowing and unify specialization around those contracts, with
   independent experiments where file ownership and interfaces permit.
4. Apply the common call/return/resume convention and the required frame and
   exception-transfer pieces. Remove superseded adapters and bookkeeping as
   their replacement becomes authoritative.
5. Qualify the combined implementation, measure all 29 and the hot set directly
   against golden54/upstream, inspect matched profiles, select winners and
   publish the follow-up plan.

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
