# Common call ownership: artifact review

**ACCEPT for normal reviewed integration; no remaining must-fix source finding.**

2026-11-06. Reviewed the final unexecuted `call-frame.patch` artifact and
baseline runtime `6a0e4c12c17`. This is source acceptance for the bounded proposal;
runtime application, compilation and execution are not established by this
review. The runtime tree was not modified by the reviewer.

The initial artifact missed a parent refresh after releasing the code pin on
an allocation/error edge. A mutated function can leave that pin as the last
code owner, whose weakref callbacks may reenter during cleanup. The revised
artifact centralizes precommit cleanup and refresh for both error and fallback.
That must-fix is resolved. Pending operation errors use the existing release
and `call_record_refresh` policy; no fallback follows ownership detachment.

The helper's exact Python-function and standard-vectorcall entry precondition
is established by `call_transaction_invoke`. It pins code over preparation,
then rechecks code/vectorcall/defaults/hook/stack assumptions. The success-edge
pin release cannot destroy code because the revalidated function still owns
it. Frame initialization and constant/default duplicates are non-allocating
under this guarded GIL path. Callable ownership moves once; argument cells
become child locals rather than being blanket-cleared. Empty kwargs is a fresh
owned dict; empty varargs is immortal. Instrumented, free-threaded, keyword,
generator-caller and unsupported argument shapes retain the existing provider.

The revised tests cover fresh kwargs identity/retention, defaults, warmed bound
calls, warmed mortal argument lifetime, retained callee frames, generator
callees and generator caller fallback. Tracing/default mutation checks observer
fallback; it does not simulate a mutation during dictionary allocation. Fault
injection, release/debug compilation and focused existing call/monitoring/frame
tests remain coordinator validation work. There is no claim that this artifact
has passed those checks or measured a speedup.

`git apply --numstat` parses the final artifact as 92 inserted C lines and 107
inserted test lines. No further source-level blocker was found. The canonical
ownership map and overlap initialization recipe are in
[call-and-frame-lifetimes.md](../../design/call-and-frame-lifetimes.md).

## Typed empty-varargs correction

**ACCEPT** this exact correction for normal reviewed application:

```c
child->base[extra++] = PyValue_DUP(
    PyValue_FromObjectBorrow((PyObject *)&_Py_SINGLETON(tuple_empty)));
```

The original artifact assigned `PyValueBorrow` to a `PyValue` home; that is a
compile-time type error missed in the initial source review. The coordinator's
release syntax check caught it. `PyValue_FromObjectBorrow` returns a borrow
(`pycore_value.h:761`); `PyValue_DUP` returns an owner (`794–799`). For the
immortal empty tuple, the former sets `Py_TAG_REFCNT`, so the latter's
`_PyValue_OwnsCountNotNull` branch does not increment a refcount. Neither helper
allocates, invokes a callback, or fails on this exact tuple path. The change
therefore supplies the required typed owner and preserves the non-escaping
commit proof. This note changes only review documentation, not runtime source;
it does not claim that the corrected runtime has compiled or passed tests.
