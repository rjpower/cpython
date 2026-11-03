# Value-native compiler constants and checked list replacement

The coordinator adopts D's critical source/lifetime/identity review of
build/pyvalue-rc-restoration-design-20261101-v1/compiler-constant-consumer-cut-v1.md.
Root implements; D independently reviews actual source and witnesses.

Distinct45-minute cut from conservative2026-11-02 00:38:35 UTC,
checkpoint00:53:35, ceiling01:23:35. Record actual first edit. Accepted common
constants remain unchanged; no prior source budget is extended.

Exact functions in flowgraph.c: add_const identity get/set; new normalized
identity-word helper; _PyCfg_OptimizeCodeUnit initial index; pure value-aware
const_folding_check_complexity and its one safe_multiply caller;
remove_unused_consts canonical list compaction. No other pass/algorithm changes.
Use canonical Borrow(value).bits as private opaque _Py_hashtable keys on the
selected64-bit profile, never PyObject pointers or temporary box addresses.
Hash/direct compare do not dereference them. Object keys remain rooted by the
private consts list until table destruction before unused compaction. Verify
append-only use and immediate-failure tail rollback. Preserve bool/int, int/float,
signedzero and heap-NaN identity distinctions. No scalar boxing cache.

Add one checked consuming _PyList_SetItemTakeValue helper in listobject.c and
pycore_list.h. Inputs are strong HeapSafe language values or the documented NULL
construction sentinel. Index failure consumes the incoming owner. Success
publishes new value before releasing old; do not reuse a cached list array after
release. The existing object PyList_SetItem adapter prepares ownership purely,
delegates and retires its primitive box only after valid state publication.
Compiler compaction DUPs its borrowed source before calling the helper; traversal
of immutable constant tuples borrows directly without allocation/boxing.

New test_pyvalue_compiler_constants.py; at most one consuming-setter C fixture
and method entry in _testinternalcapi.c (separate from A's dictionary inspection
function). Tests must actually witness index/compaction, scalar identity edges
and consumed owners including invalid indices/aliased replacement. Use existing
CFG test helpers. Native destructors preserve pending exceptions; failures are
unraisable. No invented failure mechanism or skipped contract check.

Cheap gates: exact map lifetime/source review, AST, at most three actual changed
syntax-only TUs30seconds each (flowgraph/list/test adapter). No bytecode/generator,
kernel, public object layout/API removal, object/link/candidate/timing work.
Broader compiler/Clinic/dictionary/native consumers remain required closure.

Actual first header write2026-11-02 00:40:48.373079761 UTC. Timely checkpoint
09f11fff sealed before00:53:35; source review pending. The identity helper now
uses a full-word mixing hash: review found pointer hashing would cluster inline
integral floats. Host exact-operation model:2048 keys/2048 buckets,1279 occupied,
maximum chain6; this is not native timing. Initial own API/include syntax errors
are corrected and retained. Actual flowgraph/fixture syntax passes; list syntax
retains unrelated Clinic and two preexisting sort truthiness errors. Seven
runtime methods are drafted; review corrected compaction tests to use constants
that survive LOAD_SMALL_INT/LOAD_COMMON_CONSTANT selection. No runtime run.

Independent final reviewfc3237a4 accepts corrected v2 receipt71666fa3.
The source window closes early; all C/header sources match v1, only test
expectations/direct-CFG witnesses changed. No whole-list/compiler acceptance.
