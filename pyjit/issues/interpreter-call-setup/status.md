# Attribute shared call setup costs

- State: closed
- Outcome: reported
- Decision: reject a new generic call/setup interface implementation from present evidence
- Owner: stats_design_review; coordinator selects any implementation
- Updated: 2026-10-31
- Budget: concluded within 30 minutes; no native runs
- Depends on: [residual attribution](../interpreter-residual-costs/status.md)
- Phase: attribution complete; coordinator replay verified

## Hypothesis

Generator/coroutine residual execution includes repeated generic call or
argument preparation that one shared interface simplification can remove.
The size and removability of that cost are unproven.

## Experiment

Inspect existing frozen profiles and current source consumers. Separate required
work from repeated lookup, copying or boxing; attribute any finding to whole-run
cost rather than the entire non-GC residual. Return at most one minimal shared
change, observable checks and a finite implementation experiment. Use existing
indexed evidence; no new profiles, builds, timings or runtime edits.

## Decision rule

Integrate a supported finding into a separately bounded implementation issue.
Reject this hypothesis for now if attribution is too small, ambiguous or would
require a new operation protocol. Stop at 30 minutes rather than extend the
survey. A proposal is not a runtime landing or measured speedup.

## Now / next

The exact Python call path already uses unboxed value windows. Generic setup
was absent from the coroutine samples and small in the generator capture;
observed object/value conversions were canonicalization, not demonstrated
argument boxing. Outer evaluator ancestry includes useful execution and cannot
be assigned to setup cost.

The coordinator independently replayed all four captures, verified six raw
inputs and checked ten source files against the captured revision. No runtime
implementation or new profile is selected. Shared nursery/shape qualification
continues; revisit calls only with a named consumer and removable work.

## References

- [Boundary review](design.md).
- [Attribution and reproducible evidence](results.md).
