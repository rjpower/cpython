# Residual cost decision

Outcome proposed: **reported**. Select a separate full-range frame-transfer
experiment; no interpreter runtime change or performance improvement is landed
by this issue. The existing evidence answers the attribution question without
another native run.

## Actual evidence

The frozen [finite index](../../../build/gc-post-cleanup-performance-v2/finite-completion.json)
is `88bf6c7f5c597f9f742fe942ab0df2538602d83e454ecb6a92a0978722c625d8`.
Its candidate is source/native `0b15b4bbc9b`, integrated as `aee46aacae2`.
The existing [disjoint analysis](../../../build/gc-post-cleanup-performance-v2/phase-analysis/analysis.json)
is `17c0457dc7e7e3ebfb24dacdd7eaac06f8c1f2e2d9354a2735a96eb72f707d31`.
All source files cited below are byte-identical between that candidate and
integration `b3f2bcc605c`; current documentation migration changes no runtime.
The raw captures and earlier analysis were read, not rewritten.
Replay the derived shares with
`UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python build/interpreter-residual-costs/replay.py`.
The [small extractor](../../../build/interpreter-residual-costs/replay.py)
(`f64bbe4f6876d049074764d5640379831fd69651f52a954fb357de29565e76ff`)
prints the [retained derived JSON](../../../build/interpreter-residual-costs/replay.json)
(`668f44c2e6ec42b218567974ec1c50a67aaf3d585bcfe271bec76345488392b9`).
It records absolute raw input paths/hashes and derives the coroutine PIE load
bias from the captured mmap record before selecting the ELF loop interval.
It reads captures with host perf; it starts no benchmark or interpreter target.
The [published performance record](../gc-cleanup-scanning/performance.md),
[interpreter source review](../interpreter-shared-costs/reviews/holistic.md),
[emitted shape review](../interpreter-shared-costs/reviews/polymorphic-shapes.md)
and [root/code organization review](../gc-cleanup-scanning/reviews/code-organization.md)
provide the prior contracts; their historical source identities remain explicit.

Reparsing the four frozen perf-script texts with the accepted PID, inclusive
integer hot bounds, compiler-suffix normalization and event-period weights gives:

| % of total sampled CPU | Coroutines | Comprehensions | Generators | Richards |
| --- | ---: | ---: | ---: | ---: |
| Existing residual category | 61.715 | 73.375 | 78.217 | 100.000 |
| Residual physical evaluator leaf | 37.319 | 37.539 | 56.535 | 80.960 |
| Residual innermost `_PyFrame_Copy` | 6.884 | 0.851 | 1.418 | 0.000 |
| Residual unknown physical leaf | 7.729 | 3.793 | 3.242 | 8.359 |

Physical and inline rows overlap and must not be summed. Unknown leaves may
have known outer context. There are 1,656/1,292/987/646 selected samples and no
lost records in the accepted captures. These are sampled CPU shares, not elapsed
partitions or call counts. All instruction/skid/stack-symbol limitations remain.

The coroutine physical leaves also include `_PyCode_InstrOparg` 1.993%,
`_Py_NewReference` 1.812%, `_Py_MakeCoro` 1.812%, `_PyObject_GC_NewVar` 1.570%
and `_PyObject_MiMalloc` 1.449%. These are exclusive physical-leaf observations,
not complete allocation subtrees. They demonstrate why residual CPU cannot all
be assigned to call dispatch. Comprehensions has 5.882% at `_Py_dict_lookup` and
2.864% at `_PyList_AppendValueListResize`; generators has 2.837% at
`PyObject_IsTrueValue` and 2.026% at `lookup_method_ex`. Workload data operations
and general adaptation coexist with evaluator execution.

## Concrete frame-transfer finding

[`_PyFrame_Copy`](../../../Include/internal/pycore_interpframe.h#L234) copies
the header, then loads, writes and clears every physical slot. The qualified O3
image retains a scalar loop: ELF addresses `0x4589c0`–`0x4589db` perform the
load, destination store, zero store, increment, compare and branch. Offline
`perf annotate` confirms these instructions; no new sampled process was run.

Within this capture, innermost-copy samples whose PCs lie in the loop setup/
body interval `[0x4589ad, 0x4589dd)` account for **2.476% of total CPU**
(206,030,125 of 8,321,607,000 event-period ns). The sampled return-slot store PC
alone accounts for 2.838%; RETURN_GENERATOR overwrites that field afterwards.
Store attribution can include skid/cache effects and is not proof that deleting
the store saves that fraction. The **6.884% whole-copy share is only a local
upper bound**, not a forecast for bulk-copy or prefix-elision gains. A payload
change targets a smaller part, and faster copying may still lose at tiny sizes.

There are exactly three real callers:

- [`RETURN_GENERATOR`](../../../Python/bytecodes.c#L5396) allocates final storage,
  moves the transient frame, publishes CREATED/owner state and the caller's
  result before popping the source frame. Its generated case is the same call,
  not a fourth independent user.
- [`take_ownership`](../../../Python/frame.c#L57) moves into an escaped frame
  object's storage, then repairs frame ownership/back links, including a later
  fallible back-frame allocation.
- [`gen_new_with_qualname`](../../../Objects/genobject.c#L1387) moves into a
  fresh generator and rebinds the existing frame object.

Each destination has separate allocation/storage from its source in the current
callers. The existing shared helper is therefore the first place to prove and
express a nonoverlapping full-range move. No arbitrary source-tail omission is
licensed: [`codegen_comprehension`](../../../Python/codegen.c#L5121) deliberately
inserts GET_ITER/GET_AITER before RETURN_GENERATOR. A generator expression may
have a live iterator temporary. `_PyRegCfg_OpSlots` also treats window bases
separately from compacted input slots; a stack-depth shortcut needs its own proof.
Locals-only copying is rejected; initialized-prefix elision is deferred.

## Alternatives and interpretation

Do not select a wholesale call/resume redesign from this profile. Exact Python
calls already share thread-owned argument windows; generator-owned callers use
separate storage. Return-slot publication and RESUME skipping already exist.
The profile does not prove their required stores redundant.

Richards has 6.811% innermost `_PyFb_ShapeRow` and 7.276% `_PyIR_FbEntry` samples,
but is already 1.886× upstream in the matched elapsed diagnostic. Later-row
duplicate shape admission survives O3 according to the existing emitted-code
review; dynamic row frequency is still unknown. That remains a legitimate
separate candidate, not the selected coroutine/generator repair. Current
NOARGS conversion, bool-condition and constant-mode costs are not isolated by
these captures, so none is selected on a source-presence argument alone.

The five arithmetic diagnostics remain useful guidance: candidate adjusted
instructions per inner add are about 45 integer/48 float, while raw elapsed is
about 6.17/6.95 ns. Their cold/warmed disassembly retains arithmetic, but inputs
are invariant. They do not measure a recurrence. Source inspection of
[`_BINARY_OP_ADD_INT`](../../../Python/bytecodes.c#L659) confirms compact-int
guards, int32 addition and an allocating int64 overflow path; existing
`test_ir.test_yield_to_each_kind_of_resumer` contains `n += 1`. No fresh recurrence
execution/disassembly was claimed or launched here.

The existing matched ratios are upstream/candidate 0.618 coroutine, 0.603
generator and 0.847 comprehension; same-fork parent/candidate ratios are
1.417/1.085/1.621. They describe the qualified combined change, not this proposed
transfer experiment. Serial samples are correlated and generator ranges overlap.
Neither frame-copy attribution nor removal of every sampled copy could explain
the full remaining deficit. Young allocation/death work remains independently
motivated and can proceed without edits to this frame boundary.

## Next

Assign the child [implementation proposal](plan.md#proposed-child-full-range-frame-transfer)
in a separate worktree only after its source scope is reviewed. Prove all caller
storage ranges, retain payload/header/publication semantics, inspect changed O3
instructions, then decide a small matched experiment. Reject the implementation
if it adds complexity without simplifying emitted code or measured cost.
This issue needs no new profile, full suite, benchmark campaign or runner.
