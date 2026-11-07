# Interpreter design and observer contract

The compiler emits register IR; execution does not translate stack bytecode at
runtime. Code objects keep pristine words, while `Python/ir.c` supplies the
mutable executing copy and feedback. [Compiler/IR details](compiler-and-ir.md)
describe lowering, liveness and physical frame homes.

`Python/bytecodes.c` defines operations and effects. The cases generators emit
handlers and metadata; `Python/ceval.c` dispatches them. One generated handler
domain serves execution, specialization, instrumentation, statistics and readers.
Change producers and consumers together; guessed opcode ranges are invalid.

## Dispatch and state

The register interpreter uses generated tail-call handlers under GCC. Generated C
does not explicitly control hardware register assignments as LuaJIT's assembly
does. Evaluate emitted code, dispatch, slot traffic, code density and elapsed
performance rather than assuming the IR or C compiler gets them right.

Handlers consume initialized operands and publish outputs, frame/PC and live
homes on success and failure. Calls, returns, yield/resume, unwind and redispatch
use the actual current frame. Thread-owned call windows may share frame storage;
generator-owned frames have separate storage. Both obey the same value/ownership
contract. [Values](values-and-object-protocol.md) explains the restoration target.
The [call/frame lifetime contract](call-and-frame-lifetimes.md) distinguishes
initialized homes, owned references and future-read liveness, and defines the
prepare/guard/commit boundary for overlapping calls.

## Reentry and observation

Allocation, boxing, object release and opaque calls can run callbacks. Capture
owning inputs before them. Stage a result, publish actual caller state, release
retired owners in semantic order, then revalidate frame, instruction and observer
state before decoding again. An old decoded pointer is not valid merely because
the result has been published. Deallocators preserve the entry exception.

`IR_RETIRE_PUBLISHED` and its pair form own input consumption and result
publication before receiver/displaced-owner release. Specialized attribute
loads do not issue a separate `SYNC_SP` immediately before them. The macro
keeps a folded-local result in C until retirement and preserves the final
destination write. Both retirement markers imply synchronization in generated
analysis metadata. This local contract replaces remembering publication state
across generator branches or callbacks; actual reentry still requires refresh.
See the [source review](../issues/interpreter-throughput/publication-review.md).

For the five specialized attribute-load paths, the generator also has a narrow
postcondition after receiver retirement: the unchanged result remains published
to its temporary home. Integration C reuses the existing
`published_temporary_result` fact to omit only the final duplicate temporary
store. The folded-local path still writes the result after retirement, when
replacing the old local can release its owner. This fact is specific to those
reviewed flows and does not authorize carrying publication knowledge across a
later reassignment, home change, or callback. See the
[final-publication review](../issues/interpreter-throughput/final-publication-review.md)
and [Integration C receipt](../issues/interpreter-throughput/integration-c.md).

Specialization must guard receiver behavior, representation, shape/type version
and descriptor precedence as applicable. Keep generic behavior and recovery for
hooks, mutation, misses and exceptions. Selected noallocation nomination paths
must have a real transitive proof; do not infer purity from an operation name.

Monitoring observes the current executing handler and source position. Preserve
BEFORE/AFTER events and event-poll behavior through redispatch. Instrumentation,
specialization and cleanup can invalidate cached facts. The same rules must
hold in generic, specialized and native paths.

Current restoration has reviewed components for attributes and selected native
callers, with wider iterator, CALL and parser coverage still unfinished. See the
[board](../issues/README.md). Root-map and backend proposals are evidence to
assess, not prerequisites imported from the retired tracing collector.
