# Root review of the one-assignment upstream control

2026-11-01 09:35 UTC. **ACCEPT the bounded design and host preparation.**
This is independent of the author's failure audit and
[proposal](stackref-upstream-compatibility-control-design-20261101-v1.md).
It grants no compiler, target or native activation.

The pristine build's raw failure is distinct from RC's shutdown failures. Source
inspection supports one specific hypothesis: `_PyFrame_Copy` transfers the
executable through MakeHeapSafe, which closes the source debug token; current
upstream take_ownership duplicates the new token into the destination again.
The subsequent source clear can then close an invalid token. RC already restores
the source token instead. The original fatal output lacks the creation trace
needed to attribute ID55984 to this exact path.

Permit one new project-managed source successor from321925 with exactly that
assignment's left-hand side changed, followed by a separate fresh build. Preserve
the pristine source, failed build and all raw outputs. Require clean actual
commit/tree identity, an exact one-file/one-line source proof and unchanged three
standard method bodies before native authority. No external freezer, existing
objects, other RC frame code or checker suppression is permitted.

Use the fixed same-toolchain/profile two-row control: build1200 seconds,
three-method standard comparison180 seconds, plus210 seconds reserve per row,
within one future1800-second inclusive grant. Start it only after source/helper
review and actual idle checks. Hold the canonical serialization lock through
terminal cleanup, with both slots and the applicable build lock. Include the
bootstrap/freezer executables in exact-executable/pidfd cleanup, not just the
prospective final Python and embedding image.

All three methods must produce complete ordinary results without a missing or
skipped selection. Expected standard failures are comparative data, not a pass.
Compare actual phase and diagnostic signatures, not just nonzero exit codes.
One successful corrected build would support the bootstrap hypothesis but would
not excuse RC's entire failed checker run. Any RC gate amendment remains a
separate evidence-based review. If this control still fails mechanically, stop;
there is no further upstream fix chase or automatic retry.

The coordinator authorized coroutine_design_review to prepare this source and
held helpers at09:35. Independent source/helper review and a fresh activation
remain required. RC source and packaging qualification are separate.
