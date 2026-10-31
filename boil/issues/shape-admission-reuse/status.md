# Share polymorphic shape admission

- State: review
- Owner: stats_design_review; coordinator reviews/integrates
- Updated: 2026-10-31
- Budget: 45 minutes for the decision; checkpoint at 20 minutes
- Depends on: [residual attribution](../interpreter-residual-costs/status.md)
- Branch: boil/shape-admission-reuse, candidate c46f1bef67f
- Worktree: ../boil-shape-admission-reuse
- Phase: independent review passed; selected for coherent qualification
- Decision: integrate if coherent landing qualification passes

## Hypothesis

The existing polymorphic attribute operation family can perform shape admission
once and consume the matched row locally, simplifying generated code without a
second operation protocol or effect table.

## Experiment

Inspect existing profiles, native code and DSL contracts. Prepare the smallest
shared load/store/method-family proposal that preserves pointer/type/version
admission, fallback, errors and observer/root behavior. Validate meaningful
behavior and emitted work if the source change remains simple. No new opcode
quota, type registry, cross-word row pointer or size threshold.

## Decision rule

Integrate a clean family simplification when a single admission is proven and
behavior/metadata agree. Reject if complexity, exceptions or new protocols
outweigh the work removed. Use a separate issue for any broader next idea;
small sampled shares do not justify a long noise-sensitive timing campaign.

## Now / next

The capture attributes 6.811% of Richards sampled CPU to shape-row lookup, but
only 0.774% to the duplicate payload search; the rest is required admission.
Shared feedback lookup samples span handlers and are not all removable.
Source review accepted one admission inside each existing polymorphic
load/store operation, preserving the original receiver admission.

Fresh O3 build and cleanup passed. Whole shape checks ran 156 tests with eight
existing pystats skips; ten IR/generator controls passed. Emitted matcher sites
fell from four to two, and evaluator code shrank by 213 bytes. Independent
review verified source, generated outputs and all 71 evidence references.
Next, qualify the selected combination with the GC candidate once.
No elapsed speedup is claimed.

## References

- [Design](design.md).
- [Bounded plan](plan.md).
- [Results and retained evidence](results.md).
