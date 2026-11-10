# Current41 retained profile analysis

Decoded 2026-11-10; no candidate/upstream interpreter, benchmark workload,
build or runtime source change was run.
The samples locate current work. Their percentages are **self cycle-period
shares within each recording**, not fractions of the candidate/upstream gap,
call frequencies, elapsed speedup budgets or inclusive call-tree costs.

## Identity and method

Evidence root `E` is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.
`gap41-perf-stat/status.json` records 18/18 valid invocations;
`gap41-perf-record/status.json` records 6/6. Both sessions report stable identity,
and their before/after identity manifests compare equal. Each decoded report
says Total Lost Samples: 0; retained lost-record files are empty.

| Role | Source | Retained ELF build ID |
| --- | --- | --- |
| Candidate41 | `0a35a382909ca169ce3f381cb38206b1725001b3`, clean | `6c721238a54f71e66bb76e42ed30bdce66065842` |
| Upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2`, clean | `ff51c387c04ccd5a2664e5a572bc68e491ec0db1` |

Candidate retained ELF SHA256 is
`8ad46da880df29cc61cebf371f231406a724322dd5e65fb07d2d86fd71d81f36`;
upstream is `8ed926d80536c6f768bf0bcdcb225c631bf4ff29a23cecdc88c2120345ca5867`.
Reports use the retained `gap41-perf-record/{candidate,upstream}-python`, verified
with readelf against those IDs. A temporary symfs maps original recorded paths
to those ELFs; other hit DSOs are linked only after build-ID verification.
This avoids decoding candidate41 against the mutable build executable while
root prepares annotation42. An initial incorrectly rooted symfs report had
unresolved addresses and was discarded.

The decoding command was `perf report --stdio --no-children --sort symbol
--percent-limit 0 --call-graph none --show-nr-samples --fields
overhead,sample,symbol --field-separator '|' --stdio-color never --symfs <map>
-i <record>`. Rows are sorted by reported overhead below. This deliberately
aggregates symbols rather than instruction addresses and disables inclusive
child accumulation. About 2,500 samples per record limit fine distinctions;
weighted periods need not equal simple sample-count percentages. Unresolved
address shares are 0.08–0.80%, included in the denominator.

Records use cycles:u at 499 Hz with DWARF callchains, CPU 15, JIT off and hash 0.
The record loop counts were calibrated independently by role; raw total samples
or event totals are not normalized performance comparisons. Use the stat packet's
per-loop ratios instead. Recording overhead means its wall time is not the
paired elapsed result.

## Counter ratios: candidate41 / upstream

From `gap41-perf-stat/ratios.json`, using three valid rounds for each case/role:

| Case | Instructions | Cycles | CPI |
| --- | ---: | ---: | ---: |
| logging_silent | 1.047121 | 0.949973 | 0.907224 |
| coroutines | 1.379018 | 1.342659 | 0.973634 |
| deltablue | 1.871481 | 1.689078 | 0.902536 |

Logging executes slightly more instructions with fewer cycles. Coroutines and
DeltaBlue have substantial instruction gaps and lower CPI than upstream; the
present evidence favors removing executed work over assuming a uniform dispatch
or cache-stall penalty. These counters do not identify which work is redundant.

## Top ten functions in each recording

### Logging silent: candidate

`record-r1-candidate-logging_silent-cycles-record-perf.data`; 2,535 samples.

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` | 14.41% | 364 |
| `_TAIL_CALL_CALL_PY_EXACT_ARGS` | 13.40% | 339 |
| `_PyFrame_ClearExceptCode` | 10.54% | 267 |
| `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` | 8.96% | 227 |
| `_TAIL_CALL_CALL_PY_GENERAL` | 8.64% | 218 |
| `_TAIL_CALL_RETURN_VALUE` | 7.83% | 198 |
| `_TAIL_CALL_POP_JUMP_IF_FALSE_BOOL` | 7.17% | 182 |
| `_PyDict_TryGetFirstProbeValue` | 5.53% | 140 |
| `_PyFrame_PushOverlapping.isra.0` | 5.53% | 140 |
| `_TAIL_CALL_BINARY_OP_SUBSCR_DICT` | 4.14% | 105 |

### Logging silent: upstream

`record-r1-upstream-logging_silent-cycles-record-perf.data`; 3,160 samples.

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_PyEval_EvalFrameDefault` | 48.55% | 1529 |
| `_PyFrame_ClearExceptCode` | 8.50% | 268 |
| `initialize_locals` | 7.70% | 244 |
| `_Py_dict_lookup` | 5.79% | 183 |
| `_PyEvalFramePushAndInit` | 4.40% | 139 |
| `PyDict_New` | 3.80% | 120 |
| `PyObject_GC_UnTrack` | 3.74% | 118 |
| `_PyEval_FrameClearAndPop` | 3.56% | 112 |
| `py_dealloc` | 3.08% | 97 |
| `_PyDict_Subscript` | 2.85% | 90 |

### Recursive coroutines: candidate

`record-r1-candidate-coroutines-cycles-record-perf.data`; 2,516 samples.

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_TAIL_CALL_CALL_PY_EXACT_ARGS` | 10.28% | 258 |
| `_TAIL_CALL_RETURN_GENERATOR` | 8.25% | 208 |
| `_TAIL_CALL_GET_AWAITABLE` | 6.09% | 153 |
| `_PyEval_CompleteGeneratorReturn` | 5.89% | 148 |
| `make_gen` | 5.88% | 148 |
| `_PyFrame_ClearLocals` | 5.58% | 140 |
| `_TAIL_CALL_SEND_GEN` | 5.52% | 139 |
| `_TAIL_CALL_COMPARE_OP_JUMP_INT` | 5.33% | 135 |
| `gen_dealloc` | 4.94% | 124 |
| `_TAIL_CALL_RETURN_VALUE` | 4.49% | 113 |

### Recursive coroutines: upstream

`record-r1-upstream-coroutines-cycles-record-perf.data`; 2,509 samples.

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_PyEval_EvalFrameDefault` | 52.53% | 1315 |
| `gen_dealloc` | 7.46% | 188 |
| `_PyObject_Free` | 6.86% | 172 |
| `make_gen` | 5.78% | 144 |
| `_PyObject_Malloc` | 4.22% | 106 |
| `_PyFrame_ClearExceptCode` | 3.51% | 88 |
| `_PyCompactLong_Subtract` | 2.28% | 57 |
| `PyObject_CallFinalizerFromDealloc` | 2.24% | 56 |
| `_PyObject_GC_NewVar` | 2.19% | 55 |
| `py_dealloc` | 1.76% | 44 |

### DeltaBlue: candidate

`record-r1-candidate-deltablue-cycles-record-perf.data`; 2,533 samples.

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_TAIL_CALL_CALL_PY_EXACT_ARGS` | 10.23% | 259 |
| `_PyTypeCache_Lookup` | 6.49% | 161 |
| `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` | 6.32% | 160 |
| `_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` | 6.21% | 157 |
| `_TAIL_CALL_RETURN_VALUE` | 4.77% | 121 |
| `_Py_type_getattro_impl` | 4.38% | 111 |
| `_PyFrame_ClearExceptCode` | 4.22% | 107 |
| `_TAIL_CALL_LOAD_ATTR_SHAPE` | 4.02% | 102 |
| `_TAIL_CALL_LOAD_ATTR` | 3.43% | 87 |
| `_PyType_LookupValueAndVersion` | 3.39% | 86 |

### DeltaBlue: upstream

`record-r1-upstream-deltablue-cycles-record-perf.data`; 2,458 samples.

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_PyEval_EvalFrameDefault` | 65.53% | 1606 |
| `_PyFrame_ClearExceptCode` | 3.74% | 92 |
| `_PyEval_FrameClearAndPop` | 2.72% | 67 |
| `listiter_next` | 2.56% | 63 |
| `_PyFrame_PushUnchecked` | 1.91% | 47 |
| `_PyTypeCache_Lookup` | 1.26% | 31 |
| `unicodekeys_lookup_unicode` | 1.10% | 27 |
| `_PyObject_GetMethodStackRef` | 1.06% | 26 |
| `_Py_VectorCallInstrumentation_StackRefSteal` | 1.02% | 25 |
| `_PyObject_Malloc` | 0.98% | 24 |

## What the profiles support

**Logging: inspect call overlap together with binding and retirement.** Exact
and general Python call handlers account for 13.40% and 8.64%; the out-of-line
`_PyFrame_PushOverlapping.isra.0` is 5.53% (140 samples), making those three
nonoverlapping self entries 27.57%. This is a location, not an incremental
27.57% cost caused by overlapping frames. Return/clearing and method/shape reads
are also prominent: RETURN_VALUE 7.83%, ClearExceptCode 10.54%, FrameClearAndPop
3.46%, method lookup 14.41% and shape/local-temp lookup 8.96%.

Read-only `perf annotate --stdio --no-source` of the retained overlap helper
locates samples across frame-size arithmetic, initialized-frontier checks,
header stores and the native prologue/epilogue. Within that helper, examples are
+0x4c frame-size arithmetic 9.23%, +0x7d frontier branch 8.70%, +0x118 zero/vector
initialization 10.71%, and +0x142 vector shuffle 12.77%. These percentages are
normalized within the selected symbol. The memset call at +0xa8 has no sampled
instruction; neither that fact nor zero samples in its nearby slow-path region
establishes zero initialization calls or identifies a memset bottleneck. Sampling
skid and compiler scheduling prevent treating a hot shuffle as its isolated
causal cost. The next check should count fast/initialization/extension paths and
inspect the complete caller+helper sequence; the profile does not justify
removing initialized storage or inlining the helper speculatively.

Upstream logging puts 48.55% inside its monolithic evaluator, with additional
initialize_locals 7.70%, EvalFramePushAndInit 4.40%, PushUnchecked 1.61% and
ClearExceptCode 8.50%. A tail-handler symbol cannot be compared directly with
that evaluator share. Candidate dictionary first-probe/subscript work is also
visible at 5.53% + 4.14%; this logging dictionary route is distinct from the
fixed-string micro that cannot enter the integer first-probe helper.

**Coroutines: creation/call/completion, not a demonstrated yield bottleneck.**
The actual kernel recursively awaits Fibonacci(25). It has no source yield or
external asynchronous suspension; child coroutines run to completion through
await. The recording has no YIELD_VALUE symbol samples. SEND_GEN is 5.52%,
CompleteGeneratorReturn 5.89%, RETURN_VALUE 4.49% and exact calls 10.28%.
RETURN_GENERATOR 8.25%, make_gen 5.88%, MakeCoro 1.11%, GC_NewVar 2.34% and
MiMalloc 3.61% sum to 21.19% self samples in selected construction/allocation
symbols. Clearing/deallocation is separately material: ClearLocals 5.58%,
ClearExceptCode 1.67%, gen_dealloc 4.94% and mi_free_ex 3.61%.

No separately named context/contextvars helper appears in this recording.
Exception/context link changes can be inlined into SEND or completion handlers;
absence of a symbol does not establish zero context cost. These samples support
counting coroutine creations, recursive entries and terminal deliveries before
prioritizing a heap-construction or completion experiment. They do not validate
the generator tree's yield-propagation hypothesis for this different workload.
Upstream allocation/deallocation is visible too, and 52.53% lies in its evaluator;
selected candidate self shares are not excess cost over those implementations.

This changes the next design's ranking for coroutines: the bounded
[direct-owning generator/coroutine entry contract](owning-generator-entry-proposal.md)
can precede SEND-recipient specialization here. It requires final-IR proof of
the creation prologue, shares the existing binder/storage and immediate-result
ABI, and first excludes cells/closures, genexpr iteration and observed/origin
tracking entries. COPY_FREE_VARS and MAKE_CELL currently happen at call time;
they must not silently move to first resume. The separate typed-cell bridge
port does not target this kernel's measured creation/call/completion work.

**DeltaBlue: both attributes and calls remain broad leads.** Four attr handlers
(shape/local-temp 6.32%, shape 4.02%, method 6.21%, generic 3.43%), type-cache
lookup 6.49%, type getattro 4.38%, type value lookup 3.39%, version lookup 3.04%
and value GetAttr 1.40% sum to 38.68% selected nonoverlapping self samples.
Exact Python calls are 10.23%, RETURN_VALUE 4.77%, ClearExceptCode 4.22% and
FrameClearAndPop 1.89%. Thus the remaining cost is not localized to one missing
class-scalar handler or one call primitive. Generic/type-object routes deserve
actual site/hit/proof census before extending cache coverage. Upstream puts
65.53% in its evaluator, so the 1.26% standalone upstream TypeCache_Lookup share
is not a directly comparable total attribute cost.

## What can be compared with frozen39

The older `E11/closing39-gap-perf-stat` cases are **deltablue, generators, nbody**;
current41 cases are **logging_silent, coroutines, deltablue**. Only DeltaBlue
matches. Do not compare the old generator profile with current coroutines or
invent older PMU baselines for current logging/coroutines.

Frozen39 source is `4d666e1b482cb4983c7683f91c5922af14d0ab40`, clean; its ELF
build ID is `ddb967d967336a7fa04766d44ca2b6272946a65a`. Both sessions use the
same upstream build ID and DeltaBlue input SHA256
`f418f09e89f25a67274a1c74b0fe0c182b5af84f405f32321fdf2a24ab86eabe`.
The independent sessions differ in date/calibration/loop counts; this is a
matched-case historical diagnostic, not a paired before/after experiment.

| DeltaBlue | Frozen39 / upstream | Current41 / upstream |
| --- | ---: | ---: |
| Instructions | 1.869213 | 1.871481 |
| Cycles | 1.654886 | 1.689078 |
| CPI | 0.885338 | 0.902536 |

Candidate median instructions per loop are 72,781,505.90 at39 and
72,822,039.97 at41, about +0.056%. Candidate median cycles per loop are
25,569,174.92 and 26,237,674.36, about +2.61%; upstream cycles also differ.
The evidence shows no broad instruction reduction in this case. It does not
assign the historical cycle change to the lookup/keyword packet, and it neither
negates its focused lookup gain nor quantifies that gain's workload coverage.

The practical next work is a site/shape and complete transfer census across
these distinct hot paths, then one bounded reviewed change. Current41 samples
remain evidence for41 after annotation42 is built; any42 measurement needs its
own source/ELF identities and must not be pooled with these recordings.
