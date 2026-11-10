# Current46 retained profile analysis

Candidate46’s recursive coroutine profile locates work in exact-call frame
initialization, coroutine creation/copy, await/send entry, and completion/clear.
DeltaBlue retains method/shape handler work alongside generic type lookup.
Logging retains call setup/retirement and method/shape access work. These are
locations to investigate, not causal fractions of the performance gap.

## Identity and decoding

Evidence `E` is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.
`E/gap46-perf-record/status.json` is complete: 6/6 valid invocations, stable
identity, no session or postflight error. Before/after identity manifests are
identical. Retained executable SHA256 and readelf build IDs match the manifests
and recorded build-ID lists.

| Role | Clean source HEAD | Retained ELF build ID |
| --- | --- | --- |
| Candidate46 | `b8e7e1bee6611eeda320ce163cd3ce89323da877` | `64d2f0d36ba26e3d1c79b6a165d67c30d5a65e17` |
| Upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2` | `ff51c387c04ccd5a2664e5a572bc68e491ec0db1` |

Candidate executable SHA256 is `48acdcb4cbd695edbd2b4f3eaaf0939a9309c19d8a60d1c58daa0d07a4cb2624`;
upstream is `8ed926d80536c6f768bf0bcdcb225c631bf4ff29a23cecdc88c2120345ca5867`.
Original recorded interpreter paths were mapped to the packet’s retained
`candidate-python`/`upstream-python`, never the mutable build executable.
The other recorded DSOs (select extension and libc) were mapped only after
readelf matched their recorded build IDs. All six build-ID lists agree by role.

Decoding ran pinned to CPU 14, outside benchmark CPU 15 and its sibling 7.
Only offline perf report/script ran; no interpreter workload, build or runtime
source edit was performed. Frozen images were read only. Reports are retained
in `E/gap46-profile-decoded`; the temporary symfs and callchain text were removed.

Command: `perf report --stdio --no-children --sort symbol --percent-limit 0
--call-graph none --show-nr-samples --fields overhead,sample,symbol
--field-separator '|' --stdio-color never --symfs <verified-map> -i <record>`.
Candidate callchains were inspected with `perf script -F period,ip,sym,dso
--max-stack 24 --symfs <verified-map> -i <record>`. Reports emitted no decoder
warnings. All reports say zero lost samples, and all six retained lost-record
files are empty.

Shares below are weighted **self cycle-period shares within each recording**,
not call counts, inclusive costs, elapsed ratios, or speedup budgets. About
2,500 samples per recording limit fine distinctions. Unknown shares sum the
rounded unresolved-symbol rows; their samples remain in the denominator.
DWARF ancestry and inlined symbols are used only to identify sampled routes;
ancestor counts overlap and must not be added to self shares.

## Sample quality and loop counts

Records sampled cycles:u at 499 Hz with DWARF callchains on CPU 15, JIT disabled,
hash randomization disabled. Warmup/setup precede the enabled hot interval;
small control-boundary samples still occur. Per-role loop counts were calibrated
separately to roughly five seconds, so raw event/sample totals are not a speed
comparison. Current46 paired counters are a separate coordinator result; this
report does not substitute recording wall time for them.

| Role/case | Samples | Unknown self share | Hot loops |
| --- | ---: | ---: | ---: |
| candidate/logging_silent | 2,457 | 0.12% | 5,023,864 |
| candidate/coroutines | 2,487 | 0.12% | 145 |
| candidate/deltablue | 2,500 | 0.76% | 832 |
| upstream/logging_silent | 2,514 | 0.04% | 4,448,490 |
| upstream/coroutines | 2,528 | 0.12% | 195 |
| upstream/deltablue | 2,507 | 0.76% | 1,358 |

## Top ten self symbols

### candidate: logging_silent

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` | 14.59% | 358 |
| `_TAIL_CALL_CALL_PY_GENERAL` | 12.37% | 304 |
| `_PyFrame_ClearExceptCode` | 11.03% | 270 |
| `_TAIL_CALL_CALL_PY_EXACT_ARGS` | 11.00% | 266 |
| `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` | 9.99% | 246 |
| `_TAIL_CALL_RETURN_VALUE` | 8.96% | 220 |
| `_TAIL_CALL_BINARY_OP_SUBSCR_DICT` | 6.09% | 150 |
| `_TAIL_CALL_POP_JUMP_IF_FALSE_BOOL` | 5.42% | 133 |
| `_PyDict_TryGetFirstProbeValue` | 5.16% | 126 |
| `_TAIL_CALL_start_frame` | 4.16% | 102 |

### candidate: coroutines

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_TAIL_CALL_CALL_PY_EXACT_ARGS` | 11.19% | 279 |
| `_TAIL_CALL_RETURN_GENERATOR` | 8.24% | 205 |
| `_TAIL_CALL_GET_AWAITABLE` | 7.51% | 187 |
| `_PyEval_CompleteGeneratorReturn` | 6.51% | 162 |
| `_TAIL_CALL_COMPARE_OP_JUMP_INT` | 5.75% | 143 |
| `gen_dealloc` | 5.67% | 141 |
| `_PyFrame_ClearLocals` | 5.24% | 130 |
| `_TAIL_CALL_SEND_GEN` | 5.15% | 128 |
| `mi_free_ex` | 3.89% | 95 |
| `_TAIL_CALL_LOAD_GLOBAL_MODULE` | 3.83% | 95 |

### candidate: deltablue

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_TAIL_CALL_CALL_PY_EXACT_ARGS` | 11.05% | 276 |
| `_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` | 7.62% | 189 |
| `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` | 6.81% | 170 |
| `_PyTypeCache_Lookup` | 5.60% | 140 |
| `_TAIL_CALL_LOAD_ATTR_SHAPE` | 4.20% | 105 |
| `_TAIL_CALL_LOAD_ATTR` | 3.68% | 93 |
| `_Py_type_getattro_impl` | 3.64% | 91 |
| `_PyFrame_ClearExceptCode` | 3.32% | 83 |
| `_TAIL_CALL_RETURN_VALUE` | 3.00% | 75 |
| `_TAIL_CALL_STORE_ATTR_SHAPE` | 2.92% | 73 |

### upstream: logging_silent

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_PyEval_EvalFrameDefault` | 50.77% | 1271 |
| `_PyFrame_ClearExceptCode` | 12.53% | 315 |
| `_PyEvalFramePushAndInit` | 5.61% | 142 |
| `_Py_dict_lookup` | 5.41% | 136 |
| `initialize_locals` | 4.32% | 109 |
| `PyDict_New` | 4.13% | 104 |
| `_PyEval_FrameClearAndPop` | 3.47% | 87 |
| `py_dealloc` | 2.34% | 59 |
| `dict_dealloc` | 2.33% | 58 |
| `PyObject_GC_UnTrack` | 2.30% | 58 |

### upstream: coroutines

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_PyEval_EvalFrameDefault` | 53.33% | 1348 |
| `gen_dealloc` | 7.26% | 183 |
| `_PyObject_Free` | 6.28% | 160 |
| `_PyObject_Malloc` | 5.35% | 131 |
| `make_gen` | 4.55% | 115 |
| `_PyFrame_ClearExceptCode` | 3.16% | 80 |
| `_PyObject_GC_NewVar` | 2.58% | 65 |
| `_PyCompactLong_Subtract` | 2.57% | 65 |
| `PyObject_CallFinalizerFromDealloc` | 2.10% | 53 |
| `_PyEval_FrameClearAndPop` | 2.09% | 53 |

### upstream: deltablue

| Function | Self share | Samples |
| --- | ---: | ---: |
| `_PyEval_EvalFrameDefault` | 66.46% | 1662 |
| `_PyFrame_ClearExceptCode` | 3.79% | 95 |
| `_PyEval_FrameClearAndPop` | 2.55% | 64 |
| `listiter_next` | 2.20% | 55 |
| `_PyFrame_PushUnchecked` | 1.79% | 45 |
| `_PyTypeCache_Lookup` | 1.32% | 34 |
| `gc_collect_main` | 0.95% | 24 |
| `_PyObject_Malloc` | 0.84% | 21 |
| `_PyObject_GetMethodStackRef` | 0.84% | 21 |
| `_Py_VectorCallInstrumentation_StackRefSteal` | 0.80% | 20 |

## Route interpretation and next evidence

**Logging.** The top rows span method lookup, exact/general calls, shape loads,
return and frame clear. In the candidate’s 2,457 callchains, 169 contain
`_PyFrame_PushOverlapping`. Their sampled leaves include `_PyFrame_Initialize`
(73), `_PyFrame_InitIRInstrPtr` (36), the helper itself (21), and
`_PyStackChunk_EnsureFrameStorage` (16). Thus the helper’s absence as a standalone
self symbol does not show that overlapping-frame setup vanished: much of it is
inlined into the exact-call handler. These are overlapping ancestry observations,
not extra self percentages or a claim that every logging call uses this path.
Upstream’s evaluator contains 50.77% of its self samples, so direct handler
percentage comparisons cannot isolate equivalent operations across runtimes.

**Recursive coroutines.** The actual kernel recursively awaits `fibonacci(25)`;
it has no source yield or external asynchronous suspension. One top-level send
drives the recursive completion chain. The candidate callchains contain
`_PyFrame_PushUnchecked` (144), `_PyFrame_Initialize` (128),
`_PyFrame_FillConstSlots` (77), `_Py_MakeCoro` (357), `make_gen` (300),
`_PyFrame_Copy` (79), `complete_step_impl` (382), `clear_gen_frame` (198),
and `gen_dealloc` (171). No YIELD-named handler occurs in the candidate self
report or decoded callchains. These names include compiler-generated inline
DWARF ancestry: they identify attributed machine-code locations, not physical
helper invocations or completed initialization work. In particular,
`_PyFrame_FillConstSlots` attribution can cover its zero-slot check;
`co_nconstslots` may be zero. This profile establishes neither that slots were
filled nor how many; a per-code slot/entry census is required for that claim.
The source corroborates the logical route:
`CALL_PY_EXACT_ARGS` checks and enters a thread frame; calls from a generator-owned
caller take `_PyFrame_PushUnchecked` and copy arguments rather than overlap.
`RETURN_GENERATOR` allocates via `_Py_MakeCoro`, copies the initialized frame,
sets the initial-send destination, then pops the temporary frame. `SEND_GEN`
enters the coroutine and `_PyEval_CompleteGeneratorReturn` decodes the immutable
SEND/FOR destination before the shared completion logic. These sampled locations
support investigating direct owning-frame creation ahead of optimizing a yield
that this kernel does not exercise. It does not validate such an implementation:
the entry proposal’s pinned-code, GC, observation, recursion and once-only
ownership obligations still apply. Preserve the single completion ABI.

**DeltaBlue.** Method lookup and shape reads/stores remain visible self work.
The 214 sampled candidate callchains containing `_PyTypeCache_Lookup` have
`LOAD_ATTR` as their nearest handler in 211 cases and `STORE_ATTR` in three.
This connects sampled type lookup to the generic attribute route; it does not
prove why each site stayed generic, which cache guard failed, or its dynamic
miss count. The benchmark’s class-level strength values and varied constraint
methods make both class lookup and method dispatch plausible targets, but
source plausibility alone cannot assign the samples to particular Python sites.
Pair this profile with the bounded feedback-capacity/handler census before
choosing an experiment. A site-level guard/miss census would distinguish
admission limits, receiver diversity and unsupported descriptor/class cases.

Upstream’s monolithic evaluator absorbs 53.33% of coroutine and 66.46% of
DeltaBlue self samples. The allocator names also differ between builds; this
report does not infer allocator equivalence from unlike symbol names. Current41
used a different candidate ELF; its sample percentages are contextual history,
not a paired before/after result for46. No frozen39 causal comparison is made
here. Preferred next design evidence remains a bounded owning-frame entry
census for coroutines and a mapped ATTR site/guard census for DeltaBlue; the
current samples alone establish neither a speedup estimate nor implementation
approval.
