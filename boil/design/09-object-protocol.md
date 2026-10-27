# 09: The object protocol: shapes, the lookup plan, and what the optimiser sees

Status: design, 2026-10-27. Written at the 6a review in answer to one
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
dict, every C type without a managed dict) has exactly one, its root shape.

Note 06 section 3.2 deferred this to a count at 6c: attribute guards
against `Py_TYPE` loads. The count is not the right basis. The reason to
put the shape in the header is that every guard in the system becomes
"compare the header word with a constant":

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
it is the design under which the rest of this note is small.

Cost: `Py_TYPE(o)` becomes `shape->type`, two dependent loads where it was
a mask. Two parts of the design, not tuning passes, bound that:

1. The shape carries the type flag bits that the `PyXxx_Check` macros
   test (`Py_TPFLAGS_*_SUBCLASS`, `Py_TPFLAGS_HAVE_GC` and the other bits
   fixed at `PyType_Ready`). `PyLong_Check(o)` is header, shape, flags,
   as it is header, type, flags today.
2. An exact test against a one-shape type is a header compare (above),
   which covers the builtin containers and strings that dominate the
   runtime's type tests.

The remaining `Py_TYPE` sites pay one load. 6c records the count and the
per-loop cost; it is a recorded number, not a gate (section 9).

**Validity.** A type modification (a new descriptor on the class, a
changed base) must defeat every guard on the type's shapes. Shapes are
owned by their type, at most 64 of them, so `type_modified` writes the new
version into each: `shape->type_version` is the one mutable word in an
otherwise immutable struct. An attribute guard is therefore the header
compare plus `shape->type_version == v`: two dependent loads and two
compares, against today's three loads (type, version, shape byte). This
is V8's prototype validity cell with one cell per shape. Tier 1 removes
the version compare altogether by registering compiled code as dependent
on the type, so that `type_modified` invalidates the code instead of
being checked (note 06 section 5 gains this line).

## 3. The shape of a value

A `PyValue` that is an inline int or float has no header. Its shape is
defined by the tag:

```c
static inline _PyShape *_PyValue_Shape(PyValue v) {
    if (PyValue_IsInlineInt(v))   return _PyLong_Type_root_shape;
    if (PyValue_IsInlineFloat(v)) return _PyFloat_Type_root_shape;
    return _PyObject_Shape(PyValue_ObjectBorrow(v));
}
```

Attribute access and method calls on `3` or `2.5` go through the same
plan with that shape; the guard for an inline value is the tag test the
handler performs anyway. `(1).bit_length()`, `x.real` and `o.pos` are one
mechanism. `PyObject_GetAttrValue(PyValue owner, PyObject *name)` is the
API; `PyObject_GetAttr` canonicalises its argument, calls it and boxes
the result.

## 4. The lookup plan

```c
typedef enum {
    _Py_LOOKUP_SLOT,          // instance slot at `offset`; `kind` is the slot's kind
    _Py_LOOKUP_DICT,          // dictionary-mode instance: look in the dict at slot 0
    _Py_LOOKUP_CLASS_VALUE,   // a non-descriptor class attribute: `value`
    _Py_LOOKUP_METHOD,        // a method descriptor on the type, not shadowed: `value`
    _Py_LOOKUP_DATA_DESCR,    // a data descriptor: call `value`'s tp_descr_get
    _Py_LOOKUP_PROPERTY,      // a property: call `value` (fget) with the owner
    _Py_LOOKUP_MODULE,        // module attribute: dict index `offset` under the keys version
    _Py_LOOKUP_GENERIC,       // the type's getattro is not generic, or __getattr__ exists
} _PyLookupKind;

typedef struct {
    _PyLookupKind kind;
    uint8_t slot_kind;        // BOXED, I32, F64 (section 5)
    uint32_t type_version;    // the guard: shape->type_version at the time of the plan
    uint32_t offset;
    PyValue value;            // borrowed; valid while the guard holds
} _PyLookupPlan;

_PyLookupPlan _PyShape_LookupPlan(_PyShape *shape, PyObject *name, int for_call);
_PyLookupPlan _PyShape_StorePlan(_PyShape *shape, PyObject *name, _PyShape **shape_to);
```

Rules. The plan is pure: no allocation, no user code (a descriptor's
`__get__` is not run; a type with its own `__getattribute__` or a
`__getattr__` yields GENERIC). The plan's guard is the pair (shape,
type_version). Resolution walks the type's MRO once, as `_PyType_Lookup`
does; `type_modified` already propagates to subclasses, so the receiver
type's version covers the MRO.

Consumers:

- `PyObject_GetAttrValue`: plan, then execute. SLOT loads; METHOD and
  CLASS_VALUE return the value; DATA_DESCR and PROPERTY call; GENERIC
  calls `tp_getattro` and canonicalises.
- The specialiser: the plan kind is the handler. `_Py_Specialize_LoadAttr`
  today interleaves resolution (`analyze_descriptor` and its callers) with
  writes to the cache entry; 6c separates them, so the specialiser is
  "take the plan, write the entry, pick the handler for the kind".
- Tier 0: the handler in the entry selects the template.
- Tier 1: the plan gives the guard and the load, or the callee to inline.
- The megamorphic path: a site with more than four (shape, name) pairs
  runs the generic handler, which consults a global (shape, name) to plan
  cache. `Python/typecache.c`, today keyed by (type version, name), becomes
  that cache. One hash lookup, no MRO walk, for the megamorphic case.

Inspection. `_testinternalcapi.lookup_plan(obj, name)` returns the plan as
a tuple, `_testinternalcapi.shape(obj)` returns the shape's type, slots
and kinds, and `dis` shows each attribute site's handler, shape and
version. Tests for the specialiser assert on plans, not on timings.

## 5. Slots hold PyValue; kinds are promises

Note 06 section 3.3 said an F64 slot holds a raw double. That would be a
second representation. Under the section 3.1 encoding a non-NaN float
`PyValue` already is its double bits, and an int32 is tagged in the NaN
space, so a slot holding a `PyValue` loses nothing against a raw slot.
The slot kind is therefore a promise, not a layout: every slot holds a
`PyValue`; a shape whose slot has kind F64 promises that every value
stored there is an inline float, and a store of anything else transitions
to a shape whose slot is BOXED. The load path never tests the tag when
the plan says F64 or I32, which is the gain the JIT wanted. Kinds form a
lattice I32 < F64 < BOXED and only generalise, as V8's field
representations do.

## 6. Instance layout

An instance is `header, slots[capacity]`. No dict word, no pre-header; the
managed-dict pre-header of 6a goes. Dictionary mode (a materialised
`__dict__` that has been mutated through the dict API, a non-string key,
more than the shape limit of attributes) is a shape whose slot 0 holds the
dict, as note 06 section 3.3 says; reading `obj.__dict__` without
mutating through it keeps the shape, as note 04 section 4 designs. Python
`__slots__` are the type's root shape with those names at fixed slots and
no transitions, so `__slots__` classes and ordinary classes are one
mechanism and `LOAD_ATTR_SLOT` goes; `tp_members` stays for C types.
Limits are note 04's: 30 slots and 64 shapes per type, past which the
object enters dictionary mode. Weak references stay in the side table.

## 7. A method call, end to end

`o.m(a)`:

1. `LOAD_ATTR` with the method bit: plan is METHOD under (shape, version).
   The entry records shape, version and the descriptor; the handler is
   `LOAD_ATTR_METHOD_SHAPE`: compare header, compare version, write the
   descriptor and `o` into two slots. Several shapes at one site use the
   four-row poly table through `LOAD_ATTR_SHAPE_POLY`, the one
   polymorphic handler per feedback kind (note 04 coordinator decision 1).
2. `CALL`: `CALL_PY_EXACT_ARGS` guards the function version; the callee
   frame overlaps the argument window (note 06 section 4).
3. Tier 1: the shape compare is hoisted, the version compare becomes a
   code dependency, the descriptor is a constant, and the function version
   lets the callee's IR be inlined with its own feedback.

Attribute stores follow the store plan: `STORE_ATTR_SHAPE` (replace, the
slot kind permitting), `STORE_ATTR_SHAPE_TRANSITION` (add: write the
slot, then the new shape into the header with release order), data
descriptor, generic.

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

1. The shape struct (type, type_version, flags, parent, edge, slot table,
   transitions); the type owns its shapes; every type gets a root shape in
   `PyType_Ready`; the header holds the shape; `Py_TYPE` reads
   `shape->type`; the `Check` macros read `shape->flags`.
2. Instance layout of section 6; the managed-dict pre-header, the
   `PyDictValues` header, shared keys for instances and `--with-shapes`
   are deleted; `__slots__` become shapes.
3. The lookup and store plans; the specialiser rewritten over them;
   typecache becomes the (shape, name) plan cache; inspection in
   `_testinternalcapi` and `dis`.
4. Handlers: `LOAD_ATTR_SHAPE`, `LOAD_ATTR_SHAPE_POLY`,
   `LOAD_ATTR_METHOD_SHAPE`, `STORE_ATTR_SHAPE`,
   `STORE_ATTR_SHAPE_TRANSITION`; `LOAD_ATTR_INSTANCE_VALUE`,
   `LOAD_ATTR_WITH_HINT`, `LOAD_ATTR_SLOT`, `LOAD_ATTR_METHOD_WITH_VALUES`,
   `LOAD_ATTR_METHOD_LAZY_DICT`, `STORE_ATTR_INSTANCE_VALUE`,
   `STORE_ATTR_WITH_HINT` and their guards deleted.
5. Slot kinds as promises, with the transition on a store of another kind.
6. `PyObject_GetAttrValue` and `PyObject_SetAttrValue` execute plans for
   ordinary types.

Recorded, not gated: per loop on richards, raytrace, deltablue, chaos, go
and nbody; the `Py_TYPE` site count and the per-loop cost of the extra
load on a container-heavy benchmark (json_loads); RSS for one million
four-attribute instances; `sys.getsizeof` of a small instance. The gate is
the suite on both builds, the ports, and the plan tests.

## 10. Sources

V8 maps and field representations (Clifford, "Fast properties in V8",
2017; the `Map` and `FieldType` design); JavaScriptCore structures and
watchpoints; SpiderMonkey shapes and `ObjectGroup`; the Self and
Smalltalk inline cache lineage (Hölzle, Chambers, Ungar 1991, PICs).
Note 04 for the shape tree, limits and dictionary mode; note 01 for the
feedback vector.
