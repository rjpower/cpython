# V8 recommendations: coordinator decision ledger

2026-11-08. The [source comparison](v8-review.md) produced eight suggestions.
Its ranked recommendations are the reviewer's proposals, not automatic
implementation authorization. The user requested comparison of architectural
ideas before selecting a direction. Runtime remains qualified R5; the tiny
refresh diff in the isolated worktree is uncommitted, unbuilt and untested.

| Review item | Scope | Current disposition | Evidence needed to select it |
| --- | --- | --- | --- |
| 1. Preserve specialized facts through call preparation | Structural implementation candidate | Compare with phase/frame/compiler alternatives | Actual routes, surviving provider checks/spills, a shared prepare/commit proof and covered machine work |
| 2. Known code-object canonical home | Local cleanup | Optional companion; held | Correct shared pointer/tag domain, exact ownership, removed scalar checks and measured net effect |
| 3. Duplicate successful refresh check | Local cleanup | Source-safe diff preserved; held | Include only with a justified batch; do not portray it as the parity mechanism |
| 4. Phase-aware state publication | Structural emitter candidate | Open | Hot paths' redundant PC/home/address traffic, exact escape boundaries, observer/error semantics |
| 5. Ownership-aware register equivalence | Structural compiler candidate | Open | Surviving moves/dispatches classified by snapshot, window, line boundary or compiler limitation |
| 6. Operand classes and call-window layout | Structural IR candidate | Open | Dynamic classification/window costs, range/encoding migration, supported large functions and instrumentation |
| 7. Dispatch and helper machine interface | Structural backend candidate | Open | Actual hot spills/reloads and dispatch costs under fixed GCC; code footprint and transition counts |
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

The selected workload is unchanged `logging_silent`. Its new pinned baseline
is 1.657289x upstream. No architectural choice or parity result follows from
that number alone. The coordinator will record the selected batch and why the
other alternatives wait before restarting implementation.

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
observer-aware call-return fusion are being specified as competing concrete
mechanisms. Their coverage and safety remain hypotheses pending review and
diagnostics.

Existing stats now provide a dispatch denominator: 161 handlers and 20 frames
per workload loop, with exact linear scaling after subtracting an empty
interval. The loop executes nine retained result POPs and ten line-boundary
NOPs. The tenth static benchmark POP belongs to pre-loop setup; the tenth
logging call already discards its result. Earlier reviews' ten-hot-POP wording
is superseded by this source/statistics reconciliation. See the
[diagnostic results](diagnostic-results.md) and the competing
[structural mechanisms](mechanisms-proposal.md). Dispatch shares are not time
shares or a projected speedup.
