# Stable-span second native outcome

2026-11-01, coordinator. **STOP: focused correctness incomplete.** Session
60010 returned 1 at 03:59:31 UTC. Source was
`67190a9d81c683f7e1713300b9a99feb2bc85a93`. The fresh GCC O1/debug build
completed in 94.17 seconds and verified. Image SHA256:
`7c70c4652d5b8e8cc8665f86c45d8a244c3a160706395bf5b1d1cef3aff229a5`.
Release build/tests and elapsed measurements never ran. All original evidence
remains under `build/worktrees/gc-integrated-spans/build/span-native-feedback-v2`.

The worker ran three cases: backend reallocation and the supported native-call
constructor passed; `test_collection_callback_cannot_inherit_typed_route`
failed because its observation list was empty. Three later modules were
omitted. Actual post-gate cleanup acquired the build lock and found no live
target. No whole-worker pass or qualification is claimed.

The test class disables automatic GC in setUp. Its new C helper called public
`PyGC_Collect()`, which returns immediately when GC is disabled in the pinned
runtime (`Python/gc_live.c`). Thus it never exercised the callback. Root found
the same witness defect in the new partial-mark-failure helper before that
case ran. This outcome neither establishes allocator corruption nor permits
weakening the positive collection assertions.

The narrow correction replaces those two witness calls with manual
`_PyGC_Collect(..., _Py_GC_REASON_MANUAL)`, matching the existing `gc.collect`
implementation. It preserves callback execution, route exclusion, positive
mark/shallow counts, cleanup and error propagation, and explicitly checks that
automatic GC stays disabled. Production allocator/collector policy is unchanged.
The [independent v3 source review](span-manual-review-20261101-v3.md) accepts
that correction for a fresh batch; it does not retroactively pass v2.

The original activation and failure remain immutable. The fixed GC cutoff is
15:14:44 UTC. No unused activation time grants a retry.
