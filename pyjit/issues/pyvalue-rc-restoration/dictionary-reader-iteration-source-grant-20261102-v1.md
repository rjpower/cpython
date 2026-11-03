# Dictionary semantic readers, views and iteration

Adopt coordinator critical review of recommendation87f1cc96 at
build/pyvalue-dict-parser-next-cut-design-20261102-v1/recommendation.md,
with the two required corrections documented in root-design-review.md there.
C implements; D independently reviews exact source and fixtures.

Distinct90-minute source cut: conservative start2026-11-02T02:43:40Z,
concrete checkpoint03:03:40Z, ceiling04:13:40Z. Record actual first edit.
Checkpoint must include one actual owning reader and forward/reverse canonical
iterator capture/allocation transaction, not declarations alone.

Only recommendation's finite dictobject.c function ledger: owning GetItemRef/
SubscriptKnownHash/get, Contains/metadata index, keys/values/items snapshots,
FromKeys, equality/repr/frozen hash, forward/reverse iterator implementations,
item contains/XOR, and precise Unicode keyword metadata hunks. Add one explicit
owning _PyDictIter_NextValue declaration in pycore_dict.h and focused existing
dict C/Python fixtures. Freeze its signature/phase contract for later A adoption.
Do not change accepted mutation/managed providers without an exact handoff.

Own selections before user lookup/comparison/repr or allocation. Snapshots
allocate detached output then reacquire current count/table before pure fill.
FromKeys retains canonical default, cached hashes and consuming Take2 on every
exit. Preserve frozen/subclass semantics. Reused pairs publish both fields,
reset cached hash and tracking BEFORE releasing either old owner. Reserve
iterator cursor before boxing/allocation; rollback only untouched reservation
state, never callback progress. Pin owners; no raw table/entry pointer crosses
callbacks. Object tp_iternext remains an explicit fallible outward bridge.

Owned reads distinguish entry from NEW error: restore saved entry on new
success/miss; retain genuine new hash/equality/boxing failure and drop saved
entry under that new primary. Legal fixtures preserve deallocator entry errors.

Leave named old borrowed getter/Next/SetDefault definitions/signatures and
consumers untouched. No script/equivalent API removal, transient scalar boxes,
permanent box cache, runtime representation switch or new native registration.
Record residual whole-TU diagnostics rather than suppressing them. New iterator
source alone is partial until actual value-native consumers adopt it.

Cheap ownership/phase/status review and focused fixture AST, then actual Dict
and changed fixture TUs with real headers, each at most30seconds. No objects,
link, candidate, runtime or timing in this grant. A later parser/registration
cut remains separately required.

Window closed early. Actual first edit02:48:11 UTC; concrete ce4bd36d at
02:58:08 meets03:03:40. Final187744d6 at03:16:49 passes independent review
e9a446cc:34 changed/new fragments/four contexts,29 AST-only methods with prior
22 unchanged. Independent masked-C census verifies other Dict bodies exact and
all names retained. Fixture syntax passes; whole Dict fails only six retained
borrowed bodies. Repr cleanup now preserves the primary through Writer_Discard.
Actual value-iterator VM adoption and the explicit legacy boundary remain
required. No candidate/runtime/timing acceptance.
