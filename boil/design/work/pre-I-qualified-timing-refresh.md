# Qualified post-GC timing refresh

Preparation updated2026-10-30 after qualified4da is integrated as75e8.
Final12f1ea31 and postmergef56e0b6b close GC qualification/integration; runtime
library/tests remain dac6, ordinary native3c6 and ASanf945. No refreshed
performance score or interpreter ABI implementation is claimed.

**Current phase: one owned upstream eval-nogcse comparator.** Concrete accepted
configure/make have passed (closure23c9ab25, independent actual-build review
c51871cb). Bounded18-module correctness/29 scored-driver origin packets
remain separate pending checks. This document itself grants no native
execution or timing. Reuse the
qualified fork O3 products and preserve the existing upstream stock image.
An extra stock-policy comparison is optional for a named question, not an
automatic second29-row campaign.

## Comparison and remaining qualification

| Input | Established source/configuration facts | Required before measurement |
| --- | --- | --- |
| Owned upstream | Detached `321925c92c9e67864cafa1566840927f6f07e90a`, own `build/default`, clean tracked source at this audit. Existing make closure `dddcf24d`,18-module correctness and50-row actual-driver origin evidence remain separate accepted receipts. | Preserve it unchanged. The separate `build/eval-nogcse` has configure/make/product closure23c9ab25 and independent build reviewc51871cb. Its bounded correctness/origin checks and transaction bindings remain pending. No new source checkout or stock rebuild is needed. |
| Fork | Serving `boil-buffer-xid-main-only` at4da (runtime library/tests dac6); O3 images compiled from3c6. Actual primary75e8 differs only in15 declared project Markdown paths; final12f1/postmergef56e clear review. | Bind the final qualification/source/native and owner handoff in the new transaction. Keep executable, Lib and extensions in their qualified serving tree; no primary overlay or rebuild. |
| Compiler policy | Both current Makefiles use O3, retained frame pointers, zero-initialized automatic variables, computed-goto/non-tail dispatch, static libpython, shared GIL, nondebug/nonstats, no PGO/LTO/BOLT; all JITs are off/absent. | Assert actual configuration and compiler commands in the new upstream receipt and final fork binding; directory labels alone do not establish these facts. |
| Eval policy | Stock upstream `CFLAGS_CEVAL` is empty; fork uses `-fno-gcse`. The separate eval-nogcse cohort matches that flag in its actual configuration/ceval compilation, accepted in23c9/c518. | Bind that built cohort into correctness/origins and the timing transaction. Do not substitute the stock image or infer an actual flag from the directory label. |

The preserved stock upstream Makefile hash is `38a4eb03`; fork release is
`c62c2650`. The new cohort uses its own23c9 receipt, not the stock hash.
Both have `-DNDEBUG -g -O3 -Wall`, `-fno-omit-frame-pointer`,
`-mno-omit-leaf-frame-pointer` and `-ftrivial-auto-var-init=zero`.
The build receipts agree on GCC, cc1, collect2, assembler, linker, ar,
ranlib, make and shell bytes. GCC resolves to `/usr/bin/x86_64-linux-gnu-gcc-15`,
SHA256 `b5f1b773a7c733738352000c92a077dc5852a1a2fc6d836b1e411be1e9ec5f88`.
Spelling `gcc` versus `/usr/bin/gcc` is not a compiler difference under the
accepted clean PATH. Revalidate bytes at the new build; no compiler was
executed for this audit.

Reuse the [owned upstream build recipe](pre-I-baseline-source-setup.md),
including its explicit O3/GIL/nondebug/nonstats/JIT-off/non-tail/FP/static/
no-PGO-LTO-BOLT options and zero-init environment, with only the new build
path/label and `CFLAGS_CEVAL=-fno-gcse` added. Configure then make-j4 under
normal admission and owned build locking; capture config, tools, compile
log, products and cleanup using the accepted helpers. Do not edit upstream
source or the old configured Makefile, copy old extensions, or run make in
an active fork build. Execution requires the concrete phase inputs, receipts, review and owned slot;
GC integration is complete.

This matches compiler policy, not Python implementations: allocator, object
model, GC, interpreter and supported main-only scope remain architectural
differences. Upstream is the pinned source baseline, not a claim about the
latest upstream release or best PGO/LTO deployment. Keep the known external
SDK candidate-versus-consumed-header provenance limit explicit; matching
current library paths does not reconstruct historical header consumption.
Bind the new build's actual dependencies with the existing receipt mechanism.

## Reuse the timing transaction

Reuse the accepted [quiet29 transaction](pre-I-first-quiet-timing-plan.md),
its task-owned `quiet-ce2ac-binding-v1/{paired.py,phase_runner.py}` bodies,
[registry](../../bench/run.py) and [driver](../../bench/internal/driver.py).
Create a fresh output/binding label; preserve the ce2ac result as historical.
There is no new runner, timer, workload or general evidence framework.

The registry file changed since that campaign: `f8abc0b3` became `7ec1cdf6`
through H3's Py_STATS metadata/count reporting. Source comparison confirms
that BENCHMARKS assignments, Context and measure_time are unchanged; all43
other frozen shared files, including driver7e61be5c and the29 kernel rows,
match their old hashes. Rebind the current registry byte identity rather
than ignoring the stale manifest. Py_STATS code is not executed in this
elapsed transaction.

The held measured command, constructed by unchanged Context.command, is:

```
BUILD/python -S -X pycache_prefix=Q/ROW/BUILD/pycache \
  P/boil/bench/internal/driver.py time BENCHPATH REGISTERED_LOOPS 5 [VARIANT]
```

Use each runtime's own Lib/extensions. Before timing, use the existing origin
parser and run-mode recipe for the current fork's29 scored rows, and for the
new upstream product; old upstream product origins do not qualify a different
binary. One regex_compile time-mode RUNS0 witness per runtime covers the
shared JSON tail, including its actual JSON/_json origins. It is not
per-workload time-mode tracing. Reuse existing wrong-origin/parser controls;
no blanket correctness ladder or50-row micro campaign is needed for this
comparison. Required backend absence or wrong imports stop the campaign.

Both timed runtimes must be ordinary release: JIT, Py_STATS, verifier,
root-stat output and stress off; omit PYTHON_GCSTRESS/PYTHON_GCVERIFY rather
than injecting string0. Set explicit child PYTHONHASHSEED=0 in the reviewed
clean environment, with no loader injection, site overlay or -I/-E that
silently ignores it. Use fresh task caches/tmp, same cwd/locale/timezone,
registered loops/variants and the driver's unchanged one warmup.

Reserve a quiet serial window after native gates/builds stop, using the
accepted dual-slot/build-lock supervisor. Fix the same physical core and
idle sibling, record affinity/load/readable power policy before/after,
and retain the established quietness rejection. Do not change governor or
compete with a gate. Keep raw output and rejected vectors; no automatic
retry, partial denominator, source retargeting or extension of the window.
Exact final source/product/origin/reservation/launch hashes remain held.

Each row has one fresh process per runtime, alternating first runtime across
registry order, then five serial timed calls inside each process. Five is an
inexpensive initial estimate, not five independent experiments or a confidence
threshold. Report every row, raw vector/spread and upstream/fork ratio;
complete-suite score is exp(mean(log(upstream/fork))). Extend only for a
named unstable result or decision, with balanced fresh process pairs and a
bounded extra cost. Do not measure until a desired score appears.

## Decide the next shared cost from the refreshed result

The old ce2ac score0.828243 and GC-heavy profiles are historical hypotheses,
not current ratios or promised generation savings. After the refresh, reuse
[setup/warmup/hot-window attribution](pre-I-shared-cost-attribution.md) on
three persistent regressions and one object/allocation control. Separate
elapsed timing from profiling. Existing release GC time/candidates/bytes,
root deltas and bounded native samples can distinguish collection/death
bookkeeping from frame/resume/call/value movement; queued callback time is
not fully included in GC duration. Do not add root time to GC time or infer
object ages from collected counts.

Use that evidence to accept or reject the conditional
[first-slice decision](pre-I-first-slice-decision.md): finite numeric receiver
closure versus shared frame/call work or generated producer/consumer facts.
No ABI rewrite, generational barrier rollout, typed LOAD_ATTR family or
speedup claim follows automatically from this plan.
