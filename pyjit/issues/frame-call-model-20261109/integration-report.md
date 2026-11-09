# Frame/call model: first integration measurement

2026-11-09. Intermediate runtime `6ef45e391aa8462be3d165d8d9056e4e06e0a634`,
branch `pyjit/frame-call-model-20261109`. **Not qualified or promoted.** Golden
`0765e9be47f` remains immutable and selected. Logging is near upstream in an
early screen; the parity and broad-regression gates are still open.

## What changed

Known Python calls use scalar guarded binding plans and direct owning overlap.
Eligible empty kwargs stay virtual until a real read or frame publication.
Results name a slot or discard directly, with an executable continuation.
Checked canonical transfers avoid publishing already canonical values again.
Compact instruction positions and source-only edge observations are separate;
construction, persistence, tracing and position readers use the validated map.
See the [proposal](frame-call-proposal.md), [position contract](ir-position-proposal.md)
and [independent source reviews](source-origin-post-review.md).

## Early elapsed results

Unchanged kernels, GCC 15.2, JIT disabled, CPU 15, fixed hash seed, seven timed
samples after warmup in each process. Two balanced rounds per comparison;
source, ELF, module and driver identities are stable before/after. These are
diagnostics, not the required two independent six-round parity campaigns.

| Workload | Candidate / upstream | Candidate / golden |
| --- | ---: | ---: |
| logging_silent | **1.003646** | **0.742426** |
| spectral_norm | — | 1.017657 |
| unpack_sequence | — | **0.833936** |
| pickle | — | 1.017587 |
| float | — | 1.028035 |
| deepcopy_memo | — | 1.092133 |
| nbody | — | 1.055594 |

Logging/upstream pairs are 0.991228 and 1.016219. Its mean exceeds the parity
gate; do not round this into a pass. Golden comparisons are a separate campaign,
so their ratios are not an algebraic decomposition of the upstream result.
Unpack's former regression reverses. Pickle and float losses shrink in this
screen; deepcopy_memo and nbody remain material losses. The prior candidate's
spectral gain does not survive this combined image. Repeat broad losses and
attribute them before selecting further performance changes.

Evidence: `E11/first-engineering-upstream/` and
`E11/first-engineering-golden-v2/`. The first golden attempt was rejected before
timing because the coordinator supplied a nonexistent module-log path; its
`first-engineering-golden/status.json` is retained. The corrected run does not
alter controls or the measurement protocol.

## Hardware work and profile

Eighteen pinned warmed-region counter captures pass with stable identities and
at least 99% event running. Values below are median user-space counts per
unchanged benchmark outer iteration, which performs ten logging calls.

| Counter | Upstream | Candidate | Candidate / upstream |
| --- | ---: | ---: | ---: |
| Instructions | 13,943.98 | 15,068.04 | 1.08061 |
| Cycles | 4,780.74 | 4,851.05 | 1.01471 |
| Branches | 2,257.99 | 2,561.01 | 1.13420 |
| Branch misses | 2.01175 | 2.00591 | 0.99710 |

The previous candidate retired about 19,561 instructions per outer iteration.
The combined redesign reduces the measured excess from roughly 5,617 to 1,124
instructions, about 80%. This is a combined-image comparison, without component
ablation. The compiled binder proof establishes removed generic preparation and
allocation work on eligible hits; sampling does not apportion the whole gain.
Cache events are retained as diagnostics and are not used to claim a cache cause.

Two cycle profiles complete with zero lost samples, about 2,000 samples each.
Header inspection retains `Invalid HEADER_EVENT_DESC` and zero sample
timestamps, as in the prior packet. Treat these as bounded flat-location
diagnostics, not a validated capture-clock timeline; counter captures are
independent.
Candidate flat self samples identify method lookup with values (19.92%),
shape-local reads (11.47%), direct general Python calls (11.43%), boolean
branches (7.26%), exact calls (5.87%) and frame clear/return. These percentages
are locations to inspect, not additive speedup budgets. Upstream groups much of
its execution in one evaluator symbol; do not subtract those symbol shares.

Two initially unresolved libc addresses account for 11.99% of candidate self
samples. DSO-aware reports place them in libc's short memory-fill implementation;
callchains reach `_PyFrame_Initialize` from both overlapping call paths. Native
disassembly confirms their `memset@plt` call. This motivates the separate
initialized-arena proposal; it does not prove that removing that call saves 12%
elapsed time. The actual hot logging frames have no constant homes, so constant
borrowing has no direct per-iteration budget there.

Receipts: `E11/first-perf-{stat,record}/`, `first-perf-summary.json`,
`first-perf-identities/` and `first-profile/`. Calibration uses fourteen fresh
inner timings per role from the two elapsed processes. Retained ELFs permit
later symbolization; no native build changed during these captures.

## Correctness and remaining work

Release checks 117 modules. The 22-module integration executes 2,694 tests,
18 modules passing. Remaining categories are debugger owner recipes and four
duplicate source-line scenarios. A subsequent bounded repair fixes three of
those scenarios; the nine-module follow-up runs 1,009 tests and passes GC,
weakref, finalization, UUID, frame-call, source-boundary and result-disposal
modules. Tight-loop duplicate lines and debugger resumes remain failing.
Full release/debug suites and API qualification have not run this packet.

The next admitted increment is the independently reviewed
[semantic completion model](semantic-completion-proposal.md): loop/send
outcomes, generator return/resumption and physical static programs. Compiler
and runtime ownership outcomes must agree, including throw/close and shared
exits. The [debugger proposal](debugger-owner-recipe-proposal.md) starts with
actual loop anchors and known-null call cells rather than historical stack
depth. A follow-on arena invariant will be reviewed against allocation,
header/value reuse, frame copying, exceptions, re-entry and chunk caching.

After each coherent increment: focused native validation, independent actual
source review, basic matched timings and broad-loss controls. Full suites,
29-workload screens and the two parity campaigns remain final admission gates.
Do not stop at this initial logging improvement.

`E11` denotes
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/frame-call-model-20261109/`.
