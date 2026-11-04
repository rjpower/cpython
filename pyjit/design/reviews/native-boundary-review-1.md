# Review 1: native boundary (borrow stack)

Document: [`../native-boundary.md`](../native-boundary.md) (uncommitted, 2026-11-03).
Code: `build/worktrees/pyvalue-rc-universal` at `c7194ad146e`, plus the uncommitted P1
draft in that tree (`Python/borrow.c`, `Include/internal/pycore_borrow.h`, the
`pycore_tstate.h` and `pystate.c` edits). `Python/getargs.c` line numbers refer to
`HEAD`, because P1 is editing the working copy. Other paths are relative to that tree.

**Verdict: accept with changes.** The core idea is right. The native frame is the
lifetime; the runtime brackets it; loops get an explicit scope. JNI uses the same
model, and it removes 736 call-site edits and a public signature break. The getargs
plan (§4) is correct and can be built as written. But the document overstates three
properties:

- **Soundness.** It holds only under an invariant the document never states (see
  §1.1). Two known code paths break that invariant: PEP 523 hooks, and greenlet-style
  stack switching.
- **Growth bound.** The bound is transitive, not per body. One bug in the eval-breaker
  protocol also strands entries for the life of an activation.
- **Cost.** The hot-path claim ("2 loads + 1 compare") holds only for calls that push
  nothing. Calls that push also pay two atomic read-modify-writes and an out-of-line
  `_Py_HandlePending`.

None of these needs a different design. Section 6 lists the required changes in
priority order.

## 1. Soundness

### 1.1 The invariant the eval-breaker release depends on

The instruction-boundary release (`_Py_HandlePending` → `Exit(eval_floor)`) is safe
only if:

> When `_Py_HandlePending` runs, no live C frame holds a mark or a borrowed pointer
> above `eval_floor`. "Live" means a C frame between the point that set the floor and
> the current instruction.

In the tree, `_Py_HandlePending` (Python/ceval_gil.c:1364) is called only from
`check_periodics` and `check_periodics_at_end` (Python/ceval_macros.h:533, 543). Those
run from bytecode handlers, the generated IR and JIT helper cases, and the end of the
call transaction (Python/ceval.c:2010, after `_PyNativeScope_End`). In all of these,
the native callee has already returned. The invariant therefore holds for the default
evaluator, with the two exceptions in rows 2 and 3 below.

If the invariant breaks, the failure is worse than a dangling borrow. Release
(`Python/borrow.c:_PyBorrowScope_Release`) leaves `top` below a live mark. When that
mark's owner later calls Exit, the test `top != mark` sends Release down from `top`
looking for a mark above it:

1. It frees entries that belong to outer scopes.
2. It pops past the bottom chunk.
3. The draft's `assert(done != &empty_chunk)` catches this in debug builds. Release
   builds dereference `chunk == NULL`.

§1 "Why" item 4 ("a missing scope causes retention, not a dangling pointer") is true
for a missing scope. It is false for a misplaced floor or an Exit that breaks LIFO
order.

### 1.2 Scenarios

| # | Scenario | Handled? | Design section | Notes |
|---|---|---|---|---|
| 1 | Slot code called from a bytecode handler (no trampoline) parses, then calls back into Python and keeps using the borrow | **Memory-safe; retention bug** | §2 "Eval-breaker bit", §3 "Reentrancy" | The nested `_PyEval_EvalFrame` floor sits above the slot's entries, so they survive. But the eval-breaker bit is per thread. The nested activation's first `_CHECK_PERIODIC` clears it after releasing only to its own floor. Back in the outer activation, the bit is clear, so `JUMP_BACKWARD` never releases the parked entries. A loop such as `for _ in range(N): t + 1` keeps N·k entries until the outer activation exits, if `nb_add` parses and then runs any Python code (a call, `__del__`, a GC callback). Test 5 misses this because its `nb_add` does not call Python. |
| 2 | Eval-breaker release while a C frame between the floor and the evaluator holds a borrow | **Not handled** | §3 row `_PyEval_EvalFrame` | The floor is set in the inline wrapper (Include/internal/pycore_ceval.h:119). A PEP 523 hook then runs C code before it calls `_PyEval_EvalFrameDefault` itself. Examples: Python/perf_trampoline.c:464-470, `Modules/_testinternalcapi.c:record_eval`, `Modules/_testinternalcapi/interpreter.c:20`, and any third-party frame evaluator. Anything the hook pushes sits above the floor, and the first instruction boundary releases it. The section claims hooks are "covered"; the floor is in the wrong place for that. |
| 3 | C-stack switching inside one thread state (greenlet, gevent, eventlet) | **Not handled** | none | Greenlet A's `switch` trampoline takes mark mA. Greenlet B pushes above it, then switches back while B's own `switch` trampoline (mark mB) is still live. A returns and calls Exit(mA), which releases B's live entries. When B resumes, Exit(mB) finds `top` below mB and corrupts the stack (§1.1). Greenlet already saves and restores `tstate->datastack_*` for the same reason, so the borrow stack (including `eval_floor`) needs the same per-greenlet treatment. |
| 4 | Generators and coroutines suspended with borrows | Handled | §3 "Generators and coroutines" | Correct. Native code cannot suspend. `gen_send_ex2` enters through `_PyEval_EvalFrame` (Objects/genobject.c:299). `SEND` and `FOR_ITER_GEN` add no C frame. A C iterator that keeps a parse output across `tp_iternext` calls is an escape (rule E). |
| 5 | Signal handlers | Handled | §3 "covered" | Python handlers run in nested activations, from `_Py_HandlePending` or `PyErr_CheckSignals` in C. The C-level handler never touches the stack. |
| 6 | `Py_AddPendingCall` callbacks | Handled; retention only | §3 "atexit and Py_AddPendingCall" | The document names only `_Py_HandlePending`. Pending calls also run from `Py_MakePendingCalls` inside native bodies (Python/lock.c:136, Modules/_threadmodule.c:586, Modules/_queuemodule.c:434, Python/thread.c:232). Their pushes stay in that body's scope. |
| 7 | GC callbacks and finalizers that parse arguments | Handled; retention only | §3 "Finalizers, GC and weakref callbacks" | GC started from `_Py_HandlePending` runs before the release step, so its pushes are released. GC from `gc.collect()` runs inside a trampoline. GC from `PyGC_Collect()` in C pushes into the caller's scope. |
| 8 | Thread switch with borrows outstanding (GIL released) | Handled | §3 "Threads and builds" | Each thread has its own stack and the entries are owned. This is stronger than upstream: another thread mutating a kwargs dict can no longer free a parsed value. |
| 9 | Daemon threads at `Py_FinalizeEx` | **Not handled** | §3 row pystate | `interpreter_clear` (Python/pystate.c:757) clears every thread state, including daemon threads that may be inside `Py_BEGIN_ALLOW_THREADS`. Upstream leaves such a thread's frames alone (warning at pystate.c:1696), so its objects leak. Here `_PyBorrowStack_Clear` frees boxes and objects that GIL-free code in that thread may still read. |
| 10 | Subinterpreters | Handled | §3 "Threads and builds" | The stack is per thread state. `_PyXI_Enter` swaps to another thread state with its own stack. One gap: Exit uses the trampoline's cached `tstate`, while Keep uses `_PyThreadState_GET()`. A debug assert that they match would catch a body that returns with a different thread state current. |
| 11 | `Py_FinalizeEx` | **Partly** | §2 rule 9, §3 rows pystate and pylifecycle | (a) The base region is released inside `interpreter_clear`, after `finalize_modules` (Python/pylifecycle.c:2485) and the last GC. Retained objects therefore run `__del__` after module teardown, a new class of late finalization. (b) Nothing stops a push onto a thread state that has already been cleared. Later deallocators in `interpreter_clear` can allocate a chunk that is never freed. (c) An embedder scope that spans `Py_FinalizeEx` calls Exit on a freed thread state. |
| 12 | Embedders calling `PyArg_*` with no frame | Handled; growth unbounded without a scope | §2 rule 9, §5 rule L | Same as JNI local references on an attached thread before `DetachCurrentThread`. The remedy (`PyBorrowScope`) is in `Include/cpython`, so limited-API (abi3) extensions have none. |
| 13 | Reentrant deallocators during Release | Handled | §2 "Release" | The loop re-reads `top` and `chunk` on each iteration, and `--top` happens before `Py_XDECREF`, so a deallocator's pushes reuse the popped slot. A nested activation's floor is the current `top`. This is correct. |
| 14 | `longjmp` across an activation (skips the floor restore) | Not handled | none | Already undefined behaviour in CPython (frames, recursion depth). Worth one sentence. |

## 2. Entry points (§3)

The interpreter, `Objects/` and `Modules/` were searched for direct calls to
`ml_meth`, `tp_call`, `tp_new`, `tp_init` and `vectorcall`.

| Path | Example site | In §3? | How its pushes are released | Gap |
|---|---|---|---|---|
| PyCFunction vectorcall (FASTCALL, KEYWORDS, METHOD, NOARGS, O) | Objects/methodobject.c `cfunction_vectorcall_*` | Yes | Trampoline | none |
| PyCFunction `tp_call` (VARARGS) | methodobject.c `cfunction_call` | Yes | Trampoline | none |
| Interpreter native call transaction | methodobject.c `_PyCFunction_CallScoped`; native_scope.c `_PyNativeScope_InvokeObjectVector` | Yes | Trampoline | none |
| Method descriptors | Objects/descrobject.c `method_vectorcall_*`, `classmethoddescr_call` | Yes | Trampoline | none |
| Slot wrappers | descrobject.c `wrapperdescr_raw_call` | Yes | Trampoline | Wording: only 6 `wrap_*` call `PyArg_UnpackTuple` (typeobject.c:12002-12476); the rest use owning getters |
| `type_call` and `tp_new_wrapper` | typeobject.c:2836, 2848, 12590 | Yes | Trampoline | none |
| `_PyObject_MakeTpCall` and `_PyObject_Call` (`tp_call` branch) | Objects/call.c | Yes | Trampoline | none |
| Tuple to vector bridges | call.c `_PyVectorcall_Call`, `_PyObject_Call_Prepend`, `_PyObject_CallFunctionVa`, `callmethod`; `Modules/_operator.c:methodcaller_vectorcall` | Yes | Explicit scope | none |
| Specialized CALL handlers that call a native body directly | Python/bytecodes.c:4552 (`_Py_CallBuiltinClass_Value` → `tp_vectorcall`), 4648 (`_Py_BuiltinCallFast_Value`), 4686 (`_Py_BuiltinCallFastWithKeywords_Value`), 4847, 4908 (`_PyCallMethodDescriptorFastWithKeywords_Value`), 4961, 5016 (`_PyCallMethodDescriptorFast_Value`); helpers at Python/ceval.c:1118-1214 | **Only 4847 and 4961** | Eval-breaker bit | These are the hot builtin-call paths. A FASTCALL body that parses a tuple argument (e.g. `PyArg_ParseTuple(arg, "ii")` in a METH_O function) costs an atomic RMW plus `_Py_HandlePending` per call. |
| Generic vectorcall dispatch to non-cfunction callables | Include/internal/pycore_call.h:143 `func(callable, args, nargsf, kwnames)`: builtin-type `tp_vectorcall`, `operator.*getter`, third-party function objects (Cython, nanobind) | **No** | Bit when called from bytecode; the caller's scope when called from C | §2 rule 2 says every call "through the call protocol" opens a scope. This path is the call protocol and opens none. |
| Direct `tp_new` / `tp_init` calls | Modules/_pickle.c:6657 `load_newobj`; Modules/_threadmodule.c:1747; typeobject.c:6091; Modules/_collectionsmodule.c:2488 | **No** | The caller's scope | `load_newobj` calls a C `tp_new` once per NEWOBJ opcode inside one `load()` body. A Clinic `tp_new` with a tuple receiver (`float_new` for a float subclass) pushes once per object, so `pickle.loads` of N float-subclass instances keeps N entries and N boxes until it returns. The other sites are bounded or run inside an existing scope (6091 is `type_new_get_bases`, inside `type_call`). |
| Slots called from C loops: `tp_iternext`, `tp_richcompare`, `tp_hash`, `nb_*`, `sq_*`, `mp_*`, `tp_getattro`, getset get/set, `tp_descr_get`/`set`, `tp_repr`, `bf_getbuffer`, `am_send` | e.g. Python/bltinmodule.c:1620 (`map_next`), 3312 (`zip_next`), which call each iterator's `tp_iternext` and are themselves driven from C loops; `list.sort` → `tp_richcompare` | No (by design) | Bit when called from bytecode; the caller's scope when called from C | Memory-safe. Unbounded only when such a slot parses a tuple; with eager boxing, any tuple parse pushes. |
| getset and member descriptors | descrobject.c `getset_get`, `getset_set` | "Do not parse" | as slots | The claim is false as stated. Setters do parse, e.g. `PyArg_Parse(arg, "i")` in Modules/_ssl.c:`set_min_max_proto_version`. Compat-mode parses of a scalar push nothing, so restate the claim as that rule. |
| Finalizers, `tp_dealloc`, weakref callbacks | anywhere | "Covered" | The scope that triggers them | Correct. |
| atexit | `atexit_callfuncs` → `PyObject_Call` | "Covered" | Trampoline | Correct. |
| Module init and exec | `_imp.create_dynamic`, `_imp.exec_dynamic`, `_imp.create_builtin` | "Covered" | Trampoline; base region during `pyinit_core` | Correct. |
| Thread bootstrap, `PyGILState_Ensure` callbacks (ctypes, sqlite, tkinter) | `thread_run`, `_CallPythonObject` | Implicit | Trampoline for the call itself; base region for conversion code | Correct. |
| PEP 523 hook body | perf trampoline, `record_eval` | "Covered" | **Wrong floor** | See §1.2 row 2. |
| C trace and profile functions (`PyEval_SetTrace`) | called inside instrumented instructions | Not mentioned | Bit | Correct, but worth naming. |

**Conclusion.** Every path is memory-safe except the PEP 523 hook and stack-switching
cases. Bounded release is guaranteed only for:

- bodies dispatched through the listed trampolines;
- code called directly from bytecode, once the bit protocol is fixed.

It is not guaranteed for slot, `tp_new` or vectorcall bodies called from C loops.

## 3. Cost

### 3.1 Per operation

| Operation | Upstream | A. Explicit OwnerScope | B. Borrow stack as written |
|---|---|---|---|
| Native call that pushes nothing (most FASTCALL, METH_O, NOARGS) | 0 | 0 | 2 loads, 1 compare, and a mark kept in a callee-saved register. Matches §1. |
| METH_VARARGS call `f(a, b)` from bytecode, parse `"OO"` | 0 (borrows from the tuple) | 20-field struct init, n INCREF or box, End | n INCREF or box, n DECREF or free, an out-of-line call to Release, **an atomic OR** on `eval_breaker` (`_Py_set_eval_breaker_bit`, pycore_ceval.h:367), then after the call **`_Py_HandlePending` plus an atomic AND**. §1 and the options table omit the last three. |
| Specialized CALL whose body parses a tuple | 0 | scope per parse | Same push, atomic and `_Py_HandlePending` costs as above (no trampoline). |
| `tp_new`/`tp_init` through `type_call` with a tuple (e.g. `datetime(2020, 1, 1)`) | 0 | scope | As VARARGS; 2020 is not a cached small int, so it is boxed and then unboxed by `"i"`. |
| `PyObject_Call(f, tuple)` to a vectorcall callee | 0 (`_PyTuple_ITEMS`) | OwnerScope and view | n INCREF or box and n DECREF, plus the atomic when an eval loop is active. Unavoidable with PyValue tuples. |
| FASTCALL\|KEYWORDS call with keywords, P1 to P3 interim | 0 | 0 | One extra push per keyword: `find_keyword` returns `Py_NewRef`, and §4 steps 4, 6 and 9 Keep it. Also one atomic. Avoidable; see §6 item 8a. |
| Eval activation entered from C (Python function called from C; generator resumed from C by `list(g)`, `sum(g)`, `"".join(g)`) | 0 | 0 | About 3 loads, 2 stores, 1 compare, and the saved floor kept live across the call. |
| Bytecode periodic check | 0 | 0 | 0 unless the bit is set. Holds. |
| Each item read by `_PyBorrow_TupleItems` | n/a | n/a | `PyValue_AsObject` (Python/pyvalue.c:39) calls `PyErr_Occurred()` per item. Use `PyValue_AsPyObjectNewBoxed` directly. |

The design's own refinements (open question 1) target the two largest extra costs. Its
"record, don't tune" stance fits the project's rule against intermediate performance
gates. The cost statements in §1 "Why" item 3 and the options table must still be
corrected, and the benchmarks named (§6 item 10).

### 3.2 Growth

| Where the parse runs | Upstream | A | B |
|---|---|---|---|
| Native body | 0 | Until End | Until the body returns |
| Loop of parses in one body | 0 | Exact, if End is inside the loop | N·k until return (rule L). A lexical census finds at least Modules/socketmodule.c:4663 and 5108, Modules/_ctypes/_ctypes.c:3963, Modules/_ctypes/stgdict.c:103, Modules/_winapi.c:2426 and 2644, and Modules/_io/textio.c:2008, reached from the `read(n)` loop at 2173 and the `readline` loop at 2347. |
| Slot called from bytecode | 0 | Exact | Until the next periodic check. **Until the activation exits** if any Python code ran in between (§1.2 row 1). |
| Slot, `tp_new` or vectorcall body called from a C loop | 0 | Exact | N·k until the C caller's scope exits (e.g. `pickle.load`). §2 rule 10 says this cannot happen. |
| Embedder, no scope | 0 | Exact | Thread lifetime |
| After P1 alone (no trampolines yet) | n/a | n/a | **Every parse, for the thread's lifetime** (§6 item 7) |

## 4. Getargs plan (§4)

| Step | Correct? | Notes |
|---|---|---|
| 1 `vgetargs1` | Yes | Pushes and boxes before the arity check in `vgetargs1_impl` (getargs.c:364). A wrong-arity call, or `f(*range(10**7))` to a VARARGS function, boxes every item before raising. Upstream allocates nothing. Push after the check, and only `min(nargs, max)` items. |
| 2 `converttuple` | Yes | Dropping the temporary tuple after the run takes its references is right. It also fixes the upstream case where a sequence creates items on access and the pointers dangle. The `items == NULL` return relies on `seterror` deferring to the pending exception, as the upstream `PySequence_Tuple` failure path already does. |
| 3 `find_keyword_str` | Yes | |
| 4 `vgetargskeywords_impl` | Yes | Keeping dict values fixes a real dangling box. Today the code DECREFs at 1934 while the outputs still point at a fresh box from `PyDict_GetItemStringRef`. The extraneous-keyword loop keeps up to `len(kwargs)` keys on the error path; add that to rule 10. Values from `find_keyword_str` are already owned by the caller's `kwstack`; DECREF them back to borrowed instead of Keeping them (§6 item 8a). |
| 5 `vgetargskeywords` | Yes | Same arity remark as step 1. |
| 6 `vgetargskeywordsfast_impl` | Yes | `find_keyword` (getargs.c:2339) can raise, so the extra `PyErr_Occurred()` check is needed. Same `kwstack` remark as step 4. |
| 7 `vgetargskeywordsfast` | Yes | |
| 8 `PyArg_UnpackTuple` | Yes | Outputs are backed by the scope. |
| 9 `unpack_keywords`, `scope == NULL` branch (3254-3260) | Yes | Fixes a real dangling box for unscoped Clinic `tp_new`/`tp_init`. Keep only values from the `kwargs` dict; the `kwnames` branch can DECREF as upstream does. |
| Clinic tuple receivers (§5) | **Incomplete** | `select_positional_convention` emits `PyTuple_GetItem(args, %s)` under `limited_capi` (Tools/clinic/libclinic/parse_args.py:1277-1278). That function no longer exists in this tree. Specify the limited-API form; `PyArg_UnpackTuple` is backed by the scope and public. In the keyword path, "emit first" pushes before arity is checked and includes the `*args` tail. Push `min(nargs, maxpos)`. |

**Escapes.** No function in the plan returns a borrowed pointer that escapes the
scope:

- `es`, `et`, `es#`, `et#`, the `*` buffer formats and `O&` results stay owned
  (rule 6).
- A search for converters that store a borrowed `PyObject *` found none.
  `path_converter` takes a reference (Modules/posixmodule.c, "Own a reference to the
  object"), and the `unicodeobject.c` converters store owned outputs.
- `PyArg_UnpackTuple` outputs (34 calls) are backed by the scope. A spot check of the
  `->field = obj` assignments near parses found only owned values (e.g.
  `StopIteration_init` now uses `PyTuple_GetItemRef`).

The real exposure is outside getargs. A non-box object stays alive through its source
tuple or dict, so an escape bug shows up only when the value was inline. Rule E's
lexical audit will miss escapes through helpers, globals and capsules. Running the
suite under ASan with inline-valued inputs, as test 1 already does, is the detection
mechanism; say so in §5.

`_PyBorrow_TupleItems` failure modes. §2 says MemoryError only. In fact it also
returns NULL with SystemError for a non-language value, or with an exception that was
already pending (`PyValue_AsObject`). Fix the contract or the primitive.

## 5. Comparison with V8, JNI and HPy

| Property | This design | JNI local frames | V8 HandleScope | HPy |
|---|---|---|---|---|
| Who opens the implicit scope | Runtime trampolines and eval activations | VM native-method stub, the single dispatch point | V8 around each API callback | Argument handles are valid for the call |
| Explicit scope | `Py_BEGIN_BORROW_SCOPE` | `PushLocalFrame`/`PopLocalFrame` | `HandleScope` (required) | `HPyTracker`, passed to `HPyArg_Parse` only when the format can create handles |
| Single dispatch point for native code | **No**: slots are called from many places, hence the eval-breaker sweep and the transitive bound | Yes | Yes | Yes (the ctx) |
| Asynchronous release at safepoints | Yes (novel) | No | No | No |
| Handles outside any scope | Accepted silently (base region) | Leak until `DetachCurrentThread` | Fatal in debug ("Cannot create a handle without a HandleScope") | n/a |
| Measured limit or diagnostics | None specified | 16 guaranteed; `EnsureLocalCapacity`; `-Xcheck:jni` warns past capacity | Debug fatals; `SealHandleScope` forbids handle creation in a region | Debug mode detects leaked and closed-then-used handles |
| Escaping a value | `Py_NewRef` | `NewGlobalRef`, `PopLocalFrame(result)` | `EscapableHandleScope::Escape` | `HPy_Dup`, `HPyField`, `HPyGlobal` |
| Per-coroutine state | Unspecified | Per thread | Per isolate and thread | Per context |

Lessons:

- **(a)** The closest precedent is JNI. JNI pairs the implicit frame with a capacity
  and a checker. This design should add the checker: a per-scope high-water warning in
  debug builds, and a depth assert on Exit (§6 items 3 and 4).
- **(b)** HPy's `HPyArg_Parse(ctx, ht, ...)` is option A with an optional owner (NULL
  when the format cannot create handles). That answers §1's "scopes with no boxes"
  complaint about A. B remains the better bridge for rebuilt C-API sources, because it
  needs no source edits. HPy stays the target for ported extensions, as the document
  says.
- **(c)** None of the precedents releases asynchronously. That mechanism carries the
  soundness risk here, so its invariant must be written down and asserted.

## 6. Required changes, in priority order

1. **§3 row `_PyEval_EvalFrame`; new invariant paragraph in §2.**
   - **Change:** Set and restore `eval_floor` where `_PyEval_EvalFrameDefault` pushes
     and pops its entry frame (or in a thin wrapper around the loop), not in the
     `_PyEval_EvalFrame` inline wrapper. State the §1.1 invariant explicitly.
   - **Reason:** PEP 523 hooks and the perf trampoline run C code between the
     wrapper's floor and the first instruction boundary. Anything they push is
     released while still live, which corrupts Exit (§1.1).
2. **New §3 paragraph "Stack switching".**
   - **Change:** Code that switches C stacks within one thread state must give each
     stack its own `_PyBorrowStack` and swap it like `datastack_*`. Provide internal
     save and restore functions, and put greenlet on the port list with this
     requirement.
   - **Reason:** Without this, greenlet A's trampoline Exit releases greenlet B's live
     entries (§1.2 row 3).
3. **§2 "Eval-breaker bit" and §3 row `_PyEval_EvalFrame`.**
   - **Change:** After restoring the outer floor on activation exit, re-arm
     `_PY_BORROW_RELEASE_BIT` if `top != eval_floor`. Extend test 5 with an `nb_add`
     that calls a Python function after parsing.
   - **Reason:** The bit is per thread, but the parked state belongs to each
     activation. A nested activation clears the bit and strands the outer
     activation's entries until it exits (§1.2 row 1).
4. **§2 Release and Exit; §1 "Why" item 4.**
   - **Change:** In debug builds, keep a depth (a per-chunk base depth plus the
     offset), assert `depth(mark) <= depth(top)` in Exit, and fail fatally with a
     clear message. Reword item 4: a missing scope causes retention, but a misplaced
     floor or an Exit out of LIFO order frees live entries.
   - **Reason:** A stale mark currently over-releases and then dereferences NULL in
     release builds (§1.1).
5. **§2 rule 10, §1 "Why" item 2 and §2 "Limits".**
   - **Change:**
     - Restate the bound as transitive. A scope collects every push made while it is
       innermost, including pushes from slot code, getset functions, converters,
       direct `tp_new`/`tp_init` calls and non-cfunction vectorcall bodies called
       from C.
     - Scope `Modules/_pickle.c:load_newobj` explicitly.
     - Add the measured limit: in debug builds, warn once per site when one scope
       exceeds a fixed count (JNI `-Xcheck:jni` analogue), naming the push site.
     - List the §3.2 loop sites as the starting point for the rule-L census.
   - **Reason:** The current bound is false (`pickle.loads` of float-subclass
     instances), and the owner's standard asks for stated, measured limits.
6. **§3 table.**
   - **Change:** Name the five direct-call helpers in Python/ceval.c:1118-1214 and
     the seven bytecodes.c sites, not just 4847 and 4961. Recommended: wrap them with
     Enter and Exit, which costs 2 loads and a compare. Then rule 2 is literally true
     for calls from the interpreter, and the bit is left only for slot code.
   - **Reason:** These are the hot builtin-call paths. As written, any push from them
     costs an atomic pair plus `_Py_HandlePending`.
7. **§4 P1c and the packet table.**
   - **Change:** Move the trampoline, `_PyEval_EvalFrame` and `_Py_HandlePending`
     edits into P1, or limit P1c to compile plus tests 1, 2 and 6.
   - **Reason:** Tests 3 and 9 check depth after return, which needs P2's scopes.
     Without them every VARARGS parse stays in the base region for the life of the
     thread, and `-R 3:3` reports leaks.
8. **§4 steps 1, 4, 5, 6 and 9; §5 Clinic; §2 `_PyBorrow_TupleItems`.**
   - **Changes:**
     - (a) Keep only values that come from a dict. `find_keyword` and
       `find_keyword_str` results are owned by the caller's `kwstack`, so DECREF them
       back to borrowed.
     - (b) Push tuple runs after the arity check, and only `min(nargs, max)` items:
       give `_PyBorrow_TupleItems` a prefix length.
     - (c) State every failure mode of `_PyBorrow_TupleItems`, and use the
       non-checking box primitive.
   - **Reasons:** a push and an atomic per FASTCALL keyword argument until P3;
     transient memory proportional to `*args`; a wrong contract.
9. **§3 pystate and pylifecycle rows; §2 rule 9.**
   - **Changes:**
     - Release the main thread's base region in `Py_FinalizeEx` and
       `Py_EndInterpreter` after atexit and before `finalize_modules`.
     - Assert that nothing pushes onto a cleared thread state.
     - When clearing another thread's state that still has a frame (daemon threads),
       leave its entries, matching upstream's treatment of frames.
     - State that an explicit scope must not span `Py_FinalizeEx`.
   - **Reason:** §1.2 rows 9 and 11.
10. **§1 "Why" item 3, the options table, open question 1.**
    - **Change:** Replace "2 loads + 1 compare per native call" with the §3.1 figures
      for calls that push. Name the benchmarks to record:
      - a METH_VARARGS microbenchmark;
      - `datetime(2020, 1, 1)` construction;
      - `PyObject_Call` with a tuple of inline ints;
      - `list(genexpr)` for the cost of entering the evaluator.
    - **Reason:** The stated cost is accurate only for calls that push nothing.
11. **§5 Clinic; open question 5.**
    - **Change:** Specify the limited-API emission for tuple receivers. Note that
      abi3 extensions have no remedy under rule L unless `PyBorrowScope` (or an
      equivalent) is in the limited API.
    - **Reason:** parse_args.py:1277-1278 emits a function that no longer exists.
12. **Wording.**
    - §3 "getset and member descriptors do not parse": setters do parse, but a
      compat-mode parse of a scalar pushes nothing.
    - §3 slot-wrapper row: only 6 `wrap_*` functions call `PyArg_UnpackTuple`.
    - §3 pending calls: they also run from `Py_MakePendingCalls` in C (§1.2 row 6).
    - §3: name C trace functions as covered by the bit.

## 7. Tests to add to §6

| Test | Checks |
|---|---|
| `nb_add` parses, calls a Python function, returns; loop 10^5 times | Depth returns to baseline (change 3) |
| PEP 523 hook (`_testinternalcapi`) that parses a tuple, then evaluates a frame with a backward jump | The hook's output survives (change 1) |
| Debug build: Exit with a mark above `top` | Fatal error with a clear message (change 4) |
| `pickle.loads` of 10^5 float-subclass instances | Depth delta inside `load` is 0 after change 5 |
| VARARGS function called with `*range(10**6)` and a wrong arity | No run is pushed before TypeError (change 8b) |
| Daemon thread inside `Py_BEGIN_ALLOW_THREADS` holding a box at finalization | No crash under ASan (change 9) |
| Base-region object with `__del__` at finalization | `__del__` runs before module teardown (change 9) |
