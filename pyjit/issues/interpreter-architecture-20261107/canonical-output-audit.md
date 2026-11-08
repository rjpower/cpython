# Shape-result direct-publication audit

2026-11-07. Source audit against runtime `25ce1654801`; no runtime edits,
builds, tests or workloads by this owner. **Decision: do not introduce a
known-canonical-output declaration or direct-publish fast path yet.** The
transitive home proof is incomplete at an exported internal store boundary.

## What the candidate would save

`_LOAD_ATTR_SHAPE` reads `PyValueBorrow found` from an inline-value home and
sets `attr = PyValue_XDUP(found)` (`Python/bytecodes.c:2707–2717`). The
generated handler's early `IR_STORE_TEMP_DST` publishes `attr` before receiver
retirement; `IR_STORE_DST` later handles a folded-local destination. The
retained R2 GCC release executable at
`build/pyjit-link-wave1/interpreter-architecture-20261107/r2-gcc/candidate-python`
(build ID `adade7880217a84fa00988e51aff4bf04a6a61de`) still checks exact
`PyLong_Type`/`PyFloat_Type` at `0x352df2–0x352e08` and conditionally calls
`_PyValue_Canonicalize` at `0x352e24` during the first publication. A second
check/call at `0x352fa9–0x352fd8` serves the folded-local store. Thus the
source-level duplicate check remains in actual GCC code. Its branch frequency
and effect on elapsed time are unmeasured.

## Writer chain and gap

For an ordinary Python attribute store, the public object adapter uses
`_Py_VALUE_ARG`, which is `PyValue_FromObjectBorrow`; that canonicalizes
eligible exact int/float objects (`pycore_value_slots.h:345`,
`pycore_value.h:761–781`). The specialized `STORE_ATTR_SHAPE` and
`STORE_ATTR_SHAPE_ADD` take a value from a frame home and write it to the
inline slot (`Python/bytecodes.c:2947–2977`); this preserves the frame-home
invariant. Other inline-slot writers initialize NULL, duplicate a value in
`store_instance_attr_lock_held`, or move an existing value when detaching the
split dict (`Objects/dictobject.c:7062, 7231–7232, 7475–7476`). Dict
materialization/clearing sets the shape to `_Py_SHAPE_NONE`, and shape
specialization excludes that state (`dictobject.c:7401–7407`,
`specialize.c:905–911`). Under already-canonical inputs, these writers
preserve canonical bits; `PyValue_XDUP` does too.

The chain is not fully enforced at `_PyObject_StoreInstanceValue`, which is
a `PyAPI_FUNC` taking `PyValueBorrow` (`pycore_object.h:918–920`). Its only
entry assertion is `PyValue_IsLanguageValue`; it duplicates that value and
stores it in `values->values[ix]` with `_PyValue_CheckHome` afterward
(`dictobject.c:7258–7265, 7230–7233`). `PyValue_IsLanguageValue` admits a
raw pointer-form exact int/float, while `_PyValue_IsHome` rejects one whose
value can be inline (`pycore_pyvalue.h:99–112`, `pycore_value.h:560–582`).
`_PyValue_CheckHome` is a debug-only report; it is a no-op in release
(`pycore_value.h:592–630`). The header documents this internal store as
borrowed value/NULL deletion, without stating a canonical-input
precondition. Its current in-tree object and public adapters supply canonical
values, but the exported boundary does not itself establish the stronger
invariant for all supported native/JIT callers. A release build could
therefore retain a noncanonical pointer-form exact number in a shaped inline
home, which the proposed direct publication would copy without repair.

The smallest sound next step is to specify and enforce canonical input at
that store boundary, or canonicalize there, and then repeat the writer audit
including out-of-tree adapters. Only after that proof should the generator
accept a source declaration tying `attr` to a canonical home, emit a
temporary-only direct store while retaining `_PyValue_Publish` for a folded
local, and compare GCC plus matched workload counters. That boundary change
could add store-path work, so it needs its own review and measurement. The
new shared result-publication phase alone proves *when* a result reaches its
home; it does not prove the result was canonical before the publish check.
