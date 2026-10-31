# Native suspended-generator cleanup: as built

2026-10-30. Reviewed successor `0b15b4bbc9b` combines selective death
processing, native suspended-generator cleanup, managed scanning and the
warning-context repair. Four fresh incremental build/audit capsules pass.
Focused behavior and the previously failing debug stress pass. **The operator
composite qualification is PASS; independent LAND clears this integration;
postmerge smokes pass.**
This record does not turn predecessor passes into successor full-suite passes.

## Implemented contract

`gen_close_clears_frame` shares public `close()`'s existing proof: a RESUME
instruction with the depth-one operand and no unwind tools. Collector
admission additionally requires an exact ordinary suspended generator,
generator-owned frame storage and no escaped frame. Created, running,
delegated, coroutine, async-generator and escaped-frame cases use the old path.

`native_cleanup_ready` chooses for the whole pass. Actual DEL, FIRST, HOOK,
BUFFER or registered-WEAKREFS work rejects the cohort. FINALIZE is only a
prefilter: dynamic finalizer eligibility is checked again. Queued/draining
work, debug/save-all/callback observers, tracing/profile/monitoring tools and a
custom eval-frame hook also select the unchanged path. Admission requires the
supported main-only GIL domain; free-threaded and foreign-interpreter cases
are excluded. Finished or already finalized generators are not made pending
again. Selective death's actual-work bits replace the original source
checkpoint's redundant type-capability/header checks.

Classification and admission mutate no object or graph. All fallible order
preparation and a second complete admission check precede irreversible frame
clearing. An admission mismatch discards the temporary order and runs normal
preparation; OOM retains the existing no-sweep rollback. There is no recovery
path that exposes a partly cleared cohort after preparation failure.

The admitted batch is empty, so routing adds no queue allocation. With one
interpreter there is no foreign deletion-barrier target. Domain unpinning
uses the existing non-detaching HEAD lock and scalar pin/event operations;
it calls no Python. This is the supported teardown/allocator contract, not a
proof about arbitrary custom RAW hooks.

During guarded sweep, `_PyGen_ClearUnreachable` roots its argument with the
existing one-value span, marks finalization complete, sets FRAME_CLEARED and
clears the frame. No escaped frame means no ownership-transfer allocation.
Frame/local/function/exception references and profiler state are cleared by
the existing operations. Ordinary deallocation still releases names, code
and storage, with existing live-byte accounting. No scheduler, registry,
callback cadence or ordering-policy change is introduced.

The separate deallocator simplification removes the redundant
TRACK/finalizer/UNTRACK ceremony around a helper that already returned zero in
this fork. Its exported signature and no-op return remain; actual generator,
frame, origin, name, code and storage cleanup remain.

## Source and build identities

The original five-file implementation is preserved at `32d4e339c2d`.
The composed warning-joined runtime was `4d8e690f483`; effect successor
`17efeeac9f8` added the exact noncollecting `_PyCode_InstrOparg` contract and
unit control. Source `33944d24e55` then added only the independently proved
`strrchr` effect declaration and its archived unit control. Neither effect
correction changes runtime C behavior or weakens the checker.

Debug stress on339 exposed a real constructor publication defect. Both shared
generator constructors tracked an object before initializing its
origin/finalizer slot; positive-depth coroutine origin construction then
allocated while traversal could visit that slot. Parent75e8 has the same
ordering, so the failure alone does not establish which newer scanner made
it observable. The original fatal run and its postguards remain preserved.

Reviewed repair `173adf527d5`, composed as `0b15b4bbc9b`, adds exactly two NULL
stores before TRACK. Later assignments and branches remain. Its regression
checks origin metadata and usable coroutine return data under collection
during construction. Source proof covers both the ordinary and public frame
constructors without adding a C test adapter or general publication framework.

All four configured cohorts were preserved, then rebuilt with the accepted
make-only recipes at `0b15`. GCC O3, GCC O1/Py_DEBUG, ASan Og and Clang21 O1 tail
passed compile/link, applicable precise/inline/CFI audits, 116 module checks,
product freeze and cleanup. Original configurations and tool identities were
retained. The genobject compilation proof records the changed unit; this does
not claim every unchanged unit was recompiled. Build checks alone do not
provide runtime qualification.

The retained candidate worktree is `boil-gc-native-cleanup`. Its frozen indices
are:

| Evidence | Candidate-relative path | SHA256 prefix |
| --- | --- | --- |
| Four successor build capsules | `build/gc-native-cleanup-qualification-v3/build-completion.json` | `3db801b2` |
| Successor focused/affected outcomes | `build/gc-native-cleanup-qualification-v3/focused-completion.json` | `20e132de` |
| Original339 failure and postguard | `build/gc-native-cleanup-broad/failure-completion.json` | `447af602` |
| Original four-image preservation | `build/gc-native-cleanup-preserved-339/preservation.json` | `7276117e` |

## Observable coverage and composite boundary

The five native-cleanup controls preserve finalizer/callback inspection of a
suspended generator, mutable payloads, eventual notification and clearing,
delegation/finally order, and escaped-frame data. They use existing actual-mode
child helpers. Four managed-scanning controls and the warning regressions
joined the initial coherent candidate; none is a private admission counter.

On `0b15` the unchanged old diagnostic completes with valid origin/data. The new
origin regression passes once in O3 off and O1 kill. Whole GC, generators,
coroutines and asyncgen pass in each cohort:456 tests/35 skips in release and
456/21 in debug kill. Tail ownership passes 1/0. These scopes overlap; they are
not added into a distinct-test total. All report zero root fallback; each
whole-module row has one report-only detached-snapshot visit.

Predecessor339 retains three full passes of51,255 tests each and release
stress11 of3,115/148. Those remain339 results. Its debug-stress fatal stays
FAIL, with ten returned module passes and no completed coroutine/root record.
The successor's exact debug-stress profile passes3,116/113 across11 modules,
38,251 collections, zero fallback and 43 report-only detached-snapshot visits;
the extra test is the origin regression. Final qualification must explicitly
combine source-delta proof and these fresh results with the retained339
coverage. It must not relabel the old fulls or failed stress.

ASan16 and cext/regen/traversal/header closure now pass.
Independent LAND clears the composite; this primary integration reconciles the record.
Historical source/build/filter/oracle failures remain in their original
receipts. No post-cleanup elapsed or CPU improvement has yet been measured;
the held finite performance plan follows qualification and postmerge smokes.


## Terminal qualification capsule

The operator's final index is
`build/gc-native-cleanup-broad-v2/final-completion.json` in retained
`boil-gc-native-cleanup`, SHA256
`50054725c871a7d71a0eceb71db1d8373d335837aff8ed73e6dec208cb7a27eb`.
It records qualification PASS and382 absolute artifact references, with
source/runtime/native/ASan heads all `0b15b4bbc9b`. It explicitly carries the
three339 fulls and release stress at their original identities, plus the
reviewed two-store delta and fresh successor coverage; it does not claim a
uniform0b15 full-suite rerun. Original failures remain preserved.

Fresh ASan16 passes4,038 tests/141 skips, with13,095 collections, zero root
fallback and3 report-only detached-snapshot visits. Leak/fake-stack detectors
remain off; skipped multiprocessing parent wrappers do not establish leaf
process coverage. Closure passes cext16 with the exact seven declared skips,
regeneration/clinic, traversal and19 borrowed-header receipts; `_tkinter`,
`_dbm`, `_gdbm` and `readline` remain explicitly unsupported. All four source,
configuration, tool and product postguards pass. Independent LAND clears the composite; primary integration includes this record.
The two normal postmerge smokes pass; no
post-cleanup performance result is claimed.

## Integration and postmerge checks

Integrated as `aee46aacae2`; the exact difference from qualified0b15 is13
project Markdown paths. Final postmerge index
`build/gc-cleanup-integration-v2/final-receipts.json`, SHA256
`382dd354f46d84391f2e27d6c593dd043a6ee258bb3bd2ab6e76dd8ae2ff1215`,
binds23 receipts. Release passes1,937/80 and debug1,937/50; each has all nine
modules passing, native/launcher/cleanup0 and zero root fallback. Debug has
two report-only detached-snapshot visits; release has zero. These are fresh
integration checks with the qualified images, not a new native rebuild.
