# Ninth integration: logging timing target reached

Runtime `488e85fa3a88aed7d673c154b22d40ba42080a18` is integrated and
independently reviewed. Logging passes the two-campaign timing gate. The overall
goal remains active: three broad regressions and full qualification remain open.
Golden remains immutable and selected.

## Logging confirmation

Unchanged logging_silent, GCC 15.2, JIT disabled, CPU 15, fixed hash seed, matched
native identities/configurations and balanced process order. Each campaign has
six fresh process pairs and seven timed calls per process after warmup.

| Campaign | Candidate/upstream geometric mean | Pair range |
| --- | --- | --- |
| 1 | **0.902351** | 0.884926–0.921023 |
| 2 | **0.898412** | 0.870622–0.936931 |

Both satisfy mean≤1.0 and every pair≤1.02. All pre/post identities are stable.
These establish the logging timing checkpoint at this source, not a qualified
promotion or attribution of the batch's gain to the last cold-state change.
Evidence: `ninth-logging-confirmation-{1,2}/` under the E11 evidence directory.

## Integrated model and checks

The interpreter now projects exact nonneutral committed observer edges through
one stopped-state view. It copies the actual site's context/save/origin rows,
applies the selected normal transfer, then invalidates consumed old origins.
Physical publication cannot revive an old owner token. New permission requires
inherited context, empty stopped/target saved-binding states and no incoming
target scratch demand. Existing PRE/neutral strict recipes retain their proofs;
nonneutral POST cannot use their pre-site survivor or source-only fallback.
Tracing's six argument-window reads now use explicit borrowed views.

Release/debug each pass 455 frame/observer tests with one skip, including all
of monitoring. Independent review then found a size_t allocation warning:
the narrow correction rebuilds 117 modules and passes the seven targeted checks
in both profiles. The 455 gate was not rerun after that arithmetic-only cleanup.
Committed identity builds check 117 modules each. All five core design pages are
cross-reviewed and synchronized. No IR format or metadata change; magic 64416.
See the [independent post-review](nonneutral-post-independent-post-review.md).

The first targeted run failed two new negative fixtures that listened to LEFT
instead of the actual taken RIGHT conditional. Root's encoded-IR/event probe
established their routes; the independently reviewed fixture correction retained
all refusal/state assertions. Failed and corrected evidence remains preserved.

The last full release diagnostic belongs to31: 54,563 tests, six failed modules.
Current focused checks repair monitoring and tools/API-lint. Peepholer,
sys_settrace and pdb remain open, and the GC-verifier negative control failed in
the full run despite isolated method/module passes in three build roles. No
repair or cause is established for that control. Full release/debug, current
stats remain open. The current API gate passes 306 units with zero failing
findings and zero parse errors (`ownership32-api-release-lint.log`).

## Unchanged broad screen

Two balanced pairs per control, all 29 kernels, stable pre/post identities.
Elapsed ratios are current/control; lower is better. The geometric means are
**0.966319/golden** and **1.190384/upstream**. They are equally weighted workload
ratios, not weighted production throughput or a per-bytecode comparison.

| Workload | / Golden | / Upstream |
| --- | ---: | ---: |
| richards | 0.940321 | 1.211363 |
| go | 0.942856 | 1.226527 |
| nbody | 1.057538 | 0.910579 |
| float | 0.998856 | 0.813054 |
| spectral_norm | 0.891484 | 0.772277 |
| raytrace | 0.943921 | 0.985965 |
| deltablue | 0.974985 | 1.713607 |
| chaos | 1.007879 | 0.973425 |
| nqueens | 0.996650 | 1.240705 |
| fannkuch | 0.995155 | 1.282847 |
| generators | 1.077531 | 1.504844 |
| comprehensions | 0.950310 | 1.322036 |
| coroutines | 1.078861 | 1.455585 |
| unpack_sequence | 0.823413 | 1.105755 |
| deepcopy | 0.971134 | 1.348377 |
| deepcopy_reduce | 0.981538 | 1.353681 |
| deepcopy_memo | 0.985159 | 1.309918 |
| json_dumps | 0.994087 | 1.327055 |
| json_loads | 1.004631 | 1.233735 |
| pickle | 0.997393 | 1.105412 |
| unpickle | 0.983191 | 1.210930 |
| regex_compile | 0.979421 | 1.191884 |
| logging_format | 0.983274 | 1.280433 |
| logging_silent | 0.639079 | 0.912049 |
| logging_simple | 0.979402 | 1.296595 |
| xml_etree_generate | 0.984440 | 1.231383 |
| xml_etree_iterparse | 0.989616 | 1.240056 |
| xml_etree_parse | 0.989235 | 1.123780 |
| xml_etree_process | 0.991896 | 1.355694 |

Four originally repeated regressions are flat or faster in this screen:
unpack, float, pickle and deepcopy_memo. Nbody remains slower. Generator and
coroutine losses also appear; do not hide them in the aggregate.

The separate four-pair repeat confirms all three:

| Workload | / Golden geometric mean | Pair range |
| --- | ---: | --- |
| generators | **1.070556** | 1.059214–1.079126 |
| coroutines | **1.086596** | 1.078896–1.095435 |
| nbody | **1.048799** | 1.035692–1.057417 |

Evidence: `ninth-engineering-{golden,upstream}/` and `ninth-repeat-golden/`.
The earlier local-load component campaign used ratios of round medians and a
shared-build retained executable; it is a distinct protocol and cannot be
combined with these campaign ratios.

## Current investigation and follow-up

Current PMU/profile collection is being prepared for the three repeated losses.
The retained c142 captures remain historical; no current counter attribution is
claimed. The source review identifies two testable questions: general semantic
completion on generator/coroutine returns, and repeated destination-discard
checks on nbody's arithmetic/iteration path. Both can remove dispatches while
adding C work, so inspect current counters and emitted assembly before choosing
a change. See [generator routes](generator-resume-cost-source-review.md),
[nbody routes](nbody-source-cost-review-after32.md) and the
[current PMU plan](current-pmu-plan-after32.md).

The [cold architectural review](cold-review-after32.md) finds the normal
frame/call owner model coherent. It recommends current shape measurements before
shortening guarded calls and a narrowly scoped maintenance review of overlapping
child commits. A new frame ABI rewrite is not justified by source size alone.

Debugger coverage follows the documented useful-jump contract, not every
CPython-specific crossing of active context managers. The shared NULL-only
requirements proposal remains design-only. Review each remaining failure as a
normal semantic bug, a supported safe-jump gap or an explicitly unsupported
crossing; no blanket skips or weakened assertions. The next integration must
retain logging's timing gate while repairing the confirmed broad losses, then
complete coherent qualification, current profile evidence and final review.
