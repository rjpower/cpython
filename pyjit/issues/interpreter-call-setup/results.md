# Generic call-setup hypothesis: not selected

Decision: reject a new call/argument/value-interface implementation from the
current evidence. There is some frame setup and canonicalization cost, but the
captures do not show a shared redundant generic setup or boxing boundary large
enough to justify that change. This is an attribution result, not proof that
calls are optimal or a measured speed outcome.

## Evidence and replay

Use the existing qualified `0b15b4bbc9b` O3 images and four frozen profiles in
[the finite performance packet](../../../build/gc-post-cleanup-performance-v2/finite-completion.json).
No new workload, profile, build or timing was run. A light
host extractor reads the saved perf-script text and accepted PID/hot bounds,
period weights and residual classification. It does not decode `perf.data`.

- [Extractor](../../../build/interpreter-call-setup/extract.py).
- [Derived stacks and raw input hashes](../../../build/interpreter-call-setup/extraction.json).
- [Coroutine setup PCs and inline locations](../../../build/interpreter-call-setup/coroutine-setup-pcs.json),
  using the load bias already retained by the
  [earlier residual replay](../../../build/interpreter-residual-costs/replay.json).
- [Source boundary](../../../build/interpreter-call-setup/source-boundary.json)
  proves all ten examined source/workload files still match the captured source.

The extractor is SHA256 `5ce20763f70101b17aadec8cfe251fcdf62b795a0e3a54e0413986b0e472a4bd`;
its derived JSON is `76c3761101a83285a6fa5c3e20bbb2ec35d908022d92b0eb44d60e377e929725`.
Replay with host `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python
build/interpreter-call-setup/extract.py`. The separate PC mapping uses only
host `addr2line` against the unchanged qualified image; it executes no target.

There are 1,656 coroutine, 1,292 comprehension, 987 generator and 646 Richards
samples in the accepted hot intervals. The following are percentages of total
sampled CPU, not fractions of residual alone. Each row is its own context;
inclusive and innermost rows overlap and must not be added.

| Context | Coroutines | Comprehensions | Generators | Richards |
| --- | ---: | ---: | ---: | ---: |
| Inclusive `_PyEvalFramePushAndInit` | not sampled | 0.077% | 1.722% | 1.238% |
| Innermost `initialize_locals` | not sampled | not sampled | 0.507% | not sampled |
| Inclusive frame setup helpers | 1.932% | 1.084% | 0.912% | 5.573% |
| Innermost `_PyValue_FromObject` | 1.872% | 2.012% | 0.608% | 3.560% |

The frame-helper row is the union of initialization and frame push helper
ancestries, counted once per sample. It includes required work and nested
conversion, not an available saving. No selected residual stack names
`_PyValue_BoxInline`, `_PyValue_BoxOrAbort` or `_PyValue_FromLongObject`.
Absence of samples does not establish zero executions or zero cost.

Two tempting larger attributions are invalid. `_PyEval_Vector` is an outer
entry ancestor of all residual samples, so its inclusive share merely repeats
61.715%/73.375%/78.217%/100% residual CPU. Likewise, the coroutine's outer
`gen_send` → `PyValue_Object` → evaluator ancestry encloses useful execution;
charging that subtree to value conversion would label the whole computation
boxing overhead.

## What the coroutine PCs actually show

The 1.872% innermost `_PyValue_FromObject` share separates into these call sites:

| Caller | % total sampled CPU | Actual operation |
| --- | ---: | --- |
| `RETURN_GENERATOR`, generated line 16553 | 0.725% | Canonicalize the new generator/coroutine result |
| `GET_AWAITABLE`, generated line 9554 | 0.604% | Canonicalize the returned awaitable iterator |
| `_PyFrame_Initialize`, header line 277 | 0.543% | Canonicalize the code pointer stored in the frame |

These are existing object-to-value conversions. They are not allocations of
numeric argument boxes. The generated exact-call frame setup is at line 7178;
its source branch is the already-specialized generator-owned caller path.
The innermost initialization share itself is only 0.785% of total coroutine
CPU, including required header/slot stores. Most remaining evaluator samples
are not identified as argument setup by this investigation.

The generator workload traverses Python `Tree.__iter__` methods through native
iteration slots. Its small general-call subtree therefore has a concrete
consumer, rather than proving a general setup defect in recursive coroutine
calls. Lookup, iteration, yield/resume, truth tests and allocation remain
separate work. The source and sampled distinctions are recorded in
[the boundary review](design.md).

## Disposition

Do not implement a general exact-argument shortcut, new call protocol, trusted
value constructor or slot bridge rewrite in this issue. The Python exact-call
path already bypasses generic argument parsing and uses unboxed value arrays;
the current sparse generic-setup observations cannot justify widening that
machinery. Existing captured frame movement motivated a prior bounded child,
which was rejected after its own measurement. That result remains unchanged.

Keep the reviewed nursery and shape increments on their shared qualification
path. Revisit call setup only when a named remaining consumer and repeated
removable work are demonstrated; this decision requests no new profiling or
benchmark campaign. The broad two-times aspiration is a planning goal, not
support for assigning unclassified residual time to calls.
