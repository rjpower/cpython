# Twelve-hour tracing-GC decision

- State: active hard budget; parity unproven
- Owner: coordinator
- Start: **2026-11-01 03:14:44 UTC**
- Cutoff: **2026-11-01 15:14:44 UTC**; no silent extension
- Machine-readable authority: [budget.json](budget.json), 43,200 seconds

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

Every coordinator turn and new GC execution grant checks budget.json. Clamp
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

The active [integrated-storage source attempt](../fundamental-runtime-design/status.md)
keeps its 03:21 checkpoint/04:21 ceiling; the author targets an earlier 03:40
freeze. Its initial checkpoint passed source review only, not native tests or
performance. No current candidate has earned parity or integration.
