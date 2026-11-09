# Interpreter parity: two independent implementation increments

2026-11-08. Runtime base `0953757f4e7`; proposal pending independent review.
Runtime worktree: `python-boil-the-ocean/build/worktrees/interpreter-parity-20261108`.
No runtime changes are authorized until the coordinator or design reviewer approves.

## P1: successful call refresh

In `Python/ceval.c:call_transaction_refresh`, return 0 immediately when
`value_operation_refresh(..., PyValue_NULL)` succeeds. Leave its existing
`value_frame_is_current` check and `value_revalidation_error` on failure and
return the original negative status otherwise. On success,
`value_operation_refresh` already checked `tstate->current_frame == frame`,
non-NULL `_co_exec`, and `frame->ir_words == exec->words`, and no operation
between the checks can escape or change these facts. It also validates the
instruction range and updates `instr_ptr`. The second check duplicates a
subset. On failure, preserve the existing second check: it can wrap a pending
exception with the call-preparation SystemError, including its cause; the
executing-words error and all other failure behavior remain unchanged. No
call route, owner transfer, guard, instrumentation or error ordering changes.

Focused validation: existing call/argument, exception, tracing/monitoring and
frame tests in release and debug; exercise allocating `**kwargs`, bound method
expansion, `*args` conversion and error cases. Invalid current-frame or code
state is not reachable from ordinary Python; do not claim those failure branches
are tested without a deliberate internal fault-injection test. Review the
actual source diff and optimized output to establish the success check removed.

## P2: canonical known-code frame executable

Both `_PyFrame_Initialize` and `_PyFrame_PushTrampolineUnchecked` currently
use `PyValue_DUP(PyValue_FromObjectBorrow(code))`. `code` is a non-NULL
`PyCodeObject *`; code objects are neither exact int nor exact float, so the
numeric canonicalization checks in the generic constructor cannot succeed.
The normal initializer covers regular, overlap, generator and explicit frame
construction. The trampoline initializer is separate. Sentinel `PyValue_None`
assignments, frame owner moves and copies of existing executable values are
not constructors from a code pointer and must stay as they are.

Preferred design: factor the existing final pointer/tag construction of
`PyValue_FromObjectBorrow` into a non-NULL internal pointer borrow primitive.
Its domain includes boxed exact numbers that generic canonicalization has
already decided cannot be inline, as well as known nonnumeric objects; it
asserts pointer range and returns the same immortal-tagged or mortal plain
pointer without taking an owner. The generic constructor calls this primitive
only after its existing NULL and numeric handling, keeping inline scalar and
`OBJECT_STAT_INC` behavior intact. A separate known-code frame wrapper takes
`PyCodeObject *`, asserts non-NULL and `PyCode_Check`, and calls the shared
pointer primitive followed by `PyValue_DUP`. The typed wrapper proves that the
pointer is a canonical home: code is never an exact numeric type. `PyValue_DUP`
remains the shared count acquisition: one mortal reference, none for an
immortal. No `_PyValue_PointerSteal` of a borrowed pointer. Review macro
expansions, assertions, debug and free-threaded builds, and verify no numeric
tests remain in optimized frame initialization.

Alternative: a typed frame-only helper with its own pointer/tag construction
would reduce API surface but duplicate the ordinary constructor's immortal tag
logic; reject unless the factored generic helper creates a material problem.
An owning known-code helper built using `Py_NewRef` and `_PyValue_PointerSteal`
would require a separate ownership proof and is not preferred.

Add focused ownership tests using `code.replace()` to produce mortal code:
observe code refcounts or weakref lifetime while normal, overlapping and
suspended generator frames exist and after cleanup, and code replacement during
an active frame. Exercise class construction with a Python `__init__` to reach
the call trampoline, and a `context.run` call, including exception cleanup.
Use an immortal-code test only where the runtime offers a reliable way to
obtain and verify one; otherwise test the helper directly in the existing
internal C test infrastructure with explicitly immortalized and mortal code,
checking tag, refcounts and release. Ensure lifetime tests distinguish a
function's code owner from the additional live-frame owner. Existing
`test_code` constant-slot tests cover replaced mortal constants but do not
alone prove code-object ownership. Coordinator runs focused release/debug tests
and a stats build, then reviews generated code and measurement independently.

P1 and P2 are independently reviewable. P2 follows P1's coordinator build,
tests and basic measurement. P3's specialized call preparation is separate:
no handler-specific table, duplicated call ABI, changed final guards or
premature transfer of owners is part of either increment.
