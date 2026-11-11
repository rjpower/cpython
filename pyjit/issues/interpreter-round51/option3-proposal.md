# Option 3: one exact-call reservation and initialization

Status: proposed; source unchanged. Runtime subtree:
`BASE/worktrees/round51-option3`, based on `bf810ec1a33`. Implementation
is bounded to 23:30 UTC on 2026-11-11; coordinator owns builds and measurement.

Selected release `_TAIL_CALL_CALL_PY_EXACT_ARGS` occupies 0x582 bytes at
0x37a740 and reserves 0x50 C-stack bytes. Read-only objdump confirms two
header/constant initialization regions (0x37a938 and 0x37aab9), separate
frontier-growth memset arms, and argument-copy/clear work for non-thread
callers. Function/code conversions already collapse; typed-pointer threading
alone is not a performance mechanism.

Refactor exact activation into three phases: select final homes, reserve their
initialized storage, then initialize one header and commit owners. Extract the
existing overlapping reservation into an always-inline helper taking selected
code, base, argument count and previous frame. Its debug checks preserve the
existing caller-tail NULL proof for thread owners. For a non-thread caller the
selected base is the thread value-stack top, and no heap caller addresses enter
thread-stack arithmetic. The helper saves the original thread top, reserves
through max(base + callee slots, saved top), truncates the frontier before the
header, commits the same header/value pointers, and returns the header.
Existing `_PyFrame_PushOverlapping` uses this helper and retains its public
signature and always-inline binder policy. Ordinary non-call frame push stays
unchanged.

The exact uop selects base by caller ownership, calls this reservation helper,
and invokes `_PyFrame_Initialize` once with the already selected function/code.
It then moves arguments only for non-thread callers. Move that cold copy/clear
body into one explicitly non-inlined ceval helper if release native confirms
the anticipated live-state reduction. The helper receives the destination base,
source window and explicit argument count, derives self from the original
window, preserves constant slots for zero-argument no-self calls, and clears
only transferred source homes. Callable transfers to f_funcobj and is cleared
by the common commit. No callback or fallible operation occurs in these phases.
Code acquires its separate owner during initialization; function code is never
reselected by an outlined helper.

Move only actual initialized-frontier suffix filling into an out-of-line
internal helper in pystate.c. Keep truncation and the already-initialized test
inline in `_PyStackChunk_EnsureFrameStorage`. The outlined helper fills NULL
from the current frontier to value_end and advances that frontier. It allocates
nothing, retires no owners and invokes no callback. Every reservation caller
keeps capacity admission before this function and header writes after it.
No saved frontier is added or restored on pop.

Files: `Include/internal/pycore_interpframe.h`, `Python/pystate.c`, the exact
initialization uop in `Python/bytecodes.c`, its bounded transfer helper in
`Python/ceval.c`, and focused regressions in `Lib/test/test_call.py`.
Coordinator regenerates products. No new evaluator, frame ABI, owner ledger,
operand format, source-map dependency, fusion, leaf mode or alternate config.
PEP 523, DTrace, debug entry, recursion admission, return recipient publication
and generator caller fallback retain their current paths.

Validation: warm ordinary/self exact calls and generator-owned callers, including
zero-argument callees with constant homes; exercise deep alternating narrow/wide
frames, chunk growth, frontier reuse and truncation, mortal replaced constants,
frame escape, exception unwinding and finalizer reentry. Reuse existing exact
CALL/monitoring/frame tests and add only gaps. Recommended minimal candidate
modules: test_pyvalue, test_call, test_generators, test_coroutines, test_monitoring,
test_frame and test_sys (recursion); release/debug focused selection follows the
coordinator's runner. Native success requires one initialization region and
fewer ordinary-path instructions/spills; smaller symbol size alone is not
admission. Inspect common path for newly introduced helper calls. Coordinator
screens DeltaBlue, richards, coroutines and logging, then applies round-51
matched measurement gates. Reject or cut back if GCC duplicates initialization
again or helper routing taxes the ordinary path.
