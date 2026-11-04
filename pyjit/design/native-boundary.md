# Native boundary: borrowed results from PyValue storage

Decision record, 2026-11-03, revised after
[review 1](reviews/native-boundary-review-1.md). P1 and P2 landed together as
`acbdf45627d` in the runtime tree; P3 and most review changes are in
progress. [Implementation status](#implementation-status) lists what is built.

Source references are to the runtime tree
`build/worktrees/pyvalue-rc-universal` in the boil worktree; `build/` below
means `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.

## Decisions (2026-11-03)

The coordinator accepted all 12 required changes of review 1 as stated. Status
is as of `acbdf45627d`.

| # | Decision | Review item | Status |
|---|---|---|---|
| 1 | The eval floor is set and restored in `_PyEval_EvalFrameDefault`, not in the inline `_PyEval_EvalFrame` wrapper. The invariant is stated in §2 and asserted in debug builds. | 1 | In progress |
| 2 | Code that switches C stacks inside one thread state swaps one `_PyBorrowStack` per C stack, as `datastack_*` is swapped. greenlet joins the port list with this requirement. | 2 | Specified in §3; not built |
| 3 | On activation exit, re-arm `_PY_BORROW_RELEASE_BIT` when the stack top is above the restored floor. | 3 | Landed |
| 4 | Debug builds track depth and assert on Exit that the mark is not above `top`. Item 4 of "Why" is reworded. | 4 | In progress |
| 5 | The growth bound is transitive. `_pickle.c:load_newobj` gets an explicit scope. Debug builds warn once per site when one scope passes a fixed size (the JNI `-Xcheck:jni` analogue). | 5 | In progress |
| 6 | The direct native-call sites in `Python/ceval.c` and `Python/bytecodes.c`, and the generic vectorcall dispatch, get explicit Enter/Exit. | 6 | In progress |
| 7 | Packet order: P1 and P2 land together. | 7 | Resolved (landed together) |
| 8 | Keep only dict-derived values. Push tuple runs after the arity check and only `min(nargs, max)` items. State every failure mode of `_PyBorrow_TupleItems` and box without the checking primitive. | 8 | In progress |
| 9 | Release the main thread's base region before module teardown. Assert that nothing pushes onto a cleared thread state. Leave the entries of a daemon thread that still has a frame. No explicit scope may span `Py_FinalizeEx`. | 9 | In progress |
| 10 | The cost statements use the per-operation figures for calls that push, and name the benchmarks to record. | 10 | Documented here |
| 11 | Specify the limited-API Clinic emission for tuple receivers and state the abi3 gap. | 11 | Documented here; Clinic change is P3 |
| 12 | Wording fixes in §3 (setters, slot wrappers, `Py_MakePendingCalls`, C trace functions). | 12 | Documented here |

**Problem.** Legacy C APIs hand out a borrowed `PyObject *` read from argument
storage. Tuples and dicts now store canonical `PyValue`s, and an inline int32
or float has no object to borrow. The container getters (`PyTuple_GET_ITEM`,
`PyDict_GetItem`, ...) are being ported to owning getters
(`build/pyjit-link-wave1/RECIPE.md`); that is settled and unchanged here. This
record decides the remaining borrowed boundary: the `PyArg_*` parsers on a
tuple or kwargs dict, and `PySequence_Fast_ITEMS`/`PySequence_Fast_GET_ITEM`.

## 1. Decision

Each thread state holds a **borrow stack** of owned references. The runtime
opens a **borrow scope** (a mark on that stack) around every call into a
native function body and every eval-loop activation. A parser that must
hand out a `PyObject *` for a value stored as a `PyValue` pushes an owned
reference onto the stack: a fresh box for an inline scalar, otherwise a new
reference to the object. It then returns that pointer as a borrowed reference.
The scope releases the reference when the native body returns. Parser
signatures do not change. Explicit scopes exist only for loops, embedders and
the few runtime functions that turn a tuple into an object vector.

`PySequence_Fast_ITEMS` and `PySequence_Fast_GET_ITEM` are deleted, not
backed by the stack. They are container reads; their 24 remaining uses in 13
files take the owning-getter port in RECIPE.md.

The interpreter's call transaction (`_PyNativeScope` in Python/native_scope.c)
and its result-publication protocol are unchanged.

The `_PyArg_OwnerScope` family, the proposed public `PyArgScope`, and the four
frozen templates (`build/pyvalue-format-{B-positional,B-resources,C-keywords,public-wrappers}-design-20261103-v1`)
are withdrawn. This record replaces the parser paragraphs of
[extension-api.md](extension-api.md), including its rule "no ... implicit TLS
owner engine". That rule was right for container getters and is kept for
them (see the growth bound below).

### Why

1. **The lifetime is the native frame, and the runtime already brackets it.**
   An explicit owner scope gives exactly one lifetime: until the native body
   has finished with its arguments. It makes every caller declare that
   lifetime, call Begin, and call End on every exit while preserving the first
   error. The runtime calls the body and can supply the same lifetime in
   about 30 runtime functions (§3). The alternative is 736 handwritten parse calls
   in 123 files, 38 generated Clinic headers and every exit path in each.
2. **The growth is bounded by the parse, not by reads.** One parse pushes at
   most the parsed tuple's length plus the keyword values it matched. A
   container getter on the stack would push once per read, so a loop over a
   list would grow without bound. That is why container getters stay owning
   and `PySequence_Fast_*` is deleted. The bound on a scope is transitive: a
   scope collects every push made while it is the innermost one, including
   pushes from slot code, setters, converters, direct `tp_new`/`tp_init`
   calls and vectorcall bodies that C code calls (§2 rule 10). The parser's
   unbounded case, a loop of parses in one scope, is a stated limit with an
   explicit remedy and a debug-build warning (§2 "Debug checks").
3. **A call that pushes nothing pays almost nothing.** Such a call (most
   FASTCALL, `METH_O` and `METH_NOARGS` bodies) costs two loads and a compare
   at the trampoline, all from `tstate`, which is already in a register.
   Entering the eval loop from C costs about three loads, two stores and a
   compare. Nothing is added per bytecode instruction. A call that pushes
   costs more: a `METH_VARARGS` call from bytecode pays n boxes or INCREFs,
   n frees or DECREFs, an out-of-line `_PyBorrowScope_Release`, and, when the
   trampoline does not release first, one atomic OR on `eval_breaker` plus
   `_Py_HandlePending` and an atomic AND at the next check. A `tp_new` call
   such as `datetime(2020, 1, 1)` boxes each year (2020 is not a cached small
   int) and `"i"` unboxes it again. Tuple runs box every inline item read, not
   only `O` outputs (open question 1). Decision 6 removes the atomic pair on the
   hot builtin-call paths, and decision 8 removes the extra push per FASTCALL
   keyword argument. Benchmarks to record: a
   `METH_VARARGS` microbenchmark, `datetime(2020, 1, 1)`, `PyObject_Call` with
   a tuple of inline ints, and `list(genexpr)` for the cost of entering the
   evaluator.
4. **A missing scope causes retention, not a dangling pointer.** Entries
   outlive their natural scope until an outer scope exits. So the first link
   (§4) is memory-safe before every entry point (§3) is wired. A misplaced
   eval floor or an Exit out of LIFO order is different: it frees live
   entries. Release then leaves `top` below a live mark, and the owner's later
   Exit pops past the bottom chunk. Debug builds catch this (§2 "Debug
   checks"); §2 states the invariant that prevents it.
5. **The design has known precedents.** JNI frees local references when a
   native method returns and offers `PushLocalFrame` for loops. V8 uses
   `HandleScope`. HPy remains the opaque target for ported extensions; this
   record sets the lifetime rule for the rebuilt-source bridge.

### Problems with the existing design

- **Scopes with no boxes.** `UnicodeEncodeError_init` (Objects/exceptions.c:3961)
  parses `"UUnnU"`, and `testexport_foo` (Modules/_testmultiphase.c:340)
  parses `"ll:foo"`. No unit in either format can return a box, yet both
  declare a scope, call End, and run a saved-error block.
- **One struct with five jobs.** `_PyArg_OwnerScope`
  (Include/internal/pycore_modsupport.h:91) has 20 fields. It pins the tuple,
  holds an object view, owns keyword values under a budget, holds buffer
  leases, and keeps a converter resource registry. Only the view and the
  keyword owners concern inline values. Upstream getargs already handles
  buffer and converter lifetimes with `freelist_t` and the rule that `s#` and
  `y#` take only buffers that need no release.
- **A public break for every extension.** The public-wrappers template
  changes `PyArg_ParseTuple` to `PyArg_ParseTuple(PyArgScope *, ...)`. Every
  extension's source must change, including extensions whose formats can
  never box.
- **Error plumbing at each caller.** Each wrapper template releases five
  owners, each inside a literal saved-exception block. The caller rule has
  separate cases for object, int and void returns. A helper that returns a
  parsed object needs a scope parameter added through all its callers.
  Modules/_datetimemodule.c, ported in a similar style, has 127
  `PyErr_GetRaisedException`/`_PyValue_RestoreException` sites.
- **Size.** Python/getargs.c already has about 740 lines of scope machinery
  (lines 2613–3352), and four template packets (about 770 KB on disk) have
  not landed yet.

### Limits of the chosen design

- **Loops of parses.** A native body that parses N tuples in a loop keeps
  about N·k entries until it returns. The remedy is to put
  `Py_BEGIN_BORROW_SCOPE` around the loop body (§5, rule L). Embedders that
  parse in long loops outside any callback need the same remedy.
  Limited-API (abi3) extensions have no remedy, because `PyBorrowScope` is in
  `Include/cpython` (open question 5).
- **Slot, `tp_new` and vectorcall bodies called from C loops.** The runtime
  opens no scope for these. `pickle.loads` of N float-subclass instances calls
  a Clinic `tp_new` with a tuple receiver once per instance from
  `load_newobj` (Modules/_pickle.c:6657), so one `load()` keeps N entries and
  N boxes until it returns. The fix for that site is an explicit scope (§5).
  Release is guaranteed only for bodies dispatched through the §3 trampolines
  and for code called directly from bytecode.
- **The args tuple no longer keeps outputs alive.** Some code kept an `O`
  output without a reference because a stored args tuple held it. After
  return, that pointer may refer to a released box, so the code must take a
  reference (rule E). The pattern was already fragile.
- **Tuple runs are read-only.** The run returned by `_PyBorrow_TupleItems`
  has no writable slot before element 0. Never pass
  `PY_VECTORCALL_ARGUMENTS_OFFSET` with it.
- **Release can run deallocators.** The stack holds references to arbitrary
  objects, so release must be LIFO and reentrant (§2). Boxes themselves have
  trivial deallocators.

### Options compared

| | A. Explicit owner scope (current tree) | **B. Borrow stack (chosen)** | C. Box cache on each tuple | D. Parser outputs owned | E. FASTCALL only |
|---|---|---|---|---|---|
| Lifetime rule | Until the caller's End | Until the innermost scope exits, normally the native body's return | While the tuple lives (exact legacy rule for positional); kwargs dict values not covered (dict is mutable) | Caller releases every output | Interpreter-owned argument array, for the call |
| Call sites to change | 736 calls in 123 files, plus every exit; 38 Clinic headers; public signatures | No parse call changes; 9 files drop OwnerScope; loops and escapes audited; Clinic output gets shorter | None for positional; kwargs still need A or B | 736 calls, plus `Py_DECREF` on every exit | Every VARARGS function becomes Clinic; `tp_new`/`tp_init` still receive a tuple and a dict |
| Hot native-call path | Struct init + End per parse; nothing per call | 2 loads + 1 compare per native call that pushes nothing; about 3 loads, 2 stores and a compare per eval-loop entry from C; one push per tuple item or keyword value parsed, plus a release (an atomic pair when no trampoline releases first) | +8 bytes per tuple (or a side table and a header bit); lookup per parse | INCREF/DECREF per output | Nothing |
| Outside native frames | Works (explicit) | Thread base region, released when the thread state is cleared; explicit scope available | Works | Works | Not applicable |
| Extension porting | Edit each parse and exit; thread scopes into helpers | Rebuild; wrap parse loops; fix escapes | Rebuild | Edit each parse and exit | Rewrite functions |
| Runtime machinery | ~740 lines + 4 templates; leases, budgets, registry | ~150 lines (chunked stack, keep, tuple run, release) + ~30 short entry edits | Tuple layout change, dealloc hook, and a second mechanism for kwargs | Small | Clinic conversion of 123 files |
| Verdict | Withdrawn | **Adopted** | Rejected: puts a second representation in every tuple and still needs a kwargs mechanism | Rejected: the most edits and the worst error paths | Right long-term direction for speed; not a lifetime mechanism |

**Interpreter FASTCALL boxing.** It can use the same stack, and should once
the runtime links (P4 in §5). Python/ceval.c:1304 and :1349,
`_PyObjectArray_FromValueArray` and `_PyObjectArray_Free`, already make every
argument an owned reference for the duration of the call. Pushing those
references as one run inside a scope would:

- remove the `MAX_VALUE_SCRATCH` C-stack array and the `PyMem_Malloc` path;
- remove the separate free function;
- make the interpreter's call bridge the scope boundary for calls from the
  interpreter.

The leading `PY_VECTORCALL_ARGUMENTS_OFFSET` slot becomes an owned NULL entry,
and release uses `Py_XDECREF`. This change is not on the getargs critical
path.

## 2. API and lifetime contract

### Internal: `Include/internal/pycore_borrow.h`, `Python/borrow.c`

The chunk and stack types live in `Include/internal/pycore_tstate.h`, because
`_PyThreadStateImpl` embeds the stack and `pycore_borrow.h` includes
`pycore_tstate.h`; defining them in `pycore_borrow.h` would make an include
cycle.

```c
typedef struct _PyBorrowChunk {
    struct _PyBorrowChunk *previous;
    PyObject **previous_top;    /* previous chunk's top when this one opened */
    PyObject **end;
    PyObject *slots[];
} _PyBorrowChunk;

typedef struct {
    PyObject **top;             /* next free slot */
    PyObject **end;             /* end of the current chunk */
    PyObject **eval_floor;      /* top when the innermost eval loop was
                                   entered; NULL outside any eval loop */
    _PyBorrowChunk *chunk;      /* initially the empty chunk in borrow.c */
    _PyBorrowChunk *spare;      /* one cached chunk */
} _PyBorrowStack;               /* new field `borrow` in _PyThreadStateImpl */

typedef PyObject **_PyBorrowMark;

/* Pop and Py_DECREF the entries above mark, newest first. Cannot fail and
 * does not touch the error indicator (deallocators preserve it). Reentrant:
 * code run by a deallocator pushes above the current top and either releases
 * its own entries or leaves them for this loop. */
PyAPI_FUNC(void) _PyBorrowScope_Release(PyThreadState *, _PyBorrowMark);

static inline _PyBorrowMark
_PyBorrowScope_Enter(PyThreadState *tstate)
{
    return ((_PyThreadStateImpl *)tstate)->borrow.top;
}

static inline void
_PyBorrowScope_Exit(PyThreadState *tstate, _PyBorrowMark mark)
{
    if (((_PyThreadStateImpl *)tstate)->borrow.top != mark) {
        _PyBorrowScope_Release(tstate, mark);
    }
}

/* Both functions below act on the current thread state. */

/* Steal `owned`, push it, and return it as a borrowed reference that is
 * valid until the current scope exits. On allocation failure, release
 * `owned`, set MemoryError and return NULL. */
PyAPI_FUNC(PyObject *) _PyBorrow_Keep(PyObject *owned);

/* Push one owned reference per item of the first `count` items of `tuple`
 * (0 <= count <= len) as one contiguous run and return the run. Inline
 * scalars are boxed. A zero `count` returns a static array and pushes
 * nothing. The caller checks arity first and passes
 * `min(nargs, max)`, so a wrong-arity call or `f(*range(10**7))` pushes
 * nothing. Failure returns NULL with a pending exception: MemoryError when a
 * chunk cannot grow, or SystemError when an item is not a language value.
 * (The built version also fails with an exception that was already pending,
 * through `PyValue_AsObject`; the non-checking box removes that case.) Items
 * already pushed stay until the scope exits. The run is
 * read-only and has no slot before element 0. */
PyAPI_FUNC(PyObject *const *) _PyBorrow_TupleItems(PyObject *tuple,
                                                   Py_ssize_t count);

/* Ask the innermost eval loop to release the entries above its floor at the
 * next instruction boundary. No-op outside any eval loop. */
PyAPI_FUNC(void) _PyBorrow_RequestRelease(PyThreadState *tstate);

/* Thread-state lifecycle (Python/pystate.c). Init allocates nothing. Clear
 * releases every entry, including the base region, and frees the chunks. */
extern void _PyBorrowStack_Init(_PyBorrowStack *stack);
extern void _PyBorrowStack_Clear(PyThreadState *tstate);
```

`_PyBorrow_RequestRelease` is exported because the inline
`_PyEval_EvalFrame` calls it. `_PyBorrow_TupleItems` as built takes the tuple
only and pushes every item (Python/borrow.c:_PyBorrow_TupleItems); the
`count` parameter is decision 8 and is in progress.

Implementation rules for `Python/borrow.c`:

- **Empty chunk.** The file-static `empty_chunk` in Python/borrow.c has zero
  slots. `init_threadstate` (Python/pystate.c) calls `_PyBorrowStack_Init`,
  which sets `chunk = &empty_chunk` and `top = end = chunk->slots`. Creating a
  thread allocates nothing, and a mark is never NULL.
- **Growth.** A push that does not fit takes `spare` or allocates
  `max(n, 1024)` slots (`borrow_grow`). It records `previous_top = top` and
  links `previous`. A run never straddles two chunks. Chunks come from
  `PyMem_RawMalloc`, so a thread state can be cleared from any thread.
- **Spare chunk.** Release keeps an emptied chunk as `spare` only when it has
  exactly the standard 1024 slots (`borrow_chunk_retire`). An oversized chunk
  built for a long tuple is freed, so one long tuple does not pin memory for
  the thread's life.
- **Release.** Re-read the stack fields on every iteration, because a
  deallocator may push or pop:
  ```c
  while (s->top != mark) {
      if (s->top == s->chunk->slots) {          /* chunk empty: pop it */
          _PyBorrowChunk *done = s->chunk;
          s->chunk = done->previous;
          s->top = done->previous_top;
          s->end = s->chunk->end;
          borrow_chunk_retire(s, done);           /* keep one as spare */
          continue;
      }
      PyObject *o = *--s->top;
      Py_XDECREF(o);
  }
  ```
- **Boxing.** `_PyBorrow_TupleItems` boxes each item with
  `PyValue_AsPyObjectNewBoxed(PyTuple_GET_VALUE(tuple, i))` after checking
  `PyValue_IsLanguageValue`. It does not call `PyValue_AsObject`, which runs
  `PyErr_Occurred()` per item (Python/pyvalue.c:PyValue_AsObject). As built,
  `_PyBorrow_TupleItems` still calls `PyValue_AsObject`; the change is part of
  decision 8. Boxing an exact int or float and taking a new reference cannot
  run Python code, so the run stays contiguous.
- **Eval-breaker bit.** When `eval_floor != NULL`, `_PyBorrow_Keep` and
  `_PyBorrow_TupleItems` call `_PyBorrow_RequestRelease`, which sets a new
  eval-breaker bit, `_PY_BORROW_RELEASE_BIT = (1U << 7)`
  (Include/internal/pycore_ceval.h, next to `_PY_EVAL_EXPLICIT_MERGE_BIT`).
  Bit 7 is free and lies inside `_PY_EVAL_EVENTS_MASK`, so `check_periodics`
  sees it. The bit is per thread, but the parked entries belong to one
  activation. When an activation exits and restores the outer floor, it
  re-arms the bit if `top` is above that floor
  (Include/internal/pycore_ceval.h:_PyEval_EvalFrame). Without this, a nested
  activation's first check clears the bit and strands the outer activation's
  entries until it exits.
- **Where the release runs.** `_Py_HandlePending` releases to `eval_floor`
  after the GC step and before the GIL-drop and async-exception steps. It
  tests the bit after pending calls and GC have run, because both can push and
  set it again (Python/ceval_gil.c:_Py_HandlePending).

### The release invariant

The eval-breaker release is safe only under this invariant:

> When `_Py_HandlePending` releases, no live C frame between the eval floor
> and the current instruction holds a mark or a borrow.

"Live" means a C frame that started after the point that set the floor and
that has not returned. Every caller of `_Py_HandlePending` is at an
instruction boundary (§3), where all native code that the activation called
has returned. The floor must therefore be set where the evaluator pushes its
entry frame, in `_PyEval_EvalFrameDefault`, and not in the inline
`_PyEval_EvalFrame` wrapper. A PEP 523 hook (the perf trampoline at
Python/perf_trampoline.c:470, `record_eval` in Modules/_testinternalcapi.c,
third-party evaluators) runs C code between the wrapper and the evaluator;
anything it pushes would be released while live. As built, the floor is still
set in the wrapper (Include/internal/pycore_ceval.h:_PyEval_EvalFrame); moving
it is decision 1.

A broken invariant is worse than a dangling borrow. Release leaves `top`
below a live mark. The owner's later Exit then sees `top != mark` and pops
from `top` looking for a mark above it. It frees entries of outer scopes and
pops past the bottom chunk. The release build dereferences a NULL `chunk`; the
current debug build trips `assert(done != &empty_chunk)` in
`_PyBorrowScope_Release`.

### Debug checks

Debug builds add four checks (decisions 1, 4 and 5; none is built yet):

- **Depth on Exit.** Each chunk records the depth below it. Exit asserts
  `depth(mark) <= depth(top)` and fails fatally with a message naming the
  mark, the top and the thread state, instead of popping past the bottom.
- **Same thread state.** Exit uses the trampoline's cached `tstate`, while
  Keep uses `_PyThreadState_GET()`. Exit asserts they match, which catches a
  body that returns with another thread state current.
- **Floor assert.** `_Py_HandlePending` asserts that `eval_floor` is non-NULL
  and not above `top` when it releases.
- **Size warning.** When one scope holds more than a fixed number of entries
  (a fixed count; start at 10000 and tune), warn once per push site, naming the site. This is the JNI
  `-Xcheck:jni` analogue and gives rule L a measured limit.

### Public: `Include/cpython/modsupport.h`

```c
typedef struct { void *_mark; } PyBorrowScope;
PyAPI_FUNC(void) PyBorrowScope_Enter(PyBorrowScope *scope);
PyAPI_FUNC(void) PyBorrowScope_Exit(PyBorrowScope *scope);

#define Py_BEGIN_BORROW_SCOPE \
    { PyBorrowScope _py_borrow_scope; PyBorrowScope_Enter(&_py_borrow_scope);
#define Py_END_BORROW_SCOPE \
    PyBorrowScope_Exit(&_py_borrow_scope); }
```

`PyArg_Parse`, `PyArg_ParseTuple`, `PyArg_ParseTupleAndKeywords`,
`PyArg_VaParse`, `PyArg_VaParseTupleAndKeywords`, `PyArg_UnpackTuple`,
`PyArg_ParseArray*` and the `_SizeT`/`_PyArg_*` variants keep their upstream
signatures.

### Lifetime contract

1. **What a scope is.** A borrow scope is the interval between Enter and Exit
   on one thread. Scopes nest strictly.
2. **Where the runtime opens scopes.** It opens one around every call into a
   native function body made through the call protocol, including the direct
   native-call sites in the interpreter and the generic vectorcall dispatch,
   and around every eval-loop activation (§3). At instruction boundaries it
   also releases entries left by slot code that bytecode called directly.
3. **Object outputs.** These are borrowed references, valid until the
   innermost scope that was open during the parse exits:
   - the `O`, `O!` and `S`/`U`/`Y` outputs;
   - the object passed to an `O&` converter;
   - objects inside a nested `(...)` format;
   - every object written by `PyArg_UnpackTuple`.

   Each one is backed by a scope entry or by the caller's argument array.
4. **Pointer outputs.** Outputs that point into object storage (`s`, `z`,
   `y`, `s#`, `z#`, `y#`) stay valid while that object lives, which is at
   least as long as rule 3.
5. **Keeping an output longer.** To store an output in an object or global,
   return it, or use it after `Py_END_BORROW_SCOPE`, take a strong reference
   (`Py_NewRef`) before the scope exits. `return Py_NewRef(x);` is unchanged.
6. **Owned outputs are unchanged.** `es`, `et`, `es#`, `et#`, the `*` buffer
   formats and `O&` converter results keep their existing ownership and
   cleanup rules. The borrow stack never owns them.
7. **No cache.** No entry outlives its scope.
8. **Release.** Release is LIFO, cannot fail and leaves the error indicator
   unchanged.
9. **Code outside every scope uses the thread's base region.** This covers
   code before the first eval loop on a thread, core initialization
   (`pyinit_core`), and embedding code between `PyGILState_Ensure` and
   `PyGILState_Release`. The base region is released when the thread state is
   cleared; for the main thread it is released in `Py_FinalizeEx` and
   `Py_EndInterpreter` after atexit callbacks and before `finalize_modules`,
   so retained objects run `__del__` before module teardown. Long-running code
   there must open an explicit scope. An explicit scope must not span
   `Py_FinalizeEx`: Exit would run on a cleared thread state. No push may
   reach a cleared thread state (debug builds assert this).
10. **Growth bound.** Per parse, a scope gains at most the tuple's length (or
    `min(nargs, max)` once decision 8 lands), plus the length of any nested
    `(...)` tuple, plus the number of keyword values matched, plus up to
    `len(kwargs)` keys on the extraneous-keyword error path. The bound on a
    scope is transitive: it is the sum of that for every parse made while the
    scope is the innermost one. That includes parses in slot code, setters,
    converters, direct `tp_new`/`tp_init` calls and vectorcall bodies called
    from C, none of which opens a scope of its own. Debug builds warn once per
    push site when one scope passes a fixed count (§2 "Debug checks").

## 3. Entry points that establish scopes

In each native-call trampoline, wrap only the body call. In methodobject.c
and descrobject.c this sits between `cfunction_enter_call`/`method_enter_call`
and `_Py_LeaveRecursiveCallTstate`, and every trampoline already has `tstate`:

```c
_PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
PyObject *result = meth(PyCFunction_GET_SELF(func), args, nargs);
_PyBorrowScope_Exit(tstate, mark);
```

| File | Function | Change |
|---|---|---|
| Python/ceval.c | `_PyEval_EvalFrameDefault` (entry frame push and pop) | Save `borrow.eval_floor`, set it to `top`, run the loop, `Exit` to the floor, restore, and re-arm `_PY_BORROW_RELEASE_BIT` if `top` is above the restored outer floor. The floor belongs here, not in the inline wrapper, so a PEP 523 hook's own C code (the perf trampoline at Python/perf_trampoline.c:470, `record_eval`, third-party evaluators) is outside the floor (§2 "The release invariant"). Covers `_PyEval_Vector` and generators and coroutines resumed from C (`gen_send_ex2`). **As built:** the save, set, Exit, restore and re-arm sit in the inline `_PyEval_EvalFrame` (Include/internal/pycore_ceval.h); the re-arm is landed, the move is decision 1. |
| Python/ceval_gil.c | `_Py_HandlePending` | After GC and before the GIL-drop and async-exception steps: if `_PY_BORROW_RELEASE_BIT` is set (read again after pending calls and GC), clear it and `_PyBorrowScope_Exit(tstate, borrow.eval_floor)`. Releases entries parked by slot code that bytecode handlers call directly (`nb_*`, `tp_richcompare`, `tp_iter`, `tp_getattro`, descriptors), by C trace and profile functions (`PyEval_SetTrace`) that run inside instrumented instructions, and by pending calls that ran in the same call. The only callers are `check_periodics` and `check_periodics_at_end` (Python/ceval_macros.h:533, 543), reached from the eval loop and from the call-transaction end (Python/ceval.c:2010). Every caller is at an instruction boundary, where all native code that this activation called has returned (the invariant in §2). That code already runs GC and signal handlers, so a release that runs deallocators adds no new kind of reentry. |
| Python/ceval.c | `_Py_BuiltinCallFast_Value`, `_Py_BuiltinCallFastWithKeywords_Value`, `_PyCallMethodDescriptorFast_Value`, `_PyCallMethodDescriptorFastWithKeywords_Value`, `_Py_CallBuiltinClass_Value` | Enter/Exit around the body call. These are the hot builtin-call paths; without scopes any push costs an atomic pair plus `_Py_HandlePending`. |
| Python/bytecodes.c | `_CALL_METHOD_DESCRIPTOR_O` (:4847) and `_CALL_METHOD_DESCRIPTOR_NOARGS` (:4961), which call `_PyCFunction_TrampolineCall` directly | Enter/Exit around the call. The other bytecodes.c CALL sites (:4552, :4648, :4686, :4908, :5016) go through the helpers above. |
| Include/internal/pycore_call.h | generic vectorcall dispatch, `func(callable, args, nargsf, kwnames)` (:143) | Enter/Exit around the call. It reaches builtin-type `tp_vectorcall`, `operator.*getter` and third-party function objects (Cython, nanobind). Rule 2 says every call through the call protocol opens a scope; this path is the call protocol. |
| Objects/methodobject.c | `cfunction_vectorcall_FASTCALL`, `cfunction_vectorcall_FASTCALL_KEYWORDS`, `cfunction_vectorcall_FASTCALL_KEYWORDS_METHOD`, `cfunction_vectorcall_NOARGS`, `cfunction_vectorcall_O`, `cfunction_call` | Enter/Exit around the body. `cfunction_call` covers METH_VARARGS and METH_VARARGS\|METH_KEYWORDS. |
| Objects/methodobject.c | `_PyCFunction_CallScoped` | Enter/Exit around the body call inside the existing invocation transaction. |
| Objects/descrobject.c | `method_vectorcall_VARARGS`, `method_vectorcall_VARARGS_KEYWORDS`, `method_vectorcall_FASTCALL`, `method_vectorcall_FASTCALL_KEYWORDS`, `method_vectorcall_FASTCALL_KEYWORDS_METHOD`, `method_vectorcall_NOARGS`, `method_vectorcall_O` | Enter/Exit around the body. |
| Objects/descrobject.c | `classmethoddescr_call` | Replace the OwnerScope with Enter, `_PyBorrow_TupleItems(args)`, call, Exit. |
| Objects/descrobject.c | `wrapperdescr_raw_call` | Enter/Exit. Covers `wrapperdescr_call`, `wrapper_call` and every `wrap_*` slot wrapper. Only six `wrap_*` functions call `PyArg_UnpackTuple` (Objects/typeobject.c:12002-12476); the rest use owning getters. |
| Objects/typeobject.c | `type_call` | One scope around `tp_new` and `tp_init`. |
| Objects/typeobject.c | `tp_new_wrapper` | Enter/Exit. |
| Objects/typeobject.c | `type_new` (line 6155) | Drop the OwnerScope; parse with `PyArg_ParseTuple`. |
| Objects/call.c | `_PyObject_MakeTpCall`, `_PyObject_Call` (`tp_call` branch) | Enter/Exit around `tp_call`. |
| Objects/call.c | `_PyVectorcall_Call`, `_PyObject_Call_Prepend`, `_PyObject_CallFunctionVa`, `callmethod` | Replace the OwnerScope with Enter, `_PyBorrow_TupleItems(tuple)`, call, Exit. The scope is both the view owner and the call boundary. |
| Objects/call.c | `_PyStack_AsDict` (line 1091) | No scope needed: `kwnames` holds only str, so read names with `_PyTuple_GET_OBJECT`. |
| Python/pystate.c | `init_threadstate`; `PyThreadState_Clear` | Init calls `_PyBorrowStack_Init`, which points at the empty chunk. Clear calls `_PyBorrowStack_Clear` twice: once before other teardown, and once after the finalizers that Clear runs, because those can push. Each call releases to the empty chunk's slots and frees `spare`. `PyGILState_Release` reaches Clear for thread states that Ensure created. Clearing a thread state that still has a frame (a daemon thread inside `Py_BEGIN_ALLOW_THREADS` at `interpreter_clear`, Python/pystate.c:757) leaves its entries, as upstream leaves its frames (warning at Python/pystate.c:1696), because GIL-free code may still read them. Debug builds assert that nothing pushes onto a cleared thread state (decision 9, in progress). |
| Python/pylifecycle.c | `init_interp_main` | Enter at start and Exit at end (a wrapper around `init_interp_main_impl`), so main initialization does not leave entries behind. Entries from `pyinit_core`, before this point, stay in the base region. They are bounded by startup's fixed work. |
| Python/pylifecycle.c | `Py_FinalizeEx`, `Py_EndInterpreter` | Release the main thread's base region after atexit callbacks and before `finalize_modules` (Python/pylifecycle.c:2485), so retained objects run `__del__` before module teardown. Not built yet (decision 9). |
| Modules/_pickle.c | `load_newobj` (:6657) | One scope around each `tp_new` call, because a Clinic `tp_new` with a tuple receiver pushes once per NEWOBJ opcode inside one `load()`. Not built yet (decision 5). |

**As built.** `acbdf45627d` wires the PyCFunction and method-descriptor
trampolines, `wrapperdescr_raw_call`, `type_call`, `_PyObject_MakeTpCall`,
`_PyObject_Call` and `_PyCFunction_CallScoped`; the `_PyEval_EvalFrame` wrapper;
the `_Py_HandlePending` release; `init_interp_main`; and the public
`PyBorrowScope`. The rows for `classmethoddescr_call`, `tp_new_wrapper`, the
tuple-to-vector bridges in Objects/call.c and `methodcaller_vectorcall` are P3.
The ceval.c and bytecodes.c helpers, the generic vectorcall dispatch,
`load_newobj` and the finalization rows are in progress.

**Covered without new code:**

- **Module init.** `PyInit_*` runs under `_imp.create_dynamic`, and exec slots
  under `_imp.exec_dynamic`. Both are builtin functions, so they run inside a
  trampoline.
- **Finalizers, GC and weakref callbacks.** They run inside whatever scope
  triggered them. Python-level `__del__` and callbacks enter through
  `_PyEval_EvalFrame`.
- **atexit and `Py_AddPendingCall` callbacks.** atexit callbacks go through the
  call protocol. Pending calls run in `_Py_HandlePending`, which releases
  afterwards. Pending calls also run from `Py_MakePendingCalls` inside native
  bodies (Python/lock.c:136, Modules/_threadmodule.c:586,
  Modules/_queuemodule.c:434, Python/thread.c:232); their pushes stay in that
  body's scope.
- **C trace and profile functions** (`PyEval_SetTrace`). They run inside
  instrumented instructions, so the eval-breaker bit releases their entries.
- **getset and member descriptors.** Setters can parse, for example
  `PyArg_Parse(arg, "i")` in Modules/_ssl.c:`set_min_max_proto_version`. They
  are not trampolined; they follow the slot rule below. A compat-mode parse of
  a scalar pushes nothing.

**Slots, `tp_new` and vectorcall bodies called from C.** Slots such as
`tp_iternext`, `tp_richcompare`, `tp_hash`, `nb_*`, `sq_*`, `mp_*`,
`tp_getattro`, `tp_descr_get`, `tp_repr` and `bf_getbuffer` open no scope. Called
from bytecode, the eval-breaker bit releases their pushes. Called from a C loop
(`map_next` and `zip_next` in Python/bltinmodule.c, `list.sort` calling
`tp_richcompare`, `load_newobj`), their pushes stay in the caller's scope until
it exits. That is memory-safe, and it is the transitive bound of §2 rule 10.

**Generators and coroutines.** Native code cannot suspend, so a scope never
spans a yield. Each resumption from C is a new `_PyEval_EvalFrame` activation
with its own floor. Resumption from Python (`SEND`, `FOR_ITER_GEN`) stays in
the current activation. That is safe because only native code pushes entries,
and that code has returned by the next instruction boundary.

**Reentrancy.** If a native body calls back into Python, the nested eval
activation's floor sits above the body's entries, so releasing the nested
activation cannot touch them. A slot called from bytecode that parses and then
runs Python code is the harder case: the nested activation clears the
per-thread release bit, so on exit it re-arms the bit when the outer floor is
below `top` (§2 "Eval-breaker bit"). `longjmp` across an activation skips the
floor restore; that is already undefined behaviour in CPython (frames,
recursion depth).

**Stack switching.** Code that switches C stacks inside one thread state
(greenlet, gevent, eventlet) gives each C stack its own `_PyBorrowStack`,
including its `eval_floor`, and swaps it as it swaps `datastack_*`. The runtime
provides internal save and restore functions for this, and greenlet goes on the
port list with this requirement. Without it, greenlet A's trampoline Exit would
release greenlet B's live entries, and B's later Exit would find `top` below
its mark.

**Exceptions.** C code returns through the trampoline's single exit, and Exit
does not touch the error indicator.

**Threads and builds.** The stack lives in `_PyThreadStateImpl`, so each
thread has its own. Free-threaded and 32-bit builds have no inline values,
but tuple runs still use the stack, and the code is the same.

## 4. Closing Python/getargs.c (packet P1, under two hours)

P1 landed in `acbdf45627d` with the line numbers below as of its parent. The
as-built code differs from the steps in two ways, both in progress as decision
8: tuple runs push every item before the arity check, and values from
`find_keyword` and `find_keyword_str` are kept. Step 9 landed.

There are 12 errors: the 11 listed in `build/pyjit-link-wave1/v22-errors.txt`,
plus `Include/pymacro.h:313`, which comes from line 2104. P1 leaves the
OwnerScope functions compiling and in use. P3 deletes them together with
their callers, so no intermediate tree fails to link.

**P1a, borrow stack core (~30 min).** Add:

- `Include/internal/pycore_borrow.h` and `Python/borrow.c`, as in §2;
- the `_PyBorrowStack borrow;` field in `_PyThreadStateImpl`
  (Include/internal/pycore_tstate.h);
- init and clear in Python/pystate.c;
- `Python/borrow.o` in `Makefile.pre.in` and `PCbuild/pythoncore.vcxproj`.

`PyBorrowScope_Enter/Exit` can wait for P2.

**P1b, getargs.c (~45 min), function by function.** Add
`#include "pycore_borrow.h"`.

1. **`vgetargs1` (413), error at 428.** Change `PyObject **stack` to
   `PyObject *const *stack`. Then:
   Push after the arity check in `vgetargs1_impl`, and only
   `min(nargs, max)` items, so a wrong-arity call or `f(*range(10**7))` to a
   `METH_VARARGS` function raises before it boxes anything:
   ```c
   nargs = PyTuple_GET_SIZE(args);
   /* ... arity check; raise TypeError on mismatch ... */
   stack = _PyBorrow_TupleItems(args, Py_MIN(nargs, max));
   if (stack == NULL) {
       return 0;
   }
   ```
2. **`converttuple` (565), error at 673.** After the
   `istuple`/`PySequence_Tuple` block, replace the length check and loop.
   The run owns the elements, so the temporary tuple can go first:
   ```c
   len = PyTuple_GET_SIZE(arg);
   PyObject *const *items = len == n ? _PyBorrow_TupleItems(arg, n) : NULL;
   if (!istuple) {
       Py_DECREF(arg);            /* the run owns the elements */
   }
   if (len != n) {
       levels[0] = 0;
       PyOS_snprintf(msgbuf, bufsize, "must be tuple of length %d, not %zd", n, len);
       return msgbuf;
   }
   if (items == NULL) {
       return msgbuf;             /* MemoryError set; seterror() defers to it */
   }
   format = *p_format;
   for (i = 0; i < n; i++) {
       const char *msg = convertitem(items[i], &format, p_va, flags,
                                     levels+1, msgbuf, bufsize, freelist);
       if (msg != NULL) {
           levels[0] = i+1;
           return msg;
       }
   }
   *p_format = format;
   return NULL;
   ```
   This also removes the three `if (!istuple) Py_DECREF(arg)` sites. Objects
   from a non-tuple sequence now outlive the temporary tuple. Upstream could
   return them dangling when the sequence created its items on access.
3. **`find_keyword_str` (1742), error at 1746.** Use
   `PyObject *kwname = _PyTuple_GET_OBJECT(kwnames, i);`. Vectorcall
   `kwnames` holds only str, and the existing assert stays. It still returns
   an owned value.
4. **`vgetargskeywords_impl` (1757).**
   - **Value loop (~1910).** Positional values come from the caller's array
     or tuple run. Keep only values that come from the `kwargs` dict.
     `find_keyword_str` results are owned by the caller's `kwstack`, so DECREF
     them back to a borrowed reference as upstream does and do not Keep them;
     otherwise every FASTCALL keyword argument costs a push and an atomic until
     P3. Keeping the dict values instead of releasing them after `convertitem`
     fixes a dangling box. The sketch below shows the dict branch; the
     `find_keyword_str` branch sets `current_arg` to the borrowed result:
     ```c
     PyObject *current_arg = NULL;
     if (i < nargs) {
         current_arg = args[i];
     }
     else if (nkwargs && i >= pos) {
         PyObject *owned = NULL;
         if (kwargs != NULL) {
             if (PyDict_GetItemStringRef(kwargs, kwlist[i], &owned) < 0) {
                 return cleanreturn(0, &freelist);
             }
         }
         else {
             owned = find_keyword_str(kwnames, kwstack, kwlist[i]);
         }
         if (owned != NULL) {
             current_arg = _PyBorrow_Keep(owned);
             if (current_arg == NULL) {
                 return cleanreturn(0, &freelist);
             }
             --nkwargs;
         }
     }
     if (current_arg) {
         msg = convertitem(current_arg, &format, p_va, flags,
                           levels, msgbuf, sizeof(msgbuf), &freelist);
         if (msg) { ... unchanged ... }
         continue;
     }
     ```
     Delete the `Py_DECREF(current_arg)` at 1934 for dict values. The
     duplicate check (~2005–2016) only tests presence and stays as it is.
   - **Extraneous-keyword loop, errors at 2030 and 2038.** This loop runs only
     on the error path, so keeping its keys costs nothing in practice and
     avoids DECREF plumbing. It keeps up to `len(kwargs)` keys, which rule 10
     counts:
     ```c
     if (kwargs != NULL) {
         PyObject *owned;
         int r = PyDict_NextEntryRef(kwargs, &j, &owned, NULL);
         if (r < 0) {
             return cleanreturn(0, &freelist);
         }
         if (r == 0) {
             break;
         }
         key = _PyBorrow_Keep(owned);
         if (key == NULL) {
             return cleanreturn(0, &freelist);
         }
     }
     else {
         if (j >= nkwargs) {
             break;
         }
         key = _PyTuple_GET_OBJECT(kwnames, j);
         j++;
     }
     ```
5. **`vgetargskeywords` (2100), error at `Include/pymacro.h:313`.**
   ```c
   PyObject *const *args = _PyBorrow_TupleItems(argstuple, nargs_to_push);
   if (args == NULL) {
       return 0;
   }
   ```
   Check arity first and push `min(nargs, max)` items, as in step 1.
6. **`vgetargskeywordsfast_impl` (2377), errors at 2490, 2533 and 2562.**
   - Replace the three `PyTuple_GET_ITEM(kwtuple, i - pos)` with
     `_PyTuple_GET_OBJECT(kwtuple, i - pos)`; the parser's keyword tuple
     holds interned str.
   - In the value loop, use `current_arg = args[i]` for positional values.
     For a value from the dict or from `find_keyword`, write
     `current_arg = _PyBorrow_Keep(owned)` (return `cleanreturn(0, ...)` on
     NULL). A `find_keyword` result is owned by the caller's `kwstack`: DECREF
     it back to borrowed and do not Keep it. `find_keyword` can raise, so check
     `PyErr_Occurred()` after it.
   - Delete the `Py_DECREF(current_arg)` at 2510. The duplicate check at
     2562–2572 stays owned-then-released.
7. **`vgetargskeywordsfast` (2592), error at 2606.** As step 5, with
   `PyObject *const *stack`.
8. **`PyArg_UnpackTuple` (3554), error at 3566.** As step 5.
   `unpack_stack` writes `stack[i]`, so its outputs are scope-backed.
9. **`unpack_keywords` (3103), not a compile error but a dangling box.** In
   the `scope == NULL` branch at 3254–3260, replace
   `Py_DECREF(current_arg);` with:
   ```c
   current_arg = _PyBorrow_Keep(current_arg);
   if (current_arg == NULL) {
       return NULL;
   }
   ```
   Without this, unscoped Clinic `tp_new`/`tp_init` callers of
   `_PyArg_UnpackKeywords` would read a released box for an inline keyword
   value. Keep only values from the `kwargs` dict; the `kwnames` branch can
   DECREF as upstream does. (Landed at Python/getargs.c:3292.)

**P1c, verification (~30 min).** Compile getargs.o. Then run the
configured build to the next error. Once the runtime links, run tests 1–3
and 9 from §6. Tests 3 and 9 check depth after return, which needs P2's
scopes. P1 and P2 therefore landed as one commit (`acbdf45627d`), and the
packet-order concern in review item 7 does not arise.

## 5. Caller migration rule

### Handwritten `PyArg_*` callers (736 calls in 123 files)

- **Default.** No source change; rebuild.
- **Rule L (loops).** If a parse runs inside a loop whose trip count depends
  on input, wrap the loop body in `Py_BEGIN_BORROW_SCOPE` …
  `Py_END_BORROW_SCOPE`, and take a reference to anything kept past the body.
  As with `Py_BEGIN_ALLOW_THREADS`, `break`, `continue` and `return` must not
  leave the block. A skipped Exit delays release; it does not cause a
  dangling pointer.
- **Rule E (escapes).** Every parsed object stored in a struct field, global
  or cache, or returned, must be `Py_NewRef`'d. This was already required.
  The only new failure is code that relied on a stored args tuple to keep the
  object alive. To audit, look for assignments of a parse output to `->field`
  with no `Py_NewRef`/`Py_INCREF`/`Py_XSETREF` near the parse. The lexical
  audit misses escapes through helpers, globals and capsules. An escape shows
  only when the value was inline, because a non-box object stays alive through
  its source tuple or dict. The detection mechanism is the suite under ASan
  with inline-valued inputs, as test 1 does.
- **Census starting points for rule L.** A lexical census finds parses in
  loops at Modules/socketmodule.c:4663 and :5108, Modules/_ctypes/_ctypes.c:3963,
  Modules/_ctypes/stgdict.c:103, Modules/_winapi.c:2426 and :2644, and
  Modules/_io/textio.c:2008, reached from the `read(n)` loop at :2173 and the
  `readline` loop at :2347. Also scope `Modules/_pickle.c:load_newobj`
  (:6657), which calls a C `tp_new` once per NEWOBJ opcode. A parser-aware
  scan is still needed (open question 4).
- **Helpers in the same native body.** A helper that returns a parsed
  borrowed object to its caller within the same body needs no change.
- **Error-path style.** Do not add saved-exception blocks around
  `Py_DECREF`. Deallocators preserve the pending exception
  (extension-api.md), so the release in this design needs none.

### Remove existing scope ports (P3)

- **Objects/exceptions.c** (`UnicodeEncodeError_init` 3961,
  `UnicodeTranslateError_init` 4198), **Objects/typeobject.c** (`type_new`
  6155) and **Modules/_testmultiphase.c** (`Example_demo` 62,
  `testexport_foo` 342): use `PyArg_ParseTuple(args, ...)` again. Delete the
  scope declaration, every End call, and the error blocks that existed only
  for End.
- **Objects/call.c, Objects/descrobject.c and Modules/_operator.c**
  (`methodcaller_vectorcall` 1998): Enter, then `_PyBorrow_TupleItems`, then
  Exit, as in §3.
- **Python/Python-ast.c** (`ast_type_init` 1539, generated by
  `Parser/asdl_c.py`): read positional fields from
  `_PyBorrow_TupleItems(args)`. As `tp_init`, it already runs inside the
  scope that `type_call` opens. Change the generator and regenerate.
- **Modules/_testcapi/vectorcall.c** (`fastcall_args`/`fastcall_result`):
  use `_PyBorrow_TupleItems`; the result needs no wrapping.
- **Modules/_testinternalcapi/native_scope.c**: drop the OwnerScope cases
  and keep the invocation-transaction tests.

### Argument Clinic (`Tools/clinic/libclinic/parse_args.py`)

- **Delete the scoped emission paths:**
  - `scoped_tuple_helper_names` (433);
  - the End-on-exit block in `parser_body` (662–700);
  - the `_PyArg_UnpackPositionalOwned` path in `parse_pos_only` (1381–1400);
  - the owned varargs path in `parse_var_keyword` (1472);
  - the `_PyArg_UnpackKeywordsOwned` paths in `parse_general` (1651–1665,
    1784–1840);
  - the `PyErr_Occurred()` checks added after each conversion for the scope.
- **Emit one run for tuple receivers.** For any function that receives a
  `PyObject *args` tuple (METH_VARARGS, `tp_new`, `tp_init`), emit this after
  the existing arity and `_PyArg_NoKeywords` checks, in both the
  positional-only and the keyword path. Push `min(nargs, maxpos)` items, so
  arity errors and the `*args` tail push nothing:
  ```c
  PyObject *const *_args_items = _PyBorrow_TupleItems(args, Py_MIN(nargs, maxpos));
  if (_args_items == NULL) {
      goto exit;
  }
  ```
  Use `_args_items[%s]` where `select_positional_convention` (1255–1279)
  returned `PyTuple_GET_ITEM(args, %s)`. Pass `_args_items` where 1651 and
  1952 passed `_PyTuple_CAST(args)->ob_item`. FASTCALL output is unchanged:
  its arguments are already the caller's object array.
- **Limited-API form.** Under `limited_capi`, `select_positional_convention`
  (parse_args.py:1255) still returns `PyTuple_GetItem(args, %s)` (:1277), and
  that function no longer exists in this tree; the owning replacement is
  `PyTuple_GetItemRef`. Emit `PyTuple_Size(args)` for the arity check and read
  the positional items into locals with `PyArg_UnpackTuple`, whose outputs are
  backed by the scope and which is public. abi3 extensions get no remedy under
  rule L unless `PyBorrowScope` or an equivalent joins the limited API (open
  question 5).
- **Regenerate.** Run `python3 Tools/clinic/clinic.py --make --srcdir .`.
  The 38 headers lose their OwnerScope code. Update the expected output in
  Lib/test/clinic.test.c and in Lib/test/test_clinic.py.

### Delete from the runtime (P3, with the callers above)

- **Include/internal/pycore_modsupport.h (lines 76–151).** Delete
  `_PyArg_LeaseChunk`, `_PyArg_ResourceChunk`, `_PyArg_FormatDestructor`,
  `_PyArg_FormatResource` and `_PyArg_OwnerScope`. Delete the declarations of
  `_PyArg_OwnerScope_BeginTuple`, `_Objects`, `_GetBuffer`, `_End` and
  `_KeepObject`, of `_PyArg_UnpackKeywordsOwned` and
  `_PyArg_UnpackPositionalOwned`, of
  `_PyArg_FormatResources_Begin/Register/Finish`, and of
  `_PyArg_ParseTupleOwned`.
- **Python/getargs.c.**
  - Delete `flat_format_supported` (439), `_PyArg_ParseTupleOwned` (467),
    lines 2613–3098 (`_PyArg_OwnerScope_BeginTuple` through
    `arg_scope_keep_keyword`), and `_PyArg_UnpackKeywordsOwned` (3327).
  - Remove the `tuple_scope` parameter of `vgetargs1_impl`, its branch at
    379–385, and the `PyErr_Occurred()` test it adds to the conversion loop.
  - Remove the `scope` parameter of `unpack_keywords` and its materialize
    block (3210–3218).
  - Make `find_keyword` and `find_keyword_str` return borrowed references
    (the upstream form), and keep only dict-derived values on the stack.
- **Templates.** Mark the four `build/pyvalue-format-*-design-20261103-v1`
  directories as superseded in the native-parsers status. Leave the files
  in place.
- **Docs, for the coordinator.** Point the parser paragraphs of
  extension-api.md and stages/01-runtime/workstreams/native-boundaries.md
  to this record.

### Packet order

| Packet | Content | Size | Status |
|---|---|---|---|
| P1 | Borrow stack core + getargs.c (§4) | < 2 h | Landed with P2 in `acbdf45627d` |
| P2 | Entry points (§3) and the public `PyBorrowScope` | ~1 h; landed together with P1 | Landed with P1; the ceval.c, bytecodes.c and vectorcall-dispatch sites are in progress |
| P3 | Delete OwnerScope; Clinic producer and regeneration; 9 handwritten files | ~2 h | In progress |
| P4 | After link: interpreter argument windows move onto the stack; delete `_PyObjectArray_FromValueArray/_Free` and `MAX_VALUE_SCRATCH`; measure | ~2 h | Not started |

## 6. Tests (once the runtime links)

Add `_testinternalcapi.borrow_depth()`, which returns the number of live
entries on the current thread's stack. Add each new C helper to
Modules/_testinternalcapi (or Modules/_testcapi/getargs.c for the public API
cases). Put the tests in `Lib/test/test_capi/test_borrow_scope.py`.

1. **Inline `O` from a tuple survives reuse.** The helper `parse_OO_then_churn(t)` runs
   `PyArg_ParseTuple(t, "OO", &a, &b)`, then `PyGC_Collect()`, then allocates
   and frees 10⁴ floats, then returns `Py_BuildValue("(OO)", a, b)`. Call it
   with `(1, 2.5)` and with `(2**40, "x")`; expect the input back. A
   use-after-free shows up as garbage in debug builds and as a fault under
   ASan.
2. **Inline keyword from a dict.** Use the same helper over
   `PyArg_ParseTupleAndKeywords((), {"x": 7, "y": 1.5}, "OO", ...)` and over
   `_PyArg_ParseTupleAndKeywordsFast`.
3. **Release on return.** A Python loop calls a METH_VARARGS parser 10⁵
   times with inline arguments; `borrow_depth()` is the same before and
   after.
4. **Loops in C.** `parse_pairs(list_of_pairs, scoped)` parses each element
   with `"OO"` and returns the depth delta it saw just before returning. The
   unscoped delta is `2·n` and the scoped delta is 0. After return, the depth
   is back to baseline.
5. **Entries left by slot code.** A test type in `_testinternalcapi` has an `nb_add`
   slot that parses an internal tuple holding an inline int with `"O"`. A
   Python loop `for _ in range(10**5): t + 1` is followed by a depth check:
   it must be back to baseline, which shows the `_Py_HandlePending` release
   works.
6. **Nested sequence.** Run `"(OO)"` over `([1, 2.5],)` and over
   `((1, 2.5),)` with churn as in test 1. The values survive the temporary
   tuple.
7. **Reentrancy.** An `O&` converter calls back into Python, which calls
   another parsing builtin. The outer outputs stay valid, and after the
   callback the depth is back to the outer level, not below it.
8. **Generator resumed from C.** `list(g)`, where each step of `g` calls a
   parsing builtin. After each step the depth is at baseline, and there are
   no leaks under `-R 3:3`.
9. **Error after boxing.** `"Oi"` over `(1, "x")` raises TypeError with the
   original message, the depth is at baseline after return, and the
   exception is unchanged by release.
10. **No eval loop.** A Programs/_testembed.c case calls
    `PyArg_ParseTuple` from `main` inside `Py_BEGIN_BORROW_SCOPE`, checks the
    value after a GC, and checks that the depth after Exit is 0. A second
    case leaves base-region entries and checks that `Py_FinalizeEx` leaks
    nothing (`-X showrefcount`).
11. **Out of memory.** Use `_testcapi.set_nomemory` while
    `_PyBorrow_TupleItems` grows a chunk: expect MemoryError and no leak.
12. **Slot that parses, then runs Python.** An `nb_add` slot parses an internal
    tuple with `"O"`, calls a Python function, and returns. Run
    `for _ in range(10**5): t + 1`. The depth returns to baseline; this checks
    that the nested activation re-arms the release bit. Test 5 alone misses this
    because its `nb_add` does not call Python.
13. **PEP 523 hook.** A `_testinternalcapi` frame evaluator parses a tuple, then
    evaluates a frame that has a backward jump. The hook's output survives.
14. **Debug Exit check.** In a debug build, Exit with a mark above `top` is a
    fatal error with a clear message.
15. **Scope size warning.** In a debug build, a scope that passes the fixed
    count warns once at the push site.
16. **`pickle.loads`.** Loading 10⁵ float-subclass instances leaves the depth
    delta inside `load` at 0 after `load_newobj` gets its scope.
17. **Wrong arity.** A `METH_VARARGS` function called with `*range(10**6)` and a
    wrong arity pushes no run before the TypeError.
18. **Daemon thread at finalization.** A daemon thread inside
    `Py_BEGIN_ALLOW_THREADS` that holds a box does not crash `Py_FinalizeEx`
    under ASan.
19. **Finalization order.** A base-region object with `__del__` runs it before
    module teardown.
20. **Stack switching.** A test that swaps `_PyBorrowStack` per C stack
    (a greenlet stand-in in `_testinternalcapi`) keeps each stack's entries
    live across the switch.
21. **Regression run.** `./python -m test test_capi.test_getargs test_clinic
    test_call test_descr test_capi.test_borrow_scope -R 3:3`.

## Implementation status

As built in the runtime tree, commit `acbdf45627d` ("Keep parsed argument boxes
on a per-thread borrow stack"; P1 and P2 together). Each item below was checked
in source.

1. **Types.** `_PyBorrowChunk` and `_PyBorrowStack` are defined in
   Include/internal/pycore_tstate.h, not in `pycore_borrow.h`, to avoid an
   include cycle: `_PyThreadStateImpl` embeds the stack and `pycore_borrow.h`
   includes `pycore_tstate.h`.
2. **Helpers.** `_PyBorrowStack_Init`, `_PyBorrowStack_Clear` and
   `_PyBorrow_RequestRelease` (exported) are in Python/borrow.c. The empty
   chunk is the file-static `empty_chunk`.
3. **Re-arm on eval exit.** `_PyEval_EvalFrame` re-arms
   `_PY_BORROW_RELEASE_BIT` through `_PyBorrow_RequestRelease` when `top` is
   above the restored outer floor (Include/internal/pycore_ceval.h). This is
   review item 3, landed.
4. **Release order.** `_Py_HandlePending` releases after the GC step and before
   the GIL-drop and async-exception steps, and tests the bit again at that
   point (Python/ceval_gil.c).
5. **Thread-state clear.** `PyThreadState_Clear` calls `_PyBorrowStack_Clear`
   twice, because finalizers that Clear runs can push (Python/pystate.c).
6. **Spare chunk.** Only a standard 1024-slot chunk is kept as `spare`
   (`borrow_chunk_retire`).
7. **Packet order.** P1 and P2 landed together, so review item 7 is resolved.
8. **GC verifier.** `_PyGC_TraceVisitThreadState` visits the stack entries for
   the tracing verifier (Python/gc_trace.c).
9. **getpath.** `getpath_joinpath_impl` was ported to `PyTuple_GET_VALUE`, the
   last other link blocker (Modules/getpath.c).

Review items, by state:

| State | Items |
|---|---|
| Landed | 3 (re-arm), 7 (resolved) |
| In progress (P3 and the changes below) | 1, 4, 5, 6, 8, 9 |
| Specified here, not built | 2 (stack switching) |
| Documented here | 10, 11, 12 |

## Open questions

1. **Cost of the legacy VARARGS path.** Two costs need measuring on a
   METH_VARARGS-heavy benchmark after link:
   - **Eager boxing.** `_PyBorrow_TupleItems` boxes every inline item it
     reads, so a `"ii"` parse over inline ints allocates two boxes that
     numeric units do not need.
   - **Spurious eval-breaker checks.** `_PyBorrow_Keep` and
     `_PyBorrow_TupleItems` set the release bit even when a trampoline will
     release the entries first.

   Two refinements keep the same contract:
   - In `vgetargs1_impl`, read items lazily and box only for `O`-family
     units.
   - Have trampolines publish their mark, so a push can tell whether the
     innermost scope is an eval activation.

   Record the numbers on the four benchmarks named in §1 "Why" item 3; do not
   tune before then.
2. **JIT exits.** In the interpreter, `_Py_HandlePending` runs only at
   instruction boundaries (verified, §3). When the shared JIT emits its own
   periodic checks, they must keep that property.
3. **What else moves onto the stack.** P4 moves the interpreter's argument
   windows. The `_PyValueTemps` operand boxes could follow, but they share a
   protocol with result publication (`pending_local`) and may stay separate.
   The `_PyNativeScope` invocation transaction (Python/native_scope.c) also
   owns an argument vector and should be reconciled in P4.
4. **How many parse calls are in input-sized loops (rule L).** A
   parser-aware scan is needed; a lexical census will not answer this.
5. **Public API.** `PyBorrowScope` and the macros are in Include/cpython (not
   the limited API) as landed in P2. Limited-API (abi3) extensions therefore
   have no remedy under rule L. Decide whether to add `PyBorrowScope` or an
   equivalent to the limited API.
