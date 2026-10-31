# Work board

Each issue has one authoritative `status.md`. Designs, implementation plans,
reviews and results remain in their linked documents; this board is navigation,
not a second status ledger. Stable issue IDs do not change when work closes.

**No runtime work or benchmarks are active.** The next two tracks can proceed
in parallel after the coordinator assigns owners and scopes. Queued means
not started or not yet assigned.

## Next

- [Common young allocation and reclamation](gc-common-young-space/status.md):
  one useful young-space consumer, including lifecycle and bulk reclamation.
- [Interpreter shared costs](interpreter-shared-costs/status.md): bounded
  attribution followed by shared, orthogonal implementation proposals.

## Closed

- [GC cleanup and scanning](gc-cleanup-scanning/status.md): landed, qualified
  and measured. Component failures and composite evidence remain explicit.

## Unassigned follow-ups

- [Value/native interfaces](value-native-interfaces/status.md): remaining
  slot/Clinic boundaries and identity/accessor contract reconciliation.
- [Verification orchestration](verification-orchestration/status.md): shared
  profile execution and gate integration beyond landed components.
- [Root/checker audit](root-checker-audit/status.md): reconcile historical
  handoffs and finite diagnostic/precision coverage.

## Deferred

- [H1 tail dispatch](h1-tail-dispatch/status.md).
- [H2 root maps](h2-root-maps/status.md).
- [Foreign interpreter lifecycle](gc-foreign-lifecycle/status.md).
- [Handler tooling consumers](handler-tooling-consumers/status.md): H3
  transport/schema/export follow-up, distinct from H1/H2.

Deferred means retained with a stated reactivation condition, not abandoned.
State values are `queued`, `active`, `blocked`, `review`, `deferred` and
`closed`. Closed records also name an outcome: `landed`, `reported`,
`abandoned` or `superseded`. Only `status.md` is required; use the
[template](_template/status.md) and add other documents only when useful.

The [project overview](../README.md) introduces the runtime;
[PROCESS](../PROCESS.md) governs assignments, reviews and gates. Status
transitions do not waive existing gates or add an approval step.

Later [JIT tiers](../design/06-architecture-v2.md) and
[broader package ports](../design/10-internal-benchmarks.md) remain future
milestones, not active assignments. Ports must reconcile native ABI identity
and the settled value/root interfaces; historical package results are not
qualification of the current runtime.
