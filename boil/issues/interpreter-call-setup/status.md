# Attribute shared call setup costs

- State: active
- Owner: stats_design_review; coordinator selects any implementation
- Updated: 2026-10-31
- Budget: 30 minutes from assignment; no native runs
- Depends on: [residual attribution](../interpreter-residual-costs/status.md)
- Phase: bounded source/profile investigation

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

Investigation runs alongside shared GC/shape qualification with separate
ownership. Report the evidence and one recommendation, or a concrete rejection.
