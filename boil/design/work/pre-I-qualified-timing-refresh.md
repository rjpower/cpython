# Qualified post-GC timing refresh

Source-only preparation, 2026-10-30. GC final qualification, primary integration
and its smoke are still pending. No build, native execution, timing or serving
source change is authorized here. **Use one new upstream eval-nogcse build for
the primary comparison; reuse the final qualified ordinary fork O3 products.**
Preserve the existing upstream product. An extra stock-policy comparison is
optional for a named question, not an automatic second29-row campaign.

## Comparison and finite build gap

| Input | Established source/configuration facts | Required before measurement |
| --- | --- | --- |
| Owned upstream | Detached `321925c92c9e67864cafa1566840927f6f07e90a`, own `build/default`, clean tracked source at this audit. Existing make closure `dddcf24d`,18-module correctness and50-row actual-driver origin evidence remain separate accepted receipts. | Preserve it unchanged. A new `build/eval-nogcse` in the same owned source tree needs its own configure/build/product receipts and bounded upstream correctness/origin checks. No new source checkout is needed. |
| Fork | Serving `boil-buffer-xid-main-only` at dac6; ordinary O3 images compiled from3c6. Stdlib/test-only successors have explicit bridges. Eventual primary integration may change only declared project Markdown. | Final actual qualification/index, reviewed source/native bridge, primary merge/smoke and owner handoff. Keep executable, Lib and extensions in their qualified serving tree; no primary overlay or rebuild. |
| Compiler policy | Both current Makefiles use O3, retained frame pointers, zero-initialized automatic variables, computed-goto/non-tail dispatch, static libpython, shared GIL, nondebug/nonstats, no PGO/LTO/BOLT; all JITs are off/absent. | Assert actual configuration and compiler commands in the new upstream receipt and final fork binding; directory labels alone do not establish these facts. |
| Remaining eval difference | Upstream `CFLAGS_CEVAL` is empty; fork uses `-fno-gcse`. Upstream configure.ac only appends the separate Clang22 stack-size workaround; its GCC path does not overwrite an incoming `CFLAGS_CEVAL`. | New upstream configuration supplies `CFLAGS_CEVAL=-fno-gcse`. Verify the generated Makefile and actual ceval compilation, not just the environment request. |

The existing upstream Makefile hash is `38a4eb03`; fork release is `c62c2650`.
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
an active fork build. Execution stays held until GC integration completes
and the concrete build inputs and owned slot are bound.

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
