# Broad qualification of the selected RC core

2026-11-01. Coordinator proposal against clean source
`d7df12b1d81722927e71d9734c70eeafdccfee73`. This plan is not an execution
grant. The collector decision is closed; qualify one RC/cyclic-collection
runtime without another bespoke ownership cohort or a collector selector.

## Supported scope and carried evidence

The landing scope is Linux/x86-64/GIL, GCC computed dispatch, boxed-slot shapes
and the selected eight-byte frame representation. Public container/native
ownership is restored RC; later core-wide inline heap values are deferred.
Windows, free-threading and arbitrary old extension binaries are not qualified
by this plan. The eight existing port recipes remain recorded; their rebuilds
are required before claiming their support or using them as JIT consumers.
Do not turn unspecified ecosystem ports into an unbounded core landing gate.

Carry the exact initial39 methods per image, compatibility50/48 and final
focused35/35 plus standard eight-case `-R4:5` through their independent reviews.
The last batch remains failed in host validation; its independently accepted
additive reader supports the retained observations without a native rerun.
Carry exact canonical Autoconf reproduction; it closes configure/autoheader
generation only, not cases/Clinic regeneration or complete runtime coverage.

All rows bind the same source, actual compiler/configuration, own extension
origins and product inventories. Reuse existing debug/release products while
their inputs stay exact. Native jobs, full suites and perf-map users serialize;
source/design review can overlap ordinary tests. No native timing occurs here.

## Finite rows, ordered to expose failures early

| Row | Source/image and command body | Run ceiling; cleanup reserve |
| --- | --- | --- |
| Release full | Existing release; [first-row plan](release-full-row-plan-20261101.md), complete default suite, `-q --timeout 900 -j16`, explicit seed0/order and only the existing remote-exec exclusion | 3600s;210s |
| O1/debug full | Existing debug; complete default suite, `-q --timeout 1200 -j16 --randseed 0 --no-randomize`, same sole exclusion | 5400s;210s |
| C/C++ extension closure | Existing debug; `-m test -q -u cpu --timeout 900 --randseed 0 --no-randomize test_cext` | 1200s;210s |
| Generated-source closure | Existing debug; unchanged `make -s regen-cases clinic`, compare exact tracked bytes/status before/after | 1200s;210s |
| ASan build and selection | Fresh source-local `build/rc-asan`; existing O1/debug GCC computed/shapes/mimalloc configure policy plus `--with-address-sanitizer`; existing16-module selection below | Build1200s, tests2400s;210s each |
| Native stackref checker | Fresh `build/rc-stackref-debug`; existing O1/debug policy plus `-DPy_STACKREF_DEBUG`, then the complete default debug suite | Build1200s, suite5400s;210s each |
| Source closure | Actual RC traversal report plus independent cyclic-ownership disposition; applicable existing unbuilt-module syntax checks with available pinned headers | Each600s;210s |

Each activation freezes exact literal argv, effective flags, helpers, required
ordinary nonzero tests/module results, expected platform/resource skips and
finite inclusive deadlines before target execution. Timeout ceilings only
decrease to leave cleanup time. Stop the batch at its first failure; preserve
partial results and investigate the concrete fault. No automatic rerun or
broadened exclusion. A full-suite failure may require a focused reproducer;
that is a named correction, not another speculative cohort ladder.

The existing machine-local exclusion is exactly
`test_remote_exec_deleted_static_executable`, whose timeout predates the fork.
Standard skips and environment-change results stay visible. Do not copy the
old tracing runtime's six limited-API skips: RC supports those header forms.
`test_cext` has16 source methods; the Linux C++14 case has its existing MSVC
decorator. Actual results, local pip/setuptools availability and fresh own-ABI
extension origins decide coverage; source expectations do not supply a pass.

## ASan and stack-reference profiles

The existing ASan16 selection is `test_gc`, `test_weakref`, `test_generators`,
`test_coroutines`, `test_asyncgen`, `test_descr`, `test_capi`, `test_sort`,
`test_functools`, `test_context`, `test_ctypes`, `test_multiprocessing_fork`,
`test_multiprocessing_forkserver`, `test_multiprocessing_spawn`, `test_set` and
`test_memoryview`. Retain only the applicable remote-exec exclusion. The old
`test_buffer_overflow` exclusion relied on tracing-GC allocator behavior that
RC no longer has; do not carry it forward silently.

Selected RC `pycore_pymem_init.h` switches default allocator domains to system
malloc under `_Py_ADDRESS_SANITIZER`, even with mimalloc compiled in. RC also
honors `PYTHONMALLOC=malloc`. Keep the normal configure policy; do not invent a
second allocator variant. Actual build flags, ASan linkage and the standard
sanitizer subprocess cases must demonstrate the instrumented path.

Freeze `ASAN_OPTIONS=detect_stack_use_after_return=0:detect_leaks=0:halt_on_error=1`.
This is bounded heap/address/UAF coverage: sanitizer errors fail the row, while
fake-stack and LeakSanitizer coverage are disabled. Their absence is explicit,
without relying on the removed owning conservative collector as a reason.
No leak/fake-stack coverage may be claimed from this run. Refleak evidence
remains a separate standard result.

`Py_DEBUG` implies `Py_REF_DEBUG`, not `Py_STACKREF_DEBUG`. The latter is enabled
by a CFLAGS definition and has table-backed pointer IDs in the same eight-byte
layout while preserving inline scalar forms. Freeze the exact all-unit flag
and compile/symbol witness before running its full suite. A checker-profile
pass cannot be relabeled as ordinary pointer-layout performance evidence.
No new configure/runtime option is needed.

## Collector-specific checks and source closure

The tracing owner, precise-root KILL modes, nursery/minor queue checks and
`PYTHON_GCSTRESS=1000` are removed or inapplicable to this RC runtime. Do not
export those variables or call missing getters. Historical `gc_verify_count`
and holes diagnostics are not owning collector counters; absence of rootstats
does not imply zero fallback. Ordinary RC ownership/cycles, standard debug
full-suite/refleak, ASan and actual stackref-debug behavior cover this landing.
Compiler/backend alternatives were rejected; no Clang qualification sweep is
owed for the explicitly GCC-only selected foundation.

The restored traversal tool accepts `--root`/`--missing-only`, not `--check`,
and returns0 after a heuristic report. It models tracing ownership of all
types and excludes shapes in its heuristic preprocessing. Run its actual CLI
as source evidence, then review relevant cycle-tracked RC types and helper
delegation against the real enabled shapes; neither exit0 nor every reported
non-GC owner is an RC completeness verdict. Resolve concrete missing cyclic
edges before landing. Do not add another generalized audit framework.

Reuse `boil/tools/check_unbuilt.sh --src RC_TREE RC_DEBUG` for its existing
borrowed-header scope when those headers exist, binding exact compiler/header
origins. Record unsupported modules explicitly. No syntax-only result proves
an unbuilt module links or works, and no absent header becomes a fabricated
pass. Frozen source facts and independent review must settle actual scope
before granting these closure rows.

Bound source facts are in `build/rc-broad-profile-facts-20261101-v1/facts.md`,
SHA256 `ff3aa174199dddba95094c83adb307c76df2389780a38c8bc8f3c6e362a56240`.
The plan chooses the full default stackref-debug suite rather than that facts
note's optional13-module proposal; no additional bespoke cohort is scheduled.

## Landing and first JIT increment

After actual broad results and source/closure review, independently review the
landing diff and enumerate carried versus refreshed evidence. Preserve the
five protected user files. Merge locally without remote push or force cleanup;
prove all non-project-prose source bytes match the qualified RC source and run
one fresh release and one debug integration smoke on the exact bridge. A core,
test or tool difference needs affected checks, not a prose-only bridge claim.

Only then change the main README's current-runtime description to RC. Keep the
unchanged tracing sources, failed attempts and accepted evidence available.
The first native JIT [design](../jit-first-native-tier/design.md) proceeds
through independent review in parallel; implementation receives a separate
finite grant after critical RC qualification, and elapsed decides adoption.
