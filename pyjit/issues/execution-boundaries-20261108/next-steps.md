# Next steps after the bounded boundary packet

2026-11-09 UTC. The [report](report.md) closes this as a reviewed, qualified
candidate checkpoint. **Performance promotion is withheld; golden remains
selected.** Logging is flat versus golden and 39.3% slower than upstream.
The broad geomean is flat, with five repeatable 4.6–9.8% regressions despite a
repeatable 16% spectral gain. No component cause has been isolated.

## Disposition of all nine golden-review ideas

| Rank | Idea | Current disposition |
| --- | --- | --- |
| 1 | Discard call results across return | Implemented for synchronous exact/general Python child completion; native, generator and other handlers retain existing paths |
| 2 | Logical source boundaries instead of executable NOPs | Static bool-edge execution skip implemented; canonical dormant anchors remain, general mapping/physical compaction deferred |
| 3 | Static operand ownership modes | One measured local/temp shape-reader alias implemented; broader mode products not admitted |
| 4 | Minimize exact/general frame entry/return | Next diagnostic: separate actual route and ordinary-return costs before selecting a structural change |
| 5 | Borrow child arguments from explicit anchors | Still a valid alternative; quantify removable mortal ownership pairs and finish consumer/GC/promotion proof before implementation |
| 6 | Checked semantic/effect/phase contracts | Extended only as needed for descriptors, save-IP and commit validation; no whole-handler purity or general multi-output proof claimed |
| 7 | Validity/ownership maps replacing universal NULL homes | Global maps deferred; first census one non-reentrant clear/overwrite opportunity against current owner invariants |
| 8 | Bounded polymorphic execution chains | Deferred until a real alternating site demonstrates churn/misses; existing feedback rows are not an execution PIC |
| 9 | Shared optimizing SSA/backend/recovery | Later stage; begin with a bounded reconstructible before-effect exit, separate from JIT-off interpreter tuning |

The first three are bounded mechanisms, not completion of the entire original
recommendations. Frame/region retention and borrowing remain serious options;
no selected decref position or owning-frame policy is a language law. See
[language and lifetime policy](../../design/lifetimes-and-language-semantics.md).

## Completed gates and retained limits

- Reviewed source integrated at `4b3d739d867` and pushed as
  `pyjit/execution-boundaries-20261108`; golden tags and builds are unchanged.
- Release and debug each passed 54,421 tests, debug on retry after an unexplained
  profiling failure. Retain its original log and passing controls; no runtime
  repair or baseline flakiness was established. Cache/table and incremental
  magic-header defects were repaired separately. API lint covers 306 units.
- Six-round target, two-round 29-workload and four-round regression-repeat
  campaigns have stable identities and unchanged workloads. Removed handlers
  do not establish a logging speedup. Parity fails decisively.
- Complete component sources were archived before retiring their two worktrees.
  Integration source/builds and evidence remain for reproducibility.

## First follow-up: isolate the tradeoff

Use one compiler and one source change per comparison. A Sol 6.1 medium worker
should propose the smallest source ablation separating bool-boundary selection,
discard completion and the operand alias; another independently reviews its
producer/consumer/cache mappings. Regenerate aliases and cache metadata rather
than introducing a permanent switch. The coordinator builds and times it.

First measure unchanged logging, spectral and unpack against golden and this
candidate. Add pickle/float only when the first screen identifies a useful
component separation. A result must preserve spectral's benefit or explain its
tradeoff while removing the repeatable broad costs; do not infer this from
handler sizes, fewer words or a profiling sample percentage. The cold reviewer
checks arithmetic, identity and cause claims. Broad screening and full suites
belong at the next coherent candidate, not every diagnostic ablation.

If no bounded component yields a useful result, retain golden and close that
hypothesis. Do not spend unbounded effort on address alignment. The independent
source review found no basis to attribute unpack's regression solely to return
handling: its hot unpack operations do not call Python.

## Next bounded investigation

Logging still retires about 40% more instructions than upstream with similar
cycles/instruction. The new profiles continue to identify preparation, frame
retirement, method lookup and return. Use the measured instruction gap to set a
plausible work budget; do not infer removable cost from sample shares alone.

In parallel with the reviewed ablation proposal, a second Sol 6.1 medium worker
should build one cost ledger for exact entry, guarded general preparation/commit and
ordinary return. Logging's 20 frame entries remain, so nine fewer POP entries do
not explain the remaining whole-call cost. Include the new common recipient
branch, transaction flag handling and text effects; reconcile mutually exclusive
routes rather than summing them as though every call pays every path.

Alongside it, count attributable mortal argument owner pairs and surviving
anchors from retained evidence or bounded diagnostics. Compare direct ownership,
explicitly anchored borrowing, frame/region retention and local coalescing by
removable work, memory quality and proof cost. No option wins by assertion.
The coordinator selects one small prototype only after comparing these options.
Require explicit decline/error/observer states,
independent review, focused tests and a compiled-work budget. Broad qualification
and quiet timing belong at coherent integration points. Keep golden immutable.
