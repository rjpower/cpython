# Value API: a value-native core with an object compatibility layer

Status: revision 3, 2026-11-03, after [review 1](reviews/value-api-review-1.md),
[review 2](reviews/value-api-review-2.md) and the coordinator's decisions on
them (R1–R13, R14–R25 below). It specifies the user's
decision that `PyValue` is the internal API and `PyObject *` is the
compatibility API, and it is the source for the V packets in section 9.
Accepted for implementation of V1 and V2a-0 (review 2's two must-fix items
are R14 and R15); later packets take review 2's per-packet notes.

Source tree: `build/worktrees/pyvalue-rc-universal` in the boil worktree
(`build/` is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`),
HEAD `379e92cded9`, recounted at 19:42–20:00 with ten files carrying other
agents' uncommitted edits. Line numbers in `Objects/typeobject.c` move with
those edits, so this page names its functions instead. File references are
relative to that tree. The counting scripts are `build/pyjit-link-wave1/valueapi-*.py`.

Related pages: [PyValue representation](pyvalue.md) (encoding, D1–D8),
[native boundary](native-boundary.md) (borrow stack), [extension API](extension-api.md)
(HPy direction), [representation packet](proposals/pyvalue-representation-packet.md),
[register IR](register-ir.md) (IR v2, IR-D3).

## Decisions

| # | Decision | Status |
|---|---|---|
| VA1 | Core code uses the value form. The `PyObject *` API keeps its names as thin thunks over the value form. Core code never calls a thunk. An object becomes a value once, at the API entry; a value becomes an object only when it leaves to an object consumer | Accepted. The cost of the sweep is stated in C8 |
| VA2 | Polymorphic positions are `PyValue`; typed positions keep a pointer type | Accepted as R1: generic means `PyValue`, concrete means a typed pointer (§1.2). C1 withdrawn |
| VA3 | One canonicalising borrow and one canonicalising steal convert `PyObject *` to `PyValue` | Accepted (§1.3), plus the typed entry of R3. Exact non-NaN floats, not every float, become inline |
| VA4 | Owned boxing (`PyValue_AsObject`) and borrowed boxing through the borrow stack. Small ints box to the cached immortals | Accepted. Holds as built (`_PyValue_BoxInlineValue` calls `PyLong_FromLong`). Slot adapters box into C locals, not the stack (C4) |
| VA5 | A value slot table on every type; `PyType_Ready` fills the missing side; pairs inherit together; `slot_*` functions are native value slots | Accepted with changes: value slots are inline fields (R2); `len` and `iter` need no value slot, `repr`, `str` and `send` do (C2); the extension adapter lives in the dispatcher, and core object thunks are generated per type (C3, §2.2) |
| VA6 | Value vectorcall, a value builtin convention, value-form Clinic output; object-form natives are reached through the borrow stack | Accepted, with a per-instance value vectorcall (R7, §3) |
| VA7 | Arguments borrowed, results owned. Error: `PyValue_NULL`. Absence: three-state `int` with an out parameter | Accepted. `PyValue_NULL` chosen; `PyValue_ERROR` has two producers and is deleted (§1.4). Amended by [value-api-m1b-m2.md](proposals/value-api-m1b-m2.md) (B4): a three-outcome result is `_PyStatusValue {int status; PyValue value;}` returned by value, with no out-parameter for a result; pointers remain for arrays and in-out state |
| VA8 | `_PyValue_<Op>` for generic operations; `Py<Type>_<Op>Value` for containers | Accepted with the rule stated exactly and 14 renames (§1.5). The suffix collides with the dict sense of "value" (C9) |
| VA9 | Object functions with a value form are marked compatibility-only; a call from a value-native unit is a build error | Accepted with `__attribute__((unavailable))` and no fallback (C7, R12), plus the variadic check of R4 (§4) |
| VA10 | `Objects/`, `Python/`, `Parser/` are value-native at the end; hot `Modules/` are ported by box count; polymorphic fields become `PyValue` | Accepted with a corrected field list (C5) and `Parser/` scheduled last (C6) |
| VA11 | Extensions keep the object API; a public value API is out of scope | Accepted. No `PyValue` in public headers (R10); constraints in §5.2 |
| VA12 | Box counters by site class in the stats build; recorded, not a gate | Accepted (§6). The checkpoint used a histogram patch; V1's counters replace it (R24) |
| R1 | Review B1. Every `_PyValue_<Op>` takes `PyValue` for every Python operand, including callables and the receivers of `len`, `iter`, `getitem`, `setitem`, `contains` and `setattr`. Every generic `PyObject_*`, `PyNumber_*`, `PySequence_*`, `PyMapping_*` and `PyIter_*` function with a Python-value operand becomes a marked thunk (`PyObject_Malloc` and `PyObject_GC_Track` do not). `PyValue_AsPyObjectBorrow`/`Steal` become `PyValue_AsKnownObjectBorrow`/`Steal`, checked in release builds through V7 | Coordinator, 2026-11-03, after review 1. §1.2, §1.3 |
| R2 | Review B2. Value slots are fields at the end of `PyTypeObject`; there is no `tp_as_value` pointer; each pair has its own inheritance rule, `update_one_slot` included | Coordinator, 2026-11-03, after review 1. §2.1, §2.3, §2.4. Open question 6 closed |
| R3 | The typed entry `_PyValue_FromObjectKnown`, debug-checked, for pointers that cannot be exact ints or floats | Coordinator, 2026-11-03, after review 1. §1.3. Open question 1 closed |
| R4 | Review B3. A clang-query matcher in `Tools/build/check_value_api.py`, run by a make target from V1, fails when a `PyValue` reaches a variadic argument in a core unit. Each object format conversion gets a value conversion; V1 picks the spelling | Coordinator, 2026-11-03, after review 1. §4 |
| R5 | Review B4. The dispatchers are specified completely: reflected compare, sequence fallbacks, ternary dedup, truth fallback, result checks, `NotImplemented` | Coordinator, 2026-11-03, after review 1. §2.5 |
| R6 | Rollout: mechanisms first, then a sweep file by file, then each family marked once swept. V2a splits into V2a-0 (mechanism), V2a-1 (core types), V2a-2 (remaining types and sweep). V3 splits; V3a (C level) does not wait for IR v2. `PyObject_IsTrue` and `_PyNumber_Index` are marked after Clinic value emission. Every packet names its tests. V4 uses a Clang classifier and rewriter | Coordinator, 2026-11-03, after review 1. §9; the sweep moved to VS by R21 |
| R7 | Calls: core callables get a per-instance value vectorcall; bound-method `im_self` and builtin `m_self` are `PyValue` homes; a bound call prepends `self` by the offset rule | Coordinator, 2026-11-03, after review 1. §3 |
| R8 | `PyValue_Box` is deleted in V2a; D2 removes the non-canonical form | Coordinator, 2026-11-03, after review 1. §1.3 |
| R9 | Round-trip identity is permanent compatibility behaviour | Coordinator, 2026-11-03, after review 1. §1.3 |
| R10 | `pyvalue.h` and `PyTuple_GET_VALUE` move to `Include/internal` in V1. External code gets no `PyValue` until a public value API is designed | Coordinator, 2026-11-03, after review 1. §5.2; pyvalue.md D6. Open question 2 closed |
| R11 | The value `iternext` uses the `am_send` contract, returned as `_PyStatusValue` (value-api-m1b-m2.md B4). RETURN is only for iterators that end without raising (core iterators, generators), with the return value or `PyValue_NULL` in `value`. ERROR covers every raised exception, `StopIteration` and subclasses included, left as raised. The adapters for an extension `tp_iternext` and for `slot_tp_iternext` map NULL with no exception to RETURN with `PyValue_NULL` and NULL with any exception to ERROR. Generic consumers treat ERROR with `StopIteration` as the end and clear it; `next()` passes ERROR through and raises a fresh `StopIteration(v)` only on RETURN; `_PyValue_Send`/`PyIter_Send` turn ERROR with `StopIteration` into RETURN with its value. A core type's object thunk raises `StopIteration(v)` only for a non-NULL `v`. Python behaviour is unchanged. Open question 5 closed | Coordinator, 2026-11-03, after review 1 (replacing revision 2's pending-exception end; refined so that `next()` keeps upstream behaviour). §1.4, §2.1, §2.5, V2b |
| R12 | Review I1–I8 enter the specification and the rollout | Coordinator, 2026-11-03, after review 1. §2.4 (I1), §7.7 (I2), §3 (I3), §9 (I4), §1.3 (I5), §6.2 (I6), §4 (I7), §1.3 (I8) |
| R13 | The inventory is recounted at `379e92cded9` | Coordinator, 2026-11-03, after review 1. §7 |
| R14 | Review 2 must-fix 1. The value pass runs once, after `type_ready_inherit_as_structs`, under an invariant that `PyType_Ready` asserts in debug builds: a value slot is non-zero only beside its paired object function. Any MRO base with an equal object slot supplies the value slot | Coordinator, 2026-11-03, after review 2. §2.3 |
| R15 | Review 2 must-fix 2. In V2a-0 the `PyNumber_*`, `PyObject_RichCompare*`, `PyObject_IsTrue`, `PyObject_Hash`, `_PyNumber_Index` and `PyObject_Repr`/`Str` thunks and `value_ops.c`'s mixed-operand paths go through the new dispatchers. Exit: adapter box count non-zero on the suite | Coordinator, 2026-11-03, after review 2. §9 V2a-0 |
| R16 | R11 additions: `map`, `filter`, `zip` and `enumerate` pass an inner RETURN value through; generic `FOR_ITER` keeps upstream's `sys.monitoring` RAISE event for a generator that returns a value; a test for each | Coordinator, 2026-11-03, after review 2. §2.1, §9 V2b |
| R17 | Dispatcher corrections: `__index__` returns any int unchanged, subclasses included; sequence `*` keeps its index check and "can't multiply sequence by non-int" | Coordinator, 2026-11-03, after review 2. §2.5 |
| R18 | The variadic matcher also catches a `PyValue *` passed where a `PyObject **` is expected | Coordinator, 2026-11-03, after review 2. §4 |
| R19 | `d_wrapped` keeps its meaning; `d_wrapped_value` is the new value field | Coordinator, 2026-11-03, after review 2. §2.4 |
| R20 | Calls: `tp_value_vectorcall` holds a type object's own value vectorcall; dispatch keeps the `Py_TPFLAGS_HAVE_VECTORCALL` test; `PyFunction_SetVectorcall` clears the value vectorcall | Coordinator, 2026-11-03, after review 2. §2.1, §3 |
| R21 | Sweep order: mechanism packets sweep no callers; one sweep packet, VS, edits each file once for every family whose mechanism has landed and marks families as their last caller goes. Recommended over per-family sweeps, which revisit files | Coordinator, 2026-11-03, after review 2. §9 |
| R22 | `PyValue_Box`: the cases generator emits an owned `PyValue_AsObject` local for families not yet converted, so `PyValue_Box` is deleted in V2a-1. Under A5b (in implementation) that local is a `PyObject *x_o` owned by `value_temps` | Coordinator, 2026-11-03, after review 2; A5b note 2026-11-04. §1.3, §9 V2a-1 |
| R23 | The V4 sweep tool has an ownership table per function, rules for every assignment to a retyped local, and per-function AST dumps | Coordinator, 2026-11-03, after review 2. §9 |
| R24 | Cost model: no claim about when kernels reach upstream; the measured box-site split by packet; the checkpoint gap is a per-word cost that the value API does not address; the checkpoint precedes V1 | Coordinator, 2026-11-03, after review 2. §6.2, §9 |
| R25 | Counts from review 2's scripts (`build/pyjit-link-wave1/valueapi-r2-*.py`): 267 format calls need a value form; `PyLong_From*` 288, `FromLong` 147, `FromSsize_t` 73 | Coordinator, 2026-11-03, after review 2. §4, §7.7 |
| R26 | Representation boundary: only the value headers, `pyvalue.c`, the JIT, the out-of-process readers and the encoding tests know the word's encoding. Public headers carry it as an opaque word: no inline decoding, five frozen public fields, exported operations. No encoding in an extension ABI. Enforced by `raw-bits` and `public-value` (A5b-3) | User direction, 2026-11-04. §1.6 |
| R27 | No box inside the core. A box is made only at the compatibility boundary: an object-API call into the core from outside it (a thunk), and the two adapters, which call object slots of extension types and object-form natives. A heap int beyond int51 or any other object is an allocation, not a box. Every in-tree module is core (V6 covers all of `Modules/`, not only accelerators). Gate at V7: over the full suite and the kernels, the box counters record zero boxes at core sites; boxes appear only in the thunk and adapter classes, and only for extension types and object-form natives | User direction, 2026-11-04. §1.1, §6, §9 |

## Challenges to the brief

**C1 (VA2). Withdrawn after review 1.** It made callables and the receivers
of `len`, `iter` and subscripting object positions. That gave each operation
two spellings and needed `PyValue_AsPyObjectBorrow` at every such call, which
yields a garbage pointer for an inline value in release builds (review B1).
R1 replaces it.

**C2 (VA5). The slot list.** `sq_length`/`mp_length` return `Py_ssize_t`;
`tp_iter` returns an iterator, which is never inline (`__iter__` returning `5`
raises). Neither needs a value slot: their receivers cannot be inline once the
operation has tested the tag (§2.5). `tp_repr` and `tp_str` do (an int receiver
can be inline), as do `am_send` (argument and result are polymorphic) and
`nb_index`/`nb_int`/`nb_float` (results are ints or floats).

**C3 (VA5). Extension adapters cannot be derived function pointers.** C has no
closures, so a shared adapter function cannot know which type's object slot it
stands for. Binary dispatch makes this a correctness bug, not a cost:
`binary_op1` drops the right operand's slot when it equals the left one's
(`Objects/abstract.c:958`). Two extension types with the same derived adapter
would look like one type, and the right operand's `__radd__` would never run.
The adapter therefore lives in the dispatcher: a NULL value slot means "box the
operands and call the object slot" (§2.5). In the other direction, core types
need a per-type object thunk for each value slot, because extensions call
`PyBaseObject_Type.tp_repr(self)` or `PyLong_Type.tp_richcompare(a, b, op)`
with subclass instances. That is about 425 thunks (§7.3), generated by a macro.

**C4 (VA4, VA5). Slot adapters box into C locals.** A slot has at most three
operands; owned C locals are cheaper than a push and a scope, and safe under
the D2 release contract. The borrow stack stays the mechanism for
variable-length vectors (the call adapter, P4) and parser outputs
([native-boundary.md](native-boundary.md)).

**C5 (VA10). The field list.** Range iterators hold C `long`s
(`Include/internal/pycore_range.h:11`) and `enumerate` a `Py_ssize_t`; their
overflow forms hold ints beyond the inline range. None needs a home; §7.4
lists the fields that do.

**C6 (VA10). `Parser/` and the compiler gain nothing at run time.** AST
constants are `PyObject *` (`Include/internal/pycore_asdl.h:16`) and
compile-time folding calls `PyNumber_*` (15 calls in `Python/flowgraph.c`).
They are ported only for the invariant "no core unit calls a thunk", which
an exemption would turn into an allowlist; they come last (V7).

**C7 (VA9). Use `unavailable`, not `deprecated` plus `-Werror`.**
`__attribute__((unavailable("…")))` is an error at the use site without a flag,
in Clang and GCC 12 or later. `deprecated` depends on every build passing
`-Werror=deprecated-declarations`, and `_Py_COMP_DIAG_IGNORE_DEPR_DECLS`
silences it (7 core sites use that pragma), so it is not a ratchet. A compiler
without `unavailable` gets `#error` in core builds, not a weaker fallback (R12,
review I7). Naming a function in a static initialiser is also a use: 77 core
static types name `PyObject_GenericGetAttr`. Core type definitions therefore
name only value forms, and `PyType_Ready` fills the object side of generic
pairs from a fixed table (§2.2).

**C8 (VA1). The sweep is the cost.** Core code makes 2173 handwritten calls to
object-form polymorphic APIs, plus 53 in Clinic headers (§7.1), and the
families of §7.7 add number construction, extraction, error payloads and
argument parsing. Under R1 every argument of a generic call converts; the
conversion disappears when the caller's local becomes a `PyValue`. Each family
is swept completely before it is marked (VA9).

**C9 (VA8). "Value" is also a dict word.** `PyDict_ContainsValue(d, key)`
tests a key; `PyDict_NextValue` yields a key and a value. The suffix rule
matches about 40 existing names and is kept for internal names. The public
spelling is decided with the public value API (VA11).

**C10 (VA7). Today a NULL `PyValue` result has four meanings:** error
(`PyTuple_GetValue`), empty cell or error (`PyCell_GetValue`), exhaustion or
error (`_PyDictIter_NextValue`), out of range with no exception
(`_PyList_GetValue`). §1.4 makes NULL mean error only. The three-outcome results return `_PyStatusValue` by value (value-api-m1b-m2.md B4).

**C11 (VA11). Boxing at the compatibility boundary is permanent.**
JavaScriptCore passes value bits through its C API with no boxing because
`JSValueRef` is opaque (§8). `PyObject *` is not opaque: `Py_TYPE`,
`Py_INCREF` and the object macros dereference it. The only way to stop boxing
for extensions is an opaque handle API, the HPy direction of
[extension-api.md](extension-api.md).

## 1. The model

### 1.1 Two forms and one boundary

```mermaid
flowchart LR
    X["Extensions and compatibility callers<br/>(PyObject *)"] -->|"entry: FromObjectBorrow / Steal / Known"| T["Thunks with the existing names"]
    T --> V["Value core: _PyValue_Op, Py&lt;Type&gt;_OpValue,<br/>value slots, value vectorcall"]
    V -->|"exit: PyValue_AsObject"| T
    I["Interpreter and JIT"] --> V
    V -->|"slot adapter (NULL value slot):<br/>box into locals, call object slot"| E["Object slots of extension types"]
    V -->|"call adapter: borrow-stack vector"| N["Object-form natives"]
```

Value-native units are those with `Py_VALUE_CORE` (§4). A thunk converts
its Python operands with an entry primitive, calls the value form and boxes a
polymorphic result with the exit primitive; it calls no other thunk. The
borrowed getters deleted in the link wave stay deleted. The two adapters are
the only places where the core calls object-form code it does not own.

**No box inside the core (R27).** The thunks and the two adapters are the
only places where a value becomes a box. Core code, including every in-tree
module, passes values to value forms. A heap int beyond int51, or any other
object, is allocated as an object when it is created, which is not boxing.
Until V7, boxes at core sites are migration debt. The box counters (§6)
measure that debt per site class at each step. V7 gates on zero.

### 1.2 Positions

**Generic means `PyValue`, concrete means a typed pointer.**

| Position | Type | Example |
|---|---|---|
| Every Python operand of a generic operation: operands, receivers, keys, items, attribute values, callables, call arguments, both arguments of `isinstance` | `PyValue` | `_PyValue_Add(PyValue, PyValue)`, `_PyValue_Size(PyValue)`, `_PyValue_GetItem(PyValue, PyValue)`, `_PyValue_SetAttr(PyValue, PyObject *name, PyValue)`, `_PyValue_Vectorcall(PyValue, const PyValue *, size_t, PyObject *)` |
| A result that can be any object | `PyValue` | `_PyValue_GetAttr(…) -> PyValue` |
| A result of a known type | typed pointer | `_PyValue_Repr(PyValue) -> PyObject *` (a str), `_PyValue_GetIter(PyValue) -> PyObject *` |
| Receiver of a concrete-type API, after the caller's type check | typed pointer | `PyDict_SetValue(PyObject *dict, PyValue key, PyValue value)` |
| Names, kwnames, `PyTypeObject *` arguments, code, frames, modules | typed pointer | unchanged |
| Receiver of a value slot that `int` and `float` lack (`subscript`, `item`, `ass_*`, `contains`, `iternext`, `send`, `call`, the value vectorcall) | `PyObject *` | the operation tests the tag before it reads the slot (§2.5, §3) |

An inline value in a generic position never needs a box. The operation reads
its type with `PyValue_TYPE` and raises the upstream `TypeError` itself
(`len(5)`, `5()`, `iter(2.5)`, `5[0]`).

### 1.3 Conversions

| Direction | Primitive | Contract | Today |
|---|---|---|---|
| Entry, borrowed | `PyValue_FromObjectBorrow(o)` | Canonical: an exact int in the inline range or an exact non-NaN float becomes inline; otherwise a no-count reference. NULL gives `PyValue_NULL`. The pointer case is inline in `pycore_value.h` (type load, two compares, tag); only int and float call out | Out of line (`Python/pyvalue.c:9`), calling `_PyObject_ValueIdentity` (`:227`). `PyValue_FromPyObjectBorrow` (raw, 9 hand-written and 2 generated uses) and `_PyValue_FromObjectBorrowInline` merge into it. **A5b, in implementation:** it is inline in the core and exported for modules, returns a `PyValueBorrow`, and tags only an immortal object. Its result becomes an owner only through `DUP`/`XDUP` ([proposal](proposals/register-ir-v2-a5b.md) §2.4) |
| Entry, typed | `_PyValue_FromObjectKnown(o)` | For a pointer that cannot be an exact int or float (a str, type, function, iterator, container). Tags it with no type load. Debug builds assert the precondition. A5b: returns a `PyValueBorrow` and tags only an immortal object | New in V1 (R3) |
| Entry, owned | `PyValue_FromObjectSteal(o)` | Canonical; consumes `o`; frees an int or float box at once (D2 release contract) | `PyValue_FromPyObjectSteal` (161 uses) canonicalises since representation step 8; renamed in A5b |
| Exit, owned | `PyValue_AsObject(v)` | New reference; boxes an inline value; NULL with MemoryError. In the core it does not test `PyErr_Occurred` or `IsLanguageValue` (pyvalue.md G4 item 3). `IsLanguageValue` stays as a debug assert. A pending exception is allowed, as for any upstream API that allocates: a failed box replaces it (revised 2026-11-04, representation step 7a) | Done in V1 (`48bb4d6d1d3`): `PyValue_AsPyObjectNewBoxed` merged into it |
| Exit, borrowed (call adapter) | `_PyBorrow_ValueVector`, `_PyBorrow_Keep` | Valid until the scope exits ([native-boundary.md](native-boundary.md) §2) | Exist |
| Value known to hold an object | `PyValue_AsKnownObjectBorrow(v)`, `PyValue_AsKnownObjectSteal(v)` | Masks the tag. Precondition: `v` holds an object or is NULL. Release builds test it during V1–V7 and call a cold `_PyValue_FatalNotObject` on an inline value. V7 measures the test and decides whether it stays | `PyValue_AsPyObjectBorrow`/`Steal`, 892 uses including generated code; renamed in V1 (R1) |

`PyValue_FromPyObjectNew` (53 uses: 35 hand-written and 18 generated, plus 2 of
`_PyValue_FromPyObjectNew`) is deleted:
`PyValue_DUP(PyValue_FromObjectBorrow(o))` costs the same. (A5b renames
`NewRef` to `XDUP`; see §1.5.)
`PyValue_Box` (98 uses in `generated_ir_cases.c.h`, 5 in
`generated_jit_helpers.c.h`, 2 by hand) is deleted in V2a-1 (R8, R22). Its
generated uses span every family: super attributes, `MATCH_CLASS`, dict and
set merges, `SEND`, imports, formatting, `CONTAINS_OP`, exception matching,
intrinsics, templates and unpacking. From V2a-1 the cases generator emits an
owned `PyValue_AsObject` local, released after the call, for every family not
yet converted, and the packet that converts a family removes its locals. D2
removes the non-canonical form `PyValue_Box` produces. In A5b (in
implementation) that local is a `PyObject *x_o` owned by `value_temps`
([proposal](proposals/register-ir-v2-a5b.md) §4.5).

**Identity across the boundary (R9, permanent).** An inline int or float that
crosses to the object form gets a fresh box each time, except an int in
-5..256, which boxes to the cached immortal. Upstream returned the stored
object, so a compatibility caller can see two pointers for one stored value:

* `PyObject_SetAttr(o, name, x)` then `PyObject_GetAttr(o, name)` returns `x'`
  with `x' != x` when `x` is an exact int above 256 or an exact float.
* `PyList_Append(l, x)` then `PyList_GetItemRef(l, 0)` returns `x'` likewise.
* A caller passes `x` into a thunk, and the slot adapter hands an extension
  slot a different box `x'` of the same inline value.
* Cython compiles `a is b` on `object` variables to C pointer equality, so
  `l[0] is l[0]` is false for `l = [1000]` in Cython code and true in Python
  code. Python code uses value identity ([pyvalue.md](pyvalue.md) §1.7) and
  agrees with upstream.

Python promises no identity for ints or floats, so this is compatible
behaviour, and it stays: the core does not keep the caller's pointer. V2a adds
a `_testcapi` test that pins the round trips above, so a change is deliberate.

### 1.4 Conventions

* **Ownership.** Arguments are borrowed. Results are owned. This is HPy's
  rule ([extension-api.md](extension-api.md)). A function that consumes an
  argument says so with `Take` in its name. If D4 introduces a distinct C type
  for borrows (pyvalue.md G6, register IR IR-D3), argument positions take that
  type and results stay `PyValue`.
  Since register IR v2 step A5a (`3e28e1755bd`) no frame slot holds a borrow:
  `PyValue_BorrowAt` returns the slot's bits untagged, and the typed borrow
  (step A5b) is not yet built.
  **Borrow conversion preserves the null sentinel, not non-nullness.**
  `PyValue_FromObjectBorrow(NULL)` yields `PyValue_NULL`; this says only that
  the source pointer was NULL. It does not prove a nullable public argument is
  safe for a value provider. Keep the public entry's null check, pending-error
  handling, and validation order before calling a provider that requires a
  language value. Do not weaken that provider's non-null contract to absorb a
  legacy NULL case: preserve the public API's behavior at the outer boundary.
* **Error.** A `PyValue` result signals an error with `PyValue_NULL` and an
  exception set. After D5, `PyValue_NULL` is 0, so the test is one compare,
  and a thunk passes NULL through unchanged.
* **Absence and exhaustion.** Use `_PyStatusValue f(…)`, returned by value
  in two registers (value-api-m1b-m2.md B4): status 1 found (`value`
  owned), 0 absent or exhausted (`value` is `PyValue_NULL`, no exception), -1
  error. The status values equal `PYGEN_NEXT` (1), `PYGEN_RETURN` (0) and
  `PYGEN_ERROR` (-1), and `pycore_value.h` asserts it. No value function takes an out-parameter for
  its result; pointers remain for arrays and in-out state. The object API's
  own signatures (`PyIter_Send`, `PyMapping_GetOptionalItem`) keep theirs,
  through the thunk exit. This matches `PyDict_GetItemRef` and `PyIter_NextItem`. A NULL
  result never means "absent", and core code never raises an exception to
  signal an outcome.
* **Iteration and send.** The value `iternext` and `send` slots return
  `_PyStatusValue`: `PYGEN_NEXT` with the item in `value`; `PYGEN_RETURN` when
  the iterator ends without raising, with the return value in `value`
  (`PyValue_NULL` when there is none); `PYGEN_ERROR` for every raised
  exception, `StopIteration` included, left exactly as raised. Generic
  consumers treat a raised `StopIteration` as the end, as upstream does; `next()`
  passes it through (§2.1).
  An iterator step whose box fails raises `MemoryError` and consumes the item
  (list, tuple, set and dict iterators). A pop boxes before it removes, so a
  failed box removes nothing (`list.pop`, `set.pop`, `dict.pop`).
* **Absent arguments.** `PyValue_NULL` in an argument position means "not
  given" where the object form uses NULL (delete in `setattr`).
* **`PyValue_ERROR`.** Two producers, both in `Python/ceval.c`:
  `_PyEval_GetIter` and `_PyForIter_VirtualIteratorNext`, with 4 consumers in
  `Python/bytecodes.c`. The first becomes a NULL result; the second becomes
  three-state. Then the constant is deleted.
* **Internal iteration ABI.** `_PyValueIndexPair` (`Include/object.h:319`)
  keeps its form: the index is the third state.
* **`NotImplemented`.** `PyValue_NotImplemented` is the value of the static
  immortal `_Py_NotImplementedStruct`. Value slots return it; dispatchers
  compare it with `PyValue_Is` (identity of the word, ignoring the count bit).
* **Rulings on marked functions and checks (coordinator, 2026-11-05).**
  No file-local helper may rebuild a marked object function; object exits are
  written inline at the site, and value-native consumers are preferred. A
  check has one macro, with the failure value as an argument; there are no
  twin `_V` macros. Argument checks (arity, keywords) live in the call
  trampolines with upstream's messages, never in generated code with a rebuilt
  name and never by rewriting messages after the call.

### 1.5 Naming

1. **Generic operations:** `_PyValue_<Op>`, where `<Op>` is the object-form
   name after `PyObject_`, `PyNumber_`, `PySequence_`, `PyMapping_` or
   `PyIter_` (`_PyValue_Add`, `_PyValue_GetAttr`, `_PyValue_Size`,
   `_PyValue_Vectorcall`). When two object families give the same `<Op>`,
   the `PyObject_` form takes the bare name (`_PyValue_GetItem`,
   `_PyValue_SequenceGetItem`); `PyObject_Length` is a second thunk name for
   `_PyValue_Size` (value-api-m1b-m2.md B5, §1.5). The underscore stays until
   a public value API exists.
2. **Concrete types:** the object-form name with `Item`, `ItemRef` or
   `EntryRef` replaced by `Value`, else with `Value` appended
   (`PyList_GetItemRef` → `PyList_GetValue`, `PySet_Add` → `PySet_AddValue`,
   `_PyList_AppendTakeRef` → `_PyList_AppendTakeValue`).
3. **Borrowed results:** inline unchecked accessors are upper case
   (`PyTuple_GET_VALUE`). A function that returns a borrowed view returns a
   `PyValueBorrow` and takes no `Borrowed` suffix; the type says it (V4, after
   A5b, in implementation).
4. **Representation operations** in `pycore_value.h` keep their names
   (`PyValue_Is`, `PyValue_DUP`); G6 already merges the duplicate pairs.

| Today | Becomes | Uses | When |
|---|---|---|---|
| `PyValue_FromPyObjectSteal` | `PyValue_FromObjectSteal` | 161 (95 hand-written, 66 generated) | A5b's pair merges (deferred from step 8, where it would have collided with the A1/A2 generated files); it canonicalizes since step 8 |
| `PyValue_FromPyObjectBorrow` | merged into `PyValue_FromObjectBorrow` | 11 (9 hand-written, 2 generated) | A5b (deferred from step 8); it canonicalizes since step 8 |
| `…NewMortal`, `…StealMortal` | deleted | 1, 14 | done, step 8 (`308a86c3914`) |
| `PyValue_FromPyObjectNew` | `PyValue_DUP(PyValue_FromObjectBorrow(o))` | 53 | A5b (deferred from step 8); it canonicalizes since step 8 |
| `PyValue_NewRef`, `PyValue_Close` | `PyValue_XDUP`, `PyValue_XCLOSE` | 59 (41 hand-written), 9 | A5b-4, after the heap side (G6's merged pairs; [proposal](proposals/register-ir-v2-a5b.md) §7) |
| `PyValue_AsPyObjectBorrow`, `PyValue_AsPyObjectSteal` | `PyValue_AsKnownObjectBorrow`, `…Steal`, checked | 892 | V1 |
| `PyValue_AsPyObjectNewBoxed` | merged into `PyValue_AsObject` | 13 | V1 |
| `PyValue_Box` | deleted | 105 | V2a-1 |
| `_PyValue_RichCompareObject` (returns `PyObject *`) | `_PyValue_RichCompare` returning `PyValue` | 5 | V2a-1 |
| `_PyObject_GetAttrValue(PyObject *, …)` | `_PyValue_GetAttr(PyValue, …)` | 3 | V2c |
| `_Py_type_getattro_value` | the type's value `getattro` slot | — | V2c |
| `_PyDict_NextBorrowed`, `_PySet_NextBorrowed` | return `PyValueBorrow` and lose the `Borrowed` suffix (rule 3); the names are chosen in V4, because `PyDict_NextValue` is taken | 7, 4 | V4 |
| `PyList_GET_VALUE`, `PyStructSequence_GET_VALUE` (out-of-line functions) | inline, per rule 3 | 40, 5 | V4 |
| `PyCell_GetValue` (NULL for empty or error) | `int PyCell_GetValue(PyObject *, PyValue *out)` | 2 | V4 |
| `_PyListIter_NextValue`, `_PyDictIter_NextValue`, `_PySetIter_NextValue`, `_PyTupleIter_NextValue` | the types' value `iternext` slots | — | V2b |
| `_Py_BuiltinCallFast_Value` and 6 other `_Value` call helpers (`pycore_ceval.h`) | deleted by the value vectorcall | — | V3b |

### 1.6 The representation boundary (R26)

The word's encoding is private to a few files, so that the representation can
change by editing them, as int51 (representation steps 9a and 9b) did. Code
elsewhere uses accessors. Three rules:

1. **Who may know the encoding.** Reading or writing `.bits`, building a word
   from bits or a pointer, and naming the encoding constants (`Py_INT_BASE`,
   `Py_INT_SHIFT`, `Py_TAG_*` and the NaN-box masks) are allowed only in:
   - the value headers: `pycore_pyvalue.h`, `pycore_value.h` and
     `pycore_value_api.h`, plus `Python/pyvalue.c`;
   - the JIT (`Python/jit_*.c`, `jit_x86.h`), which emits tag tests;
   - out-of-process readers, which get the layout through `_Py_DebugOffsets`
     and its version: `pycore_debug_offsets.h`, `Modules/_remote_debugging/`
     and `Tools/gdb/libpython.py`;
   - the encoding's own tests in `_testinternalcapi`;
   - the generated-code exception `_PyIR_AdoptTemp`, the type cache's
     guarded word (`pycore_typecache.h`), and the data stack's address
     conversions (`_PyDataStack_Slots`, `_PyDataStack_Address`).

   A cast between a word pointer and a byte pointer (`void *`, `char *`) is
   allowed everywhere, because code computes a word's address as a byte
   offset: `structmember.c` and the `STORE_ATTR_*` offset stores add a
   member offset to a `char *`, and `listobject.c` allocates with
   `PyMem_*`. So these need no entry. A second cast must not launder the
   first: the lint rejects a word pointer made from the address of a
   non-word object or from a cast of a pointer to another type, and a
   typed pointer made from a cast of a word pointer
   (`*(PyValue *)(void *)&bits`, `*(uintptr_t *)(char *)&word`). A match
   written in a value header's macro body is the header's; one written in
   the macro's argument is the caller's.

   At `a6002e5854b` there are 21 other sites:
   - **Exact-word compares**, 12 in `setobject.c` and `dictobject.c`. They
     get an exact-word equality accessor, which is not `PyValue_Is`
     (`PyValue_Is` masks bit 0 until A5b's target form).
   - **Debug dumps in `dictobject.c`**, 3. They get the word as a
     `uintptr_t`, for hashing and debug output only.
   - **Set's dummy key** (`setobject.c:66`). It becomes a named sentinel in
     the value header.
   - **Others**, 5:
     - `cellobject.c:164` (identity hash);
     - `flowgraph.c:1368` (hash key);
     - `ceval.c:2282` (a poison word);
     - `Modules/_testcapi/list.c:127, 129`, which tests the public API
       and so uses only public operations.
2. **Public headers carry the word opaquely.** In `Include/` outside
   `internal/`:
   - `PyValue` and `PyValueBorrow` appear only as parameter, result and field
     types.
   - No inline function or macro decodes a word or builds one. An inline
     accessor may copy a word out of a public field (V4's `PyList_GET_VALUE`).
   - Every other operation is an exported function. At `a6002e5854b` there
     are 22 such functions and no inline decoding.
   - The public layouts that hold a word are frozen at five:
     `PyListObject.ob_item`, `PyTupleObject.ob_item`, `PyCellObject.ob_ref`,
     `setentry.key` and `_PyValueIndexPair.value`. Adding one needs a
     decision. V4 decides whether these layouts move to internal headers.
3. **No encoding in an extension ABI.**
   - `Py_VALUE_CORE` units, the core and in-tree modules (V6), may use the
     internal accessors, because they are rebuilt with the interpreter.
   - A value API for third-party extensions uses exported functions over the
     opaque word, or handles that an implementation maps to words (HPy's
     universal mode, §8). It never uses the bit layout.

**Enforcement.**
- A5b-3 widens `check_value_api.py`'s `raw-bits` rule from construction to
  every `.bits` access and every encoding constant outside rule 1's list.
- It adds a `public-value` rule for rule 2, covering inline decoding in a
  public header and a word field outside the frozen five.
- Both rules get self-tests.
- A5b-3 also converts the 21 sites.

## 2. Value slots (VA5, R2)

### 2.1 Slots

```c
/* Include/cpython/object.h: appended to struct _typeobject after _tp_cache.
   _tp_iteritem stays where it is (bytecodes.c reads it on FOR_ITER). */
typedef struct {
    binaryvaluefunc nb_add, nb_subtract, /* … the 13 binary and 12 in-place nb_ slots */;
    ternaryvaluefunc nb_power, nb_inplace_power;  /* PyValue (*)(PyValue, PyValue, PyValue) */
    unaryvaluefunc nb_negative, nb_positive, nb_absolute, nb_invert,
                   nb_int, nb_float, nb_index;    /* PyValue (*)(PyValue) */
    int (*nb_bool)(PyValue);
    PyValue (*richcompare)(PyValue, PyValue, int);
    Py_hash_t (*hash)(PyValue);
    PyObject *(*repr)(PyValue);                   /* str result */
    PyObject *(*str)(PyValue);
    PyValue (*getattro)(PyValue self, PyObject *name);
    int (*setattro)(PyValue self, PyObject *name, PyValue value);  /* NULL deletes */
    PyValue (*descr_get)(PyObject *descr, PyValue obj, PyObject *type);
    int (*descr_set)(PyObject *descr, PyValue obj, PyValue value);
    PyValue (*subscript)(PyObject *self, PyValue key);
    PyValue (*item)(PyObject *self, Py_ssize_t i);
    int (*ass_subscript)(PyObject *self, PyValue key, PyValue value);
    int (*ass_item)(PyObject *self, Py_ssize_t i, PyValue value);
    int (*contains)(PyObject *self, PyValue item);
    _PyStatusValue (*iternext)(PyObject *self);   /* am_send contract */
    _PyStatusValue (*send)(PyObject *self, PyValue arg);
    vectorcallvaluefunc call;                     /* type-level fallback, §3 */
} PyValueSlots;

    /* in struct _typeobject: */
    PyValueSlots tp_value;
    Py_ssize_t tp_value_vectorcall_offset;        /* §3 */
    vectorcallvaluefunc tp_value_vectorcall;      /* calls this type object, §3 */
```

Each entry pairs with the object slot of the same meaning (`nb_add` with
`tp_as_number->nb_add`, `iternext` with `tp_iternext`, `call` with
`tp_call`). A receiver is `PyValue` when `int` or `float` has the slot;
otherwise it is `PyObject *` (§1.2). The fields are inline, so:

* a dispatcher reads `PyValue_TYPE(v)->tp_value.nb_add`, one load less than
  upstream's `tp_as_number->nb_add`;
* positional static initialisers, which stop before `_tp_cache`, leave every
  value slot 0; core types use designated initialisers (`.tp_value.nb_add =`);
* inheritance works per field (§2.3) with no shared sub-table;
* each type grows by 53 fields (424 bytes): about 78 KB over the 183 static
  types in the tree (129 core, 54 in `Modules/`), and the same per heap type,
  about 4 MB for a program with 10,000 classes. `test_sys`'s `sizeof` checks
  for type objects change in V2a-0.

**`iternext` contract (R11).** The value `iternext` follows `am_send`. It
returns `_PyStatusValue`: `PYGEN_NEXT` with the item in `value`; `PYGEN_RETURN` when the iterator
ends without raising, with the return value in `value` (`PyValue_NULL` when
there is none); or `PYGEN_ERROR` for every raised exception, `StopIteration`
and its subclasses included, left exactly as raised. RETURN comes only from
core iterators (exhaustion, `PyValue_NULL`) and from generators and the
async-generator `asend` iterator, which return their value through the send
path (`gen_iternext` gives `PyValue_NULL` for a `None` return). Core
iterators that wrap another (`map`, `filter`, `zip`, `enumerate`) pass the
inner RETURN through unchanged, value included, and the inner ERROR
untouched: upstream they return NULL with the inner exception pending
(`Python/bltinmodule.c:661` onwards, `:1595` onwards), so
`next(map(str, g()))`, with `g` a generator that returns 5, raises
`StopIteration` whose `value` is 5 (R16). Two core
callers need the return value: `PyIter_Send` (`Objects/abstract.c:3043–3052`,
reached from `SEND` at `Python/bytecodes.c:1635` for `yield from` an iterator
without `am_send`) and `builtin_next` (`Python/bltinmodule.c:1853–1867`).

| Caller or adapter | Use of the result |
|---|---|
| Adapters for an extension `tp_iternext` and for a Python `__next__` (`slot_tp_iternext`) | An item: NEXT. NULL with no exception: RETURN with `PyValue_NULL`. NULL with any exception, `StopIteration` included: ERROR, exception untouched |
| Generic consumers: `_PyValue_IterNext`, `FOR_ITER`, `list()`, unpacking, `PyIter_NextItem` | Three-state 1/0/-1 (§1.4). RETURN is the end and drops the value; ERROR with `StopIteration` pending is also the end and clears it, as upstream `FOR_ITER` does; any other ERROR is -1. Monitoring is kept (R16): upstream's generic `FOR_ITER` calls `_PyEval_MonitorRaise` before it clears a pending `StopIteration`, which a generator that returns a value raises inside `gen_iternext`. On RETURN with a non-NULL value, generic `FOR_ITER` therefore emits that RAISE event when it is active, creating `StopIteration(v)` only then, as `INSTRUMENTED_END_FOR` does for `STOP_ITERATION` |
| `next()` | ERROR passes through unchanged, so a subclass, its arguments and its traceback reach the caller; with a default, a pending `StopIteration` is cleared and the default returned, as upstream. RETURN raises a fresh `StopIteration(v)`, or bare `StopIteration` for `PyValue_NULL`, or returns the default |
| `_PyValue_Send`, `PyIter_Send` | RETURN gives the value of `yield from` (`PyValue_NULL` becomes `None`). ERROR with `StopIteration` pending becomes RETURN with its value and the exception is cleared, as upstream does with `_PyGen_FetchStopIterationValue`; any other ERROR stays ERROR |
| Object-form thunk of a core type's `tp_iternext` | NEXT: the item, boxed. RETURN: raises `StopIteration(v)` only when `v` is not `PyValue_NULL`, as `gen_iternext` does, and returns NULL. ERROR: NULL with the exception as raised |

Check for readers of the value:
`rg -n '_PyGen_FetchStopIterationValue|\(PyStopIterationObject' Objects Python Modules`
and the 12 direct `tp_iternext` calls in `Objects/` and `Python/`.

### 2.2 Who defines which side

| Type | Value side | Object side |
|---|---|---|
| Core static type | Defined in the type's file | Specific slots: per-type thunks generated by a macro (`_Py_VALUE_THUNKS(long, …)`) and named in the static definition. Generic slots: left 0 and filled by `PyType_Ready` from the pair table |
| Extension type (static or `PyType_FromSpec`) | 0, except fields inherited by the rule of §2.3 | As written by the extension |
| Python class | `slot_*` value functions | Shared object thunks `slot_*_obj` that convert and call the value `slot_*` (§2.4) |

Pair table (generic functions; the object pointer keeps its identity, so
`tp_getattro == PyObject_GenericGetAttr` stays true):

| Object side | Value side | Core static types |
|---|---|---|
| `PyObject_GenericGetAttr` | `_PyObject_GenericGetAttrValue` | 77 |
| `PyObject_GenericSetAttr` | `_PyObject_GenericSetAttrValue` | 8 |
| `PyObject_HashNotImplemented` | `_PyValue_HashNotImplemented` | 9 |
| `PyObject_GenericHash` | `_PyValue_GenericHash` | 2 |
| `_PyObject_NextNotImplemented` | `_PyValue_NextNotImplemented` | 0 (installed by `update_one_slot` for `__next__ = None`) |
| `PyVectorcall_Call` (`tp_call`) | `_PyValue_InstanceVectorcall` (§3; value-api-m3.md) | 6 |

The core makes 22 identity tests on these pointers at HEAD (an earlier count
said 20); [value-api-m1b-m2.md](proposals/value-api-m1b-m2.md) §2.6 lists them by file. M2 moves 10 to the
value side, M1b 1 and I4 1; 10 stay. The rule: a presence test reads the
object slot and does not move; only an identity test of a pair-table
function moves, and tests naming a `slot_*` thunk or comparing two types'
slots stay.

### 2.3 Inheritance

`inherit_slots` keeps `COPYSLOT`/`COPYNUM`/`COPYSEQ`/`COPYMAP` and its
`SLOTDEFINED` tests for the object side. The value side rests on an invariant,
which `PyType_Ready` asserts for every type in debug builds (R14):

> **A value slot is non-zero only beside the object function paired with it:
> a core type's per-type thunk, a `slot_*_obj` thunk, or a pair-table entry.**

The value pass runs once per type, in `type_ready_inherit` after
`type_ready_inherit_as_structs`. That order matters: the sub-table pointers
(`tp_as_number` and the rest) are shared with a base only after
`inherit_slots` has run for every MRO base, so a pass inside `inherit_slots`
would see no `nb_*` object slots for case 2 below. The pass fills each value
field that is still 0 from **any MRO base whose object slot equals the
subtype's**. Under the invariant, equal object functions have the same paired
value function, so the order of bases does not matter.

| Case | Object slot | Value slot |
|---|---|---|
| Subtype defines neither side (inherits by `COPYSLOT`) | base's | base's |
| Static subtype with `tp_as_number` NULL (`type_ready_inherit_as_structs` shares the base's table pointer, review B2) | base's, through the shared table | base's: the object pointers are equal |
| Extension subtype of `int` that sets `nb_add` | its own | 0: the adapter runs its `nb_add`; the other slots it did not set keep `int`'s value slots |
| Extension subtype that sets only `tp_repr` | base's for every slot but `repr` | base's for every slot but `repr` |
| Core type (defines both) | its own | its own; the invariant holds after the pair table is applied |

Group rules carry over: `tp_richcompare` and `tp_hash` inherit together, as do
`tp_getattr`/`tp_getattro` and `tp_setattr`/`tp_setattro`; the value pair
follows the object pair's group decision. `tp_value.call` and
`tp_value_vectorcall_offset` follow `tp_call` and `tp_vectorcall_offset`
(including upstream's `Py_TPFLAGS_HAVE_VECTORCALL` rule). `_tp_iteritem` is
never inherited, because a subclass may override `__iter__`; neither is
`tp_value_vectorcall`, as `tp_vectorcall` is not. Behaviour alone cannot tell
the adapter from an inherited value slot, so V2a-1 adds a `_testinternalcapi`
check that compares each extension subtype's `tp_value.nb_add` with
`PyLong_Type`'s.

### 2.4 Python classes and slot updates (review I1)

| Mechanism | Change |
|---|---|
| `slotdefs` | Each entry names both offsets (object slot, value field) and both wrappers |
| Wrapper descriptors | `d_wrapped` keeps its meaning, the object function (`Objects/descrobject.c:551`, `:563`, `:1092`), because it is a public struct field (R19). `PyWrapperDescrObject` gains `d_wrapped_value`, the value function, and the wrapper calls it when it is set, so `int.__add__(5, 3)` from Python boxes nothing. `add_operators` sets `d_wrapped_value` when the type has the value slot, else leaves it 0 |
| `update_one_slot`, specific case | `specific` becomes a pair. Where the class's dunder is the base's wrapper descriptor for the same slot (`d->d_base->wrapper == p->wrapper`), both `d_wrapped` and `d_wrapped_value` are installed, so `class MyInt(int): pass` keeps `int`'s value slot. A descriptor from an extension type installs its object function and leaves the value slot 0 |
| `update_one_slot`, generic case | Installs the value `slot_*` function and its object thunk `slot_*_obj` |
| `generic = *ptr` (keep the current slot) | Keeps both sides |
| `__hash__ = None` | Both sides of the hash pair: `PyObject_HashNotImplemented` and its value form (the `update_one_slot` case and the `add_operators` test) |
| `__next__ = None` | `_PyObject_NextNotImplemented` pair |
| `tp_new` (`tp_new_wrapper`) | Unchanged: `tp_new` is not a value slot |
| `__getattribute__`/`__getattr__` resolving to object's | Generic attribute pair, so the identity tests of §2.2 still hold |
| `update_slot` (dunder assigned after creation, on the class or on a base with subclasses) and `fixup_slot_dispatchers` (creation) | Call `update_one_slot`, so they write pairs |
| `__class__` assignment | Unchanged layout check; the value slots come with the new type |
| `slot_*` functions (`SLOT1BINFULL`, `slot_tp_richcompare`, `slot_tp_hash`, `slot_tp_iternext`, `slot_sq_item`, the getattr hook) | Become value functions: they look up the dunder and call it with the value vectorcall (§3). A Python `__add__` that receives an inline int gets it in a frame slot with no box. `SLOT1BINFULL`'s own two-sided dispatch is kept |

### 2.5 Dispatchers (review B4)

Each dispatcher is a line-by-line port of the upstream function. Two changes
apply everywhere: `Py_TYPE(x)` becomes `PyValue_TYPE(x)`, and each slot read
becomes "the value slot if non-zero, else the object slot through the
adapter". The **adapter** boxes the Python operands into owned C locals, calls
the object slot, releases the boxes and converts the result with
`PyValue_FromObjectSteal`. For `iternext` (an extension `tp_iternext` or a
Python `__next__`) it maps NULL with no exception to RETURN with
`PyValue_NULL`, and NULL with any exception, `StopIteration` included, to
ERROR with the exception untouched (§2.1), returning `_PyStatusValue`. Where the upstream function compares slot pointers,
the port compares the chosen function (the value pointer if non-zero, else the
object pointer), so two extension types never merge and an `int` subclass that
inherits the pair deduplicates against `int` as upstream. The functions below
replace `value_binary_operation` (`Python/ceval.c:989`) and the boxing in
`Objects/value_ops.c`.

| Operation | Upstream | Behaviour the port keeps |
|---|---|---|
| Binary number ops | `binary_op1` (`Objects/abstract.c:941`) | Right slot dropped when the types are equal or the chosen functions are equal (`:958`); a right operand whose type is a proper subtype goes first; a `NotImplemented` result (§1.4) moves on |
| `+` and `*` | `PyNumber_Add` (`:1139`), `PyNumber_Multiply` (`:1177`) | After `NotImplemented`: the left operand's `sq_concat`; for `*`, `sequence_repeat` with either operand's `sq_repeat`. It keeps upstream's `_PyIndex_Check` on the count, with `TypeError` "can't multiply sequence by non-int of type '%.200s'", and converts the count with `_PyValue_AsSsize_t(n, PyExc_OverflowError)`, which reads an inline int directly (R17). Sequence slots are object slots, called with a boxed operand only if the other operand is inline |
| In-place ops | `binary_iop1` (`:1229`), `PyNumber_InPlaceAdd` (`:1308`) and `InPlaceMultiply` | The left in-place slot first, then `binary_op1`; then `sq_inplace_concat`/`sq_inplace_repeat`, then `sq_concat`/`sq_repeat` |
| `pow(a, b, c)` | `ternary_op` (`:1031`) | Three chosen functions; `slotw` dropped as in binary; `slotz` tried only if it differs from both (`:1089`); `c` boxed only for an object slot |
| Unary ops | `PyNumber_Negative` and the rest | Chosen slot or `TypeError` "bad operand type for unary -: '%.200s'" with `PyValue_TYPE` |
| Compare | `do_richcompare` (`Objects/object.c:1048`) | The reflected slot of `w` first when the types differ, `PyValue_TYPE(w)` is a subtype of `PyValue_TYPE(v)` and it has a compare slot (no override test, as upstream); then `v`'s; then `w`'s reflected unless already tried; then identity for `==`/`!=`; then the `TypeError` naming both types |
| Truth | `PyObject_IsTrue` (`:2097`) | `True`, `False`, `None` by identity; `nb_bool` (value or object; `slot_nb_bool` keeps its check that `__bool__` returns a bool); else `mp_length`, else `sq_length`, true when positive (object slots; an inline receiver always has `nb_bool`); else true |
| `__index__` | `_PyNumber_Index` (`abstract.c:1411`) | Any int (`PyLong_Check`: exact, `bool` and subclasses) returns itself without calling `__index__` (R17; `Lib/test/test_index.py:61–67`); otherwise the slot result must be an int (`TypeError` "__index__ returned non-int"), and an int subclass result warns (`DeprecationWarning`) and is accepted; the result is canonicalised by `PyValue_FromObjectSteal` |
| `__int__`, `__float__` | `PyNumber_Long` (`:1520`), `PyNumber_Float` (`:1603`) | Same checks: `__int__` must return an int (subclass warns), else `__index__` is tried; `__float__` must return a float (subclass warns), else `__index__`; strings and buffers parse as upstream |
| Hash | `PyObject_Hash` (`object.c:1158`) | A type that is not ready is readied; a zero slot raises "unhashable type"; `-1` from a slot is an error; `slot_tp_hash` keeps its result conversion of a large int |
| Get attribute | `PyObject_GetAttr` | Non-str name raises `TypeError`; `int` and `float` have the generic value pair, so an inline receiver never reaches an object slot |
| `len`, `iter`, `getitem`, `setitem`, `contains`, `call` on an inline value | `PyObject_Size`, `PyObject_GetIter`, `PyObject_GetItem`, `PyObject_SetItem`, `PySequence_Contains`, `PyObject_Call` | `TypeError` with the upstream message and `PyValue_TYPE(v)->tp_name`; no box |

Direct slot calls in core code bypass the dispatchers (33 sites, for example
`(*Py_TYPE(it)->tp_iternext)(it)` in `builtin_next`); each becomes a
dispatcher call or a value-slot call in the packet of its family, and the
check of §4 finds new ones.

When code must call a *specific base implementation* to bypass an override,
generic dispatch is not equivalent: it may select the subtype's value slot or
redispatch through another operation. Use a narrow typed-base helper that
receives both the exact object slot and the `PyValue` operands. The helper
selects that base type's paired value slot when available; otherwise it adapts
the exact supplied object slot and closes only the temporary operand boxes.
It must not repeat generic argument validation, perform name handling, or
redispatch to a different slot. The current helpers follow this rule:

| Helper | Base behavior preserved |
|---|---|
| `_PyValue_CallRepr` | Calls the supplied type's exact `tp_repr` implementation. |
| `_PyValue_CallRichCompare` | Calls the supplied type's exact `tp_richcompare` implementation and operation. |
| `_PyValue_CallSetAttrBase` | Calls the supplied base `tp_setattro`; if no value row exists, it adapts the exact slot with the original `self`, name, and nullable value. |
| `_PyValue_CallPowerBase` | Calls the supplied base `nb_power` row, including its third operand; it must not redispatch through a derived type's power slot. |

These helpers are internal mechanisms, not replacements for ordinary
`_PyValue_*` dispatch. Add one only when a port must preserve a proven base-slot
call and no existing value-slot interface expresses that behavior. The direct
slot-call checker covers call expressions; stored slot pointers called later
remain a separate finite source audit. Reads, comparisons, and copies are not
certified by that matcher, and the audit must list each copied pointer's
producer and call site before the gate can claim closure.

## 3. Calls (VA6, R7, review I3)

* **Value vectorcall per instance.** `typedef PyValue (*vectorcallvaluefunc)
  (PyThreadState *tstate, PyObject *callable, const PyValue *args, size_t
  nargsf, PyObject *kwnames);` (value-api-m3.md D1: one signature for every
  core callable, `tp_value.call` and the interpreter). The callable is a
  pointer because the operation has tested the tag (§1.2). `args` is a window
  of `PyValue` words that the caller keeps valid for the call. The callee
  borrows each element and never closes or stores one without `PyValue_DUP`
  (D2). The result is one owned word; `PyValue_NULL` with an exception is the
  only error (D3).
  A type with `tp_value_vectorcall_offset` stores the function in each
  instance, as PEP 590 does with `tp_vectorcall_offset`; `tp_value.call` is
  the type-level fallback. Core callables set it: Python functions (they
  initialise a frame from the values, as Python-to-Python calls do), bound
  methods, builtins (chosen by `METH_*` flags), method descriptors and
  classmethod descriptors. The method-wrapper, wrapper-descriptor and
  classmethod-descriptor calls are type-level `tp_value.call` entries, not
  per-instance fields (value-api-m3.md §1.3).
* **Type objects (R20).** Calling a type constructs an instance. `type` sets
  `tp_value_vectorcall_offset` to `offsetof(PyTypeObject,
  tp_value_vectorcall)`, so each type object holds its own constructor's value
  vectorcall in `tp_value_vectorcall`, the analogue of `tp_vectorcall` (§2.1);
  the 20 core types with a `tp_vectorcall` (§7.3) set it.
* **Dispatch.** `_PyValue_Vectorcall(PyValue callable, …)` follows
  `_PyObject_VectorcallTstate`: an inline callable raises "'int' object is not
  callable"; if the callable's type has `Py_TPFLAGS_HAVE_VECTORCALL`, its
  value vectorcall if set, else its object vectorcall through the call
  adapter; otherwise `tp_value.call`, else `tp_call` through the call adapter.
  The flag test keeps upstream's rule that a Python metaclass overriding
  `__call__` loses `Py_TPFLAGS_HAVE_VECTORCALL`, so `C()` runs `M.__call__`
  and never `type`'s constructor directly.
* **`PyFunction_SetVectorcall`.** It replaces a function's object vectorcall
  and also clears the function's value vectorcall, so dispatch reaches the
  replacement through the call adapter.
* **Offset rule.** `PY_VECTORCALL_ARGUMENTS_OFFSET` in `nargsf` keeps its
  meaning for value vectors: `args[-1]` exists and the callee may overwrite
  it temporarily.
* **Bound calls.** `im_self` (`PyMethodObject`), `m_self` (`PyCFunctionObject`)
  and the method-wrapper `self` are `PyValue` homes (V3a). A bound call
  prepends `self` as `_PyValue_VectorcallPrepend` does it, mirroring
  `_PyObject_VectorcallPrepend` (`Objects/call.c:925`): with the offset flag,
  save `args[-1]`, store `self` there borrowed, call with `nargs + 1` and the
  flag cleared, restore. `args[-1]` is an argument position for the duration
  of the call, not a home, so D4 ("no home holds a borrow") holds. Without the flag, copy `self`, the arguments and the
  keyword values into a stack array of `_PY_FASTCALL_SMALL_STACK` values, or a
  heap array when larger. With the flag, M3 writes `PyValue_DUP(self)` into
  `args[-1]` and closes it before restoring the saved word, because
  `args[-1]` can be a frame slot (value-api-m3.md D5). The borrowed getters
  of `self` (`PyMethod_GET_SELF`, `PyCFunction_GET_SELF` and kin) are
  deleted; `PyMethod_GetSelfRef` and `PyCFunction_GetSelfRef` replace them
  (D10).
* **Builtins.** A new flag `METH_VALUE` (D9: it carries no `METH_FASTCALL`,
  `METH_O` or `METH_NOARGS` bit, so code that tests those bits never calls a
  value function with object arguments). The final convention is in
  `Include/internal/pycore_value_call.h`; the keyword flag splits it as
  `METH_FASTCALL` is split. `METH_VALUE` alone takes positional arguments:
  the vectorcall of the function or its method descriptor rejects keywords
  with upstream's message and calls `ml_meth` with `kwnames` NULL.
  `METH_VALUE | METH_KEYWORDS` passes `kwnames` to `ml_meth`, which parses
  them. `METH_VALUE | METH_METHOD` requires `METH_KEYWORDS`, as upstream's
  `METH_METHOD` does. `METH_VALUE | METH_NOARGS` and `METH_VALUE | METH_O` are
  the fixed-arity forms: the vectorcall checks keywords and arity before the
  call, with the object forms' messages. All combine with `METH_CLASS`,
  `METH_STATIC` and `METH_COEXIST`. The signature is `PyValue (*)(PyValueBorrow
  self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)`
  (value-api-m3.md §1.5). `self` is a value because methods of `int` and
  `float` receive inline receivers.
* **Object-form natives.** A builtin without `METH_VALUE`, an extension
  `tp_call`, or an extension type's vectorcall is reached through
  `_PyBorrow_ValueVector` inside a borrow scope (native-boundary P4). This
  replaces `VALUES_TO_PYOBJECTS` (8 uses at `15859d9a592`, all in `Python/ceval.c`). Each core value vectorcall keeps
  an object-form vectorcall thunk, because `PyVectorcall_Function` exposes the
  pointer and Cython calls it directly.
  Decimal's selective Clinic constructor also registers this paired thunk;
  `THUNK_FILES` admits its creator in `Modules/_decimal/_decimal.c` while
  continuing to reject core callers that invoke the object thunk.
* **Argument Clinic.** A file-level directive selects value emission. The
  generated header then starts with `#ifndef Py_VALUE_CORE #error`. In value
  emission `object` parameters become borrowed `PyValue`; `int`, `Py_ssize_t`,
  `double` and `bool` converters read the inline word directly;
  `object(subclass_of=…)` and the str, bytes and buffer converters test the
  tag and then work as today. 181 of 459 core Clinic functions take a
  polymorphic parameter (§7.6; the converter table is value-api-m3.md §2.2). The 53 calls to `_PyNumber_Index` and
  `PyObject_IsTrue` in core Clinic headers go with value emission, so those
  two functions are marked after it (R6).
* **Monitoring.** `_Py_call_instrumentation_arg`, `_2args` and `_exc2`
  (`Include/internal/pycore_instruments.h:56–65`) take `PyValue` arguments;
  the callbacks they reach are object-form and go through the call adapter.
  This work belongs to I5, not M3a: all its callers are interpreter
  handlers, I5 stores one value trace function per thread, and
  `PyEval_SetTrace` and `PyEval_SetProfile` install a boxing adapter
  (value-api-m3.md D13, §1.8).

## 4. Enforcement (VA9, R4, review I7)

```c
/* Include/pyport.h, after the Py_BUILD_CORE_BUILTIN/MODULE logic */
#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN) \
    && !defined(Py_BUILD_CORE_MODULE) && !defined(Py_VALUE_CORE)
#  define Py_VALUE_CORE 1
#endif
#if defined(Py_VALUE_CORE)
#  if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 12)
#    define Py_VALUE_COMPAT(REPLACEMENT) \
         __attribute__((unavailable("compatibility API; use " #REPLACEMENT)))
#  else
#    error "value-native units need __attribute__((unavailable)): Clang or GCC 12+"
#  endif
#else
#  define Py_VALUE_COMPAT(REPLACEMENT)
#endif
```

| Rule | Mechanism |
|---|---|
| Value-native units | Every unit built with `PY_CORE_CFLAGS`: `Objects/`, `Python/`, `Parser/`, and the `Modules/` and `Programs/` files built into the core with those flags (`Modules/main.c`, `getpath.c`, `getbuildinfo.c`, `Programs/python.c`). A ported module adds `#define Py_VALUE_CORE 1` before `Python.h`. No Makefile change and no file list |
| Marking | A family is marked in the packet that sweeps its last core caller: `Py_VALUE_COMPAT(_PyValue_Add) PyAPI_FUNC(PyObject *) PyNumber_Add(…);`. A thunk definition compiles; a thunk that calls another thunk fails |
| Header inlines | GCC 15.2 rejects a use inside an unused `static inline` function (Clang accepts it inside an unavailable function). Six inlines call functions that will be marked: `PyObject_CallMethodNoArgs`, `PyObject_CallMethodOneArg` (`Include/cpython/abstract.h:62`, `:69`), `_PyObject_VectorcallTstate`, `_PyObject_CallNoArgsTstate`, `_PyObject_CallNoArgs` (`pycore_call.h:129–162`), `_PyObject_HashDictKey` (`pycore_object.h:842`). The packet that marks the callee ports each internal one to the value form and wraps each public one in `#ifndef Py_VALUE_CORE` |
| Variadic arguments | Neither GCC nor Clang warns when a `PyValue` reaches `%R`. `check_value_api.py` runs `clang-query` (`/usr/lib/llvm-21/bin`) over the core units with `compile_commands.json`, matching `callExpr(callee(functionDecl(isVariadic())), hasAnyArgument(hasType(anyOf(asString("PyValue"), pointsTo(asString("PyValue"))))))` on the desugared argument type, and fails on any match. The `pointsTo` arm catches a `PyValue *` passed where a `PyObject **` is expected, as in `PyArg_ParseTuple(args, "O", &v)` or `PyArg_UnpackTuple`, which would write a pointer into a value with no diagnostic (R18). V1 adds the make target `check-value-api` and builds the compile database with `intercept-build` (`/usr/lib/llvm-21/bin`) |
| Value format conversions | Each conversion that takes an object (`%R`, `%S`, `%A`, `%T`, `%N`, and `%#T`/`%#N`) gets a value form that takes a `PyValue`; `%U` and `%V` take a str and stay. V1 picks the spelling. 267 core calls use a conversion that needs a value form, out of 1136 calls to 13 formatting functions; `Python-ast.c` adds 103, fixed in its generator `Parser/asdl_c.py` (§7.7) |
| Direct slot-field reads | The same script flags reads of object slot fields (`tp_as_number->nb_*`, `tp_richcompare`, `tp_iternext`, `tp_call` and the rest) in core units; 33 today. A read of a slot whose family is marked fails; other reads are listed. Units that implement adapters or slot machinery (`abstract.c` dispatchers, `typeobject.c` slot code, the thunk macro) `#define Py_VALUE_ADAPTER` and are skipped |
| External extensions | Never define `Py_VALUE_CORE` and see no attribute |
| Generated code | `Python/generated_*.c.h`, Clinic headers and `Python-ast.c` compile in value-native units, so each family's packet fixes its generator, `Tools/cases_generator/jit_helpers_generator.py` included |
| Self-test | V1 adds one marked test declaration; the script compiles a one-line unit that calls it and expects the error. Two more units, one passing a `PyValue` to `PyErr_Format` and one passing `&v` for a `PyValue v` to `PyArg_ParseTuple`, must each give a match |

The ratchet has no allowlist: a family is either swept and marked, or not yet
marked.

## 5. Scope (VA10) and external extensions (VA11)

### 5.1 Internal scope

* **End state.** No unit with `Py_VALUE_CORE` calls a marked function.
* **`Modules/`.** Hot accelerators are ported in V6, ordered by the box
  counters (§6). Until then the proxy is the object-form call count:
  `_pickle.c` 149, `_ctypes/_ctypes.c` 116, `posixmodule.c` 104,
  `_datetimemodule.c` 92, `_operator.c` 90, `_asynciomodule.c` 88,
  `_elementtree.c` 81, `itertoolsmodule.c` 71. Unported modules need no change.
* **Fields.** §7.4 is the V5 list.

### 5.2 Public headers and a future public value API

* **No `PyValue` outside the core (R10).** V1 moves `Include/pyvalue.h`
  (included by `Python.h` at line 82 for every non-limited extension) and
  `PyTuple_GET_VALUE` (`Include/cpython/tupleobject.h:32`) to
  `Include/internal`. Of the 24 `Modules/` files that use `PyValue`, 22 are
  built with core flags (`Py_BUILD_CORE_BUILTIN`, `_MODULE`, or
  `PY_CORE_CFLAGS`); the two public-API test files (`_testcapi/list.c`,
  `_testcapi/tuple.c`) move their cases to `_testinternalcapi`. External code
  gets no `PyValue` declaration until a public value API is designed.
* **Conventions for that API.** Borrowed arguments, owned results, Dup and
  Close, NULL on error: HPy's rules. An HPy-compatible backend can then map an
  `HPy` handle to a `PyValue` word one to one, as JavaScriptCore maps
  `JSValueRef` to `JSValue` bits on 64-bit platforms (§8). Handles carry no
  borrow bit, and no public header exposes the encoding or pointers into
  container storage.
* Rebuilt `Python.h` extensions are compatibility users for good. Their cost
  is the box at each exit (C11).

## 6. Measurement (VA12) and cost model

### 6.1 Box counters

* **Counters.** `PyStats` gains `box[site]` and `entry_unbox[site]` (boxes
  freed or skipped by canonical entry). In stats builds the boxer takes a site
  (`_PyValue_AsObjectAt(v, SITE)`); `summarize_stats.py` prints "Boxing by
  site"; `inline_int_boxes`/`inline_float_boxes` become the totals.
* **Site classes:** compatibility exit by family (number, compare, repr,
  sequence, iteration, attribute, call, list, tuple, dict, set,
  cell/structseq); slot adapter by slot; call adapter
  (`_PyBorrow_ValueVector`, `_PyBorrow_TupleItems`); parser output
  (`getargs.c`); interpreter handler (`_PyValueTemps_Object`, until V2a the
  generated `PyValue_Box`); field store (V5 list); object thunk of a core type
  called by an extension.
* **Use.** The stats run of the in-tree benchmark set records the table with
  every V packet. The counts order the VS batches and V6. They are not a gate.

### 6.2 Cost model (review I6; estimates until measured)

| Cost | Where | Size | Bound or removal |
|---|---|---|---|
| Entry | `PyValue_FromObjectBorrow` is out of line and calls `_PyObject_ValueIdentity`; after int51, an int of 31–51 bits also runs `PyLong_AsLongAndOverflow` | a call, a type load, two compares per conversion | V1 inlines the pointer case; `_PyValue_FromObjectKnown` drops the type load for typed pointers |
| Type of a value | `PyValue_TYPE` in every dispatcher | one predictable branch on the tag | none; upstream loads `ob_type` |
| Slot adapter | an extension object slot with an inline operand: NumPy `arr + 1000` | one allocation and free per operation | permanent for unported extensions; counted |
| Exit box | compatibility callers reading stored inline values | one allocation per read | permanent (C11); counted |
| Release check | `PyValue_AsKnownObjectBorrow`/`Steal` | one compare and a cold branch | measured at V7, which decides whether it stays |
| Thunks | about 425 slot thunks and the API thunks | 30–40 KB of text | none |
| Type size | 53 inline value fields | 424 bytes per type, static or heap: about 4 MB for 10,000 classes | none |

**What the checkpoint measured (R24).** The checkpoint
([checkpoint-20261103.md](../bench/results/checkpoint-20261103.md), recorded
at python-jit `1f59c1ab480` from runtime `dbce7b1260d`) finds the runtime 4.7
times slower than upstream over 29 kernels. On object-heavy kernels a word
costs 75–125 ns, against 3.4–7.5 ns per upstream instruction, and no
Python-heavy kernel makes more than 0.11 boxes per word. The gap is a
per-word cost, not boxing. Its cause is being triaged (preliminary: method-JIT
admission running generic helpers); the value API does not address it. The
value API removes boxes, and this page claims nothing about when kernels
reach upstream speed.

Measured box sites by packet. The checkpoint's top 20 sites, 94.6% of boxes
per pass, from a histogram patch (`build/pyjit-link-wave1/checkpoint-box-callers.diff`)
that V1's counters replace; each packet re-derives the table from the
counters:

| Packet | Measured sites | Share of boxes | Main kernels |
|---|---|---:|---|
| V3a/V3b | builtin, method-descriptor and class calls through `_PyBorrow_ValueVector` and `_PyNativeScope_Objects`; `list.pop` | 35.9% | `fannkuch`, `float`, `raytrace`, `chaos`, `go`, `regex_compile` |
| V5 | `BUILD_SLICE` boxing `start` and `stop` (two sites) | 24.1% | `fannkuch` |
| V2a-1 | `value_binary_operation`, `COMPARE_OP` | 14.4% | `raytrace`, `nbody`, `chaos` |
| V2b | `STORE_SUBSCR`, `listiter_next`, `INTERPRETER_EXIT` (generator results) | 12.9% | `regex_compile`, `spectral_norm`, `nqueens`, `generators` |
| V4 and the sweep | `SET_ADD`, `PyList_GetItemRef`, `_PyDict_NextEntryRefKnownHash`, `MAP_ADD` | 7.3% | `nqueens`, `regex_compile`, `json_dumps`, `comprehensions` |

V2c has no measured site. `go`, `deltablue` and `richards` make 0.004–0.012
boxes per word, so no packet will move them measurably. Every packet is gated
on correctness and records its numbers.

## 7. Inventory (measured at `379e92cded9`)

Commands run from the source tree; `B=build/pyjit-link-wave1`. Counts are
lexical: a name followed by `(`, not a column-0 definition or `#define`.

### 7.1 Object-form polymorphic calls in `Objects/`, `Python/`, `Parser/`

`python3 -I $B/valueapi-objcalls.py .` (excludes `generated_*.c.h`,
`Python-ast.c` and `*/clinic/*.h`; Clinic headers are listed after the table;
`… Modules` gives the `Modules/` column).

| Family | Calls | Files | Top files | Top functions | `Modules/` |
|---|---|---|---|---|---|
| Number | 153 | 31 | `rangeobject.c` 31, `flowgraph.c` 15, `abstract.c` 11, `longobject.c` 10 | `PyNumber_AsSsize_t` 32, `_PyNumber_Index` 25, `PyNumber_Add` 18 | 176 |
| Compare, hash, bool | 162 | 44 | `rangeobject.c` 23, `bltinmodule.c` 10, `codeobject.c` 7, `object.c` 7 | `PyObject_RichCompareBool` 55, `PyObject_IsTrue` 43, `PyObject_Hash` 33 | 157 |
| Repr, str, format | 66 | 29 | `exceptions.c` 12, `object.c` 5, `unicodeobject.c` 5 | `PyObject_Str` 33, `PyObject_Repr` 23 | 29 |
| Sequence and mapping | 242 | 44 | `abstract.c` 39, `ceval.c` 22, `dictobject.c` 13, `import.c` 12 | `PyMapping_GetOptionalItem` 25, `PyObject_GetItem` 21, `PySequence_List` 20 | 239 |
| Iteration | 111 | 26 | `bltinmodule.c` 22, `dictobject.c` 14, `abstract.c` 12, `setobject.c` 10 | `PyObject_GetIter` 52, `PyIter_Next` 37, direct `tp_iternext` 10 | 116 |
| Attribute | 275 | 44 | `object.c` 31, `typeobject.c` 30, `bltinmodule.c` 15, `crossinterp.c` 15 | `PyObject_GetOptionalAttr` 72, `PyObject_GetAttr` 57, `PyObject_SetAttr` 30 | 299 |
| Call | 284 | 58 | `call.c` 27, `typeobject.c` 27, `import.c` 13, `descrobject.c` 11 | `PyObject_CallOneArg` 55, `PyObject_Vectorcall` 29, `_PyObject_CallNoArgs` 27 | 469 |
| List | 130 | 39 | `exceptions.c` 10, `stringlib/split.h` 10, `import.c` 9, `marshal.c` 8 | `PyList_Append` 55, `PyList_GetItemRef` 34, `PyList_SET_ITEM` 24 | 278 |
| Tuple | 297 | 62 | `exceptions.c` 35, `typeobject.c` 25, `genericaliasobject.c` 22 | `PyTuple_GetItemRef` 115, `PyTuple_SET_ITEM` 100, `PyTuple_Pack` 43 | 452 |
| Dict | 353 | 47 | `typeobject.c` 67, `import.c` 35, `sysmodule.c` 27, `moduleobject.c` 26 | `PyDict_SetItem` 88, `PyDict_GetItemRef` 77, `PyDict_SetItemString` 64 | 342 |
| Set | 69 | 18 | `symtable.c` 29, `codeobject.c` 6, `ceval.c` 5 | `PySet_Add` 27, `PySet_Contains` 25 | 41 |
| Struct sequence, cell | 31 | 10 | `sysmodule.c` 10, `longobject.c` 4, `typeobject.c` 4 | `PyStructSequence_SET_ITEM` 19, `PyCell_Get` 6 | 125 |
| **Total** | **2173** | | | | **2723** |

Clinic headers in core add 53 calls (27 `_PyNumber_Index`, 26
`PyObject_IsTrue`), which value emission removes; `Modules/` Clinic headers
add 333. Under R1 every call in the table is swept, `PyObject_GetIter` and
`PySequence_List` included.

### 7.2 Existing value-form functions

`python3 -I $B/valueapi-valuefuncs.py .` lists every declaration in
`Include/` with `PyValue` in its signature (241 rows at `379e92cded9`, 234 in
revision 1, after removing false matches such as `PyUnicode_MAX_CHAR_VALUE`).

| Area | Representative signatures and conventions |
|---|---|
| Representation and ops (`pycore_value.h`, `pyvalue.h`, `pycore_value_ops.h`) | `PyValue_FromObjectBorrow(PyObject *)` canonical; `PyValue_FromPyObjectSteal` raw; `PyValue_AsObject` new ref or NULL; `PyValue_Box` returns its input on failure; `Py_hash_t _PyValue_Hash(PyValue)`; `PyObject *_PyValue_RichCompareObject(…)` |
| Containers (tuple, list, dict, set, cell, structseq) | `PyTuple_GetValue` owned, NULL on error; `int PyDict_GetValue(PyObject *, PyValue, PyValue *)` three-state; `PyDict_SetValue` borrows both; `_PyList_AppendTakeValue` steals; `_PyTuple_FromValueStealOnSuccess` |
| Interpreter (`pycore_ceval.h`, `pycore_code.h`, frames, JIT, native scope, borrow) | `_Py_BuiltinCallFast_Value` object result; `_PyEval_GetIter` ERROR on error; 16 `_Py_Specialize_*` take `PyValue` operands |
| Object, type, member | `_PyObject_GetAttrValue(PyObject *, PyObject *)` object receiver; `_PyMember_GetValue`; `_PyObject_GetMethodValue` |

Inconsistencies besides C9 and C10, with the fix:

| Inconsistency | Fix |
|---|---|
| `PyValue_FromObjectBorrow` canonicalises and accepts NULL; `PyValue_FromPyObjectBorrow`, one `Py` apart, does neither | merged (§1.5) |
| Three boxing exits fail three ways: `PyValue_Box` returns its input with an exception set; `PyValue_AsObject` returns NULL, raises `SystemError` for a non-language value and fails if an exception is pending; `PyValue_AsPyObjectNewBoxed` returns NULL | one exit, `PyValue_AsObject` (§1.3) |
| `PyValue_AsPyObjectBorrow` of an inline value is a garbage pointer in release builds | `PyValue_AsKnownObjectBorrow`, checked (R1) |
| Generic value operations return objects (`_PyValue_RichCompareObject`) or take object receivers (`_PyObject_GetAttrValue`) | renamed and retyped (§1.5) |
| `PyList_GET_VALUE`, `PyStructSequence_GET_VALUE` are out-of-line functions with macro names | inline (§1.5 rule 3) |
| Three spellings of "consumes": `Take`, `Take2` (`_PyDict_SetValue_Take2`), `StealOnSuccess` | `Take` for one argument, `StealOnSuccess` for arrays |
| `_Py_dict_lookup` takes a `PyValue` key with no marker in its name | V4 adds the marker |

### 7.3 Polymorphic slots in core static types

`python3 -I $B/valueapi-slots.py .` parses 129 static `PyTypeObject`
definitions in `Objects/`, `Python/`, `Parser/` (positional and designated
initialisers, with their number, sequence, mapping and async tables). The
"Spec" column counts `PyType_Slot` entries in core spec-based types.

| Slot | Types | Spec | Slot | Types | Spec |
|---|---|---|---|---|---|
| `tp_getattro` | 88 (77 generic) | 2 | `nb_or` | 18 | 2 |
| `tp_repr` | 67 | 8 | `sq_contains` | 18 | 0 |
| `tp_iternext` | 37 | 1 | `mp_subscript` | 17 | 0 |
| `tp_richcompare` | 35 | 2 | `nb_subtract` | 9 | 0 |
| `tp_hash` | 32 (11 generic) | 0 | `nb_bool`, `nb_and`, `nb_xor` | 8 each | 0 |
| `tp_call` | 16 (6 generic) | 1 | `mp_ass_subscript` | 8 | 0 |
| `tp_vectorcall` | 20 | 0 | `nb_remainder`, `nb_inplace_or` | 7 each | 0 |
| `tp_descr_get` | 12 | 0 | `sq_item` | 7 | 0 |
| `tp_setattro` | 12 (8 generic) | 1 | `nb_add`, `nb_multiply`, `nb_power`, unary ops, `nb_true_divide` | 5 each | 0 |
| `tp_str` | 11 | 0 | `nb_int`, `nb_float`, `nb_floor_divide`, `nb_divmod`, `nb_invert` | 4 each | 0 |
| `am_send` | 4 | 0 | `nb_index`, `nb_lshift`, `nb_rshift` | 3 each | 0 |
| `tp_descr_set` | 3 | 0 | `sq_ass_item`, in-place number slots | 2–3 each | 0 |

Totals (the script's per-slot output summed over the value slots of §2.1):
527 entries, of which 102 name a function of the pair table (§2.2), so
**about 425 need a per-type object thunk** (C3). `tp_vectorcall` adds 20
object-form vectorcall thunks (547 in all). One definition is misaligned by
the parser (a `bytes_str` in a `tp_setattro` position), so single counts may
be off by one. `Modules/` adds 54 static types and 1487 spec slot entries of
all kinds, which stay object-side until ported. `sq_length`/`mp_length` (14
each) and `tp_iter` (61) need no value slot (C2).

### 7.4 Polymorphic `PyObject *` fields of core objects

`rg -n -U '\{\s*"(\w+)",\s*(_Py_T_OBJECT|Py_T_OBJECT_EX)\s*,\s*\n?\s*offsetof\((\w+)' -o -r '$3.$1' Objects Python -g '*.c'`
lists the object members by struct (105 tokens, against 25 `_Py_T_VALUE`
tokens). The fields below were classified by hand: a field is a home if an
exact int or float can be stored in it.

| Field | Where | Heat |
|---|---|---|
| `PySliceObject.start`, `stop`, `step` | `Include/cpython/sliceobject.h:14` | Hot: `BUILD_SLICE` boxing `start` and `stop` is 24.1% of the checkpoint's boxes (§6.2), although `_PyEval_BinarySlice` slices exact list, tuple and str without a slice object |
| `rangeobject.start`, `stop`, `step`, `length` | `Objects/rangeobject.c:37` | Hot: every `range(n)` with `n` above 256 boxes `stop` and `length` |
| `PyCFunctionObject.m_self`, `wrapperobject.self`, `PyMethodObject.im_self` | `Include/cpython/methodobject.h:10`, `Objects/descrobject.c`, `Include/cpython/classobject.h:15` | Warm: `(5).bit_length`, `(5).__add__` as bound objects (V3a, §3) |
| `PyStopIterationObject.value` | `Include/cpython/pyerrors.h:73` | Warm: generator return values seen by `next()` and `send` |
| `_PyAsyncGenWrappedValue.agw_val`, `PyAsyncGenASend.ags_sendval` | `Objects/genobject.c` | Warm: every async-generator `yield` |
| `calliterobject.it_sentinel` | `Objects/iterobject.c:211` | Warm: compared with every result of `iter(f, 0)` |
| `interpolationobject.value` | `Objects/interpolationobject.c:43` | Warm: t-strings |
| `PyContextVar.var_default`, `PyContextToken.tok_oldval`, HAMT `b_array[]` | `pycore_context.h:35`, `:49`; `pycore_structs.h:54` | Warm: context variables; the HAMT is a container |
| `_odictnode.key` | `Objects/odictobject.c:512` | Warm: `OrderedDict` with int keys |
| `PyOSErrorObject.myerrno`, `winerror`, `filename`; `PySyntaxErrorObject.lineno`, `offset`, `end_lineno`, `end_offset`; `PySystemExitObject.code`; `PyAttributeErrorObject.obj` | `Include/cpython/pyerrors.h` | Cold |
| `func_doc`, `func_module`, `prop_doc`, `cm_callable`, TypeVar `bound`, `default_value`, type alias `value` | `funcobject.h`, `descrobject.c`, `funcobject.c`, `typevarobject.c` | Cold (any value is legal) |
| AST `constant` | `Include/internal/pycore_asdl.h:16` | Compile time only (C6) |

Not homes: the iterators of C5, and callables, iterators, exceptions, tuples
and dicts held in fields. Candidate `Modules/` homes for V6, from reading and
not counted: `collections.deque` blocks, `itertools` (`count`, `repeat`,
`accumulate`, `groupby`, `tee` data, `zip_longest` fill value),
`operator.itemgetter` items, `_pickle` memo arrays.

### 7.5 Boxing sites in `Objects/` and `Python/`

`rg -c '\b(PyValue_AsObject|PyValue_AsPyObjectNewBoxed|PyValue_Box|_PyValue_BoxInto|_PyValueTemps_Box|_PyValueTemps_Object|_PyBorrow_TupleItems|_PyBorrow_ValueVector|_PyValue_BoxInlineValue|VALUES_TO_PYOBJECTS)\s*\(' Objects Python -g '!generated_*' -g '!Python-ast.c' -g '!*/clinic/*'`
gives 131 sites in 25 files, unchanged from revision 1: `Python/ceval.c` 31,
`Python/bytecodes.c` 14, `Objects/dictobject.c` 14, `Objects/call.c` 11,
`Objects/typeobject.c` 8, `Python/getargs.c` and `Objects/listobject.c` 6
each, `Python/pyvalue.c` and `Python/borrow.c` 5 each, and 16 more files.
Generated code adds 118 (`generated_ir_cases.c.h` 111,
`generated_jit_helpers.c.h` 7). By function: `PyValue_AsObject` 61,
`_PyValueTemps_Object` 28, `_PyBorrow_TupleItems` 13,
`PyValue_AsPyObjectNewBoxed` 11, `VALUES_TO_PYOBJECTS` 8 (10 before the call transactions replaced two helpers; value-api-m3.md),
`_PyBorrow_ValueVector` 4, `PyValue_Box` 2, others 2. The owning object
getters box too: core calls `PyTuple_GetItemRef` 115 times,
`PyDict_GetItemRef` 77 and `PyList_GetItemRef` 34 (§7.1).

### 7.6 Clinic

`python3 -I $B/valueapi-clinic.py .` reads every `[clinic input]` block and
counts parameters whose converter is `object` without `subclass_of`, plus
`*args`.

| Scope | Functions | With a polymorphic parameter | Parameters | Polymorphic |
|---|---|---|---|---|
| `Objects/`, `Python/`, `Parser/` | 459 | 181 | 515 | 258 |
| `Modules/` | 2111 | 726 | 3087 | 1191 |

The core row counts 459 functions: 435 with the default return converter and
24 with a typed one (`-> int`, `-> Py_ssize_t`, `-> bool`, `-> double`), which
the earlier script skipped; the converter table is value-api-m3.md §2.2. The
polymorphic columns are unchanged.

Top core files: `bltinmodule.c` 34 functions, `bytearrayobject.c` 18,
`sysmodule.c` 16, `bytesobject.c` 12, `typevarobject.c` 12,
`unicodeobject.c` 11, `listobject.c` 9.

### 7.7 Families outside the object-call table (review I2)

Counts follow review 2's method (R25): `python3 -I $B/valueapi-r2-calls.py .
'REGEX' [DIR ...]` and `python3 -I $B/valueapi-r2-formats.py .`, which read `.c`
and `.h` files, skip generated and Clinic files, strip comments and count calls,
not lines. The `Modules/` column and the rows other than number construction
and format calls come from revision 2's `rg` count of `.c` call lines, which
agrees with the scripts to within 2 where both were run.

| Family | Core | `Modules/` | Value form | Packet |
|---|---|---|---|---|
| Number construction: `PyLong_From*` (`FromLong` 147, `FromSsize_t` 73; 262 C-integer constructors) | 288 | 1107 | `PyValue_FromInt64`, `PyValue_FromSsize_t`, `PyValue_FromDouble`: inline when the value fits, else a box | V2a-1 adds them; sweep in V2a-2 and with each family |
| `PyFloat_FromDouble` | 51 | 123 | `PyValue_FromDouble` | as above |
| Number extraction: `PyLong_As*` (`AsInt` 79, `AsSsize_t` 60, `AsLong` 44) | 158 | 371 | `_PyValue_AsSsize_t`, `_PyValue_AsLong`, `_PyValue_AsInt`, `_PyValue_AsDouble`: read the inline word, else call the object form | V2a-1, sweep in V2a-2 |
| `PyFloat_AsDouble` | 21 | 30 | `_PyValue_AsDouble` | as above |
| Error payloads: `PyErr_SetObject`/`_PyErr_SetObject` 56, `_PyErr_SetKeyError` 10, `_PyGen_SetStopIterationValue` 5 | 71 | 43 | `_PyErr_SetValue(type, PyValue)`, `_PyErr_SetKeyErrorValue`; the exception object boxes at creation until V5 makes `StopIteration.value` a home | V2b |
| Handwritten `PyArg_Parse*`/`PyArg_UnpackTuple` (`UnpackTuple` 24, `ParseTuple` 14, `ParseTupleAndKeywords` 7) | 45 | 698 | Core sites convert to Clinic with value emission; then `PyArg_*` are marked | V3a |
| Format calls (13 formatting functions) | 1136; 267 need a value conversion (`%R %S %A %T %N`), 387 take any object; `Python-ast.c` 107 and 103 more | 1025; 238 need a value conversion | value conversions (§4) | V1 conversions and check; sweep with each family |
| Getset getters that return numbers (`int.real`, `float.imag`, `long_get_real`) | — | — | A typed `d_value` record in `PyGetSetDescrObject` (record macros, shared thunk, SOABI bump under D15; value-api-m1b-m2.md §2.3); `int.real` returns its inline receiver | M2 |
| Bound objects from `descr_get` (`PyMethod_New` 4, `method_get`, method-wrapper) | 4 | 3 | `self` stored as a value (§3) | V3a |
| Monitoring (`_Py_call_instrumentation_arg`, `_2args`, `_exc2`) | 3 functions | — | `PyValue` arguments (§3) | I5 (V3b) |
| JIT helpers (`generated_jit_helpers.c.h`: 6 object calls, 5 `PyValue_Box`) | 11 | — | generator emits value calls | each family's packet |
| V5 field support: `tp_traverse` (`_Py_VISIT_VALUE`), `tp_clear`, `tp_dealloc`, `_Py_T_VALUE` members, `__reduce__`, the home checker | per field | — | per field | V5 |

Weak references and the buffer protocol take typed objects and need nothing.

## 8. Prior art

| System | Internal form | API form | What it changes here |
|---|---|---|---|
| V8 | Tagged `Object` (Smi or heap pointer) | `Local<Value>` handles in `HandleScope`s. `src/api/api.cc` converts with `Utils::OpenHandle`/`Utils::ToLocal`; the engine core does not include `v8.h`, while `src/inspector`, `d8` and extensions do ([embedding](https://v8.dev/docs/embed), [api.cc](https://chromium.googlesource.com/v8/v8/+/refs/heads/main/src/api/api.cc)) | Supports VA1 and VA9. Internal operations such as `Object::GetProperty` take a tagged receiver, Smi included, and in the common case find the prototype without allocating a wrapper: the model for R1. Fast API calls pass unboxed C arguments from a typed signature ([v8-fast-api-calls.h](https://chromium.googlesource.com/v8/v8/+/refs/heads/main/include/v8-fast-api-calls.h)); value-form Clinic does the same for `int` and `double` converters |
| JavaScriptCore | NaN-boxed `JSValue` ([JSCJSValue.h](https://github.com/WebKit/WebKit/blob/main/Source/JavaScriptCore/runtime/JSCJSValue.h)) | Opaque `JSValueRef`. On 64-bit, `toRef` is a bit cast of the encoded value; on 32-bit, a non-cell is boxed in a `JSAPIValueWrapper` ([APICast.h](https://github.com/WebKit/WebKit/blob/main/Source/JavaScriptCore/API/APICast.h)). Property access on a primitive uses `synthesizePrototype` instead of a wrapper | An opaque handle can carry immediates with no box (C11, §5.2). Our compatibility layer is JSC's 32-bit case: `PyObject *` is not opaque, so it boxes |
| Lua 5.4, LuaJIT | Tagged `TValue` | Stack-indexed C API: `lua_tointeger(L, i)`, `lua_pushnumber`. Values stay in stack slots; the pointers that do leave (`lua_tostring`, `lua_touserdata`) are valid only while the value stays on the stack ([manual §4](https://www.lua.org/manual/5.4/manual.html#4)) | The borrow stack is the same idea for parser outputs and call vectors: lifetime is the native frame, not the caller's bookkeeping |
| HPy | Interpreter's own | Opaque `HPy` handles; borrowed arguments, owned results, `HPy_Dup`/`HPy_Close`. In CPython ABI mode a handle is a `PyObject *`; in universal mode the representation is the implementation's ([ABI modes](https://docs.hpyproject.org/en/latest/overview.html)) | Fixes the value API's conventions (§1.4) so that a universal-mode backend can map a handle to a `PyValue` word. CPython ABI mode would box like the thunks |
| PyPy cpyext | `W_Root` objects, list strategies | Materialised `PyObject *` per object crossing to C, linked to the `W_Root`. `PyList_GetItem` on an integer-strategy list converts the whole list, because a borrowed box would leak ([Cuni, 2018](https://www.pypy.org/posts/2018/09/inside-cpyext-why-emulating-cpython-c-8083064623681286567.html)) | The deleted borrowed getters and counted boxes (VA12) answer the same problem; no identity map and no box cache (R9) |
| CPython `_PyStackRef` | Tagged references in frames only | Every C call converts with `PyStackRef_AsPyObjectBorrow`; heap storage is `PyObject *` ([pycore_stackref.h](https://github.com/python/cpython/blob/main/Include/internal/pycore_stackref.h)) | Upstream keeps the boundary at the frame. We move it to the C API edge, which is why this page needs value slots and value calls that upstream does not |

## 9. Rollout (R6, R21, review I4)

**Order (R21).** Mechanism packets (V2a-0 to V5) add value forms, value
slots, dispatchers and interpreter paths. Each converts only the code its
mechanism needs: thunks, dispatchers, slot functions, `value_ops.c`,
interpreter handlers and their generators. None sweeps other callers. One
sweep packet, **VS**, then edits each core file once, for every family whose
mechanism has landed, and marks each family as soon as its last core caller
is swept. This page recommends VS over a sweep per family, because:

* a function that adds numbers and then subscripts the result is edited and
  reviewed once, and its locals change type once, with no temporary
  conversion at a family boundary for the next packet to remove;
* the measured boxes (§6.2) sit in the interpreter, the dispatchers and the
  call adapter, which the mechanism packets convert, so deferring the caller
  sweep defers little gain;
* the V4 rewriter is built once and runs over every family together, so its
  mechanical/judgement split is measured on the real mix.

The cost is that marks, and with them the ratchet, arrive late; until VS, new
core callers of an object form are possible, and the check script lists them.
The alternative, a mechanism and then that family's sweep in each packet,
revisits every file once per family; choose it only if early marks matter more
than single edits. Every packet lands alone with the full suite at its
baseline pass set, records the box table (§6) and the in-tree benchmarks, and
is gated on correctness only. Sizes are estimates in agent-hours.

```mermaid
flowchart LR
    C["Checkpoint (done, 1f59c1ab480)"] --> V1["V1 checks, conversions, counters"]
    V1 --> V2a0["V2a-0 slot mechanism, thunks over dispatchers"] --> V2a1["V2a-1 core types"] --> V2a2["V2a-2 other types"]
    R8["Representation steps 5-8, 9a"] --> V2a1
    V2a2 --> V2b["V2b subscript, iteration, send"] --> V2c["V2c attributes, descriptors"]
    V2c --> V3a["V3a value vectorcall, METH_VALUE, Clinic"]
    P4["native-boundary P4"] --> V3a
    V3a --> V3b["V3b interpreter call handlers"]
    IR["IR v2"] --> V3b
    V2b --> V4["V4 container forms, sweep tool"]
    V2a1 --> V5["V5 fields"]
    V3a --> VS["VS sweep, each file once"]
    V4 --> VS
    VS --> V6["V6 Modules by box count"]
    VS --> V7["V7 close"]
    V3b --> V7
    V5 --> V7
    V6 --> V7
```

| Packet | Content | Entry | Exit | Tests | Size |
|---|---|---|---|---|---|
| **Checkpoint** | JIT-off baseline against upstream; box sites from a histogram patch | — | Done at 19:49 (python-jit `1f59c1ab480`, runtime `dbce7b1260d`) | full suite | — |
| **V1** | Box counters (§6.1), replacing the histogram patch; `Py_VALUE_CORE`, `Py_VALUE_COMPAT` with `#error` (§4); `check_value_api.py` (unavailable self-test, variadic matcher with the `PyValue *` arm, slot-field reads), the compile database from `intercept-build`, and `make check-value-api`; value format conversions; `PyValue_AsKnownObject*` rename with the release check; `_PyValue_FromObjectKnown`; inline pointer case of the borrowed entry; `pyvalue.h` and `PyTuple_GET_VALUE` to `Include/internal`; delete `PyValue_ERROR`; merge `PyValue_AsPyObjectNewBoxed` | Checkpoint | Stats build prints the box table; the check passes on the tree and fails on its self-tests; no family marked | `test_capi`, `test_pyvalue_slots`, `test_pystats`, a `_testinternalcapi` test that the release check aborts in a subprocess | ≈5 |
| **V2a-0** | Mechanism with no type converted: inline `tp_value` fields; the inheritance invariant and pass (§2.3); slotdefs pairs, `d_wrapped_value`, `update_one_slot` pairs (§2.4); pair table (each value side filled by the packet that writes it); the number, compare, truth, hash, index and repr dispatchers of §2.5 with the adapter; `PyValue_NotImplemented`; thunk macro. The `PyNumber_*`, `PyObject_RichCompare*`, `PyObject_IsTrue`, `PyObject_Hash`, `_PyNumber_Index` and `PyObject_Repr`/`Str` functions become thunks over the dispatchers, and `value_ops.c`'s mixed-operand paths call the dispatchers, so the whole suite runs through the adapter (R15) | V1 | Suite at baseline; **adapter box count non-zero on the suite** | `test_binop`, `test_descr`, `test_class`, `test_richcmp`, `test_compare`, `test_pow`, `test_index`, `test_bool`, `test_hash`, `test_sys` (type `sizeof`), `test_capi.test_number`, plus new `_testcapi` cases: an extension type with only object `nb_add` and `tp_richcompare`; static extension subtypes of `int` overriding `nb_add`, overriding only `tp_repr`, overriding nothing; a Python subclass of an extension type; a dunder assigned after class creation and on a base after a subclass exists; `__class__` assignment between classes with different value slots; `pow(a, b, c)` with a third type; reflected comparison with an `int` subclass | ≈10 |
| **V2a-1** | Value slots (number, compare, hash, bool, repr, str) on `int`, `float`, `bool`, `NoneType`, `str`, `tuple`; value `slot_*` for Python classes; interpreter generic `BINARY_OP`, `COMPARE_OP`, `TO_BOOL` call dispatchers; rewrite `value_ops.c`; `PyValue_FromInt64`/`FromSsize_t`/`FromDouble` and `_PyValue_As*` (§7.7); the cases generator emits owned `PyValue_AsObject` locals for unconverted families, then `PyValue_Box` is deleted (R22); round-trip identity test (§1.3) | V2a-0; representation steps 7, 8 and 9a (int slots written once for int51) | The six types box nothing on the number kernels' generic paths | V2a-0 list, `test_long`, `test_float`, `test_int`, `test_math`, `test_fstring`, `test_str`, `test_tuple`; a `_testinternalcapi` check that each extension subtype's `tp_value.nb_add` equals `PyLong_Type`'s where §2.3 says it inherits | ≈8 |
| **V2a-2** | Value slots for number, compare, hash, bool and repr on the remaining core static types | V2a-1 | The §2.3 invariant holds for every core type | V2a-1 list, `test_complex`, `test_decimal`, `test_fractions`, `test_set`, `test_range` | ≈5 |
| **V2b** | Value slots `subscript`, `item`, `ass_*`, `contains`, `iternext` (send contract, §2.1), `send`; the `iternext` adapters and object thunks; wrapping iterators pass the inner RETURN (R16); generic `FOR_ITER` emits the RAISE event on RETURN with a value; three-state `_PyValue_IterNext`; `next()` and `PyIter_Send`; the four `*Iter_NextValue` helpers become slots; error-payload value forms (§7.7) | V2a-2 | `FOR_ITER` generic path boxes nothing for core iterators | `test_iter`, `test_itertools`, `test_generators`, `test_yield_from`, `test_coroutines`, `test_asyncgen`, `test_builtin`, `test_dict`, `test_list`, `test_contains`, `test_exceptions`, `test_capi.test_abstract`; `next(map(str, g()))`, and the same through `filter`, `zip` and `enumerate`, with `g` returning 5 gives `.value == 5`; a `test_monitoring` case for `for` over a generator that returns a value; new `_testcapi` cases: an extension `tp_iternext` that sets `StopIteration(v)`, one that returns NULL with nothing pending, and one that raises another error, each through `for`, `next()` and `yield from`; a Python `__next__` raising `StopIteration(v)` through `next()`, `for` and `yield from`; a `StopIteration` subclass with arguments raised by `__next__` reaches the caller of `next()` with its type, arguments and traceback intact; a core generator's object `tp_iternext` called from C for `return None` and `return 5` | ≈8 |
| **V2c** | `getattro`, `setattro`, `descr_get`, `descr_set` with value receivers; generic attribute pair; getset value getters; the 22 identity tests | V2b | `(5).real` boxes nothing | `test_descr`, `test_class`, `test_property`, `test_builtin`, `test_capi.test_object`, `test_opcache` | ≈6 |
| **V3a** | Per-instance value vectorcall, `tp_value_vectorcall_offset` and `tp_value_vectorcall`; the `Py_TPFLAGS_HAVE_VECTORCALL` test; `PyFunction_SetVectorcall`; the offset rule and prepend (§3); `im_self`, `m_self`, wrapper `self` homes; `METH_VALUE`; Clinic value emission and regeneration of core Clinic output; core `PyArg_Parse*` sites to Clinic; monitoring arguments; the call adapter on the borrow stack | V2c; native-boundary P4. Not IR v2 | Clinic headers call no thunk | `test_call`, `test_clinic`, `test_monitoring`, `test_sys_settrace`, `test_descr`, `test_inspect`, `test_functools`; a Python metaclass overriding `__call__`; a `PyFunction_SetVectorcall` replacement that runs | ≈10 |
| **V3b** | Interpreter call handlers use the value vectorcall; delete `VALUES_TO_PYOBJECTS` and the seven `_Value` call helpers | V3a; IR v2 (call handler shapes, IR-D1 G4) | Call handlers box nothing for core callables | `test_call`, `test_generated_cases`, `test_monitoring`, `test_optimizer` | ≈6 |
| **V4** | Container value forms primary; renames of §1.5; object getters and setters become thunks; the sweep tool below, with its ownership table and first-hour M/J measurement. **Heap homes adopt `PyValueField`** (A5b, in implementation, leaves the field type to V4): a load returns a `PyValueBorrow` and a store takes a `PyValue`, which closes the copy-of-home gap ([proposal](proposals/register-ir-v2-a5b.md) §2.3). Rule 3 drops the `Borrowed` suffix | V2b | M/J split reported; container value forms complete | `test_list`, `test_tuple`, `test_dict`, `test_set`, `test_structseq`, `test_capi.test_list`, `.test_dict`, `.test_tuple`, `.test_set` | ≈6 |
| **V5** | §7.4 hot and warm fields become `PyValue` homes, with traverse, clear, dealloc, `_Py_T_VALUE` members, `__reduce__` and the home checker. The new homes use `PyValueField` (see V4) | V2a-1 | Field-store box count near zero on the benchmarks | `test_range`, `test_slice`, `test_exceptions`, `test_asyncgen`, `test_iter`, `test_context`, `test_ordered_dict`, `test_tstring`, `test_pickle`, `test_gc` | ≈8 |
| **VS** | Sweep each core file once for every family whose mechanism has landed: the calls of §7.1 and the families of §7.7. M sites by the tool, J sites by agents. Mark each family when its last core caller is swept, `PyObject_IsTrue`, `_PyNumber_Index` and `PyArg_*` included. Batches by directory: `Objects/`, `Python/`, then `Parser/` and the compiler (C6) | V3a, V4 | Every family with a landed mechanism marked | full suite per batch | ≈30 plus the judgement list |
| **V6** | Port every in-tree `Modules/` unit, accelerators first in box-count order (R27); each adds `Py_VALUE_CORE` | VS | Each ported module calls no thunk | each module's tests | per module |
| **V7** | Families whose mechanism landed after VS reached them (V3b, V5), generator fixes; every object function with a value form is marked; measure the release check of `PyValue_AsKnownObject*` and decide whether it stays | All above | No `Py_VALUE_CORE` unit calls a marked function; zero boxes at core sites over the suite and the kernels (R27) | full suite | ≈4 |

**Sweep tool (V4, R23).** Clang 21 is installed, with `intercept-build`;
Coccinelle and the libclang Python bindings are not.
`Tools/build/value_sweep.py` dumps one function at a time with
`clang -fsyntax-only -Xclang -ast-dump=json -Xclang -ast-dump-filter=<function>`
and the unit's flags from `compile_commands.json`, because a full dump of a
unit that includes `Python.h` runs to hundreds of MB. It finds each call to an
object function with a value form and classifies it, using two tables:

* **Ownership table.** One row per object function and its value form, giving
  what each argument and the result do: borrow, steal, `Take`, new reference.
  `PyList_SET_ITEM`, `PyTuple_SET_ITEM` and `PyList_SetItem` steal;
  `PyList_Append` and `PyDict_SetItem` borrow; value forms borrow or `Take`.
  A rewrite between forms with different ownership adds or drops the release
  the table names. A function missing from the table makes its sites J.
* **Assignment rules** for a local that changes type. Every assignment must
  have a rule: an object-returning call becomes its value form, or an entry
  primitive wraps it. `return local` from a `PyObject *` function adds a
  `PyValue_AsObject` exit, or the function stays object-form. `Py_SETREF`,
  `Py_CLEAR`, `Py_XDECREF` and `Py_DECREF` map to the value forms the table
  lists; `== NULL` becomes `PyValue_IsNull`. `&local` passed out makes the
  site J.

| Class | Pattern | Handling |
|---|---|---|
| M1 | Argument is `PyValue_AsKnownObjectBorrow(v)` or the result of an entry primitive, and the ownership table covers both forms | Mechanical: call the value form with `v`, adjusting releases |
| M2 | Result stored in a local whose every assignment and use has a rule | Mechanical: change the local's type and apply the rules |
| M3 | Result passed straight to another call with a value form | Mechanical: chain the value forms |
| J | Anything else: an unswept object API, a struct field, a macro body, a `goto` error path that releases mixed forms, a borrowed reference, a function missing from the table | Judgement: written to a work list for an agent |

The tool applies M edits by byte offset and leaves J sites unchanged. The
precedent is the step 6a classifier of the representation packet. V4's first
hour runs it over one family and reports the M/J split; VS's size assumes at
least two thirds mechanical.

Dependencies on other plans:

* **Representation packet.** V2a-1 needs canonical construction (step 8), so a
  value slot never meets a boxed small int, the release contract and
  restore-helper removal (steps 5–7), so `value_ops.c` is rewritten once, and
  int51 (step 9a, landed in `bf0639df819`), so `int`'s value slots are written once. D5 (NULL = 0) is
  not a semantic dependency.
* **Register IR v2.** Only V3b waits for it. V2a–V3a change helpers that the
  handlers call. The typed borrow (IR-D3, D4) changes argument types (§1.4).
* **Checkpoint.** It happened before V1, at 19:49 (`1f59c1ab480`), with box
  sites from a histogram patch. V2a-0's checkpoint entry is met; V1's counters
  replace the patch.

## 10. Open questions

1. **Public names (C9).** Decide the public spelling with the HPy backend:
   keep `Py<Type>_<Op>Value`, or take an HPy-style prefix.
2. **Getset value getters.** Settled by value-api-m1b-m2.md (§2.3): record
   macros, a shared thunk, and a typed `d_value` field appended to
   `PyGetSetDescrObject`, set once at creation, with a SOABI bump under D15.
   `PyGetSetDef` is unchanged.

Closed in revision 2: typed entry cost (R3), `pyvalue.h` visibility (R10),
`StopIteration.value` (R11, §2.1), extension subtypes' value storage (R2,
§2.3), and the pyvalue.md D7 row (now marked superseded).
