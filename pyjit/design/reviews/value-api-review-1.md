# Review 1 of `value-api.md`

Reviewed 2026-11-03 against the draft committed as `ea408d02efd` (740 lines).
The page pins the runtime tree at `e0a0f05c47d`; that tree is now at
`379e92cded9` with 6 uncommitted files. Every line number the page cites still
points at the right code, except two that moved by a few lines (finding M1).
File references below are relative to the runtime tree
`build/worktrees/pyvalue-rc-universal`. Scratch compiler tests used GCC 15.2
and Clang 21 (`/usr/lib/llvm-21/bin/clang`).

## Verdict

**Do not start implementation yet. Four blocking changes are needed, all of
them small to write down.** The direction is right and most of the page is
sound: one entry primitive, one exit primitive, NULL meaning error only, a
value slot table paired with object slots, an adapter in the dispatcher
rather than derived function pointers (C3 is a good catch), and a ratchet with
no allowlist. The inventory holds where I checked it.

The four problems:

1. **B1.** The rule for object positions (C1) hides a second form, and the page
   does not say how a value gets into an object position safely.
2. **B2.** Storing the value table behind a `tp_as_value` pointer leaves no
   correct inheritance rule. §2.3 also misstates what upstream does.
3. **B3.** Variadic formatting calls (`%R`, `%S` and the like) can take a
   `PyValue` without any diagnostic. The sweep will create such calls, and
   nothing in §4 catches them.
4. **B4.** The dispatcher specification (§2.5) gets rich comparison wrong and
   leaves out four upstream dispatch rules.

Eight findings are important, mostly about completeness and rollout order.

## Findings

### Blocking

**B1. Object positions hide a dual form and an unsafe conversion (C1, §1.2,
§1.3).**

*Evidence.* C1 (lines 37–50) says an inline value that reaches an object
position "is boxed on that cold path only so that the upstream `TypeError` is
raised". The page never says who boxes. §1.3 (line 181) gives
`PyValue_AsPyObjectBorrow`/`Steal` as the conversion for "value in an object
position". It masks the tag and asserts only in debug builds, and §7.2
inconsistency 3 says it returns a garbage pointer for an inline value in
release builds.

Inline values reach object positions through every polymorphic source the page
creates:

* a `METH_VALUE` builtin's arguments (`len(x)`, `iter(x)`, the class argument
  of `isinstance(x, C)`);
* value-vectorcall arguments;
* a `_PyValue_GetAttr` result that is then called, because
  `_PyValue_Vectorcall` takes `PyObject *callable` (line 168);
* frame slots.

At each such site the author must choose between masking, which is wrong if
the value can be inline, and boxing. The choice depends on the C1 test ("does
the operation succeed for some exact `int` or `float`?"). That test is a
property of the operation, not of the value the caller holds, and neither the
compiler nor a script can check it.

The rule also splits the generic API in two. Core code keeps calling
`PyObject_GetIter`, `PyObject_Size` and `PyObject_Call*` (§7.1: "keep their
object form and are not swept"). At the same time, `PyObject_GetItem` and
`PyObject_SetAttr` are thunks the core must not call, and
`_PyValue_GetItem(PyObject *, PyValue)` takes an object receiver. So a reader
must know, function by function, whether a `PyObject_*` name is a forbidden
thunk or an allowed object operation, and whether a `_PyValue_*` function takes
values. That is the dual form the user asked to avoid. The brief's VA2 ("a
receiver *known* to be a str, tuple, type …") was closer to right. C1 widened
"typed" from "known from its C type" to "would fail for an int".

*Fix.*

1. Every generic operation `_PyValue_<Op>` takes `PyValue` for every operand
   that comes from Python. That includes the receivers of `len`, `iter`,
   `getitem`, `setitem`, `contains` and `setattr`, the callable, and the class
   argument of `isinstance`. The inline test goes inside the operation:
   `PyValue_TYPE(v)` already returns `&PyLong_Type` or `&PyFloat_Type`, and
   their NULL slot raises the upstream `TypeError` without a box. Box only
   where an object slot of `int` or `float` must run on the receiver (for
   example `(5).x = 1` reaching `PyObject_GenericSetAttr`), inside the
   dispatcher.
2. Object positions remain only where the C type says the value cannot be
   inline:
   * concrete-type receivers, after a type check;
   * names and kwnames;
   * type arguments (`PyTypeObject *`);
   * receivers of value slots that `int` and `float` do not have (`subscript`,
     `contains`, `iternext`, `call`, `send`).
3. With that rule, every `PyObject_*`, `PyNumber_*`, `PySequence_*`,
   `PyMapping_*` and `PyIter_*` function becomes a thunk and is marked. The
   ratchet becomes a rule about name prefixes, and core code never has to make
   the C1 judgement.
4. A `PyValue` becomes a `PyObject *` only after a check that excludes inline
   values. Until a measurement says the check costs too much, make
   `PyValue_AsPyObjectBorrow`/`Steal` test `bits < 2^48` in release builds
   too, and fail fatally if the test fails. That is one compare and a branch
   that is never taken.
5. Decide open question 1 now. A `PyObject *` whose C type already excludes
   numbers becomes a value with a raw tag and a debug assert
   (`_PyValue_FromTyped`). The canonical borrow is used only for polymorphic
   `PyObject *`: compatibility entries and results of object-form natives.
   These two primitives cover two different kinds of position, so this is not
   a second form of one operation.

V8 and JavaScriptCore draw the boundary this way. `Object::GetProperty` and
`Object::ToObject` in V8, and `JSValue::get` with `synthesizePrototype` in
JSC, take the tagged value as the receiver and turn a primitive into an object
inside the operation.

**B2. A `tp_as_value` pointer has no correct inheritance rule, and §2.3
misstates upstream (§2.1, §2.3, open question 6).**

*Evidence.* §2.3 (lines 328–331) says that upstream does not inherit indirect
slots when the subtype has no `tp_as_number` storage. In fact,
`type_ready_inherit_as_structs` (`Objects/typeobject.c:10914`, called at
`:10978`) copies the base's table *pointer* when the subtype's own pointer is
NULL. This leads to a problem whichever way `tp_as_value` is handled:

* **If `tp_as_value` copies that pattern.** Take an extension subtype of `int`
  that sets its own `tp_repr`, or its own `tp_as_number->nb_add`, and leaves
  `tp_as_value` NULL. It shares `int`'s value table, so `int`'s value `repr` or
  `nb_add` runs for the subclass. Rule 2 forbids exactly that. `PyValueSlots`
  mixes direct slots (`richcompare`, `hash`, `repr`, `str`, `getattro`,
  `iternext`, `call`) with indirect ones. So sharing the pointer is wrong
  whenever the subtype overrides any one of them.
* **If it does not.** `tp_as_value` stays NULL for static extension types.
  Two things then go wrong:
  * The pair table has nowhere to put a value side for extension types that
    name `PyObject_GenericGetAttr`, which most do. §2.2 moves the 20
    `tp_getattro == PyObject_GenericGetAttr` identity tests to the value side.
    Those tests then fail for extension types, and the attribute
    specialisations in `Python/specialize.c` stop applying to them. That is a
    regression against today's tree.
  * Deduplication compares `int`'s value function with the per-type object
    thunk that the subtype inherited. The two pointers differ, so
    `binary_op1` calls `int`'s implementation twice where upstream calls it
    once. That is harmless for pure core slots, but it departs from upstream's
    rule.

*Fix.* Put the value slots inline, as a block appended to the end of
`PyTypeObject`, after `_tp_cache`. Extensions' positional static initialisers
still compile and leave the block zero. Then:

* every type has storage, and inheritance works slot by slot under the pair
  rules;
* the generic pair table fills the value side for any type, core or
  extension, whose object side is a known generic function;
* `_tp_iteritem` stays where it is. Moving it into a separate table (§2.1)
  adds a load to generic `FOR_ITER` (`Python/bytecodes.c:1684`, `:3747`).

Dispatch then needs one dependent load fewer than either the page's design or
upstream. The memory cost is one block of about 45 pointers per static type
(129 core and 54 module static types, about 66 KB). Heap types pay what they
would pay for the page's embedded `as_value`. This also answers open
question 6.

**B3. Variadic calls bypass both the struct type and the `unavailable`
attribute (§4, C8).**

*Evidence.* Core makes 1129 calls to `PyErr_Format`, `PyUnicode_FromFormat`
and `_PyErr_Format`. 444 of them have a `%R`, `%S`, `%A`, `%U`, `%V`, `%T` or
`%N` conversion. I counted with `rg -U` over `Objects/`, `Python/` and
`Parser/`, excluding generated files and Clinic headers.

A `PyValue` passed where `%R` expects a `PyObject *` compiles cleanly. I tested
`int f(PyValue v) { return PyErr_Format(0, "%R", v); }` with `-Wall -Wextra`
on GCC 15.2 and Clang 21; neither warns. C8 says that every call returning a
polymorphic result changes the caller's local to `PyValue`. Any such local
that later goes into an error message becomes silent undefined behaviour. This
happens mostly on error paths, which the suite seldom runs.

*Fix (before V2a's sweep).*

1. Give the format functions a way to print a value: a value conversion such
   as `%vR`/`%vS`, or a rule that callers format through `_PyValue_Repr` and
   pass the result with `%U`.
2. Make the build fail when a `PyValue` reaches a variadic argument. Either:
   * have `check_value_api.py` run a `clang-query` matcher over
     `compile_commands.json`:
     `callExpr(callee(functionDecl(isVariadic())), hasAnyArgument(hasType(asString("PyValue"))))`.
     `clang-query` 21 is installed in `/usr/lib/llvm-21/bin`.
   * or, under `Py_VALUE_CORE`, wrap the format macros so that each argument
     goes through a `_Generic` that rejects `PyValue`.

**B4. The dispatcher specification is incomplete for upstream semantics
(§2.5).**

*Evidence.*

* **Rich comparison is two-sided.** Line 371 says "Unary, compare, hash,
  truth, repr and getattr have one receiver and use the same rule with one
  side". That is wrong for comparison. `do_richcompare`
  (`Objects/object.c:1048`) tries the reflected operation first when the
  right operand's type is a strict subclass of the left's. It tracks
  `checked_reverse_op`, tries the right operand's slot with the swapped
  operator, and falls back to identity for `==` and `!=`.
* **Sequence fallbacks are missing.** `PyNumber_Add` falls back to
  `sq_concat` (`Objects/abstract.c:1148`). `PyNumber_Multiply` falls back to
  `sq_repeat` on either side, using `nb_index` of the other operand
  (`:1184`–`:1188`). The in-place forms use `sq_inplace_concat` and
  `sq_inplace_repeat` (`:1316`–`:1343`). `str`, `list` and `tuple` have no
  `nb_add`, so the most common object additions go through these paths. The
  page's table and dispatcher never mention them.
* **Ternary deduplication is not covered.** `ternary_op` drops `slotw` when it
  equals `slotv` (`:1052`) and drops `slotz` when it equals either
  (`:1089`). The page states the "chosen function" deduplication for binary
  operations only.
* **Truth has a fallback chain.** It tries `nb_bool`, then `mp_length`, then
  `sq_length`, then returns true. Truth is not single-slot.
* **Result checks are not mentioned.** `PyNumber_Index`, `PyNumber_Long` and
  `PyNumber_Float` reject non-int or non-float results and emit
  `DeprecationWarning` for strict subclasses. `slot_nb_bool` rejects a
  non-bool result. `slot_tp_hash` maps -1 to -2 and hashes an out-of-range
  result with `PyLong_Type.tp_hash`, which is a direct call to a core slot.
* **`NotImplemented` is undefined in value form.** The page does not say how
  it is represented or compared.

*Fix.* In §2.5, list each dispatcher (`binary_op1`, `binary_iop1`,
`ternary_op`, `do_richcompare`, `PyObject_IsTrue`, `PyNumber_Index`/`Long`/
`Float`, `PyObject_Hash`, the sequence fallbacks). State that each one is a
line-by-line port of the named upstream function, with two changes:
`Py_TYPE` becomes `PyValue_TYPE`, and each slot read becomes "the value slot,
else the object slot". Every deduplication compares the chosen function.
`NotImplemented` is the static immortal pointer value and is compared by
bits. Name the tests (I4).

### Important

**I1. The pair handling in `update_one_slot` is a requirement, not a design
(§2.4).**

*Evidence.* A wrapper descriptor stores one function, `d_wrapped`
(`Include/cpython/descrobject.h:57`). `update_one_slot` chooses the "specific"
function when `d->d_base->wrapper == p->wrapper` and
`specific == d->d_wrapped` (`Objects/typeobject.c:14148`–`14152`). A pair
needs two pointers. It also needs two wrapper kinds: a value wrapper for core
types, and an object wrapper for extension types, which have no value side.

The special cases each need a pair as well:

* `__next__` uses `_PyObject_NextNotImplemented` (`:14094`);
* `__hash__ = None` uses `PyObject_HashNotImplemented` (`:14169`, `:14727`);
* `tp_new` uses `tp_new_wrapper`;
* the `generic = *ptr` path reads whatever is in the current slot.

*Fix.*

* Give wrapper descriptors both `d_wrapped` (the value function, or NULL) and
  `d_wrapped_object`.
* Have `add_operators` choose the value wrapper when the type defines a value
  side.
* Make `specific` a pair.
* List in the pair table every generic object function that core slots name:
  `PyObject_GenericGetAttr`, `PyObject_GenericSetAttr`,
  `PyObject_HashNotImplemented`, `_PyObject_NextNotImplemented` and
  `PyVectorcall_Call`. Six core types set `tp_call = PyVectorcall_Call`.

**I2. Families missing from the inventory and the plan (§7.1, §9).**

Each row below is a core path with a polymorphic position that no V packet
covers. Counts are core calls, measured with `rg` over `Objects/`, `Python/`
and `Parser/`, excluding generated files and Clinic headers.

| Missing piece | Evidence | Assign to |
|---|---|---|
| Number construction | `PyLong_From*` 271 and `PyFloat_FromDouble` 53. Each one allocates a box that the canonical steal then frees | V2a: `PyValue_FromInt64`, `PyValue_FromSsize_t` and `PyValue_FromDouble` as the core constructors |
| Number extraction | `PyLong_As*` 156, `PyFloat_AsDouble` 23 | V2a: `_PyValue_AsSsize_t` and the others, with the inline fast path |
| Errors carrying a value | `PyErr_SetObject` 47, `_PyErr_SetKeyError` 11, `_PyGen_SetStopIterationValue` 6 | V2b |
| Handwritten `PyArg_Parse*` in core | 50 calls. Their outputs are boxes on the borrow stack, a third path from the core to object form beside the two adapters (contradicts §1.1 line 154) | V3: convert them to Clinic, then mark `PyArg_*` |
| Getset descriptors | `PyGetSetDef.get` is `PyObject *(*)(PyObject *, void *)`. `(5).real` runs `long_get_real` with a boxed receiver, so V2c's exit "`(5).real` boxes nothing" cannot be met without a value getter type, which the page does not define | V2c |
| Bound objects from `descr_get` | `PyMethod_New`, `method_get` and the method-wrapper store `self` as `PyObject *` | V2c, or V5 if `im_self`/`m_self` become homes |
| Monitoring | `_Py_call_instrumentation_arg`, `_2args` and `_exc2` take `PyObject *` (`Include/internal/pycore_instruments.h:56`–`65`). They receive return values, yields and the first `CALL` argument | V3: value form internally; box only for C-level `Py_tracefunc` |
| JIT helpers | `Python/generated_jit_helpers.c.h`: 6 object-form calls and 5 `PyValue_Box`. The page does not name `jit_helpers_generator.py` | each family's packet, beside the IR generator |
| V5 fields | Each new `PyValue` field needs `tp_traverse` (`_Py_VISIT_VALUE`), `tp_clear`, dealloc, `PyMemberDef` (`_Py_T_VALUE`), `__reduce__` and the home checker. None of this is listed | V5 |

Weakrefs and the buffer protocol need nothing new. Their receivers are object
positions, and under B1 they raise through `PyValue_TYPE`. Exceptions need
only the error-payload row above and `StopIteration.value`.

**I3. Value calls go back on PEP 590's per-instance dispatch (§3).**

*Evidence.* `PyCFunctionObject`, `PyMethodDescrObject` and `PyFunctionObject`
each carry a per-instance `vectorcall`, chosen once at creation from the
`METH_*` flags. The page's `call` is a per-type slot that "dispatches on the
flag" (line 388), so every call goes through the flag switch that PEP 590
removed.

Two more points are unspecified:

* A bound-method value call has to prepend `self`. That needs a writable slot
  before `args[0]`, as `PY_VECTORCALL_ARGUMENTS_OFFSET` gives object
  vectorcall, and §3 does not specify it.
* `im_self` stays `PyObject *` (§7.4, "cold"), so every bound-method call pays
  a canonical borrow on `self`.

*Fix.*

* Give core callables a per-instance value vectorcall pointer, set at
  creation. A value-native callable then has only that pointer, and its
  object `vectorcall` is a shared thunk.
* State the offset rule for value arrays.
* Decide whether `im_self` and `m_self` are `PyValue` homes, in V3 rather
  than V5.

**I4. Rollout: order, size and exit criteria (§9).**

* **V2a cannot meet its exit.** V2a marks `_PyNumber_Index` and
  `PyObject_IsTrue`. Core Clinic headers call them 53 times (§7.1 line 520)
  until value emission arrives in V3. Either move the Clinic generator change
  and the regeneration of core headers into V2a, or mark those two functions
  in V3.
* **V4 is not independent of V2.** V4 turns getter results into `PyValue`
  while their consumers are still object-form. Each converted site therefore
  gains a temporary `PyValue_AsObject` and is touched again in V2 or V3. More
  generally, a sweep family by family touches a function once for each family
  it calls. Better order:
  1. Land the mechanisms and value forms of every family: the V2a, V2b and
     V2c mechanisms, the V3 mechanism, and the V4 value forms.
  2. Sweep file by file, all families at once.
  3. Mark each family when its last core caller is gone.

  The ratchet is unchanged; each site is edited once.
* **V2a is too big for one packet.** For about 20 agent-hours it lists the
  table, inheritance, slotdefs, the dispatcher, the thunk macro, value slots
  for six families on all 129 types, value `slot_*` for Python classes, the
  interpreter, a rewrite of `value_ops.c`, and 381 call sites. The whole
  representation packet is sized at 42 agent-hours. Split it:
  * **V2a-0.** Only the mechanism, with every core type still on the object
    side. The whole suite then runs through the adapter, which tests pair
    inheritance, slotdefs, `update_one_slot` and deduplication before any type
    changes.
  * **V2a-1.** `int`, `float`, `bool`, `NoneType`, `str` and `tuple`.
  * **V2a-2.** The remaining types and the sweep.
* **V3 waits on IR v2 for no reason.** Only the interpreter's call handlers
  depend on IR v2. Split V3:
  * **V3a.** C-level value vectorcall, `METH_VALUE` and Clinic value emission.
  * **V3b.** The interpreter's call handlers, after IR v2.

  If IR v2 slips, V3a does not.
* **No packet names its tests.** Add these to each packet's exit:
  * existing modules: `test_binop`, `test_descr`, `test_class`,
    `test_richcmp`, `test_compare`, `test_pow`, `test_index`, `test_bool`,
    `test_hash`, `test_capi.test_number`;
  * an extension type with only object `nb_add` and `tp_richcompare`;
  * a static extension subtype of `int` that overrides `nb_add` (rule 2),
    another that overrides only `tp_repr` (B2), and one that overrides
    nothing;
  * a Python subclass of an extension type;
  * a dunder assigned after class creation, and assigned on a base after a
    subclass exists;
  * `__class__` assigned between classes with different value slots;
  * `pow(a, b, c)` where `c` is of a third type;
  * reflected comparison with a subclass of `int`.
* **The sweep needs a tool, and V4 is not mechanical.** V4's "879 calls
  (mechanical)" changes result types from `PyObject *` to `PyValue`. Every
  later use of the local then changes too: `Py_DECREF`, type checks, pointer
  comparisons, `PyObject *` fields and variadic error messages (B3). Use a
  Clang-based rewriter (LibTooling or libclang; Clang 21 is installed, and
  Coccinelle is not). It should sort sites into two classes:
  * **mechanical:** the result is used only in polymorphic positions and then
    released;
  * **judgement:** the result is used as a typed object, stored in a
    `PyObject *` field, compared by pointer, or passed to a variadic call.

  Step 6a's classifier is the precedent. Measure how the sites split before
  sizing V4.

**I5. Identity on round trips (§1.3).**

The page covers two exits that give two different boxes. It does not cover a
round trip, where a compatibility caller passes in a box `X` and later gets
back a different box `X'`:

* `PyObject_SetAttr(ext_obj, name, X)` and
  `PyObject_RichCompare(X, ext_obj, op)`: the thunk canonicalises `X`, and the
  adapter hands the extension's slot `X'`;
* `PyList_Append(l, X)` followed by `PyList_GetItemRef(l, -1)` returns `X'`.

Cython compiles `is` to C pointer equality. So Cython code that compares an
int or float it stored with one it read back now gets a different answer.
Inline floats are never NaN, so the identity shortcut in
`PyObject_RichCompareBool` never changes a result.

*Fix.* State the round-trip rule and the Cython case, and add a `_testcapi`
test. Do not keep the caller's pointer alongside the value; that would be a
second form.

**I6. Cost claims and what each packet delivers (C8, §2.1, §9).**

* **Entry cost is understated.** C8 calls it "a type load and two compares".
  `PyValue_FromObjectBorrow` is out of line (`Python/pyvalue.c:9`). It calls
  `_PyObject_ValueIdentity` (`:227`), which for a non-compact int calls
  `PyLong_AsLongAndOverflow`. After int51, every int of 31 to 51 bits takes
  that call. Fix: inline the pointer case, keep an out-of-line tail for ints,
  and state the int51 cost.
* **The adapter boxes again what the thunk just unboxed.** When an extension
  calls a thunk that reaches another extension's object slot (for example
  `PyNumber_Add(X, ndarray)`), the path makes two conversions and one
  allocation for each inline operand. Upstream passes the pointer through.
  NumPy and pandas code mixing arrays with Python scalars (`arr + 1000`,
  `x * 0.5`) pays one allocation per operation, where upstream uses the
  constant's object. This is permanent (C11). §6 should state it as a known
  cost.
* **"Same loads as upstream" (§2.1) is not quite right.** `PyValue_TYPE` adds
  a branch for inline values. The inline table of B2 removes one load.
* **Thunk code size is not a concern.** 430 macro-generated thunks of 60 to
  100 bytes each come to about 30–40 KB.
* **The page does not say which packet removes which regression.** That is
  needed to judge the order, even without performance gates. Proposed
  mapping, from the G4 list in `pyvalue.md` and §7.4 here:

  | Regression against upstream | Removed by |
  |---|---|
  | `hash(tuple)`, mixed compares, `str(x)` and f-strings | V2a |
  | Generic `d[k]`, `x in s`, `for` over dict views | V2b |
  | `(5).real`, attribute reads that return inline values | V2c, with value getters (I2) |
  | `lst.append(1000)`, `len`, `isinstance`, `min`/`max` and other builtins with int arguments | V3 |
  | `range(n)` for `n` above 256 | V5 |
  | `_pickle`, `itertools`, `_operator`, `_collections` | V6 |

  Over upstream, the kernels gain only from the representation itself: no
  allocation for int and float results. The value API removes the
  regressions that the representation added at C boundaries. So the end
  state can beat upstream on the kernels only after V3 and V5. V2 alone
  should not be expected to.

**I7. Enforcement mechanics (§4).**

* **Header inline functions.** On GCC 15.2, a use inside an *unused*
  `static inline` body is an error, and so is a use inside a function that is
  itself marked `unavailable`. Clang 21 allows the second case. (Tested.) So
  six header inline functions would break every value-core unit as soon as
  their family is marked:
  * `PyObject_CallMethodNoArgs` and `PyObject_CallMethodOneArg`
    (`Include/cpython/abstract.h`);
  * `_PyObject_VectorcallTstate`, `_PyObject_CallNoArgsTstate` and
    `_PyObject_CallNoArgs` (`pycore_call.h`);
  * `_PyObject_HashDictKey` (`pycore_object.h`).

  Fix: guard them with `#ifndef Py_VALUE_CORE`, or port them with their
  family. Macros that expand to a marked function, such as
  `PyTuple_SET_ITEM` and `PyList_SET_ITEM`, are caught at the expansion site
  as intended.
* **Direct slot calls.** A direct call through an object slot (33 sites, for
  example `Py_TYPE(it)->tp_iternext(it)`) cannot be marked, because slot
  fields are not functions. Fix: have `check_value_api.py` flag reads of
  paired object-slot fields in value-core units. The adapter units opt out
  with `#define Py_VALUE_ADAPTER` in the file itself, which needs no separate
  list.
* **The `Py_DEPRECATED(0)` fallback** in §4 contradicts C7, which says
  `deprecated` is not a ratchet. The build profile already requires GCC 12 or
  later, or Clang. Use `#error` for anything else.

**I8. `PyValue_Box` keeps the non-canonical form that D2 removes (§1.3, line
185).**

`PyValue_Box` turns an inline value into a `PyValue` that points to a box of
an exact int or float. That is the second form of one number that D2
forbids in homes, and it competes with `PyValue_AsObject` as a second boxing
exit. Its failure contract differs as well (§7.2 inconsistency 2).

*Fix.* Delete it in V2a. A caller that needs an object calls
`PyValue_AsObject`, and the generator emits that call.

### Minor

**M1. Stale line numbers.** `value_binary_operation` is now at
`Python/ceval.c:989` (the page says `:996`), and `_PyValueIndexPair` is at
`Include/object.h:319` (the page says `:316`). Re-pin the tree.

**M2. The 430-thunk count.** The script does not print the 527 and 430
totals. I could reproduce 527 only by leaving out `tp_vectorcall` (20
entries). Six of the 16 `tp_call` entries are the generic
`PyVectorcall_Call`, which the 97 generic entries do not include, so the true
number of per-type thunks is about 424. Fix: have the script print the totals
and the generic set.

**M3. Prior-art accuracy (§8).** Corrections are in the prior-art section
below.

**M4. Wording.** §2.3 rule 3 says a type with only a value side "does not
occur". Under B2, the pair table makes it impossible by construction. Say so,
and keep the debug assert.

## Answers to the eight questions

1. **Boundary.**
   * **Not one form per position.** C1 makes "object position" depend on the
     operation, while the values involved come from polymorphic sources.
     That forces a judgement and an unchecked conversion at each site (B1).
   * **Places where a reader still chooses between a value API and an object
     API for the same thing:**
     * allowed object-form generics next to forbidden thunks with the same
       prefix (B1);
     * `PyValue_AsPyObjectBorrow` against `PyValue_AsObject` for a value
       that may be inline (B1);
     * `PyValue_Box` against `PyValue_AsObject` (I8);
     * the deferred `_PyValue_FromObjectKnown` (open question 1; decide it
       now, B1).
   * **Not dual forms:** C4's two boxing lifetimes (C locals for slots, the
     borrow stack for vectors). They have different reasons.
2. **Semantics.**
   * **Binary deduplication** ("chosen function") holds for every case I
     traced:
     * two extension types;
     * an extension subtype of `int` with and without its own `nb_add`;
     * Python classes sharing `slot_nb_add`;
     * `class P(Ext)`.
   * **The exception** is an extension static subtype with no value storage,
     which gives a double call (B2).
   * **Broken or unspecified as written:**
     * rich-compare reflection, three-argument `pow` deduplication, the
       sequence fallbacks, and the `__index__`, `__int__`, `__float__`,
       `__bool__` and hash result rules (B4);
     * `update_slot` and the specific case in `update_one_slot` (I1).
   * **Holds:**
     * `__class__` assignment, because value slots are reached through the
       type and change with it;
     * multiple inheritance, because `inherit_slots` runs once per MRO entry
       and the first base that defines a slot wins on both sides.
   * **Extension subclasses of core types** depend on B2. **Python subclasses
     of extension types** work once I1 is fixed.
   * **Identity:** the exit rule is correct, but the round-trip and Cython
     cases are missing (I5).
3. **Completeness.**
   * **Missing:** getset value getters, monitoring, number construction and
     extraction, error payloads, handwritten `PyArg_*` in core, the JIT
     helper generator, and V5's GC, clear and dealloc work (I2); per-instance
     value vectorcall and bound-method `self` (I3); variadic formatting (B3).
   * **Covered by the slot table:** `tp_descr_get` with a value instance and
     `PyObject_GenericGetAttr` results. Covered by B1's rule: weakrefs and
     buffers.
   * **Covered by the slot type but not the dispatcher:** three-argument
     `pow` (B4).
4. **Cost.**
   * **Entry:** understated (I6).
   * **Adapter:** it boxes again on round trips; that is permanent, so state
     it (I6).
   * **Thunks:** fine.
   * **Table location:** put it inline (B2).
   * **End state:** plausibly faster than today's tree after V2b and V3, and
     faster than upstream on numeric kernels after V3 and V5. Mixed
     extension-and-scalar code stays slower than upstream (I6).
5. **Enforcement.** `unavailable` is the right tool, with four limits:
   * header inline functions on GCC (I7);
   * direct slot-field calls (I7);
   * variadic arguments (B3);
   * Clinic object emission in core before V3 (I4).

   Static initialisers are handled correctly by the pair table, once
   `PyVectorcall_Call` and `_PyObject_NextNotImplemented` are added to it
   (I1).
6. **Rollout.**
   * **Order:** fix the V2a/V3 Clinic ordering and V4's independence, split
     V2a and V3, and sweep by file (I4).
   * **Representation packet:** the dependencies are stated correctly. V2a
     needs steps 5–8 and 9a, and step 4 (`NULL = 0`) is not a dependency.
   * **Tooling:** the sweep needs a Clang-based classifier and rewriter
     (I4).
   * **Criteria:** entry criteria are present; test exits are missing (I4).
7. **Prior art.** See the section below. V8 and JSC suggest the better
   boundary: generic operations take the tagged value (B1).
8. **Inventory.** Three counts checked; all hold (section below).

## Prior art (§8)

* **JavaScriptCore** is described accurately. On 64-bit, `toRef` encodes the
  `JSValue` as bits; on 32-bit, a non-cell is wrapped in a `JSAPIValueWrapper`
  (`APICast.h`).
* **`_PyStackRef`** is described accurately.
* **V8.** "Internal code never includes `v8.h`" is too strong. `src/inspector`,
  `src/d8` and `src/extensions` are written against the public API. The
  claim holds for the engine core (runtime, builtins, compiler), which is the
  part that matters here.
* **Lua.** "No pointer to a value leaves the VM" holds for `TValue`, but
  `lua_tostring` and `lua_touserdata` do return pointers into objects.
* **HPy.** I could not verify within the budget that CPython's universal mode
  uses a handle table. My recollection is that the CPython universal backend
  also casts handles to `PyObject *`, and that handle tables are used on PyPy
  and GraalPy and in debug mode. Check `hpy/universal/src/handles.h` before
  citing it.
* **PyPy.** I did not re-verify the cpyext figures from Cuni (2018).

The more useful lesson is the one the page leaves out. In both V8 and JSC,
internal generic operations take the tagged value for every operand, including
receivers, and turn a primitive into an object inside the operation
(`Object::ToObject`; `synthesizePrototype`). That is B1's fix.

## Inventory spot-check

| Count | Page | Rerun at `379e92cded9` | Holds? |
|---|---|---|---|
| §7.1 object-form calls in core (`valueapi-objcalls.py`) | 2172; dict 352 | 2173; dict 353; every other family identical | Yes; the tree moved by one call |
| §7.6 Clinic (`valueapi-clinic.py`) | 435 functions, 181 with polymorphic parameters, 501 parameters, 258 polymorphic | Identical | Yes |
| §7.3 slots (`valueapi-slots.py`) | 527 polymorphic, 430 thunks | 547 including `tp_vectorcall`; 527 without it; 430 after the 97 generic entries | Yes by the page's definition; about 424 with `PyVectorcall_Call` counted as generic (M2) |
| §7.5 boxing sites (the page's `rg` command) | 131 in 25 files | 131 in 25 files | Yes |

## Decisions for the coordinator

1. **Object positions (B1).** Generic operations take `PyValue` for every
   Python operand, with the inline test inside the operation. Or keep C1 and
   accept a judgement at each site plus a conversion that checks in release
   builds. I recommend the first.
2. **Table storage (B2).** Inline in `PyTypeObject`, or a pointer with
   allocation at `PyType_Ready` for static types. I recommend inline. This
   also settles open question 6.
3. **Typed entry (open question 1).** Adopt `_PyValue_FromTyped` now, as a
   raw tag with a debug assert.
4. **Variadic check (B3).** Choose `clang-query` in `check_value_api.py` or
   `_Generic` wrappers, and choose a value format conversion.
5. **Sweep organisation (I4).** Land the mechanisms and value forms first,
   then sweep by file, then mark by family. Or keep the page's order and
   accept double edits.
6. **Packet splits (I4).** V2a into V2a-0, V2a-1 and V2a-2. V3 into V3a and
   V3b, so that V3a does not wait for IR v2. Move Clinic value emission into
   V2a, or delay marking the number and bool families until V3.
7. **Per-instance value vectorcall (I3)**, and whether `im_self` and `m_self`
   become homes in V3.
8. **Delete `PyValue_Box` in V2a (I8).**
9. **Round-trip identity (I5).** Document it as permanent, as I recommend,
   or not.
