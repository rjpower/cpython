# 09: The object protocol: shapes, the lookup plan, and what the optimiser sees

Status: design, 2026-10-27; revised the same day after the peer review
(boil/design/reviews/09-object-protocol-review.md, verdict revise).
Sections 2, 4, 5, 6 and 9 are rewritten; section 11 maps each blocking
issue to its decision. Revised again after the second review
(boil/design/reviews/09-object-protocol-review-2.md); section 11.2. Written at the 6a review in answer to one
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
struct; generalising a kind gives the affected shapes fresh versions
(section 5). A version of zero means "no version": it never authorises a
cache entry, a specialisation or a compiled guard. A type that has no
version (version tags exhausted for the class or the interpreter, or an
MRO that cannot be versioned) has shapes with version zero, and nothing
about it is cached; that is how exhaustion disables speculation. An attribute guard is therefore the header
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

### 4.1 Dispatch, phases and preconditions

`PyObject_GetAttr` and `PyObject_SetAttr` dispatch on `tp_getattro` and
`tp_setattro` first, as now. A plan describes only the generic protocol
(`PyObject_GenericGetAttr`, `PyObject_GenericSetAttr`) for a shape. A
type whose slot is the generic one is executed from its plan directly.
Any other slot is called (a Python `__getattribute__` or `__getattr__`
through `_Py_slot_tp_getattr_hook`, `module_getattro`, `type_getattro`,
a C type's own function). When that slot reaches the generic protocol
(`object.__getattribute__`, `_PyObject_GenericGetAttrWithDict`, the
`__getattr__` hook's first step), it executes the same plan, and
execution never dispatches through the slot again. So hooks are outer
dispatch, plans are the protocol beneath them, and nothing recurses.

The generic protocol has three phases, and only the middle one is pure:

1. **Facts.** The MRO lookup of the name (`_PyType_Lookup`'s contract)
   gives `descr`. For the specialiser, the type's hook facts are looked up
   the same way: the `__getattribute__` and `__getattr__` it would call,
   and whether `tp_setattro` is generic. Phase 1 also decides whether the
   result may be cached. It may if the name is an interned exact `str`,
   the type's version is not zero, every class dict on the MRO has only
   exact-`str` keys (so the lookup ran no user code), and `descr`'s type
   is immutable (`Py_TPFLAGS_IMMUTABLETYPE`), so that its `__get__` and
   `__set__` cannot change without a type change. A result that may not
   be cached is still used; it is computed again at the next execution.
2. **Classification**, `_PyShape_ResolveLoad(shape, name, descr)` and
   `_PyShape_ResolveStore(shape, name, descr, op, value_kind)`. These
   allocate nothing, run no user code and write no cache. They read the
   shape, `shape->type` and `descr`'s type.
3. **Execution**, which may call descriptors, allocate a dict or a
   transition, box, and raise.

A name that is not an exact `str` is classified the same way and is not
cached: data descriptors keep their precedence, and instance storage is
materialised only if execution reaches it.

### 4.2 Precedence

Classification follows the generic protocol exactly, with `descr` the MRO
result:

1. For a load, `descr` is a data descriptor if its type has both
   `tp_descr_get` and `tp_descr_set`: it wins. For a store, `descr` wins
   if its type has `tp_descr_set`. A set-only descriptor is therefore a
   class value for loads and a data descriptor for stores, as now.
2. Otherwise the instance's storage: a shape slot, or the dict in
   dictionary mode.
3. Otherwise `descr` is a non-data descriptor (`tp_descr_get`): bind it.
4. Otherwise `descr` is a plain class value: return it.
5. Otherwise the attribute is missing: AttributeError.

`__getattr__` is not part of the plan: it belongs to the hook that calls
the plan (4.1). The specialiser, which replaces the whole dispatch at a
site, may specialise a type with `__getattr__` and the default
`__getattribute__` only for plans whose execution cannot raise
AttributeError (SLOT, METHOD, CLASS_VALUE, and DICT with one of those as
its fallback), as today's specialiser does. A Python `__getattribute__`
comes from the hook facts, not from the plan, for
`LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`.

For a shape-mode object, step 2 is decided by the shape: the name is in
a slot or it is not. In dictionary mode step 2 is a dict lookup at
execution, so the plan carries the outcome of steps 3 to 5 as its
fallback.

### 4.3 Plans

```c
typedef enum {
    _Py_LOOKUP_SLOT,          // a shape slot at byte `offset`; `slot_kind`
    _Py_LOOKUP_DICT,          // the instance dict; on a miss, `fallback`
    _Py_LOOKUP_DATA_DESCR,    // a data descriptor `value`; its slot is read at execution;
                              // MEMBER: a __slots__ member loaded or stored at `offset`
    _Py_LOOKUP_PROPERTY,      // a property `value`; its getter is read at execution
    _Py_LOOKUP_METHOD,        // a function or method descriptor `value`, not shadowed
    _Py_LOOKUP_DESCR,         // another non-data descriptor `value` (classmethod, staticmethod, ...)
    _Py_LOOKUP_CLASS_VALUE,   // a class attribute without __get__: `value`
    _Py_LOOKUP_ABSENT,        // AttributeError
} _PyLookupKind;

typedef struct {
    uint8_t kind;
    uint8_t fallback;         // every kind but DATA_DESCR and PROPERTY: the kind of
                              // steps 3-5 (METHOD, DESCR, CLASS_VALUE, ABSENT)
    uint8_t slot_kind;        // SLOT: BOXED, I32, F64 (section 5)
    uint8_t flags;            // CACHEABLE; MEMBER
    uint32_t type_version;    // the guard: the shape's version when classified; 0: none
    uint32_t offset;          // SLOT, MEMBER: byte offset from the object start
    PyValue value;            // borrowed: the MRO result, whatever the kind
} _PyLookupPlan;
```

The plan's guard is (shape, type_version). Resolution walks the MRO once;
`type_modified` propagates to subclasses, so the receiver type's version
covers the MRO. Only a CACHEABLE plan is cached or specialised; the
others are executed as classified and are classified again next time.
`value` is always the MRO result of the name, so that the plan of a root
shape answers `_PyType_Lookup` too (4.6).

`fallback` is set on every plan except data descriptors and properties,
so that a caller that supplies its own dict
(`_PyObject_GenericGetAttrWithDict(obj, name, dict, ...)`) executes
"dict, then fallback" from the same plan.

**Members.** A member descriptor is a data descriptor of an immutable
type. Its plan is MEMBER, which means a direct load or store at `offset`,
only if four conditions hold:

- the member is a `__slots__` value member (`_Py_T_VALUE_EX`);
- `shape->type` is a subtype of the descriptor's owner (`d_type`);
- the member has no `_Py_AFTER_ITEMS` flag, so its offset is constant;
- the offset lies within the type's `tp_basicsize`.

A load also needs no `Py_AUDIT_READ`, and a store needs no `Py_READONLY`.
Any other member descriptor, such as a native pointer or numeric member,
or one transplanted onto an unrelated class (`B.x = A.x`, where `B().x`
must raise TypeError), is DATA_DESCR without MEMBER and runs through its
descriptor. Deleting an unbound member raises AttributeError, as
`structmember.c` does.

**Modules and classes as receivers** are not special kinds. A module's
`tp_getattro` is `module_getattro`, which is a hook (4.1). It runs the
generic protocol on the module's shape: a DICT plan over the module's
own dict, with data-descriptor precedence (a dict entry named
`__class__` does not shadow the type's descriptor) and the class as
fallback. Then it does what is its own: the module `__getattr__` and
lazy-import resolution. The specialiser's `LOAD_ATTR_MODULE` replaces
that whole dispatch at a site, under its own guards: the exact module
type and the dict's keys version and index, which it reads from the
receiver. Class receivers (`C.x`) are the same with `type_getattro` and
`LOAD_ATTR_CLASS`.

### 4.4 Binding

Plans do not depend on the binding mode, so there is no `for_call`
argument and no binding mode in a cache key. Two executors share the plan:

- **The value form** (`o.m`, the generic protocol's load):
  - SLOT loads.
  - DICT looks up, then executes the fallback.
  - METHOD and DESCR call `Py_TYPE(value)->tp_descr_get(value, o, type)`.
    So `f = o.m` is a bound method, and `staticmethod` and `classmethod`
    behave as now.
  - CLASS_VALUE returns `value`.
  - DATA_DESCR with MEMBER loads the slot at `offset`, and raises
    AttributeError if it is unbound. Without MEMBER it calls the
    descriptor type's current `tp_descr_get`.
  - PROPERTY calls the property's current getter (AttributeError if it
    has none).
  - ABSENT raises with the usual suggestion.
- **The call-pair form** (`LOAD_ATTR` with the method bit,
  `_Py_LoadAttr_Values`): METHOD yields (`value`, `o`) without binding;
  every other kind yields (the value form's result, NULL).

### 4.5 The store plan

`_PyShape_ResolveStore(shape, name, descr, op, value_kind)` takes `op`,
SET or DELETE, and `value_kind`, the narrowest slot kind the incoming
value satisfies (section 5). It returns one of these outcomes:

| Outcome | Meaning | Execution |
|---|---|---|
| REPLACE | the name is in slot `offset` and the value's kind fits | write the slot |
| GENERALISE | the name is in a slot whose kind the value does not fit | generalise the field (section 5), then write the slot; the header is unchanged |
| ADD | a new name; `shape_to` the ADD edge, or NULL if it does not exist yet | prepare, write the slot, then the header |
| REMOVE | DELETE of a slot name; `shape_to` the parent (last slot) or the hole shape, or NULL | prepare, clear the slot, then the header |
| TO_DICT | a limit (capacity, 30 slots, 64 shapes) forbids a new shape | build a dict from the slots plus this store, then publish it (below) |
| DICT | dictionary mode | store in or delete from the dict |
| DATA_DESCR | `descr` has `tp_descr_set` | MEMBER: write or clear the slot at `offset`; otherwise call its type's current `tp_descr_set` |
| MISSING | DELETE of a name the object does not have | AttributeError |
| NO_STORAGE | SET or DELETE of a name on an object without instance storage and no data descriptor | AttributeError, as now |

Preparation is the one allocating step, outside the resolver.
`_PyShape_Prepare(shape, outcome, name)` creates the edge and returns it.
On failure it returns one of two distinct results: NOMEMORY, with
MemoryError set, or LIMIT, which turns the outcome into TO_DICT. The
specialiser may prepare, so a site whose transition is new can still be
specialised.

TO_DICT builds the provisional dict from the slots, inserts the
triggering store into it, and only then publishes it: slot 0, then the
header. A failure at any step before publication discards the provisional
dict. In every failure the object's slots and header are unchanged. A
successful execution writes the slot first and then the shape, with
release order.

Store plans are not cached. The store resolver reads `descr` from the
root shape's cached entry, and the transition from the shape tree.

### 4.6 The plan cache

`Python/typecache.c` becomes the (shape, name) cache of load plans.

- **Per type.** The type owns the shapes, and `type_modified` already
  clears the type's cache.
- **Entries.** An entry is keyed by (shape, interned name) and holds a
  plan. It is valid while the plan's nonzero `type_version` equals the
  shape's version. A stale entry, after a kind generalisation, is
  replaced in place when it is next classified. The free-threaded build
  no longer exists (note 06 section 3.4), so no lock-free publication
  protocol is needed beyond the present one.
- **The root entry.** The entry for the root shape holds the MRO result
  in `value`. That is the raw lookup `_PyType_Lookup` and
  `_PyType_LookupValueAndVersion` answer, so the special-method lookups
  beneath ordinary lookup keep their contract.
- **No recursion.** A root entry is filled by `find_name_in_mro` alone.
  A non-root entry reads the root entry for `descr`.
  `find_name_in_mro` never consults plans, and the hook facts are looked
  up as ordinary root entries of their own names.
- **The megamorphic path** (a site with more than four shapes) is header,
  shape, `shape->type`, cache: one hash probe, no MRO walk.

### 4.7 Lifetime, rooting and invalidation

- **Borrowed, validated through the receiver.**
  - A feedback entry's shape is compared with the live receiver's header
    before anything reads it. The version is then read from the
    receiver's shape, which is live.
  - After a type dies, its version matches no live object, because
    version tags are never reused within an interpreter, so a reused
    shape address fails the version compare.
  - `shape_to` is read only after the guard on its source shape has
    passed, and it belongs to the same type.
  - `dis` and the test hooks print an entry's shape as an address and
    never follow it. `_testinternalcapi.shape(obj)` reads only the live
    object's shape.
- **No cache is a root.**
  - Plan caches and feedback entries are not traversed, and they hold
    their values borrowed. A value is valid while its type's version
    holds, because the type's dict holds it. The cache dies with its
    type.
  - The tables are PyMem blocks reached only from the type, so the
    collector does not scan them. It does scan one transiently while a C
    stack points into it (gc_trace.c's transitive scan of PyMem blocks),
    which only delays the reclamation of what the table names; it never
    frees something still in use.
- **Rooting during execution.** Some executors may run code or allocate:
  descriptor calls, hooks, dict lookups with non-str keys, boxing a slot
  for a `PyObject*` caller, dictionary conversion, preparing a
  transition. Across those calls they keep the receiver, the descriptor,
  the name, the incoming store value, the provisional dict and every
  intermediate box in `_PyValueRoot`s, as `_Py_LoadAttr_Values` does. The
  eval loop's C locals are not scanned. Phase 1 on a cacheable type
  allocates no object and runs no code. On any other type its
  intermediate results are rooted the same way.
- **Descriptor contents.**
  - PROPERTY and DATA_DESCR plans hold the descriptor object, which the
    class dict holds. They read its current getter, or `tp_descr_get`, at
    every execution, so `p.__init__(new_getter)`, which changes no type,
    takes effect at the next load.
  - A handler that pushes the getter's frame guards the getter after
    loading it from the property: it must be a function with the
    recorded version. A replaced getter, a NULL one or a non-function
    one therefore fails the guard instead of being called through a
    stale borrowed pointer.
- **Active compiled code** (a note for steps 8 and 9; no code in 6c).
  - Tier 1 replaces the version compare with a dependency on the shape's
    version: the type's version, or a kind generalisation's fresh one.
    Both `type_modified` and generalisation notify the dependants. Tier 1
    may load a property's getter once per region.
  - Invalidation marks the dependent code. Every point in compiled code
    where user code can run (a call, a descriptor, a dict lookup that can
    compare) is a deoptimisation point, with the interpreter state
    recorded. On return from such a point the code checks its mark and,
    if it is set, leaves for the interpreter there (lazy deoptimisation,
    as in V8).
  - A guard hoisted across such a point is covered by a dependency, or
    checked again after it. A getter read from a property is checked
    again after every such point.

### 4.8 Consumers and inspection

- The generic protocol and `PyObject_GetAttrValue` and
  `PyObject_SetAttrValue`: dispatch (4.1), then plan, then execute.
- The specialiser: the plan kind, the hook facts and the store outcome
  select the handler.
  - Today `_Py_Specialize_LoadAttr` interleaves resolution
    (`analyze_descriptor` and its callers) with writes to the entry. 6c
    separates them, so the specialiser is "take the plan, write the
    entry, pick the handler".
  - A DICT plan is a cached path, not the generic one. The entry records
    the dict's keys version and the name's index as a hint, and
    `LOAD_ATTR_WITH_HINT` and `STORE_ATTR_WITH_HINT` guard them. So an
    object that copy or pickle put into dictionary mode keeps a
    one-compare load.
  - Store handlers into an I32 or F64 slot also guard the incoming
    value's kind (section 5).
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
Kinds only generalise. A kind belongs to a field: the slot added by one
ADD edge, in the shape that edge leads to (the field's owner) and in
every shape below it, which are the shapes that contain the field. The
first store of a name picks the kind of its value. ADD edges are keyed by
name alone. The load path does not test the tag when the plan says I32 or
F64, which is the gain the JIT wants.

**Stores guard the value.** A specialised store into an I32 or F64 slot
(`STORE_ATTR_SHAPE`, `STORE_ATTR_SHAPE_TRANSITION`) also tests the
incoming value's tag; a value of another kind leaves for the slow path,
which generalises. Such values are an int out of int32 range, a NaN, a
`bool`, an `int` subclass, or anything else. A BOXED slot needs no test.
Tier 1 omits the test only where it has proved the value's kind.

**Generalisation is in place** (coordinator ruling, 2026-10-27; the field
rule is the second review's correction). Every slot holds a `PyValue`,
so a kind change moves no field. The field's kind becomes BOXED in its
owner and in every shape below the owner. Each of those shapes takes a
fresh `type_version` from the global tag counter, or zero if the type
has no version. That invalidates every feedback entry, plan and compiled
guard on those shapes and nothing else, and every object carrying them
stays valid. Because the kind changes in the whole subtree,

- a delete back to the parent, or to a hole shape,
- an ADD from a generalised shape, and
- a new object built along the same edges

all keep promises that hold for the values present.
`o.x = 1; o.y = 2; o.x = "s"; del o.y` returns `o` to a shape whose `x`
is already BOXED. There is no second shape, no deprecation and no
migration. V8 generalises field types over the owner's subtree for the
same reason. It deprecates maps only when a representation change moves
fields, and ours never does.

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
get inline values today. Two other cases have only their root shape, and
their plans are DICT:

- **A type whose native base has a dict** (`BaseException`) keeps that
  field at its `tp_dictoffset`.
- **A variable-size Python subclass** (of `int`, `tuple`, `bytes`, or a
  variable-size extension type) has `Py_TPFLAGS_MANAGED_DICT` and
  `tp_dictoffset == -1` today, with the dict in the pre-header. The
  pre-header is deleted, so it gets a conventional computed dict offset:
  a dict word after the variable part, found by
  `_PyObject_ComputedDictPointer` from a negative `tp_dictoffset`, as
  before 3.12. Creation, allocation, traversal, clearing and
  `_PyObject_GetDictPtr` change together in rung 3.

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

*Amendment, 2026-10-27.* A class decorator that generates `__init__`
cannot add to `__static_attributes__` from the class body, so an 8-field
dataclass got six slots and every instance converted. Capacity stays a
type constant computed from `__static_attributes__`, with one addition:
assigning or deleting `__static_attributes__` on a type that has never
allocated an instance and has no subclasses recomputes the capacity. The
allocation path sets the type's `allocated` flag (`_PyTypeShapes`, one
byte store per allocation); after it, or once a subclass exists (whose
capacity counted the old names), the assignment is an ordinary attribute
store with no effect on capacity. The invariant holds because no object
has the type's shapes before its first allocation; an object given the
type by `__class__` assignment is in dictionary mode and uses slot 0
alone. `dataclasses._process_class` assigns the existing names plus the
field names before anything can instantiate the class; attrs and pydantic
can use the same path. The micro benchmarks `attr_load_dataclass` and
`attr_store_dataclass` measure the case.

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
type, so classification returns DATA_DESCR with MEMBER set and the
member's offset (under the conditions of 4.3), and its executor is the slot load with the shape guard,
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

**`__class__` assignment** between types with different capacities is
allowed when their `tp_basicsize` agrees, as now. The object enters
dictionary mode, which uses slot 0 only, so the destination type's
capacity is never relied on for it. Clearing and traversal in dictionary
mode touch slot 0 only.

**Limits** are note 04's: 30 slots and 64 shapes per type, and the
capacity above. A plain instance with six slots is 56 bytes (a 64-byte
bin); note 08's proof for the pre-header row says so.

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
   `LOAD_ATTR_METHOD_WITH_VALUES`, `LOAD_ATTR_METHOD_LAZY_DICT`,
   `LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES`, `STORE_ATTR_INSTANCE_VALUE` and
   their guards deleted. `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` stay for
   members, and `LOAD_ATTR_WITH_HINT` and `STORE_ATTR_WITH_HINT` for DICT
   plans (reading slot 0), each with the shape guard.
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
shapes, int32 overflow, NaN, infinities and signed zero; warmed store
sites given an int out of range, a float, a NaN, a `bool` and an `int`
subclass; generalisation followed by ADD, delete-last and hole deletion
across shared shapes; delegating `__getattribute__` overrides, explicit
`object.__getattribute__` and `object.__setattr__`, mutable descriptor
types, and the generic C API; transplanted member descriptors and
variable-size subclasses with members; module `__getattr__`, a module
dict entry named `__class__`, lazy imports and module subclasses; raw
`_PyType_Lookup` beside ordinary lookup on a class with a Python
`__getattribute__`; version exhaustion (`_testcapi` version hooks);
variable-size subclasses with several payload lengths; forced
collections during execution, shapes whose names only the shape holds,
transient types created and dropped in a loop, and shape-address reuse.

Recorded, not gated: per loop on richards, raytrace, deltablue, chaos, go
and nbody; the `Py_TYPE` site count, the masks in the eval loop and the
per-loop cost of the extra load on a container-heavy benchmark
(json_loads); boxes per loop; RSS for one million four-attribute
instances; `sys.getsizeof` of a small instance; shape and plan-cache
memory per type; wall time beside instructions for the dependent-load
cost; generalisation fan-out (shapes re-versioned per generalisation);
fixed-capacity waste (slots allocated against slots used);
`attr_load_after_dict` and `attr_store_after_dict`; a
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

### 11.1 First review

The review (boil/design/reviews/09-object-protocol-review.md) raised nine
blocking issues; each is resolved by a decision above.

| Issue | Decision | Section |
|---|---|---|
| 1. Module lookups; `for_call` missing from the cache key | MODULE is a receiver-dependent strategy executed against the receiver's dict; the specialiser takes keys version and index from the receiver; plans are independent of binding, so there is no `for_call` (revised in 11.2: no MODULE kind) | 4.3, 4.4 |
| 2. Ordinary loads must bind | the precedence of `PyObject_GenericGetAttr` in five steps; DESCR for other non-data descriptors; DICT carries a class fallback; a value form that binds and a call-pair form that does not | 4.2, 4.4 |
| 3. Descriptor mutation | a descriptor of a mutable type yields an uncacheable plan (11.2); PROPERTY and DATA_DESCR read the descriptor's current getter at execution; handlers guard the getter after loading it; lazy deoptimisation for active compiled code, a note for steps 8 and 9 | 4.3, 4.7 |
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

### 11.2 Second review

The second review (boil/design/reviews/09-object-protocol-review-2.md)
raised eight blocking issues; three of them it shares with the review of
rung 2's work doc.

| Issue | Decision | Section |
|---|---|---|
| 1. In-place generalisation and transitions | a kind belongs to a field; generalisation covers the field's owner and every shape below it, each re-versioned; ADD edges keyed by name | 5 |
| 2. Stores need value guards | specialised stores into I32 and F64 slots test the incoming value's tag; a miss generalises | 5, 4.8 |
| 3. GENERIC recursion | hooks are outer dispatch; plans describe only the generic protocol, which a hook may run but never re-dispatches; GENERIC and MODULE are no longer kinds; a mutable descriptor type makes a plan uncacheable, not generic | 4.1, 4.2, 4.3 |
| 4. Member offsets | MEMBER only for a `_Py_T_VALUE_EX` member whose owner is a base of the shape's type, at a constant offset inside `tp_basicsize`, unaudited for loads and writable for stores; any other member runs through its descriptor | 4.3 |
| 5. `plan.value` | always the MRO result; `__getattribute__` and `__getattr__` are hook facts of phase 1 | 4.1, 4.2, 4.3 |
| 6. Zero and exhausted versions | zero never authorises caching, specialisation or a compiled guard; an unversioned type has unversioned shapes; generalisation notifies compiled dependants | 2, 4.6, 4.7 |
| 7. Variable-size managed-dict types | a computed dict offset after the variable part, replacing the pre-header | 6 |
| 8. MODULE | a module's getattro is a hook over a DICT plan on its own dict; `LOAD_ATTR_MODULE` replaces the whole dispatch at a site | 4.3 |

Non-blocking points: TO_DICT stages the triggering store in the
provisional dict before publishing (4.5); the cache tables and the
execution roots are named (4.7); DICT plans keep the `WITH_HINT`
handlers (9); measurements gain wall time, fan-out and capacity waste
(9). Answers to its questions: the cache holds load plans only, and
store plans are computed from the root entry and the shape tree;
NO_STORAGE is the outcome for an object without instance storage;
dictionary mode after `__class__` assignment never relies on capacity
(6); the `str +=` representation is rung 4's decision.
