# Runtime architecture

The accepted runtime compiles Python into register IR, executes generated C
handlers over eight-byte NaN-boxed frame values, and uses reference counting
with cyclic collection. The accepted images were built from `eba351fee7d`.
[Current work](../issues/README.md) distinguishes implemented contracts from
proposals and isolated experiments.

## Compiler and execution

`compile.c` and `codegen.c` produce stack-shaped control-flow graphs. `regcfg.c`
converts them to register operations; `regcfg_passes.c` folds loads/stores,
propagates values, computes liveness and compacts frame slots. `assemble.c`
emits fixed-width IR words, feedback initializers, line and exception tables.
There is no runtime conversion from stack bytecode.

An IR word contains a handler ID and four physical operand fields. Executable
IR registers name frame homes. They are distinct from hardware registers and
from a proposed optimizing compiler's SSA values. RegCFG has fresh temporaries
within regions and canonical homes at joins; it is not a general SSA optimizer.

Code objects retain pristine words. `Python/ir.c` creates the executing copy
and feedback state, which specialization and instrumentation update.
`Python/bytecodes.c` defines operations. The cases generators produce handler
bodies and metadata; `Python/ceval.c` runs them. Calls, returns, exceptions,
yield/resume and observation obey real frame and initialized-home contracts.

## Values, ownership and memory

`Include/internal/pycore_stackref.h` defines `_PyStackRef`. On the supported
Linux/GIL builds, exact int32 values and non-NaN doubles may be inline in an
eight-byte frame value. Other values retain object references and ownership.
Heap container fields and shape slots remain boxed; a core-wide inline heap
representation and its ABI were removed during RC restoration. The user has
reaffirmed that [universal PyValues must be restored under RC](../issues/pyvalue-rc-restoration/status.md).
The boxed heap is an interim implementation, not the selected final design.

Borrowing, stealing, duplicating and closing a reference are semantic operations.
Closing a mortal object can run a destructor and reenter Python. Boxing at a C
API boundary can allocate and fail. A native compiler must model these effects
and preserve observable ownership, identity, exceptions and frame state.

The object allocator and ordinary cyclic collector remain in use. Tracing-GC
optimization and nursery work are closed; no collector selector is planned.
`Python/gc_trace.c` remains as an RC tracing verifier, not the selected freeing
collector. Its diagnostic presence does not imply a tracing-GC runtime.

## Native compilation

The accepted main images contain no new native tier. Three isolated frame-entry
JIT experiments were rejected on elapsed results. A subsequent arithmetic-block
source candidate passed source review but was superseded before activation;
its source and helper evidence are retained as components.

The active [method and tracing JIT design](jit-e2e-system.md) proposes a baseline
whole-method compiler and a shared optimizing SSA/backend pipeline. Method
compilation and trace recording are different region builders over common
semantic, state-recovery, ownership and native-code lifetime contracts. These
are proposed contracts, not claims of implemented coverage or speed.

## Current limits

RC integration qualified Linux x86-64/GIL, GCC computed dispatch, boxed-slot
shapes and the private `boilrc1` ABI. Global `Py_STACKREF_DEBUG` remains unsupported
and unvalidated. See [RC integration](../issues/rc-integration/status.md) for
profile, optional-module and qualification limits. Historical tracing/value
papers describe earlier source and must not be used as the current ABI.
