# Value API: steps M1b and M2 (proposal)

Status: proposal, revision 2, 2026-11-04, after
[review 1](../reviews/value-api-m1b-m2-review-1.md) and the coordinator's
rulings. Two separable mechanism steps of [the route to R27](value-api-v2a0.md)
§1.4: **M1b** (subscript, item, `ass_*`, contains, `iternext` and send
dispatch) and **M2** (attribute and descriptor dispatch with value
receivers). Both integrate on `typeobject.c` in the order V2a-1a, M1b, M2.

Base: the shared runtime tree at `f7d374b1f92` (A5b-3 and V2a-0 part 2
landed); A3 step 2's `a3s2-interface.md`; the M3 proposal, revision 1.

## Changes after review 1

| # | Finding | Revision 2 |
|---|---|---|
| 1 | Out-parameter core plus a two-word wrapper is two forms | Ruled: one form. Every three-outcome result is `_PyStatusValue` returned by value, from the slot through the dispatcher to the handler (B4, §1.2, §2.2). Thunks over it (§1.3). I3 and I4 call the dispatchers directly; `PySendResultPair` and `_PyIter_Send` go at I3. The VA7 edit is in §5 |
| 2 | A7 misses `slot_tp_descr_get`; the hook's pair fails the debug check | `_PyType_WriteSlotPair` is the only writer after ready; the `(_Py_slot_tp_getattro, value)` row joins the `slot_*` pair table; `check_value_api.py` rejects other writes (A7, §2.4) |
| 3 | Three "boxes nothing" claims are false | Corrected in §2.1, §2.2 and the test tables. The 14 value wrappers take M3a's value shape and land with or after M3a's wrapper reshape (B9, §1.3); the "no box" rows name the step that makes them true; `list` and `dict` cases replaced by `tuple`, `range` and `str` |
| 4 | `PyIter_Check` on the value side rejects extension iterators | Rule: a presence test reads the object slot; only an identity test of a pair-table function moves (§2.6). `PyIter_Check` keeps `tp_iternext != NULL` |
| 5 | The getset record tags `closure` | Typed `d_value` field in `PyGetSetDescrObject`, set once at creation; SOABI tag bumped under D15 (A3, §2.3) |
| 6 | The asm-label alias is untyped | `Objects/value_pairs.c`, compiled with the marks off, holds the pair tables (A8) |
| 7 | `super()`'s cell branch boxes; a rebound `__class__` gives an inline `self` | Cell read through `_PyCell_GetValue`; out-parameter retyped; both cases tested (§2.5) |
| 8 | Strict `zip` and `map` must drop the inner value | Strict mode closes `v` and checks the lengths (§1.2 contract table; tests in §1.6) |
| 9 | 2.4 h of mechanical work sat with the lead | Moved to workers; §4 recomputed; W2's decrement in units |
| 10 | `ceval.c` and `call.c` edits tie M2 to A3a and M3a | M2 edits neither: the object functions those files call become thunks; `_PyValue_GetMethod`'s declaration is published at M2's start; M3a owns `call.c`, I4 owns `ceval.c` (§3) |
| 11 | StopIteration notes; undefined `AsValue` suffix | `_PyGen_SetStopIterationValue` and `_PyGen_FetchStopIterationValue` retyped in place; `StopIteration_init` and the Cython read go to W2's field batch and the ports list (Q1) |
| 12 | Iteration layers; two tests | Kept; `list(agen.asend(None))` and the RAISE callback's object added (§1.6) |
| 13 | Receivers | Kept |
| 14 | `_PyValue_Size` against value-api.md's `_PyValue_Len` | Kept `_PyValue_Size` (§1.5 rule 1); value-api.md edit in §5 |

## 0. Decisions, one line each

| # | Decision |
|---|---|
| B1 | Value slots keep V2a-0's typed signatures, except `iternext` and `send` (B4). The receivers of `subscript`, `item`, `ass_subscript`, `ass_item`, `contains`, `iternext` and `send` stay `PyObject *` (§1.1). Keys, items and sent values are `PyValueBorrow`; results are owned |
| B2 | Generic dispatchers take a `PyValueBorrow` receiver and test the tag before they read a slot (R1). Dispatchers on an iterator take `PyObject *`, the typed result of `_PyValue_GetIter` |
| B3 | Three iteration layers: `_PyValue_IterStep` (R11's raw slot contract), `_PyValue_IterNext` (the normalising consumer), `_PyValue_Send` (`PyIter_Send`). `next()` and the wrapping iterators use the raw layer (R11, R16) |
| B4 | **VA7 as amended.** A result with three outcomes is `_PyStatusValue {int status; PyValue value;}`, returned by value in two registers. No value function takes an out-parameter for its result; pointers remain for arrays and in-out state |
| B5 | When two object families give the same `<Op>` (§1.5 rule 1), the `PyObject_` form takes the bare name: `_PyValue_GetItem`, `_PyValue_SequenceGetItem` |
| B6 | `_PyErr_SetKeyErrorValue` builds the args tuple from the word and boxes nothing. `StopIteration(v)` still boxes `v` into `PyStopIterationObject.value`, a counted CORE box, until W2 makes that field a home (Q1) |
| B7 | M1b converts by hand only the types whose iteration is subtle or that its tests need: 6 generator-family functions, `map`, `filter`, `zip`, `enumerate`, and (by workers) the 9 container iterators behind the four `_Py*Iter_NextValue` helpers. The 52 subscript-family slots stay in W2 |
| B8 | A Python class's object `slot_*` becomes the thunk of its value `slot_*` and keeps its name. The five `slot_*` that pass a key, index or value to a dunder need a value call and land in M1b-d, on V2a-1a-iii |
| B9 | M1b's 8 and M2's 6 value wrappers are written once, in M3a's value shape. They are diffs M1b-e and M2-e, which land after M3a's wrapper reshape; until then their slotdef rows keep wrapper `NONE` and the object wrapper calls the thunk |
| A1 | `getattro`, `setattro`, `descr_get` and `descr_set` keep V2a-0's signatures, with a `PyValueBorrow` receiver or `obj`: `object.__getattribute__(1, 'real')` and `int`'s own descriptors see inline receivers. The descriptor is a `PyObject *` |
| A2 | The generic attribute pair gets value sides, `_PyObject_GenericGetAttrValue` and `_PyObject_GenericSetAttrValue`, filled by the pair table for 81 + 9 core types and their heirs |
| A3 | Getset value getters (open question 2): the getset entry names a shared object thunk whose `closure` is a typed record; `PyDescr_NewGetSet` copies the record pointer into a new `d_value` field once. Reads test `d_value`, as wrapper descriptors test `d_wrapped_value` (R19) |
| A4 | M2 gives value sides to `type`, `module`, `super`, getsets and members. Method, classmethod, wrapper and function descriptors wait for M3a's `self` homes; until then their adapter boxes an inline `obj` |
| A5 | `superobject.obj` becomes a `PyValue` home; zero-argument `super()` reads frame local 0, or its cell, as a word |
| A6 | Identity tests of pair-table functions move to the value side; presence tests, `slot_*` tests and two-type comparisons stay (§2.6) |
| A7 | Every slot write after `PyType_Ready` goes through `_PyType_WriteSlotPair(tp, row, obj, value)`. Its two callers are the getattr hook and `slot_tp_descr_get` |
| A8 | A marked function's address is taken only in `Objects/value_pairs.c`, which defines `Py_VALUE_PAIR_TABLE` so that `Py_VALUE_COMPAT` expands to nothing, holds only table data, and uses the real declarations |

## 1. M1b

### 1.1 Receivers

**Only a dispatcher can reach these slots with an inline receiver**: an
inline value is an exact int or float, and neither `int`, `float` nor
`object` defines `__getitem__`, `__setitem__`, `__delitem__`, `__contains__`,
`__next__` or `send`. The dispatcher raises upstream's `TypeError` before
`PyValue_AsKnownObjectBorrow` (R1), and the 93 core slot functions keep
upstream's bodies. V8 draws the same line: a Smi reaches property lookup
through its map, never an element or iterator stub.

### 1.2 Interfaces

```c
/* pycore_value.h (moved from A3 step 2's ceval.c). status -1: error, value NULL;
 * 0: absent, ended or RETURN, value owned or NULL; 1: found or NEXT, value owned. */
typedef struct { int status; PyValue value; } _PyStatusValue;
static_assert(PYGEN_ERROR == -1 && PYGEN_RETURN == 0 && PYGEN_NEXT == 1, "");

/* pycore_value_slots.h: the two retyped value slots; the other five are unchanged */
typedef _PyStatusValue (*_Py_iternextvaluefunc)(PyObject *self);          /* R11 */
typedef _PyStatusValue (*_Py_sendvaluefunc)(PyObject *self, PyValueBorrow arg);
```

A value `send` slot returns RETURN with the return value (`None` when there
is none), as `am_send` does; a value `iternext` slot returns RETURN with
`PyValue_NULL` for a `None` return (R11). Dispatchers live in
`pycore_value_ops.h`; each ports the third column line by line.

| Value form | Status | Port of | Object thunks |
|---|---|---|---|
| `PyValue _PyValue_GetItem(PyValueBorrow o, PyValueBorrow key)` | – | `PyObject_GetItem`: `subscript`; `item` after `_PyIndex_Check`; `__class_getitem__`; "is not subscriptable" | `PyObject_GetItem` |
| `int _PyValue_SetItem(PyValueBorrow o, PyValueBorrow key, PyValueBorrow v)`, `int _PyValue_DelItem(o, key)` | – | `PyObject_SetItem`, `DelItem` | both |
| `PyValue _PyValue_SequenceGetItem(PyValueBorrow s, Py_ssize_t i)`; `int …SetItem(s, i, PyValueBorrow v)`; `int …DelItem(s, i)` | – | `PySequence_*Item`, negative index through the object `sq_length` | the three |
| `_PyStatusValue _PyValue_GetOptionalItem(PyValueBorrow o, PyValueBorrow key)` | 1 found, 0 missing | `PyMapping_GetOptionalItem`; an exact dict reads the entry's word | `PyMapping_GetOptionalItem`, `…String` |
| `int _PyValue_Contains(PyValueBorrow seq, PyValueBorrow item)` | – | `PySequence_Contains`, then `_PyValue_IterSearch` | `PySequence_Contains`, `PySequence_In` |
| `Py_ssize_t _PyValue_IterSearch(PyValueBorrow seq, PyValueBorrow obj, int op)` | – | `_PySequence_IterSearch` | `_PySequence_IterSearch` |
| `Py_ssize_t _PyValue_Size(PyValueBorrow o)` | – | `PyObject_Size`; `len(5)`: "object of type 'int' has no len()" | `PyObject_Size`, `PyObject_Length` |
| `PyObject *_PyValue_GetIter(PyValueBorrow o)` | – | `PyObject_GetIter` | `PyObject_GetIter` |
| `_PyStatusValue _PyValue_IterStep(PyObject *it)` | R11 raw | one call of the value slot, or the adapter | – |
| `_PyStatusValue _PyValue_IterNext(PyObject *it)` | 1 item, 0 end (value NULL) | `abstract.c`'s `iternext()`: RETURN closes `v`; ERROR with `StopIteration` clears it and gives 0 | `PyIter_Next`, `PyIter_NextItem` |
| `_PyStatusValue _PyValue_Send(PyObject *it, PyValueBorrow arg)` | `PySendResult` | `PyIter_Send`: the `send` slot; else for `None` `_PyValue_IterStep` (RETURN NULL gives `None`; ERROR with `StopIteration` gives RETURN with `_PyGen_FetchStopIterationValue()`); else `.send(arg)`, boxing an inline `arg` until M3a | `PyIter_Send`; `_PyIter_Send` until I3 |
| `_PyStatusValue _PyValue_NextNotImplemented(PyObject *it)` | -1 | `_PyObject_NextNotImplemented` | pair-table value side |
| `void _PyErr_SetKeyErrorValue(PyValueBorrow key)` | – | `_PyErr_SetKeyError` (§1.5 rule 2) | `_PyErr_SetKeyError`, kept for `ceval.c` until I3 |
| `int _PyGen_SetStopIterationValue(PyValueBorrow)`; `PyValue _PyGen_FetchStopIterationValue(void)` (`None` when no value; NULL on error) | – | retyped in place; callers `genobject.c`, `abstract.c`, `_asynciomodule.c` (2) | none |

About 40 more functions of the families (`PySequence_List`, `Fast`, the
slice and `InPlace` forms, `PyMapping_Keys`, `HasKey*`, `*String`,
`PyObject_LengthHint`, …) get worker-written value forms that test the tag,
raise upstream's `TypeError` for an inline receiver and run the existing
body; the object functions become thunks, so W2 can mark both families
(P3). Core calls: 283 sequence and mapping, 108 iteration.

**The iteration contract, by caller (R11, R16):**

| Caller | Calls | RETURN with `v` | ERROR with `StopIteration` pending |
|---|---|---|---|
| Generic consumers: `list()`, `tuple()`, `sum`, unpacking, `_PyValue_IterSearch`, `list_extend_iter` | `_PyValue_IterNext` | end; `v` closed | end; cleared |
| `map`, `filter`, `zip`, `enumerate` value slots | `_PyValue_IterStep` on the inner iterator | returned unchanged, `v` included; strict `zip` and `map` close `v`, check lengths, and return RETURN with `PyValue_NULL` or raise `ValueError` | returned unchanged; strict mode clears it and checks as upstream |
| `next(it[, d])` | `_PyValue_IterStep` | `StopIteration(v)`, bare for `PyValue_NULL`; with `d`, `d` | passed through; with `d`, cleared, `d` returned |
| `PyIter_Send`, `SEND` (I3) | `_PyValue_Send` | RETURN `v` (`None` for NULL) | RETURN with the exception's value; cleared |
| Generic `FOR_ITER` (I3) | `_PyValue_IterStep` | end; only if RAISE is monitored and `v` is not NULL does it make `StopIteration(v)` and emit RAISE, as `gen_iternext` does upstream | RAISE event, then cleared |
| Object thunk of a core `iternext` | the value slot | `StopIteration(v)` only for a non-NULL `v`; NULL | NULL, exception as raised |

**StopIteration with a value.** A generator that returns `5` gives
`{PYGEN_RETURN, 5}`: no exception, no box. An object-form iterator raises
`StopIteration(5)`, and the adapter gives ERROR with it untouched. An
exception object is made only where upstream's caller could observe it
(`next()`, an object thunk, `FOR_ITER` under monitoring), boxing `v` (B6).
This is LuaJIT's `ITERC` shape (the end as a status) in two registers;
V8's allocated `{value, done}` result is rejected.

### 1.3 Adapters, thunks, value wrappers

The adapters (static, `abstract.c`) box only an inline key, item or value,
as SLOT_ADAPTER, class SEQUENCE or ITERATION:

| Kind | Object slot | Boxes | Result |
|---|---|---|---|
| SUBSCRIPT, ITEM | `mp_subscript(self, key_o)`, `sq_item(self, i)` | key / nothing | `_PyValue_FromObjectResult` |
| ASSSUBSCRIPT, ASSITEM | `mp_ass_subscript(self, key_o, value_o)`, `sq_ass_item(self, i, value_o)` | key, value / value | int |
| CONTAINS | `sq_contains(self, item_o)` | item | int |
| ITERNEXT | `tp_iternext(self)` | nothing | an item: `{1, v}`; NULL, nothing pending: `{0, NULL}`; NULL, any exception: `{-1, NULL}`, untouched |
| SEND | `am_send(self, arg_o, &r)` | arg | `{status, r}` |

The thunks share one exit, beside V2a-0's `_PyValue_THUNK_EXIT`:

```c
/* pycore_value_slots.h: box the value (kind THUNK); a failed box is an error. */
static inline int
_PyValue_THUNK_EXIT_STATUS(_PyStatusValue r, PyObject **result)
{
    *result = _PyValue_THUNK_EXIT(r.value);         /* NULL stays NULL */
    return (*result == NULL && !PyValue_IsNull(r.value)) ? -1 : r.status;
}

PyObject *PyIter_Next(PyObject *it)
{   PyObject *o; (void)_PyValue_THUNK_EXIT_STATUS(_PyValue_IterNext(it), &o); return o; }
PySendResult PyIter_Send(PyObject *it, PyObject *arg, PyObject **result)
{   return _PyValue_THUNK_EXIT_STATUS(_PyValue_Send(it, _Py_VALUE_ARG(arg)), result); }
```

`PyIter_NextItem`, `PyMapping_GetOptionalItem`, `PyObject_GetOptionalAttr`,
`_PyObject_LookupSpecial`, `_PyType_LookupRef`, `_PySuper_Lookup` and the
`_SEND` thunk macro (`am_send`) are one call of the same exit. The
`_ITERNEXT` macro raises `StopIteration(v)` for RETURN with a non-NULL `v`
(closing it), exits NEXT's item, and returns NULL otherwise. The other
macros are `_SUBSCRIPT`, `_ITEM`, `_ASSSUBSCRIPT`, `_ASSITEM`, `_CONTAINS`.

**Value wrappers (B9)** take M3a's shape,
`PyValue (*)(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames, void *wrapped)`
(M3 §1.4; depends on M3a's interface note, its review being open). Until
M3a's `wrapperdescr_value_call`, `wrapperdescr_call` boxes `self` with
`PyTuple_GetItemRef` (`descrobject.c:597`), so the old shape would save
nothing. M1b's eight (`wrap_subscriptvalue`, `sq_itemvalue`,
`asssubscriptvalue`, `delitemvalue`, `sq_setitemvalue`, `sq_delitemvalue`,
`containsvalue`, `nextvalue`) check arity as M3a's do and pass
`PyValue_AsKnownObjectBorrow(self)`; `wrap_nextvalue` raises for RETURN as
`wrap_next` does (`StopIteration(v)`, bare for NULL).

### 1.4 What M1b converts

| Unit | Change | Count |
|---|---|---:|
| `pycore_value.h`, `pycore_value_slots.h` | `_PyStatusValue` (unless A3 step 2 has moved it, §5); the two slot typedefs; V2a-0's `iternext`/`send` test types and their 2 adapters retyped | – |
| `abstract.c` | 14 dispatchers (§1.2); 7 adapters; about 40 thin value forms with thunks; `_PyIter_Send` a thunk of `_PyValue_Send`; `PyIter_Check` (§2.6) | 14 + 7 + 40 |
| `genobject.c` | `gen_send_value(gen, PyValueBorrow arg) -> _PyStatusValue` replaces `gen_send_ex2`'s body; it stores `DUP(arg)` and, until I3 or I5, unboxes the eval loop's result with `PyValue_FromObjectSteal`. Value `send` for the 4 `am_send` types (`PyGen_am_send`, `asend`); value `iternext` for generator, coroutine wrapper, `asend` and `athrow`. Object sides by macro | 6 functions |
| `bltinmodule.c`, `enumobject.c` | Value `iternext` for `map`, `filter`, `zip`, `enumerate` (R16, with strict mode as in §1.2's table); `next()` on `_PyValue_IterStep`; `all`, `any` on `_PyValue_IterNext`: all 10 `tp_iternext` calls in the two files | 4 types, 10 calls |
| `listobject.c`, `tupleobject.c`, `dictobject.c`, `setobject.c` | The four `_Py*Iter_NextValue` helpers become the value `iternext` of listiter, tupleiter, the six dict iterators and setiter (worker units); NULL then means only error (C10); `list_extend_iter` calls `_PyValue_IterNext` | 9 types |
| `typeobject.c` | Value `slot_tp_iternext` with the object side its thunk (B8); slotdef value functions; the `_PyObject_NextNotImplemented` pair row; M1b-d's five value `slot_*`; M1b-e's 8 wrappers and rows | 6 functions, 8 wrappers |
| `errors.c`, `_asynciomodule.c` | `_PyErr_SetKeyErrorValue`; 2 retyped calls | – |
| `ceval.c`, `bytecodes.c` | None (§1.5). Of the core's 15 `tp_iternext` calls M1b converts 13; I3 takes `ceval.c`'s, W2 `bytearrayobject.c`'s | 0 |

### 1.5 What I3 calls

A3 step 2's `value_for_iter_next`, `value_mapping_get_optional_item` and
`value_async_gen_send` become direct calls returning two registers.

| Handler | Today | After I3 |
|---|---|---|
| generic `BINARY_OP` `NB_SUBSCR` | `value_binary_operation` boxes both operands | `_PyValue_GetItem`; the exact-dict and exact-list arms stay ahead of it |
| `STORE_SUBSCR`, `DELETE_SUBSCR` | `value_store_subscript` boxes all three | `_PyValue_SetItem`, `_PyValue_DelItem` |
| `CONTAINS_OP` | `PySequence_Contains` on object views | `_PyValue_Contains` |
| `GET_ITER` | `_PyEval_GetIter` boxes with `PyValue_Box` | `_PyValue_GetIter` |
| `FOR_ITER`, `INSTRUMENTED_FOR_ITER` | `_PyForIter_VirtualIteratorNext(…, &item)` calls `tp_iternext` | it returns `_PyStatusValue`; the virtual index stays in its frame slot (R-d); otherwise `_PyValue_IterStep` with §1.2's `FOR_ITER` row |
| `SEND` | `_PyIter_Send` returns `PySendResultPair` on a boxed `v` | `_PyValue_Send`; I3 deletes `_PyIter_Send` and `PySendResultPair` |
| `LOAD_GLOBAL`/`BUILD_*` lookups, `DICT_MERGE` | `PyMapping_GetOptionalItem` through the wrapper | `_PyValue_GetOptionalItem` |
| `UNPACK_*`, `GET_LEN`, `CALL_LEN` | object iteration and size | `_PyValue_GetIter`, `_PyValue_IterNext`, `_PyValue_Size` |
| `INTERPRETER_EXIT` | `PyValue_AsObject(retval)` | returns the word, so `gen_send_value` drops its conversion. I5 makes the change if it integrates first |

### 1.6 Tests (M1b)

Suites: `test_iter`, `test_itertools`, `test_generators`, `test_yield_from`,
`test_coroutines`, `test_asyncgen`, `test_builtin`, the container and
`test_contains`/`test_enumerate` suites, `test_exceptions`,
`test_capi.test_abstract`, `test_monitoring`, `test_value_slots`. New cases:

| Case | Expectation |
|---|---|
| R16: `next(map(str, g()))`, `g` returning 5; through `filter`, `zip` (first and later argument), `enumerate`; `g` returning `None` | `StopIteration` with `.value == 5`; bare for `None` |
| Strict mode: `zip(g(), [], strict=True)`, `zip([], g(), strict=True)`, `map(f, g(), [1], strict=True)` | upstream: bare `StopIteration`, or `ValueError` |
| `_testcapi` iterators raising `StopIteration(v)`, returning NULL with nothing pending, raising `ValueError`; each through `for`, `next()`, `next(it, d)`, `list()`, `yield from`, `zip`, `map`; `iter()` on each | upstream's exception, value and traceback; `iter()` accepts them (finding 4) |
| A Python `__next__` raising `StopIteration(v)` or a subclass | reaches `next()`'s caller intact |
| A core generator's object `tp_iternext` from C for `return None` and `return 5`; `PyIter_Send` with `1000` and `None`; `list(agen.asend(None))` | NULL with nothing pending / `StopIteration(5)`; upstream results |
| `for` over a generator returning 5 and `None`, under `sys.monitoring` | one RAISE event whose callback receives a `StopIteration` with `.value == 5` / none |
| `__next__ = None`; `__next__` added after creation, and on a base after a subclass exists | "'X' object is not iterable"; both sides rewritten (debug invariant) |
| Extension types with only `mp_subscript` or only `sq_item`, indexed by `-1`, `1000`, `2**70`, `'a'`; `5[0]`, `len(5)`, `iter(2.5)`, `1 in 5`, `next(5)` | upstream's order, adjustment and messages |
| `tuple.__getitem__(t, -1)`, `range.__getitem__(r, 1000)`, `tuple.__contains__(t, 2.5)`, `str.__getitem__('abc', 1)`; `_PyErr_SetKeyErrorValue(1000)` | upstream results, `args == (1000,)`; no box once M3a's wrapper value call and M1b-e land |
| Stats build: `for i, x in enumerate(floats)`, `for a, b in zip(floats, floats)` | no SLOT_ADAPTER or THUNK box at M1b |

### 1.7 Box rows (route §1.2; recorded, not gated)

| Row | Per pass | M1b alone | Rest of the removal |
|---|---:|---|---|
| List and tuple iterators | 730,260 | `spectral_norm`'s 676,260 go: its `enumerate(u)` and `zip(u, v)` over float lists now read listiter's value side. Tupleiter's 27,000 go where the consumer is converted | W2, remaining consumers |
| `STORE_SUBSCR` | 862,590 | none: the box is in `value_store_subscript` | I3, plus W2's container `ass_subscript` (W4 for `array`) |
| Generator return to C | 288,654 | none: `INTERPRETER_EXIT` boxes | I5 or I3 |
| Dict iteration and lookup | 232,084 | none measured: `_json.c`'s `PyDict_NextEntryRef` | W1b, W4 |

## 2. M2

### 2.1 Receivers; `(1).real` and `(2.5).hex`

`(1).real` reaches its getter with no box: `PyLong_Type`'s value `getattro`
is `_PyObject_GenericGetAttrValue` (pair table), which finds a getset
descriptor; `getset_get_value` runs `descr_check` on `PyValue_TYPE(v)` and
calls `d_value`'s getter (§2.3). `long_real_value` returns
`PyValue_DUP(self)` for an inline int and `long_long`'s value for a heap
int, so `True.real` is the int 1. `(2.5).hex()`: `_PyValue_GetMethod`
returns `{1, descriptor}` with the value as `self`; no box. `(2.5).hex`:
`method_get` has no value side until M3a's `m_self` home, so the adapter
boxes `2.5` (SLOT_ADAPTER). `object.__getattribute__(1000, 'real')` boxes
`1000` in `wrapperdescr_call` until M3a and M2-e (B9). `obj` is
`PyValue_NULL` for class access; an inline class attribute is never a
descriptor, so the lookup tests `PyValue_IsObject` before a descriptor slot.

### 2.2 Interfaces

| Value form | Status | Port of | Object thunks |
|---|---|---|---|
| `PyValue _PyValue_GetAttr(PyValueBorrow v, PyObject *name)` | – | `PyObject_GetAttr`; `tp_getattr` through the adapter | `PyObject_GetAttr`, `…String`; `_PyObject_GetAttrValue` (its `ceval.c` caller converts at I4) |
| `_PyStatusValue _PyValue_GetOptionalAttr(PyValueBorrow v, PyObject *name)` | 1 found, 0 missing | `PyObject_GetOptionalAttr`, fast paths keyed on value sides | `GetOptionalAttr`, `…String`, `HasAttr`, `HasAttrWithError`, `…String` |
| `int _PyValue_SetAttr(PyValueBorrow v, PyObject *name, PyValueBorrow value)` (NULL deletes) | – | `PyObject_SetAttr` | `SetAttr`, `…String`, `DelAttr`, `…String` |
| `_PyStatusValue _PyValue_GetMethod(PyValueBorrow obj, PyObject *name)` | 1 unbound method (call with `obj`), 0 attribute | `_PyObject_GetMethodValue` | `_PyObject_GetMethodValue`, signature kept until its `call.c` (M3a) and `ceval.c` (I4) callers convert |
| `PyValue _PyValue_DescrGet(PyObject *d, PyValueBorrow obj, PyTypeObject *t)`; `int _PyValue_DescrSet(PyObject *d, PyValueBorrow obj, PyValueBorrow v)` | – | slot choice for `tp_descr_get`, `tp_descr_set` | – |
| `PyValue _PyObject_GenericGetAttrValue(PyValueBorrow v, PyObject *name)`; `_PyStatusValue _PyObject_GenericLookupAttrValue(v, name, PyObject *dict)` | 1, 0 | `_PyObject_GenericGetAttrWithDict`; instance reads `DUP` the `PyDictValues` word or the dict entry's | `PyObject_GenericGetAttr`, `_PyObject_GenericGetAttrWithDict` |
| `int _PyObject_GenericSetAttrValue(v, name, value)`, `…WithDict` | – | `_PyObject_GenericSetAttrWithDict` | `PyObject_GenericSetAttr`, `…WithDict` |
| `_PyStatusValue _PyType_LookupValue(PyTypeObject *, PyObject *name)` | 1, 0; never -1 | `_PyType_LookupRef` (15 calls) | `_PyType_LookupRef` |
| `_PyStatusValue _PyValue_LookupSpecial(PyValueBorrow self, PyObject *name)` | 1, 0 | `_PyObject_LookupSpecial` (15 calls) | `_PyObject_LookupSpecial` |
| `_PyObject_LookupSpecialMethod` | – | fixed in place: it stops boxing `self` and writing the box back | – |
| `PyValue _PyMember_GetOneValue(const char *, PyMemberDef *)`; `int _PyMember_SetOneValue(char *, PyMemberDef *, PyValueBorrow)` | – | `PyMember_GetOne`, `SetOne`; numeric members return V2a-1a's `PyValue_FromInt64`/`FromDouble` | both |
| `_PyStatusValue _PySuper_LookupValue(PyTypeObject *, PyValueBorrow obj, PyObject *name, int want_method)` | 1 unbound method, 0 attribute | `_PySuper_Lookup` | `_PySuper_Lookup` (I4 converts its caller) |

Core calls of the attribute family: 294 (`PyObject_GetOptionalAttr` 73,
`PyObject_GetAttr` 58). The 4 adapters (SLOT_ADAPTER, class ATTRIBUTE) box
an inline value or `obj`; `int` and `float` carry the generic pair, so no
inline receiver reaches GETATTRO or SETATTRO. Thunk macros: `_GETATTRO`,
`_SETATTRO`, `_DESCRGET`, `_DESCRSET`. M2-e's 6 wrappers (`wrap_getattrvalue`,
`setattr`, `delattr`, `descr_get`, `descr_set`, `descr_delete`) take M3a's
shape (B9).

### 2.3 Getset value getters (open question 2, settled)

```c
/* Include/internal/pycore_descrobject.h */
typedef PyValue (*_Py_valuegetter)(PyValueBorrow self, void *closure);
typedef int (*_Py_valuesetter)(PyValueBorrow self, PyValueBorrow value, void *closure);
typedef struct { _Py_valuegetter get; _Py_valuesetter set; void *closure; } _PyValueGetSet;
PyAPI_FUNC(PyObject *) _PyGetSet_ValueGet(PyObject *self, void *record);   /* shared thunks */
PyAPI_FUNC(int) _PyGetSet_ValueSet(PyObject *self, PyObject *value, void *record);
#define _Py_VALUE_GETSET(NAME, VGET, VSET, DOC, CLOSURE) /* and _Py_VALUE_GETTER */ \
    {NAME, _PyGetSet_ValueGet, _PyGetSet_ValueSet, DOC, \
     (void *)&(const _PyValueGetSet){VGET, VSET, CLOSURE}}

/* Include/cpython/descrobject.h */
typedef struct {
    PyDescr_COMMON;
    PyGetSetDef *d_getset;
    void *d_value;      /* const _PyValueGetSet *, or NULL (R19, as d_wrapped_value) */
} PyGetSetDescrObject;

/* PyDescr_NewGetSet, once per descriptor */
descr->d_value = getset->get == _PyGetSet_ValueGet ? getset->closure : NULL;
```

The record is the shared thunk's own `closure`, as any getset closure
belongs to its getter: only `_PyGetSet_ValueGet`/`ValueSet` interpret it.
`getset_get_value` and its setter follow the rule of every value slot:
`d_value` non-NULL means a value call (3 loads), NULL means the adapter.
C code that calls `d_getset->get` reaches the shared thunk. **SOABI:**
abi3 is unchanged (`PyGetSetDescrObject` is outside the limited API;
`PyGetSetDef` keeps its stride), but the struct grows, so D15's standing
rule bumps the fork's tag once more (`boilvalrc2` to `boilvalrc3`, or the
next when M2 lands). A new descriptor type is rejected: `type(int.real)`
and `types.GetSetDescriptorType` would change. V8's `AccessorInfo` record
likewise pairs a C getter with its data and passes a primitive receiver
unwrapped. Workers convert the 7 getsets an inline receiver reaches: `int`'s
`real`, `imag`, `numerator`, `denominator`; `float`'s `real`, `imag`;
`object.__class__` (get and set). The other 132 are W2's.

### 2.4 Generic pair, `type`, `module`, Python classes, writes after ready

- **Generic pair.** `_PyObject_GenericLookupAttrValue` removes the two
  `BOX_SITE_ATTRIBUTE` exits (inline-values hit, plain class attribute).
  `_PyObject_TryGetInstanceAttribute` keeps one caller (`import.c`, W2's).
  `AttributeError.obj` is an object field, so `(1).foo` boxes on the error
  path (Q4).
- **`type`, `module`.** `_Py_type_getattro_value` becomes `type`'s value
  `getattro`, with `_Py_type_getattro` and `…_impl` its thunks;
  `type_setattro` and `_Py_module_getattro` get value sides.
- **Python classes.** The value hook and `_Py_slot_tp_getattro` need no
  value call and land in M2 proper; `slot_tp_setattro`, `slot_tp_descr_get`
  and `slot_tp_descr_set` call a dunder with a value and land in M2-d.
- **Writes after ready (A7).** The hook rewrites `tp_getattro`, and
  `slot_tp_descr_get` clears `tp_descr_get` when `__get__` is gone
  (`typeobject.c:12514`). Both call
  `_PyType_WriteSlotPair(PyTypeObject *tp, const slotdef *row, void *obj, _Py_valuefunc value)`,
  which stores both fields with nothing between (correct under the GIL).
  The hook writes `(_Py_slot_tp_getattro, its value side)`, a row added to
  the `slot_*` pair table so V2a-0 §4.4's debug check accepts it;
  `slot_tp_descr_get` writes `(NULL, 0)`, so the data-descriptor test reads
  alike on both sides. `check_value_api.py` fails on any other assignment
  to an object slot outside `PyType_Ready`, `update_one_slot` and the helper.

### 2.5 `super()`

`superobject.obj` becomes a `PyValue` home (M4a's traverse and clear).
`super_init_without_args` takes `PyValue *obj_p`: a plain local 0 is
copied with `PyValue_DUP` (no `BOX_SITE_FRAME` box), a cell local 0 (`self`
captured by a closure) is read with `_PyCell_GetValue` (no `PyCell_Get`
box). `supercheck` uses `PyValue_TYPE` and `_PyValue_GetOptionalAttr`;
`do_super_lookup` calls `_PyValue_DescrGet`; `__self__` is a `_Py_T_VALUE`
member. Zero-argument `super()` can see an inline `self`: after
`A.f.__closure__[0].cell_contents = object`, `A.f(5)` runs
`super(object, 5)`; the home holds the word. `super(int, 5).__repr__`
boxes at the wrapper descriptor's adapter until M3a. `super().m()` with a
heap `self` boxes nothing at I4: `_PySuper_LookupValue` returns
`{1, function}` and the handler keeps `self`.

### 2.6 Identity tests

**Rule.** A presence test (`slot != NULL`) reads the object slot, which an
extension fills while its value slot is 0. An identity test moves to the
value side only when it names a pair-table object function, since naming
it becomes an error once W2 marks it. Tests naming a `slot_*` thunk (B8) or
comparing two types' slots stay (R14). HEAD has 22 tests:

| File | Tests | Change |
|---|---|---|
| `object.c` (6) | `_PyObject_GetAttrValue`'s `_Py_type_getattro`; `GetOptionalAttr`'s `PyObject_GenericGetAttr`, `_Py_type_getattro`, `_Py_module_getattro`; `_PyObject_GetMethodValue`'s `!= PyObject_GenericGetAttr`; the setter's `== PyObject_GenericSetAttr` | M2: value side; the first becomes the slot call |
| `abstract.c` (2) | `PyIter_Check`'s `_PyObject_NextNotImplemented`; `PyAIter_Check`'s | M1b: `tp_iternext != NULL && tp_value.iternext != _PyValue_NextNotImplemented` (the pair table fills the value side for an extension that names `_PyObject_NextNotImplemented`); `PyAIter_Check` stays (`am_anext` is not a value slot) |
| `typeobject.c` (9) | `update_one_slot`'s `PyObject_HashNotImplemented` | M2: value side |
| | `_PyType_HasSlotTpIternext`, the `slot_tp_descr_get` test (`slot_*` names); `hackcheck` (4); `tp_descr_get == base->tp_descr_get`; `tp_hash == tp_base->tp_hash` | stay |
| `specialize.c` (4) | `!= PyObject_GenericGetAttr`; `!= PyObject_GenericSetAttr`; `!= _Py_type_getattro` | M2: value side |
| | `_Py_slot_tp_getattr_hook`/`_getattro` | stays (`slot_*` names) |
| `bytecodes.c` (1, plus its generated copy) | `_LOAD_SUPER_ATTR_METHOD`'s `PyObject_GenericGetAttr` | I4 |

M2 moves 10, M1b 1, I4 1; 10 stay. A7's lint and D6's debug assertion
catch a write that would break the equality. Marked functions are named
only in `Objects/value_pairs.c` (A8), which `check_value_api.py` fails if it
gains a function body; `typeobject.c` reads its tables through an `extern`.

### 2.7 What I4 calls

| Handler | After I4 (A3 step 2's `super_lookup` wrapper becomes a direct call) |
|---|---|
| generic `LOAD_ATTR` | `_PyValue_GetMethod` (method form) or `_PyValue_GetAttr`; the boxed `owner` view and its temporary box go |
| generic `STORE_ATTR`, `DELETE_ATTR` | `_PyValue_SetAttr`; the two temporary boxes go |
| `LOAD_SPECIAL` | `_PyObject_LookupSpecialMethod` as M2 fixed it; the error path's box goes |
| `LOAD_SUPER_ATTR_ATTR`, `_METHOD` | `_PySuper_LookupValue`, and §2.6's `bytecodes.c` test |
| `IMPORT_FROM` | `_PyValue_GetOptionalAttr` on the module |

### 2.8 Tests (M2)

Suites: `test_descr`, `test_class`, `test_property`, `test_super`,
`test_module`, `test_opcache`, `test_capi`, `test_inspect`,
`test_dataclasses`, `test_enum`, `test_value_slots`. New cases:

| Case | Expectation |
|---|---|
| `(1).real`, `(2.5).imag`, `(1000).numerator`, `(2**60).denominator`, `(1).__class__` | upstream results (`True.real` is the int 1); no box in the stats build at M2 |
| `object.__getattribute__(1000, 'real')`, `type.__getattribute__(int, 'real')` | upstream results; no box once M3a and M2-e land |
| `(1).foo`; `setattr(1, 'real', 2)`; `(1).__class__ = bool`; `del (1).real` | `.name`, `.obj == 1`; upstream's messages |
| `_testcapi` `ObjGetAttr` (object `tp_getattro`/`tp_setattro` only); an extension getset | `setattr(o, 'x', 1000)` gives one SLOT_ADAPTER box and round-trips; the getset works through the adapter |
| `_testinternalcapi` type with a `_Py_VALUE_GETSET`, read from Python, by `PyObject_GetAttr` and by a direct `d_getset->get`; `type(int.real) is types.GetSetDescriptorType` | the same value each way; true |
| `super(int, 5).__self__`, `super(int, 5).__repr__()`, `super(float, 2.5).__sizeof__()`; `C.f(5)` with zero-argument `super()`; a `super` subclass; `super()` in a classmethod | upstream results (one SLOT_ADAPTER box per bound wrapper until M3a); upstream's `TypeError` |
| `self` captured by a lambda in a method using `super()`; `A.f.__closure__[0].cell_contents = object`, then `A.f(5)` and `A.f(2.5)` | upstream results; the cell read boxes nothing |
| `class C: x = 1000`: `C().x`, `C.x`, `getattr(C(), 'x')` | no box at the C level |
| A class with only `__getattribute__`, read once, then given `__getattr__`; one that loses it | the pair holds (debug invariant) |
| A class whose `__get__` is deleted after first use, as instance attribute and as data descriptor test | upstream results; both sides NULL |
| A module with `__getattr__`; `math.pi` through `PyObject_GetAttr` | upstream results |
| `test_opcache`: an extension type with `tp_getattro = PyObject_GenericGetAttr` | still specialises `LOAD_ATTR` |

### 2.9 Box rows (M2)

Instance attributes (101,083 per pass): the generic getter's 71,242
(richards, raytrace, deltablue) and `super()`'s frame and cell reads go at
M2; `value_load_attribute`'s 20,200, `LOAD_SUPER_ATTR`'s and
`LOAD_SPECIAL`'s at I4; bound methods and wrapper calls of inline values at M3a.

## 3. Files and collisions

| Step | Files | Collides with | Resolution |
|---|---|---|---|
| M1b | `abstract.c`, `genobject.c`, `bltinmodule.c`, `enumobject.c`, the four container files (iterators), `typeobject.c`, `errors.c`, `_asynciomodule.c` (2 lines), `pycore_value.h`, `pycore_value_slots.h`, `pycore_value_ops.h`, `pycore_genobject.h`, `pycore_abstract.h`, test modules | V2a-1a (`typeobject.c`); W1b (container files, disjoint functions); A3 step 2 (`_PyStatusValue`, §5) | integrate after V2a-1a; rebase over W1b |
| M1b-e, M2-e | `typeobject.c` (14 wrappers and rows) | M3a's wrapper reshape | after it; folded into M1b-c and M2-c if M3a lands first |
| M2 | `object.c`, `typeobject.c` (`type`, `super`, the hook, `_PyType_WriteSlotPair`, identity tests), `descrobject.c` (getset and member functions), `structmember.c`, `moduleobject.c`, `dictobject.c` (instance reads), `specialize.c`, `Objects/value_pairs.c` (new; the pair tables move from `typeobject.c`), `Makefile.pre.in`, `PCbuild`, `configure.ac` (SOABI), `check_value_api.py`, `Include/cpython/descrobject.h`, `pycore_descrobject.h`, `pycore_object.h`, test modules | M3a (`descrobject.c`'s method, classmethod and wrapper descriptors; `typeobject.c`) | split by function; no `ceval.c` or `call.c` edit. M2-a's first commit publishes `_PyValue_GetMethod`'s declaration for M3a's `_PyValue_VectorcallMethod` |
| I3, I4 | `bytecodes.c`, `ceval.c`, the generator | A3a | after A3a (route §1.4) |

## 4. Steps and estimates

Review 1's rates: by hand, a dispatcher 8 lead minutes, an adapter 5, a
semantic slot 6. Workers (Sonnet or Haiku, 16 at a time, one 30-minute
cleanup) take every mechanical unit at 3 minutes.

| Part | M1b units | M1b (h) | M2 units | M2 (h) |
|---|---|---:|---|---:|
| Lead, by hand | 14 dispatchers (112 min); 7 adapters (35); `gen_send_value` and 6 generator slots (42); 4 R16 slots, `next`, `all`, `any` (42); `slot_tp_iternext` (6); M1b-d's 5 `slot_*` (30) | 4.45 | 9 dispatchers (72); generic pair, 4 functions (40); 4 adapters (20); getset `d_value` and record (20); 2 member functions (16); `type` (16); `module` (8); hook, `_PyType_WriteSlotPair`, lint (15); `super`, 6 functions (36); `LookupSpecialMethod` (6); M2-d's 3 `slot_*` (18) | 4.45 |
| Review changes | `_PyStatusValue`, 2 typedefs, the status exit, 2 adapters, test types (30) | 0.5 | cell read, 2 tests (12); `value_pairs.c` and its lint, SOABI (21) | 0.55 |
| Workers | 7 macros, 8 wrappers and 8 rows (M1b-e), about 40 thin forms, 9 iterator slots, `list_extend_iter`, 1 identity test, `_PyGen_*` and `_PyErr_SetKeyErrorValue` call sites (5), about 25 test cases: 104 units | 5.2 | 4 macros, 6 wrappers (M2-e), 10 identity tests, 9 thin forms, 3 object functions as thunks, 7 getters, about 25 test cases: 64 units | 3.2 |
| Integration, both compilers, stats table | – | 1.0 | – | 1.0 |
| **Total** | lead 5.95, workers 5.2 | **11.15** | lead 6.0, workers 3.2 | **9.2** |

About **20.35 agent-hours**: lead 11.95 with integration, workers 8.4.
With a second agent on diff b (generators, R16, getsets, `super`), diff a
sets the lead's time: 177 minutes in M1b and 153 in M2; worker rounds take
under 1 h beside them. With integration, about **4 h for M1b and 3.5 h for
M2**, inside the route's T7–T12 window, with no wait on A3a. The d diffs
follow V2a-1a-iii (about T12.5), the e diffs M3a's wrapper reshape; neither
gates W2.

**What leaves W2.** The 8.35 h above the route's 12 are W2 units moved
forward plus review fixes. W2 shrinks by **49 thin forms** (40 sequence,
mapping and iteration; 9 attribute), **9 container iterator slots**, **7
getters**, **6 generator-family functions**, **4 R16 slots** and the
attribute sides of **3 types** (`type`, `module`, `super`): 5.25 h at W2's
rates, plus 2.5 h of their tests. The other 0.6 h is review 1's mechanism.

## 5. Edits other documents need

| Document | Edit |
|---|---|
| `value-api.md` VA7 row, §1.4 "Absence and exhaustion" and "Iteration and send", C10 | Three-outcome results return `_PyStatusValue {int status; PyValue value;}` by value (status -1, 0, 1 = `PYGEN_ERROR`, `RETURN`, `NEXT`); no out-parameter for a result; pointers only for arrays and in-out state; NULL still means only error |
| `value-api.md` R11 row, §2.1 slot table, §2.5 | `iternext` and `send` value slots return `_PyStatusValue`; "in `*out`" becomes "in `value`"; dispatcher signatures as §1.2 and §2.2 here |
| `value-api.md` §1.2 table, §1.5 rule 1, §2.5 | `_PyValue_Len` becomes `_PyValue_Size`; `PyObject_Length` is a second thunk name |
| `value-api.md` open question 2, §7.7 getset row | Settled: record macros, shared thunk, `d_value` in `PyGetSetDescrObject`, SOABI bump under D15 |
| `value-api.md` §2.2 | 22 identity tests at HEAD; the presence/identity rule of §2.6 |
| `value-api-v2a0.md` §4.4 | Replace "nothing in the tree writes an object slot after `PyType_Ready`" with the two writers and `_PyType_WriteSlotPair`; the pair tables live in `Objects/value_pairs.c` (A8) |
| `value-api-v2a0.md` §1.4, §1.6 | M1b and M2 rows: diffs a–e and §4's hours; W2's decrement in §4's units; W1b gains a script class converting the existing out-parameter value functions (`PyDict_GetValue`, `_PyDict_GetValue_KnownHash`, `_PyDict_LoadGlobalValue`, `_PyDict_LookupIndexAndOwnedValue`, `_PyType_LookupValueAndVersion`, about 15 sites) to returned pairs; W2's field batch makes `PyStopIterationObject.value` a home, `StopIteration_init` reading `PyTuple_GetValue` |
| A3 step 2 (`a3s2-interface.md`, `register-ir-v2-a3.md` §3.5) | Define `_PyStatusValue` in `pycore_value.h`, not `ceval.c`, in the fold before it lands; otherwise M1b-a deletes the `ceval.c` typedef, its one `ceval.c` line. Record `value_for_iter_next`, `value_mapping_get_optional_item`, `value_async_gen_send` and `super_lookup` as interim, deleted by I3 and I4; `_PyForIter_VirtualIteratorNext` returns `_PyStatusValue` |
| M3 proposal (§1.4, collision table) | M3a's interface note carries the value-wrapper typedef that M1b-e and M2-e use; M3a owns `call.c` and calls `_PyValue_GetMethod`; the last of M3a and I4 deletes `_PyObject_GetMethodValue` |
| Ports list (after 10b) | Cython reads `((PyStopIterationObject *)e)->value` |

## 6. Risks and open questions

| # | Question or risk | Recommendation |
|---|---|---|
| Q1 | `PyStopIterationObject.value` is a public `PyObject *`, so `StopIteration(v)` boxes | Settled by review 1: move the struct internal and make `value` a home in W2's field batch (§5) |
| Q4 | `AttributeError.obj` and other exception fields box an inline receiver on error paths | Record as CORE boxes on error paths; W2's §7.4 cold fields remove them |
| Q5 | `next(it)` is an object-form native, so it boxes an inline item at return | Record; M3b's `METH_VALUE` removes it |
| Q7 | B9 depends on M3a's wrapper shape, whose review is open | If the shape changes, only M1b-e and M2-e change; nothing else in either step reads it |
| R1 | Workers may move a thin form's body instead of wrapping it, or drop the inline-receiver `TypeError` | One before-and-after example per class in the batch header; §1.6's message cases |
| R2 | `gen_send_value` sits under `SEND_GEN`'s inlined frames and the throw paths; a change in when the return value is stored can lose a `yield from` value | `test_yield_from`, `test_coroutines`, `test_asyncgen`, `test_generators`' throw cases on both compilers, plus §1.6's `return 5` case from C |
| R3 | SLOT_ADAPTER boxes rise on core containers between M1b and W2 (C callers of thunks with inline keys) | Recorded, not gated; W2 removes them |
| R4 | A 16-byte struct returns through memory on Windows x64 | Costs what an out-parameter costs there; the choice rests on one form (review 1, Q2), not on speed |
