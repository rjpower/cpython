# Twelve-hour tracing-GC decision

- State: active hard budget; parity unproven
- Owner: coordinator
- Start: **2026-11-01 03:14:44 UTC**
- Cutoff: **2026-11-01 15:14:44 UTC**; no silent extension
- Machine-readable authority: [budget.json](budget.json), 43,200 seconds, plus the narrow [pre-score compiler-profile addendum](compiler-profile-addendum-20261101.json)

The user requires tracing GC to reach upstream parity averaged across the test
suite within twelve hours. Otherwise stop tracing-GC optimization and restore
one reference-counting model so JIT work can proceed. Current source, evidence
and rejected experiments remain preserved. A promising partial result does not
extend the cutoff; evidence can justify rejection earlier.

## Parity and deciding evidence

Use all **29 registered workloads**, with original setup, loops and variants
frozen in budget.json. For each, divide the fork's median elapsed per loop by
matched upstream's; the geometric mean must be **at most 1.0**. This is the
same aggregate convention as the existing broad report, whose reciprocal
speedup was 0.871× upstream. Do not select a favorable subset or substitute
instruction counts, phase timers, synthetic loops or debug/stats timings.

This assesses the integrated fork **with tracing GC**, not an isolated estimate
of collector time. Independently useful IR/interpreter changes may contribute;
the measured source and complete configuration must be coherent. Use pinned
upstream `321925c92c9`, matched GCC/O3 and disabled JIT/PGO/LTO/debug/stats/tail,
with each image's bound standard library/extensions. Freeze rotating order,
fresh-process rounds and the variability rule before scores. Borderline,
inconsistent, invalid or missing evidence does not establish parity. A known
critical correctness failure cannot qualify a candidate.

At05:44:57 UTC, before any VM0 elapsed score, the coordinator adopted the
independently [reviewed compiler-profile amendment](compiler-profile-addendum-review-20261101-v1.md).
Original budget/protocol bytes remain unchanged. Select T only if T/C and T/G
pass every already-declared gate; else C if C/G passes; else G. One selected
profile remains fixed for the timebox. A selected Clang profile earns parity
only against fresh unchanged upstream built with the same compiler/backend and
matched effective policy, with actual ordinary correctness/import evidence.
No mixed-compiler credit, arithmetic composition, profile shopping or clock
extension is allowed. All29 rows, three rounds/five values and aggregate/each-
round GM<=1 remain. New parity bindings must bind both original budget and
addendum; this declaration grants no native execution or backend adoption.

The [fixed measurement protocol](measurement-protocol.md) declares the early
three-arm storage pilot and the full-29 three-round consistency decision before
either active prototype has scores. There are no adaptive extra samples.

Aim for an early measured candidate by 05:14, broad evidence by 09:14 and the
final performance decision by 13:14, leaving time for qualification. These are
reporting checkpoints, not reasons to delay a ready measurement. The hard
decision remains 15:14:44. Full qualification is required before integration;
if later qualification invalidates the candidate, it does not reset this clock.

## Gates ordered for throughput

1. State a concrete hypothesis and the finite performance decision. Review
   critical ownership, roots/effects and recovery contracts.
2. Implement in isolation. Run cheap source/generator checks, then fresh focused
   semantic, root/lifecycle and failure tests. Review actual source/test/native
   outcomes. Preserve failures and exact source/image identities.
3. Run an early fixed elapsed/RSS pilot. Reject clear losses promptly; broad
   qualification cannot rescue them. Run all 29 as soon as a pilot warrants it.
4. For a promising surviving candidate, complete the existing expensive full
   stdlib, stress/KILL, ASan and extension qualification before landing. Qualify
   one coherent source rather than repeating the whole matrix for every idea.

This changes scheduling, not correctness requirements. A partial ownership
implementation must not be timed. Routine host helpers get a concise review
and actual import smoke checks; avoid another survey, executor or large proof
packet when existing reviewed machinery suffices.

## Enforcement and fallback

Every coordinator turn and new GC execution grant checks budget.json and its
pre-score compiler-profile addendum. Clamp
each GC activation to the earlier of its own ceiling and the hard cutoff,
including termination/cleanup reserve. Unused windows grant no retry. At the
cutoff, if valid full-suite parity is absent, stop GC work, record the failed
decision and begin the prepared RC successor; do not request another extension.
IR/JIT work continues independently of the collector choice.

`restart_audit` froze the [RC fallback decision](../fundamental-runtime-design/rc-fallback-decision-20261101.md)
early, before 03:32 UTC. Root independently inspected actual historical
Py_DECREF/stackref ownership, cyclic finalization and configure defaults, and
accepts the coherent restoration route for preparation. Source `b5541cbbbcd9`
retains register CFG/compaction and NaN-boxed frame values. Later core-wide
PyValue containers/native ABI and header shapes are deferred. The proposed
restoration is a new commit descended from the decision HEAD, not an ancestor
merge; preserve current boil records and user work. Remove owning-tracer
optionality in the successor. The manifest is available locally, but source
restoration, current-machine buildability and qualification remain unproved.

Root then prepared `build/worktrees/rc-successor`, branch `boil/rc-successor`,
from actual primary `74f1705774b`. Commit `1440f8c6fd1` restores exactly the
manifest's 887 changed core paths to historical b554 bytes; scoped source did
not change between the reviewed comparison HEAD and this actual base. Boil
records and AGENTS remain unchanged; primary user work and tracing branches
remain untouched. Two EOF whitespace warnings are inherited historical bytes.
The bounded source grant ends **05:07 UTC** and permits RC-only closure after
the owner's span review. No compiler, target or promotion is authorized by
that grant. Historical restoration still contains the old collector choice;
it is an intermediate preparation commit, not the chosen final product.
Closure then froze early at **d7df12b1d81722927e71d9734c70eeafdccfee73**:
owning collector/selector removed, RC selected and extension/cache identity
isolated. Independent source review has accepted first native feedback;
actual native validation remains.

The [integrated-storage candidate](../fundamental-runtime-design/status.md)
retains its earlier failures: v4 passed 26 debug cases, then failed release's
allocator-wrapper lifetime assertion after four passes. Diagnostic changed-call-
graph nonreproduction is inconclusive about the original cause. The reviewed
cold fixture correction froze at5a457c73 and passed exact source/helper review.
Fresh v5 feedback completed05:16:16 UTC, session83297 exit0. Debug27 passed;
release23 passed plus four declared skips. Independent focused-outcome acceptance
allows elapsed readiness. The v5 pilot then stopped before targets because root
created a missing empty upstream lock after inventory seal. Source/images stayed
unchanged. The separate v6 pilot completed all120 values and twelve origins,
then independent review **rejected** the candidate: coroutines+50.74%/+51.07%,
four-row geometric means+14.79%/+10.26%. No more samples/full qualification.
All source, native failures and the first pre-target launch remain preserved.

The separate IR region increment passed semantics and actual work-removal
checks, then was **rejected**: coroutine elapsed +10.86%/+14.11%, four-row
geometric means +16.21%/+13.74%. That supplies the first measured decision before
05:14; it supplies no GC parity credit and will not be composed into a candidate.
The unchanged-runtime compiler/backend control completed all14 native rows,
independently accepted:105 selected invocations,102 passes/3 declared skips,
and five stale children/3,000 iterations. Its fixed G/C/T elapsed helpers are
being prepared through06:04. No elapsed backend selection is established.

One new page/start-map inventory source attempt is authorized through06:16,
checkpoint06:00, from maintained417 in its own worktree. The selective port
excludes rejected spans and47 nursery/cadence/ledger changes and must close an
actual nonempty failed-prefix rollback witness. It has no native acceptance.

The isolated RC-only closured7df12b passed all39 focused methods in both images,
zero skips, with independent actual-outcome acceptance and idle cleanup. It is
a working fallback candidate, not a selected collector or fully qualified core.
Canonical Autoconf reproduction then matched every source byte; ownership/modes
were restored and independently verified, as were1,268 old product/evidence files.
That obligation is closed without a new source commit or native rerun.
No native job is active. No candidate has earned parity or integration.
