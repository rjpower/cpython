# Managed scanning: as built

2026-10-30. The reviewed direct-scanning source is part of composed successor
`0b15b4bbc9b`. Four build/audit cohorts and focused/affected behavior pass;
**The operator composite qualification is PASS; independent LAND clears this integration;
postmerge smokes remain pending.**
No direct-scanning speedup or allocation/lookup reduction is claimed yet.

## Implemented boundary

`_PyGCValueRange` is a borrowed address/length view. Cell, tuple and frame
accessors provide the shared definitions of value fields and physical bounds.
Generic cell/tuple traversal uses the same accessors. The original three-line
`Python/gc_live.c` frame-loop fragment was applied and independently reviewed
in the named native-cleanup/scanner composition; it is no longer an unapplied
or cross-owner proposal.

`live_precise_child` contains the old `live_visit` operation, including exact
`live_lookup`, current-domain validation and invalid/dangling-pointer checks.
`live_visit` remains the native `visitproc` adapter. Managed loops invoke the
concrete helper after tag checks, without boxing or canonicalizing values.
Tuple scanning preserves reverse element order. The constant direction
argument is available for compiler specialization; actual dispatch savings
and linked-code size still require post-landing measurement.

Exact cells and nonempty exact tuples dispatch directly in `live_drain`.
Their type edge remains first, and the layouts exclude runtime-attribute and
inline-attribute storage. Other objects, tuple subclasses and empty tuples
retain `_PyGC_TraceTraverse`, including its type, runtime-attribute and static
rules. Diagnostic traversal remains generic. There is no type-count cutoff,
allocator change or broad elision of object-start/domain checks.

A shared private frame enumerator serves generic and live adapters. It keeps
chunk order, pushed-but-unlinked frames, the uninitialized executable sentinel,
avoidance of duplicate thread-owned frames and active generator frames.
Interpreter-owned entry/base frames still visit only their return slot.
`Python/frame.c` shares the fixed-header visitor; the live adapter marks the
array directly after those fields. Its callback is per frame, while the small
header set keeps the existing visitor contract. Generic traversal propagates
visitor errors as before.

These views are neither registered roots nor writable relocation proofs.
They do not turn weak/borrowed fields into strong edges, authorize moving an
interior pointer, infer domain ownership from an object-map bit or skip
SHALLOW/publication rules. Mark-stack OOM keeps the existing rollback; range
enumeration creates no allocation or new failure state.

`Include/object.h` only corrects the opening description to the actual
one-word shape/flags header, tracing ownership and explicit resize rules. It
changes no layout. A cell's source layout is one 8-byte header plus one 8-byte
value on this platform; this is not a new runtime size measurement.

## Behavior and source history

The four `ManagedScanningTests` controls use the existing child helper with
actual mode/extras/stress assertions. They check a cell-owned mixed tuple's
mutable data across collection, tuple-subclass elements and attributes, locals
across normal/raising callbacks, and generator payloads across suspension,
running collection and escaped-frame lifetime. Automatic collection is off
inside these fixtures; explicit collections select the liveness boundaries.
The generator case is not exclusive proof of direct active-frame dispatch:
generic traversal of a rooted generator can also reach those values.

The original `d0f4671006c` checkpoint and both source packets remain preserved.
Reviewed scanning source `b9d829` joined selective death processing and native
cleanup before builds under the coordinator's named composition decision.
The released frame-loop hunk retained bounds, order, initialized-slot rules and
visitor error propagation. Root and the native-cleanup owner supplied the
independent scanner review; the scanner author did not self-review it.

The initial coherent339 candidate passed focused controls, whole affected
modules, three full profiles and release stress. These remain evidence for339,
not fresh fulls for `0b15`. Debug stress then exposed coroutine origin traversal
before constructor initialization. Both common constructors now initialize
the shared slot before tracking, via the independently reviewed two-store
repair. This preserves the traversal contract instead of weakening tag/type
checks or exempting the object. Parent75e8 has the same late initialization;
there is no matched-control basis to assign the defect specifically to direct
scanning.

Successor0b15 has fresh O3/O1/ASan/tail build audits and module checks.
Its focused origin/data controls, whole GC/generators/coroutines/asyncgen in
release off and debug kill, tail ownership and exact debug stress pass.
The retained build and focused indices are
`build/gc-native-cleanup-qualification-v3/build-completion.json` (`3db801b2`)
and `focused-completion.json` (`20e132de`) in `boil-gc-native-cleanup`.
The final index must preserve both the339 broad identities and fresh0b15
coverage, original failures, source/native/compiler bridges and resource skips.

ASan16 and closure pass; independent final LAND clears the composite; postmerge smokes remain pending. Linked
direct-loop cost, total code-size effect, post-cleanup timing and the next
trusted-repeat-edge proposal remain unmeasured. This slice introduces no
moving backend, universal layout registry or relaxed precise-edge validation.


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
The two normal postmerge smokes remain pending; no
post-cleanup performance result is claimed.
