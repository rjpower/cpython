# Review 1: value API steps M3a and M3b (proposal)

Reviewed: [value-api-m3.md](../proposals/value-api-m3.md) (revision 1, 768
lines, uncommitted, 2026-11-04), with its scripts `m3-clinic_conv.py` and
`m3-impl_returns.py`. The proposal was checked against
[value-api.md](../value-api.md) (§3, R7, R20, R26, R27),
[value-api-v2a0.md](../proposals/value-api-v2a0.md) (§1.4, §1.7, §2, §3),
[register-ir-v2-a3.md](../proposals/register-ir-v2-a3.md) revision 3 (G9,
R-a to R-d) and [pyvalue.md](../pyvalue.md) (B1, H1, L1, D4, D5). Code:
`build/worktrees/pyvalue-rc-universal`, read-only, at HEAD `c71a3ca6aa2`.
That is two commits after the proposal's `15859d9a592` (A3 step 1e and
A5b-3). Both scripts reproduce the proposal's counts there: 459 functions,
515 parameters, and 1,075 return statements in 429 bodies.

## Verdict

**Accept with changes.** The core of M3a is sound: D1–D10, D12, D14 and D16
hold once findings 3 and 4 correct two details. That covers the
five-argument signature, the borrowed window, the one-word result, the DUP
into `args[-1]`, the flag test, the disjoint `METH_VALUE` bit, the
per-instance thunks and the struct move. M3b is sound as well. Four
must-fix findings, in three groups, stand between M3a and adoption:

- **D11 adds a second conversion layer.** D8's per-instance thunks already
  convert every object-vector call into a value call, so the vector half of
  D11 does the same work again (finding 1).
- **D13 builds stopgaps that I5 deletes.** Move monitoring and tracing to I5
  as a whole (finding 2).
- **Two specification errors break Python behaviour.** `method_hash` would
  call `__hash__` on `self` (finding 3). The new post-check would reject
  every metaclass that defines `__call__`, `EnumType` included (finding 4).

With these changes M3a costs about **13 agent-hours** (5.5 h wall), not 20.5.
M3b's 7.0 stands (§ Estimate).

## Findings

| # | Severity | Section | Problem | Evidence | Change |
|---|---|---|---|---|---|
| 1 | must-fix | D11, §1.7 "call API thunks" | **Two layers do the same conversion.** Under D8, the object field of every core callable is a thunk that converts the object vector once and calls the value field. The upstream body of `PyObject_Vectorcall` and its 30-odd relatives reaches that field through `_PyVectorcall_FunctionInline`. D11's branch on the form of the callee repeats the same conversion one level up: same entry cost, same exit box, one more form test. The suite claim ("runs through the value path") holds without it, because D8's thunks run every C-to-core call through the value bodies. The one real gain is on the tuple entries. There `_PyVectorcall_Call` boxes inline tuple items with `_PyBorrow_TupleItems`, and the thunk then unboxes them again | `call.c:246–300` (`_PyVectorcall_Call`, `:255` the tuple borrow); `pycore_call.h:138–150` (upstream body plus the fork's borrow scope); §1.7's own thunk text | Drop the branch on the callee. The object-vector entries keep upstream's body and reach value callables through D8's thunks. The tuple entries (`PyObject_Call`, `_PyObject_Call`, `PyObject_CallObject`, `PyVectorcall_Call`, `_PyVectorcall_Call`, `_PyObject_Call_Prepend`) call `_PyValue_Call` for every callee, so the choice depends on the form of the arguments, not on the callee. `_PyValue_TpCallAdapter` passes the original tuple to an object `tp_call` when nothing was prepended, instead of rebuilding it. That is about 0.3 h of a1; w1 (2.0 h) goes. W2 sweeps the callers, then the whole family is marked |
| 2 | must-fix | D13, §1.8, §5 w6–w7 | **Monitoring work that I5 throws away.** M3a writes value entry points that have no value caller, then turns the three object entry points into thunks over them, "kept for the `ceval.c` unit and deleted by I5". All ten callers are I5's. That is the stopgap pattern the plan excludes. `_Py_tracevaluefunc`, stored beside `c_tracefunc`, also adds a second trace convention inside the core | `pycore_instruments.h:56–65`; 5 callers in `ceval.c`, 5 in `bytecodes.c` (§1.8) | Move D13 to I5 whole. `instrumentation.c`, `legacy_tracing.c` and `sysmodule.c` lie outside the `ceval.c` unit, so I5 can give them to Sonnet workers (w6 + w7, 3.0 h) beside its lead. Store one value trace function per thread. `PyEval_SetTrace` and `PyEval_SetProfile` install a boxing adapter that carries the extension's `Py_tracefunc`, so the object form exists only at the C-API entry. `c_tracefunc` keeps its public meaning for readers |
| 3 | must-fix | §1.6, `PyMethodObject.im_self` row | **`method_hash` changes behaviour.** Upstream hashes `self` by identity. `_PyValue_Hash` is the value form of `PyObject_Hash`: `hash([].append)` would raise `TypeError`, and `hash(o.m)` would run `o.__hash__`. The `m_self` and wrapper rows name no hash at all | `classobject.c:285` and `methodobject.c:348`: `PyObject_GenericHash(self)` | Add `_PyValue_IdentityHash(PyValueBorrow)`: `Py_HashPointer` for an object; for an inline value, a hash of the word, which agrees with `PyValue_Is`. Use it in `method_hash`, `meth_hash` and the wrapper hash. Test with an unhashable `self` and with an inline `self` |
| 4 | must-fix | §1.2 "Inheritance and flags", 4th bullet | **The post-check rejects metaclasses with `__call__`.** `inherit_slots` always copies the object offset and drops the flag when `tp_call` is overridden. D6 copies the value offset in the same block. A metaclass that defines `__call__` then has a non-zero value offset and no flag, so "a non-zero `tp_value_vectorcall_offset` requires `Py_TPFLAGS_HAVE_VECTORCALL`" fails `PyType_Ready`. `EnumType` is such a metaclass, and most of the suite imports `enum` | `typeobject.c:9526–9536`; `Lib/enum.py:714` | The check is: a non-zero value offset requires a non-zero object offset. The flag alone decides whether dispatch reads the field (D7), as upstream does for `tp_vectorcall_offset` |
| 5 | should-fix | §1.3 dispatcher | **A release-build result check on every value call.** Each value call goes through `_PyValue_CheckFunctionResult`. Upstream's specialised call handlers only assert this. Every value callee is core code, because the typedef and `METH_VALUE` are internal. In a handler that inlines the dispatcher (§1.7, G9 fit), the check is one more call on the hottest path | the dispatcher listing in §1.3; upstream's `_CALL_BUILTIN_O` and its siblings only `assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL))`; the fork's `bytecodes.c` and `ceval.c` never call `_Py_CheckFunctionResult` | Make the check a debug assertion for value callees. Keep the release check in `_PyValue_CallAdapter` and `_PyValue_TpCallAdapter`, where extension code returns |
| 6 | should-fix | D2, §1.1 window rule, §7 | **Nothing checks how the window is read, and the L1 exception is not recorded.** `const PyValue *args` lets `PyValue x = args[i]` make a second owner without a count, the gap A5b names. Value callees store arguments (`list.append`, frame push, containers). The window rule also allows `PyValue` scratch arrays that hold words with no count, an exception to L1 that §7 does not carry to pyvalue.md | pyvalue.md L1 and the A5b paragraph ("does not enforce that a `PyValue` owns a count") | Add a `check_value_api.py` rule with a self-test: in a function of type `_Py_vectorcallvaluefunc`, `_PyCFunctionValue` or `_PyCMethodValue`, every `args[i]` read is an operand of `PyValue_Borrow`, `PyValue_DUP` or the window primitive. Add pyvalue.md (L1: "call-scratch arrays, written only by `_PyValue_WindowFromObjects`, are not homes") to §7 |
| 7 | should-fix | D15, §2.4 `__new__`/`__init__` | **The index parser does not cover a keyword dict.** Indices into the tuple cannot reach dict values. Pointers into dict entries would dangle if a converter (`__index__`) mutates a kwargs dict that other code shares | §2.4 says only "the parser reads `PyTuple_GET_VALUE`" | Copy the dict's value words, after the tuple's items, into a call-scratch window using the pyvalue.c primitive (`_PyValue_WindowFromDictValues`), then index it. Both entry kinds then use one parser form |
| 8 | should-fix | Q5, Q7, §3 box counters, §5 schedule | **The interim costs are bounded only by order, and the order is not stated as a dependency.** Builtins that W2 converts fall to `CALL_NON_PY_GENERAL` and gain a thunk box per inline result until I5 lands. C-to-Python calls box twice until I3. Under v2a0 §1.7, moved by this proposal, I5 runs T11.5–T17.5 and W2 T12–T18: a margin of 0.5 h. I3 also has a boundary that §1.4 leaves unnamed: the PEP 523 hook returns `PyObject *` | `Include/cpython/pystate.h:327` (`_PyFrameEvalFunction`); `bytecodes.c:1473` | Make "W2 integrates after I5" a dependency in v2a0 §1.7, not "or record the dip" (Q7). Leave `INTERPRETER_EXIT` with I3, which integrates by T18 either way. State that `_PyEval_EvalFrameValue` keeps a steal for frames run by a PEP 523 hook as permanent compatibility. With the estimate below, M3 lands near T9.5, which widens the margin to about 2 h |
| 9 | should-fix | §5 | **The estimate is inflated by scope and by unit size.** Of the 20.5 h, 5.0 h are findings 1–2. w2–w4 price each home touch point (traverse, clear, dealloc, member, compare) at 8 minutes, the rate for a new function. w5 waits for a2, but it needs only the value-wrapper shape, which can go in a short interface note | §5 table | Use the recount below: about 13 agent-hours, 5.5 h wall, with I5 taking 3.0 worker-hours |
| 10 | note | D1, Q3 `tstate` | **Keep `tstate`, but correct the reason.** The gate builds are static (`CONFIG_ARGS` in `build/a3-release` and `a3-clang-release` has no `--enable-shared`), so `_PyThreadState_GET` is one `%fs` load, not `__tls_get_addr`. The argument still earns its register: the caller holds it pinned (one move), and the dispatch layer needs it for the frame push, the recursion check and errors. V8's C++ builtins take the isolate and LuaJIT's C functions take `L`. Do not add it to `_PyCFunctionValue`, whose leaves rarely need it | `pycore_pystate.h:93,119`; `configure.ac:4044` (global-dynamic only for shared modules) | Rewrite the row in §1.1 |
| 11 | note | D10, Q2 | **The struct move is right.** Deleting the four borrowed getters follows the link wave's ruling on borrowed container getters. If the five frozen fields became seven, extension code that reads `->m_self` as a `PyObject *` would keep compiling and misread an inline word at run time. With the move, the same code fails to compile. The SOABI bump is needed: four sizes and two field types change. Cython is affected twice: it embeds `PyCMethodObject` in its function type, and it unpacks bound methods with `PyMethod_GET_SELF`. Both fall under the ports step after 10b | self reads (`rg` on `im_self\|m_self` and the getters): 2 `_lsprof.c`, 1 `_pickle.c`, 1 `native_scope.c`, 1 `typeobject.c`, 3 `ceval.c`, 1 `bytecodes.c` outside the two type files | None. Bump once per landing train with V2a-0 and M4b |
| 12 | note | V2a-0 part 2, w5 | Part 2's 12 value wrappers are being rewritten to the value shape before they reach the shared tree | §1.4 "Value wrappers" | If part 2 has not yet integrated, apply the reshape as a fix-up on part 2. That costs the same and avoids landing a shape that is about to change |

## Answers to the author's questions

**Q1 (DUP or borrow in `args[-1]`).** Keep the DUP. The count is needed for
the GC, not for lifetime. `_PyGC_VisitFrameStack` visits every slot of a
generator frame or of a frame that has a frame object, and it rechecks each
slot as a home (`gc.c:481–491`). Under B1, a borrow of a mortal object has
the same bits as its owner. If one sat in a slot, `subtract_refs` would
remove a reference that nobody counted, and `gc_refs` would underflow. The
callee cannot hold on to `args[-1]`, because it gets no flag and a `const`
window. The fixed rule, which D5 already states, is that a word written into
memory the writer did not allocate is an owner. Its cost is small, because
the interpreter's generic path already expands bound methods
(`call_transaction_expand_method`, `ceval.c:1638`, used at `:3225`). The
count pair therefore falls mostly on C callers' scratch arrays, where it is
harmless. Count it in the stats run, as proposed.

**Q2 (struct move or seven frozen fields).** Move the structs (finding 11).

**Q3 (`tstate`).** Keep it, with the corrected reason (finding 10).

**Q4 (one `METH_VALUE` form).** One form is enough. For a `METH_O`-shaped
function, the arity test moves from the handler's guard (`CALL_BUILTIN_O`
exits when `total_args != 1`) into the callee, which adds one `kwnames`
compare. The argument load moves from caller to callee, and the caller adds
a `lea` and an `xor` for `args` and `kwnames`: about three instructions.
Against that, a value result such as `math.sin`'s float saves an allocation. The
three hottest builtins named in the brief never use the convention, because
they have their own handlers (`CALL_LEN`, `CALL_ISINSTANCE`,
`CALL_LIST_APPEND`, `bytecodes.c:4164–4166`). I5 should keep those handlers
and not fold them into `CALL_BUILTIN_VALUE`. That is LuaJIT's split between
fast functions inside the VM and a generic C fallback. The receiver should be
a `PyValueBorrow`, because the function type is shared with `int` and
`float` methods. A typed receiver costs Clinic's wrapper one mask in
`PyValue_AsKnownObjectBorrow`.

**Q5 (double box until I3).** Accept it and record it. It is bounded by
order (finding 8), and avoiding it would need a function-specific body that
I3 throws away.

**D11 placement.** Neither in M3a nor in W2 as written. The object-vector
entries need no change (finding 1). The tuple entries move to `_PyValue_Call`
in M3a's a1. W2 sweeps the core callers, and P3 then marks the family.

**Core paths that still reach a thunk after M3a.** There are four: the
call-API callers (W2), the interpreter's object-vector calls through
`VALUES_TO_PYOBJECTS` (I5), `_PyEval_Vector` (I5 and W2), and the
`PyMethod_New` and `PyCMethod_New` callers (W2). None remains once W2 and I5
have landed. The `vectorcall-thunk` rule checks names, not reachability.
What proves that no core path is left is the gate's THUNK count by owner
(step G), so cite that as the check in §1.7.

**M3b's split.** Three choices are right: the index parser, an `_impl` that
returns `PyValue`, and moving the 1,075 return statements to W2. The returns
are edits in the same 41 files that W2 already opens for the 260 `object`
parameters, so R21's rule of one edit per file holds. An `_impl` that still
returned `PyObject *` would keep `PyLong_From*` boxes inside the core, which
R27 forbids. Regenerate file by file inside W2's batches, which the
per-file directive allows, not as one first act.

## Signature recommendation

Adopt D1 as written:
`PyValue (*)(PyThreadState *tstate, PyObject *callable, const PyValue *args, size_t nargsf, PyObject *kwnames)`.

- **Borrowed view of a window the caller owns, not stolen arguments.** If the
  callee consumed its arguments, every builtin would have to close each one.
  The interpreter pushes Python frames itself, and that path already steals.
  This keeps VA7.
- **One word, with `PyValue_NULL` as the error.** A status pair needs a third
  outcome to carry, and a call has none. Frame pushes stay in A3's
  `call_transaction_run_*`, which returns `{status, child}` under G9 R-a.
- **`kwnames` stays a tuple of interned `str`.** It is never inline, so a
  value form gains nothing. R1 would allow `PyTupleObject *`, but the thunks
  pass `kwnames` unchanged to PEP 590 callees, so keep `PyObject *`.
- **Two changes to the call contract.** The result check becomes debug-only
  for value callees (finding 5), and reads of the window are linted
  (finding 6).
- **Fit with I5 and a JIT.** In a tail-call handler the five arguments go in
  `rdi` to `r8`, with `tstate` copied from its pinned register. The result
  comes back in `rax`, and no handler local has its address taken, so G9 R-a
  holds. A JIT must spill the arguments to the frame window before a generic
  call, as V8 and LuaJIT both do. Its gain from registers comes from calling
  `_impl` directly, not from changing this signature.

| From | Mechanism | Take or reject |
|---|---|---|
| V8 JS linkage | `target`, `new.target`, `argc` and `context` in registers; arguments on the stack with a receiver slot | Take callee identity and VM state as arguments, and in-memory arguments that the caller points at (both done). Reject `new.target`, since `type.__call__` constructs. Reject a mandatory receiver slot, since a tuple call has no writable slot before its items |
| V8 fast API calls | A typed C entry beside the generic builtin, chosen by the optimiser when argument types match | Nothing to build in M3. Clinic's typed `_impl` already is that entry, and `ml_meth` identifies it. Record in §1.10 that the JIT step calls `_impl` directly after guards |
| LuaJIT fast functions | `base` into the Lua stack and `nargs`; results written back below `base` with `nres`; `FFH_RETRY`/`FFH_TAILCALL`; builtins recorded by `ffid` | Take the in-place window (done) and identification by identity (`ml_meth`, as `ffid`). Reject `nres`: Python returns one value, and writing into the caller's slots breaks G9 R-a. Reject the tail-call return: C callers cannot take a frame back |

## Estimate

Units and minutes follow the proposal's own model: a Sonnet or Haiku worker
per disjoint file set, up to 16 at once, one builder round and one cleanup
pass. The scope follows findings 1 and 2. A new function counts 10 minutes, a
home touch point or reshaped wrapper 3, a moved-field site 2, a test case 4.

| Part | Who | Units × minutes | h |
|---|---|---|---:|
| a0 interface note: typedefs, `METH_VALUE` types, value-wrapper shape, declarations | lead | – | 0.5 |
| a1 dispatcher, 2 adapters, prepend, window primitive, thunk macro, `_PyValue_Call` and 6 tuple entries, 4 fixed-arity forms, `NotCallable`, result check, `StackUnpackDict`, `InstanceVectorcall` | lead | 14 × 8 + 6 × 3 | 2.2 |
| a3 function value vectorcall, `_PyEval_EvalFrameValue`, `PyFunction_SetVectorcall`, `cfunction_value_vectorcall` example | lead | 4 × 15 | 1.0 |
| a2 layout: 4 fields, 2 struct moves, SOABI, inheritance and check, member, pair rows, value `slot_tp_call` | Sonnet | 10 × 7 | 1.2 |
| w2 `classobject.c` | Sonnet | 2 × 10 + 9 × 3 | 0.8 |
| w3 `methodobject.c` | Sonnet | 3 × 10 + 9 × 3 | 0.95 |
| w4 `descrobject.c` | Sonnet | 5 × 10 + 6 × 3 | 1.15 |
| w5 `typeobject.c` (starts after a0) | Sonnet | 13 × 3 + 10 | 0.8 |
| w8 moved-field users | Haiku | 25 × 2 | 0.85 |
| w9 tests, two workers | Sonnet | 20 × 4 + 2 × 15 | 1.85 |
| a4, builder round, cleanup, GCC debug and Clang release suites | lead | – | 1.75 |
| **M3a** | | | **13.05** (lead 5.45, workers 7.6) |

The wall clock is 0.5 h for a0, then the lead's a1 and a3 (3.2 h) while the
workers finish by about 1.7 h, then 1.75 h of a4, builder and cleanup: about
**5.5 h**, against 8. I5 takes on w6 and w7 (34 × 3 + 19 × 4 minutes =
3.0 worker-hours), run beside its lead at no cost in wall time.

M3b: generator 23 converter and return kinds × 5 minutes, index-parser and
tuple-dict emission 2 × 20, directive, refusals, selection, prototypes,
`@vectorcall` and guard 6 × 5, giving 3.1 h. Then `getargs.c` helpers 5 × 12
(1.0 h), `clinic.test.c` 30 blocks × 3 (1.5 h), `_testclinic_value` 40 × 2
(1.3 h) and the dry run (0.5 h): **about 7.4 h**. That matches the proposal's
7.0. With the converters split between two workers after the lead's skeleton,
the wall clock is about 3 h.

**M3a plus M3b: about 20.5 agent-hours** (13 + 7.4), against the proposal's 27.5 and
v2a0's 10. I5 gains 3.0 hours. W2's 8.4 h for 337 return sites (1.5 minutes
each) plus 0.5 h for the script stands.

## Review 2 (revision 2)

Revision 2 was checked at runtime HEAD `f7d374b1f92`, which includes V2a-0
part 2. The coordinator has ruled on two of the author's questions: the hash
change for bound methods gets no What's New note, and `_Py_wrappervaluefunc`
does not take `tstate`. This section therefore covers the four must-fixes,
D17 and `_PyValue_TpCallThunk`, and the author's question 3.

### Verdict

**Accept after one more change.** All four must-fixes of review 1 are
resolved, and so are should-fixes 5–10. D17 introduces no new problem. One
must-fix remains, and it predates revision 2: the dispatcher's type-level
branch calls `tp_value.call` without the recursion guard that upstream puts
around every `tp_call`. The fix is a few lines in §1.3. Implementation can
start once it is written in, with no third review. The estimate (M3a 14.3,
M3b 7.4, T9.5) is arithmetic from the units and holds; the guard costs
minutes inside a1.

### Review 1's must-fixes

| # | Status | Where |
|---|---|---|
| 1 | Resolved. D11 covers the six tuple entries only, chosen by the form of the arguments. The object-vector entries keep upstream's body and reach value callables through D8. w1 is gone | D11, §1.3, §1.7 |
| 2 | Resolved. M3a has no monitoring part. I5 gets one value trace function per thread, with a boxing adapter at `PyEval_SetTrace`/`SetProfile` | D13, §1.8 |
| 3 | Resolved. `method_hash`, `meth_hash` and `wrapper_hash` use `_PyValue_IdentityHash`, which agrees with `PyValue_Is` | §1.6 |
| 4 | Resolved. A non-zero value offset requires a non-zero object offset, and the flag alone decides dispatch | §1.2 |

### New findings

| # | Severity | Section | Problem | Evidence | Change |
|---|---|---|---|---|---|
| 14 | must-fix | §1.3 dispatcher, `_PyValue_TpCallAdapter` row; §1.11 | **The type-level branch has no recursion guard.** Upstream calls every `tp_call` inside `_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")`. With `vf(tstate, c, …)`, the dispatcher reaches value `slot_tp_call`, the wrapper descriptor, the method-wrapper and the classmethod descriptor without it. `A.__call__ = A(); A()()` then recurses through value `slot_tp_call` and the dispatcher until the C stack overflows, where upstream raises `RecursionError`. This is the check that the author's question 3 asks about. Moving `wrapperdescr_call` and `wrapper_call` from `tp_call` to `tp_value.call` takes them out from under the guard | `call.c:224` (`_PyObject_MakeTpCall`), `:278` (`_PyVectorcall_Call`), `:372` (`_PyObject_Call`); `Lib/test/test_descr.py:3903` `test_recursive_call`; `slot_tp_call` is `call_method(self, __call__, …)` (`typeobject.c:12310`) | Wrap both type-level calls in the dispatcher, `vf(…)` and `_PyValue_TpCallAdapter`, in `_Py_EnterRecursiveCallTstate`/`_Py_LeaveRecursiveCallTstate` with upstream's message. Vectorcall callees keep their own guards, as upstream does (`cfunction_value_vectorcall` and the frame push). Add `test_recursive_call` to §3's existing-tests list |
| 15 | should-fix | §1.11 `args, nargs, kwnames` row; `_PyValue_TpCallThunk` | **The keyword check sits in each wrapper.** Upstream rejects keywords once, in `wrapperdescr_raw_call`, before it chooses a wrapper. §1.11 puts "takes no keyword arguments" in every value wrapper: 13 now, plus M1b's and M2's. The object fallback in `wrapperdescr_value_raw_call` then needs its own copy. Upstream also accepts an empty dict (`PyDict_GET_SIZE(kwds) != 0`) | `descrobject.c:557–562` | Make the check once in `wrapperdescr_value_raw_call`, for every slotdef without `PyWrapperFlag_KEYWORDS`, ahead of both paths. Value wrappers then check only arity. State that `_PyValue_TpCallThunk` and `_PyValue_Call` turn an empty dict into NULL `kwnames`, so `int.__add__(5, 2, **{})` still works |
| 16 | note | §1.11 `wrapperdescr_value_call` | The self check upstream is `_PyObject_RealIsSubclass` on types, which never calls `__subclasscheck__` | `descrobject.c:601` | Spell it `PyType_IsSubtype(PyValue_TYPE(self), PyDescr_TYPE(descr))` |

### The author's question 3

No error message is lost. `wrapperdescr_value_call` keeps "needs an
argument" and "requires a '%s' object but received a '%s'". The keyword
message stays, though it belongs in one place (finding 15). Arity keeps
`check_num_args`'s text. Nothing outside `descrobject.c` names
`wrapperdescr_call`, `wrapper_call` or `wrapperdescr_raw_call`, so deleting
them is safe. One check is lost, the recursion guard (finding 14).
`_PyValue_TpCallThunk` itself is sound. Object callers that arrive through
`_PyObject_MakeTpCall` keep their guard there, and the tuple it builds holds
canonical words, so the thunk boxes only an inline result.
