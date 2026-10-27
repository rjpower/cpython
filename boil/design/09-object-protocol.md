# 09: The object protocol: shapes, the lookup plan, and what the optimiser sees

Status: design, 2026-10-27; revised the same day after the peer review
(boil/design/reviews/09-object-protocol-review.md, verdict revise).
Sections 2, 4, 5, 6 and 9 are rewritten; section 11 maps each blocking
issue to its decision. Written at the 6a review in answer to one
question: the value API of note 08 section 4 makes number and container
protocols box-free, but real code is user classes. How do attribute access
and method calls get fast, and how does ordinary lookup become something
the optimisation pipeline can read rather than something that happens
inside `tp_getattro`? This note decides the open choice of note 06 section
3.2 (the header word) and redefines step 6c. Step 6b's signatures are
shaped by it but 6b does not implement it.

## 1. One mechanism per job

Four mechanisms were named: hidden classes, inline lookup caches, side
caches on instructions, and lifting lookups during tracing. All four exist
here in some form. The design is to keep one of each, with one job each,
and to have them share one description of "where attribute `x` lives".

| Job | Mechanism | Where it lives |
|---|---|---|
| Layout | the shape (hidden class): one word per object that says which type the object has, which attributes it has, in which slots, with which kinds | the object header (section 2) |
| Observation | the feedback entry: the per-instruction side cache. The IR has no cache bytes in the instruction stream; the entry in the code object's feedback vector is the one cache, read by the interpreter, tier 0 and tier 1 | `_PyFbAttr`, `_PyFbCall`, `_PyFbPoly` (note 01, note 06 section 2.5) |
| Resolution | the lookup plan: a pure function from (shape, name) to "where the attribute is and what guard makes that true" | section 4 |
| Speculation | tier 1 reads feedback entries and plans, hoists guards, inlines monomorphic callees | note 06 section 5 |

Nothing is lifted during tracing because nothing traces. Tier 1 is a
method JIT over SSA; it asks the plan the same question the interpreter
asked, and gets the same answer, so interpreter, specialiser and JIT
cannot disagree about what a lookup means.

## 2. The shape is the header

Decision: `ob_header` holds the shape pointer in bits 0 to 47 and the flag
bits in 48 to 63. The type is `shape->type`. Every object has a shape. A
type whose instances carry no attribute slots (int, str, tuple, list,
dict, every C type without inline values) has exactly one, its root shape,
embedded in the type object (`tp_root_shape`), so a static object's header
is an address constant.

Note 06 section 3.2 deferred this to a count at 6c: attribute guards
against `Py_TYPE` loads. The count is not the right basis. The reason to
put the shape in the header is that every guard in the system becomes
one form, `(ob_header & _Py_HEADER_SHAPE_MASK) == shape`, a masked compare
with a constant. The mask is needed because the collector's flag bits
change independently of the layout; every write of a shape keeps them.

- an attribute load's guard proves the type, the layout, that the slot is
  filled, that there is no instance dict, and that no instance attribute
  shadows a method, in one compare;
- an exact type test on a type with one shape (`PyList_CheckExact` and
  every other builtin container) is the same compare;
- tier 0 bakes the constant into the template and tier 1 hoists the
  compare out of the loop; neither needs a second load to find a shape.

With the type in the header and the shape beside it, every one of these is
two loads, and a type test and a shape test are different code. One guard
word per object is the design of V8, JavaScriptCore and SpiderMonkey, and
it is the design under which the rest of this note is small. The mask is
the cost of keeping the flags in the same word; note 08's register moves
them to side bitmaps in step 7, after which the compare has no mask. 6c
records the number of masks in the eval loop and their cost.

Cost: `Py_TYPE(o)` becomes `shape->type`, two dependent loads where it was
a mask. Two parts of the design, not tuning passes, bound that:

1. The shape carries a copy of `tp_flags` (the `Py_TPFLAGS_*_SUBCLASS`
   bits the `PyXxx_Check` macros test, and the rest, written by the one
   setter `type_set_flags`). `PyLong_Check(o)` is header, shape, flags, as
   it is header, type, flags today.
2. An exact test against a one-shape type is a header compare (above),
   which covers the builtin containers and strings that dominate the
   runtime's type tests.

The remaining `Py_TYPE` sites pay one load. 6c records the count and the
per-loop cost; it is a recorded number, not a gate (section 9).

**Validity.** A type modification (a new descriptor on the class, a
changed base) must defeat every guard on the type's shapes. Shapes are
owned by their type, at most 64 of them, so `type_modified` writes the new
version into each. `shape->type_version` and the slot kinds, which only
generalise (section 5), are the mutable parts of an otherwise immutable
struct; changing a kind gives that one shape a fresh version. An attribute guard is therefore the header
compare plus `shape->type_version == v`: two dependent loads and two
compares, against today's three loads (type, version, shape byte). This
is V8's prototype validity cell with one cell per shape. Tier 1 removes
the version compare by registering compiled code as dependent on the type
(section 4.7 says how active code is deoptimised).

**Ownership.** A type owns its shapes: the root inside the type object;
the others in a table of PyMem blocks (`ht_shapes`) freed in the type's
dealloc. Shapes are not objects. The names a shape holds (edge names, slot
names) are visited by the type's traverse, because PyMem blocks reached
only from objects are not scanned; from step 3 of section 9 the shapes are
the only holders of those names. Every instance holds its type through its
shape, and the instance's traverse visits the type, so the sweep, which
frees referrers first, frees instances before their type and its shapes.
A static type used without `PyType_Ready()` fills its root shape at its
first object (`_PyObject_Init`).

## 3. The shape of a value

A `PyValue` that is an inline int or float has no header. Its shape is
defined by the tag:

```c
static inline _PyShape *_PyValue_Shape(PyValue v) {
    if (PyValue_IsInlineInt(v))   return &PyLong_Type.tp_root_shape;
    if (PyValue_IsInlineFloat(v)) return &PyFloat_Type.tp_root_shape;
    return _PyObject_SHAPE(PyValue_ObjectBorrow(v));
}
```

Attribute access and method calls on `3` or `2.5` go through the same
plan with that shape; the guard for an inline value is the tag test the
handler performs anyway. `(1).bit_length()`, `x.real` and `o.pos` are one
mechanism. `PyObject_GetAttrValue(PyValue owner, PyObject *name)` is the
API; `PyObject_GetAttr` canonicalises its argument, calls it and boxes
the result.

## 4. The lookup plan

### 4.1 Phases and preconditions

A lookup has three phases, and only the middle one is pure:

1. **The MRO lookup**, `_PyType_Lookup` with its present contract: the
   name is an exact `str`, the type is ready; it may assign a version tag,
   fill the type's cache and hash the name, and it runs user code only if
   a class dict on the MRO has a key that is not an exact `str` (the
   plan is then GENERIC and nothing is cached).
2. **Classification**, `_PyShape_ResolveLoad(shape, name, mro_result)`
   and `_PyShape_ResolveStore(shape, name, mro_result, op, value_kind)`:
   no allocation, no user code, no cache writes. They read the shape, the
   type's flags and slots, and the MRO result and its type.
3. **Execution**, which may call descriptors, allocate a dict or a
   transition, box, and raise.

A name that is not an exact `str` takes the generic path, as now. Only
interned names are cached; `PyObject_GetAttrValue` interns nothing.

### 4.2 Precedence

Classification follows `PyObject_GenericGetAttr` exactly, with `descr` the
MRO result:

1. `descr` is a data descriptor (its type has `tp_descr_set`): it wins;
2. otherwise the instance's storage: a shape slot, or the dict in
   dictionary mode;
3. otherwise `descr` is a non-data descriptor (`tp_descr_get` only): bind
   it;
4. otherwise `descr` is a plain class value: return it;
5. otherwise the attribute is missing: `__getattr__` if the type has one,
   else `AttributeError`.

A type with `__getattr__` and the default `__getattribute__` calls
`__getattr__` on an AttributeError from any step, so for such a type a
plan whose execution can raise AttributeError (ABSENT, PROPERTY,
DATA_DESCR, DESCR, and DICT whose fallback is ABSENT) is GENERIC; SLOT,
METHOD and CLASS_VALUE cannot raise it and are unaffected, as in today's
specialiser. A custom `__getattribute__` makes every plan GENERIC; when it
is a Python function and there is no `__getattr__`, `value` holds it, for
`LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`.

For a shape-mode object, step 2 is decided by the shape: the name is in
a slot or it is not. In dictionary mode step 2 is a dict lookup at
execution, so a DICT plan carries the outcome of steps 3 to 5 as its
fallback.

### 4.3 Plans

```c
typedef enum {
    _Py_LOOKUP_SLOT,          // a shape slot at byte `offset`; `slot_kind`
    _Py_LOOKUP_DICT,          // the instance dict; on a miss, `fallback`
    _Py_LOOKUP_DATA_DESCR,    // a data descriptor `value`; its tp_descr_get is read at execution;
                              // a __slots__ member records its byte `offset` and `member`
    _Py_LOOKUP_PROPERTY,      // a property `value`; its getter is read at execution
    _Py_LOOKUP_METHOD,        // a function or method descriptor `value`, not shadowed
    _Py_LOOKUP_DESCR,         // another non-data descriptor `value` (classmethod, staticmethod, ...)
    _Py_LOOKUP_CLASS_VALUE,   // a class attribute without __get__: `value`
    _Py_LOOKUP_ABSENT,        // AttributeError
    _Py_LOOKUP_MODULE,        // the receiver's module dict, then the class (receiver-dependent)
    _Py_LOOKUP_GENERIC,       // tp_getattro is not generic, __getattr__ and a plan
                              // that can raise AttributeError, or descr's type is mutable
} _PyLookupKind;

typedef struct {
    uint8_t kind;
    uint8_t fallback;         // DICT: the kind of steps 3-5 (METHOD, DESCR, CLASS_VALUE, ABSENT)
    uint8_t slot_kind;        // SLOT: BOXED, I32, F64 (section 5)
    uint8_t member;           // DATA_DESCR: a __slots__ member, loaded at `offset`
    uint32_t type_version;    // the guard: shape->type_version when classified
    uint32_t offset;          // SLOT, member: byte offset from the object start
    PyValue value;            // borrowed: the MRO result; valid while the guard holds
} _PyLookupPlan;
```

The plan's guard is (shape, type_version). Resolution walks the MRO once;
`type_modified` propagates to subclasses, so the receiver type's version
covers the MRO. A descriptor whose type is mutable (a heap type without
`Py_TPFLAGS_IMMUTABLETYPE`) can gain or lose `__get__` and `__set__`
without modifying the receiver's type, so such a `descr` yields GENERIC,
as today's specialiser's `MUTABLE` classification does; every specialised
plan therefore depends only on immutable descriptor types. `value` is the
MRO result whatever the kind, so that the plan of a root shape answers
`_PyType_Lookup` too (4.6).

Modules and classes as receivers are receiver-dependent: every module
shares one root shape, but each has its own dict. MODULE says "look up
the name in the receiver's dict at execution, then the class", and the
plan stays a function of (shape, name). The specialiser, not the plan,
reads the receiver's dict keys version and index and writes them into the
entry, as `LOAD_ATTR_MODULE` does now; the handler guards them. Type
receivers (`C.x`) go through `type_getattro`, which is GENERIC for the
plan, and keep their receiver-aware specialisations (`LOAD_ATTR_CLASS`).

### 4.4 Binding

Plans do not depend on the binding mode, so there is no `for_call`
argument and no binding mode in a cache key. Two executors share the plan:

- **The value form** (`o.m`, `PyObject_GetAttrValue`): SLOT loads; DICT
  looks up, then executes the
  fallback; METHOD and DESCR call `Py_TYPE(value)->tp_descr_get(value, o,
  type)`, so `f = o.m` is a bound method and `staticmethod` and
  `classmethod` behave as now; CLASS_VALUE returns `value`; DATA_DESCR
  for a member loads the slot at `offset` and raises AttributeError if it
  is unbound, and otherwise calls the descriptor type's current
  `tp_descr_get`; PROPERTY calls the
  property's current getter (AttributeError if it has none); ABSENT
  raises with the usual suggestion; GENERIC calls `tp_getattro`.
- **The call-pair form** (`LOAD_ATTR` with the method bit,
  `_Py_LoadAttr_Values`): METHOD yields (`value`, `o`) without binding;
  every other kind yields (the value form's result, NULL).

### 4.5 The store plan

`_PyShape_ResolveStore(shape, name, mro_result, op, value_kind)`, with
`op` SET or DELETE and `value_kind` the slot kind the incoming value
satisfies (section 5), returns one of:

| Outcome | Meaning | Execution |
|---|---|---|
| REPLACE | the name is in slot `offset` and the value's kind fits | write the slot |
| GENERALISE | the name is in a slot whose kind the value does not fit | set the slot's kind to BOXED in the shape, give the shape a fresh version, then write the slot; the header is unchanged |
| ADD | a new name; `shape_to` the ADD edge, or NULL if it does not exist yet | prepare, write the slot, then the header |
| REMOVE | DELETE of a slot name; `shape_to` the parent (last slot) or the hole shape, or NULL | prepare, clear the slot, then the header |
| TO_DICT | a limit (capacity, 30 slots, 64 shapes) forbids a new shape | convert to dictionary mode, then store in the dict |
| DICT | dictionary mode | store in or delete from the dict |
| DATA_DESCR | a data descriptor wins | a member: write or clear the slot at `offset`; otherwise call its type's current `tp_descr_set` |
| GENERIC | a custom `tp_setattro`, or `descr`'s type is mutable | call `tp_setattro` |
| MISSING | DELETE of a name the object does not have | AttributeError |

Preparation is the one allocating step, outside the resolver:
`_PyShape_Prepare(shape, outcome, name, kind)` creates the edge, or
returns NULL with MemoryError set, or reports the limit, in which case
the outcome becomes TO_DICT. The conversion to dictionary mode builds the
dict from the slots before it touches the object. In every failure the
object's slots and header are unchanged. A successful execution writes the
slot first and then the shape, with release order.

### 4.6 The plan cache

`Python/typecache.c` becomes the (shape, name) plan cache. It stays per
type, because the type owns the shapes and `type_modified` already clears
the type's cache: an entry is keyed by (shape, interned name) and holds a
plan, valid while its `type_version` equals the shape's, so a shape
whose kind was generalised drops its own entries and no others. The entry for the root shape holds the MRO result in `value`, which
is the raw lookup `_PyType_Lookup` and `_PyType_LookupValueAndVersion`
answer, so the special-method lookups beneath ordinary lookup keep their
contract. There is no recursion: classification of a non-root shape reads
the root entry, or calls `find_name_in_mro` on a miss; `find_name_in_mro`
never consults plans. The megamorphic path (a site with more than four
shapes) is header, shape, `shape->type`, cache: one hash probe, no MRO
walk.

### 4.7 Lifetime, rooting and invalidation

- **Borrowed, validated through the receiver.** A feedback entry's shape
  is compared with the live receiver's header before anything reads it;
  the version is then read from the receiver's shape, which is live.
  After a type dies, its version matches no live object, because version
  tags are never reused within an interpreter, so a reused shape address
  fails the version compare. `shape_to` is read only after the guard on
  its source shape passed, and belongs to the same type. `dis` and the
  test hooks print an entry's shape as an address and never follow it;
  `_testinternalcapi.shape(obj)` reads only the live object's shape.
- **No cache is a root.** Plan caches and feedback entries are not
  traversed and hold their values borrowed; a value is valid while its
  type's version holds, because the type's dict holds it. The cache dies
  with its type.
- **Rooting during execution.** Executors that may run code or allocate
  (descriptor calls, `tp_getattro`, dict lookups with non-str keys,
  boxing a slot for a `PyObject*` caller, dictionary conversion, preparing
  a transition) keep the receiver, the descriptor, the name and every
  intermediate box in `_PyValueRoot`s across the call, as
  `_Py_LoadAttr_Values` does: the eval loop's C locals are not scanned.
- **Descriptor contents.** PROPERTY and DATA_DESCR plans hold the
  descriptor object, which the class dict holds, and read its current
  getter or `tp_descr_get` at every execution: `p.__init__(new_getter)`
  changes no type, and its effect is seen at the next load. A handler that
  pushes the getter's frame guards the getter's function version after
  loading it from the property, so a replaced getter fails the guard
  instead of being called through a stale borrowed pointer.
- **Active compiled code** (a note for steps 8 and 9, no code in 6c).
  Tier 1 replaces the version compare with a dependency on the type, and
  may load a property's getter once per region. Invalidation marks the
  dependent code; every point in compiled code where user code can run (a
  call, a descriptor, a dict lookup that can compare) is a deoptimisation
  point with the interpreter state recorded. On return from such a point
  the code checks its mark and leaves for the interpreter there (lazy
  deoptimisation, as in V8). A guard hoisted across such a point is
  covered by a dependency or checked again after it; the getter read from
  a property is checked again after every such point.

### 4.8 Consumers and inspection

- `PyObject_GetAttrValue` and `PyObject_SetAttrValue`: plan, then execute.
- The specialiser: the plan kind selects the handler. Today
  `_Py_Specialize_LoadAttr` interleaves resolution (`analyze_descriptor`
  and its callers) with writes to the entry; 6c separates them, so the
  specialiser is "take the plan, write the entry, pick the handler".
  A DICT plan is a cached path, not the generic one: the entry records
  the dict's keys version and the name's index as a hint (the guard of
  today's `WITH_HINT` handlers), so an object that copy or pickle put
  into dictionary mode keeps a one-compare load.
- Tier 0: the handler in the entry selects the template.
- Tier 1: the plan gives the guard and the load, or the callee to inline.

`_testinternalcapi.lookup_plan(obj, name)` returns the plan as a tuple,
`_testinternalcapi.shape(obj)` returns the shape's type, slots and kinds,
and `dis` shows each attribute site's handler, shape and version. Tests
for the specialiser assert on plans and also on behaviour.

## 5. Slots hold PyValue; kinds are promises

Note 06 section 3.3 said an F64 slot holds a raw double. That would be a
second representation. Every slot holds a `PyValue`. An inline float is
its double's bits plus 2^49 (`_PyValue_DOUBLE_OFFSET`), and an inline int
is tagged in the space below, so a `PyValue` slot loses nothing against a
raw one, and an F64 slot is decoded with `PyValue_Double()`, never read as
a C double.

The slot kind is a promise about the values, not a layout:

- **I32**: every value stored in the slot is an inline int.
- **F64**: every value stored is an inline float. NaN is boxed (note 02),
  so it does not satisfy F64; infinities and signed zero do.
- **BOXED**: any value.

I32 and F64 each lie directly below BOXED and are unrelated to each other:
`int` and `float` are different types, so there is no I32 to F64 step.
Kinds only generalise. The first store of a name picks the kind of its
value (the ADD edge is keyed by name and kind); a store whose value does
not satisfy the slot's kind (an int out of int32 range, a NaN, a value of
another type) takes the GENERALISE outcome to a shape with that slot
BOXED in place. The load path does not test the tag when the plan says
I32 or F64, which is the gain the JIT wants.

Generalisation is in place (coordinator ruling, 2026-10-27). Every slot
holds a `PyValue`, so a kind change moves no field: the slot's kind in the
shape becomes BOXED, and the shape takes a fresh `type_version` from the
global tag counter. That invalidates every feedback entry, plan and
compiled guard on that shape and nothing else, and every object carrying
the shape stays valid. There is no second shape, no deprecation and no
migration. V8 deprecates maps because its representation changes move
fields; ours do not. An ADD edge still records the kind its first value
gave, so a later object built along the same edge gets the generalised
shape.

`__slots__` members are BOXED in 6c. Dict code never sees a kind: a dict
holds `PyValue`s, so dictionary conversion copies slots as they are.

## 6. Instance layout

**Layout.** An instance is the header word, then the native base's fields
up to the base's `tp_basicsize`, then `__slots__` members at their fixed
offsets, then `capacity` shape slots from the type's `tp_basicsize`. No
dict word, no pre-header, no `PyDictValues` header, no capacity byte.
Plans carry byte offsets, so the slot base is part of the plan, not a
load. Weak references stay in the side table. This applies to types with
`tp_itemsize == 0` whose bases have no dict of their own, the types that
get inline values today; a variable-size type (an `int` or `tuple`
subclass) or one whose native base has a dict (`BaseException`) has only
its root shape and keeps its dict at `tp_dictoffset`, and its plans are
DICT.

**Capacity is a type constant.** At type creation, `capacity = min(30,
max(6, S + 2))`, where S is the number of distinct names in
`__static_attributes__` over the MRO (0 for a type made by
`PyType_FromSpec`). Every instance of the type is allocated with it, and
it never changes. `_PyShape_Prepare` refuses a slot at or past the
capacity, which makes the add TO_DICT. A shape therefore proves that each
of its slots exists in every object that carries it, and no handler checks
capacity: `STORE_ATTR_SHAPE_TRANSITION`'s guard on its source shape is
enough. Today's per-object decay (29 slots for early instances, down to
`nentries + 1`) gave equal shapes unequal allocations, which a cached
transition cannot see. The cost is a fixed limit: a class whose attributes
are set from outside its methods (`setattr` in a loop, `class NS: pass`)
gets six slots, then dictionary mode. Rung 3 counts dictionary-mode
entries by reason over the benchmark set and the suite.

**Dictionary mode** is the type's dictionary-mode shape, whose slot 0
holds the instance dict or NULL (empty, created on demand); the other
slots are cleared. It is terminal for the object in 6c. Its loads and
stores are still cached, with the dict's keys version and an index hint
(4.8). It is entered by a limit
(capacity, 30 slots, 64 shapes), by `__class__` assignment, by `obj.__dict__
= d`, and by materialising the dict: `obj.__dict__`, `vars(obj)`,
`PyObject_GenericGetDict`, `_PyObject_GetDictPtr` and the default
`__getstate__`. Conversion builds the dict from the slots first; if that
fails the object is unchanged and MemoryError is raised; then it writes
slot 0 and the header.

**The materialised `__dict__`** (the ruling's option A). Reading
`__dict__` enters dictionary mode at once. Note 04 section 4 had chosen
the other option, a dict that aliases the slots through per-shape split
keys. That is not small under this layout and this collector. It needs a
third dict representation (values in the owner, keys from the owner's
shape) and a dict word in every instance. It also needs a relation between
dict and owner that is not a strong back-pointer yet keeps the owner's
memory valid while the dict lives. Today the sweep copies the values out
of a dying owner (`detach_dict_from_object` allocates in
`PyObject_ClearManagedDict`), although the sweep may not allocate, and on
failure it empties the dict. With option A the dict is an ordinary dict
held by slot 0: it has no back-pointer, a retained dict outlives its owner
with its contents, and `obj.__dict__ is obj.__dict__` holds. In the
internal set, 2 of 22 kernels read an instance `__dict__`: `logging`
(each `LogRecord` once, when it is formatted, near the end of its life)
and `deepcopy` (copy's `__reduce_ex__` path reads the source's state, and
`copy._reconstruct` updates the copy's `__dict__`, so copied and unpickled
objects live in dictionary mode). Two micro benchmarks
(`attr_load_after_dict`, `attr_store_after_dict`) measure the cost
directly, and rung 5 records them. test_class `TestInlineValues`
(`test_inspect_dict`, `test_update_dict`) pins the old behaviour and gets
the new expectation.

**`_PyObject_GetDictPtr`** keeps its writable contract: it enters
dictionary mode and returns the address of slot 0. Storing a dict there
replaces the instance dict; storing NULL empties it.

**`__slots__`** members stay on `tp_members` with their member
descriptors, so `C.a`, `copyreg._slotnames`, pickling's `(dict, slots)`
state, inheritance of base members and the duplicate-name check are
unchanged. They are not shape slots: a member has a fixed offset and no
occupancy fact, and its kind is BOXED. The handler is still chosen
through the plan: a member descriptor is a data descriptor of an immutable
type, so classification returns DATA_DESCR with `member` set and the
member's offset, and its executor is the slot load with the shape guard,
which checks for NULL and raises AttributeError when the member is
unbound; a delete clears it without a transition. The 30-slot limit does not apply to members. A class with
`__slots__` and `__dict__` has members before `tp_basicsize` and shape
slots after it. `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` stay as the
handlers for may-be-empty slots, with the shape guard in place of the
type-version guard; `tp_members` stays for C types too.

**Traversal.** An instance's traverse visits shape slots `0..nslots-1` in
shape mode or slot 0 in dictionary mode, never both, then its members and
its type as now. `gc.get_referents(obj)` returns the slot values or the
dict, plus the type.

**Limits** are note 04's: 30 slots and 64 shapes per type, and the
capacity above.

## 7. A method call, end to end

`o.m(a)`:

1. `LOAD_ATTR` with the method bit: plan is METHOD under (shape, version),
   call-pair form. The entry records shape, version and the descriptor;
   the handler is `LOAD_ATTR_METHOD_SHAPE`: compare the masked header,
   compare the version, write the descriptor and `o` into two slots.
   Several shapes at one site use the four-row poly table through
   `LOAD_ATTR_SHAPE_POLY`, the one polymorphic handler per feedback kind
   (note 04 coordinator decision 1).
2. `CALL`: `CALL_PY_EXACT_ARGS` guards the function version; the callee
   frame overlaps the argument window (note 06 section 4).
3. Tier 1: the shape compare is hoisted, the version compare becomes a
   code dependency, the descriptor is a constant, and the function version
   lets the callee's IR be inlined with its own feedback.

Attribute stores follow the store plan: `STORE_ATTR_SHAPE` (REPLACE),
`STORE_ATTR_SHAPE_TRANSITION` (ADD with an existing `shape_to`: guard
the source shape, write the slot, then the new shape
into the header with release order; no capacity check, section 6), the
data descriptor, and the generic path for the rest.

## 8. What 6b fixes in its signatures

6b does not change the header or the instance layout. It defines the
value-typed object protocol so that 6c fills it in:

- `PyObject_GetAttrValue(PyValue owner, PyObject *name) -> PyValue`,
  `PyObject_SetAttrValue(PyValue owner, PyObject *name, PyValue v)`,
  `_Py_LoadAttr_Values` (already present) for the method form.
- `PyObject_VectorcallValue(PyValue callable, const PyValue *args, size_t nargsf, PyObject *kwnames) -> PyValue`.
- `tp_getattro`, `tp_setattro`, `tp_call` keep their `PyObject*` shapes
  in 6b; a type whose getattro is the generic one takes the value path in
  6c, any other type is called through the slot and its result
  canonicalised.

## 9. Step 6c, redefined

The rungs of the step (each with its work doc under boil/design/work/):

1. The shape struct (type, flags, type_version, parent, edge, slot table,
   transitions); the type owns its shapes; every type has a root shape
   embedded in it; the header holds the shape; `Py_TYPE` reads
   `shape->type`; the `Check` macros read `shape->flags`; the type's
   traverse visits shape names (section 2).
2. The lookup and store plans of section 4 with their three phases; the
   specialiser rewritten over them; typecache becomes the (shape, name)
   plan cache; `PyObject_GetAttrValue` and `PyObject_SetAttrValue` execute
   plans; inspection in `_testinternalcapi` and `dis`.
3. The instance layout of section 6: the managed-dict pre-header, the
   `PyDictValues` header, shared keys for instances and the per-object
   capacity are deleted; capacity is a type constant; `__dict__` enters
   dictionary mode. Handlers: `LOAD_ATTR_SHAPE`, `LOAD_ATTR_SHAPE_POLY`,
   `LOAD_ATTR_METHOD_SHAPE`, `STORE_ATTR_SHAPE`,
   `STORE_ATTR_SHAPE_TRANSITION`; `LOAD_ATTR_INSTANCE_VALUE`,
   `LOAD_ATTR_WITH_HINT`, `LOAD_ATTR_METHOD_WITH_VALUES`,
   `LOAD_ATTR_METHOD_LAZY_DICT`, `LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES`,
   `STORE_ATTR_INSTANCE_VALUE`, `STORE_ATTR_WITH_HINT` and their guards
   deleted. `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` stay for `__slots__`
   members, with the shape guard.
4. Slot kinds as promises (section 5), with GENERALISE in place; the
   `str +=` decision (note 08's register).
5. Measurements and the write-up.

Boundaries. Custom `tp_getattro` and `tp_setattro`, `tp_descr_get` and
`tp_descr_set`, and native descriptor calls stay `PyObject*` interfaces in
6c: the executor boxes at them. Note 08's register records this as
"object slots box", retired by step 6d.

Later decisions, handed off (not 6c work): re-shaping a dictionary-mode
object whose dict has only string keys back to a shape on a slow-path
access.

Gates (boil/PROCESS.md): level 0 after every build; level 1 with the
rung's test files before each commit; level 2 at milestones; level 3 at
each rung's end (the full debug suite, `-u cpu test_cext`, and
`regen-cases` and `clinic` leaving the tree unchanged); a pyc magic bump
in the same commit as any change to compiler or specialiser output, the
handler set or the feedback layout; level 4 (`GATE_LABEL=6c`) as the
benchmark record of the step. Ports are deferred to phase P and are not a
gate of 6c.

Tests. Plan tests assert on the plan and on behaviour, through both the
value and the `PyObject*` APIs: extracted methods, `staticmethod`,
`classmethod`, custom `__get__`, instance shadowing of methods and class
values, missing attributes with and without `__getattr__`; one site
alternating modules with different insertion orders, then deletion and
reinsertion; descriptor mutation after warm-up and inside a hot loop (a
descriptor class gaining `__set__`, `p.__init__(new_getter)`), with
`gc.collect()` between mutation and reuse; `__dict__` identity, mutation
through the dict, replacement, and a dict retained after its owner dies;
instances at the capacity boundary, native subclasses, and injected
allocation failure during preparation and conversion (values and header
unchanged); `__slots__` unset, read, delete and reassign, inheritance, more
than 30 members, mixed kinds; kinds alternating int and float over shared
shapes, int32 overflow, NaN, infinities and signed zero; forced
collections during execution, shapes whose names only the shape holds,
transient types created and dropped in a loop, and shape-address reuse.

Recorded, not gated: per loop on richards, raytrace, deltablue, chaos, go
and nbody; the `Py_TYPE` site count, the masks in the eval loop and the
per-loop cost of the extra load on a container-heavy benchmark
(json_loads); boxes per loop; RSS for one million four-attribute
instances; `sys.getsizeof` of a small instance; shape and plan-cache
memory per type; `attr_load_after_dict` and `attr_store_after_dict`; a
five-shape site (`attr_load_poly`); shape-cap exhaustion; transient-type
churn; an allocation-heavy loop (`new_instance`); dictionary-mode entries
by reason.

## 10. Sources

V8 maps and field representations (Clifford, "Fast properties in V8",
2017; the `Map` and `FieldType` design); JavaScriptCore structures and
watchpoints; SpiderMonkey shapes and `ObjectGroup`; the Self and
Smalltalk inline cache lineage (Hölzle, Chambers, Ungar 1991, PICs).
Note 04 for the shape tree, limits and dictionary mode; note 01 for the
feedback vector.

## 11. Review response

The review (boil/design/reviews/09-object-protocol-review.md) raised nine
blocking issues; each is resolved by a decision above.

| Issue | Decision | Section |
|---|---|---|
| 1. Module lookups; `for_call` missing from the cache key | MODULE is a receiver-dependent strategy executed against the receiver's dict; the specialiser takes keys version and index from the receiver; plans are independent of binding, so there is no `for_call` | 4.3, 4.4 |
| 2. Ordinary loads must bind | the precedence of `PyObject_GenericGetAttr` in five steps; DESCR for other non-data descriptors; DICT carries a class fallback; a value form that binds and a call-pair form that does not | 4.2, 4.4 |
| 3. Descriptor mutation | a descriptor of a mutable type yields GENERIC; PROPERTY and DATA_DESCR read the descriptor's current getter at execution; handlers guard the getter after loading it; lazy deoptimisation for active compiled code, a note for steps 8 and 9 | 4.3, 4.7 |
| 4. Materialised `__dict__` | option A: reading `__dict__` enters dictionary mode; slot 0 holds an ordinary dict; no back-pointer; `_PyObject_GetDictPtr` returns slot 0's address; DICT plans stay cached with a keys-version hint; re-shaping is a later decision | 6, 4.8, 9 |
| 5. Allocation and capacity | capacity is a type constant from `__static_attributes__`; a shape proves its slots exist; byte offsets in plans cover native bases; conversion builds the dict before touching the object | 6, 4.5 |
| 6. `__slots__` presence and kinds | members stay on `tp_members`, are not shape slots, are BOXED and may be empty; a member is a DATA_DESCR plan whose executor is the slot load with the shape guard; `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` stay | 4.3, 6 |
| 7. Kind lattice and float encoding | I32 and F64 each below BOXED; slots hold `PyValue`, F64 decoded by `PyValue_Double()`; generalisation in place with a fresh shape version, no deprecation (coordinator ruling) | 5, 4.5 |
| 8. Store-plan inputs and allocation | three phases; the resolver takes `op` and `value_kind`; preparation is the one allocating step; failure leaves slots and header unchanged | 4.1, 4.5 |
| 9. The collector's contract | the type's traverse visits shape names; caches are borrowed, never roots, validated through the live receiver; value roots across allocating calls; instances are swept before their type | 2, 4.7, 6 |

Non-blocking points: the guard is written as the masked compare (2); the
plan cache keeps `_PyType_Lookup`'s contract (4.6); the measurements and
gates of section 9 are extended. The review's three questions were ruled
on by the coordinator: native descriptor calls and custom
`tp_getattro`/`tp_setattro` stay `PyObject*` boundaries until step 6d
(section 9); an escaped instance dict does not retain its owner (6); the
`str +=` decision stays in 6c's rung 4.
