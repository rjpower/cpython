# Eleventh integration: specialized shared completion

Runtime `bfd67cbe4b410c9c78292a8eb17849bb24edcb20` uses one completion
implementation with known SEND/FOR kind and canonical input on proved VM
returns. Native and THROW providers retain general publication. There are no
new frame fields, per-resume continuation stores or ownership transactions.
The existing PRE/POST observation, alias-safe detach/publication, exception
preservation and final PC reload remain in the same source order.

## Review and focused qualification

Both independent reviewers accepted the actual one-file C diff, the canonical
caller proof and native results. Release/debug build 117 modules each and pass
575 tests across 11 modules each; committed identity builds also check 117.
No tests, generated handlers, metadata or cache magic changed in this increment.
These focused gates do not replace final full qualification.

Compiled VM completion removes the nested eight-argument helper call, redundant
publication and inapplicable kind/owner checks. The generic provider still
canonicalizes. Combined completion text grows **1,194 bytes** (2,198→3,392),
whole text grows 1,152 bytes and no separate completion clone appears. Static
inspection finds no unconditional owner spill bank on normal VM paths;
observation/error paths still spill. Code-size and path facts do not predict
elapsed gains. The distinct retained33 baseline is
`E11/completion34-baseline/python`; a separate copy captured during rebuilding
is explicitly unqualified and is not that baseline.

Evidence: `completion34-{release,debug}-{build,focus}.log`,
`completion34-committed-{release,debug}-identity.log`,
`sharedstep34-assembly-review/`, the
[compiled review](sharedstep34-compiled-review.md) and
[independent post-review](shared-completion34-independent-source-review.md).

## Basic elapsed performance

Unchanged kernels, GCC 15.2, JIT disabled, CPU 15, hash seed0, four balanced
fresh process pairs per workload and seven timed calls per process. Native,
configuration/module and before/after identity gates pass. Ratios are
current/control, lower is better.

| Workload | / Golden | / Upstream |
| --- | ---: | ---: |
| logging_silent | — | **0.801505** |
| generators | **1.070491** | 1.500233 |
| coroutines | **1.026246** | 1.371763 |
| nbody | **1.045276** | 0.892967 |

Every logging/upstream pair is between0.781748 and0.819897. Logging preserves
parity in this basic check; the full two-campaign six-round gate belongs to32.
The three golden losses persist in all four pairs. The gains/losses relative
to prior campaign ratios are not isolated before/after component effects.
In particular, this campaign does not attribute logging's change to a
generator-completion helper. The last full29 screen remains the32 evidence in
the [ninth report](ninth-integration-report.md).

Evidence: `completion34-repeat-golden/` and `completion34-basic-upstream/`.

## Current normalized counters

The sealed FIFO interval excludes setup and warmup. Three balanced rounds per
role/case produce 18 valid captures, both events100% running, no capture errors
and byte-identical before/after identity snapshots. Independently recomputed
ratios use the arithmetic mean of raw counter/hot-loop counts per role.

| Workload | Cycles / golden | Instructions / golden | CPI / golden |
| --- | ---: | ---: | ---: |
| generators | 1.084226 | 1.032996 | 1.049594 |
| coroutines | 1.029274 | 0.990849 | 1.038779 |
| nbody | 1.036633 | 0.986041 | 1.051308 |

Coroutine and nbody losses now accompany fewer instructions than golden;
generators retain3.3% extra instructions. No single added-work explanation fits
all three. These counts do not establish a layout, allocator or cache cause.
The source32 recordings remain historical location evidence, not new34 profiles.
No new recording was needed to verify this bounded instruction-count question.
Evidence: `completion34-workload-perf-stat/` and
`completion34-workload-perf-summary.json`.

## Next source streams

1. **Canonical-home transfer.** Actual SEND_GEN assembly already coalesces its
   apparent duplicate state loads, so consolidation has no demonstrated gain.
   Plain YIELD instead performs real publication checks on an unchanged value
   taken from a canonical home. A bounded full-chain proof can reuse the
   existing canonical return sink. A separate exact list-child proof can reuse
   the canonical final store while preserving mandatory early publication
   before list retirement. Neither changes state checks, recursion, recipients,
   pending redirects or instrumented-yield semantics. Actual source and mutation
   tests require independent review before generation and native validation.
2. **Safe NULL-only PRE transfer.** In an isolated source worktree, a shared
   cold physical query and the existing frame analysis gain forward must-NULL
   facts and independent VALUE/NULL_ONLY demands. Existing strict adapters
   retain their ALL-demand projection. First permission is ordinary real-LINE
   PRE with inherited context and empty saved bindings; three unchanged
   peepholer witnesses and strong negative/reentry controls drive admission.

Workers have nonoverlapping files. Only the coordinator builds, commits,
integrates and measures the merged runtime. Full release/debug qualification,
current stats, remaining debugger cases, unresolved GC negative control and
final cold review remain open. Golden stays immutable and selected; the goal
remains active.
