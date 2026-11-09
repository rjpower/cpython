# One interpreter queue drainer

Source checkpoint from primary ff6ed9d6bfbe; native validation is pending
on the composed main-only/buffer/drainer candidate. This is a main-only,
shared-GIL increment, not completion-graph or foreign-owner support.

One interpreter claim covers the whole drain, including yielded callbacks
and unraisable reporting. Other collectors and nested calls collect and
enqueue, then return without waiting. FIFO selection and signal precedence
stay unchanged. An explicit three-slot value span anchors the saved raised
exception, active object and callback before their queue node is removed.
The original opaque drainer classification remains.

Busy main-thread finalization returns -1 with the runtime initialized.
Entry and final stopped-world checks prevent teardown beneath a callback;
late rejection unwinds HEAD/world/pending locks and restores the original
main-interpreter attachment and finalizing flag. Already-run preparation
effects remain. Late foreign-entry or non-main-thread busy is fatal, as
are void finalize/end-interpreter misuse and the defensive collector-close
guard. Py_RunMain returns120 without clearing the live runtime's globals.

The private exported int _PyOS_BeforeFork is shared by os's three fork
entry points, _posixsubprocess and the public void wrapper. Busy Python
fork/preexec calls raise RuntimeError after safe unwind; the void wrapper
is fatal. Rejected preexec skips AfterFork cleanup. Immediate fork/exec
without Python preexec remains allowed.

Seven isolated regression methods cover blocked-worker progress/order,
nested weakref/unraisable delivery, early/late finalization rejection,
early/hook-started fork rejection and preexec cleanup/immediate exec. They
request actual N0 and preserve the existing child verifier assertions.
Host syntax/source checks passed; no native behavior is credited yet.
The combined focused selection is five main-only, five buffer and these
seven drainer methods, followed by the reviewed affected modules once.
