# Proposed retention oracle amendment

State: held for design review, 2026-11-01. This corrects the settling-witness
assumption documented in the [diagnosis](retention-observer-diagnosis-20261101-v1.md).
It grants no helper implementation, target or campaign execution.

Keep the exact kernel bytes, global roots, wrapper hot-clock boundaries,
workloads, two loops for Richards, three unscored retention calls and three
explicit majors per scored process. Keep all 132 fresh processes, 120 scored
samples and 360 retention calls. Other kernels keep the existing object
oracle. Every arm keeps the 16 KiB total live-byte growth limit and 32-object
limit on unexplained tracked growth. Do not use the partial timing results
to change ordering, sample count or thresholds.

For Richards only, verify its explicit owned graph independently at each
existing retention point, after reading live bytes and the tracked-object
count. Preallocate fixed scalar result rows before setup/warmup. A temporary
walker reads direct known fields and exact types; it must not call `vars`,
read `__dict__`, materialize instance dictionaries, use generic referent
discovery or retain graph objects/IDs in the result.

The walker follows the global `taskWorkArea.taskList` chain. For each group
of six task nodes, require the constructor's head order: Device B, Device A,
Handler B, Handler A, Work, Idle, with exact classes and task identifiers.
Require a distinct correctly typed `handle` for every task. Follow task
`input`, the appropriate record's `pending`/`work_in`/`device_in`, and packet
`link` edges. Deduplicate packet identities: repeated paths to a packet are
legitimate and are not an ownership error. Require eight distinct packets
per six-task group, no packet sharing across iteration groups, and one
distinct exact four-element data list per packet. Reject a task-link cycle,
duplicate task/record/data ownership, unknown type, missing node, extra node
or an overlong traversal. Use exact counts as traversal ceilings so a malformed
graph cannot cause an unbounded walk.

Check all source-defined payload fields directly, rather than only counting
named graph nodes. Task priorities/identifiers are the exact role constants;
its three state fields are exact booleans. Idle/worker record numeric fields
remain exact bounded int32 values, with the worker destination restricted to
the handler identifiers. Packet identifiers/kinds are the source's task/kind
constants, datum is a bounded scalar integer, and every data-list element is
an exact integer equal to zero or a generated A–Z code. Pointer fields accept
only None or the nominated exact packet type. Pin an exhaustive source field
inventory so a new strong payload field requires a new oracle review; do not
use dynamic dictionary inspection to discover it. This prevents an omitted
strong payload from being credited as accounted graph growth.

Every counted node must be in the same population as `gc.get_objects()`:
require `gc.is_tracked(node)` for each exact supported task, record, packet
and list instance before allowing its count. Class objects, global constants,
numeric values, borrowed/static nodes and untracked objects receive no
allowance. The source audit establishes six new tasks, six records, eight
packets and eight lists per iteration. The witness must establish the actual
typed identities and counts; source arithmetic alone is insufficient.

With two loops, cumulative iteration counts at the three scored retention
points are six, eight and ten. Require exact per-class counts and graph totals
168, 224 and 280. Store only the verified scalar counts in the preallocated
rows. At each point, the scratch sets/queues and walker frame must be dropped,
including after the final point. Do not add a cleanup major: earlier scratch
is eligible for the next already-planned major, and final scratch is not part
of a subsequent measurement. Peak RSS remains captured at the timed return,
before retention. Any unexplained observer retention still faces the same
limits.

Only after all three graph witnesses pass, compute
`unexplained_growth = (tracked_last - tracked_first) - (graph_last - graph_first)`.
Require `unexplained_growth <= 32`. Do not subtract the observed historical
112 by fiat or accept a partial graph. Keep
`live_bytes_last - live_bytes_first <= 16384` unchanged; no graph byte
allowance is introduced. Preserve raw total counts, graph counts, both growth
values and the independent graph-validity result. A graph failure rejects the
row before normalization can credit it.

Retain the existing inspection calls. Label three majors as explicit;
historical `gc.get_objects()` additionally performs its existing manual
collection after each live-byte reading, while adopted/cadence inspection
does not. Make no claim of equal total post-hot collection counts. This
amendment changes neither that implementation nor the measured interval.

Required causal controls before a fresh campaign:

- A missing/extra/wrong-type graph node, duplicate task/record/data owner,
  task-link cycle, wrong iteration count or untracked claimed member rejects
  graph validity. A repeated packet edge deduplicates successfully.
- A normal complete source-derived graph with bounded unexplained growth
  passes; 33 newly retained unexplained tracked owners per batch rejects the
  object bound even with valid graph counts and byte growth below 16 KiB.
  A fabricated graph allowance cannot hide that excess.
- Excess total bytes still rejects. Wrong kernel bytes, loops or actual
  graph counts reject before subtraction. Host synthetic checks validate
  arithmetic/guards only; actual graph ownership and missing-reclamation
  controls require a separately authorized bounded native feedback row.

After independent design acceptance, propose at most 20 minutes host-only
helper preparation using the accepted controller and narrow observer changes.
Freeze exact hashes and controls for review before any native work. Root must
then choose a finite actual witness/control exposure and, if those pass, a
new complete campaign with its own explicit clock and unchanged overall
04:18:11 UTC limit. Discard the incomplete campaign's timings from any
performance decision; preserve all original evidence. No automatic retry,
appended remaining samples, reset, bound widening or runtime change follows
from this proposal.
