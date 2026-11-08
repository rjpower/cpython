# Interpreter architecture and cleanup integration

Coordinator execution plan, 2026-11-07. Start from clean runtime
`7780e32e0e2` and its existing GCC release/debug/stats builds. The preceding
[assessment](../interpreter-throughput/holistic-assessment-20261107.md) and
[measurement report](../../bench/results/interpreter-throughput-20261106.md)
are the baseline, not evidence that this batch has passed.

## Outcomes

1. Remove write-only call bookkeeping and give common Python calls and the
   consuming fallback a clear implementation boundary. Preserve the existing
   prepare/guard/infallible-commit ownership protocol.
2. Replace generator knowledge of five attribute opcode names with a declared
   semantic phase/output contract. Use the same facts in emission and an
   independent analyzer or verifier, including exceptional and folded-local
   behavior. Keep one producer of semantic information.
3. Apply a measured interpreter improvement enabled by those contracts or by
   a proven call-path invariant. A smaller source or ELF alone is insufficient
   evidence for a throughput claim.
4. Specify the optimizing IR's Action/FrameState/recovery boundary precisely:
   owned homes, PC, exception state, completed effects, guard exits and callback
   invalidation. Keep this distinct from interpreter operands and future-read
   liveness. The native method/tracing backend remains its own stage.
5. Land the reviewed composition, validate it and publish current performance
   and remaining limits without treating zero-box counts as an admission gate.

## Parallel work and file ownership

| Stream | Owner | Boundary | First result |
| --- | --- | --- | --- |
| Calls and dead state | Terra medium | `Python/ceval.c`, assigned call tests | Source-backed proposal, then separate cleanup and common/fallback implementation |
| Shared phase/output design | Sol | Proposal and independent source review; no runtime edits | Exact semantic facts, producer/checker consumers and ownership/error proof |
| Generator implementation | Luna medium | Cases-generator producers and assigned generator tests | Implementation map first; implement only the reviewed contract |
| Recovery interface | Sol after first contract review | Canonical optimizing-IR/recovery design | Concrete boundary and validation obligations, separating planned from built |
| Integration | Coordinator | Shared headers, generated products, builds, tests, measurements, commits | Review composition, freeze each integration and resolve cold-review findings |

Workers do not commit, run shared builds or native performance workloads.
Independent proposals and source review run in parallel. Runtime edits wait
for the initial timing freeze to end, and dependent interfaces are agreed
before implementation. Shared header and generated-output changes belong to
the coordinator. Existing unrelated edits and old worktrees remain untouched.

Each first packet has a 45–60 minute proposal/first-result budget, a progress
file updated at least every 30 minutes, and a scope cut to the smallest useful
result. Missing proof triggers a narrower implementation, not speculative
state tracking, a second opcode table or a second compiler experiment.

## Review and integration ladder

1. Capture a fresh seven-kernel upstream/candidate baseline at frozen source
   and executable identities. Retain the existing load guards and raw vectors.
2. Independently review proposals before workers edit. Check real producers,
   consumers, generated C and the relevant GCC output; distinguish required
   publication/revalidation from duplicated work.
3. Implement one coherent increment. Start with syntax/generator checks or the
   changed semantic tests. The coordinator release-compiles the actual source;
   debug checks cover ownership assertions. Regenerate from producers.
4. Review the actual diff and test evidence, commit it, refresh build identities
   and run the basic matched performance screen. A fresh cold reviewer reads
   the integrated source before its rationale and checks correctness, simpler
   abstractions and remaining machine-work opportunities.
5. Resolve review findings before a dependent landing. Keep changes with
   structural merit distinct from measured speedups; reject performance-only
   complexity that cannot justify its cost.
6. At the coherent calls/contracts/interpreter boundary, run full release and
   debug suites once. Repeat only for a subsequent semantic correction that
   warrants them. Complete module and value-API checks on the merged builds.
7. Freeze source/builds for the final matched 29 kernels and declared
   pyperformance subset. Use selected hot-region counters/profiles to answer
   actual remaining questions, with the existing source/build/load/roster
   checks. Do not pool failed prefixes or select favorable scores.
8. Review the evidence and report independently, update canonical designs and
   status, then give the user the landing summary and measured limits.

## Non-negotiable contracts

Caller homes remain owning through fallible/reentrant preparation. Final
guards precede transfer; detachment cannot be followed by fallback or an
escaping/failing operation before the child is published. Unsupported providers
keep their semantics and error/monitor ordering.

Every physical home remains a valid PyValue or NULL. Known-NULL overlap reuse
does not authorize sparse roots, early observable-local destruction or use of
future-read liveness as ownership. Generator/error/frame observers must agree.

Attribute results are published before receiver retirement; folded-local
replacement happens afterward, preserving displaced-owner finalization order.
Phase/output facts must describe success, failure and callback state. A fact
does not survive reassignment or an opaque mutation merely because it is named
in metadata.

Performance comparisons use GCC and fixed build flags. Native instructions
per workload loop are not per-bytecode costs. Ownership correctness, removed
source work and repeatable elapsed improvement are separate conclusions.

## Evidence and live state

Raw evidence uses the established build storage:
`build/pyjit-link-wave1/interpreter-architecture-20261107/` under
`python-boil-the-ocean`. Canonical plans, proposals, reviews and receipts live
here. R1–R5 are integrated through `0953757f4e7`; release and debug full suites
passed with zero failures, and release/debug/stats builds match the source.
Collection is complete: the 29-kernel candidate/upstream and candidate/interim
factors are 1.247431/1.044650, and the selected 12-result subset factors are
1.048074/0.878774. The direct R4/R5 whole-kernel factor is 0.992103; independent
review accepts keeping R5 with its local costs documented. The profile
campaign has 54 valid perf-stat invocations and six valid DWARF captures with
zero lost samples. No upstream parity is established. Independent final
evidence/report review is complete; the batch is closed. See the [R4](integration-r4.md),
[R5](integration-r5.md) and [current report](../../bench/results/interpreter-architecture-20261107.md).
The recovery-interface amendment passed independent re-review and landed in
canonical commit `cc52f097df0`; its native implementation remains future work.
