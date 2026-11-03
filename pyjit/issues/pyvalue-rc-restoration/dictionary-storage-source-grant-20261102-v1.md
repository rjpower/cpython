# Dictionary copy, conditional insertion and managed storage

Final disposition: source window closed early. Final-v2 author7ab4e86a and
independent reviewf7fc0884 accept36 scoped fragments/seven context files,
22 AST-only methods and four adapter entries. External object/import acceptance
is limited to the named provider/error-check hunks. Whole Dict/native/runtime
qualification remains open; legacy API-removal action remains unexecuted.

The coordinator adopts D's critical design review of corrected proposal
build/pyvalue-register-interpreter-checkpoint-20261101-v1/dict-mutation-source-v1/next-storage-cut.md,
SHA256809751f8556bb778e0076e2f0a3845ce4c00de192513b7a2a35cb6718ebde6b9.
C implements; D reviews actual ownership, callback and failure transactions.
Prior mutation source remains frozen and accepted; no old budget is extended.

Distinct90-minute source window: conservative start 2026-11-02T00:59:45.000Z,
early checkpoint 2026-11-02T01:19:45.000Z, ceiling 2026-11-02T02:29:45.000Z.
Record actual first edit. The early checkpoint must show concrete copy versus
detach ownership/allocation seams, not merely declarations or a design summary.

Exact function ledger is the reviewed proposal: copy/clone plus coupled merge,
owning conditional insertion, popitem, shared-key construction, managed
materialization/detachment and canonical instance providers/object adapters.
Only named functions/declarations and their necessary direct handoffs are
authorized, not whole-file ownership. Add focused tests to existing dict.c and
test_pyvalue_dict.py. No legacy getter/SetDefault declaration or API removal;
the superseded removal script/equivalent stays outside scope.

Pin live instances across allocation only on the live-object paths.
PyObject_ClearManagedDict can enter from subtype_dealloc at refcount0;
its clear/detach path is deallocator-owned, with NO INCREF0 or resurrection.
Retain type/key metadata and externally retained dictionaries as documented.
The OOM fallback must detach/publish NULL1 arrays, used/count/order/validity and
dictionary associations before any callback-capable close; no stale live array
or association can survive allocation/reentry.

Copy duplicates owners into detached allocation after reacquiring live source;
detach transfers owners and NULLs old homes without allocation. Preserve source
snapshot count through later destination allocation. CLONED/merge revalidates
actual target/source state after notification and allocation, preserving mode
semantics and callback mutations. No PyValue-array/object-array casts.

SetDefaultValue returns0 for insertion,1 for existing, BOTH successful with an
independent requested owned canonical output. ONLY -1 guarantees NULL1 output.
Pin inputs before callbacks; conditional insertion re-probes and cannot blindly
overwrite a newly present value. Retained owning object adapters must stage any
fallible result boxing before a mutation that should not occur on boxing OOM.
Preserve existing watcher event/timing and one-shot notification rules.
Borrowed legacy SetDefault remains a separately designed scope bridge, not an
unsafe temporary box, cache, ownership-contract change or hidden removal.

The four coupled boxed-error consumer handoffs are exact hunks only:
Objects/object.c _PyObject_GetMethod, _PyObject_GetMethodValue,
_PyObject_GenericGetAttrWithDict; Python/import.c lazy initialization probe.
Prefer the canonical provider for the value method path. NULL plus error must
not be treated as missing, descriptor fallback or a successfully initialized
module. A owns later specializer/VM reader and managed-store generation;
freeze additive declarations and coordinate that handoff before interface edits.

Cheap gates: exact named-function projection, owner/callback/OOM/ABA review,
lexer/AST and focused drafted tests; at most3 meaningful actual syntax-only
translation units30seconds each. Retain wider legacy/Clinic failures honestly.
No object output/link/candidate execution/timing. Full Dict/native/ABI and
whole-runtime qualification remain required; deallocators preserve entry errors.

Formal design review00647992 binds the corrected zero-RC design and grant
conditions. Actual first source edit01:01:27.560330 UTC; concrete early
checkpointb4323811 includes eight copy/lifecycle fragments, before01:19:45.
Copy allocates then reacquires and duplicates; detach allocates/revalidates then
transfers/NULLs without INCREF(instance). Review corrected prior-exception
handling so a legitimate deallocator-entry exception is not a false detach OOM.
Actual remaining transaction source remains under review; no compilation/runtime
or whole-dictionary acceptance is claimed by this checkpoint.

Additive reader handoff, same02:29:45 ceiling: C owns only new
_PyDict_LookupIndexAndOwnedValue and the existing owned-object _PyDict_Subscript
adapter plus necessary declarations. Retain legacy object-out getter symbols.
Use canonical owned lookup output, preserve KeyError/subclass __missing__/frozen
behavior and fallible owned boxing. An opaque __missing__ object result is not
canonicalized-and-released before caller publication. A owns the separately
granted four specializer calls and actual VM cold/warm consumers. Freeze these
two provider bodies/declarations before A's final checks; no other C body or
deadline is transferred/extended. This is not an API-removal amendment.
