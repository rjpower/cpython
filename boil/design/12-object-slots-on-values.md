# 12: The object slots on values

Status: design, 2026-10-27, revised 2026-10-28 after review round 1,
for step 6d (note 06 section 6). It decides the value signatures of the
attribute, descriptor and call slots and the native descriptor calls,
the compatibility layer for `PyObject *` callers, and the five 6d items
of note 06 section 20.9. It retires note 08's "object slots box" row,
the boundary where the note 09 review said 6c leaves boxing.

## Decisions

1. **The slot types change in place**, as 6b's calling conventions did:
   `getattrofunc`, `setattrofunc`, `descrgetfunc`, `descrsetfunc`, the
   new `callfunc` of `tp_call`, and `getter` and `setter` return or take
   `PyValue` for the attribute value, with no object-form twin (2.1).
2. **`self` stays `PyObject *` in every slot**, as do names, type
   arguments, argument tuples and keyword dicts. Only exact int and float
   are inline, and their types fill none of these slots. Through
   attribute lookup an inline receiver reaches C only by int's and
   float's own methods and getsets, a closed set; an explicit `__get__`,
   `__set__` or `__delete__` with an inline instance boxes it in the
   wrapper (2.4).
3. **The public functions that are slots change with them**
   (`PyObject_GenericGetAttr`, `PyObject_GenericSetAttr`,
   `PyVectorcall_Call`), so their 146 initialisers stay as written (2.2).
4. **The compatibility layer is the protocol functions**
   (`PyObject_GetAttr`, `PyObject_SetAttr`, `PyObject_GetOptionalAttr`,
   `PyObject_Call`, `PyMember_GetOne`, `PyMember_SetOne`): canonicalise
   in, call the value form, box out; the core never calls them where an
   inline value can arrive (2.3).
5. **Mismatches are caught where they can be**: a debug build checks
   each slot result's canonical form and traps an inline value in a
   `PyObject *` field; a value-v1 binary is kept out by its suffix, the
   platform mark of `SOABI` becoming `boil2` (2.1).
6. **The dunder bridges and descriptors speak values**: the `slot_*`
   functions pass values to the Python methods they call, `wrapperfunc`
   returns `PyValue`, and property, members, getsets, `_tuplegetter`,
   functions, classmethod and staticmethod return values (3 to 5).
7. **A value stored into a native `PyObject *` field is boxed there.**
   What still boxes is listed with owners (4, 8).
8. **Polymorphic stores get `STORE_ATTR_SHAPE_POLY`** over the existing
   poly table. Its opcode, and `LOAD_ATTR_METHOD_LAZY_DICT`'s, come from
   folding the two `_NO_DICT` loads into the shape handlers, whose guard
   is one load shorter (6.1, 6.2).
9. **Capacity fills its block**: `S + 2` slots (six when `S` is 0),
   rounded up to the allocator's size class; no block grows, and S of 1
   to 3 get smaller ones (6.4, measured).
10. **Members guard the class version** through a mirror of
    `tp_version_tag` in every shape, so generalisation does not deopt a
    member site (6.5). The dictionary conversion presizes (6.3).
11. **Precise roots**: changed functions in 10a's call layer stay
    precise and pass the C mode in the changing commit; the typeobject.c
    bridges are written to the C mode's subset for 10a rung 1b to mark
    (7). Stress and kill-mode tests check the other lifetimes (9).
12. **Four rungs, two days, hard stop at three**: the hand-offs; the
    attribute and descriptor slots; calls and wrappers together, since
    the wrappers' consumers are `tp_call` functions; getsets last and
    first to be cut (11).

## 1. Scope

| In 6d | Out of 6d (owner) |
|---|---|
| `tp_getattro`, `tp_setattro`, `tp_descr_get`, `tp_descr_set`, `tp_call` | number, sequence, mapping, comparison, hash, iter, init, new and repr slots, which stay `PyObject *` and box at the call as 6b's value protocols do (note 06 19.11, "slot wrappers"; open question 2) |
| `getter` and `setter` of `PyGetSetDef`; `PyMember_*` value forms | `tp_getattr` and `tp_setattr`, the `char *` legacy slots (2.5) |
| the `slot_*` and `wrap_*` bridges of those slots; `wrapperfunc`'s result | the arguments of the other wrappers, which box through `PyArg_UnpackTuple` (19.11) |
| property, `_tuplegetter`, structseq fields, member and getset descriptors, functions, classmethod, staticmethod | a bound method for a classmethod call (an allocation, not a box; 8) |
| the five 6d items of 20.9 | re-shaping a dictionary-mode object (note 09's open question) |

## 2. Signatures

| Typedef | Value form | Was |
|---|---|---|
| `getattrofunc` | `PyValue (*)(PyObject *self, PyObject *name)` | returns `PyObject *` |
| `setattrofunc` | `int (*)(PyObject *self, PyObject *name, PyValue value)`; `PyValue_NULL` deletes | value `PyObject *`, NULL deletes |
| `descrgetfunc` | `PyValue (*)(PyObject *descr, PyObject *obj, PyObject *type)` | returns `PyObject *` |
| `descrsetfunc` | `int (*)(PyObject *descr, PyObject *obj, PyValue value)` | value `PyObject *` |
| `callfunc` (new; `tp_call`) | `PyValue (*)(PyObject *callable, PyObject *args, PyObject *kwargs)` | `ternaryfunc`, shared with `nb_power` |
| `getter` | `PyValue (*)(PyObject *self, void *closure)` | returns `PyObject *` |
| `setter` | `int (*)(PyObject *self, PyValue value, void *closure)` | value `PyObject *` |
| `wrapperfunc`, `wrapperfunc_kwds` | return `PyValue` | `PyObject *` |

The error channel is `PyValue_NULL` with an exception set, as for
`tp_iternext` and vectorcall. A `tp_call`'s `args` tuple holds values
already (6a), so only the result changes.

### 2.1 In place, not beside

A value slot beside each object slot would be two mechanisms for one
job, with an inheritance rule for the pair and a winner to choose when a
type fills both. 6b changed four conventions in place and froze the
ports for phase P; 6d does the same, so the port patches are written
once against the settled API. What an unconverted C type meets:

- **A static initialiser** naming an object-form function is a compile
  error: gcc 14 and later reject incompatible pointer types by default,
  and the gated compiler is gcc 15. A C++ extension fails too.
- **A cast or a `PyType_Slot` entry** (`void *`) compiles silently, and
  nothing makes every such mismatch loud. An object-form getter,
  getattro, `__get__` or call slot returns a heap pointer, a valid value,
  unless it returns an int that fits in int32 or a non-NaN float: a
  non-canonical box, which the debug check catches at the dispatch site
  (`_PyValue_CheckSlotResult(v, type, "tp_getattro")`, a fatal error
  naming both). An object-form setter handed an inline value faults at
  once if it dereferences it (top bits 0xFFFE for an int, 0x0002 to
  0xFFF2 for a float are not mapped). One that saves it in a `PyObject *`
  field without looking leaves a non-pointer in the heap: a reader
  faults later, far from the cause, and the debug build's tracer stops
  at the next collection (section 9). One that ignores it is harmless.
  The tree is closed by the audit below, binaries by the suffix.
- **A binary built for value-v1** has today's suffix and would load.
  6d's first ABI change, rung 1's `class_version` (it grows `_PyShape`,
  embedded in `PyTypeObject`), changes `SOABI`'s platform mark
  (configure.ac; note 08's ABI row) from `boil` to `boil2`, giving
  `.cpython-316-x86_64-boil2-linux-gnu.so`; a value-v1 binary, like an
  upstream one since 6c, is then not found (`ModuleNotFoundError`). The
  ports SDK's abi_id becomes `cpython-3.16-value-v2`. The mark versions
  the step, not the rung: in-tree extensions build with the interpreter,
  and the ports, the only other binaries, are frozen until phase P and
  rebuilt only at a landed step's end. The suffix keeps out
  conventionally named binaries only; an untagged `.so` or an explicit
  loader bypasses it.

Removing the tree's 101 `(getter)`, 99 `(setter)` and 9 slot-type
casts makes the compiler check every static initialiser. `PyType_Slot`
entries stay `void *`: 97 for the five slots, 38 naming a function other
than the generic ones. Each rung's work doc lists its slots' entries
(an `rg`, with the count) and the converted function each names, and its
review checks the list. The tree has about 100 implementations of the
five slots besides the generic ones, and 388 getset entries, 135 of
them Argument Clinic's.

### 2.2 The generic functions are slots

`PyObject_GenericGetAttr` is documented as the function to put in
`tp_getattro`; 146 initialisers and spec entries name it, and its 18
direct calls, with `PyObject_GenericSetAttr`'s, are custom slots'
fallbacks (`_pickle`, `_decimal`, `genericalias`, `union`, `super`, the
xx modules). Changing it with the slot keeps both uses correct and the
ten slot-identity tests meaningful. The same holds for
`PyObject_GenericSetAttr`, `PyVectorcall_Call` and the internal
`_PyObject_GenericGetAttrWithDict` and `_PyObject_GenericSetAttrWithDict`.
Note 06 section 3.5's rule that "functions that take `PyObject *` keep
taking it" is about the API; these are slot implementations first. In
rung 4 the rule takes `PyObject_GenericGetDict` and
`PyObject_GenericSetDict`, which getset tables name: they change with
`getter` and `setter`, keeping dict identity, the conversion's failure
and the refusal of a non-dict or a deletion.

### 2.3 The compatibility layer

| Object form | Value form | In | Out |
|---|---|---|---|
| `PyObject_GetAttr(o, name)` | `PyObject_GetAttrValue` (6b) | `PyValue_FromObject(o)` | box |
| `PyObject_SetAttr(o, name, v)` | `PyObject_SetAttrValue` (6b) | canonicalise `v` | int |
| `PyObject_GetOptionalAttr(o, name, &res)` | `PyObject_GetOptionalAttrValue` (new; 19.11's gap) | as above | box into `*res` |
| `PyObject_Call(c, args, kw)` | `PyObject_CallValue` (new) | canonicalise `c` | box |
| `_PyObject_Call_Prepend` | `_PyObject_Call_PrependValue` (new, internal) | as above | box |
| `PyMember_GetOne(addr, m)` | `PyMember_GetOneValue` (new) | none | box |
| `PyMember_SetOne(addr, m, v)` | `PyMember_SetOneValue` (new) | canonicalise `v` | int |

```c
int     PyObject_GetOptionalAttrValue(PyValue o, PyObject *name, PyValue *result);
PyValue PyObject_CallValue(PyValue callable, PyObject *args, PyObject *kwargs);
PyValue _PyObject_Call_PrependValue(PyThreadState *tstate, PyValue callable,
                                    PyValue self, PyObject *args, PyObject *kwargs);
PyValue PyMember_GetOneValue(const char *obj_addr, PyMemberDef *m);
int     PyMember_SetOneValue(char *obj_addr, PyMemberDef *m, PyValue v);
```

`callable` is a value, as in `PyObject_VectorcallValue`; an inline one
raises "not callable". `PyObject_GetOptionalAttrValue` returns 1 with
the value, 0 with `PyValue_NULL` and no exception when the attribute is
missing, -1 with `PyValue_NULL` and the exception set.
`PyMember_SetOneValue` deletes on `PyValue_NULL`. The value forms are
`PyAPI_FUNC`; each object form stays an exported function of a few lines
beside its value form (note 06 19.3), returning NULL or -1 with
`MemoryError` if its box cannot be allocated (`*result` NULL).

No public function calls `tp_descr_get` or `tp_descr_set`. Outside
object.c's executor and typeobject.c (`super_getattro`, `type_getattro`),
the direct callers are classobject.c's `method_getattro` and
`instancemethod_getattro`, which return the slot's value, and
`legacy_tracing.c` and `_lsprof.c`, which box it where they need an
object. `getattr(o, name, default)` and `hasattr` call
`PyObject_GetOptionalAttrValue` and join the box audit.

### 2.4 `self` stays an object

6b kept `self` as `PyObject *` for `METH_*` functions. 6d keeps it for
every slot, because through attribute lookup no slot implementation
outside int and float receives an inline receiver:

- `tp_getattro`, `tp_setattro`: int and float have the generic ones,
  which the value executor runs on the value itself (section 3).
- `tp_call`: int and float are not callable; `_PyObject_MakeTpCall`
  names the type in its error from `PyValue_Type(callable)`.
- `tp_descr_get(descr, obj, type)`: `descr` comes from a class dict and
  has a `tp_descr_get`, so it is never an int or float. `obj` is inline
  only when the attribute was found on `int`, `float` or `object`, whose
  dicts are immutable and hold only method, getset, wrapper and
  classmethod descriptors.
- `tp_descr_set(descr, obj, value)`: `obj` is inline only for a store to
  an int or float, which these same descriptors refuse.
- `getter`, `setter`: the same closed set (`int.real`, `float.imag`,
  `numerator`, `__class__`).

An explicit wrapper call escapes the argument: `property(f).__get__(3,
int)`, `d.__get__(None, 3)`, `d.__set__(3, v)`, `d.__delete__(3)` and
`object.__getattribute__(o, 3)` pass inline values where the slot takes
`PyObject *`. Every wrapper boxes an inline argument bound for a
`PyObject *` parameter (`obj`, `type`, `name`), anchors the box raw for
the slot call, and validates after boxing where the slot did (a numeric
name still raises the `TypeError` naming int); only the attribute value
passes as a value. These are the slot-wrapper row's boxes (open question
2). test_descr makes these calls with inline ints and floats on a
property, a Python descriptor (which sees the number as `type`) and a
member, checks results and errors, counts one box per inline argument,
and fails the box with `set_nomemory`.

Through lookup, the inline-receiver box is thus confined to int's and
float's C methods and getsets (chaos: 20,000 per loop, from
`n.bit_length()` in `random._randbelow`; open question 1). The executor
boxes once at the descriptor call and anchors the box as a raw pointer
(`_PyValueSpan_AppendObject`): a root holding `PyValue_FromObject(box)`
would hold the immediate and leave the box collectable.

### 2.5 `tp_getattr` and `tp_setattr`

The `char *` slots stay object slots; the dispatch canonicalises and
boxes around them, as today. Only `xx`, `xxlimited_35` and
`_testmultiphase` fill `tp_setattr`, nothing `tp_getattr`; note 08's
register gives them to the ports phase.

## 3. The attribute protocol on values

**Dispatch.** `PyObject_GetAttrValue(PyValue v, name)` reads
`PyValue_Type(v)->tp_getattro`. If it is `PyObject_GenericGetAttr`, it
runs the plan executor on the value; otherwise it calls the slot on the
object (`v` is then never inline, by 2.4). Today's three-way dispatch
and its receiver box go. `PyObject_SetAttrValue` is the same with
`tp_setattro`, passing the value through.

**The executor takes the owner as a value.**
`generic_getattr_value(PyValue owner, name, dict, suppress)` reads the
owner's shape with `_PyValue_Shape()` (note 09 section 3). SLOT and DICT
plans are impossible on an inline owner's shape; CLASS_VALUE and ABSENT
need nothing of the owner; METHOD, DESCR, DATA_DESCR and PROPERTY box an
inline owner once, at the descriptor call (2.4). `generic_setattr_value`
takes the owner as an object: an inline owner reaches only NO_STORAGE
and DATA_DESCR, which box for the error or the refusing descriptor.

**Hooks.** `_Py_slot_tp_getattro` calls `__getattribute__` through
`vectorcall_method_value`. `_Py_slot_tp_getattr_hook` runs the executor
with `suppress`, then calls `__getattr__` through `call_attribute`, whose
value form uses `PyObject_VectorcallValue` and the value `tp_descr_get`.
`slot_tp_setattro` passes `[self, name, value]` to `__setattr__` or
`[self, name]` to `__delattr__` as values. None boxes.

**Wrappers.** With `wrapperfunc` on values, `wrap_getattro` (new; today
`__getattribute__` goes through `wrap_binaryfunc`'s cast) returns the
slot's value, and `wrap_setattr` and `wrap_delattr` read the tuple's
items instead of `PyArg_UnpackTuple`, which boxes. The other wrappers
return `PyValue_FromResult` of their object result; `wrap_next` returns
the iterator's value, retiring 19.11's `wrap_next` row. So
`object.__getattribute__(self, n)` and a frozen dataclass's
`object.__setattr__(self, n, v)` move values without a box.

**Types, modules, super.** `type_getattro` and `type_setattro` use the
class dict by value (today `C.x = i` boxes the new and old values).
`module_getattro`, `super_getattro`, `proxy_getattr` and the `_ctypes`,
`_pickle` and `_decimal` slots return what they build or read.

## 4. Descriptors on values

| Descriptor | `__get__` | `__set__` | Remaining box or allocation |
|---|---|---|---|
| property | `PyObject_VectorcallValue(fget, &obj, 1)` | `fset` with `[obj, value]` | none |
| Python descriptor (`slot_tp_descr_get`, `_set`) | the raw, unbound `__get__` from `_PyType_LookupValueAndVersion`, called by `PyObject_VectorcallValue` with `[self, obj, type]`, `None` for absent; no `__get__` returns `self` and clears the slot, as now | `__set__` with `[self, obj, value]`, `__delete__` with `[self, obj]`, through `vectorcall_method_value`, which binds as `vectorcall_method` does now | none |
| member, `__slots__` (MEMBER plan) | the slot load, as in 6c | the slot store | none |
| member, other (`member_get`) | `PyMember_GetOneValue`: `_Py_T_VALUE` gives `None` for NULL, `_Py_T_VALUE_EX` raises `AttributeError`; `_Py_T_OBJECT` and `Py_T_OBJECT_EX` canonicalise the field, with the same NULL rules; `Py_T_BOOL` gives `True` or `False`; signed members `PyValue_FromLongLong`, unsigned ones `PyValue_FromUnsignedLongLong`, `Py_T_FLOAT` and `Py_T_DOUBLE` `PyValue_FromDouble` | `PyMember_SetOneValue`: numeric members read the value with the value accessors, with today's errors; `_Py_T_VALUE` stores the value; `_Py_T_OBJECT` and `Py_T_OBJECT_EX` box an inline value (below) | none on number reads; a box per inline value stored in an object field |
| getset (`getset_get`) | the getter's value | the setter takes the value | what each getter computes (rung 4 makes the number-returning ones direct values) |
| `_tuplegetter` (namedtuple field) | the tuple item, a value | refuses | none |
| structseq field | member `_Py_T_VALUE` | refuses | none |
| function (`func_descr_get`) | a bound method | | the bound method (value form only; call-pair forms do not bind) |
| method descriptor (`method_get`) | a builtin method | | an inline `self` box (2.4) |
| classmethod (`cm_descr_get`, `classmethod_get`) | a bound method of the type | | the bound method, for both forms |
| staticmethod (`sm_descr_get`) | the callable | | none |
| wrapper descriptor | a method-wrapper | | an inline `self` box (2.4) |

- Plans and the `PyMemberDef` and `PyGetSetDef` layouts do not change;
  `descr_get_value`'s canonicalisation goes. `LOAD_ATTR_PROPERTY` keeps
  pushing the getter's frame; the executor's PROPERTY path (megamorphic
  sites, `getattr()`, C callers) stops boxing in `PyObject_CallOneArg`.
- A classmethod's bound method is an allocation per generic call
  (section 10); binding it as (callable, type) is left for tier 1.
- **Native object fields box.** A `_Py_T_OBJECT` or `Py_T_OBJECT_EX`
  member (`property.__doc__`), and a setter that keeps its argument in a
  `PyObject *` field (`Pickler.persistent_id`), hold an object, so
  `p.__doc__ = 1.5` makes a box. The setter boxes with `PyValue_Object`
  and stores the box with no collecting call between allocation and
  store, after which the field, traced with its owner, holds it; an
  allocation failure returns -1 with the field unchanged. Turning such a
  field into `_Py_T_VALUE` changes a type's layout, not a slot, and waits
  for a field measured to matter (section 8). test_capi stores int32
  bounds, -0.0 and a NaN into a `_testcapi` type's `_Py_T_OBJECT`,
  `Py_T_OBJECT_EX`, `_Py_T_VALUE`, `Py_T_BOOL`, `Py_T_UINT` and
  `Py_T_ULONGLONG` members, reads them through both APIs and Python, and
  covers each NULL case.

## 5. Calls on values

- `tp_call` is `callfunc`. `_PyObject_MakeTpCall` returns the slot's
  value after `_Py_CheckFunctionResultValue`, with no canonicalisation.
- `PyObject_CallValue` is the value form of `PyObject_Call`;
  `_PyObject_Call` becomes `_PyObject_CallValue`. `CALL_FUNCTION_EX`
  calls the value form.
- `slot_tp_call` calls `__call__` through `_PyObject_Call_PrependValue`,
  which passes the tuple's items after `self` with
  `_PyObject_VectorcallPrependValue`. A callable instance's result is no
  longer boxed; the argument tuple of the tp_call protocol remains.
- `cfunction_call`, `wrapperdescr_call`, `classmethoddescr_call`,
  `wrapper_call`, `type_call`, `sm_call`, `instancemethod_call` and the
  module types' `tp_call` (`partial`, `lru_cache`, `itemgetter`,
  `attrgetter`, `methodcaller`, `ctypes`' function pointers, `_asyncio`'s
  step wrapper, `sqlite3`'s connection) return values.
- The wasm trampolines (pycore_wasm_trampoline.h, wasm_trampoline.c)
  gain `_PyWasm_TrampolineCallValue` for `tp_call` dispatch (rung 3);
  `descr_get_trampoline_call` and the setter trampoline change with the
  getset types (rung 4). No gate builds wasm; each work doc lists its
  trampolines as unbuilt.

## 6. The 6c hand-offs

### 6.1 Polymorphic stores (20.9 item 1)

**Handler.** `STORE_ATTR_SHAPE_POLY` = `unused/1 + _GUARD_SHAPE_POLY +
_STORE_ATTR_SHAPE_POLY + POP_TOP`. The guard is the load's, reused: it
finds the row whose shape is the owner's and whose version matches. The
store checks `slot_kind[row]` with `_PyValue_FitsKind` (exiting on a
misfit, as `_GUARD_SLOT_KIND` does, so the generic store generalises),
writes the value to `index[row]`, and, if `handler[row]` is
`STORE_ATTR_SHAPE_TRANSITION`, writes `descr[row]`'s `shape_to` into the
header after the slot, with release order. The poly table already
records each declined row (20.3), so its layout does not change. Rows
are REPLACE or ADD: a store in a base class's `__init__` adds the same
name to each subclass's own shapes, so ADD rows are the common
polymorphic case besides go's REPLACE rows. A site with more than four
shapes stays generic.

**Opcodes.** The specialised numbers are full; 125 to 127 are the three
left below RESUME. 6d frees two and spends two, so the reserve stays at
three. `LOAD_ATTR_METHOD_NO_DICT` and `LOAD_ATTR_NONDESCRIPTOR_NO_DICT`
are deleted: their types' instances have no attribute storage, so each
carries the type's root shape (note 09 section 2), whose version is the
type's since a root has no field to generalise, and `_GUARD_SHAPE`
against the root plus `_GUARD_SHAPE_VERSION` proves what
`_GUARD_TYPE_VERSION` proved, in two loads instead of three. The
specialiser gives METHOD and CLASS_VALUE on such types the `_SHAPE`
handlers. One number goes to `STORE_ATTR_SHAPE_POLY`, one to
`LOAD_ATTR_METHOD_LAZY_DICT` (6.2). The pyc magic is bumped in the same
commit, and test_opcache, test_dis and test_shapes change their handler
names. Numbering specialised handlers past 255, as the IR-only ones are
(the handler field is 16 bits; `_PyOpcode_Deopt` and `dis` assume 256),
would end the scarcity but changes the IR metadata, not the object
protocol: open question 4.

### 6.2 The lazy-dict method path (item 3)

A type whose instances keep a dict word (`tp_dictoffset` nonzero,
negative offsets included: exceptions, functions, int and tuple
subclasses, C types with a dict) has only its root shape, and a method
lookup on it is a DICT plan with METHOD as fallback. When the observed
instance's dict word is NULL, the specialiser picks
`LOAD_ATTR_METHOD_LAZY_DICT` = `_GUARD_SHAPE + _GUARD_SHAPE_VERSION +
_CHECK_DICT_WORD_NULL + _LOAD_ATTR_METHOD_SHAPE`; the check reads the
word at the entry's `dictoffset` (the union field the `WITH_HINT`
handlers use) through `_PyObject_DictAt` and exits if it is not NULL. No
benchmark measures it; a micro row does (section 10).

### 6.3 The dictionary conversion (item 2)

`dict_from_slots` allocates with `_PyDict_NewPresizedUnicode(nslots + 1)`,
the named slots plus the triggering store, so a conversion resizes no
table. The table holds str keys, which the `WITH_HINT` handlers take;
`_PyDict_NewPresized` gives the general kind above five entries
(boil/design/work/6d-1.md, section G).
Whether a read of `__dict__` must convert at all stays note 09's open
re-shaping question. `convert_to_dict` was 9.0% of logging_simple's
instructions and `dictresize` 3% within it; logging_simple and
logging_format record the change.

### 6.4 The capacity (item 4)

Rule: `capacity = min(30, (good(B + 8w) - B) / 8)`, where `B` is the
type's `tp_basicsize`, `w` is `S + 2` if `S > 0` and 6 if `S` is 0, and
`good(n)` is `_PyMem_SizeClass(n)`, the block mimalloc gives a request
of `n` bytes on a release build, a pure function of `n`, so the capacity
depends on neither `PYTHONMALLOC` nor the build. `mi_good_size()` would
count the padding of debug builds (boil/design/work/6d-1.md, section
D). For a plain class (`B` = 8):

| S | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 8 | 10 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|
| capacity today | 6 | 6 | 6 | 6 | 6 | 7 | 8 | 10 | 12 | 16 |
| block today (bytes) | 64 | 64 | 64 | 64 | 64 | 64 | 80 | 96 | 112 | 144, in 160 |
| capacity 6d | 7 | 3 | 5 | 5 | 7 | 7 | 9 | 11 | 13 | 19 |
| block 6d | 64 | 32 | 48 | 48 | 64 | 64 | 80 | 96 | 112 | 160 |

The capacity is at least `S + 2` and its block never larger than
today's: smaller for S from 1 to 3, with a slot or more of spare for S
of 0, 4, 6 and above at no cost. The floor of six stays for classes with
no static attributes, which get theirs from outside their methods.

The measurement: a per-type sample of every live object with inline
slots, every 200,000 call events, over 21 kernels at their record loop
counts (`boil/tools/measure/occupancy_types.py`; the sums are
`capacity_rule.py` over its output). No sampled object holds more than
`S + 1` slots (`ModuleSpec`, go's `Square` and `UCTNode`,
`logging.Logger`, `re._parser.State`); S = 0 classes reach four (enum
members). Block bytes summed over the samples (a time integral, so the
ratio counts) fall from 111,068 KiB to 84,289 (-24.1%), or from 4,818 to
4,602 (-4.5%) without generators' tree, with no object over capacity;
`--spare 0` would put 13 `re._parser.State` samples into dictionary
mode. Predicted: generators' 100,000 tree nodes (S 3) from 64 to 48
bytes, 1.6 MB of its 66.5 MB peak, and raytrace's, chaos's and
deltablue's small classes likewise; instances_1m (S 4), the control,
unchanged. Measured at the rung: RSS of the memory group and kernels;
dictionary-mode entries by reason over the suite on a Py_STATS gate 2
(6c: `capacity` 26,542, `shape_limit` 3,148) and per kernel and type,
since sampling misses transient peaks; `sys.getsizeof` of instances,
whose tests change. The dataclass recomputation (20.3) uses the rule.

### 6.5 Members across a generalisation (item 5)

A member's offset and descriptor are facts of the class, so by the
pairing ruling (note 09 section 5) a member site guards `tp_version_tag`,
not the shape version that generalisation renews. To keep the guard at
two loads, `_PyShape` gains `class_version`, with the invariant
`shape->class_version == shape->type->tp_version_tag` for every shape,
zero included, independent of the shape's `type_version`. Its writers
are `type_version`'s today except generalisation: `_PyShape_InitRoot`
and `_PyShape_InitTypeShapes` from the type, `new_shape` from its
parent, `_PyShape_SetTypeVersion` (root, table and dictionary-mode
shape) whenever `set_version_unlocked` runs, and `_PyObject_Init` when it
fills an unreadied static type's root. A debug build asserts the
invariant in `_PyShape_SetTypeVersion` and the shape guards' slow paths.
`_GUARD_TYPE_VERSION` reads `_PyObject_SHAPE(o)->class_version` instead
of `Py_TYPE(o)->tp_version_tag`, saving a load in every handler that
guards the class version (`LOAD_ATTR_PROPERTY`,
`LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`,
`LOAD_ATTR_CLASS_WITH_METACLASS_CHECK`, `TO_BOOL_ALWAYS_TRUE`, the two
`POP_JUMP_IF_*_ALWAYS_TRUE`), and `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT`
move from `_GUARD_SHAPE_VERSION` to it, the specialiser recording
`tp_version_tag`. Tests: a class with `__slots__ = ('m', '__dict__')`
whose instances generalise in one subtree, and a member site that stays
`LOAD_ATTR_SLOT` across both (zero deoptimisations on a Py_STATS build);
and the same site after the member's descriptor is replaced, after
`__bases__` is assigned, after an instance goes to dictionary mode, and
with the type's versions exhausted (as test_type_cache's
`test_version_exhaustion` does), each giving the new answer and the
last never specialising.

## 7. Precise roots (10a)

10a's rung 2 marks call.c's vectorcall core, the `tp_call` path, the
prepend forms and the C call functions (10a-2.md); typeobject.c's
bridges wait for its rung 1b. The stamp rule fails a marked function the
C mode rejects, so a 6d change to one is checked in its commit.

| Function | 6d change | Class | Marked by |
|---|---|---|---|
| `_PyObject_MakeTpCall` | the slot's value is the result | precise; its span (callable, tuple, dict, keywords) unchanged | 10a rung 2 |
| `_PyObject_CallValue` (was `_PyObject_Call`), `PyObject_Call` | value form and wrapper | precise; the body keeps `_PyObject_Call`'s anchors; the wrapper boxes after the last use of its arguments | 10a rung 2; 6d moves the marks with the body |
| `_PyObject_Call_PrependValue`, `_PyObject_Call_Prepend` | value form and wrapper | precise, as above | the same |
| `PyVectorcall_Call`, `_PyVectorcall_Call`, `cfunction_call`, `wrapperdescr_call`, `classmethoddescr_call` | return values | precise | 10a rung 2 |
| `slot_tp_getattro`, `_Py_slot_tp_getattr_hook`, `call_attribute`, `slot_tp_setattro`, `slot_tp_descr_get`, `slot_tp_descr_set`, `slot_tp_call`, `call_method`, `vectorcall_method_value` | rewritten on values | `root_check_c.py --function` run on each and recorded in the work doc (work/6d-2.md, "C mode: hand-off to 10a"); left opaque. The spans over their parameters and the clean results are 10a's deliverable, with the marks | 10a rung 1b (open question 3) |
| `wrap_*`, `wrapper_call`, `wrapperdescr_raw_call` | return values | opaque (not in note 11's row) | 10b |
| object.c's executor, `PyObject_GetAttrValue`, `PyObject_SetAttrValue`, `PyObject_GetOptionalAttrValue` | value owner | opaque; their roots stay, since a box made for a descriptor call must outlive it | 10b |
| descriptor gets and sets, getters and setters, `PyMember_*Value`, `type_getattro`, `type_setattro`, module, super | value forms | opaque | 10b |
| `STORE_ATTR_SHAPE_POLY`, `LOAD_ATTR_METHOD_LAZY_DICT` | new handlers | generated, precise, checked by root_check | already |

`_PyValue_CheckSlotResult` is a static inline function that reads a type
and, on a non-canonical box, writes to stderr with stdio and calls
`abort()`; `Py_FatalError` is not used, since it can run Python code.
The C mode checks the function clean, but classes its stdio and `abort`
calls as collecting, so a precise caller sees it as a collecting call
until 10a adds them to call_effects.py's non-collecting list with a
reviewed comment.

**Files shared with 10a.** call.c, descrobject.c, methodobject.c,
classobject.c and typeobject.c are 10a's too. Rung 1 touches none of
them and starts at once. Rung 2 touches classobject.c, descrobject.c's
descriptor functions and typeobject.c; rung 3 call.c, descrobject.c's
call functions and typeobject.c. So rungs 2 to 4 start from the first
integration commit holding 10a's rung 2 (the `tp_call` path, d348fd4 on
boil/10a until it merges), which the coordinator names at the merge, and
rewrite typeobject.c's bridges before 10a's rung 1b, which owns their
marks and so marks the final bodies once.

## 8. What boxes remain

After 6d an inline value is boxed at these slot boundaries:

- **an inline receiver** of int's and float's C methods and getsets
  (2.4): open question 1;
- **the other slot families** (number, sequence, mapping, comparison,
  hash, iteration, init, new, repr) at the call and in their wrappers'
  arguments, and an inline instance passed to an explicit `__get__`,
  `__set__` or `__delete__` (2.4): 19.11's row, open question 2;
- **an inline value stored into a native `PyObject *` field** (4): a
  later per-type change to `_Py_T_VALUE`, when a field is measured;
- **the legacy `char *` slots** (2.5): the ports phase.

Unconverted third-party slots are an ABI mismatch (2.1), not boxes.
Allocations that remain: a classmethod's bound method and a `tp_call`'s
argument tuple (tier 1), and what a getter computes if rung 4 is cut.

Note 08's register: "object slots box" is retired with its proof (the
box audit's slot cases at zero, and the identity tests). New rows:
"inline receiver of int and float methods" (open question 1), "native
`PyObject *` fields box an inline value" (no owner until measured),
"`tp_getattr` and `tp_setattr` are object slots" (the ports phase), and,
if rung 4 is cut, "getset getters and setters are object functions",
for step 6e with the other slot families and their ABI revision,
value-v3 (open question 2).

## 9. Tests

- **Identity as the canonical-form test.** An inline int's identity is
  its encoding, so `self.assertIs(o.attr, 12345)` fails if a slot
  returned a box. test_descr and test_class gain, for each path of
  sections 3 to 5, results of int32 bounds, int32 + 1, -0.0, inf, NaN
  (boxed, so `assertIsNot` another NaN box), `True` and an int subclass
  instance, and stores of the same through every store path, read back
  by identity.
- **test_capi**: a heap type and a static type whose getattro, setattro,
  descr get and set, call and a getset return and take inline values,
  used from Python and through both forms of each function of 2.3 (the
  object forms return fresh boxes); a spec type naming the generic
  functions; and, on debug builds, two subprocesses. One installs,
  through a cast, a getattro returning a box of 5 and expects the fatal
  error naming the type and slot. The other installs a setter that saves
  its argument in a `PyObject *` field, stores 5, collects, and expects
  the tracer's new debug check to stop naming the owner's type:
  `_Py_VISIT_VALUE` filters inline values before it visits, so an inline
  encoding reaching `live_visit` can only be a value in an object field.
- **The ABI suffix**: test_sysconfig's suffix tests expect `-boil2-`,
  and a test extension copied under the `-boil-` suffix does not import.
- **Lifetimes.** Identity proves a canonical result, not that a box, a
  fresh heap result or an argument array survives a collection. The slot
  tests run in debug subprocesses under `PYTHON_GCSTRESS=1` and kill mode
  (`PYTHON_GCVERIFY=kill`), with these cases: C methods reading an
  uncached inline receiver (`(100000).bit_length()`, `(2.5).hex()`) with
  a collection injected at a new site, `_PyGC_SITE_SLOT_BOX`, after a
  boundary box is anchored and before the call that reads it
  (`gc_collect_at`, its count asserted); a `_testinternalcapi` fixture
  whose span is a box's only root, holding it raw or, as the negative
  control, canonical, where kill mode passes the first and traps the
  second; a property returning a fresh `2**40` or str at a
  `CALL_FUNCTION_EX` site whose `sys.monitoring` callback collects; a
  `__get__` that deletes itself from its class, and a getter that
  replaces its property's `fget`, each collecting before returning; a
  callable instance called with more positional arguments than
  `_PY_FASTCALL_SMALL_STACK` and with `**kwargs`; and allocation failure
  (`_testcapi.set_nomemory`) at the box of each object form of 2.3 and of
  `wrap_descr_get`, checking the `MemoryError` and the unwound roots
  (`_PyThreadState_PopValueRoot`'s assertion).
- **The box audit** (`box_audit.py`) takes `slot_boxes.py`'s slot cases,
  which must make no box: the hack row's proof. The inline-receiver and
  explicit-descriptor cases are listed as residue with their reason.
- **test_shapes and test_opcache**: `STORE_ATTR_SHAPE_POLY` over two to
  four shapes with REPLACE, ADD and mixed rows, a kind misfit (exits), a
  fifth shape (generic), one row's version renewed; the lazy-dict
  handler on an exception, a function and an int subclass, then with
  the dict made (exits); the shape handlers on a no-storage type; 6.5's
  tests; capacities by S, `B` other than 8, the dataclass recomputation;
  a conversion with no resize.
- **Gates**: each rung's gate 1 runs its ladder (section 11); gate 3 runs
  the debug suite, test_cext, regen, the traverse audit and the lifetime
  set, with `make clinic` leaving the tree unchanged after rung 4.

## 10. Measurements

**Before**, on 6086cccc9c2 (Py_STATS build), boxes and allocations per
iteration of 1,000, from `boil/tools/measure/slot_boxes.py`:

| Case | boxes | allocations | where |
|---|---|---|---|
| property through `getattr()` | 1 | 0 (small int) / 1 (float) | `PyObject_CallOneArg` |
| property store; `__setattr__` store | 1 | 1 | the store's root |
| `__getattribute__` through `getattr()`; `__getattr__` at a site | 1 | 0 | `_PyObject_VectorcallTstate` |
| `__get__`, data `__get__`, `__set__` | 1 each | | vectorcall, the store's root |
| callable instance | 1 | 2 | `_PyObject_Call_Prepend` |
| namedtuple field, structseq field, `slice.start` | 1 each | | `descr_get_value` |
| `complex.real`, `datetime.year` | 0 | 1 each | the getter's object, canonicalised away |
| `float.real`, `(7).bit_length()` | 1, 2 | | inline receiver (2.4) |
| class attribute store | 2 | 2 | value and old value |
| `object.__setattr__` | 1 | 3 | `PyArg_UnpackTuple` |
| specialised property and `__getattribute__` sites, `__slots__` members, staticmethod | 0 | 0 | |
| classmethod call, a bound method taken as a value | 0 | 1 each | binding |

The kernels' boxes (chaos 155,290 per loop at 6c, regex_compile
986,325) come from range arguments, `array`, subscript stores and
chaos's inline receivers, none from a slot call, so the micro set
carries the slot boundary. It gains a row per case above (int and float
where the value matters) and `method_lazy_dict` (6.2); the `boxes` metric
also records `Object allocations`, which sees transient number objects.

**Recorded at gate 4** (label `6d`), against `6c.json` and
`6c-pystats.json`: boxes and allocations per loop over the kernels and
micro set; instructions per loop with attribution per rung (go and
`attr_store_poly` for 6.1, logging for 6.3, `slots_load` and
`property_load` for 6.5, the slot rows for rungs 2 to 4); peak RSS of the
memory group and kernels (generators, raytrace, chaos, deltablue for
6.4); the occupancy and dictionary-mode counts of 6.4; go's generic
`STORE_ATTR` executions per loop (441,162 at 6c) and its POLY store
sites. Expected, without a target: go's generic store path, about 10% of
its instructions (note 06 20.9; +24.2% is the whole regression), mostly
gone; every slot row at zero boxes; logging a few percent down;
generators about 1.6 MB lower.

## 11. Rungs

Two days, hard stop at the end of day three. Each rung has a reviewed
work doc (`boil/design/work/6d-<n>.md`) and ends at gate level 3.

| Rung | Days | Work | Ladder (smallest first) |
|---|---|---|---|
| 1 | 0.5 | The hand-offs: `class_version` and the member guard (6.5); presize (6.3); the capacity rule (6.4); the `_NO_DICT` fold, `STORE_ATTR_SHAPE_POLY` and `LOAD_ATTR_METHOD_LAZY_DICT` (6.1, 6.2), magic bump; the `SOABI` mark and abi_id (2.1); the micro rows of section 10 with their before values. Touches no file of 10a's | `python -c pass`; a two-shape store site in a script; the suffix test; test_sysconfig, test_shapes, test_opcache, test_dis; test_class, test_descr, test_sys, test_dataclasses, test_capi; both suites; the Py_STATS suite for 6.4's counts |
| 2 | 0.75 | Attribute and descriptor slots: the four typedefs; the generic functions in place; the executor on a value owner; `PyObject_GetOptionalAttrValue`; the hooks and `slot_tp_descr_*`; type, module, super, `method_getattro`, `instancemethod_getattro` and the custom slots, with 2.1's `PyType_Slot` list; the descriptors of section 4 and `PyMember_*Value` with the native-field boxes; `getset_get` and `getset_set` canonicalising and boxing around the object-form getters and setters; both debug checks and `_PyGC_SITE_SLOT_BOX`; docs (typeobj.rst, object.rst). `wrapperfunc` stays an object form, since its consumers are `tp_call` functions: `wrap_getattro` (new) and `wrap_descr_get` box their slot's value at return, a box rung 3 removes; `wrap_setattr`, `wrap_delattr`, `wrap_descr_set` and `wrap_descr_delete` read the tuple's items as values | one property and one `__getattribute__` case by identity; test_descr, test_class, test_property, test_module, test_super, test_structseq, test_collections, test_capi; test_pickle, test_decimal, test_ctypes, test_typing, test_dataclasses; the attribute lifetime cases under stress; box audit; both suites |
| 3 | 0.5 | Calls and wrappers, one dependency: `callfunc`; `wrapperfunc` and `wrapperfunc_kwds` with every `wrap_*` (rung 2's boxes go), `wrapperdescr_raw_call`, `wrapperdescr_call`, `wrapper_call`; `slot_tp_call`; the call layer of section 5 with the C mode clean; `CALL_FUNCTION_EX`; the module types' `tp_call`; the wasm trampolines | a callable instance and `object.__getattribute__` by identity; the C mode on call.c and descrobject.c; test_call, test_descr, test_functools, test_capi, test_ctypes, test_asyncio, test_sqlite3, test_monitoring; the call lifetime cases under stress and kill mode; box audit; both suites |
| 4 | 0.25 | Getsets: `getter` and `setter`, Clinic's `@getter` and `@setter`, the 388 entries (mechanical, by subagent, reviewed and committed by the step agent; number-returning getters return values; setters read values with the value accessors or box at their top, or at a kept field as section 4 says); `PyObject_GenericGetDict` and `PyObject_GenericSetDict` (2.2); rung 2's getset conversions go; docs (structures.rst); gate 4 and the write-up (note 06 section 21, note 08's register, note 09's handler table) | a `complex.real`-style getset by identity; test_clinic; test_capi, test_datetime, test_memoryview, test_re, test_io, test_descr; `make clinic` clean; both suites; gate 4 |

**Each rung's tree** builds and passes level 3 alone: each changes its
typedefs with every function of their types, and is pushed whole.

**Scope cut**, in order, if a rung passes its deadline by more than half
a day: rung 4's conversion (getsets stay object functions, with section
8's register row; gate 4 and the write-up stay); then
`LOAD_ATTR_METHOD_LAZY_DICT` (its number stays in the reserve). Never
cut: the five slot types, the compatibility layer, the debug checks, the
suffix mark, the store poly handler, the lifetime cases, and the C mode
clean on every marked function.

**Hard stop.** Commit what passes level 3, write the hand-off list in
note 06 section 21, report.

## 12. Open questions

1. **Inline receivers of int and float.** Their C methods and getsets
   box an inline `x` for `x.bit_length()`, `x.is_integer()`, `x.real`
   (chaos: 20,000 per loop). A closed fix: a type flag saying int's and
   float's method descriptors and getsets take `PyValue self`, and
   Clinic's `self: value` converter for their roughly 50 entries.
   Recommendation: a step after 10a, or tier 1's intrinsics for the hot
   ones, ruled with chaos's numbers.
2. **The remaining slot families on values**, with the wrappers'
   argument boxes. Recommendation: step 6e after 10a, scoped by a
   measurement of 19.11's slot-wrapper row; it would take rung 4 if cut,
   and the coordinator adds it to note 06's table at 6d's write-up.
3. **Order with 10a.** Section 7's order: rung 1 at once; rungs 2 to 4
   from the integration commit holding 10a's rung 2; 10a's rung 1b marks
   the bridges 6d rewrites. Confirm, or move the marks to 6d.
4. **Specialised handlers past 255.** Recommendation: a cleanup step
   before tier 0 (step 8), which will want handlers of its own.
5. **Capacity acceptance.** The rule lands as written, untuned. Recorded:
   the suite's dictionary-mode entries by reason (6c: `capacity` 26,542)
   and `capacity_rule.py`'s over-capacity objects per kernel, type and S.
   A coordinator decision, not an automatic rule: if the suite's
   `capacity` entries on the same gate more than double 6c's, the
   alternative is six slots for S from 1 to 3.
6. **`tp_getattr` and `tp_setattr`.** Recommendation: object slots until
   the ports phase decides.

## Review

Round 1 (codex, verdict revise), each blocking issue a change:
`slot_tp_descr_get` calls `__get__` unbound (4); native `PyObject *`
fields box, with member semantics (4, 8); explicit wrapper calls box
their pointer arguments (2.4); the claim that every mismatch is loud is
withdrawn for the `PyType_Slot` audit, the suffix mark and the tracer's
check (2.1, 9); `wrapperfunc` moves to rung 3 with `tp_call` (11);
stress, kill-mode and allocation-failure cases (9). The non-blocking
points were taken (6.5's writers, exact signatures, the probes
committed, go's store cost at about 10%).

Round 2 (boil/design/reviews/12-object-slots-on-values-review.md, codex),
verdict revise; the last round, each blocking issue a change:

1. *Wrappers' other pointer arguments.* Every inline argument bound for
   a `PyObject *` parameter is boxed and anchored raw, validated after,
   with `__get__(None, 3)` and numeric names tested (2.4).
2. *The dict accessors are getters.* `PyObject_GenericGetDict` and
   `PyObject_GenericSetDict` change in rung 4 (2.2).
3. *ABI isolation started late.* The mark moves to rung 1, whose
   `class_version` grows `PyTypeObject`. Rebutted in part: the mark
   versions the step, since no binary is built against an intermediate
   rung (2.1).
4. *Receiver tests did not force a collection.* Uncached receivers, an
   injection site with its count asserted, and a kill-mode fixture with a
   canonical-root negative control (9).

Non-blocking, taken: `getattr` with a default and `hasattr` (2.3), the
suffix's limit (2.1), capacity conversions per kernel (6.4), trampolines
per rung (5). The questions are open questions 2, 3 and 5.
