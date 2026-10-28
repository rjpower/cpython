# Branch architecture review

Reviewed 2026-10-28 at `4bb672e0bf4870ab6dd69080772a9d88d4c8dbb4`.
Comparison base: `3a440b65695615e9fd9912342b79cdd54fced0d2`, the merge
base with local `main`. The cumulative diff touches 1,242 files, with
187,087 insertions and 161,380 deletions, including generated code,
benchmarks, ports, tests and design records. These are measures of scope,
not measures of implementation complexity.

My assessment is that the branch has a coherent destination and has built
substantial parts of it. The central idea is sound: use one value format
and one execution representation, then let the interpreter and future
JITs share them. The principal concern is the amount of unfinished
correctness and compatibility work underneath the next performance steps.
I would finish the collector's lifetime contracts and establish an
explicit compatibility contract before adding a moving heap.

This report combines three Terra reviews of execution, collection, and
objects/API with inspection of the integration tree and its records.
It is a source and architecture review. No builds, tests, benchmarks or
crash reproducers were run for it. Historical results below are attributed
to their records; they do not certify this exact head.

## What changed and how the pieces fit

| Area | Upstream model | Current branch | Remaining work |
|---|---|---|---|
| Execution | Stack bytecode, adaptive interpreter, upstream tier 2/JIT | Fixed-width register IR and typed feedback; upstream tier 2/JIT removed | New tiers 0 and 1 |
| Values | Heap objects passed as `PyObject *` | Canonical 64-bit `PyValue`, with inline int32 and non-NaN doubles throughout core storage | Remaining object slots and inline receivers |
| Lifetime | Reference counting plus cycle collection | Tracing, non-moving collection over mimalloc; release operations retain C lifetime significance | Precise-root coverage, callback ownership, generations, moving heap |
| Attributes | Managed dictionaries, split tables, type-based specialization | Shape in the object header, positional value slots, terminal dictionary mode, shape/version guards | Remaining value slot conversions and tuning |
| Extensions | Upstream object layout and C ABI | Changed layout and calling conventions; value API plus boxing compatibility functions | Settled lifetime/handle API and package ports |

### Execution

The compiler still starts with a stack-shaped control-flow graph and its
optimizations. It converts that graph into a register graph during
compilation and assembles the register instructions into the code object
([compile.c](../../Python/compile.c#L1673),
[regcfg.c](../../Python/regcfg.c)). This removes the earlier phase's
first-call lowering and the need to maintain bytecode-to-IR mappings at
runtime.

Each instruction is a 64-bit word: a 16-bit handler and four 12-bit fields
([pycore_ir.h](../../Include/internal/pycore_ir.h#L63)). Registers are
frame slots. At first execution, the runtime allocates a mutable instruction
copy and typed feedback beside it; specialization and instrumentation use
that copy ([ir.c](../../Python/ir.c#L247)). The immutable code remains the
source from which execution state can be reconstructed.

That is a useful foundation for tiering: the interpreter and compiled code
can agree about where values belong. However, the current implementation
retains `COPY`, `SWAP` and `POP_TOP` in some paths, especially around windows
and block boundaries. Note 06's as-built sections describe this more
accurately than its opening proposal
([note 06](design/06-architecture-v2.md#L1486)). The new tiers and their
deoptimization machinery are still planned, not delivered.

### Values and objects

`PyValue` makes inline numbers possible in containers, attributes, arguments
and frames, rather than merely on the evaluation stack. Removing reference
counts makes copying a value cheap and removes ownership bookkeeping from
the hot interpreter paths. This is the largest unifying change.

Shapes give the object model a corresponding structure. An instance's
header identifies its shape; the shape describes positional attributes and
their kinds. Unusual attribute behavior or capacity exhaustion can move an
instance into dictionary mode. The branch deletes split dictionary tables
rather than keeping both schemes indefinitely
([pycore_shape.h](../../Include/internal/pycore_shape.h#L169)).

The version distinction is valuable: a class mutation and an instance
layout change need not invalidate the same guards. Tests cover capacity,
polymorphic stores, generalization, lazy dictionaries and member guards
([test_shapes.py](../../Lib/test/test_shapes.py)). These are substantive
invariant tests, not just speed examples.

### Collection and C code

Tracing collection replaces reference counting, but existing C code still
holds pointers and borrowed views into object storage. The branch therefore
has a transitional lifetime system: explicit roots and precise frames where
converted, conservative scanning elsewhere, including transitive PyMem
storage scanning. Release calls act as keep-alive barriers in compatibility
code so the compiler cannot discard an owner too soon
([hack register](design/08-review-at-6a.md#L38)).

This is the hardest part of the architecture. A `PyObject *` argument that
looks live in C source is not automatically a collector root after compiler
optimization. A pointer into an object's backing allocation also need not
keep its owner alive. Precise annotations, root spans, call-effect analysis
and the kill verifier address this problem from different directions.

## What is actually landed

The integration head includes the fixed-word IR, compiler passes, tracing
collector, deleted refcounts, core/value C API, shape object protocol, 6d
rung 1, and precise-root work through 10a rung 1b. Some call-layer work has
also landed. The eval loop is now precise.

The remaining attribute/descriptor slot conversions are not present at
this head: `getattrofunc`, `setattrofunc`, `descrgetfunc` and `descrsetfunc`
still use object signatures
([object.h](../../Include/object.h#L327)). PROCESS describes 6d rung 2 as
gated and landing, which must not be read as landed. Precise-root rung 2 is
in progress, PyMem/struct work is open, and generations, the replacement
JITs, moving collection and settled package ports remain ahead.

Use [PROCESS](PROCESS.md) and the as-built work records for present state.
[PR.md](PR.md) describes Phase 1, including first-call lowering and a ports
gate that are no longer the current architecture or procedure. It should
be marked historical or replaced before it serves as a branch introduction.

## Concerns and recommendations

### 1. Callback ownership is a current correctness defect

A collection during shared-heap subinterpreter startup can execute the main
interpreter's callbacks or finalizers on the initializing interpreter.
That interpreter is not ready to handle them, and the process can crash.
The reproducer remains skipped
([test_gc.py](../../Lib/test/test_gc.py#L2040)); the work record includes
the C stack and interpreter attribution
([10a rung 1b](design/work/10a-1b.md#L957)). This is a known defect, not a
newly reproduced finding from this review.

**Recommendation:** keep promoted step 7a ahead of further collector
features. Carry callback and finalizer ownership explicitly, deliver them
on the owning interpreter, and specify behavior during owner shutdown.
Acceptance should include shared-heap startup, teardown, weakref callbacks,
`__del__`, and reentrant collection, with the excluded cases restored.

### 2. Precise roots are an unfinished system contract

The eval-loop conversion is a meaningful milestone, but remaining core
entry paths and PyMem/struct scanning mean the root transition is not
complete. Stress has also exposed a bytes-writer failure assigned to rung
3. Passing ordinary suites cannot establish that all optimized C lifetimes
are safe.

The walker is specifically enabled for x86-64 ELF with suitable glibc;
other platforms use whole-stack scanning. Even on supported hosts,
unwind failures can trigger conservative fallback
([pycore_gc_walk.h](../../Include/internal/pycore_gc_walk.h#L50),
[gc_walk.c](../../Python/gc_walk.c#L848)). Zero fallback on this machine
does not imply precise roots on other platforms or in external native code.

**Recommendation:** close 10a with an inventory of precise, opaque and
unsupported boundaries. Require the planned evidence that core PyMem and
struct scanning can be removed. Document unwind and compiler requirements
as supported-platform conditions. Define external-code roots and handles
before permitting object movement; otherwise the moving collector changes
the lifetime contract while parts of it are still implicit.

### 3. Several changes are product compatibility decisions

Canonical numeric identity is observable. Exact int32 values and non-NaN
floats use encoded identity, while separately created C boxes retain
different pointer identities. Python `is` and `id()` therefore follow a
different model, and C `Py_Is()` can disagree with Python identity
([pyvalue.h](../../Include/pyvalue.h#L35)). This deserves an explicit
language contract, including signed zero, NaNs and numeric subclasses.

The 12-bit operand format also creates a hard limit of 4,095 locals and
temporaries. Previously valid generated functions can fail to compile.
The old large-namedtuple test is skipped, while a compiler test asserts the
new failure ([test_collections.py](../../Lib/test/test_collections.py#L546),
[test_compile.py](../../Lib/test/test_compile.py#L1608)). That establishes
the chosen restriction; it does not establish compatibility.

Object-form accessors without error channels can also need allocation to
box a number and ultimately abort on allocation failure
([pyvalue.h](../../Include/pyvalue.h#L292)). Stable-ABI support is explicitly
removed. These are substantial consequences for embedders and extensions.

**Recommendation:** publish a short compatibility contract separating
intentional breaks from temporary defects. Decide whether large generated
functions need a wider encoding or an accepted limit. Explain fatal boxing
behavior and provide fallible value APIs for migration. Retain a visible
record of upstream behaviors lost; a green modified suite cannot stand in
for that record. This also matters under the repository's
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
which cautions against bypassing tests or removing desired functionality
to obtain passing results.

### 4. Extension ABI isolation is incomplete

The `boil2` SOABI marker isolates normally named extension binaries, and
removing `.abi3` loading is sensible. However, the loader still accepts
untagged `.so` files
([dynload_shlib.c](../../Python/dynload_shlib.c#L44)). Wheel ABI and platform
tags deliberately remain upstream-shaped
([configure.ac](../../configure.ac#L1421)). Untagged or renamed foreign
extensions can therefore cross a boundary with incompatible layouts and
calling conventions. SOABI filenames alone cannot validate that boundary.

**Recommendation:** before package distribution, give the fork a distinct
package compatibility identity and require an ABI-generation check at
native module initialization. Validate install/build tooling end to end.
Deferring full ports is reasonable while the API moves; a small maintained
native fixture should still exercise value returns, descriptors, ownership,
and intentional rejection of foreign binaries before the ports phase.

### 5. Address-space assumptions should be enforced

Values reserve only the bottom 48 bits for object pointers; shape pointers
are also masked to 48 bits. The implementation checks pointer alignment
but does not check the address bound at value conversion
([pyvalue.h](../../Include/pyvalue.h#L208)). A pointer outside that range
could be misclassified or truncated. This is a conditional portability
risk, not a failure demonstrated on the current host.

**Recommendation:** state the supported architecture, endian, allocator and
address-range requirements together. Reject incompatible configurations
early and enforce the pointer bound where addresses enter the value/header
representation. Include externally allocated and static objects, not only
the usual mimalloc heap, in that decision.

### 6. Performance recovery is still a hypothesis

The representation changes remove dispatch and boxing costs, but the
tracing collector adds costs elsewhere. The historical 6a review recorded
a 1.46x timing geomean and allocation-heavy regressions of roughly
1.7–2.75x ([note 08](design/08-review-at-6a.md#L144)). Those figures are
not measurements of this head. They explain why full-heap behavior and
allocation-heavy workloads must remain central to the decision.

The known `str +=` regression is more structural: losing refcount-based
uniqueness makes common append loops quadratic. Its remedy is deferred to
tier 1, with a compiler proof described as fallback. Ordinary string
building should have a credible linear path even when code is cold or
fails speculative guards.

**Recommendation:** establish a current-head baseline before the next
major performance claim. Track elapsed time, peak RSS, collection pauses
and throughput at several live-heap sizes alongside instruction counts.
Give quadratic string growth its own acceptance criterion. Do not assume
generations or a future JIT will recover every regression automatically.

### 7. Verification records need executable enforcement

The process has strong ideas: separate design and landing review, immutable
commit histories, exact-commit gate records, kill verification, stress,
traverse auditing and investigation of intermittent failures. These have
already exposed real lifetime bugs.

The scripts do not yet enforce the full written gate. Level 3 runs the
ordinary suite, C-extension checks, regeneration and traverse audit;
kill/ASan/stress and repeated probes require additional invocations
([gate.sh](tools/gate.sh#L77)). The 1b record reports stress success after
excluding `*test_resize*` and `*test_misc*`, and ASan coverage is qualified
([10a rung 1b](design/work/10a-1b.md#L1015)). A generic “gate 3 passed”
therefore conveys less than the standing process requires.

**Recommendation:** make one machine-readable verification manifest list
the required modes, exclusions, reasons, owners and removal conditions.
Generate the landing summary from results tied to source SHA, build
configuration and that manifest. Keep “ordinary suite passed” distinct
from “collector verification complete.” Restore skipped regressions as
their owning work lands. Missing build modules and unsupported platforms
should remain visible coverage gaps.

## Suggested order of work

1. Fix interpreter ownership of callbacks and finalizers; restore the
   shared-heap reproducer and relevant stress cases.
2. Finish precise-root coverage and its verification, including the
   bytes-writer/PyMem boundary; make exclusions and gate requirements
   executable.
3. Write the semantic/platform/API compatibility contract and enforce
   native ABI and pointer-range boundaries.
4. Record a current baseline for wall time, memory and pauses; settle the
   linear string-building path before relying on tier 1.
5. Continue generations and execution tiers against those contracts.
   Require explicit root and handle discipline before moving collection.

I would keep the unified values, shared register IR/feedback, and shape
protocol. The next architectural payoff should be fewer transitional
lifetime mechanisms and clearer boundaries. That gives the later JIT and
moving collector a stable substrate, and makes their benefits measurable
without masking correctness or compatibility losses.
