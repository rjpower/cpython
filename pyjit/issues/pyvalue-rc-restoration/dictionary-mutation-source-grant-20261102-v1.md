# Coherent dictionary mutations and immediate growth

Adopt the independently reviewed mutation design at
build/pyvalue-register-interpreter-checkpoint-20261101-v1/dict-provider-source-v1/next-mutation-cut.md,
SHA256 c245db5a305dce9d3a90cc22767f13ba1060cf2f42268c6265189297eba09da4,
subject to the deallocator-contract correction below. D found no new ownership
blocker in the proposed transaction. C implements; D reviews actual source.

The accepted additive owning-provider window closes early at review95427515.
This distinct90-minute mutation+growth source cut uses conservative2026-11-02
00:04:24 UTC start,00:24:24 checkpoint,01:34:24 ceiling. Record actual first edit.
Complete the existing native fixture correction before starting production edits.
No original dictionary, provider or native fixture budget is extended.

Exact mutation closure: setitem_take2_lock_held_known_hash,
setitem_take2_lock_held, insertdict, insert_combined_dict, insert_to_emptydict,
insert_split_key, _PyDict_InsertSplitValue, replace_value,
delete_index_from_values, delitem_common; declared value setter/deleter/Take2
wrappers; retained object setter/deleter adapters; directly affected
delitemif_lock_held, _PyDict_ReplaceItemIf, _PyDict_Pop_KnownHash callers.
Immediate growth includes dictresize, insertion_resize, both build_indices
helpers and invalidate_and_clear_inline_values. Preserve existing algorithms.

Pin canonical inputs before callbacks; Take2 consumes on every exit. Notify once
before mutation using the original event, retaining the documented watcher
restrictions. Reacquire/re-probe after notification, unraisable reports,
allocation and old-storage releases. Allocate detached empty capacity before
capturing live source pointers; revalidate count/kind/fit, then perform pure
copy/transfer and publish all keys/values/count/order/inline validity before
old storage can call out. No stale entry or value-array pointer survives a
callback. Keep no-op identity rules, frozendict checks and owned pop results.
The retained object-returning _PyDict_Pop_KnownHash must box its pinned result
before removal, then re-probe/revalidate selected key/value after boxing can
reenter. OOM removes nothing. An owned PyValue snapshot alone is insufficient
for this fallible object adapter; follow the reviewed set-pop transaction.

Complete typed notification declarations/implementation in pycore_dict.h and
dictobject.c; only builtins_dict_watcher in pylifecycle.c, module_dict_watcher
in moduleobject.c and the three dictionary callbacks in _testcapi/watchers.c.
Pin callback payloads; DEALLOCATED retains resurrection rules and no INCREF0.
A owns exact BUILD_MAP/STORE_SUBSCR/hinted-store bytecode handoffs and generators;
agree those interfaces before changing shared signatures. Watched hinted stores
use the checked provider. Root's set consumers use the accepted owning reads.

Preserve tp_dealloc's unchanged-exception contract. Do not introduce pending
errors from DECREF or use invalid destructor-error injections as evidence.
Checked operation failures and legal unraisable/reentry remain testable. Retain
primary exceptions across cleanup, and check allocation/operation errors; this
does not grant new deallocator semantics or an API removal.

Copy, managed materialization, instance dictionary storage, views, old getters,
complete deallocation and wider native consumers remain required later cuts.
The rejected API-removal script/equivalent remains outside scope. Cheap gates:
exact transaction/consumer source review, lexer/AST, focused tests drafted and
at most three actual changed syntax-only TUs30seconds each when meaningful.
No object output, link, candidate execution or timing in this source cut.

Additive handoffs authorized during this window: C owns
_PyDict_FromValueItems and its internal declaration, borrowing stable owning
strided canonical arrays and returning a new dictionary. A owns the actual
BUILD_MAP private-window proof and caller. Legacy FromItems remains unchanged.
One new _testinternalcapi/dict.c adapter, Init_Dict/Setup/parts registration and
test_pyvalue_dict.py additions exercise one-shot boxing OOM, real allocation
address reuse, and once-only equality; at most one third unique syntax TU.
This records the earlier coordinator handoff, without extending the ceiling.

Final source window closed early after independent review56bbc658 accepts
author receiptdb99c46e. Forty-three fragments/ten contexts include33 core
functions, five callbacks and five typed-notification argument bridges only.
Fourteen methods remain AST-only. Watcher/fixture syntax passes; Module Clinic
and whole-dictionary consumers remain open. No candidate execution claimed.
