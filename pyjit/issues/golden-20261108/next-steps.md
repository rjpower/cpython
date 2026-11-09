# After golden: separate logical boundaries from execution work

2026-11-08. This is the coordinator's follow-up to the
[fresh cold review](v8-cold-review.md), the
[qualified measurements](../../bench/results/interpreter-parity-20261108.md)
and the [golden publication](README.md). It supersedes the earlier ordering in
the [completed parity packet](../interpreter-parity-20261108/plan.md).
No new runtime implementation is included in this publication.

## Design judgment

The runtime is a real register interpreter with a frame stack, overlapping
calls and explicit owners. Its main architectural weakness is that physical
words and homes still carry several different contracts: source observation,
ownership retirement, instruction position and debugger reconstruction. Local
dispatch shortcuts preserve that coupling. The next work should remove
unnecessary execution while giving those contracts explicit representations.

The review's V8 comparison supports a cheap executable baseline, source-position
metadata, bounded call specialization and a separate optimizing graph. It does
not justify assuming JavaScript's collection or call semantics apply to Python.
All nine recommendations are assessed below; none has an established speedup.

## Recommendation dispositions

| Rank | Recommendation | Decision and first evidence | Risk / condition |
| --- | --- | --- | --- |
| 1 | Finish discarded call-result lowering across return | Design now: identify the nine retained logging result POPs and model their post-return events/destruction order | Medium; tracing may start inside the callee, so removing POP must preserve the caller line and finalizer order |
| 2 | Replace executable NOP source anchors with logical boundary metadata | Design now, independently of borrowing: census NOP reasons/incoming edges, then specify one boolean-continuation edge boundary | Medium for a bounded form, large generally; jumps, monitoring, exception attribution, suspended frames and `f_lineno` must use coherent mappings |
| 3 | Encode frequent static operand ownership modes once | Bounded experiment after a census: one hot reader's dominant local/temp mode; inspect generated GCC output before timing | Medium; avoid handler Cartesian products and code growth that worsens spectral |
| 4 | Minimize actual exact/general frame entry and return | Diagnose now: separate route-specific loads, stores, guards and owner operations; prototype a code-layout descriptor only if it removes demonstrated work | Medium/large; keep dynamic guards, code ownership and post-kwargs revalidation |
| 5 | Borrow synchronous child arguments from explicit anchors | Diagnose now as an alternative: count attributable mortal argument pairs and surviving owners; finish consumer/GC/promotion design only if the removable budget is substantial | Large; the prior proposal permits diagnostics and a representation decision, not implementation |
| 6 | Extend checked semantic/effect/phase contracts | Design alongside ranks 1–3, limited to facts required by those transformations; begin with one multi-output reader | Medium/large; generic paths retain conservative effects, and warmed specialization is not a compile-time purity proof |
| 7 | Replace universal valid/NULL homes with explicit validity/ownership maps | Defer global maps; first inventory non-reentrant clear/overwrite pairs and assess one local coalescing | Small locally, very large globally; future-read liveness is not an owner or GC map |
| 8 | Execute bounded polymorphic feedback chains | Defer until alternating real sites show misses/churn; a two-shape synthetic case alone is insufficient | Medium; current logging is monomorphic, and recorded POLY feedback is not an execution PIC |
| 9 | Complete shared optimizing SSA, backend and recovery | Retain as later stage; define a bounded before-effect recovery gate before native execution | Very large; saves nothing in current JIT-off timings, separate from baseline frame-slot allocation |

Detailed source anchors, covered work and semantic gates are in the review.
The ranking is a review priority, not permission to combine all changes into
one patch. Borrowing remains a serious option; it no longer predetermines the
next implementation. No new word format, permanent dual representation or
second compiler is selected.

## Workstreams and order

The table names roles, not currently running agents. The next runtime work
starts in a new experiment/branch based on the immutable golden snapshot.
Workers never commit or run concurrent shared builds/benchmarks.

| Packet | Role | Concrete deliverable and first gate |
| --- | --- | --- |
| A: logical boundaries | Sol design/review, Terra medium implementation when admitted | Separate designs for post-return/discard and NOP edge metadata; map logical event identities to physical resume positions and required state. Demonstrate event order before code removal |
| B: operand modes | Terra medium census/implementation, Sol peer review | Mode frequency, one dominant variant's generator/deopt/instrumentation mapping, GCC instruction and text delta. Stop if the compiled comparison already costs little or text cost dominates |
| C: call/owner cost ledger | Terra medium bounded diagnostics, independent Sol review | Exact versus general entry/return work and attributable borrowed-argument candidates; reconcile mutually exclusive routes and surviving owner identities |
| D: contracts and documentation | Luna medium, after accepted designs | Keep current specs, generated-contract rationale, decision ledger and diagrams synchronized; proposed and implemented behavior stay separate |
| Integration | Coordinator | Choose the next coherent increment from A/B/C evidence, serialize native validation and measurements, obtain cold post-review, and make an explicit keep/reject decision |

Use at most available concurrency. First useful packets should fit 30–60
minutes; preserve progress if a source census needs longer. A's two proposals
may share a logical-boundary model, but should retain independent coverage and
admission evidence. Do not make cheap operand-mode work depend on the complete
metadata redesign. Do not delay every simpler improvement behind borrowed homes.

For C, existing call route counts, raw IR and profiles come first. New temporary
counters must distinguish argument provenance, owner anchor and exact/general
path, scale against an empty interval, and be removed before quiet timing.
Aggregate 90 increments/100 decrements per logging loop do not identify how many
pairs borrowing can remove. Fresh kwargs allocation may call a reference tracer;
it is not a proven no-callback region. General-path final guards remain necessary.

## NOP and discarded-result acceptance

Ordinary execution should have no handler, NOP word decode or PC store solely
to traverse an empty boundary in the admitted form. Instrumented execution may
take an explicit observation path. Preserve distinct ordered boundaries even
when they map to the same physical resume PC; source line numbers alone are
insufficient. Boundaries on taken edges cannot be represented only as metadata
on a following fallthrough instruction.

A discarded result must still be released at the correct point relative to
caller line/opcode events and finalizers. The current return holds the result
owned across child cleanup, then publishes or discards it in the caller;
child-cleanup finalizers must not see the caller destination changed early.
`IR_NO_DST` already exists; proving
an unused result is only half of the lowering. A mandatory post-return event
must not silently become another equivalent unconditional handler/check.
Compare event and retirement sequences against upstream and golden, including
tracing/monitoring enabled inside a callee, exceptions in callbacks, caller
local mutation, returned last-reference objects and frame escape.

The same design must address word-indexed jumps, line/exception tables,
traceback/lasti, legal and illegal `f_lineno` jumps, monitoring's patchable
positions, generators and suspended frames. A bounded prototype can cover one
boundary kind, but must identify all consumers rather than declare the others
irrelevant. Unsupported shapes retain their existing semantics during a staged
migration; the accepted final design has one selected mapping contract.

## Integration and performance gates

1. Propose → independent peer review → implement → focused tests → post-review
   → integrate. Regenerate from producers and compile release as well as debug.
   For an opcode change, run its meaningful generator/value/observer/call cases
   and the established smoke set; save full suites for coherent checkpoints.
2. At each integration, compare unchanged `logging_silent` against golden and
   upstream with pinned CPU, loop counts, native JIT-off assertions and clean
   identities. Use route/handler counts and GCC instructions to explain covered
   work; a lower dispatch count alone is not a speedup.
3. Screen Richards, call/attribute cases, scalar kernels and **spectral**. The
   latter's +8.6% retained-R5 regression remains accepted baseline debt, not a
   new normal to hide. A narrow earlier-increment comparison is a useful
   discriminator if its CPI regression blocks admission; compiler changes and
   speculative layout tuning are not the first response.
4. At a coherent kept batch, run the broader 29-kernel comparison, full release
   and debug suites, module checks, API lint and an independent evidence/code
   review. Capture perf stat/profiles only for unresolved mechanisms. Preserve
   warning and lost-sample receipts. Retain failed or rejected experiments.
5. Logging parity still requires two fresh balanced six-round upstream/candidate
   campaigns, paired elapsed GM ≤1.00 and every round ≤1.02, under the
   [existing protocol](../interpreter-parity-20261108/plan.md). Meaningful
   progress may be kept with declared control regressions; label it progress.

The next user checkpoint should report the chosen structural change, what
physical work it removed, semantics proved, target/control timing and remaining
gap. A measured rejection is also a useful result. Keep startup RSS and zero-box
counts outside the throughput admission gates. The golden tags never move.
