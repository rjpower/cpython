# Three GC safety repairs: integration review

Coordinator review,2026-10-30. **LAND_INCREMENTAL: source and closure clear.**
Reviewed native candidate `168f30a96ffdb5516fe94e9ed13b0e60dcb9fae6`
and test-only successor `9e223040a2070c75f88be10366e0f0a7b35f6ed3`
against common ancestor `5a45f9bb6a73e7351c861faf05624ae4810f6d87`.
The primary runtime d6a23 has identical native source to that ancestor.
The candidate changes only six source/test files; a normal merge preserves
primary-only documentation. Do not replace the primary tree with a checkout
or copy of the candidate.

## Source findings

Reviewed the complete six-file delta: pycore_gc, test_gc, _testinternalcapi,
weakrefobject, gc_live and gc_walk, plus the existing component source reviews.

- Sweep ordering is prepared at the final nonreentrant boundary, after call
  routing and pin retirement and before any deallocation. Allocation/traversal
  failure retains the dead snapshot and live-byte/heap accounting. The unsafe
  heap-order fallback is removed. Already queued calls retain the documented
  partial-delivery behavior; this is not an all-or-nothing delivery transaction.
- All six weakref/range mutex acquisitions keep the GIL. Their holders perform
  native-only operations; private weakref metadata uses libc allocation rather
  than replaceable RAW/tracemalloc hooks. Lookup-only weakref queries no longer
  construct a table. This closes the reviewed snapshot detach/reentry hazard,
  not every foreign-object or extension-traversal hazard.
- Debug output copies bytes while the snapshot is borrowed, then invokes the
  Python writer after garbage publication, stop callback and collecting reset.
  Buffer OOM flushes to native stderr without Python. Garbage publication has
  an explicit root over its native pointer array. Nested output ordering is
  deliberately not a global ordering promise.
- Scratch-stack growth is checked and sticky. Temporary MARKED bits are set
  only after successful cleanup-journal insertion; all exits clear that journal.
  Visitor/resurrection failures propagate to the common retain-all rollback.
  Incomplete cycle traversal cannot publish garbage as a completed result.
- The final test-only change disables automatic collection during a diagnostic
  counter comparison. Manual collection and every assertion remain. Existing
  support.disable_gc restores the prior enabled state. It does not suppress
  stress for the rest of the module or change native products.

No source blocker found for this incremental landing. The supplied causal
controls restore the unsafe fallback, immediate writer and mark-before-journal
behaviors separately and fail their intended ordinary assertions. Their
historical native/build/harness failures remain distinct from those controls.

## Evidence and remaining landing checks

The coordinator independently checked all86 held artifact references, raw
artifacts and cleanup for six passed profiles: both ASan groups, release full,
debug full, debug-kill full and release stress11. The debug stress original
remains failed. Its ten passed modules carry forward explicitly; the fresh
whole-GC debug/off/N1000 successor passes152 tests/4 skips, with21445 collections
and zero fallback. Its raw artifacts and cleanup were independently checked.
Native168 and source9e identities remain explicit throughout this composite.

The reviewed closure helper49b05232/plan dcd98f03 preserves C-extension,
regeneration, traversal and borrowed-header checks. It checks the normal
source/test bridge and the separate native168 ASan capsule; it neither fakes
the old source HEAD nor reruns full profiles. Closure passes: C-extension16
tests/exact7 declared skips, unchanged regen-cases/Clinic, traversal and all19
borrowed-header receipts/four explicit unsupported modules. Final index
9bacd1901e16ace8fe72fd03ef7e4a968eceb5429a071ec7becaf9dd1d096362
binds43 fresh references plus the retained86-reference held packet. All43
fresh files were independently checked by the coordinator. Closure has zero
fallback, and post-gate source/configuration/product checks pass.

Next: merge the unchanged candidate,
verify only project prose differs from qualified source, then run the existing
bounded release/debug integration smoke with explicitly reused code-identical
products. Do not repeat the full qualification for Markdown-only differences.

This increment does not fix concurrent callback completion, ordinary native
buffer release, all remaining root/port contracts or unsupported lifecycle
modes. Main-only admission, one-drainer delivery and finite buffer ownership
remain separate changes in the revised closure plan. It establishes no new
performance result or full-GC correctness claim.
