# Python VM State

The VM is the classic Python bytecode interpreter, including the specializing adaptive interpreter described in [PEP 659](https://peps.python.org/pep-0659/) and introduced in Python 3.11.

# Frame state

Almost all interpreter state is nominally stored in the frame structure.
A pointer to the current frame is held in `frame`. It contains:

- **local variables** (a.k.a. "fast locals")
- **evaluation stack** (tacked onto the end of the locals)
- **stack top** (an integer giving the top of the evaluation stack)
- **instruction pointer**
- **code object**, which holds things like the array of instructions, lists of constants and names referenced by certain instructions, the exception handling table, and the table that translates instruction offsets to line numbers
- **return offset**, only relevant during calls, telling the interpreter where to return

There are some other fields in the frame structure of less importance; notably frames are linked together in a singly-linked list via the `previous` pointer, pointing from callee to caller.
The frame also holds a pointer to the current function, globals, builtins, and the locals converted to dict (used to support the `locals()` built-in).

## Fast locals and evaluation stack

The frame contains a single array of object pointers, `localsplus`, which contains both the fast locals and the stack.
The top of the stack, including the locals, is indicated by `stacktop`.
For example, in a function with three locals, if the stack contains one value, `frame->stacktop == 4`.

The interpreter uses the same memory but caches the depth (as a pointer) in a C local, `stack_pointer`.

## Instruction pointer

The canonical, in-memory, representation of the instruction pointer is `frame->instr_ptr`.
It always points to an instruction in the bytecode array of the frame's code object.
Dispatching on `frame->instr_ptr` would be very inefficient, so the interpreter caches the upcoming value of `frame->instr_ptr` in the C local `next_instr`.

## Unwinding

Unwinding uses exception tables to find the next point at which normal execution can occur, or fail if there are no exception handlers.
During unwinding both the stack and the instruction pointer should be in their canonical, in-memory representation.

# Thread state and interpreter state

Another important piece of VM state is the **thread state**, held in `tstate`.
The current frame pointer, `frame`, is always equal to `tstate->current_frame`.
The thread state also holds the exception state (`tstate->exc_info`) and
recursion tracking data (`tstate->py_recursion_remaining`, `tstate->c_stack*`).

The thread state is also used to access the **interpreter state** (`tstate->interp`), which is important since the "eval breaker" flags are stored there (`tstate->interp->ceval.eval_breaker`, an "atomic" variable), as well as the "PEP 523 function" (`tstate->interp->eval_frame`).
Note that the eval breaker may be moved to the thread state soon as part of the multicore (PEP 703) work.
