# Comprehension hot-path workstream

This workstream follows the [combined interpreter plan](../interpreter-performance56/plan.md).
The [candidate design](../../design/proposals/comprehension-hot-paths57.md)
records the four experimental implementation contracts. This workstream is
complete: selected57 adopts the dictionary-only subset. Golden56 remains
immutable at `883524c180e4486b47df77ee2276c9276c0ec32c`.

## Completed work and disposition

Four lanes were implemented and received focused native qualification on
candidate `9ec291998f7395fedf21f456e5eb58c2187d2e23`: checked native method
entry, shared dictionary lookup layers, checked private list iteration and
inline spare-capacity append. This composition is not selected.

The direct 29-workload comparison measured 1.001784 versus golden56, with 15
wins, and 1.144639 versus upstream, with 7 wins. In six-pair confirmation,
comprehensions measured .948526, DeltaBlue .966836, silent logging 1.131569 and
coroutines 1.046046. The observed comprehension gain does not offset the logging
and coroutine regressions; do not promote this bundle or claim all four changes
as landed in the selected runtime.

Two layout experiments were rejected. Uniform 64-byte alignment did not support
selection and was reverted. The local append trial at `0748` was also rejected:
its screen measured comprehensions .927003, silent logging 1.211611, coroutines
1.032974 and Richards 1.036581. Keep both trials and their evidence for
reference. Do not run an arbitrary layout or padding sweep.

The earlier two-lane and four-lane screens, counters and native reviews remain
in E57 (`BASE/pyjit-link-wave1/comprehensions57`). The detailed
[workstream report](report.md) and [benchmark tables](benchmark-tables.md)
record the final closure and campaign evidence. The [handoff](../../HANDOFF.md)
defines BASE, control paths and execution rules. Older round-56 and model55
results remain in their respective reports.

## Closed: dictionary provider and lifetime repair

Select `0dcba75bb3df40c32137db453b001f7e8f101de0` on
`pyjit/round57-golden`, tag `pyjit-golden-20261114-dictionary-readers`.
The comparison range from golden56 changes only dictionary source and tests.
Typed `dict.get` and public readers share lookup/result acquisition below API
validation. Callback-capable lookup retains one dictionary/query scope;
cached exact strings also require a Unicode-only table for callback-free
admission under the GIL. Both GetValue and ContainsValue preserve that rule.

The earlier minimal lifetime-only repair, `72ee`, is not selected: a flat broad
mean masks repeated DeltaBlue and comprehension regressions. One final
source-backed subset removes nested lookup/query-pin layers as well. It is
measured independently from method/list changes, rather than seeking a layout
seed. No further candidate or measurement repeat follows this closure.

GC implemented, frames reviewed the proposal and actual composition, and root
built, qualified and measured. An independent cold reviewer supports selection
with explicit tradeoffs. Both builds complete 117 modules/79 extensions; 203
methods across three dictionary modules pass per role. The 28 narrower reader
methods overlap that coverage. These are focused checks over the inherited
selected56 checkpoint, not a new full-system run of this subset.

All29 is .995586/golden56 and an observed 1.127137/upstream; upstream XML pairs
are unstable, so do not treat that aggregate as a precise gap estimate. Six-pair
confirmation keeps comprehension/DeltaBlue/logging near flat, with repeated
Richards/spectral_norm/nqueens gains. Root explicitly accepts roughly 2%
coroutine/raytrace losses and generator mean debt with a wide range. The report
owns exact ratios, ranges and rationale. PMU still finds 36.14%, 43.64% and
30.78% extra native instructions in DeltaBlue, comprehensions and coroutines
versus upstream. This checkpoint does not solve interpreter parity.

## Follow-up proposals

A checked list-pointer view could combine projection, checking, exhaustion and
element acquisition. A shared iterator-pair protocol could support removing the
remaining cursor-tag test. Both need separate ownership, invalidation and
constructed-image proofs; they are proposals only and are not prerequisites for
the dictionary closure.

Canonical documentation is this checkout; runtime sources here are stale and
must not be built. Root serializes native and timing work. Keep the golden56
control immutable and retain rejected branches, builds, logs and profiles.
