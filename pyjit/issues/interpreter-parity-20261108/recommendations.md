# V8 recommendations: coordinator decision ledger

2026-11-08. The [source comparison](v8-review.md) produced eight suggestions.
Its ranked recommendations are the reviewer's proposals, not automatic
implementation authorization. The user requested comparison of architectural
ideas before selecting a direction, then authorized execution of the reviewed
batch. The selected batch is qualified and kept with explicit measured tradeoffs;
the [report](../../bench/results/interpreter-parity-20261108.md) records the
source identities and evidence. No upstream parity is established.

| Review item | Scope | Current disposition | Proof and evidence |
| --- | --- | --- | --- |
| 1. Preserve specialized facts through call preparation | Structural implementation candidate | P3 integrated at `10627c339fa`; kept at this checkpoint; measured limits in the report | All ten outer calls use allocating overlap; share final guards/commit and keep generic fallback |
| 2. Known code-object canonical home | Local cleanup | Landed at `0539e0c1ff9`; kept with the qualified measured batch | Shared pointer/tag domain and exact ownership reviewed; focused release/debug checks passed |
| 3. Duplicate successful refresh check | Local cleanup | Landed at `0539e0c1ff9`; kept with the qualified measured batch | Success-path GCC check removed; failure revalidation preserved; no parity claim |
| 4. Phase-aware state publication | Structural emitter candidate | M1 integrated at `d93b83800a5`; kept at this checkpoint; measured limits in the report | Reuse canonical publication and ordered retirement; preserve generic slow path and local callbacks |
| 5. Ownership-aware register equivalence | Structural ownership/compiler candidate | Cross-frame borrows promoted to follow-up design investigation; current-IR copy rewrite deferred | Current contract needs one owning message snapshot per call. Upstream demonstrates a borrowed alternative with lifetime anchors and heap-escape promotion; measure coverage before selecting representation |
| 6. Operand classes and call-window layout | Structural IR candidate | Encoding revision deferred; assess class costs in GCC | Current windows already overlap; no measured range/density limit justifies format migration. M1 preserves format and uses checked class-sensitive publication |
| 7. Dispatch and helper machine interface | Structural backend candidate | M2b landed at `73c20472a9f`; kept at this checkpoint; measured limits in the report | Stats confirms ten fewer handler entries per loop, with words/PC/wrappers retained. Backend/word-density rewrite deferred |
| 8. Shared native recovery | Future JIT architecture | Accepted boundary, separate stage | Prepared-owner ledger, once-only transfer and forced native exits; cannot count toward JIT-off parity |

The second pass also examines frame/constant placement and call-return line
boundaries as concrete alternatives within items 4–7. Per-frame canonical
constant homes and retained end-of-line POP_TOPs have semantic purposes;
their costs and alternatives require direct evidence. A tracing GC, changed
Python observability, shared mutable kwargs dictionary or benchmark rewrite
are not admissible ways to obtain parity.

Selection requires a comparative case: what work occurs on the warmed target,
what can safely disappear, how the mechanism generalizes, the migration and
code-maintenance cost, and what result would reject it. Do not rank by small
source diff alone or treat sample percentages as guaranteed savings. A
contiguous register window is not inherently a bad design; neither does its
existence justify every current preparation layer.

The selected workload is unchanged `logging_silent`. Its pinned R5 baseline
is 1.657289x upstream. The batch above follows the source comparison, exact
contracts, route counts and bounded migration costs; the elapsed number alone
does not select it or establish parity. Keep each increment only after source,
correctness and performance review.

## Evidence that changes the comparison

The coordinator rechecked all 12 baseline children and all 84 raw timings,
balanced order, native affinity/JIT/hash checks, quiet-load limits, stable
before/after identities and summary arithmetic. The pinned 1.657289 factor
requires about 39.7% less candidate elapsed time to match this control. The
older 1.743034 campaign is a different protocol, not a source improvement.

Warmed frame inspection finds zero per-frame constant homes in both
`Logger.debug` and `Logger.isEnabledFor`. Constant-pool relocation therefore
does not address this target. Their temporary capacities are not counts of
stores or clears: overlap reuses known-NULL entries and ordinary frame
retirement already clears locals only.

The next source pass confirms that shape handlers already avoid refresh when
retirement cannot deallocate, and method-with-values lookup does not refresh.
Recommendation 4 must identify remaining redundant state work, rather than
claiming these existing optimizations as missing. Dictionary subscription and
ordinary-NOP continuation were selected and integrated after exact contract
review; call-return fusion remains deferred. Their measured coverage and
throughput admission are recorded separately from source correctness.

Existing stats now provide a dispatch denominator: 161 handlers and 20 frames
per workload loop, with exact linear scaling after subtracting an empty
interval. The loop executes nine retained result POPs and ten line-boundary
NOPs. The tenth static benchmark POP belongs to pre-loop setup; the tenth
logging call already discards its result. Earlier reviews' ten-hot-POP wording
is superseded by this source/statistics reconciliation. See the
[diagnostic results](diagnostic-results.md) and the competing
[structural mechanisms](mechanisms-proposal.md). Dispatch shares are not time
shares or a projected speedup.

## Ownership comparison correction

A later pinned-source audit corrects the original item 5 premise. One owning
message snapshot is required by the candidate's **current** home contract,
not by register IR or eight-byte values in general. Upstream can transfer a
borrowed argument into a synchronous child frame, protect overwritten caller
locals, and promote borrowed slots when copying an escaped frame. The current
candidate duplicates the retained message snapshot. This is a real structural
alternative for the next design packet; aggregate refcount totals alone do
not isolate its cost or establish a speedup. Preserve the current owner until
a complete replacement lifetime contract is reviewed and implemented. See the
[source comparison](borrow-frame-followup.md). The separate
[fresh-kwargs hypothesis](followup-analysis.md) must account for reference
tracer callbacks even on freelist hits.
