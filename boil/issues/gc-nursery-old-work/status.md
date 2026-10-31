# Reduce old-object work during minor collection

- State: queued
- Owner: unassigned
- Updated: 2026-10-31
- Budget: one working day for one selected owner/barrier boundary; refine before dispatch
- Depends on: [minimal nursery](../gc-nursery-minimum/status.md) and
  [coverage/reclamation result](../gc-nursery-coverage/status.md)

## Hypothesis

Replacing a measured part of the conservative old-object scan with a complete
remembered-owner boundary lowers minor cost while retaining unported native
owners safely.

## Experiment

Select one managed owner family and its stores, including constructor/error and
reentrant paths. Retain the safe scan for other owners. Use independent full-trace
comparisons and meaningful missing-barrier controls; measure scan work, pauses,
memory and mutator cost. Refine nursery/major policy only from observed traffic.

## Decision rule

Integrate a complete boundary when all its stores are covered and saved scan
work exceeds barrier/metadata cost on the declared workloads. Reject or narrow
if omissions, complexity or overhead defeat the hypothesis. Additional owner
families and policy changes become separate experiments.

## Now / next

Queued behind the first two results so the selected boundary follows real cost.
No barriers or measurements are running under this issue.
