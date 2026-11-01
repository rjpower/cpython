# Qualification of the selected nursery runtime

Draft updated 2026-11-01; **not execution-ready**. Follow the
[validation contract](../../design/validation.md). Qualify one coherent source
after the [cost decision](cadence-cost-plan.md), not both scheduling alternatives.
Mandatory cell/nonempty-base-tuple admission, checked explicit minors and the
removed global switch belong to either outcome. Automatic cadence is included
only if its correctness and elapsed/resource evidence earns acceptance.

## Freeze one source

The coordinator records the selected commit/tree, exact diff from maintained
primary, independent source reviews, carried feedback/negative capsules and
cost disposition before preparation becomes ready. Current cadence source
`47ccf35e21a94fde9351d1c2e4571bf207257a4c` has bounded debug/release feedback;
its earlier failures remain preserved. Broad qualification has not run.
A carried receipt needs an explicit unchanged
source/test/product bridge. Earlier manual-nursery and cadence feedback are
component evidence, not full qualification of the final composition.

Host-only intake at `build/nursery-qualification-intake-20261101/intake.json`
binds the current source rosters: adopted automatic-majors source has twenty
nursery methods, cadence source has twenty-seven. It also identifies the two
remaining Python broad-helper consumers of the old class name. This is not
selection of a source, proof of all metadata/JSON consumers, native coverage or
readiness; the frozen final plan must bind its actual roster and child scripts.

The handler-local slot-base proposal is rejected on its fixed elapsed result
and cannot join. Qualify the selected GC source alone. The separate VM0
compiler/backend investigation does not change this source composition.
No mid-cohort runtime, test or generator merge; a
changed input requires a newly reviewed affected scope and preserved history.
The rejected constant-operand increment is excluded.

## Reuse the accepted protocol

Adapt the sealed nursery/shape union helpers in
`../boil-nursery-shape-qualification/build/nursery-shape-{validation,early,common,broad}`.
The [union plan](../nursery-shape-landing/plan.md) and
[final review](../nursery-shape-landing/reviews/final-qualification-20261031.md)
identify their accepted scope. Reuse gate, process-group, build-lock, worker,
root, auxiliary, product and cleanup bodies; review a narrow helper diff and
all changed selections before activation. Do not inherit historical carry,
source identity or opt-in assumptions merely because a helper was accepted.

Use fresh GCC O3, GCC O1/Py_DEBUG, ASan Og/Py_DEBUG and Clang21 O1/Py_DEBUG tail
images of the selected source. A cost/feedback image may stand only through an
exact source/configuration/tool/product bridge to the final candidate and the
same effective build policy. Preserve frame pointers, zero initialization,
GIL/mimalloc/nonstats/JIT-off/no-PGO-LTO, GCC eval `-fno-gcse` and the reviewed
tail CEVAL override. All builds need module/import results and applicable
linked precise-root, effect, inline, tail-membership/CFI and ownership audits.

## Mandatory refreshed scopes

| Scope | Required selection |
| --- | --- |
| Early debug | Every selected-source `test.test_gc.NurseryTests` method, with exact child roster/mode/extras/stress and useful assertions, plus affected precise-root and lifecycle protections. Current cadence source has 27 methods; derive the final count and IDs from the frozen source. Preserve specific positive and source-negative provenance; direct expected-failure children earn no parent pass credit. |
| Backend/root | Registered six stale-root controls ×100 iterations on O3 OFF, O1 OFF and O1 KILL. On ASan and tail, run the complete selected-source nursery class OFF/N0 with actual supported children, plus the existing 24 critical shape cases. Tail also runs `test.test_gc.StackWalkTests.test_tail_call_handlers_precise` KILL/N0 unskipped. |
| Full ordinary | One release full OFF/N0, one O1 full OFF/N0 and one O1 full KILL/N0 through the accepted full cohort observer/validator. Discover and freeze the actual full module roster; retain compiled-root, bytecode-writing/trigger, queue/lifecycle and source-specific nursery protections. Derive tests/skips/module states from returned results, not historical counts plus new tests. |
| Stress | Existing eleven-module release and debug OFF/N1000 selections, once each, with exact inherited exclusions and reasons below. Actual stress1/both-kind and inherited KILL/extras controls remain separately identified component or refreshed focused evidence; they are not an eleven-module cross-product. |
| ASan | Existing sixteen-module KILL/N0 selection once, with two exact exclusions and sanitizer limits below. No new blanket nursery exclusion. |
| Closure | O1 `test_cext` with CPU resource; unchanged `make -C <debug> -s regen-cases clinic`; `Tools/boil/traverse_audit.py --check`; nineteen unbuilt-module syntax checks with frozen borrowed headers/tools. Reuse an exact-input early traversal receipt if unchanged. |

The stress eleven are `test_gc`, `test_weakref`, `test_generators`,
`test_coroutines`, `test_asyncgen`, `test_descr`, `test_capi`, `test_sort`,
`test_functools`, `test_context`, `test_ctypes`. Its existing exclusions are
`*test_resize*`, `*Writer*`, `*test_capi.test_unicode*test_from_format`,
`*test_verify_kill*`, and `test_remote_exec_deleted_static_executable`.
The existing [manifest](../../tools/verify.manifest.json) records their reasons:
resize has unrooted Writer buffer state under allocation stress; Writer cases
include recorded corruption/segfaults, including recover_error; from_format is
a Writer consumer outside that class-name selector; verify-kill stress exposes
compiler u_metadata held only through PyMem; remote-exec has a recorded
machine-local timeout predating the fork. Preparation must retain those reasons,
owners/removal conditions, enumerate the actual matching IDs in the new source
and reject any unreviewed expansion. In particular, resize-pattern exclusions match new
retirement fixtures: their normal/focused checker execution remains mandatory
and is not stress-profile coverage. A relevant new match requires an explicit
coverage disposition before readiness, not a silent broad skip.

ASan16 is stress11 plus `test_multiprocessing_fork`,
`test_multiprocessing_forkserver`, `test_multiprocessing_spawn`, `test_set`,
`test_memoryview`. Its exclusions are exactly
`test_remote_exec_deleted_static_executable` and
`*test_capi.test_mem.AddressSanitizerTests.test_buffer_overflow`.
The latter deliberately requests PYTHONMALLOC=malloc, which this fork does not
honor: mimalloc intercepts the deliberate overflow before ASan.
Retain `ASAN_OPTIONS=detect_stack_use_after_return=0:detect_leaks=0`: no fake-stack
or leak-check coverage follows. Multiprocessing umbrella modules supply no
unexecuted leaf coverage. Full ordinary retains only the existing exact remote
deleted-static-executable exclusion, with its original reason bound in the
ready manifest. No filter may be widened to turn a failure into a pass.

## Remove stale opt-in assumptions from helper consumers

The runtime source removes `gc_set_young_enabled`, its restore prelude and
`MinimalNurseryTests` verifier/stress restrictions. Never reintroduce a setter,
compatibility fallback or second enabled/disabled campaign in the helpers.
Update all live copies used by the new campaign; preserve historical artifacts.

- `nursery-shape-broad/run_broad_v2.py` and `verify_asan_broad.py` currently
  require the five old unsupported nursery skips. Replace that rule with the
  exact current class/protected cases and reviewed legitimate skip identities.
- Early/common/tail job filters, protected selectors, fixed five/29 counts and
  plan selection inventories refer to the old class. Rebind them to the actual
  selected-source class and backend counts, including current precise-helper
  availability guards. No disappeared old ID earns coverage.
- Rebind source/native/library/tool hashes in current-input loaders and worker
  observers, and carried integration smoke filters/counts. Check these exact
  consumers for old setter preludes, enabled-state assumptions or historical
  schema fields before sealing. Do not reuse an old mode/age schema for a new
  diagnostic getter by guessing compatible fields.

Existing main-interpreter Linux/GIL/mimalloc support remains the boundary.
Legitimate debug-only, unsupported-backend and missing-helper skips must be
listed by exact ID/reason before activation; supported debug nursery controls
cannot regain blanket verifier/stress skips. Inspect any changed count rather
than deriving it from the former five unsupported skips.

## Missing modules and closure limits

The current fresh build reports 116 checked modules: 37 built-in, 67 shared,
one platform-inapplicable, one disabled, ten missing and zero import failures.
Record each final image's actual report. The ten are `_bz2`, `_dbm`, `_decimal`,
`_gdbm`, `_hashlib`, `_ssl`, `_tkinter`, `_uuid`, `_zstd`, `readline`;
`_sqlite3` is explicitly disabled. These are environmental coverage limits,
not permission to exclude their whole suite silently or to claim their imports
passed. Preserve all returned SKIPPED/RESOURCE_DENIED/ENV_CHANGED states and
exact reasons. New missing modules/import failures stop preparation/execution
until independently assessed.

Closure's nineteen syntax receipts cover decimal twice, SSL/hashopenssl,
all existing sqlite C units, bz2, zstd C units and uuid with its existing stub.
Freeze borrowed header aliases/contents and compiler identities. This supplies
syntax coverage only; `_tkinter`, `_dbm`, `_gdbm`, `readline` remain unchecked
for lack of headers. Preserve SSL/pip diagnostics. `test_cext` keeps the six
limited/stable-ABI skips (Python.h rejects Py_LIMITED_API) and the MSVC-only
C++14 skip, by exact identity/reason; other counts come from actual results.

The earlier adoption whole-GC OFF/KILL runs each executed 204 tests with their
recorded skips. They remain that historical source's first-feedback scope,
not a replacement for the selected source's full GC module or full suite.

## Admission, budget and acceptance

Keep both existing machine slots and each actual build lock; serialize full,
StackWalk and perf-map-sensitive jobs. Assert actual worker/child
mode/extras/stress, origins, seed and source/configuration bindings. N0 omits
the stress environment variable. Use writable disposable caches, admit only
recorded regular cache additions, preserve terminal raw streams on timeout,
and require current idle cleanup. Expected KILL subprocesses and informational
auxiliaries cannot substitute for normal terminal worker/root witnesses.
Zero unwind fallback remains a gate; report-only unpinned visits stay diagnostic.

Propose one **90-minute** qualification batch from first native launch,
including builds, checks, closure, cleanup and independent outcome review,
bounded also by the issue's existing overall deadline. Preparation must show
the actual remaining budget is feasible before activation; otherwise the
coordinator explicitly selects a new bounded decision before execution.
Use accepted per-command ceilings (build2400s, focus900s, worker600s,
release-full3600s/debug-full5400s, stress/ASan2400s; closure cext1200s,
regen600s, traversal300s, syntax600s), always downward-clamped to the overall
deadline with verified cleanup reserves. Checkpoints: early feedback15min,
build/backend30min, full-three65min. These are bounds, not permission to run
past the batch deadline or reset a failed clock.

Stop at the first unexpected audit, test, environment, protection, skip,
fallback or timeout failure. Preserve the failed envelope before diagnosis;
no automatic retry or second qualification ladder. A final independent review
must bind actual cohorts/products, source negatives, carried/refreshed scopes,
deadline and current cleanup. If integration differs only by enumerated project
prose, prove exact remaining tracked bytes/modes and use the accepted fresh
release/debug post-merge smoke protocol. Other changes require affected checks.
Qualification establishes correctness within these limits; the separate cost
decision owns performance and nursery architecture remains mandatory.
