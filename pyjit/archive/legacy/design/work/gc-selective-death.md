# Selective death processing

Closed component/as-built record. The combined integration and measurements
are complete; see [current GC status](../../issues/gc-cleanup-scanning/status.md).
Pending gates and no-measurement statements in the historical timeline below
retain their original checkpoint meaning and do not assign new work.

Current combined measurements: [performance report](gc-post-cleanup-performance.md)
records1.417× coroutine/1.621× comprehension parent throughput and0.871× upstream
across29. These do not isolate this component. Earlier no-measurement statements
below describe the qualification checkpoint.


2026-10-30 as-built reconciliation. Current reviewed source is `0b15b4bbc9b`;
its operator composite qualification is PASS. Independent LAND clears this integration;
postmerge smokes pass. The selective-only measurements and failed
campaign below retain their original source/native identities. They are not
measurements or full-suite passes for the current combined source.

This increment starts at `070ce55ea16`. Implementation `788695e30e9`
classifies potential cleanup work during the existing heap walk:

- Weakref work uses the registry flag, retaining the static builtin type's
  separate list case.
- Finalizer roots exclude already finalized objects and finished generators
  whose finalizer is a no-op. All objects remain available as ordering
  connectors; delivery still checks current eligibility.
- An independent exact memoryview/managed-buffer bit gates retention work and
  avoids repeated type tests. Current death and release-readiness checks remain.

There are no new allocations or worklists, callback cadence changes, ownership
transfers, or collector budget changes. Existing OOM rollback remains intact.
This is category selection, not compact stage lists. The completed O3
measurements below show a coroutine improvement; broader effects remain bounded
by the observed variation.

The fresh GCC O1/Py_DEBUG build passed its audits and 116 module checks, with
zero import failures and ten missing optional modules. Tests used actual
kill/extras0/N0. The finite controls passed 13/0 skips; the four whole modules
(`test_gc`, `test_weakref`, `test_memoryview`, `test_buffer`) passed 599/46 skips.
These scopes overlap. They recorded 219 and 10,784 collections respectively,
with zero unwind fallbacks. The whole-module run reported one unpinned detach
record, consistent with the passing `StackWalkTests.test_unpinned_counter`
control; its conservative scan is distinct from an unwind fallback. The module
aggregate is not a separate per-case trace. Original products, configuration,
tools and post-cleanup guards passed.

The first new test incorrectly expected a callback-free weakref to clear
before resurrection. Its failure and twelve passing controls remain preserved.
Pinned upstream `321925c92c9`, `Python/gc.c:762–800`, deliberately delays that
clearing through finalization (GH-135552). Test-only successor `69c34fb1f66`
asserts the surviving plain ref's identity and usable data, callback-bearing
ref clearing, eventual clearing of both refs, and single finalization. O1
native products remain from `788695e30e9`; no runtime repair or rebuild was needed.

The immutable bounded index is
`build/selective-death-resurrection-validation/completion.json`, SHA256
`f463bf790acbd55efd20bd254763cfb168bbb6d7e9ff1907793f85ea7a95e2ea`.
Broad qualification and landing remain pending.

The fresh GCC O3 build at source/native `10d8773815c` completed its audits and
116 module checks, with zero import failures and ten missing optional modules.
The accepted frame-pointer, zero-initialization and evaluator `-fno-gcse`
configuration was retained. Source, tools, configuration, 588 products and
post-cleanup guards passed. Its build result is
`build/selective-death-release-build/release/build-result.json`, SHA256
`7f1d593e315d134281e95407126045c67cf9283fc8ea5c0e0c7a321f2f44bc5f`;
post-cleanup receipt SHA256 is
`11ca7793785718733633aa9f5ef719f93b89fc1e0bce6356eb70dff95078de00`.
Build/import checks do not provide O3 broad behavioral qualification.

The quiet paired-four comparison used qualified parent checkout `4da`/runtime
`dac6`/native `3c6` and candidate native `10d`, with matched declared O3 build
policy. Both built-in JSON-tail origin witnesses and final product/source
cleanup checks passed. Each side ran one warmup and five serial measured calls
in one fresh process. All 40 values were retained; variant order alternated
between rows. Values below are milliseconds per timed workload call, with each
call executing the listed number of registered loops.

| Row | Loops | Parent median [min, max] ms | Candidate median [min, max] ms | Parent/candidate |
| --- | ---: | ---: | ---: | ---: |
| Coroutines | 4 | 219.667 [203.076, 223.784] | 167.858 [161.473, 171.423] | 1.308649 |
| Comprehensions | 6000 | 205.070 [192.763, 276.260] | 193.040 [178.265, 228.361] | 1.062317 |
| Generators | 2 | 104.795 [102.577, 127.604] | 97.200 [94.132, 116.972] | 1.078140 |
| Richards | 2 | 61.635 [61.351, 63.262] | 62.567 [62.163, 62.750] | 0.985113 |

Coroutine median elapsed fell 23.59%, with nonoverlapping observed ranges.
Comprehension/generator median elapsed fell 5.87%/7.25%, but ranges overlap;
Richards was 1.51% slower and also overlaps. The five calls are correlated
within each process, not independently randomized pairs. These observations
provide no confidence interval, 29-row extrapolation, or upstream performance
claim. The paired index is
`build/selective-death-paired-four/paired-completion.json`, SHA256
`34033f1352e2979760c6587a80db293c3a74d12ae4d35439ccab1931cd40a4a7`.

The subsequent finite13 native CPU/counter diagnostic corroborated the
coroutine result at the same 32 MiB budget and 301 hot-window collections:
parent/candidate hot elapsed was 12.294/8.729 seconds, collection duration was
6.022/3.302 seconds, and measured hardware counts were 50.08/37.21 billion
cycles and 85.28/79.34 billion instructions. Whole-process peak RSS was
67.4/67.5 MiB. The recorded CPU stacks, counters, PID and hot timestamps are
preserved; all 13 processes and cleanup/source/product checks passed.
This single instrumented 50-call window has different wrapper/setup scope
from paired-four, so it is not a sampler-overhead measurement or an additional
set of independent timing samples. GC duration excludes queued Python delivery;
sample absence does not establish zero callback invocations.

The finite13 index is in the primary worktree at
`build/checkpoint-i-current-cost-attribution/native-comparison-v1/completion.json`,
SHA256 `9ef2649b698565b56f73fc2e95809a220d4c155dc3c46bccd4f04c9788698598`;
its counter/RSS summary SHA256 is
`4a8a087e75d01b69669d786351a6d2ce8015de90144e3f8357bff5221dc2904d`.
The documentation commit containing this update is later than measured source
`10d8773815c`; runtime files and native products remain unchanged. O1 behavior
remains the bounded scope above. O3 measurement is complete; candidate broad
qualification and landing have not been claimed.

The subsequent source-frozen campaign at `94ae1b1d4a2` passed release and
O1 debug normal/kill fulls, release stress11, ASan16 and the early controls.
Final debug stress11 remains **FAIL**: native regrtest returned zero, but
`test_asyncgen` reported an unraisable `KeyError('__import__')`; ten other
modules passed. The unchanged validator rejected that environment change,
and closure did not run. Failure index
`build/selective-death-qualification/stress11-debug-failure-completion.json`
has SHA256 `287d7efde85701ba43b6e732d8095ab0e2928a4a810e2277c498add1aa156d6a`.
All three original product/configuration/tool guards passed after cleanup.

A single deterministic diagnostic on the sealed O1 image reproduced the
external bug without asyncio: collecting a never-awaited coroutine inside a
lambda with empty builtins lost its RuntimeWarning and emitted two
`KeyError('__import__')` unraisables. Its result SHA256 is
`8312369807635565b088547b54eaf31d299357ded35dd5578b027bfff7f354b3`.
The warning helper tried importing through the interrupted frame's builtins,
then repeated that lookup during fallback display. The focused repair reads
the current interpreter's warnings module first and, when absent and imports
are available, imports through interpreter machinery. It preserves dynamic
attributes, explicit blocked module entries, late-finalization fallback and
genuine helper errors. New behavioral regressions cover restricted-frame
delivery and cached/replaced/missing/blocked modules; the existing broken
coroutine-helper regression remains required. Warning repair `b184fa91128` passed independent source review and joined the
coherent runtime at `4d8e690f483`. Historical campaign results and sealed products
remain unchanged; they are not repaired by that later source commit. Current
combined qualification is recorded below.


## Current combined source and evidence

The coordinator composed the selective category gates with native suspended-
generator cleanup, direct cell/tuple/frame scanning and the warning repair.
Exact shared-effect declarations for the instruction decoder and `strrchr`
followed actual body proof and unit controls. They did not change program C or
weaken the root/linked audit. Frozen source `33944d24e55` contains that reviewed
composition; its actual build histories retain earlier native identities.

Source339 passed three normal/kill full profiles of 51,255 tests each and
release stress11 of 3,115/148. Final debug stress remains FAIL: a coroutine
constructor tracked its object before initializing the origin/finalizer slot,
then positive-depth origin construction allocated while traversal could visit
that slot. The named fatal and postguards are preserved in
`build/gc-native-cleanup-broad/failure-completion.json` (`447af602`) in the
retained `boil-gc-native-cleanup` worktree. Parent75e8 has the same initialization
order; no matched native comparison assigns this defect to a particular new
scanner. A single unchanged-public-case diagnostic on the sealed339 debug
image reproduced the fatal, with source/configuration/product guards intact.

Reviewed repair `173adf527d5`, composed as `0b15b4bbc9b`, adds only two NULL
initializations in the shared constructors before TRACK, plus the external
origin/data regression. Existing later assignments and branches stay intact.
Four configured cohorts were independently preserved, then incrementally
rebuilt at `0b15`: GCC O3, GCC O1/Py_DEBUG, ASan Og and Clang21 O1 tail. Audits,
116 module checks and cleanup pass. Changed-unit compile proof and earlier
source/config/compiler history remain explicit; this is not a claim every
unchanged unit was recompiled.

The identical former-failure diagnostic now completes once with usable data.
The regression passes once in release off and debug kill; whole GC, generators,
coroutines and asyncgen pass 456/35 in release and 456/21 in debug kill. Tail
ownership passes 1/0. These overlap with the focused methods. Their frozen index
is `build/gc-native-cleanup-qualification-v3/focused-completion.json`
(`20e132de`); four build capsules are indexed by `build-completion.json`
(`3db801b2`) in the same directory.

The exact successor debug-stress profile passes 3,116/113 in eleven modules,
with 38,251 collections, zero unwind fallback and 43 report-only detached-snapshot
visits. Its result is
`build/gc-native-cleanup-broad-v2/broad-v2/stress11-debug.result.json`
(`1b62d612`). These report-only counts are valid detached snapshot visits,
not object counts or unwind failures. Independent final review clears the composite.

The proposed qualification is an explicit source-delta composite: retain339's
three full passes and release stress at their real source, combine them with
the proved two-store delta, four fresh build audits and current affected,
debug-stress, ASan16 and closure evidence. Do not relabel339's failed stress or
old fulls as0b15 uniform full passes. ASan16 and closure now pass, and the operator final index records qualification
PASS for that explicit composite. Independent LAND clears this primary integration; postmerge
smokes pass. No combined-source speedup is claimed. The held post-landing
measurement plan will refresh all29, the parent comparison and native profiles
only after those prerequisites and actual origins are complete.


## Terminal qualification capsule

The operator's final index is
`build/gc-native-cleanup-broad-v2/final-completion.json` in retained
`boil-gc-native-cleanup`, SHA256
`50054725c871a7d71a0eceb71db1d8373d335837aff8ed73e6dec208cb7a27eb`.
It records qualification PASS and382 absolute artifact references, with
source/runtime/native/ASan heads all `0b15b4bbc9b`. It explicitly carries the
three339 fulls and release stress at their original identities, plus the
reviewed two-store delta and fresh successor coverage; it does not claim a
uniform0b15 full-suite rerun. Original failures remain preserved.

Fresh ASan16 passes4,038 tests/141 skips, with13,095 collections, zero root
fallback and3 report-only detached-snapshot visits. Leak/fake-stack detectors
remain off; skipped multiprocessing parent wrappers do not establish leaf
process coverage. Closure passes cext16 with the exact seven declared skips,
regeneration/clinic, traversal and19 borrowed-header receipts; `_tkinter`,
`_dbm`, `_gdbm` and `readline` remain explicitly unsupported. All four source,
configuration, tool and product postguards pass. Independent LAND clears the composite; primary integration includes this record.
The two normal postmerge smokes pass; no
post-cleanup performance result is claimed.

## Integration and postmerge checks

Integrated as `aee46aacae2`; the exact difference from qualified0b15 is13
project Markdown paths. Final postmerge index
`build/gc-cleanup-integration-v2/final-receipts.json`, SHA256
`382dd354f46d84391f2e27d6c593dd043a6ee258bb3bd2ab6e76dd8ae2ff1215`,
binds23 receipts. Release passes1,937/80 and debug1,937/50; each has all nine
modules passing, native/launcher/cleanup0 and zero root fallback. Debug has
two report-only detached-snapshot visits; release has zero. These are fresh
integration checks with the qualified images, not a new native rebuild.
