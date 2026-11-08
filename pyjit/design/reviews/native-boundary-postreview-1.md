# Post-implementation review 1: native boundary (borrow stack)

Design: [`../native-boundary.md`](../native-boundary.md) and
[review 1](native-boundary-review-1.md). Code: `build/worktrees/pyvalue-rc-universal`,
commits `acbdf45627d` (P1+P2), `56ff66a596b` (P3 Clinic, review follow-ups, tests),
`643c69c7244` and `6d39221996d` (regenerated Clinic output). Paths below are relative
to that tree. Reviewed: the hand-written code in those commits, the Clinic producer
(`Tools/clinic/libclinic/`), and these generated samples:

- `Modules/_io/clinic/bufferedio.c.h` (`BufferedRWPair`, a tp_init without keywords);
- `Modules/_sqlite/clinic/row.c.h` (a tp_new);
- `Objects/clinic/typevarobject.c.h` (a tp_new with keywords and `*args`);
- `Modules/_ctypes/clinic/cfield.c.h` (keywords, `maxpos` 0);
- the limited-API cases in `Lib/test/test_clinic.py`.

The working copy of the tree has uncommitted edits by another agent to `borrow.c`,
`pystate.c` and others. They were not reviewed, except where finding 1 mentions them.

**Verdict.** The release invariant holds on every path I traced. The eval floor is
in `_PyEval_EvalFrameDefault`, a PEP 523 hook has its own scope below that floor,
re-arm and reentrant release are correct, and no tuple run is passed with
`PY_VECTORCALL_ARGUMENTS_OFFSET`. An escape audit found no parse output that outlives
its scope in `Objects/`, `Python/` or the changed modules. Three things need work:

- one finalization bug (the `test_late_untrack` abort);
- one unbounded-retention site (the Unicode error handlers, rule L);
- the tests: one main test is weak, and four §6 tests are missing.

The claim that decision 6 removes the atomic pair on hot paths is false as built
(finding 6).

## Findings

Format: `file:function` — severity — finding — fix.

### Soundness and finalization

1. **`Python/pystate.c:interpreter_clear` and `Python/borrow.c:_PyBorrowStack_Clear`
   — bug — late push after Clear (the `test_late_untrack` abort).**
   `interpreter_clear` calls `PyThreadState_Clear` on every thread state, including
   the current one (pystate.c:757). It then sets `tstate->_status.cleared = 0`
   (:765), because it keeps running finalizers on that thread state: it clears
   `codecs.search_path` and `before_forkers`, then calls `_PyGC_CollectNoFail`.
   `support.late_deletion` keeps the test's `Tracked` object alive in exactly those
   two places. Its `__del__` then calls `_testcapi.tracemalloc_untrack(domain, ptr, 1)`,
   which is METH_VARARGS and parses `"IO|i"` (Modules/_testcapi/mem.c:688).
   `_PyBorrow_TupleItems` must grow a chunk, because Clear already freed `spare`.
   So `borrow_grow` trips `assert(!s->cleared)`. In a release build the trampoline
   scope releases the entries, but the new chunk stays as `spare` and leaks.
   Any METH_VARARGS or Clinic tuple-receiver call from a late finalizer does the same.
   Clear on the current thread state is not terminal, so treat it like
   `_status.cleared`.
   **Fix:** `PyThreadState_Clear` only releases entries; free the chunks when the
   thread state is freed. The in-progress working-copy change
   (`_PyBorrowStack_Fini` frees the chunks and drops the `cleared` flag) has that
   shape. But as written, Fini frees chunks that may still hold entries without
   `Py_DECREF`ing them. It should assert depth 0 in debug builds, or release while
   the thread state can still run code. Add a direct test in
   `test_borrow_scope.py`: a `support.late_deletion` object whose `__del__` calls a
   METH_VARARGS parser, run under `assert_python_ok`.

2. **`Python/borrow.c:borrow_thread_has_frame` (used by `_PyBorrowStack_ReleaseAll`)
   — risk — base-region entries of a frameless foreign thread.**
   Finalization keeps a thread's entries only when it has a Python frame
   (`current_frame != base_frame`). Consider a thread created in C
   (`PyGILState_Ensure`) that parsed in its base region, released the GIL, and is
   still reading the borrowed output when `interpreter_clear` clears its thread
   state from the main thread. Its boxes are freed under it. Upstream keeps such an
   object alive through the tuple the thread owns. This is rare, and the thread
   cannot reacquire the GIL anyway.
   **Fix:** release only the finalizing thread's own base region (and the base
   regions of threads that have exited). Leave every other thread state's entries
   until the thread state is freed, as with frames. Low priority.

3. **`Python/ceval.c:_PyEval_EvalFrameDefault` — verified, with a cost note.**
   - **Floor placement.** The floor is saved and set around `_PyEval_EvalFrameLoop`.
     `_PyEval_EvalFrame` gives a PEP 523 hook its own scope, below the floor that
     the hook's call to `_PyEval_EvalFrameDefault` sets (pycore_ceval.h:120-133).
     The perf trampoline, `record_eval` and the `_testinternalcapi` evaluators all
     enter through `_PyEval_EvalFrameDefault`.
   - **Re-arm and resumption.** Re-arm on exit is correct. Generators resumed from
     C (`gen_send_ex2`) go through the wrapper. No path leaves an activation
     without returning through it; stack switching is still unbuilt (decision 2).
   - **Release callers.** `_Py_HandlePending` has two callers: `check_periodics`
     and `check_periodics_at_end`. The second runs in the call transaction
     (ceval.c:2025), after `_PyNativeScope_End`. The JIT's `_PyJIT_LIR_POLL` slow
     path runs the instruction's handler after `_PyJit_TracePublish`, so it too
     releases at an instruction boundary.
   - **Cost.** The wrapper is a separate non-inlined frame. It costs about
     27 instructions per activation, including the extra call (see the cost
     table). The design estimated "about three loads, two stores and a compare".
     Recording only. If it matters, set and restore the floor inside the loop where
     the entry frame is pushed and popped.

4. **`Python/borrow.c:borrow_release` — verified.**
   - **Reentrancy.** Release reads `top` and `chunk` again on every iteration.
     A C deallocator that pushes without a scope has its entry popped by the same
     loop. A Python `__del__` gets a nested activation whose floor is the current
     `top`, in the middle of the region being released. The nested activation's
     exit re-arms the bit, because `top` is still above the outer floor. That
     costs one extra no-op `_Py_HandlePending`, which is harmless.
   - **Chunks.** Chunk pop and retire during release are safe under LIFO.
   - **Error paths.** In `_Py_HandlePending`, an early `return -1` from signals or
     pending calls leaves the bit set, so the next boundary releases.

5. **`Python/borrow.c:_PyBorrowScope_Release` (debug check) — style.**
   `borrow_depth(s, mark)` checks only that the address lies on the live stack.
   Suppose a stale mark points into a chunk that was retired and then reused as
   `spare` at a different depth. The check passes, and the release pops the wrong
   entries. The design asked for a depth comparison. That needs the depth stored
   with the mark, which costs a word per scope. Accept as is, but say so in the
   design's "Debug checks".

### Entry points and cost

6. **`Python/borrow.c:_PyBorrow_RequestRelease`, called from `_PyBorrow_Keep` and
   `_PyBorrow_TupleItems` — risk (performance) — decision 6 did not remove the
   atomic pair.**
   - **What happens.** Inside any activation, every push does a `lock or` on
     `eval_breaker`: `cmp [rdi+0x430],0; je; lock or [rdi+0x18],0x80` in the
     release build. That includes pushes from bodies whose trampoline or ceval
     helper releases the entries before the next boundary. So every METH_VARARGS
     call with arguments, every Clinic tp_new/tp_init with a tuple receiver, and
     every kwargs-dict keyword still costs an atomic OR, an out-of-line
     `_Py_HandlePending`, and an atomic AND. The release itself is then a no-op.
   - **The JIT.** A set event bit sends `_PyJIT_LIR_POLL` to its slow path, which
     ends in `jit_jmp(b, fallback)` (Python/jit_backend_x86.c:218-238). So a
     compiled loop that calls such a builtin may leave compiled code at every back
     edge. I did not measure this; check what `fallback` costs.
   - **The design.** "Why" item 3 says decision 6 removes this pair. It does not.
   - **Fix:**
     - Correct that sentence in the design.
     - Record the four §1 benchmarks, plus a JIT-hot loop calling `datetime(2020, 1, 1)`.
     - If the numbers warrant it, apply the second refinement of open question 1:
       trampolines publish their mark, and a push requests release only when the
       innermost scope is an eval floor.

7. **`Include/internal/pycore_call.h:_PyObject_VectorcallTstate` — risk (cost),
   correct.**
   - **Redundant scope.** For PyCFunction and method-descriptor callees, this scope
     repeats the callee trampoline's own. A C→builtin call through
     `PyObject_Vectorcall` therefore pays two scopes, about 10 instructions.
   - **No-push case.** For a Python function callee, the scope adds the same five
     instructions in front of the eval floor.
   - **Recommendation.** Keep the scope: rule 2 requires it for third-party
     vectorcall objects. Record the cost.

8. **`Python/ceval.c:_Py_BuiltinCallFast_Value` and the other four helpers — style
   (cost).** The helpers take no `tstate`, so each new scope adds a TLS load of
   `_PyThreadState_GET()` (two instructions) to the five-instruction scope. Pass
   `tstate` from the caller if these show up in profiles. Record only.

9. **Entry-point coverage — verified.**
   - **Trampolines.** Every row of design §3 is wired: the PyCFunction and
     method-descriptor trampolines (all flag forms), `cfunction_call`,
     `_PyCFunction_CallScoped`, `wrapperdescr_raw_call`, `classmethoddescr_call`
     and `type_call` (one scope over tp_new and tp_init).
   - **`call.c`.** `_PyObject_MakeTpCall` and the `tp_call` branch of
     `_PyObject_Call` are wired. So are the tuple-to-vector bridges:
     `_PyVectorcall_Call`, `_PyObject_Call_Prepend`, `_PyObject_CallFunctionVa`
     and `callmethod`.
   - **Interpreter.** The five ceval.c helpers, both bytecodes.c sites (and their
     generated IR copies), and `_PyObject_VectorcallTstate` are wired.
   - **Elsewhere.** Also wired: `methodcaller_vectorcall`, `save_reduce`,
     `load_newobj`, `init_interp_main`, and the finalization release in
     `_Py_Finalize` and `Py_EndInterpreter`.
   - **`tp_new_wrapper`.** It is a METH_VARARGS|METH_KEYWORDS PyCFunction, so
     `cfunction_call` covers it.
   - **`ARGUMENTS_OFFSET`.** No run is passed with this flag:
     - `_PyVectorcall_Call` copies the run through `_PyStack_UnpackDict` before it
       sets the flag;
     - `methodcaller_vectorcall` and `_PyObject_Call_Prepend` copy the run into a
       local array;
     - `callmethod` calls `_PyObject_VectorcallPrepend` without the flag.

10. **Escape audit (`Objects/`, `Python/`, `_pickle`, `_operator`,
    `_testmultiphase`, `_functools`, `itertools`, `_collections`) — no dangling
    escapes found.**
    - **What was checked.** About 50 hand-written parse sites, plus a scan of
      about 675 Clinic `_impl` functions for parameters stored in fields or
      statics, or returned without a new reference.
    - **Notable safe cases.**
      - `oserror_parse_args`: type_call's single scope covers the handoff from
        tp_new to tp_init.
      - `save_reduce_impl`: the memo takes `Py_NewRef`.
      - `parse_weakref_init_args`: it stores `wr_object` without a reference, but
        exact int and float cannot be weakly referenced, so a box is never the
        target.
      - `cm_set_callable` and `sm_set_callable`: the identity check fails for an
        inline argument, because tp_new and tp_init each box it again, so
        `functools_wraps` runs twice. This is harmless.
    - **Outside this design.** The Windows branch of `oserror_parse_args` passes
      `*myerrno` to `PyTuple_SET_ITEM` and then keeps using it. That is safe only
      because mapped errno values are cached small ints. It belongs to the
      container-getter port.

11. **`Objects/unicodeobject.c`, the four error-handler helpers (parses at :4387,
    :4492, :7250, :9568) — risk — rule L.**
    - **What happens.** Each call to a Python error handler parses its result
      tuple. That pushes two entries: the replacement and a freshly boxed `newpos`,
      which is boxed even though the format unit for it is `n`. The entries stay
      until the enclosing `decode`/`encode`/`translate` call returns.
    - **Effect.** With a Python handler, retention grows with the number of errors
      in the input.
    - **Fix:** wrap the parse and the use of `restuple` in `Py_BEGIN_BORROW_SCOPE`
      … `Py_END_BORROW_SCOPE` in each helper. The encode helpers already
      `Py_INCREF` the replacement before returning it, and the decode helpers copy
      it into the writer, so nothing escapes the scope.

### getargs.c

12. **`Python/getargs.c` push order and error paths — verified.**
    - **Arity first.** `vgetargs1_impl` pushes `nargs` items after the
      `min`/`max` check, so the count is ≤ `max`. `PyArg_UnpackTuple` pushes after
      `_PyArg_CheckPositional`. `vgetargskeywordsfast_impl` pushes after the
      `nargs + nkwargs > len` and `parser->max < nargs` checks. `converttuple`
      pushes only when `len == n`, and drops a temporary tuple after the run owns
      its items.
    - **Keyword values.** Values from the kwargs dict are kept, which is necessary,
      because they may be fresh boxes. `kwnames` values are released back to
      borrowed, because the caller's `kwstack` owns them. The duplicate check
      looks up owned values and releases them.
    - **Error paths.** Every early return after a push leaves the entries to the
      scope. That is correct: the entries are owned, the caller does not use the
      outputs on failure, and release preserves the error indicator.
    - **`O&` converters.** A converter that runs Python gets a nested activation
      whose floor is above the run. The run was filled before any conversion, so
      it stays contiguous. `test_reentrant_converter` checks this.

13. **`Python/getargs.c:vgetargskeywords_impl` — style.** The push happens after
    `nargs + nkwargs > len` but before the `$` check inside the loop. So a call with
    too many positional arguments for a format with keyword-only parameters pushes
    up to `len` items before its TypeError. That is bounded by the format, not by
    the input, so it is not the `f(*range(10**7))` case. Either compute `max` from
    the format before pushing, or state the bound as `min(nargs, len)` in §2 rule 10.

14. **`Python/getargs.c:find_keyword` and `find_keyword_str` — style.** Both still
    return new references, and all five callers release them at once. §5
    ("Delete from the runtime") says to restore upstream's borrowed return. Do it,
    or drop the line from the design.

### Clinic

15. **`Tools/clinic/libclinic` — verified.**
    - **Static keyword tuples.** `_PyValue_STATIC_IMMORTAL(op)` is
      `(uintptr_t)op + Py_TAG_REFCNT`. For aligned objects this equals the
      `| Py_TAG_REFCNT` encoding of `PyValue_None`. The macro is added under the
      same `Py_BUILD_CORE && !Py_BUILD_CORE_MODULE` condition as `_kwtuple`.
    - **Tuple receivers.** They push after `_PyArg_CheckPositional` /
      `_PyArg_NoKeywords`. The keyword path pushes `Py_MIN(nargs, max_pos)` before
      `_PyArg_UnpackKeywords`, which checks the arity. That function's inline fast
      path, for `varpos`, returns the run, and only the first `max_pos` items are
      read. Tuple-typed `*args` takes the tail with `PyTuple_GetSlice`, which owns
      it. The array converter pushes all `nargs` items, as intended.
    - **Limited API.** Generated code reads into a local `_args_items[max_pos]`
      with `PyArg_UnpackTuple`, and option groups do the same after the group
      check. Generation fails for limited-API `*args` after positional parameters.
    - **Borrowed reads.** A scan of every `*/clinic/*.h` and
      `Lib/test/clinic.test.c` finds no `PyTuple_GET_ITEM(args`,
      `_PyTuple_ITEMS(args)`, `PyTuple_GetItem`, `PyDict_GetItem` or
      `_PyTuple_CAST(args)->ob_item` outside finding 17.

16. **`Tools/clinic/libclinic/parse_args.py` (keyword path) — style.** When
    `max_pos` is 0 it still emits `_PyBorrow_TupleItems(args, 0)`
    (Modules/_ctypes/clinic/cfield.c.h:64), an out-of-line call that returns the
    static empty run. Skip the call when the count is `'0'` and pass a static
    non-NULL empty array. The macro's fast path needs `args != NULL`.

17. **`Modules/clinic/_remote_debugging_module.c.h` — style.** This tracked header
    has no includer, because the source moved to `Modules/_remote_debugging/`.
    It still has `PyObject *ob_item[NUM_KEYWORDS]` with raw pointers and
    `_PyTuple_CAST(args)->ob_item`. `clinic --make` cannot regenerate it. It is
    not compiled. Delete it, so later greps do not report false positives.

### Tests (`Lib/test/test_capi/test_borrow_scope.py`)

18. **Weak test — `test_release_on_return` (design test 3) and the `varargs_OO`
    half of `test_wrong_arity_pushes_nothing` — risk.** Both measure depth from
    Python after the calls. The eval-breaker release at the next instruction
    boundary also empties the stack, so both tests pass with the trampoline scope
    removed. **Fix:** add a C helper that calls a callable N times through
    `PyObject_Call` and through `PyObject_Vectorcall`, and returns the depth delta
    seen just before it returns. It should be 0 for a METH_VARARGS callee and for
    a Clinic tp_new. This also covers the `_PyObject_VectorcallTstate` and
    `type_call` scopes, which nothing tests now. A test of decision 6 (no release
    bit after a pushing FASTCALL builtin) depends on finding 6.

19. **Tests that check the right things.**
    - **Design tests 1, 2, 4, 5, 6, 7, 9, 12 to 17 and the debug fatal-exit test.**
      The churn helper reuses freed int and float boxes, so a premature release
      changes the values read back.
    - **Test 5 and test 12.** `check_slot_release` covers both: with and without a
      nested activation.
    - **Test 13.** The PEP 523 test would have failed with the old floor in the
      wrapper.
    - **Test 16.** The pickle test measures the delta inside `load` between two
      `__reduce__` probes.
    - **Test 8.** `test_generator_resumed_from_c` covers it, but not under
      `-R 3:3`.

20. **Missing §6 tests:**
    - 10: embedding, a `Programs/_testembed.c` case with an explicit scope, and
      base-region leaks across `Py_FinalizeEx` checked with `-X showrefcount`;
    - 11: out of memory, `_testcapi.set_nomemory` while a chunk grows;
    - 18: a daemon thread at finalization, under ASan;
    - 19: finalization order, where a base-region `__del__` runs before module
      teardown;
    - 20: stack switching, which is acceptable to defer, because it is not built;
    - 21: a run recipe, not a test.

    Add the finding 1 regression test as well.

## Hot-path cost (record, not a gate)

Release build `build/pyvalue-rc-release-20261103`, rebuilt with `make -j12 python`
at 17:42 from the tree above. The rebuild included the uncommitted working-copy
edits, which do not touch the functions measured. Counts are instructions on the
path where the body pushes nothing.

| Site | Added by the borrow scope | Notes |
|---|---|---|
| `cfunction_vectorcall_O` (all trampolines alike) | 5: load `top`, spill mark, reload, compare with `top`, branch. 3 loads and 1 store; no atomics | The design's "2 loads + 1 compare" plus the spill across the call |
| `PyObject_Vectorcall` (inlined `_PyObject_VectorcallTstate`) | 5, the same sequence | Nests with the callee trampoline's scope: 10 per C→builtin call |
| `_Py_BuiltinCallFast_Value` and the four sibling helpers | 5 + 2 (TLS load of `tstate`) | The helpers take no `tstate` |
| `_PyEval_EvalFrameDefault` (floor wrapper) | ≈27 including the new call frame: 7 prologue, 2 null check, 4 floor set, call, 2 Exit check, 5 restore/re-arm check, 6 epilogue | The design estimated about 5; the extra frame dominates |
| Each push inside an activation (`_PyBorrow_Keep`, `_PyBorrow_TupleItems`) | `cmp` + `lock or` on `eval_breaker`; then at the next boundary an out-of-line `_Py_HandlePending` and an atomic AND | Even when a trampoline already released (finding 6) |

## Required fixes

1. Finding 1: Clear must not end the stack's life. Free chunks when the thread state
   is freed, and do not drop entries silently there: release them, or assert depth 0
   in debug builds. Add the late-`__del__` regression test.
2. Finding 11: give the four Unicode error-handler helpers an explicit borrow scope.
3. Finding 18: replace the Python-level depth checks for return-time release with a
   C-loop helper, so the trampoline, `_PyObject_VectorcallTstate` and `type_call`
   scopes are tested.
4. Finding 20: add §6 tests 10, 11, 18 and 19.
5. Finding 6: correct the decision-6 cost claim in the design. Record the four
   benchmarks and a JIT-hot `datetime(...)` loop before choosing whether to suppress
   the spurious release bit.

Recommended, not required: findings 2, 3 (cost), 13, 14, 16, 17. The design's §3
"As built" paragraph, the status column and §2 still describe `acbdf45627d`, with the
floor in the wrapper and decisions 1, 4, 5, 6, 8 and 9 in progress. The coordinator
should bring them up to `6d39221996d`.
