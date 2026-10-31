# Runtime architecture

This fork compiles Python into register IR and executes it over a single tagged
value representation. The collector traces object graphs rather than updating
reference counts. [Current work](../issues/README.md) is separate from these
implemented contracts.

## Compiler and execution

`compile.c` and `codegen.c` produce the usual stack-shaped control-flow graph.
`regcfg.c` converts it to a register graph; `regcfg_passes.c` folds loads/stores,
propagates values and compacts slots. `assemble.c` emits fixed-width IR words,
feedback initializers, line information and exception tables. There is no
runtime lowering from stack bytecode.

An IR word contains a handler ID and four physical operand fields. Registers
are frame slots, not a second operand stack. Code objects retain immutable words;
`ir.c` creates the executing copy and feedback state. Specialization and
instrumentation update that executing state under their guards.

`Python/bytecodes.c` is the operation source. The cases generators produce
dispatch bodies and handler metadata; the evaluator dispatches by handler ID.
Call, return, yield, resume and unwind preserve the current-frame, exception,
instruction-position and initialized-slot contracts.

See [compiler and IR](compiler-and-ir.md) and the
[handler contract](handler-and-observer-contract.md).

## Values and objects

`PyValue` is eight bytes: exact int32 and ordinary non-NaN doubles can be inline;
other values use an object pointer representation. Frames and managed container
fields hold values. The `PyObject *` C API remains a compatibility boundary that
can box, allocate and collect.

The object header combines shape identity and flags. Shapes describe positional
attribute storage and its guards. Materializing a dictionary changes the storage
mode; type versions, descriptors and custom lookup retain their semantics.
Representation promises do not replace owner/type/version guards or generic
fallbacks. See [values and object protocol](values-and-object-protocol.md).

## Memory and native code

Mimalloc storage and the authoritative object-start map support full-heap
mark/sweep. Roots are precise for declared frame/value/C-span contracts and
conservative for supported opaque native storage. The collector preserves
weakref, finalization, resurrection, ordering and resource-release behavior.
Callback delivery uses one rooted drainer in the supported interpreter domain.

The supported configuration is Linux, one main interpreter, ordinary threads
and the GIL. Unsupported interpreter creation rejects explicitly. Foreign
adoption and broader lifecycle guarantees are outside this contract.

No nursery, relocation or write-barrier protocol is enabled by this description.
See [GC roots and lifetimes](gc-roots-and-lifetimes.md) and
[performance direction](gc-and-interpreter-performance.md).

## Subsystem ownership

Share facts with their actual consumers: generated operation/effect metadata,
container/frame field ranges and space/lifecycle interfaces. Managed scanners
and generic traversal must agree on strong fields and initialized bounds.
Native traversal remains a compatibility adapter. New writable-slot, pinning
or age contracts must not be inferred from read-only mark coverage.
