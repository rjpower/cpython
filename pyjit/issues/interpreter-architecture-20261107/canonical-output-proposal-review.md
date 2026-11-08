# Canonical shape result: proposal review

2026-11-07. **Decision: revise before authorizing the direct-publication
experiment.** The proposed local optimization is plausible, and its
temporary-only store preserves the existing publication-before-retirement
order. The current proposal does not yet close the input contract and producer
checker needed to skip `_PyValue_Publish` safely. I reviewed runtime
`225480a4250` read-only; no native code, build, test or workload was changed
or run.

The transitive source argument is mostly sound for in-tree writers. The public
object adapter and value-slot thunks call `PyValue_FromObjectBorrow`; the
specialized shape stores transfer an existing frame-home value; the generic
setter reaches `store_instance_attr_lock_held` through
`_PyObject_StoreInstanceValue`; that function duplicates the borrowed input
into `values->values[ix]`. Initialization writes NULL, and copy/move paths
preserve bits. `LOAD_ATTR_SHAPE` reads that inline home and `PyValue_XDUP`
preserves its bits. A direct first temporary store could therefore retain the
same canonical value and owner, while the folded-local final store keeps
`_PyValue_Publish` and its original displaced-owner retirement. The existing
GCC release image still contains a first-publication canonicalization check,
so there is actual code to test removing; it proves no elapsed benefit.

Two conditions must be settled in the proposal before code changes:

1. `_PyObject_StoreInstanceValue` is an exported internal `PyAPI_FUNC` whose
   present contract permits a general borrowed language value. A raw
   pointer-form exact int/float can pass that contract and reach an inline
   shaped home in release. Merely documenting a stronger precondition and
   checking it in debug makes the new optimization sound **only if every
   supported caller is required to pass a canonical home value**. State that
   decision explicitly, audit/update all supported native/JIT callers, and
   make the entry debug check fatal even when the optional home-violation
   reporter is configured to count and continue. If the broad input contract
   remains supported, retain `_PyValue_Publish` or normalize on entry; do not
   use the direct store. The `Py_GIL_DISABLED` path is outside shape stores,
   but both build modes still need an unambiguous API contract.
2. The proposal calls `result_publication_contract(inst, fmt)` “existing,”
   but that checker and `IR_RETIRE_PUBLISHED_RESULT` are still only in
   `contract-proposal.md`; they are absent from the integrated generator and
   `Python/bytecodes.c`. Name successful integration of that first contract
   as a hard dependency. For the new `IR_CANONICAL_HOME_DUP(attr, found)`,
   specify an exact accepted source form and guard chain for this one shape
   read. The generator cannot infer from an arbitrary C pointer cast that
   `found` came from a canonical home. It should reject changed provenance,
   branch/join, reassignment, address-taking, or intervening callback syntax;
   a debug `_PyValue_RecheckHome(found)` is a backstop, not a release proof.

The smallest clean interface is one source declaration for this exact
producer, consumed by the existing result contract's checked record and by
the first temporary-destination flush. Keep the generic
`IR_STORE_TEMP_DST`/`_PyValue_Publish` elsewhere, including uncertain source
forms. No new global value representation or second opcode table is needed.
The proposal's stated tests for number forms, immortals, replacement
finalizers, reentry and folded locals are appropriate after those two
conditions are resolved. A matched workload comparison must decide whether
the extra declaration pays for itself.
