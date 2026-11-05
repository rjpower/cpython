# Review 1: value-API steps M1b and M2 (proposal)

Reviewed: [value-api-m1b-m2.md](../proposals/value-api-m1b-m2.md) (revision 1,
462 lines, uncommitted, 2026-11-04). Checked against
[value-api.md](../value-api.md) (VA7, R1, R11, R14, R16, R19, C7, §1.2–§1.5,
§2, open question 2), [value-api-v2a0.md](../proposals/value-api-v2a0.md)
(§1.4, §1.7, §4), [register-ir-v2-a3.md](../proposals/register-ir-v2-a3.md)
revision 3 (§3.5, G9 rules R-a to R-d), the M3 proposal (in flight), the
common brief, and A3 step 2's `a3s2-interface.md` and `a3-2.diff`. Code,
read-only: `build/worktrees/pyvalue-rc-universal` at `c71a3ca6aa2`, with
V2a-0 part 2 staged. Line numbers are at that tree.

## Verdict

**Accept with changes.** The receiver split (B1, B2), the R11/R16 contract
table, the adapter kinds, B8's thunk identity and the `super` home are sound.
Four must-fix findings change the interfaces before M1b-a starts:

- **One result form (Q2).** Three-state results should be a two-word
  `{status, value}` return, from the slot to the interpreter. Keeping VA7's
  out-parameter makes A3 step 2's reshaping wrappers a permanent second layer.
  This needs a coordinator ruling that amends VA7 (finding 1).
- **A7 misses a writer.** `slot_tp_descr_get` also clears its slot after
  ready, and the hook's new pair is not one that the debug check accepts
  (finding 2).
- **Three claims that a path makes no box are false at this tree** (finding 3).
- **`PyIter_Check` on the value side** would reject every extension iterator
  (finding 4).

The estimate's total holds within 5% when recomputed from the proposal's own
counts. Its split between lead and workers does not (finding 9).

## Findings

| # | Severity | Section | Problem | Change |
|---|---|---|---|---|
| 1 | must-fix | B4, §1.2, §2.2, Q2 | **Out-parameter core plus a two-word wrapper is two forms.** G9 R-a says that a function the interpreter calls takes values and returns a value or a two-word struct. B4 keeps `int f(…, PyValue *out)` for 10 new functions and the two slots, and sends I3 and I4 through `ceval.c` wrappers. A3 step 2 already has nine `Py_NO_INLINE` wrappers whose only job is to turn an out-parameter into a return: `value_for_iter_next`, `value_get_iter`, `value_mapping_get_optional_item` (4 handlers), `super_lookup`, `value_load_global`, `value_async_gen_send` and three for object-form APIs. Under B4 the first five become permanent, and M1b and M2 add more for `_PyValue_Send`, `_PyValue_GetMethod`, `_PyValue_GetOptionalAttr`, `_PyValue_LookupSpecial` and `_PySuper_LookupValue`. Each wrapper costs one more C call (7–11 instructions under GCC, A3 §2.2) on `FOR_ITER`, `SEND` and the generic `LOAD_ATTR` method path. Upstream itself already returns two words here: `PySendResultPair _PyIter_Send(iter, arg)` (`pycore_abstract.h:66`, GH-143732, called by `SEND` at `bytecodes.c:1646`), and the proposal turns it into a thunk of an out-parameter form. Q2's reason, "the helper is needed anyway for the virtual index", covers `FOR_ITER` only. Even there the helper keeps its index in a slot (R-d) and could return two words itself. `_PySuper_LookupValue(…, int *method)` adds a third shape for the lookup that `_PyValue_GetMethod` already has. The cost is lowest now: no core type implements the value `iternext` or `send` slot yet, only V2a-0's test types | Amend VA7: **a result of at most two words is returned by value as `_PyStatusValue {int status; PyValue value;}`.** Status is −1, 0 or 1. `PYGEN_ERROR`, `PYGEN_RETURN` and `PYGEN_NEXT` are the same three numbers, so one struct serves both the three-state functions and `PySendResult`. Pointers stay only for arrays and in-out state (`_PyDict_NextValue`'s position, R-d slots). Move the struct from `ceval.c` to `pycore_value.h`. `_Py_iternextvaluefunc` and `_Py_sendvaluefunc` return it, and so do `_PyValue_IterStep`, `IterNext`, `Send`, `GetOptionalItem`, `GetOptionalAttr`, `GetMethod` (status 1 means a method, 0 an attribute), `_PyValue_LookupSpecial`, `_PyType_LookupValue`, `_PyObject_GenericLookupAttrValue` and `_PySuper_LookupValue` (`int want_method` in, status out). Delete `PySendResultPair` and `_PyIter_Send`. The existing out-parameter value functions (`PyDict_GetValue`, `_PyDict_GetValue_KnownHash`, `_PyType_LookupValueAndVersion`, `_PyDict_LoadGlobalValue`, `_PyDict_LookupIndexAndOwnedValue`: about 15 sites) become a script class in W1b. I3 and I4 call the dispatchers directly; `_PyForIter_VirtualIteratorNext` returns `_PyStatusValue` and needs no wrapper. **Cost:** about 0.5 h in M1b-a (2 typedefs, 2 thunk macros, 2 adapters, the test types). W2 rewrites every non-interpreter caller anyway (about 190 calls: `PyObject_GetOptionalAttr` 73, `PyIter_Next` 38, `PyMapping_GetOptionalItem` 26, `_PyObject_LookupSpecial` 15, `_PyType_LookupRef` 15, and the `NextItem`, `Send` and `HasAttr` forms), so the sweep costs nothing extra. A thunk stays one statement longer. On Windows x64 a 16-byte struct returns through memory, which costs the same as an out-parameter |
| 2 | must-fix | A7, §2.4, §2.6 | **The tree writes slots after ready in two places, and the debug check rejects the hook's new pair.** (a) `slot_tp_descr_get` sets `tp->tp_descr_get = NULL` when `__get__` is gone (`typeobject.c:12514–12515`). Once M2-d gives it a value side, that side stays non-zero beside a NULL object slot. This breaks R14, and the generic getattr's "is it a data descriptor" test then reads differently on each side. (b) The hook writes `(_Py_slot_tp_getattro, value)`. That pair is not in `slotdefs`, whose function for `__getattribute__` and `__getattr__` is the hook. So the debug dispatcher's assertion (V2a-0 §4.4: MRO base, pair table or `slot_*` table) fails on the first read after the rewrite. Under the GIL, writing both fields is otherwise correct: nothing runs between the two stores, and free threading is a non-goal | Route every write after ready through one helper, `_PyType_WriteSlotPair(tp, row, obj, value)`, used by the hook and by `slot_tp_descr_get` (writing 0, 0). Add the `(_Py_slot_tp_getattro, value)` row to the `slot_*` pair table. Have `check_value_api.py` fail on any other assignment to an object slot field outside `PyType_Ready` and `update_one_slot`. Correct V2a-0 §4.4's sentence "nothing in the tree writes an object slot after `PyType_Ready`". Tests: a class whose `__get__` is deleted after first use; a class with only `__getattribute__`, read once and then given `__getattr__` |
| 3 | must-fix | §1.3, §2.2 last sentence, §1.6, §2.8 | **These claims are false at this tree.** (a) `wrapperdescr_call` takes `self` with `PyTuple_GetItemRef(args, 0)` (`descrobject.c:597`). That call boxes an inline value (`tupleobject.c:127`, `BOX_SITE_TUPLE`), and V2a-0's value wrappers read their arguments the same way (`wrap_binaryvalue`). So `object.__getattribute__(1000, 'real')` boxes `1000`, and `tuple.__getitem__(t, -1)` boxes −1 into the cached int, which V2a-0 Q5 counts. (b) `list.__getitem__` and `dict.__getitem__` are `METH_COEXIST` method descriptors (`listobject.c:3239`, `dictobject.c:4894`). `list.__getitem__(l, 1000)` therefore never reaches a value wrapper; it boxes at the call adapter until M3a and W2 | M3a gives `wrapper_descriptor` a value call (`wrapperdescr_value_call`) and rewrites V2a-0's 12 value wrappers to a value shape (M3 proposal, wrapper descriptor row and collision table). Write M1b's 8 and M2's 6 value wrappers in that shape from the start, agreed with M3a, so that none is written twice. Move the "no box" expectations to M3a. Until then, have the wrappers read their arguments with `PyTuple_GetValue`. Restate the test rows with the step that makes each count zero. Replace the `list` and `dict` cases with wrapper-only types (`tuple`, `range`, `str` slots) |
| 4 | must-fix | §1.4 `abstract.c` row, §2.6 | **"`PyIter_Check` reads `tp_value.iternext`" breaks extension iterators.** Its first conjunct, `tp_iternext != NULL` (`abstract.c:3281`), tests whether the slot is present. An extension iterator has an object `tp_iternext` and a value side of 0 (the adapter), so `iter()` would raise "iter() returned non-iterator" for every one | State the rule in §2.6: **a presence test reads the object slot; only an identity test of a pair-table function moves to the value side** (because once the function is marked, naming it is an error). `PyIter_Check` keeps `tp_iternext != NULL` and compares `tp_value.iternext` with `_PyValue_NextNotImplemented`. Audit the other 13 moved tests the same way; `specialize.c`'s are identity tests and hold |
| 5 | should-fix | A3, §2.3 | **The getset record carries the hidden second encoding that the option table rejects.** `closure` means "record pointer" only when `get == _PyGetSet_ValueGet`. That is "tagging `closure`", the encoding the "flag and union" row rejects. The read also takes 5 loads (`d_getset`, `get`, `closure`, the record's `get` and `closure`). The rejected "field in `PyGetSetDescrObject`" option needs no hand-built map: the record already pairs the two getters | Keep the definition macros and the shared object thunk, which serves C callers of `d_getset->get`. Add `void *d_value` to `PyGetSetDescrObject`, set once in `PyDescr_NewGetSet` when `get == _PyGetSet_ValueGet`, as V2a-0 added `d_wrapped_value` (R19, `cpython/descrobject.h:60`). `getset_get_value` and its setter then follow the rule every value slot follows: non-zero means a value call, zero means the adapter. The comparison runs once per descriptor, not once per read, and the read takes 3 loads. D15 already bumps the SOABI for public layout changes. Reject a new descriptor type: `type(int.real)` would change, and so would `types.GetSetDescriptorType` and `inspect.isgetsetdescriptor` |
| 6 | should-fix | Q3, §2.6 | **The asm-label alias drops type checking and adds an allowlist.** The alias's prototype is never checked against `PyObject_GenericGetAttr`'s, so a later change to one of them is silent. On Mach-O the label needs `__USER_LABEL_PREFIX__`. A declaration in a shared header that a script keeps out of other units is an allowlist by another name. The same problem applies to `_PyObject_NextNotImplemented` and the hash pair (`typeobject.c:9969`) | One fixed rule: **a marked function's address is taken only in `Objects/value_pairs.c`**, a unit that defines `Py_VALUE_PAIR_TABLE` so that `Py_VALUE_COMPAT` expands to nothing there. The unit holds only the pair-table data and uses the real, type-checked declarations. `check_value_api.py` fails if the unit gains a function body. `typeobject.c` reads the table through an internal declaration |
| 7 | should-fix | §2.5, A5 | **`super()`'s frame read has two box sites, not one, and one claim is false.** (a) When `self` is captured by a closure, local 0 is a cell, and the fork reads it with `PyCell_Get` (`super_init_without_args`), which boxes an inline content. §2.5 deletes only the `BOX_SITE_FRAME` box. (b) "Zero-argument `super()` cannot see a valid inline `self`" is false: `__class__` cells are writable, so after `A.f.__closure__[0].cell_contents = object`, `A.f(5)` runs `super(object, 5)`. The design (a `DUP` into a value home) handles this; only the effects paragraph is wrong | Read the cell's value word (V4's `PyCell_GetValue`, or `ob_ref` under the cell's own accessor). Retype `super_init_without_args`'s out-parameter to `PyValue`. Add both cases to §2.8's tests: `self` captured by a lambda, and the rebound `__class__` cell |
| 8 | should-fix | §1.4 `bltinmodule.c` row, R16 | **`zip(…, strict=True)` and `map(…, strict=True)` do not pass the inner end through.** `zip_next`'s `check:` path (`bltinmodule.c:3292`) and `map_next`'s (`:1634`) clear the inner `StopIteration`, test the remaining iterators, and return NULL with no exception or raise `ValueError`. Passing RETURN `v` through in strict mode would make `next(zip(g(), strict=True))` raise `StopIteration(5)` where upstream raises a bare `StopIteration` | In strict mode, RETURN closes `v` and goes to the length check, and the slot returns RETURN with `PyValue_NULL`. Test it for `zip` and `map`, with `g` first and later, under `strict=True` |
| 9 | should-fix | §4 | **The estimate's split misassigns about 2.4 h of mechanical work to the lead.** Recomputed from §1.4, §2.2 and §4's unit counts below: about 20 agent-hours, as stated. But 11 thunk macros, 14 value wrappers, 8 table rows, 12 identity tests and both test sets are mechanical units that a 16-way fan-out finishes in under an hour of wall clock per step. The route's 12 h left out the 49 thin forms, 9 iterator slots and 7 getters, which come out of W2, plus the tests | Use the table below. Move the mechanical units to the workers. State the decrement to W2 in units (49 thin forms, 9 slots, 7 getters, 6 generator functions, 4 R16 slots), not "about 4 hours" |
| 10 | should-fix | §3, M2 row | **The `ceval.c` and `call.c` edits tie M2 to A3a and M3a.** M2 edits 3 lines in the `ceval.c` unit, which belongs to A3a until A3a lands (V2a-0 P4). A3 step 2 is still in rework (`a3-2r.diff`). "Whichever of M2 and M3a lands second converts `call.c`" leaves open who defines the value method lookup: M3's `_PyValue_VectorcallMethod` (M3 proposal, call family row) needs M2's `_PyValue_GetMethod` | Under finding 1, the 3 `ceval.c` lines become direct calls in I4, and M2 edits no `ceval.c` line. Publish `_PyValue_GetMethod`'s declaration (two-word) at the start of M2, give it to M3a, and let M3a own every `call.c` edit. M2 then collides with neither step |
| 11 | note | Q1, B6 | **StopIteration.** Moving the struct internal and making `value` a home in W2's field batch is right, with the same reason as `PySliceObject` in M4b. Three additions. Cython reads `((PyStopIterationObject *)e)->value`, which is a ports item after 10b. `StopIteration_init` reads `args[0]` with `PyTuple_GetItemRef`, a box, and should use `PyTuple_GetValue`. `_PyGen_SetStopIterationValueAsValue` and `…FetchStopIterationValueAsValue` add a suffix that §1.5 does not define, beside internal functions whose callers are all core. Retype `_PyGen_SetStopIterationValue` and `_PyGen_FetchStopIterationValue` in place: the object forms become counted thunks under P3 and are deleted by W2 | – |
| 12 | note | B3, §1.2 | **Three iteration layers are the minimum.** The raw step is the slot choice, which `next()`, the R16 wrappers and `FOR_ITER` need unnormalised. `IterNext` is the one normalising consumer, so the roughly 110 generic callers do not each repeat the `StopIteration` clear. `Send` is a separate operation. I found no change visible to Python. `yield from` over a core iterator still gets `None` (RETURN with `PyValue_NULL`), and over a generator still takes `SEND_GEN`. `gen_iternext` already ends a `None` return with NULL and no exception. Python code compares values, so `StopIteration.value` identity is preserved under R9 for every exact int and float. `next()` raises in the caller's exception context, as `gen_iternext` did after its frame popped. Upstream's `_PyIter_Send` already clears a non-generator's `StopIteration` with no RAISE event, so `SEND` emits none either way. `INSTRUMENTED_END_FOR`'s STOP_ITERATION path is untouched. `FOR_ITER` must test that RAISE is monitored before it creates `StopIteration(v)`. Add `list(agen.asend(None))`, whose RETURN carries a yielded value that consumers drop as upstream does, and the `StopIteration` object a RAISE callback receives | – |
| 13 | note | B1, §1.1 | **The argument for the `PyObject *` receivers holds.** Only exact ints and exact non-NaN floats become inline (value-api.md §1.3), so no subclass instance is inline. `object`'s methods (`__getattribute__`, `__setattr__`, `__delattr__`, `__hash__`, `__repr__`, `__str__`, comparisons, `__format__`, `__reduce_ex__`, `__sizeof__`, `__dir__`, `__class__`) include none of the M1b slots. An extension subtype's slot wrapper checks `self`'s type before it calls the slot. Each dispatcher must raise its `TypeError` before calling `PyValue_AsKnownObjectBorrow`, which R1's release check enforces | – |
| 14 | note | §1.2 | **Naming.** `_PyValue_Size` contradicts value-api.md's `_PyValue_Len` (§1.2 table, §2.5). §1.5 rule 1 gives `_PyValue_Size`; update value-api.md and make `PyObject_Length` a second thunk name. B5 is consistent with rule 1 | – |

## Answers to Q1–Q3 and A7

- **Q1.** Agreed: internal struct, and `value` becomes a home in W2's field
  batch. Until then the `next()` exit is a counted CORE box. See finding 11.
- **Q2.** Rejected. Use one two-word form from the slot through the
  dispatcher to the handler, and amend VA7 (finding 1). Measuring `FOR_ITER`
  first is not needed: the choice rests on G9 R-a and on there being one
  form, not on speed.
- **Q3.** Replace the alias with a pair-table unit in which the marks are off
  (finding 6).
- **A7.** Writing both fields is correct under the GIL. The step is
  incomplete without the second writer, the invariant row and one helper for
  every write after ready (finding 2).

## Estimate (item 9), from the proposal's counts

Unit rates are the proposal's: a dispatcher ported by hand takes 8 minutes,
an adapter 5, a semantic slot 6, and a mechanical unit 3 for a Sonnet or
Haiku worker. Workers run 16 at a time and end with one 30-minute cleanup.

| Part | M1b units | M1b (h) | M2 units | M2 (h) |
|---|---|---:|---|---:|
| Lead, by hand | 14 dispatchers (112 min); 7 adapters (35); `gen_send_value` and 6 generator slots (42); 4 R16 slots, `next`, `all`, `any` (42); `slot_tp_iternext` (6); M1b-d's 5 `slot_*` (30) | 4.45 | 9 dispatchers (72); generic pair, 4 functions (40); 4 adapters (20); getset `d_value` and record (20); 2 member functions (16); `type` (16); `module` (8); hook and finding 2 (15); `super`, 6 functions (36); `LookupSpecialMethod` (6); M2-d's 3 `slot_*` (18) | 4.45 |
| Finding 1, 2, 3 and 7 changes | 2 typedefs, 2 macros, 2 adapters, test types; wrapper reads | 0.5 | cell read, 2 tests | 0.2 |
| Workers | 7 macros, 8 wrappers, 8 table rows, about 40 thin forms, 9 iterator slots, `list_extend_iter`, 2 identity tests, about 25 test cases: 100 units | 5.0 | 4 macros, 6 wrappers, 12 identity tests, 9 thin forms, 7 getters, about 25 test cases: 63 units | 3.15 |
| Integration, both compilers, stats table | – | 1.0 | – | 1.0 |
| **Total** | | **10.95** | | **8.8** |

That is about **19.75 agent-hours**: the lead 11.6 (with integration) and
the workers 8.15. The proposal says 20. Wall clock per step: the worker part
takes 7 or 4 rounds of 3 minutes plus the cleanup, under 1 h. With the second
agent on diff b, the lead part is set by diff a: 177 minutes in M1b (the
dispatchers, the adapters and finding 1) and 132 minutes in M2. With 1 h of
integration, that is about **4 h for M1b and 3.25 h for M2**, against the
proposal's 4.5 h and 4 h, not counting the d diffs. Both fit the route's
T7–T12 window. Finding 10 removes the wait on A3a. The 7.75 h above the
route's 12 is W2 work moved forward (finding 9). W2 must fall by the same
amount.

## What holds

- **B1, B2.** The tag is tested once, in the dispatcher, and the 93 slot
  functions keep upstream's bodies. The V8 comparison is apt.
- **The R11/R16 contract table** maps each caller to upstream's behaviour,
  including `next()` passing ERROR through and the object thunk raising only
  for a non-NULL `v`.
- **Adapters per kind** box only the key, item or value, and only when it is
  inline, under SLOT_ADAPTER with a family class.
- **B8.** The `slot_*` functions keep their names as thunks, so the identity
  tests on `slot_tp_iternext` hold without edits.
- **A5.** The `superobject.obj` home with M4a's helpers. `super().m()` with a
  heap `self` boxes nothing at I4.
- **Q4, Q5, R3.** Boxes on error paths and at adapters are recorded, not
  gated, with the step that removes each.

## Review 2 (revision 2)

Reviewed: revision 2 of the proposal, written in place, against M3
revision 3 (accepted at `65aa8c5e24f`, §1.11 and D17) and the shared tree
at `f7d374b1f92`, where V2a-0 part 2 has landed. The coordinator's rulings
are taken as given: `_PyType_LookupValue` returns `_PyStatusValue` (C10),
and `_PyErr_SetKeyErrorValue` keeps its suffix, with its object form a thunk
until I3.

### Verdict

**Accept. Implementation can start.** All four must-fix findings are
resolved, and so are should-fix findings 5–10. Nothing new is must-fix. Two
should-fix findings and three notes remain; each is a text edit or a step of
under 15 minutes, and each can be folded in while M1b-a is written.

| Review 1 | Status in revision 2 |
|---|---|
| 1 One result form | Resolved. `_PyStatusValue` moves to `pycore_value.h`, with a static assert that `PYGEN_*` equals −1, 0 and 1. Both slot typedefs, the 10 three-state dispatchers and lookups, and `_PySuper_LookupValue` (`want_method` in) return it. I3 and I4 call them directly. `PySendResultPair` and `_PyIter_Send` go at I3, and the W1b script class covers the existing out-parameter functions. The only out-parameter left is `_PyValue_THUNK_EXIT_STATUS`'s `PyObject **`, which serves the object API's own signatures (`PyIter_Send`, `PyMapping_GetOptionalItem`). That is correct: the out-parameter is the compatibility shape, not a second core form |
| 2 Writes after ready | Resolved. `_PyType_WriteSlotPair` is used by both writers, `slot_tp_descr_get` writes `(NULL, 0)`, the hook's row joins the pair table, and both cases are tested. See note E for the lint's scope |
| 3 False "no box" claims | Resolved in substance. The 14 wrappers are written once, in exactly §1.11's `_Py_wrappervaluefunc` shape (borrowed `self`, the window, `kwnames`, `wrapped_value`). `wrap_nextvalue` raises for RETURN itself, which is §1.11's rule for three-state slots. Every "no box" row names M3a and the e diffs. **The citation is stale** (should-fix A) |
| 4 `PyIter_Check` | Resolved. `tp_iternext != NULL && tp_value.iternext != _PyValue_NextNotImplemented`. The pair table fills the value side for an extension that names `_PyObject_NextNotImplemented`, and `iter()` on extension iterators is tested |
| 5–14 | Resolved as stated in revision 2's table. §4's arithmetic checks: 11.15 + 9.2 = 20.35 agent-hours, lead 11.95 and workers 8.4. §2.6's counts check: 10 + 1 + 1 move and 10 stay, which makes 22 |

### Remaining findings

| # | Severity | Section | Problem | Change |
|---|---|---|---|---|
| A | should-fix | Base line, §1.3, Q7 | The text still cites "the M3 proposal, revision 1", "M3 §1.4" and "its review being open" (lines 11, 166, 475). M3 revision 3 is accepted and fixes the shape in §1.11 (D17). The shape quoted in §1.3 is identical to it | Cite M3 revision 3, §1.11 and D17. Close Q7: the shape is ruled. Keep B9's order, since M3's collision table has M3a add `_Py_wrappervaluefunc` first |
| B | should-fix | A8, §2.6, §3 | **`value_pairs.c` is incomplete.** (a) `Py_VALUE_COMPAT` is defined unconditionally in `Include/pyport.h:290–303`, so the unit cannot switch the marks off without `&& !defined(Py_VALUE_PAIR_TABLE)` there, and `pyport.h` is not in §3's file list. (b) Only `value_slot_pairs[]` (`typeobject.c:9962`) and the hook row can move. `slotdef_values[]` names static `slot_*` functions and wrappers, so "the pair tables move" must not include it. (c) A8 does not say what happens to the other named uses when W2 marks the pair. There are 196: `Objects/` 90, `Python/` 23, `Modules/` 83 | Add `pyport.h` to the file list. Move only `value_slot_pairs[]`. State the classes of named use for W2's script. **Static initialisers and spec slots** (`{Py_tp_getattro, PyObject_GenericGetAttr}`, `Python-ast.c:2225`) drop the field and inherit it from `object` under upstream's rule, or use D13's `_Py_value_getattro` id. **`PyBaseObject_Type` and `type_new_set_slots`** (`typeobject.c:5131`, `:5134`) read the address from an exported const in `value_pairs.c`. **Direct calls** (`unionobject.c:458`, `genericaliasobject.c:779`, `super_getattro` at `typeobject.c:14319`, `:14336`) become `_PyObject_GenericGetAttrValue`. **Identity tests** are already done in §2.6 |
| C | note | §2.3 | **`d_value` is correct.** It is appended after `d_getset`, so that field's offset is unchanged and even a stale reader works. Only the core allocates the struct (`tp_basicsize`), `PyGetSetDef`'s stride and abi3 are unchanged, and the record pointer is resolved once in `PyDescr_NewGetSet`. Two loose ends. `test_sys.py:1768` (`check(collections.defaultdict.default_factory, size('3PP'))`) gains a `P` and belongs in §3's files. The bump should follow M3's D16, "once per landing train", so that M2 and M3a, which both move `boilvalrc2` to `boilvalrc3`, do not bump twice or collide on `configure.ac`. Also make `getset_get` the generated `_DESCRGET` thunk of `getset_get_value` (C3), not a second body | – |
| D | note | §2.6 rule | The rule moves only tests that name "a pair-table object function", yet the table also moves tests of `_Py_type_getattro` and `_Py_module_getattro` (`object.c` 3, `specialize.c` 1). Neither is in the pair table; both become thunks in §2.4 | Restate the rule as "names an object function that is marked, or that becomes a thunk" |
| E | note | A7, §2.4 | **The lint's scope is not named.** "Outside `PyType_Ready`, `update_one_slot` and the helper" would flag the legitimate writes before ready: `type_new_set_slots` (`typeobject.c:5131`), `inherit_slots` (`:9517`, `:9521`) and `structseq.c:606`. `PyType_FromMetaclass` writes through an offset that a syntactic lint cannot see. In `Modules/`, writes before ready are normal, and D6's debug assertion is the only check after ready | Name the type-construction functions that the lint allows. Limit the lint to `Objects/` and `Python/`. Say that D6 covers `Modules/`. §5's list of interim A3 wrappers also omits `value_get_iter` (`_PyEval_GetIter`'s index out-parameter: I3 makes it return `_PyValuePair`) and `value_load_global` (I1 and W1b's `_PyDict_LoadGlobalValue` class) |

M1b-a and M2-a can start on `f7d374b1f92`. If A3 step 2 has not landed,
`_PyStatusValue` goes into `pycore_value.h` there, as §5 says. Findings A
and B(a) are needed before M2-a's first commit, B(c) before W2 marks the
attribute pair, and C–E before integration.
