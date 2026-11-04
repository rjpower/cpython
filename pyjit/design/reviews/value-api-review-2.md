# Review 2 of `value-api.md`

Reviewed 2026-11-03 at python-jit `662daee3550` (872 lines), against
[review 1](value-api-review-1.md) and the coordinator's decisions R1–R13.
The runtime tree is now at `fdd7fed3216`; the page pins `379e92cded9`. The
lines this review checked have not moved. Line references `L…` are to
`value-api.md` at `662daee3550`. Source references are relative to
`build/worktrees/pyvalue-rc-universal`.

## Verdict

**V1 can start now. V2a-0 can start after two short changes to the
specification (must-fix list below).**

All twelve items of review 1 are addressed: eight are closed and four are
partly closed. None of the partial items blocks V1 or V2a-0. The new material
is sound in its structure. It has one ordering gap in the inheritance rule
and one gap in V2a-0's exit criterion; both concern V2a-0. It also has six
smaller errors, which can be fixed in the packets that implement them.

The checkpoint recorded at 19:49 (`1f59c1ab480`) changes what the cost model
in §6.2 can claim. Boxing is too rare to explain the 4.7× gap to upstream, so
"kernels should beat upstream only after V3 and V5" (L599) has no support in
the data. This does not block anything, because every packet is gated on
correctness only.

## 1. Review 1 items

| Item | Verdict | Evidence | What remains |
|---|---|---|---|
| B1 object positions | **Closed** | R1 (L36); §1.2 L151–164: every Python operand of a generic operation is `PyValue`, and the operation raises the upstream `TypeError` from `PyValue_TYPE`; §1.3 L171 typed entry; L175 checked `AsKnownObject*` | Wording only: "every generic `PyObject_*` function" (L36) should say "every one with a Python-value operand"; `PyObject_Malloc` and `PyObject_GC_Track` are not thunks. L160 lists `call` among the `PyObject *` slot receivers, but §3 L445 gives `vectorcallvaluefunc` a `PyValue callable` |
| B2 storage and inheritance | **Closed, one clarification** | R2 (L37); §2.1 L271–314 inline `tp_value`, `_tp_iteritem` kept; §2.3 L364–387 | Case 2 of §2.3 holds only if the value pass runs after `type_ready_inherit_as_structs` (must-fix 1) |
| B3 variadic arguments | **Closed** | R4 (L39); §4 L515–516, L520 self-test | The matcher misses `PyValue *` (follow item F1). The disputed counts are settled in section 3 |
| B4 dispatchers | **Closed, two corrections** | R5 (L40); §2.5 L406–441, `NotImplemented` L231 | The `__index__` row L432 is wrong for `int` subclasses; the `*` row L426 leaves out `sequence_repeat`'s index check (F2) |
| I1 `update_one_slot` | **Closed** | §2.4 L389–404, pair table L348–358 | Keep the meaning of `d_wrapped` (F3) |
| I2 missing families | **Closed** | §7.7 L748–774, assigned to packets | Counts settled in section 3 |
| I3 calls | **Partly closed** | §3 L445–466 | No field holds the value vectorcall of a type object; dispatch omits the `Py_TPFLAGS_HAVE_VECTORCALL` test; `PyFunction_SetVectorcall` is not handled (section 2.4) |
| I4 rollout | **Partly closed** | R6 (L41); §9 L815–847 | R6 says "mechanisms first, then a sweep file by file", but V2a-2, V2b, V2c and V3a each "sweep and mark" their own family, so a function is still edited once per packet (section 2.5) |
| I5 round-trip identity | **Closed** | R9 (L44); §1.3 L184–199, with a pinning test | One more case is worth a line: a caller passes `x` into a thunk, and the adapter hands an extension slot a different box `x'` |
| I6 cost and packet effects | **Partly closed** | §6.2 L575–600 | The table predates the measured checkpoint and disagrees with it (section 2.7) |
| I7 enforcement | **Closed** | §4 L498–507 `#error`; L514 header inlines; L517 slot-field reads with `Py_VALUE_ADAPTER` | – |
| I8 `PyValue_Box` | **Partly closed** | R8 (L43); §1.3 L179–182 | The rationale is wrong: the 98 generated uses span every family, not only V2a's (F4) |

## 2. New material

### 2.1 Inline `tp_value` and the inheritance rule (§2.3)

The rule at L370 is: *"take the base's value slot exactly when its own value
slot is 0 and its object slot equals the base's object slot."* It is correct
for the five cases in the table, and it removes the double call of review
B2. One gap remains, and V2a-0 writes this code.

* **Order.** `type_ready_inherit` runs `inherit_slots` for each MRO base
  first. Only afterwards, at `Objects/typeobject.c:10978`, does it call
  `type_ready_inherit_as_structs`, which shares the base's `tp_as_number`
  pointer. A static subtype with `tp_as_number` NULL (case 2, L376)
  therefore has no `nb_*` object slots while `inherit_slots` runs. If the
  value rule runs inside `inherit_slots`, as L366–368 suggest ("keeps
  `COPYSLOT` … After the object side is settled"), case 2 gets 0. The
  subtype then takes the adapter, its chosen function becomes `int`'s thunk,
  and deduplication against `int` fails, which is review B2's double call
  again.

  This cannot be seen in V2a-0, because every value slot there is 0. It
  appears first in V2a-1, and then only as a speed loss and a double call of
  a pure core slot, so no behavioural test will catch it.

  **Fix (must-fix 1).** State that the value pass runs once, after
  `type_ready_inherit_as_structs`. State also which base it compares with
  under multiple inheritance. The simplest statement is an invariant: *a
  value slot is non-zero only beside the object function paired with it* (a
  core per-type thunk, a `slot_*_obj`, or a pair-table entry). With that
  invariant, the value slot is taken from any MRO base whose object slot is
  equal, and the order of bases does not matter. Debug builds assert the
  invariant in `PyType_Ready`. Add a `_testinternalcapi` check in V2a-1
  that compares the subtype's `tp_value.nb_add` with `PyLong_Type`'s for
  each of the three extension subtypes in the V2a-0 test list, because
  behaviour alone cannot tell the adapter from the value slot.
* **Size.** 52 fields of 8 bytes, 416 bytes per type (L313), checks out
  against the struct at L274–300. Heap types grow by the same amount. That
  is about 4 MB for a program with 10,000 classes, which is worth one line
  in §6.2. `test_sys`'s `sizeof` checks for type objects change in V2a-0.

### 2.2 The dispatcher table (§2.5)

The table ports the upstream functions faithfully, with two exceptions.

* **`__index__` (L432).** The row says "An exact int returns itself". In
  upstream `_PyNumber_Index` the test is `PyLong_Check`, so *any* int,
  `bool` and int subclasses included, returns itself without calling
  `__index__`. Following the row literally would call an int subclass's
  `__index__`. `Lib/test/test_index.py:61–67` (`operator.index(MyInt(7)) ==
  7` while `MyInt.__index__` returns 8) would catch this, so it does not
  block. Fix the row (F2).
* **`*` with a sequence (L426).** Upstream `sequence_repeat` first tests
  `_PyIndex_Check(n)`, raising "can't multiply sequence by non-int of type
  '%.200s'", and passes `PyExc_OverflowError` to `PyNumber_AsSsize_t`. The
  port's `_PyValue_AsSsize_t` call needs both (F2).

The compare row matches `do_richcompare` (`Objects/object.c:1048`), including
the `checked_reverse_op` rule. The truth, hash, ternary and in-place rows
match their upstream functions.

### 2.3 The `iternext` contract (R11, §1.4, §2.1)

The contract is correct for every consumer it names. `next()` matches
`builtin_next` (`Python/bltinmodule.c:1853–1867`). `PyIter_Send` matches
`Objects/abstract.c:3043–3054`. The core thunk's "raise `StopIteration(v)`
only for a non-NULL `v`" matches `gen_iternext`. Two cases would change
Python behaviour, and both belong to V2b.

* **Core iterators that wrap another iterator.** `map_next`, `filter_next`,
  `zip_next` and `enumerate` return NULL with the inner iterator's exception
  still pending (`Python/bltinmodule.c:1595` onwards, `:661` onwards). So
  upstream, `next(map(str, g()))`, where `g` is a generator that returns 5,
  raises `StopIteration` whose `.value` is 5. L320–323 say that RETURN comes
  only from "core iterators (exhaustion, `PyValue_NULL`)" and from
  generators. A wrapper written to that text turns the inner RETURN(5) into
  RETURN(NULL), and `next()` then raises a bare `StopIteration`.

  **Fix (F5).** A core iterator that delegates to another returns the inner
  RETURN unchanged, value included. Add the case to V2b's tests.
* **Monitoring.** Upstream's generic `FOR_ITER` calls `_PyEval_MonitorRaise`
  before it clears a pending `StopIteration`. A generator that returns 5
  raises such an exception inside `gen_iternext`, so `sys.monitoring` sees a
  RAISE event there. Under R11 that generator returns RETURN(5) and raises
  nothing, so the event disappears.

  **Fix (F5).** Generic consumers that receive RETURN with a non-NULL value
  emit the event upstream would have emitted, as `INSTRUMENTED_END_FOR`
  already does for `STOP_ITERATION`. Add a `test_monitoring` case to V2b.

### 2.4 Per-instance value vectorcall and `self` homes (§3)

The design follows PEP 590 and closes most of review I3. The offset rule
(L456–458) and the prepend (L459–466) match `_PyObject_VectorcallPrepend`.
Four points are open, all of them for V3a:

* **No field for a type's own value vectorcall.** Calling a type object
  constructs an instance. The type's metatype, `type`, has a value vectorcall
  offset, and that offset must point at a field of each type object, the
  value analogue of `tp_vectorcall`. Neither §2.1 nor L299–300 declares one.
  §7.3 counts 20 constructor vectorcalls that need it.
* **The flag test.** Dispatch (L453–455) uses "the instance's value
  vectorcall" with no flag test. Upstream `PyVectorcall_Function` returns
  NULL unless the type has `Py_TPFLAGS_HAVE_VECTORCALL`. A Python metaclass
  that overrides `__call__` loses that flag. Without the same test, `C()`
  would run `type`'s constructor and skip `M.__call__`.
* **`PyFunction_SetVectorcall`.** This public function replaces a
  function's object vectorcall. It must also clear the value pointer, so
  that dispatch falls through to the call adapter and the replacement runs.
* **D4.** A `self` stored borrowed in `args[-1]` puts a borrow in a frame
  slot for the duration of the call. Note that this is an exception to D4
  ("no home holds a borrow"), or store an owned copy.

### 2.5 Rollout split and criteria (§9)

The split is the one review 1 asked for. Entry criteria are explicit, every
packet names its tests, and V3a no longer waits for IR v2.

* **V2a-0's exit can pass without testing anything (must-fix 2).** V2a-0
  adds "the number, compare, truth, hash, index and repr dispatchers … with
  the adapter" (L819), but nothing in V2a-0 calls them. The interpreter
  moves to them in V2a-1 (L820), and the page does not say when the thunks
  move. If `PyNumber_*`, `PyObject_RichCompare*`, `PyObject_IsTrue`,
  `PyObject_Hash`, `_PyNumber_Index` and `PyObject_Repr`/`Str` keep their
  object implementations in V2a-0, the suite does not run the adapter.
  "Adapter box count recorded" then reads 0, and the mechanism lands
  untested.

  **Fix.** In V2a-0 those functions become thunks over the new dispatchers,
  and `value_ops.c`'s mixed-operand paths call the dispatchers too, so the
  whole suite goes through the adapter. Make the exit "adapter box count
  non-zero on the suite".
* **R6 and §9 disagree.** R6 says "mechanisms first, then a sweep file by
  file, then each family marked". In §9, V2a-2 sweeps the number families
  before the V2b, V2c and V3a mechanisms exist, and each later packet
  sweeps again. A function that adds numbers and then subscripts the result
  is therefore edited in V2a-2 and again in V2b. Either is a defensible
  choice. The text should state the one that was made. If double edits are
  accepted, R6 should say "per packet: mechanism, then that family's sweep
  file by file".
* **Sizes.** V3a at about 14 agent-hours carries the per-instance value
  vectorcall, three new homes (with their GC work), `METH_VALUE`, Clinic
  value emission and regeneration of 435 functions, 45 parser sites,
  monitoring, the call adapter and a 284-call sweep. It is the next
  candidate for a split.
* **The checkpoint has already happened.** The diagram (L798–799) and L858
  place the checkpoint after V1 and before V2a-0. It was recorded at 19:49
  (`1f59c1ab480`), with box sites taken from a histogram patch
  (`build/pyjit-link-wave1/checkpoint-box-callers.diff`) rather than V1's
  counters. Treat V2a-0's "Checkpoint" entry as met, and make V1's counters
  the permanent replacement for the patch.

### 2.6 The V4 classifier and rewriter

The class scheme M1, M2, M3 and J is the right shape, and the first-hour M/J
measurement is a good gate on V4's size. Points for V4's first hour:

* **Ownership.** The object setters steal (`PyList_SET_ITEM`,
  `PyTuple_SET_ITEM`, `PyList_SetItem`) or borrow (`PyList_Append`,
  `PyDict_SetItem`). Their value forms borrow or `Take`. An M1 rewrite
  between a stealing and a borrowing form must add or drop a release. The
  rewriter needs an ownership table for each function, and a site whose
  ownership changes is not mechanical unless that table covers it.
* **M2.** A local can only change type if every assignment to it changes
  too. `return local` from a `PyObject *` function, `Py_SETREF` and
  `Py_CLEAR` uses, and `== NULL` tests all need rules. Without a rule, each
  is a J site.
* **AST size.** A full `-ast-dump=json` of a CPython unit that includes
  `Python.h` runs to hundreds of MB. Use `-Xclang -ast-dump-filter=<function>`,
  or dump each unit once and stream it.
* **Compile database.** `intercept-build` is installed in
  `/usr/lib/llvm-21/bin`. Name it in V1, which builds the database (L817).

### 2.7 Cost model and the packet-to-kernel table (§6.2)

The cost rows (L577–585) are accurate. The packet-to-kernel table (L590–597)
is an estimate, and it predates the measured checkpoint
(`pyjit/bench/results/checkpoint-20261103.md`). The measurements disagree
with it in three places.

* **Boxing does not explain the gap.** The checkpoint finds C 4.7 times
  slower than upstream. On object-heavy kernels a word costs 75–125 ns,
  against 3.4–7.5 ns per upstream instruction, and no Python-heavy kernel
  makes more than 0.11 boxes per word (checkpoint lines 9–25 and 275–305).
  Removing every box cannot close a 12–25× gap in the cost per dispatch, so
  L599 "Kernels should beat upstream only after V3 and V5" should go. The
  value API removes boxing; the gap to upstream is per-word cost, which is
  outside this page.
* **The table should follow the measured box sites.** The checkpoint's top
  20 sites (94.6% of boxes per pass) map to packets as follows:

  | Packet | Measured sites | Share | Main kernels |
  |---|---|---:|---|
  | V3a/V3b | builtin, method-descriptor and class calls through `_PyBorrow_ValueVector` and `_PyNativeScope_Objects`; `list.pop` | 35.9% | fannkuch, float, raytrace, chaos, go, regex_compile |
  | V5 | `BUILD_SLICE` boxing `start`/`stop` (two sites) | 24.1% | fannkuch |
  | V2a-1 | `value_binary_operation`, `COMPARE_OP` | 14.4% | raytrace, nbody, chaos |
  | V2b | `STORE_SUBSCR`, `listiter_next`, `INTERPRETER_EXIT` (generator results) | 12.9% | regex_compile, spectral_norm, nqueens, generators |
  | V4 | `SET_ADD`, `PyList_GetItemRef`, `_PyDict_NextEntryRefKnownHash`, `MAP_ADD` | 7.3% | nqueens, regex_compile, json_dumps, comprehensions |
* **Rows that disagree.**
  * Slices are a quarter of all boxes, yet §7.4 rates them "warm".
  * `spectral_norm` belongs under V2b (`listiter_next`), not V2a.
  * The V2c row has no measured site at all.
  * `go`, `deltablue` and `richards` make 0.004–0.012 boxes per word, so no
    packet will move them measurably.

  Replace the table with the measured mapping, and re-derive it from V1's
  counters at each packet.

## 3. The disputed counts

Both scripts are recorded in `build/pyjit-link-wave1/`. They read `.c` and `.h`
files in `Objects/`, `Python/` and `Parser/`, skip `generated_*`,
`Python-ast.c` and `*/clinic/*`, strip comments, and count calls rather than
lines. A column-0 name, a `#define` line and a `PyAPI_FUNC` or `extern` line
do not count as calls. I ran them at `fdd7fed3216`.

**Format calls.** Method:
`python3 -I build/pyjit-link-wave1/valueapi-r2-formats.py SRC`. It covers 13
variadic formatting functions (`PyErr_Format`, `PyUnicode_FromFormat`,
`_PyErr_Format`, `PyErr_FormatUnraisable`, `PyErr_WarnFormat` and eight
more), each with its format-argument index. It joins adjacent string
literals and parses the conversions.

| Population | Count |
|---|---:|
| All 13 functions, `.c` and `.h` | 1136 |
| The four main functions (`PyErr_Format`, `PyUnicode_FromFormat`, `_PyErr_Format`, `PyErr_FormatUnraisable`), `.c` only | 1027 |
| Calls with a conversion that needs a value form (`%R %S %A %T %N`, any flags) | 267 |
| Calls with any object conversion (adds `%U %V`) | 387 |
| `Python-ast.c` (generated, compiled in core): calls, and calls with `%R`-class conversions | 107, 103 |
| `Modules/`: calls, and calls with `%R`-class conversions | 1025, 238 |

* **Calls.** The drafter's 1025 is the four main functions in `.c` files
  (1027 here; the difference is two calls). My 1129 in review 1 was a line
  count with `rg`. It included `Python-ast.c` and `PyErr_WarnFormat`, and
  it counted declarations and comment lines. Use **1136 calls in 13
  functions**, or 1027 for the four main ones.
* **Object conversions.** Neither 444 nor 351 measures the population the
  page names. My 444 came from a multiline `rg` pattern that ran past the
  format literal and included `%U`, `%V` and `Python-ast.c`. The drafter's
  351 is closest to "any object conversion, four functions, `.c`" (358
  here). For L516 and §7.7, the numbers are **267** calls that need a value
  conversion, **387** that take any object, and 107/103 more in
  `Python-ast.c`, whose generator (`Parser/asdl_c.py`) is fixed with the
  family. None of this changes the design, because the `clang-query` check
  covers every variadic call whatever the count.

**`PyLong_From*`.** Method:
`python3 -I build/pyjit-link-wave1/valueapi-r2-calls.py SRC 'PyLong_From\w*'`.

| Population | Count |
|---|---:|
| Every `PyLong_From*` call (`.c` 284, `.h` 4) | **288** |
| Of these, the C-integer constructors (`FromLong` 147, `FromSsize_t` 73, `FromUnsignedLong` 14, `FromSize_t` 9, `FromLongLong` 7, `FromUnsignedLongLong` 6, `FromInt64` 3, `FromInt32`, `FromUInt32` and `FromUInt64` 1 each) | 262 |
| Other constructors (`FromVoidPtr` 14, `FromDouble` 6, `FromString` 4, `FromUnicodeObject` 2) | 26 |
| `PyFloat_FromDouble` | 51 |

The drafter's 286 (L759) is a line count of `.c` files without comment
stripping, and is right to within 2. My 271 was a line count of the 10
integer constructors in `.c` and `.h` files, with comments included. Use
**288** for the family, since every constructor returns a value that can be
inline. The per-name figures at L759, `FromLong` 157 and `FromSsize_t` 85,
do not reproduce: they are 147 and 73. `PyFloat_FromDouble` 51 agrees.

## 4. Can V1 and V2a-0 start?

**V1: yes.** Nothing in its content (L817) is wrong. F1 should go into V1's
check, because V1 is where the check is built. Without F1 the check has a
hole, but V1 can still start.

**V2a-0: yes, after the must-fix list.**

### Must fix before V2a-0

1. **§2.3, where and against what the value rule runs.** State that the
   value pass runs once, after `type_ready_inherit_as_structs`. State it as
   the invariant "a value slot is non-zero only beside its paired object
   function", so that under multiple inheritance any MRO base with an equal
   object slot supplies it. Debug-assert the invariant in `PyType_Ready`.
   Without this, case 2 of L376 silently gets 0, and no V2a-0 test can tell
   (section 2.1).
2. **V2a-0, who calls the new dispatchers.** State that in V2a-0 the number,
   compare, truth, hash, index and repr thunks and `value_ops.c`'s
   mixed-operand paths go through the new dispatchers. Make the exit
   "adapter box count non-zero on the suite". Otherwise the mechanism lands
   without being exercised (section 2.5).

### Follow during implementation

| # | Item | Packet |
|---|---|---|
| F1 | Extend the variadic matcher to `PyValue *`: `&v` passed where `PyObject **` is expected (`PyArg_ParseTuple`, `PyArg_UnpackTuple`) writes a pointer into a value without a diagnostic. Use `hasType(anyOf(asString("PyValue"), pointsTo(asString("PyValue"))))` and run it on the desugared type | V1 |
| F2 | §2.5 fixes: the `__index__` row says any int (`PyLong_Check`) returns itself; `*` with a sequence keeps the `_PyIndex_Check` test, its message and `PyExc_OverflowError` | V2a-0 (`test_index` catches the first) |
| F3 | Keep `d_wrapped` meaning "the object function", as today (`Objects/descrobject.c:551`, `:563`, `:1092`), and add `d_wrapped_value`. Inverting its meaning (L394) breaks any external reader of a public struct field | V2a-0 |
| F4 | `PyValue_Box`'s 98 generated uses cover super attributes, `MATCH_CLASS`, dict and set merges, `SEND`, imports, formatting, `CONTAINS_OP`, exception matching, intrinsics, templates and unpacking, not only V2a's families. To delete it in V2a-1, the generator must emit an owned object local (`PyValue_AsObject`) for the rest | V2a-1 |
| F5 | R11: wrapping core iterators (`map`, `filter`, `zip`, `enumerate`) pass the inner RETURN unchanged, value included; generic consumers emit the monitoring event that upstream's `StopIteration` produced. Tests: `next(map(str, g()))` with `g` returning 5 gives `.value == 5`; a `test_monitoring` case for `for` over a generator that returns a value | V2b |
| F6 | §3: a value-vectorcall field for type objects (the analogue of `tp_vectorcall`); dispatch tests `Py_TPFLAGS_HAVE_VECTORCALL`; `PyFunction_SetVectorcall` clears the value pointer; the borrowed `self` in `args[-1]` is noted against D4 | V3a |
| F7 | Make R6 and §9 agree on the sweep order (section 2.5) | text |
| F8 | Correct the checkpoint ordering (L798–799, L858). Replace the §6.2 kernel table with the measured site mapping and drop L599. Rate slices "hot" in §7.4 | text |
| F9 | V4 rewriter: an ownership table for each function, M2's assignment rules, filtered AST dumps (section 2.6) | V4 first hour |
| F10 | Counts in §4 L516 and §7.7 L759, L765: use section 3's figures and methods | text |
| F11 | Wording: R1's "every generic function" means one with a Python-value operand; L160 versus L445 on the `call` receiver; §6.2 notes 416 bytes per heap type; `test_sys` `sizeof` checks change in V2a-0 | text, V2a-0 |
