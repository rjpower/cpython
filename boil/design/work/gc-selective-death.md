# Selective death processing

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
