# 08: Review at the 6a boundary

Status: decided 2026-10-27. Sections 4, 5 and 6 are adopted: 6b is the
`PyValue` C API, `BINARY_OP_POWER_FLOAT` goes in 6b, step 10 is split
with 10a before 7; note 09 answers the object-protocol question and
redefines 6c; note 06 section 6 carries the revised plan. Written to pause
the plan between steps 6a and 6b. The question is whether the work so far is a design or an
accumulation of patches, and what 6b should be.

## 1. Where the system stands

| Part | State | Note 06 |
|---|---|---|
| IR | one 64-bit word per instruction; compiler emits it; register CFG passes; dispatches -40% on richards and go against the stack machine | 9-13 |
| Frame | static, no stack pointer, callee overlaps the argument window, result into a local | 12-13 |
| Values | `PyValue` everywhere, canonical, bit-compare identity; boxes only as C locals around a `PyObject*` call | 18.1-18.2 |
| Header | one word: type pointer and 16 flag bits; weakrefs in a side table; no pre-header except the managed dict's | 18.4-18.6 |
| Memory | mark-sweep on mimalloc, no reference counting, hooks and finalizers queued before the sweep, referrer-first by flag, object-start bitmap | 15-17 |
| Ports | 5 of 8 pass; cffi, pyyaml, numpy fail at the `PyObject*` accessor boundary | 18.7 |
| Measurement | instructions per loop on five benchmarks; no wall-clock since Phase 1 | 18.9 |

## 2. What is designed

- The IR word, the register CFG and its passes, and the static frame are
  one design with one rule for where a value lives: in a slot.
- The value model has one type, one encoding, one canonical form and one
  identity rule, and every container holds it.
- The collector was built as a sequence of contracts, each written down
  before the code: hooks run before the sweep; a type whose dealloc reads
  its referents says so with a flag; borrowed caches live in a heap the
  scan skips; an eval-loop activation's C words are never roots, and the
  generator proves the invariant that makes that safe.
- Every step has a decision section, measurements with attribution, and a
  hand-off list. The ports tree is the compatibility metric and is run
  under the verifier with objects alive.

## 3. The hack register

Each row is an interim mechanism. "Retired by" names the step whose design
makes it unnecessary; "proof" is the test or measurement that shows it gone.
Nothing joins this table without those two columns.

| Mechanism | Why it exists | Retired by | Proof |
|---|---|---|---|
| `-ftrivial-auto-var-init=zero` for the whole build | stale C locals read as roots | 10 (precise roots) | flag removed; stale-root probe 100/100 |
| 16 KB stack clear before and after a collection; 16 KB clear in `gc.collect()` (6b: stores below the caller's stack pointer, as the 1 KB clear does, so that no clearing frame covers the top of the region) | padding in collector entry frames | 10 | clears removed; same probe |
| 1 KB clear at each C entry to the eval loop | padding and spills in the eval frame | 10 | removed; same probe |
| `_PyGC_CLEAR_ARRAY` after handler calls; scratch arrays zeroed | argument arrays read as roots | 10 | removed |
| popped `_PyValueRoot` slots cleared; saved registers cleared after use | stale words | 10 | removed |
| eval-loop activation ranges skipped by the scan; `Py_NO_INLINE` on about 25 helpers | partial stack map by exclusion | 10 replaces exclusion with a map | attribute dropped; stack map covers the eval loop |
| about 12 tests that skip on release builds or allow one survivor (test_weakref x3, test_finalization, test_enum, test_ctypes, test_descr, test_context, `test_release_task_refs`; 6b adds the multiprocessing `test_finalize`, a padding slot in `PyObject_VectorcallValue`'s frame, and on debug builds test_set `test_container_iterator` and test_memoryview `test_gc`, the never-written alignment slot at the bottom of the `gc.collect()` clinic parser's frame; 6c adds, on debug builds, test_gc `test_get_objects_arguments`'s count compare, where the first call's dead result list survives into the second call's snapshot) | callee-saved register spills in `PyObject_Vectorcall`'s frame | 10 | skips removed, tests pass 20/20 on both builds |
| test_asyncgen `test_async_gen_asyncio_gc_aclose_09` and `test_async_gen_asyncio_shutdown_exception_02` skip on release builds | a one-past-the-end argument pointer held in a callee-saved register aliases the next block: the conservative scan accepts interior and block-start pointers. The probe found `_PyObject_MakeTpCall`'s saved r13 holding `context_run`'s `args + 1`, the end of a live one-item argument tuple's items, which is the start of the next 32-byte block, a dead object | 10a for core C frames (precise roots: a core frame's registers are not scanned at all); 10b's handle discipline for extensions. The alternative, allocator slack so that one-past-the-end stays inside its block, is a decision for 10a with its memory cost measured | the two asyncgen tests pass on release builds without the skip; the probe finds no block-start alias |
| `Py_DECREF`, `Py_XDECREF`, `Py_CLEAR`, `Py_SETREF` and `Py_XSETREF` are a keep-alive barrier: an empty `asm` that uses the released pointer, or an out-of-line `_Py_KeepAlive()` call on compilers without GNU inline assembly (6b). The compatibility contract: an object stays alive until its last release call, so reference-correct code is collector-correct | reference-correct C reads memory an object owns and releases the owner afterwards; with a no-op release the compiler drops the owner's pointer early, and a collection frees the owner and the memory with it. Latent since 6a. Found in 6b as a release-build crash in `PyUnicode_Join` (`''.join(map(chr, ...))` in test_str's `test_codecs`, after `test_adaptive_find` moved when collections run): `fseq` was dead once `PySequence_Fast_ITEMS()` was taken, the scan kept the item block but not the list, and sweeping the list freed the block | 10b's handle discipline (10a does not touch extension frames) | the `PyUnicode_Join` repro (`test_adaptive_find`'s allocations, then `''.join(map(chr, ...))` over all code points) passes with the barrier compiled out |
| C-local PyMem arrays whose contents are reachable only through the transitive scan: `list.sort()`'s keys array, its `saved_ob_item` while `ob_item` is NULL, and its merge temp array; `_PyObjectArray_FromValues()`'s block of boxes for a long argument list (ceval.c); `PyEval_EvalCodeEx()`'s argument copy; `partial_vectorcall()`'s stack (`_functoolsmodule.c`); `mro_implementation_unlocked()`'s `to_merge`. Found in 6b's audit after `map()` with more than five iterables, which now keeps its items in a tuple held by a `_PyValueRoot` | the live collector scans every PyMem block that a C stack or register points into, and the blocks those point into, so an array a C local points to holds its items | 10a (explicit roots for core frames) | a scan that no longer follows PyMem blocks passes the suite |
| inline-fallback collection scans everything | allocation past a second threshold without reaching the eval loop | 10 | one scan path |
| trigger counts requested bytes, not block bytes | 5a trigger written before the header shrink | 7 | RSS on the 10M-tuple loop back to the 5c figure |
| allocator does not collect when an allocation fails | 5a design gap | 7 | a test that fails an allocation and succeeds after a collection |
| managed-dict pre-header is 16 bytes for alignment | one-word pre-header leaves objects 8-aligned | 6c (instance is header, shape, slots) | no pre-header: a plain instance is the header word and its type's capacity of slots, 56 bytes (a 64-byte bin) at the minimum capacity of six (note 09 section 6) |
| flags in the header word (bits 48-63, beside the shape since 6c); a mask on every shape and type read (347 `movabs` masks in the eval loop at 6a) | first layout; 6c keeps note 09's layout by the 2026-10-27 ruling. Side bitmaps beside the object-start map are the alternative: step 7 decides where the mark and sticky-mark bits live, and the other per-object bits (weak references, finalizer queued) with them, so that the header becomes a bare shape pointer in one move | 7 | shape reads without a mask |
| `LOAD_NAME` and `STORE_NAME` box through the `PyObject*` dict API | module-level code not converted | retired in 6b (7f2099c61f5) | module-level int and float loops: 6.0M boxes per 2M iterations at 6a, none now (note 06 section 19.10) |
| `PyList_GET_ITEM` and family box, abort on allocation failure | compatibility accessors used by C code | 6b made them the compatibility layer and moved the hot callers to values; 194 uses in Objects/, 205 in Python/ and 273 in Modules/ remain (`PyTuple_GET_ITEM`, `PyList_GET_ITEM`, `PySequence_Fast_GET_ITEM`, `PyDict_Next`), recorded in note 06 section 19.11 | no core caller; ports count the uses |
| `BINARY_OP_POWER_FLOAT` with a positive-base guard | nbody boxed `** -1.5` | retired in 6b (602881feefd): `PyNumber_PowerValue` makes the generic `**` box-free | nbody 0 boxes per loop without the handler |
| str `+=` quadratic | in-place growth needed a unique reference | a design decision in 6c (rope or builder); 6b did not take it | append cost per character linear |
| object slots box: custom `tp_getattro` and `tp_setattro`, `tp_descr_get` and `tp_descr_set`, and native descriptor calls keep `PyObject*` signatures, so the plan executor boxes the receiver and the result at them | 6c changes the generic path only (note 09 section 9, ruling of 2026-10-27) | 6d | a property, a native descriptor and a custom `__getattribute__` returning an int or float allocate no box |
| no ABI flag; the platform part of `SOABI` marks the ABI instead (6c): `SOABI` is `cpython-316-x86_64-boil-linux-gnu`, so `EXT_SUFFIX` and `EXTENSION_SUFFIXES` differ from upstream's while the wheel tag (`cp316`), `PLATFORM_TRIPLET`, `MULTIARCH` and `sysconfig.get_platform()` do not; the ports SDK's abi_id (`cpython-3.16-value-v1`) marks the value ABI for the ports tree | a `cp316v` tag is rejected by packaging's `_cpython_abis()`, which pip and setuptools vendor (note 06 section 19.4). An extension built by upstream 3.16 has the suffix `.cpython-316-x86_64-linux-gnu.so`; 6c's suffix makes it a `ModuleNotFoundError` instead of a module that reads values as `PyObject *` (test_sysconfig `test_upstream_extension_not_loaded`). What remains: pip still installs an upstream `cp316` wheel, which then fails at import | the ports phase, with a packaging, pip and setuptools port that gives the wheel tag an ABI flag | a wheel tagged `cp316v` builds and installs; pip refuses an upstream `cp316` wheel |
| Cython's type vectorcall (`CYTHON_VECTORCALL_TPNEW`) is off in the cython port | its `tp_vectorcall` code keeps object signatures | the ports phase, after 10b | the port with it on passes |
| `_uuid` keeps a weaklist field; `tp_weaklist` unused; typeobj.rst says MANAGED_WEAKREF needs GC; orphan clinic file | remnants | next cleanup pass | grep clean |

The first seven rows are one thing: conservative roots. They are small
individually and were each measured, and together they are the reason step
10 exists. Section 6 asks whether step 10's root half should move earlier.

## 4. What 6b should be

Note 06 section 6 defines 6b as "PyValue at the C API boundary and in the
ports: item accessors become boxing functions; Cython port patched and the
Cython-generated ports regenerated; numpy". Done as written, it would patch
cffi, Cython and numpy against the boxing accessors and leave every
`PyObject*` protocol call boxing its inline arguments. That entrenches the
boxes that 6a was meant to remove (float's 300k per loop, go's 88k).

6b redefined: **the PyValue C API.**

1. The core's protocols take and return `PyValue`: number
   (`PyNumber_AddValue` and the binary table, `PyNumber_PowerValue`),
   object (`PyObject_RichCompareValue`, `PyObject_GetAttrValue`,
   `PyObject_IsTrueValue`, `PyObject_HashValue`), sequence and mapping
   (`PySequence_GetValue`, `PyMapping_GetValue`), call
   (`PyObject_VectorcallValue` with a `PyValue` argument array), iteration
   (`PyIter_NextValue`, already present). The `tp_*` slots that the
   interpreter calls on hot paths get `PyValue` signatures; the `PyObject*`
   slot stays as the compatibility entry and boxes. The generic
   `BINARY_OP`, `COMPARE_OP`, `CALL` and attribute handlers call the value
   forms, so an inline operand is never boxed to reach a C implementation
   of the same operation.
2. The `PyObject*` API is the compatibility layer: it boxes on the way out,
   canonicalises on the way in, and the boxing accessors keep the abort
   rule of 18.3. Nothing in the core calls it.
3. The ports: the Cython port is patched once, at the generated-code
   idioms (`&PyTuple_GET_ITEM`, `ob_item` as `PyObject**`,
   `m_weakreflist`, `ob_type`), and pyyaml and numpy are regenerated;
   cffi is patched by hand. Patch lines per port are the metric, as
   before.
4. Measurement: boxes per loop on float and go go to the C-extension
   residue only; per loop recorded.

## 5. A rule for specialised handlers

A specialised handler exists for a feedback kind the JIT will guard on:
int and float arithmetic and comparison, attribute load and store by
shape or type version, call by callable kind, iteration by iterator kind,
subscript by container kind. It does not exist because a benchmark boxes.
Under this rule `BINARY_OP_POWER_FLOAT` is a legitimate member of the
float arithmetic family only if its guard is the family's guard (two inline
floats) and its result rule is the family's (box a non-finite or complex
result through the generic path). The positive-base guard is a benchmark
artefact. 6b either generalises the handler to the family's shape or
removes it once the value-typed number protocol makes the generic path
box-free, and records which.

## 6. Order of the remaining steps

Current order: 6b, 6c (shapes), 7 (generational), 8 (tier 0), 9 (tier 1),
10 (precise moving collector).

Proposed: split 10 into **10a, precise roots** (stack maps for the eval
loop and for every C entry point that holds values; the conservative scan
remains only for extension C code until the handle API) and **10b, moving**
(nursery, compaction, handle discipline at the C API). Move 10a to run
after 6c and before 7. Reasons:

- The hack register's first seven rows retire together, two steps earlier.
- A JIT designed with stack maps from day one (8, 9) is simpler than one
  retrofitted, and tier 0's frame layout is decided once.
- The generational collector (7) needs a write barrier and a remembered
  set; a precise root set makes its nursery design honest about what a
  root is.

Cost: 10a is roughly a week of work pulled forward, and the ports' C code
stays conservatively scanned until 10b, which is the same as today.

## 7. Measurement at the waypoint

One pyperformance run of the 24-benchmark subset against the Phase 1
baseline (`build/baseline-default`, `boil/bench/default.json`), on an idle
machine, recorded with instruction counts. Not a gate and not a target: the
plan has had no wall-clock number for two days of work, and the review
should know whether the instruction counts track time.

### 7.1 Result (2026-10-27, `boil/bench/step6a.json`, `compare-default-vs-step6a.txt`)

Geometric mean 1.46x slower than the Phase 1 baseline over 22 benchmarks
(1.47x against Phase 1's final build). The compute-bound reference set is
close to flat: nbody 1.47x faster, richards 1.03x slower, go 1.16x slower,
float 1.23x slower (its math.* argument boxes are rung 2 of 6b). The
regression is in allocation-heavy benchmarks: coroutines 2.75x,
generators 2.55x, unpack_sequence 2.07x, comprehensions 1.98x, pickle
1.81x, deepcopy 1.70x, unpickle 1.69x, nqueens 1.66x.

Attribution. On generators the whole process executes 3.26G instructions
against 1.00G on the baseline, so this is instruction count, not cache or
branch behaviour. `perf record` on generators and coroutines puts 25 to 30
percent of samples in the collector (`live_visit`, `collect`,
`live_walk_block`, `_mi_heap_area_visit_blocks`, `_PyGC_FreeDeadBlock`,
`_PyWeakref_ListPtr` at 1.5 to 3.5 percent on its own) and the rest in the
eval loop and allocation. A non-generational mark-sweep marks the whole
live heap at every collection, and the collection rate is proportional to
allocation, so an allocation-heavy loop pays the live heap's size per
allocated byte; the 5a trigger counting requested rather than block bytes
makes the rate higher still. This is the cost that step 7 (generational
nursery and sticky marks, over 10a's precise roots) is meant to remove, and
the rows "trigger counts requested bytes" and "allocator does not collect
on failure" of section 3 belong to the same account. The eval-loop share
carries the 1 KB clear at each C entry and the 16 KB clears around a
collection (section 3, rows 2 and 3; retired by 10a), which generator and
coroutine resumption pay at every switch.

Two lessons, neither of them a tuning. First, the per-loop instruction
set (richards, go, nbody, float) does not see this: the reference set must
include an allocation-heavy benchmark, so generators and comprehensions
join the recorded set from 6b on. Second, the first run died because a
prebuilt abi3 wheel loaded against the new header; 6b removes the stable
ABI so that such a wheel fails to import.

Not acted on: per the plan revision, the number is recorded and
attributed; no tuning is done before steps 7 and 10a, which own it.
