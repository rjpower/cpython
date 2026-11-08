# Prepared RC fallback: restore a coherent compiler/runtime checkpoint

2026-11-01. **Recommend one RC-only successor based on the complete runtime
tree at `b5541cbbbcd961ac29ab7c03000d8331af0bc0a3`.** This is preparation,
not authority to change a checkout, build, target or measurement. Preserve the
current tracing work and evidence on their existing branches.

The user's GC clock is **03:14:44–15:14:44 UTC on 2026-11-01**. The coordinator
must accept geometric-mean elapsed parity across **all 29 registered workloads**
against matched upstream before that deadline: fork/upstream elapsed ratio
at most 1. Individual kernels need not all beat upstream. Historical scores,
six-workload counters, proxy improvements or an unfinished campaign do not
substitute for an eligible accepted comparison. If parity is not established
in time, stop GC optimization and switch to this RC route; do not extend the
clock to complete another GC hypothesis. Preparing RC now avoids making the
deadline the start of an unplanned ownership rewrite.

## Actual checkpoint and evidence

`b5541cbbbcd` is the direct parent of
`834938a4ffa4c3582a7e1e505f7bd41ce0eec480`, the commit which mechanically
resolved Py_GC true and deleted RC/cyclic-collector code. At b554, configure.ac
explicitly defaults with_gc=no. Source inspection also establishes real RC:
GIL Py_DECREF decrements ob_refcnt to `_Py_Dealloc`/tp_dealloc; stackref
DUP/CLOSE acquire/release owned pointers; `Python/gc.c` owns tracked
generations/cycle detection/tp_clear; tuple mutation and long negation retain
their RC uniqueness branches. This is not merely an old configure option.

Historical gates are recorded at b554: the step 3b compiler at
`691c92537c6cf76a3278b82fe2445af896397a51` had dev-default/dev-debug full suites
with the known remote-exec test_sys failure, seven ports, and a tail-call
focused gate. The step 5b handoff records dev-default rung 2 plus test_gc_verify
at f58c4aeb94a: nine files/1,067 tests passed. These are retained reports,
not new executions or current-machine qualification. The old stackref-debug
full suite had 15 known failing files; do not turn that into a clean gate.
Between the recorded source `12619dbe13877b8b5a224fd0b1553183d4340598` and b554,
only WORKLOG/design and a Py_GC-only multiprocessing test branch changed.

Evidence is bound in `build/rc-fallback-preparation-20261101/evidence.json`
SHA `0f7ec7b42812a010c031595227a214cdde2b7808460fd3bc621b3309a588e1cb`.
It records historical source blob hashes, commits and comparison to inspected
current HEAD `5c24163405d0d70f568720a43931a1859599a260`. Selected source/build/test
directories differ 809 files; manually adding a count field to today's runtime
would not be a small coherent fallback.

The literal restoration manifest is
`build/rc-fallback-preparation-20261101/restoration-manifest.json`, SHA
`821fcbf270811ed956bfdb29cb428475271092c20e6f20ee2114a3e471d6b192`.
It identifies 840 replacements, 14 restored deleted files and 33 deletions
across the complete scope below. All 5,608 distinct historical blobs for its
5,770 ancestor paths exist locally. This proves source availability, not fresh
build success. Recompute/review the manifest against the actual decision HEAD
before any later restoration; the current manifest is read-only preparation.

## What survives, and the real tradeoff

| Area | RC successor |
| --- | --- |
| Compiler/interpreter | Fixed eight-byte compiler-produced IR; register CFG/liveness/compaction; load/store/copy/truth rewrites; call result into local; feedback vectors and generated handlers. RegCFG originated at `084e8468ed58cdfd86daccee962bdf9f758d8dfd`, already an ancestor. |
| Inline representation | Eight-byte NaN-boxed `_PyStackRef`: inline int32/non-NaN doubles, owned/borrowed pointer forms, explicit boxing. Inline values own no references. This remains a usable value representation for JIT work. |
| Shapes | Historical per-type boxed-slot shape tables and guards in pycore_dict.h/dictobject.c/specialize.c survive. Freeze that existing path enabled in the chosen GIL RC build and qualify it; header shapes and later unboxed attribute layouts do not survive. |
| Heap/native values | Restore PyObject-pointer list/tuple/dict/native ownership. Defer public core-wide PyValue, inline list/tuple storage, value-aware METH_O/FASTCALL and later value slots. These are deliberate representation/ABI losses, not simple renamed APIs. |
| JIT groundwork | IR, compiler/generator, feedback and publication/perf/unwind hooks survive. jit_publish.c is byte-identical to current. The tier 2/copy-patch JIT was already deleted at `69ffcec5a77` and tier 0 at `5e00842e99e`; this fallback does not restore a working optimizing JIT or discard an accepted current one. |

RC restores count/GC-tracking storage and owned boxes at container/native
boundaries. Memory footprint and ownership traffic may increase. No historical
speed claim predicts the successor's elapsed parity; measure its actual costs.

The incompatible later changes are explicit commits: public PyValue `98f38e1da76`,
canonical values `a495d4cbdc2`, inline containers `20693f43a82`, value protocols
`e64b29341d9`, value native calls `3832173d942` and header shapes `eb4249ad318`.
Do not cherry-pick them into the RC successor as incidental fixes. They require
their own ownership design. The JIT can first operate on logical values whose
pointer homes retain the restored strong-reference contract.

## One concrete migration, not an ineffective ancestor merge

1. Pin the decision's current mainline HEAD and protect every active tracing
   source/ref, capsule, product, user file and log. Create a **new managed
   isolated successor from that HEAD**, on a new RC branch, only after a source
   grant. Primary and current experiment worktrees stay untouched. Do not
   merge an ancestor branch and expect Git to reverse current code.
2. Make an explicit **core tree-restoration commit**. A reviewed manifest takes
   the union of current/b554 runtime paths and restores b554 blobs/modes,
   resurrects deleted files such as gc.c/stackrefs.c, and removes current-only
   runtime files. Exact directory scope is Doc, Grammar, Include, InternalDocs,
   Lib, Mac, Misc, Modules, Objects, PC, PCbuild, Parser, Platforms, Programs,
   Python, Tools and ports. Exact root-file scope is Makefile.pre.in,
   aclocal.m4, config.guess, config.sub, configure, configure.ac, install-sh and
   pyconfig.h.in. Thus matching tests, generated headers, parser, frozen code,
   build machinery and public runtime docs move together. Preserve the current
   boil coordination/docs/bench/evidence tree, AGENTS instructions and all
   user-owned paths. Current port work remains on its existing branch; this
   isolated core snapshot selects matching historical RC port sources. Review
   additions and deletions explicitly. No blanket reverse-commit list, primary
   working-tree copy, reset, history rewrite or force cleanup.
3. Close the restored collector choice in a separate focused commit: resolve
   the owning Py_GC paths false, remove --with-gc and the live tracing collector
   build path, and select the GIL RC owner plus its cyclic collector. Preserve
   RC debug/stackref checks. The historical tracing verifier/root API, if retained
   as diagnostic tooling, owns no memory and is not a second selectable
   collector. No nursery/no-nursery production switch remains. Freeze historical
   boxed shapes enabled as one source/configuration choice, not a flag search.
   They were historically optional/default-off; their presence in the tree is
   not a new qualification claim for this chosen enabled build.
4. Give this successor its own extension/pyc/build identities; rebuild CAPI
   modules and ports. Do not import boil2 tracing extensions or assume that an
   old unmarked SOABI makes fork code/frame layouts upstream-compatible. Keep
   current host measurement/reporting tooling as source, but explicitly rebind
   its image/capabilities: current tracing-root/statistics helpers cannot be
   made to pass by inventing missing APIs or zero counters.
5. Add later runtime changes only through a reviewed ownership-safe allowlist.
   Current pending region/storage designs remain documents; they are not a
   requirement to port before JIT work. In particular, today's call_effects.py
   labels DECREF/CLEAR noncollecting because RC is absent. Carrying that table
   unchanged would hide arbitrary destructor reentry in the RC successor.

## Ownership boundaries and finite gates

Restore the complete header/refcount/allocator/deallocator group, not a header
overlay. A new mortal object starts owned; zero count calls its deallocator,
trashcan and freelist logic. Frame locals/temporaries use matching DUP/CLOSE
and steal/borrow/new conversions. Inline-to-object boxing yields owned boxes
with exact cleanup on partial OOM. Container replacement releases the old
strong reference only after establishing the new safe state. Native functions
again follow their original borrowed/new/stolen PyObject APIs.

DECREF, CLEAR, tp_dealloc, weakref callbacks and finalizers can run Python,
mutate locals, resurrect objects and raise unraisable errors. Preserve original
exception/IP/frame materialization, generator teardown, buffer exporter
ownership, capsule/m_free and type lifetime behavior. Cycle collection retains
tp_traverse/tp_clear and PEP 442 handling. Old RC interpreter creation/teardown
also replaces today's GC-domain rejection, so embedding/subinterpreter behavior
needs explicit testing, not an assumption of unchanged API coverage.

Propose separate grants: 90 minutes for restoration/owner closure and source
capsule, 15-minute independent review, 45-minute fresh debug/release focused
feedback, then a 90-minute full-validation/port cohort if focused gates pass.
These are planning ceilings, not current authority or reasons to extend the
GC deadline. Prepare the source/first-feedback candidate before 15:14:44 where
scheduling permits; do not wait for the trigger to discover its buildability.

Host gates: complete path/blob manifest; generated files and magic consistent;
no enabled owning-tracer branch; ownership-aware generated output and release
call effects; new ABI/cache isolation. Native gates: tagged-values/IR/shapes,
RC/refcount CAPI tests, weakrefs/resurrection/cycles, callback-local writes,
exception/OOM cleanup, generators/coroutines/escaped frames, ctypes, buffer
exports, embedding/interpreters and native-port rebuilds. Restore RC tests
removed by the tracing transition; do not carry its requires_refcounting skips
or claim nursery tests apply to RC. First unexpected failure stops the batch
and receives a bounded reviewed correction. Full qualification precedes
promoting this successor; JIT planning need not wait for another GC redesign.

This route sacrifices later heap-value/native-ABI work to regain a coherent
ownership implementation while keeping the optimizing compiler and inline
frame representation. That is the concrete user tradeoff for unblocking JIT.
