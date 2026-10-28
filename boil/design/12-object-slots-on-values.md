# 12: The object slots on values

Status: design, 2026-10-27, for step 6d (note 06 section 6). It decides
the value signatures of `tp_getattro`, `tp_setattro`, `tp_descr_get`,
`tp_descr_set`, `tp_call` and the native descriptor calls, the
compatibility layer that `PyObject *` callers use, and the five 6d items
of note 06 section 20.9. It retires note 08's "object slots box" row. The
note 09 review named 6d as the boundary where 6c leaves boxing; this is
the design of that boundary.

## Decisions

1. **The slot types change in place.** `getattrofunc`, `setattrofunc`,
   `descrgetfunc`, `descrsetfunc`, the new `callfunc` of `tp_call`, and
   the getset `getter` and `setter` return or take `PyValue` where they
   return or take an attribute value. There is no second slot and no
   object-form slot beside each value slot, as 6b did for `tp_iternext`,
   `METH_O`, `METH_FASTCALL` and `vectorcallfunc` (section 2.1).
2. **`self` stays `PyObject *` in every slot**, and so do names, type
   arguments, argument tuples and keyword dicts. Only exact int and float
   are ever inline, and their types fill none of these slots; an inline
   receiver reaches a C function only through int's and float's own
   methods and getsets, a closed set (section 2.4).
3. **The public functions that are slots change with them.**
   `PyObject_GenericGetAttr`, `PyObject_GenericSetAttr` and
   `PyVectorcall_Call` have the slot's type, so the 146 initialisers that
   name them stay as written and every custom slot that falls back to
   them keeps its tail call (section 2.2).
4. **The compatibility layer is the protocol functions**, one per slot:
   `PyObject_GetAttr`, `PyObject_SetAttr`, `PyObject_GetOptionalAttr`,
   `PyObject_Call`, `PyMember_GetOne` and `PyMember_SetOne` canonicalise
   their arguments, call the value form and box the result. Nothing in
   the core calls them on a path that can see an inline value (2.3).
5. **A debug build checks every value slot's result for canonical form**
   at the dispatch site, so an unconverted extension slot that returns a
   box of a small int or a float stops with the type and slot named
   (2.1).
6. **The dunder bridges speak values**: the `slot_*` functions that call
   Python methods use `vectorcall_method_value`, and `wrapperfunc`
   returns `PyValue`, so `object.__getattribute__`,
   `object.__setattr__`, `__get__`, `__set__` and `__call__` move values
   between Python and C without a box (sections 3 and 5).
7. **Descriptors on values**: property, Python descriptors, members
   (`PyMember_GetOneValue`), getsets, `_tuplegetter`, functions,
   classmethod and staticmethod return values; what still boxes is listed
   with owners (sections 4 and 8).
8. **Polymorphic stores get `STORE_ATTR_SHAPE_POLY`**, with REPLACE and
   ADD rows in the existing poly table. Its opcode, and one for
   `LOAD_ATTR_METHOD_LAZY_DICT`, come from folding
   `LOAD_ATTR_METHOD_NO_DICT` and `LOAD_ATTR_NONDESCRIPTOR_NO_DICT` into
   the shape handlers: on a type with one shape the shape guard is the
   type guard, with one load fewer (section 6.1).
9. **Capacity fills its block**: `S + 2` slots (six when `S` is 0),
   rounded up to what the allocator's size class holds. No class gets a
   larger block than today; classes with one to three static attributes
   get a smaller one (section 6.4, measured).
10. **Members guard the class version**, read through a mirror of
    `tp_version_tag` in every shape, so a generalisation elsewhere in the
    type does not make a member site alternate (section 6.5). The
    dictionary conversion presizes (6.3).
11. **Precise roots**: every changed function in 10a's call layer stays
    precise and passes the C mode in the commit that changes it; the
    typeobject.c bridges are written to the C mode's subset and left
    opaque for 10a rung 1b to mark; the rest stays opaque (section 7).
12. **Four rungs, two days, hard stop at three**; the hand-offs first,
    getsets last and first to be cut (section 11).

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
`tp_iternext` and vectorcall. `args` of `tp_call` is a tuple, whose items
are already values (6a), so the call layer passes an inline argument
through a tuple without a box; only the result changed.

### 2.1 In place, not beside

A value slot beside each object slot (`tp_getattrov` and the rest) would
be two mechanisms for one job, with an inheritance rule for the pair, a
`slotdefs` entry for each, and a choice of winner when a type fills both.
6b changed four calling conventions in place, patched the tree, and froze
the ports for phase P; this step does the same, for the same reason: one
form per slot, and the port patches written once against the settled API.

What an unconverted C type meets:

- **A static initialiser** naming an object-form function is a compile
  error in C: gcc 14 and later reject incompatible pointer types by
  default, and the gated compiler is gcc 15. A C++ extension fails too.
- **A cast or a `PyType_Slot` entry** (`void *`) compiles silently. The
  value ABI makes that loud rather than corrupt in every case. A heap
  object's value is its pointer, so an object-form getter, getattro,
  `__get__` or call slot returns a valid value unless it returns an int
  that fits in int32 or a non-NaN float. That is a non-canonical box,
  which the debug check of decision 5 catches at the dispatch site
  (`_PyValue_CheckSlotResult(v, type, "tp_getattro")`, a fatal error
  naming both). An object-form setter, setattro or `__set__` handed an
  inline value dereferences a non-canonical address (top bits 0xFFFE for
  an int, 0x0002 to 0xFFF2 for a float) and faults at once.
- **A binary built for value-v1** has the same suffix; the ports SDK's
  abi_id becomes `cpython-3.16-value-v2`, and the frozen ports stay at
  value-v1 until phase P (note 10).

The tree's casts are removed as part of each conversion, so the compiler
checks every in-tree slot: 101 `(getter)` and 99 `(setter)` casts, and 9
casts to the five slot types. The slot functions in the tree are about 20
getattro, 15 setattro, 20 descriptor get, 6 descriptor set and 40 call
implementations, besides the generic ones, and 388 getset entries in 126
tables, 135 of them Argument Clinic's `@getter` and `@setter`.

### 2.2 The generic functions are slots

`PyObject_GenericGetAttr` is documented as the function to put in
`tp_getattro`; 146 initialisers and spec entries in the tree name it. Its
18 direct calls in the tree, with `PyObject_GenericSetAttr`'s, are all a
custom getattro's or setattro's fallback, `return PyObject_GenericGetAttr(self, name);` (`_pickle`,
`_decimal`, `genericalias`, `union`, `super`, the xx modules). Changing
it with the slot keeps both uses correct once the custom slot is
converted, and the slot-identity tests (`tp_getattro ==
PyObject_GenericGetAttr`, ten of them in the core and the specialiser)
keep their meaning. The same holds for `PyObject_GenericSetAttr`
(`value` becomes `PyValue`) and for `PyVectorcall_Call`, the `tp_call`
of every vectorcall type. Note 06 section 3.5's rule that "functions
that take `PyObject *` keep taking it" is about the API's functions; these
three are slot implementations first, and C code that wants an object
calls the protocol function instead.

`_PyObject_GenericGetAttrWithDict` and `_PyObject_GenericSetAttrWithDict`,
the exported internal entries that take a dict, change with them.

### 2.3 The compatibility layer

| Protocol, object form | Value form it wraps | In | Out |
|---|---|---|---|
| `PyObject_GetAttr(o, name)` | `PyObject_GetAttrValue` (6b) | `PyValue_FromObject(o)` | box |
| `PyObject_SetAttr(o, name, v)` | `PyObject_SetAttrValue` (6b) | canonicalise `v` | int |
| `PyObject_GetOptionalAttr(o, name, &res)` | `PyObject_GetOptionalAttrValue` (new; 19.11's gap) | as above | box into `*res` |
| `PyObject_Call(c, args, kw)` | `PyObject_CallValue` (new) | none | box |
| `_PyObject_Call_Prepend` | `_PyObject_Call_PrependValue` (new) | none | box |
| `PyMember_GetOne(addr, m)` | `PyMember_GetOneValue` (new) | none | box |
| `PyMember_SetOne(addr, m, v)` | `PyMember_SetOneValue` (new) | canonicalise `v` | int |

Each object form is a static three-line wrapper over its value form, in
the file that defines the value form, as 6b's layer is (note 06 19.3).
There is no public function that calls `tp_descr_get` or `tp_descr_set`;
outside object.c and typeobject.c, the two sites that call the slot
directly (`legacy_tracing.c`, `_lsprof.c`) take the value and box it
where they need an object.
`PyObject_GenericGetDict` and the `__dict__` paths are unchanged.

### 2.4 `self` stays an object

6b kept `self` as `PyObject *` for `METH_*` functions. 6d keeps it for
every slot, because no slot implementation outside int and float can
receive an inline value as its receiver:

- `tp_getattro`, `tp_setattro`: int and float have the generic ones,
  which the value executor runs on the value itself (section 3), so the
  slot function is never called with an inline receiver.
- `tp_call`: int and float are not callable. `_PyObject_MakeTpCall` names
  the type in its error from `PyValue_Type(callable)` instead of boxing.
- `tp_descr_get(descr, obj, type)`: `descr` comes from a class dict and
  has a `tp_descr_get`, so it is never an int or float. `obj` is inline
  only when the attribute was found on `int`, `float` or `object`, whose
  dicts are immutable and hold only C descriptors: method, getset,
  wrapper and classmethod descriptors.
- `tp_descr_set(descr, obj, value)`: `obj` is inline only for a store to
  an int or float, which these same descriptors refuse.
- `getter`, `setter`: the same closed set (`int.real`, `float.imag`,
  `numerator`, `__class__`).

So the inline-receiver box is confined to the C methods and getsets of
int and float: the executor boxes once at the descriptor call, held by a
value root. It is measured (chaos makes 20,000 per loop, two per
`n.bit_length()` in `random._randbelow`) and is open question 1.

### 2.5 `tp_getattr` and `tp_setattr`

The `char *` slots stay object slots, called only from the getattr and
setattr dispatch, which canonicalises the result and boxes the stored
value there, as today. In the tree only the three example modules
(`xx`, `xxlimited_35`, `_testmultiphase`) fill `tp_setattr`, and nothing
fills `tp_getattr`. They are listed in note 08's register with
the ports phase as owner, which decides whether the legacy slots survive
at all.

## 3. The attribute protocol on values

**Dispatch.** `PyObject_GetAttrValue(PyValue v, name)` reads
`PyValue_Type(v)->tp_getattro`. If it is `PyObject_GenericGetAttr`, it
runs the plan executor on the value; otherwise it calls the slot on the
object (`v` is then never inline, by 2.4). Today's three-way dispatch
(type, generic, other with a canonicalising fallback) and the box of an
inline receiver before dispatch go; `type_getattro` is an ordinary value
slot. `PyObject_SetAttrValue` is the same with `tp_setattro`, and passes
the value through.

**The executor takes the owner as a value.**
`generic_getattr_value(PyValue owner, name, dict, suppress)` reads the
owner's shape with `_PyValue_Shape()` (note 09 section 3). SLOT and DICT
plans need an object and are impossible on an inline owner's shape;
CLASS_VALUE and ABSENT need nothing of the owner; METHOD, DESCR,
DATA_DESCR and PROPERTY box an inline owner once, at the descriptor call
(2.4). `generic_setattr_value` takes the owner as an object, since an
inline owner has no storage and only the NO_STORAGE and DATA_DESCR
outcomes, which box for the error or the refusing descriptor.
`PyObject_GenericGetAttr(obj, name)` is the slot entry: it calls the
executor with `PyValue_FromObject(obj)`, whose cost for an object is the
type compare that `PyValue_FromObject` does.

**Hooks.** `_Py_slot_tp_getattro` calls `__getattribute__` through
`vectorcall_method_value`. `_Py_slot_tp_getattr_hook` runs the executor
with `suppress`, then calls `__getattr__` through `call_attribute`, whose
value form uses `PyObject_VectorcallValue` and the value `tp_descr_get`.
`slot_tp_setattro` passes `[self, name, value]` to `__setattr__` or
`[self, name]` to `__delattr__` as values. None boxes.

**Wrappers.** `wrapperfunc` returns `PyValue`, so each `wrap_*` function
returns its slot's result unboxed: `wrap_getattro` (new; today
`__getattribute__` goes through `wrap_binaryfunc`, which casts the slot to
`binaryfunc`), `wrap_setattr` and `wrap_delattr` read the value from the
argument tuple's items, which are values, instead of
`PyArg_UnpackTuple`, which boxes. The other wrappers return
`PyValue_FromResult` of their object result; `wrap_next` returns the
iterator's value, which retires 19.11's `wrap_next` row.
`wrapperdescr_raw_call`, `wrapperdescr_call` and `wrapper_call` return
the value. So `object.__getattribute__(self, n)` inside a Python
`__getattribute__`, and `object.__setattr__(self, n, v)` in a frozen
dataclass's `__init__`, move values without a box.

**Types, modules, super.** `type_getattro` and `type_setattro` read and
write the class dict by value (today a class attribute store boxes the
new value and the old one it reads, two boxes per `C.x = i`).
`module_getattro`, `super_getattro`, `proxy_getattr` and the
`_ctypes`, `_pickle` and `_decimal` slots convert; each returns
`PyValue_FromResult` of what it builds, or passes a value it read.

## 4. Descriptors on values

| Descriptor | `__get__` | `__set__` | Remaining box or allocation |
|---|---|---|---|
| property | `PyObject_VectorcallValue(fget, &obj, 1)` | `fset` with `[obj, value]` | none |
| Python descriptor (`slot_tp_descr_get`, `_set`) | `__get__` through `vectorcall_method_value` with `[self, obj, type]`, `None` for absent | `__set__` with `[self, obj, value]` | none |
| member, `__slots__` (MEMBER plan) | the slot load, as in 6c | the slot store | none |
| member, other (`member_get`) | `PyMember_GetOneValue`: `_Py_T_VALUE`/`_EX` return the field; numeric members return `PyValue_FromLongLong`/`FromDouble` | `PyMember_SetOneValue` reads numbers with the value accessors | none for numbers; strings and `char *` members allocate as now |
| getset (`getset_get`) | the getter's value | the setter takes the value | what each getter computes (rung 4 converts the number-returning ones to direct values) |
| `_tuplegetter` (namedtuple field) | the tuple item, a value | refuses | none |
| structseq field | member `_Py_T_VALUE` | refuses | none |
| function (`func_descr_get`) | a bound method, an object | | the bound method (value form only; call-pair forms do not bind) |
| method descriptor (`method_get`) | a builtin method, an object | | an inline `self` box (2.4) |
| classmethod (`cm_descr_get`, `classmethod_get`) | a bound method of the type | | the bound method, for both forms |
| staticmethod (`sm_descr_get`) | the callable | | none |
| wrapper descriptor | a method-wrapper | | an inline `self` box (2.4) |

Notes:

- Plans do not change. The executor calls the value `tp_descr_get` of the
  descriptor's type for METHOD, DESCR and DATA_DESCR without MEMBER, as
  note 09 section 4.4 says, and `descr_get_value`'s canonicalisation goes:
  the slot returns the value.
- `LOAD_ATTR_PROPERTY` keeps pushing the getter's frame. The executor's
  PROPERTY path is the generic one (megamorphic sites, `getattr()`, C
  callers); today it boxes the getter's result in `PyObject_CallOneArg`.
- A classmethod's bound method is an allocation per call through the
  generic call-pair form (one per `C.cm(x)` in the probe of section 10).
  Binding a classmethod in the call-pair form as (callable, type) is a
  plan-executor change of its own and is left for tier 1, which inlines
  through it (section 8).
- `PyDescr_NewGetSet`, `PyMemberDef` and `PyGetSetDef` keep their layouts;
  only the function types change.

## 5. Calls on values

- `tp_call` is `callfunc`. `_PyObject_MakeTpCall` returns the slot's
  value after `_Py_CheckFunctionResultValue`, with no canonicalisation.
- `PyObject_CallValue(callable, args, kwargs)` is the value form of
  `PyObject_Call` (tuple and dict); `_PyObject_Call` becomes
  `_PyObject_CallValue` with `PyObject_Call` its wrapper. The eval loop's
  `CALL_FUNCTION_EX` calls the value form.
- `slot_tp_call` calls `__call__` through `_PyObject_Call_PrependValue`,
  which passes the tuple's items, already values, after `self` with
  `_PyObject_VectorcallPrependValue`. A callable instance's result is no
  longer boxed; the argument tuple the tp_call protocol builds remains.
- `cfunction_call`, `wrapperdescr_call`, `classmethoddescr_call`,
  `wrapper_call`, `type_call`, `sm_call`, `instancemethod_call` and the
  module types' `tp_call` (`partial`, `lru_cache`, `itemgetter`,
  `attrgetter`, `methodcaller`, `ctypes`' function pointers, `_asyncio`'s
  step wrapper, `sqlite3`'s connection) return values;
  `_PyCFunctionWithKeywords_TrampolineCall` gains its value twin for the
  wasm trampoline header, used only by `tp_call` dispatch.

## 6. The 6c hand-offs

### 6.1 Polymorphic stores (20.9 item 1)

**Handler.** `STORE_ATTR_SHAPE_POLY` = `unused/1 + _GUARD_SHAPE_POLY +
_STORE_ATTR_SHAPE_POLY + POP_TOP`. The guard is the load's guard uop,
reused: it finds the row whose shape is the owner's and whose version
matches. The store reads the row: `slot_kind[row]` is checked with
`_PyValue_FitsKind` (exit on a misfit, as `_GUARD_SLOT_KIND` does, so the
generic store generalises); the value goes to `index[row]`; if
`handler[row]` is `STORE_ATTR_SHAPE_TRANSITION`, `descr[row]` holds
`shape_to` and the header is written after the slot, with release order.
The poly table already records each declined row (20.3), so its layout
does not change. Rows are REPLACE or ADD: a store in a base class's
`__init__` adds the same name to each subclass's shapes, which are
distinct because shapes belong to their type, so ADD rows are the common
polymorphic case besides go's REPLACE rows. A site with more than four
shapes stays generic.

**Opcodes.** The specialised numbers are full; 125 to 127 are the three
left below RESUME. 6d frees two and spends two, so the reserve stays at
three:

- `LOAD_ATTR_METHOD_NO_DICT` and `LOAD_ATTR_NONDESCRIPTOR_NO_DICT` are
  deleted. Their types have instances with no attribute storage, so
  every instance carries the type's root shape (note 09 section 2), and
  `_GUARD_SHAPE` against the root plus `_GUARD_SHAPE_VERSION` proves what
  `_GUARD_TYPE_VERSION` proved. The root shape's version is the type's,
  since a root has no field to generalise. The two loads replace three.
  The specialiser gives METHOD and CLASS_VALUE on such types the
  `_SHAPE` handlers.
- One number goes to `STORE_ATTR_SHAPE_POLY`, one to
  `LOAD_ATTR_METHOD_LAZY_DICT` (6.2).

The handler set changes, so the pyc magic is bumped in the same commit,
and test_opcache, test_dis and test_shapes change their handler names.
The alternative, numbering specialised handlers past 255 as the IR-only
handlers are (the handler field is 16 bits; `_PyOpcode_Deopt` and `dis`
assume 256), would end the scarcity but is a change to the IR metadata,
not to the object protocol. It is open question 4.

### 6.2 The lazy-dict method path (item 3)

A type whose instances keep a dict word (`tp_dictoffset` nonzero, the
computed negative offsets included: exceptions, functions, int and tuple
subclasses, C types with a dict) has only its root shape, and a method
lookup on it is a DICT plan with METHOD as fallback. When the observed
instance's dict word is NULL, the specialiser picks
`LOAD_ATTR_METHOD_LAZY_DICT` = `_GUARD_SHAPE + _GUARD_SHAPE_VERSION +
_CHECK_DICT_WORD_NULL + _LOAD_ATTR_METHOD_SHAPE`. The check reads the
word at the entry's `dictoffset` (the union field the `WITH_HINT`
handlers use) through `_PyObject_DictAt` and exits if it is not NULL.
No benchmark measures it; a micro row does (section 10).

### 6.3 The dictionary conversion (item 2)

`dict_from_slots` allocates with `_PyDict_NewPresized(nslots + 1)`, the
named slots plus the triggering store, so a conversion resizes no table.
Whether a read of `__dict__` must convert at all stays note 09's open
re-shaping question. logging_simple and logging_format record the change;
`convert_to_dict` was 9.0% of logging_simple and `dictresize` 3% within
it.

### 6.4 The capacity (item 4)

Rule: `capacity = min(30, (good(B + 8w) - B) / 8)`, where `B` is the
type's `tp_basicsize`, `w` is `S + 2` if `S > 0` and 6 if `S` is 0, and
`good(n)` is `mi_good_size(n)`, the block size mimalloc gives a request
of `n` bytes. It is a pure function of `n`, so the capacity does not
depend on `PYTHONMALLOC`. For a plain class (`B` = 8):

| S | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 8 | 10 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|
| capacity today | 6 | 6 | 6 | 6 | 6 | 7 | 8 | 10 | 12 | 16 |
| block today (bytes) | 64 | 64 | 64 | 64 | 64 | 64 | 80 | 96 | 112 | 144, in 160 |
| capacity 6d | 7 | 3 | 5 | 5 | 7 | 7 | 9 | 11 | 13 | 19 |
| block 6d | 64 | 32 | 48 | 48 | 64 | 64 | 80 | 96 | 112 | 160 |

The new capacity is at least `S + 2`, and its block is never larger than
today's; it is smaller for S from 1 to 3, and holds a slot or more of
spare for S of 0, 4, 6 and above, at no cost. The floor of six stays for
classes with no static attributes, which by definition get their
attributes from outside their methods.

The measurement it rests on: a per-type sample of every live object with
inline slots, every 200,000 call events over 21 kernels (scratch probe,
`occupancy.py`'s sampler with S, capacity and the shape's slot count per
object; kernels at their record loop counts). No sampled object holds
more than `S + 1` slots. The objects
that exceed S do so by one: `ModuleSpec` (S 8, 17,066 of 18,614 samples),
go's `Square` (S 10) and `UCTNode` (S 6), `logging.Logger`,
`re._parser.State`. Of the S = 0 classes, enum members reach four slots
and the rest none. Block bytes summed over the samples fall from 111,068 KiB to 84,288
(-24%; the sum is a time integral, so the ratio is what counts), or from
4,596 to 4,390 (-4.5%) without generators' tree, with no object over
capacity under the rule; a rule of `S + 0` would put 16 sampled objects
into dictionary mode. Predicted:
generators' 100,000 tree nodes (S 3) from 64 to 48 bytes, 1.6 MB of its
66.5 MB peak; raytrace's `Vector` and `Point`, chaos's `GVector` and
deltablue's `Strength` likewise; instances_1m (S 4) unchanged, its
control. To measure at the rung: the memory group and the kernels' RSS;
`occupancy.py`'s unused share; dictionary-mode entries by reason over the
suite on a Py_STATS gate 2, against 6c's `capacity` 26,542 and
`shape_limit` 3,148; and `sys.getsizeof` of instances, whose tests in
test_sys and test_shapes change. The dataclass recomputation (20.3) uses
the same function.

### 6.5 Members across a generalisation (item 5)

A member's offset and descriptor are facts of the class, so by the
pairing ruling (note 09 section 5) a member site guards `tp_version_tag`,
not the shape version that generalisation renews. To keep the guard at
two loads, `_PyShape` gains `class_version`, a copy of `tp_version_tag`
that `_PyShape_SetTypeVersion` writes into the root, the table and the
dictionary-mode shape whenever `set_version_unlocked` does, and that
`_PyObject_Init` copies when it fills an unreadied static type's root;
generalisation leaves it alone. `_GUARD_TYPE_VERSION` reads
`_PyObject_SHAPE(o)->class_version` instead of
`Py_TYPE(o)->tp_version_tag`, so every handler that guards the class
version saves a load (`LOAD_ATTR_PROPERTY`,
`LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`, `LOAD_ATTR_CLASS_WITH_METACLASS_CHECK`,
`TO_BOOL_ALWAYS_TRUE` and the two `POP_JUMP_IF_*_ALWAYS_TRUE`), and the
member handlers `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` move from
`_GUARD_SHAPE_VERSION` to `_GUARD_TYPE_VERSION`. The
specialiser records `tp_version_tag` for them. The test: a class with
`__slots__ = ('m', '__dict__')`, instances whose fields generalise in one
subtree, and a member site that stays `LOAD_ATTR_SLOT` across both (its
deoptimisation count is zero on a Py_STATS build).

## 7. Precise roots (10a)

Note 11 section 2 lists the call layer that 10a makes precise; 10a's
rung 2 has marked call.c's vectorcall core, the `tp_call` path and the
prepend forms, `cfunction_*`, `method_vectorcall_*`, `wrapperdescr_call`
and `classmethoddescr_call` (10a-2.md), and typeobject.c's `slot_*` and
`vectorcall_method` family wait for its rung 1b. The build's stamp rule
fails a `Py_GC_PRECISE` function that the C mode rejects, so a 6d change
to a marked function is checked in the commit that makes it.

| Function | 6d change | Class | Marked by |
|---|---|---|---|
| `_PyObject_MakeTpCall` | the slot's value is the result | precise; its span (callable, tuple, dict, keywords) unchanged | 10a rung 2, before 6d |
| `_PyObject_CallValue` (was `_PyObject_Call`), `PyObject_Call` | value form and its wrapper | precise; the body keeps `_PyObject_Call`'s anchors; the wrapper's box allocates after the last use of its arguments | 10a rung 2; 6d moves the marks with the body |
| `_PyObject_Call_PrependValue`, `_PyObject_Call_Prepend` | value form and wrapper | precise, as above | the same |
| `PyVectorcall_Call`, `_PyVectorcall_Call` | returns a value | precise | 10a rung 2 |
| `cfunction_call`, `wrapperdescr_call`, `classmethoddescr_call` | return values | precise | 10a rung 2 |
| `slot_tp_getattro`, `_Py_slot_tp_getattr_hook`, `call_attribute`, `slot_tp_setattro`, `slot_tp_descr_get`, `slot_tp_descr_set`, `slot_tp_call`, `call_method`, `vectorcall_method_value` | rewritten on values | written to the C mode's subset: a parameter used after a collecting call is covered by a span on entry, no address escapes; `root_check_c.py --function` run on each and its count recorded in the rung's work doc; left opaque | 10a rung 1b (open question 3) |
| `wrap_*`, `wrapper_call`, `wrapperdescr_raw_call` | return values | opaque (not in note 11's row) | 10b |
| object.c's executor, `PyObject_GetAttrValue`, `PyObject_SetAttrValue`, `PyObject_GetOptionalAttrValue` | value owner | opaque; their `_PyValueRoot`s stay, since a box made for a descriptor call must outlive the call | 10b |
| descriptor gets and sets, getters and setters, `PyMember_*Value`, `type_getattro`, `type_setattro`, module, super | value forms | opaque | 10b |
| `STORE_ATTR_SHAPE_POLY`, `LOAD_ATTR_METHOD_LAZY_DICT` | new handlers | generated, precise by the generator, checked by root_check | already (handlers) |

`_PyValue_CheckSlotResult` is a static inline function that reads a
type and calls nothing but `Py_FatalError`, which does not return and
joins the C mode's non-collecting list with a reviewed comment; the C
mode then classifies the check through its body, and the inline audit
accepts it in a precise caller.

**Files shared with 10a.** call.c, descrobject.c, methodobject.c,
classobject.c and typeobject.c are 10a's files too. 6d's rung 1 touches
none of them. Rung 3's call.c and descrobject.c edits start from the
integration branch after 10a's rung 2 has landed there; rungs 2 and 3
rewrite typeobject.c's bridges before 10a's rung 1b marks them, so that
1b marks the final bodies once. The coordinator rules on the order
(open question 3).

## 8. What boxes remain

After 6d, an inline value is boxed at these slot boundaries, each with
its owner:

- **an inline receiver** of int's and float's C methods and getsets
  (2.4): open question 1;
- **the legacy `char *` slots** (2.5): the ports phase;
- **the other slot families** (number, sequence, mapping, comparison,
  hash, iteration, init, new, repr) and the arguments of their wrappers:
  19.11's "slot wrappers" row, open question 2;
- **unconverted third-party slots** are not boxes but an ABI mismatch
  (2.1): the ports phase.

Allocations that remain at these boundaries, recorded, not boxes: a
classmethod's bound method per generic call (tier 1), the argument tuple
of a `tp_call` (a callable instance has no vectorcall; tier 1 calls
`__call__` directly), and what a getter computes when rung 4 is cut.

Note 08's register: "object slots box" is retired by 6d with its proof
(section 9: the box audit's slot cases at zero, and the identity tests).
New rows: "inline receiver of int and float methods" (owner per open
question 1), "`tp_getattr` and `tp_setattr` are object slots" (the ports
phase), and, if rung 4 is cut, "getset getters and setters are object
functions".

## 9. Tests

- **Identity as the canonical-form test.** An inline int's identity is
  its encoding, so `self.assertIs(o.attr, 12345)` fails if a slot
  returned a box. test_descr and test_class gain, for each path of
  sections 3 to 5 (property generic and specialised, Python `__get__`,
  `__getattribute__`, `__getattr__`, `object.__getattribute__`,
  member, `_tuplegetter`, structseq, callable instance,
  `CALL_FUNCTION_EX`), cases that return int32 bounds, int32 + 1, -0.0,
  inf, NaN (boxed, so `assertIsNot` against another NaN box), `True`
  and an int subclass instance, and that store the same through
  `__setattr__`, `__set__`, property setters, `object.__setattr__`,
  members and class attributes, then read them back by identity.
- **test_capi**: a `_testcapi` heap type and a static type whose
  getattro, setattro, descr get and set, call and a getset return and
  take inline values, exercised from Python and through
  `PyObject_GetAttr`, `PyObject_GetAttrValue`, `PyObject_Call`,
  `PyObject_CallValue`, `PyMember_GetOne` and `PyMember_GetOneValue`
  (the object forms return fresh boxes, the value forms inline values);
  a spec type with `{Py_tp_getattro, PyObject_GenericGetAttr}` and
  `{Py_tp_setattro, PyObject_GenericSetAttr}`; and, on debug builds, a
  subprocess that installs a getattro returning a box of 5 through a
  cast and expects the fatal error naming the type and slot.
- **The box audit** (`boil/tools/measure/box_audit.py`) gains the slot
  cases of section 10's probe, which must make no box: the hack row's
  proof. The inline-receiver cases are listed as residue with their
  reason.
- **test_shapes and test_opcache**: `STORE_ATTR_SHAPE_POLY` over two to
  four shapes, with REPLACE and ADD rows and mixed rows, a kind misfit in
  one row (it exits and the generic store generalises), a fifth shape
  (stays generic), version renewal of one row's type; the lazy-dict
  handler on an exception, a function and an int subclass, with the dict
  materialised later (it exits); METHOD and CLASS_VALUE on a no-storage
  type picking the shape handlers; the member site of 6.5; capacities
  by S per 6.4, including `B` other than 8 (a list subclass) and the
  dataclass recomputation; a conversion's dict needing no resize
  (`_testinternalcapi` dict size before and after).
- **Gates**: each rung's gate 1 runs its ladder (section 11); gate 3
  runs the debug suite, test_cext, regen and the traverse audit, with
  `make clinic` leaving the tree unchanged after rung 4's clinic change.

## 10. Measurements

**Before**, on this tree (6086cccc9c2, Py_STATS build), boxes and
allocations per iteration of 1,000, from a scratch probe in the manner of
`box_audit.py`:

| Case | boxes | allocations | where |
|---|---|---|---|
| property through `getattr()` | 1 | 0 (small int) / 1 (float) | `PyObject_CallOneArg` |
| property store | 1 | 1 | `generic_setattr_value`'s root |
| `__getattribute__` through `getattr()` | 1 | 0 | `_PyObject_VectorcallTstate` |
| `__getattr__` at a site | 1 | 0 | the same |
| `__setattr__` store | 1 | 1 | `PyObject_SetAttrValue` |
| `__get__`, data `__get__`, `__set__` | 1 each | | vectorcall, the store's root |
| callable instance | 1 | 2 | `_PyObject_Call_Prepend` |
| namedtuple field, structseq field, `slice.start` | 1 each | | `descr_get_value` |
| `complex.real`, `datetime.year` | 0 | 1 each | the getter's object, canonicalised away |
| `float.real`, `(7).bit_length()` | 1, 2 | | inline receiver (2.4) |
| class attribute store | 2 | 2 | value and old value |
| `object.__setattr__` | 1 | 3 | `PyArg_UnpackTuple` |
| specialised property and `__getattribute__` sites, `__slots__` members, staticmethod | 0 | 0 | |
| classmethod call, bound method | 0 | 1 each | binding |

The kernels' box counts (raytrace 110,104, chaos 155,290, nqueens
40,332, regex_compile 986,325 per loop at 6c) come from range
arguments, `array`, subscript stores and, in chaos, 20,000 inline
receivers; none is a slot call. So the kernels are the regression
record, and the micro set carries the slot boundary.

**The micro set** (note 10) gains rows that a later step can track:
`getattr_property`, `getattribute_hook`, `getattr_hook`,
`setattr_hook`, `descr_get_py`, `descr_set_py`, `property_store`,
`call_instance`, `namedtuple_field`, `member_double` (`complex.real`),
`object_setattr` and `class_attr_store`, each over int and float values
where the value matters, and `method_lazy_dict` for 6.2. The `boxes`
metric also records `Object allocations` per iteration, which the stats
file already has, so a transient number object shows where the box
counter cannot see it.

**Recorded at gate 4** (label `6d`), against `6c.json` and
`6c-pystats.json`: boxes and allocations per loop over the kernels and
the micro set; instructions per loop with attribution per rung (go and
`attr_store_poly` for 6.1, logging for 6.3, `slots_load` and
`property_load` for 6.5, the slot rows for rungs 2 to 4); peak RSS of the
memory group and kernels (generators, raytrace, chaos, deltablue for
6.4); the occupancy and dictionary-mode entries of 6.4; go's generic
`STORE_ATTR` executions per loop (441,162 at 6c) and the share of its
store sites that are POLY. Expected, without a target: go down by most
of its 24% store cost, every slot row at zero boxes, logging a few
percent down, generators about 1.6 MB lower.

## 11. Rungs

Two days, hard stop at the end of day three. Each rung has a reviewed
work doc (`boil/design/work/6d-<n>.md`) and ends at gate level 3.

| Rung | Days | Work | Ladder (smallest first) |
|---|---|---|---|
| 1 | 0.5 | The hand-offs: `class_version` and the member guard (6.5); presize (6.3); the capacity rule (6.4); the `_NO_DICT` fold, `STORE_ATTR_SHAPE_POLY` and `LOAD_ATTR_METHOD_LAZY_DICT` (6.1, 6.2), magic bump; the micro rows of section 10 committed with their before values | `python -c pass`; a two-shape store site in a script; test_shapes, test_opcache, test_dis; test_class, test_descr, test_sys, test_dataclasses, test_capi; both suites; the Py_STATS suite for 6.4's counts |
| 2 | 0.5 | Attribute slots: `getattrofunc`, `setattrofunc`, the generic functions in place, the executor on a value owner, `PyObject_GetOptionalAttrValue`, the hooks, `wrapperfunc` on values and `wrap_getattro`/`setattr`/`delattr`, type, module, super and the tree's custom slots, the debug check, docs (typeobj.rst, object.rst) | one property and one `__getattribute__` case by identity; test_descr, test_class, test_module, test_super, test_capi; test_pickle, test_decimal, test_ctypes, test_typing, test_dataclasses; box audit; both suites |
| 3 | 0.5 | Descriptors and calls: `descrgetfunc`, `descrsetfunc`, `callfunc`; property, function, classmethod, staticmethod, method and wrapper descriptors, members and `PyMember_*Value`, `_tuplegetter`, structseq; the `slot_tp_descr_*` and `slot_tp_call` bridges; the call layer of section 5 with the C mode clean; `CALL_FUNCTION_EX`; the module types' slots | a callable instance and a namedtuple field by identity; the C mode on call.c and descrobject.c; test_call, test_property, test_descr, test_functools, test_collections, test_structseq, test_capi, test_ctypes, test_asyncio, test_sqlite3; box audit; both suites |
| 4 | 0.5 | Getsets: `getter` and `setter`, Clinic's `@getter` and `@setter`, the 388 entries (mechanical, by subagent, reviewed and committed by the step agent; number-returning getters return values directly; setters read values with the value accessors or box at their top); docs (structures.rst); gate 4 and the write-up (note 06 section 21, note 08's register, note 09's handler table) | `complex.real`-style getset by identity; test_clinic; test_capi, test_datetime, test_memoryview, test_re, test_io, test_descr; `make clinic` clean; both suites; gate 4 |

**Scope cut**, in order, if a rung passes its deadline by more than half
a day: rung 4's conversion (getters and setters stay object functions,
with the register row of section 8; gate 4 and the write-up stay); then
`LOAD_ATTR_METHOD_LAZY_DICT` (its number stays in the reserve). Never
cut: the five slot types, the compatibility layer, the debug check, the
store poly handler, and the C mode clean on every marked function.

**Hard stop.** Commit what passes level 3, write the hand-off list in note
06 section 21, and report.

## 12. Open questions

1. **Inline receivers of int and float.** Their C methods and getsets
   take `PyObject *self`, so `x.bit_length()`, `x.is_integer()` and
   `x.real` box an inline `x` (chaos: 20,000 per loop). A closed fix
   exists: a type flag on int and float saying their method descriptors
   and getsets take `PyValue self`, and Clinic's `self: value` converter
   for their roughly 50 methods and getsets, so that
   `method_vectorcall_*` and `getset_get` pass `args[0]` unboxed.
   Recommendation: not in 6d; a step after 10a, or tier 1's intrinsics
   for the hot ones, ruled with chaos's numbers.
2. **The remaining slot families on values.** Number, sequence, mapping,
   comparison and hash slots box an inline operand for a user type's
   slot. Recommendation: a step 6e after 10a, scoped by a measurement of
   19.11's slot-wrapper row over the micro set, which 6d adds no rows
   for.
3. **Order with 10a.** Recommendation: 6d's rung 1 at once; rungs 2 and
   3 on typeobject.c before 10a's rung 1b marks the bridges, and rung 3's
   call.c and descrobject.c edits after 10a's rung 2 lands; or 6d marks
   the bridges itself if the C mode passes, which moves work from 1b to
   6d.
4. **Specialised handlers past 255.** The handler field is 16 bits; the
   256-entry tables and `dis` keep specialised handlers below 256.
   Recommendation: a cleanup step before tier 0 (step 8), which will
   want handlers of its own.
5. **Capacity acceptance.** If the suite's `capacity` dictionary-mode
   entries more than double, the rule keeps six slots for S from 1 to 3.
   Recommendation: record the counts and rule on them.
6. **`tp_getattr` and `tp_setattr`.** Recommendation: keep them as object
   slots until the ports phase decides.

## Review

(to be written after the review)
