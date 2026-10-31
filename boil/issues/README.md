# Work board

Each issue's `status.md` owns its state. Experiments have a hypothesis, concrete
budget and integrate/reject decision; see the [template](_template/status.md).

## Running in parallel

| Track | Active experiment | Owner | Next decision |
| --- | --- | --- | --- |
| GC | [Minimal nonmoving nursery](gc-nursery-minimum/status.md) | string_design | Bounded hypothesis passed; select for coherent landing qualification |
| Interpreter | [Share polymorphic shape admission](shape-admission-reuse/status.md) | stats_design_review | Independent review passed; select for coherent landing qualification |

The nursery's first decision finished within its one-hour budget. Source work,
reviews and builds can overlap with separate ownership; quiet measurements,
shared full-suite/perf-map windows and integration serialize. Qualify the
selected GC/interpreter combination once rather than repeat unchanged ladders.

## Next GC experiments, in order

1. [Broader coverage and cheaper reclamation](gc-nursery-coverage/status.md).
2. [Reduce old-object scan work](gc-nursery-old-work/status.md).

Each follows the preceding result; the initial nursery need not solve the full
future allocator, copying, lifecycle and barrier design at once.

## Unassigned follow-ups

- [Value/native interfaces](value-native-interfaces/status.md).
- [Verification orchestration](verification-orchestration/status.md).
- [Root/checker audit](root-checker-audit/status.md).

These are intake scopes. Split a bounded hypothesis before dispatching work.

## Deferred

- [Tail-dispatch frame passing](h1-tail-dispatch/status.md).
- [Physical liveness/root maps](h2-root-maps/status.md).
- [Foreign interpreter lifecycle](gc-foreign-lifecycle/status.md).
- [Handler tooling consumers](handler-tooling-consumers/status.md).

JIT tiers and broader package ports follow interpreter work and settled native
contracts. Roughly 2× interpreter speed is guidance, not a numerical gate.

## Rejected

- [Bulk frame transfer](frame-transfer-bulk/status.md): no repeatable gain in two matched process rounds; no further qualification.

## Reported

- [Documentation consolidation](documentation-migration/status.md): complete; current contracts, concise workflow and bounded experiment template adopted.

- [Residual attribution](interpreter-residual-costs/status.md): independently replayed finding selected the frame-transfer experiment; no new benchmark was needed.

## Landed

- [GC cleanup and scanning](gc-cleanup-scanning/status.md): qualified, integrated
  and measured. This is the comparison baseline for the new experiments.
