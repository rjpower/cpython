# Integrated span native correctness outcome, v5

2026-11-01. **Accept this focused correctness feedback; the candidate is ready for a separately reviewed elapsed experiment.** No performance, parity, full qualification or landing credit.

Exact source `5a457c738310f9dd762c88743af9042258841c74`; source capsule `628989536cb1104ea8d05efab05a9a5132e6860d900b5665fcc3c824ef5a2d14`; source review `f1647711d057b29c20ca35c2bac9b4825f790ffb85cb899d9b5aa223ac8c7df4`. The coordinator confirmed supervisor session 83297 terminal exit 0 at 05:16:42 UTC. Completion `efb8924a0a43ca9c38c3da22c72bea1e17d072061e84859bc5e886900a6fb261` records 05:16:16.290202 UTC, inside the unchanged 05:12:09.511639–05:57:09.511639 window.

All four literal build/test rows passed their launch, verifier and cleanup contracts. Debug selected 27 tests (21 span methods plus six existing controls): 27 ordinary passes, no skips. Release selected the same 27: 23 ordinary passes and exactly four declared skips. Both cohorts completed all four selected modules. This is a focused selection, not four whole-module passes or general support qualification.

The actual raw logs report `ok` for `test_public_unpublished_free_under_wrapper`, `test_custom_wrapper_declines_and_forwards_old_free`, and `test_wrapper_restores_after_exception` in both images. The exact compiled fixtures require the public typed allocator/free, same-descriptor keeper, one nominated free and owned-free state before reuse; published coroutine death and one nominated saved-free while the wrapper remains installed after producer-thread exit; ordinary new admission decline; and allocator restoration. These are strict passing source assertions, not separately printed scalar telemetry. No callback or forwarding witness was inferred from an empty event list.

The exact release skips are:

- `test_declined_reservation_is_gc_visible`: one-shot allocation fault is debug-only.
- `test_failed_mark_clears_real_constructor_prefix`: actual constructor-prefix fault is debug-only.
- `test_quarantine_release_and_oom`: quarantine requires Py_DEBUG.
- `test_raw_slot_guard`: slot guard checking requires Py_DEBUG.

Actual ordinary driver/worker start/end records show OFF/no extras, stress zero, seed/config/source/CAPI origin and selection bindings. Each of the five ordinary PIDs per image maps to native root records, as does its separately observed platform auxiliary PID. Debug records 497 collections, release 451; both have zero fallbacks and observed zero unpinned. Unpinned remains report-only. Seven debug and six release inner root-only PIDs remain explicitly unattributed; they earn no additional case/child, stress or full-KILL credit.

Independent replay verified all 4,737 completion artifacts, all 4,707 plan inputs, 19 source-capsule references and 33 helper-capsule mode/size/hash records; exact HEAD stayed clean. Each image's 589 recorded build products, Makefile/pyconfig and selected toolchain identities remain unchanged. Builds report 116 modules, ten missing and zero failed imports; that does not silently qualify the missing modules. All four post-gate records show no live target PIDs before deadline. A final independent current `/proc` executable scan found neither image running. The release image hash is `3a2b9aa1ee98390bb2c35734d531d7da78466642ceed4a782a9a07dc30a922b1`; debug is `3e41aa45f08da18334041eed821ea5e6d4c4ac3c22a3f737bb1bbc884bbbdee2`.

The accepted unchanged full-result and roster validators were replayed. An initial reviewer-only replay incorrectly treated the four declared release skips as protected; it was corrected to the exact existing runner derivation that excludes only those named skips. The initial host failure log is preserved. No target, replacement test, compiler, ELF command or new probe ran during this audit.

Earlier failed v1 invalid-frame, v2 disabled-GC witness, v3 hidden-module-import and v4 wrapper-lifetime cohorts remain failed. The original v4 release failure's cause remains unresolved. Diagnostic nonreproduction is not original-case acceptance; the new producer-thread fixture establishes a different explicit lifetime precondition. Remaining qualification, broader lifecycle/allocator support and measured resource/performance decisions stay open.

Evidence: [independent receipt](../../../build/span-native-outcome-review-20261101-v5/receipt.json), with per-PID mappings, exact skip reasons and immutable native artifact identities.
