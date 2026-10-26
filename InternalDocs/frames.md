# Frames

Each call to a Python function has an activation record, commonly known as a
"frame". It contains information about the function being executed, consisting
of two parts:

* The header: the per-frame object references needed by the VM, including
  globals dict, code object, instruction pointer, the previous frame, etc.
* The slots: the local variables (including arguments, cells and free
  variables), then the temporaries, which hold the values of expressions
  being evaluated.  The instructions of the register IR name slots by
  number (see [Include/internal/pycore_ir.h](../Include/internal/pycore_ir.h)).

A frame has `co_nlocalsplus + co_stacksize` slots, a fixed number.  There is
no stack pointer: every slot holds a value or NULL.  A temporary that no
instruction has written yet, or that an instruction consumed, is NULL, so
the code that walks a frame (clearing it, the garbage collectors, `f_locals`,
setting `f_lineno`) visits every slot and skips NULL.

The definition of the `_PyInterpreterFrame` struct is in
[Include/internal/pycore_interpframe_structs.h](../Include/internal/pycore_interpframe_structs.h).

# Allocation

Python semantics allows frames to outlive the activation, so they need to
be allocated outside the C call stack. To reduce overhead and improve locality
of reference, most frames are allocated in two per-thread stacks: the frame
stack holds their headers and the value stack holds their slots, and
`frame->base` points to slot 0 (see `_PyThreadState_PushFrame` in
[Python/pystate.c](../Python/pystate.c)).

Frames of generators and coroutines are embedded in the generator and coroutine
objects, with their slots, so are not allocated in the per-thread stacks. See
`_PyGenObject` in
[Include/internal/pycore_interpframe_structs.h](../Include/internal/pycore_interpframe_structs.h).
A frame that keeps its slots, as these do, has them in `owned_slots`, and
`base` points there.

## Layout

The two stacks share the chunks of the thread's data stack.  The value
stack grows from the start of a chunk, up to `tstate->datastack_top`, and
the frame stack grows from its end, down to `tstate->datastack_limit`, so
that one test, `_PyThreadState_HasStackSpace()`, checks the room for both:

```
  chunk:  | slots of A | slots of B ... |    free    | header B | header A |
          ^ data[0]                     ^ top        ^ limit
```

When a call has no room, both the header and the slots go to a new chunk.
A header records the top of the value stack before it was pushed,
`saved_top`, which popping the frame restores.

#### Overlapping frames

The caller evaluates the callable into slot `c`, `self` or NULL into
`c + 1`, and the arguments into the slots after.  `CALL_PY_EXACT_ARGS`
pushes the callee with its base at the first argument, `c + 2`, or at
`self`, `c + 1`: the arguments are already where the callee's parameters
go, and nothing is copied (`_PyFrame_PushOverlapping()`).  This is Lua's
convention, and the layout that part of 3.11 alpha used.  The slots of
the caller above the call's window are dead temporaries, NULL, which the
callee uses and leaves NULL.  The caller's slots must be on the value stack:
a generator's caller copies, as do calls with keywords or defaults.

This needs the slots above a call's window to be dead, as they are when
temporaries are numbered by stack depth.

### The header

The header contains the following pointers:

* Globals dict
* Builtins dict
* Locals dict (not the "fast" locals, but the locals for eval and class creation)
* Code object
* Heap allocated `PyFrameObject` for this activation record, if any.
* The function.

The pointer to the function is not strictly required, but it is cheaper to
store a strong reference to the function and borrowed references to the globals
and builtins, than strong references to both globals and builtins.

### Frame objects

When creating a backtrace or when calling `sys._getframe()` the frame becomes
visible to Python code. When this happens a new `PyFrameObject` is created
and a strong reference to it is placed in the `frame_obj` field of the header
section. The `frame_obj` field is initially `NULL`.

The `PyFrameObject` may outlive a stack-allocated `_PyInterpreterFrame`.
If it does then `_PyInterpreterFrame` is copied into the `PyFrameObject`,
whose temporaries are NULL at this point.
The previous frame link is updated to reflect the new location of the frame.

This mechanism provides the appearance of persistent, heap-allocated
frames for each activation, but with low runtime overhead.

### Generators and Coroutines

Generators (objects of type `PyGen_Type`, `PyCoro_Type` or
`PyAsyncGen_Type`) have a `_PyInterpreterFrame` embedded in them, so
that they can be created with a single memory allocation.
When such an embedded frame is iterated or awaited, it can be linked with
frames on the per-thread stack via the linkage fields.

If a frame object associated with a generator outlives the generator, then
the embedded `_PyInterpreterFrame` is copied into the frame object (see
`take_ownership()` in [Python/frame.c](../Python/frame.c)).  The copy,
`_PyFrame_Copy()`, moves the slots and leaves NULL behind, because the slots
of a frame on the value stack may be its caller's.

### Field names

Many of the fields in `_PyInterpreterFrame` were copied from the 3.10 `PyFrameObject`.
Thus, some of the field names may be a bit misleading.

For example the `f_globals` field has a `f_` prefix implying it belongs to the
`PyFrameObject` struct, although it belongs to the `_PyInterpreterFrame` struct.
We may rationalize this naming scheme for a later version.


### Shim frames

On entry to `_PyEval_EvalFrameDefault()` a shim `_PyInterpreterFrame` is pushed.
This frame is stored on the C stack, and popped when `_PyEval_EvalFrameDefault()`
returns. This extra frame is inserted so that `RETURN_VALUE`, `YIELD_VALUE`, and
`RETURN_GENERATOR` do not need to check whether the current frame is the entry frame.
The shim frame points to a special code object containing the `INTERPRETER_EXIT`
instruction which cleans up the shim frame and returns.


### Base frame

Each thread state contains an embedded `_PyInterpreterFrame` called the "base frame"
that serves as a sentinel at the bottom of the frame stack. This frame is allocated
in `_PyThreadStateImpl` (the internal extension of `PyThreadState`) and initialized
when the thread state is created. The `owner` field is set to `FRAME_OWNED_BY_INTERPRETER`.

External profilers and sampling tools can validate that they have successfully unwound
the complete call stack by checking that the frame chain terminates at the base frame.
The `PyThreadState.base_frame` pointer provides the expected address to compare against.
If a stack walk doesn't reach this frame, the sample is incomplete (possibly due to a
race condition) and should be discarded.

The base frame is embedded in `_PyThreadStateImpl` rather than `PyThreadState` because
`_PyInterpreterFrame` is defined in internal headers that cannot be exposed in the
public API. A pointer (`PyThreadState.base_frame`) is provided for profilers to access
the address without needing internal headers.

See the initialization in `new_threadstate()` in [Python/pystate.c](../Python/pystate.c).

#### How profilers should use the base frame

External profilers should read `tstate->base_frame` before walking the stack, then
walk from `tstate->current_frame` following `frame->previous` pointers until reaching
a frame with `owner == FRAME_OWNED_BY_INTERPRETER`. After the walk, verify that the
last frame address matches `base_frame`. If not, discard the sample as incomplete
since the frame chain may have been in an inconsistent state due to concurrent updates.


### Remote Profiling Frame Cache

The `last_profiled_frame` and `last_profiled_frame_seq` fields in
`PyThreadState` support an optimization for remote profilers that sample call
stacks from external processes. When a remote profiler reads the call stack, it
writes the current frame address to `last_profiled_frame`. The eval loop then
keeps this pointer valid by updating it to the parent frame whenever a frame
returns (in `_PyEval_FrameClearAndPop`) and increments the sequence.

This creates a "high-water mark" that always points to a frame still on the stack.
On subsequent samples, the profiler can walk from `current_frame` until it reaches
`last_profiled_frame`, then validate the pointer and sequence before using cached
callers. This prevents a later frame that reuses the same `_PyInterpreterFrame`
address from being mistaken for the sampled frame. The cache significantly
reduces the amount of remote memory reads needed when call stacks are deep and
stable at their base.

The update in `_PyEval_FrameClearAndPop` is guarded: it only advances the
pointer and sequence when `last_profiled_frame` is non-NULL AND matches the
frame being popped. This prevents transient frames (called and returned between
profiler samples) from corrupting the cache anchor, while avoiding any overhead
when profiling is inactive.


### The Instruction Pointer

When a frame is executing, `instr_ptr` points to the instruction currently being
executed. In a suspended frame, it points to the instruction that would execute
if the frame were to resume. After `frame.f_lineno` is set, `instr_ptr` points to
the next instruction to be executed. During a call to a python function,
`instr_ptr` points to the call instruction, because this is what we would expect
to see in an exception traceback.

A return or a yield resumes the caller at the word after `instr_ptr`.  When a
generator returns to the `SEND` or `FOR_ITER` that resumed it, the caller
goes on to that instruction's exit instead, which
`_PyEval_GeneratorReturnTarget()` finds from the caller's word.  The entry
frame and the `__init__` shim keep `instr_ptr` on a `NOP` before the word that
a return reaches.

The value that comes back goes to the caller's slot `return_slot`: the dst of
the call, which the calling instruction sets.  For a suspended generator,
`return_slot` names the slot of the value it yielded, where the value sent in
goes.

### RESUME

A frame that a call pushes, that the interpreter enters, or that a generator
resumes starts after its `RESUME` when that would do nothing: the eval
breaker has nothing pending and the code's instrumentation is current
(`IR_SKIP_RESUME` in [Python/ceval_macros.h](../Python/ceval_macros.h)).
Otherwise `RESUME` runs, and it does after the instructions that make cells
and copy free variables, and at a generator's first start.
