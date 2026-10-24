# 04: Shapes (hidden classes) with inline slots (Phase 1, item 1e)

Status: design, 2026-10-23. Line numbers refer to commit `8dc2e138d6d`; no
file cited here differs from `321925c92c9`, the base of notes 01 and 02.
Measurements come from `build/baseline-default/python` with `perf stat -e
instructions:u`, using the method of `boil/tools/micro_loops.py`.

## Decisions

1. A shape is an immutable C struct owned by one heap type: parent, edge label,
   transitions, slot count and a property table of (name, kind) in slot order.
   A property's slot is its position in insertion order.
2. The shape pointer replaces the four header bytes of `PyDictValues` at
   `tp_basicsize`. The block becomes `{shape, capacity, slots[]}` and the
   insertion-order bytes go. A plain four-attribute instance stays 104 bytes.
3. Shapes live exactly as long as their type. Feedback entries borrow shape
   pointers. The type-version guard that precedes every shape compare makes a
   stale pointer harmless, so shapes need neither refcounts nor immortality.
4. Limits: 30 slots per shape (today's `SHARED_KEYS_MAX_SIZE`) and 64 shapes
   per type. Past either, the object enters dictionary mode, which is today's
   `valid == 0` state under a new name.
5. Deleting the property added last returns to the parent shape. Any other
   delete moves to a shape with a hole. Neither leaves the fast paths.
6. `obj.__dict__` stays a live dict that aliases the slots, as split dicts do
   today, and the object keeps its shape.
7. Attribute loads guard the type version and the shape pointer. Method loads
   guard the type version and "not dictionary mode", backed by a per-type name
   set.
8. Every property has a kind byte. Phase 1 writes only BOXED; I32 and F64 are
   reserved, and section 6 shows the byte is enough.
9. `__slots__` stay on `tp_members`.
10. The interpreter gains one polymorphic handler, `LOAD_ATTR_SHAPE_POLY`. All
    other handlers stay monomorphic, as note 01 decided.
11. Eight steps behind `--with-shapes`. The specialisation step waits for typed
    feedback (note 01, step 2).

## 1. What the code does today

| Piece | Where | Behaviour |
|---|---|---|
| Shared keys | `_PyDict_NewKeysForClass`, Objects/dictobject.c:7278 | One split `PyDictKeysObject` per heap type in `ht_cached_keys` (Include/cpython/object.h:289), made by `type_ready_managed_dict` (Objects/typeobject.c:9370) and pre-filled from `__static_attributes__` (dictobject.c:7297-7310). At most 30 keys (Include/internal/pycore_dict.h:260). |
| Inline values | `struct _dictvalues`, pycore_dict.h:269 | Bytes `capacity, size, embedded, valid`, four bytes of padding, `values[capacity]`, then `capacity` insertion-order bytes (:344). Found at `tp_basicsize` (`_PyObject_InlineValues`, Include/internal/pycore_object.h:946). |
| Capacity | `_PyObject_InitInlineValues`, dictobject.c:7325 | `dk_nentries + dk_usable`. `dk_usable` falls by one per new instance until it reaches 1 (:7340-7344), so early instances get up to 29 slots and later ones `nentries + 1`. |
| Absence | `_LOAD_ATTR_INSTANCE_VALUE`, Python/bytecodes.c:2861 | A key in the type's table whose slot is NULL. |
| New name | `insert_split_key`, dictobject.c:1947 | Appends to the type's keys and modifies the type (:1966-1972). |
| `__dict__` | `_PyObject_MaterializeManagedDict`, dictobject.c:7398 | A dict with `ma_keys` = shared keys and `ma_values` = the inline block, stored at `MANAGED_DICT_OFFSET` (pycore_object.h:920). |
| Leaving inline mode | dictobject.c: `dictresize` :2246-2287, `set_dict_inline_values` :7811, `detach_dict_from_object` :7981; `object_set_class_world_stopped`, typeobject.c:7668 | Non-str key, full keys, `popitem` (dictobject.c:5034), `__dict__` assignment or deletion, `__class__` assignment. `valid` drops to 0 and never returns. |
| Fast paths | Python/bytecodes.c | `LOAD_ATTR_INSTANCE_VALUE` (:2879) is `_GUARD_TYPE_VERSION` (:2833), `_CHECK_MANAGED_OBJECT_HAS_VALUES` (:2854) and a load that deopts on NULL. `STORE_ATTR_INSTANCE_VALUE` (:3113) adds `_GUARD_DORV_NO_DICT` (:3078) and appends an order byte on a first store (:3090). `LOAD_ATTR_METHOD_WITH_VALUES` (:4211) checks type version and `valid`. |
| Cache | Include/internal/pycore_code.h:102, :108; Python/specialize.c:634 | Type version and the slot's 16-bit byte offset from the object. |

There is no `_GUARD_KEYS_VERSION` at this commit. The method fast path relies on
`insert_split_key` modifying the type, and its keys-version cache field is
skipped (`unused/2`, bytecodes.c:4216). There is no
`_PyObject_GetInstanceAttribute`; the reader is `_PyObject_TryGetInstanceAttribute` (dictobject.c:7662).

| Loop body, skeleton removed | Instructions | Handler alone |
|---|---|---|
| `z = o` | 36.1 | `LOAD_FAST_BORROW` + `STORE_FAST` |
| `z = o.a` | 71.0 | `LOAD_ATTR_INSTANCE_VALUE` ≈ 35 |
| `z = s.a`, `__slots__` | 68.0 | `LOAD_ATTR_SLOT` ≈ 32 |
| `z = oh.a`, after `oh.__dict__ = dict(...)` | 93.1 | `LOAD_ATTR_WITH_HINT` ≈ 57 |
| `z = od.a`, after `od.__dict__` | 328.2 | unspecialised `LOAD_ATTR` ≈ 292 |
| `o.a = i` | 68.1 | `STORE_ATTR_INSTANCE_VALUE` ≈ 35 |
| `od.a = i`, after `od.__dict__` | 408.0 | unspecialised `STORE_ATTR` |
| `z = C()` minus `z = E()`: four `self.x = k` in `__init__` | 395 | ≈ 99 per statement |

One finding matters for every option below. A site cannot specialise on an
object whose dict exists: `specialize_dict_access` treats the dict as a lost
race (specialize.c:719-725) and the hint path rejects split dicts (:678).
`vars()`, `dir()`, `copy.copy`, `copy.deepcopy` and `pickle.loads` all create
the dict, so attribute access on their results costs four to six times as
much. Loads are correct while the dict aliases the values,
so a small change at specialize.c:719-725 fixes loads (question 5).

## 2. Shape representation

```c
/* Include/internal/pycore_shape.h */
typedef enum { _Py_SLOT_BOXED = 0, _Py_SLOT_I32 = 1, _Py_SLOT_F64 = 2 } _PySlotKind;
typedef enum { _Py_SHAPE_ROOT, _Py_SHAPE_ADD, _Py_SHAPE_DELETE, _Py_SHAPE_KIND } _PyShapeVia;

typedef struct {
    PyObject *name;               /* interned exact str, strong; NULL marks a hole */
    uint8_t kind;                 /* _PySlotKind; BOXED in Phase 1 */
    uint8_t _reserved[7];
} _PyShapeProp;                   /* 16 bytes */

typedef struct _PyShape {
    PyTypeObject *type;           /* owner, borrowed: the type owns the tree */
    struct _PyShape *parent;      /* NULL for the root */
    PyObject *via_name;           /* edge label from parent, strong; NULL for the root */
    uintptr_t transitions;        /* 0, one child (tag 0) or a child array (tag 1) */
    PyDictKeysObject *keys;       /* split keys, built on first __dict__ or slow lookup */
    uint16_t nslots;              /* slots in use, holes included */
    uint16_t nprops;              /* live properties */
    uint8_t via;                  /* _PyShapeVia */
    uint8_t flags;                /* _Py_SHAPE_DICT_MODE (sentinel), _Py_SHAPE_HAS_UNBOXED */
    uint16_t id;                  /* per-type serial for gdb and stats */
    _PyShapeProp props[];         /* nslots entries; slot order is insertion order */
} PyShape;                        /* 48 bytes + 16 per slot */

typedef struct {                  /* replaces PyHeapTypeObject.ht_cached_keys */
    PyShape *root;
    PyDictKeysObject *names;      /* every name in any shape of the type; no values */
    uint16_t nshapes;             /* at most _Py_SHAPE_MAX_PER_TYPE */
    uint8_t max_slots;            /* longest shape; floor len(__static_attributes__) */
    uint8_t slack;                /* today's dk_usable decay, 29 - max_slots down to 1 */
    PyMutex mutex;                /* free-threaded build: guards transitions */
} _PyTypeShapes;

extern PyShape _PyShape_DictMode; /* static sentinel: nslots 0, _Py_SHAPE_DICT_MODE */
```

**Flags.** Dictionary mode is one static sentinel. Fast paths compare against a
cached real shape, so the sentinel misses without a separate test. "Dict
materialised" is not a shape flag: it belongs to one object, and making it a
transition would give every load site two shapes per layout. The managed-dict
word keeps that role. There is no "frozen" flag: Python has no
`Object.freeze`, and the per-type counter `nshapes` decides when transitions
stop.

**Names and lifetime.** Names are interned mortal strings, as `type_setattro`
interns them (typeobject.c:6675), so transition lookup compares pointers. A str
subclass name enters dictionary mode, as it leaves split tables today. The root
is made where `ht_cached_keys` is made (typeobject.c:9383); the tree is freed
where `_PyDict_RemoveKeysForClass` runs (:6937). Shapes are `PyMem`
allocations, not Python objects. The invariant is that an object's shape is the
sentinel or belongs to `Py_TYPE(obj)`; the object holds its type, so its shape
is alive. Slow-path name lookup scans `props` when `nslots <= 8` and uses
`keys` otherwise.

| Limit | Value | Reason | Past the limit |
|---|---|---|---|
| Slots per shape, holes included | 30 | today's `SHARED_KEYS_MAX_SIZE`; capacity policy and memory stay comparable | that object enters dictionary mode |
| Shapes per type | 64 | the benchmarks in section 10 need at most 12 | no new shapes; the object enters dictionary mode; existing shapes keep working |
| Names per type | 64 | bounds type invalidations from new names (5.3) | the type stops getting `METHOD_WITH_VALUES` |
| Polymorphism | 4 | V8's limit and note 01's | megamorphic: generic path |

**Where the pointer lives.**

| Location | Fast path | Memory | Verdict |
|---|---|---|---|
| `ob_type`, the shape pointing at the type (V8 puts the map in word 0) | one load, no type guard | none | breaks every `Py_TYPE` user; belongs to the C API redesign, item 2a |
| Third pre-header word | still needs a type guard: reading below an object without a pre-header is out of bounds | +8 bytes on every pre-header object; changes `_PyType_PreHeaderSize` (pycore_object.h:850) and `sys.getsizeof` | rejected |
| The managed-dict word | same | the dict pointer must move, but `_PyObject_GetDictPtr` (Objects/object.c:1609) returns that word's address | rejected |
| The `PyDictValues` header at `tp_basicsize` | type guard, then a load at a cached offset | header 8 to 16 bytes; order bytes (8 or more) removed | **chosen** |

```c
/* Default build, 64-bit. class C: def __init__(s): s.a = s.b = s.c = s.d = 0 */
/* -32 */ PyObject *weakreflist;      /* MANAGED_WEAKREF_OFFSET, unchanged */
/* -24 */ PyDictObject *dict;         /* MANAGED_DICT_OFFSET, NULL until __dict__ */
/* -16 */ PyGC_Head gc;
/*   0 */ ob_refcnt, ob_flags; PyTypeObject *ob_type;
/*  16 */ /* tp_basicsize: __slots__ members and C fields end here */
typedef union { PyObject *obj; double f64; int64_t i64; } _PyShapeSlot;
typedef struct _dictvalues {          /* PyDictValues, redefined; at tp_basicsize */
    PyShape *shape;                   /* +0: the guard reads this */
    uint8_t capacity;                 /* +8: slots allocated */
    uint8_t _reserved[7];
    _PyShapeSlot slots[];             /* +16: slot i is property i of the shape */
} PyDictValues;                       /* no insertion-order bytes */
```

| | V8 map | CPython shared keys (today) | Proposed shape |
|---|---|---|---|
| Unit shared | map per object layout | one keys table per type | shape per layout, per type |
| Pointer in object | word 0 | none; `valid` byte | inline block header |
| Name to slot | descriptor array, shared along a chain | hash table, fixed index per name | property table; slot = insertion position |
| Presence | map | non-NULL slot | shape |
| Insertion order | map | per-object order bytes | shape |
| Delete | last property rolls back; others go to dictionary mode | NULL slot, order bytes updated | last rolls back; others leave a hole |
| Field kinds | Smi, Double, HeapObject, Tagged, generalised by deprecation | none | BOXED; I32 and F64 reserved |
| Fallback | dictionary ("slow") mode | combined dict, `valid = 0` | dictionary mode sentinel |
| Lifetime | GC heap object, weak from ICs | refcounted, owned by type and dicts | owned by type, borrowed by caches |
| Cache key | map | type version + offset | type version + shape + offset |
| Capacity | in-object slack tracking | per object, decaying | per object, today's policy |

## 3. Transitions

**Add.** `_PyShape_Add(S, name, kind)` follows or creates the (ADD, name, kind)
edge. The new slot is `S->nslots`, so the same insertion order always yields the
same shape. The census in section 10 finds one insertion order per class in
every benchmark. A name new to the type joins `names` and modifies the type
(5.3). The object needs `nslots + 1 <= capacity`.

**Delete.** If `S` was reached by adding `name`, the object returns to
`S->parent`. Otherwise it follows a (DELETE, name) edge to a shape with that
property's name set to NULL; `nprops` drops and `nslots` does not. Re-adding
appends a new slot, which gives dict semantics: a deleted and re-added key goes
last (test_dict `test_splittable_del`, `test_splittable_pop`). Holes count
against capacity, so a delete-heavy object ends in dictionary mode instead of
growing. V8 sends non-last deletes to dictionary mode. We do not, because
today's split table keeps such objects inline, and `del self.x` or a
`cached_property` reset would regress.

**Change kind** (Phase 2). A (KIND, name, BOXED) edge gives the same layout with
one kind generalised. Kinds move only towards BOXED: I32 to BOXED and F64 to
BOXED. I32 to F64 is wrong because `int` and `float` are distinct types.

**`__class__` assignment** enters dictionary mode, as today
(typeobject.c:7668-7690). Shapes are per type, not shared across compatible
types: a shape then implies its type, which lets tier 1 drop the version check
under a type watcher and keeps lifetimes trivial. Replaying the shape into the
new type's tree is possible later; no benchmark assigns `__class__`.

| Event | Today | Shapes |
|---|---|---|
| No room in capacity or 30 slots | combined dict | dictionary mode |
| 65th shape of a type | does not arise | dictionary mode for that object |
| Non-str or str-subclass key via `__dict__` | `dictresize` to combined | dictionary mode |
| `obj.__dict__.popitem()` | `dictresize` (dictobject.c:5034) | dictionary mode |
| `obj.__dict__ = d`, `del obj.__dict__` | inline values cleared | dictionary mode; an aliasing dict is detached first |
| `obj.__class__ = T` | detach | dictionary mode |
| `del obj.x`, `__dict__.pop`, `__dict__.clear()` | stays inline | stays shaped: parent, hole or root |

## 4. Dictionary materialisation

The materialised dict reads through the shape; the object does not enter
dictionary mode. `vars()`, `dir()` (typeobject.c:8416), `copy._reconstruct`
(Lib/copy.py:263), `pickle`'s `load_build` (Lib/pickle.py:1918) and the default
`__getstate__`, which returns the live dict (typeobject.c:7827), all create it.
Permanent dictionary mode would slow every copied or unpickled object, and
test_class `TestInlineValues.test_inspect_dict` and `test_update_dict` expect
the object to stay inline.

`ma_values` points at the object's block, as now. `ma_keys` is `shape->keys`:
an immutable `DICT_KEYS_SPLIT` table whose entry i names slot i, with no index
entry for holes. Split-table lookups therefore work unchanged. The dict reaches
the owner's shape through `ma_values->shape`, so it needs no back pointer.
Iteration walks slots `0..nslots-1` and skips NULL; the order bytes and
`get_index_from_order` (dictobject.c:672) go.

| Operation | Result |
|---|---|
| `obj.__dict__`, `vars(obj)`, `_PyObject_GetDictPtr`, `PyObject_GenericGetDict` | build `shape->keys` if absent; create the dict; store it at `MANAGED_DICT_OFFSET`; shape unchanged |
| `d[k] = v`, `k` present | write the slot; send the watcher event |
| `d[k] = v`, `k` a new exact str | `_PyShape_Add`; write the slot; update `ma_values->shape`, `ma_keys`, `ma_used`; on refusal, convert to combined |
| `del d[k]`, `d.pop(k)` | delete transition |
| `d.clear()` | set the root shape before releasing values, so reentrant finalisers see an empty object (test_dict `test_clear_reentrant_*`) |
| `d.copy()`, `dir(obj)`, `dict(d)` | combined dict in slot order; shape keys are immutable, so a copy cannot share them as today's split copies share keys (dictobject.c:4502) |
| `copy.copy(obj)` | `y.__dict__.update(state)` replays x's additions; y reaches x's shape through cached edges |
| Owner dies, or `__dict__` is reassigned, while the dict lives | detach: convert the dict to combined (today `copy_values`, dictobject.c:7981) |
| `STORE_ATTR` while the dict exists | slow path keeps the dict in step; replacing stores stay fast if the dict has no watchers (5.4) |

Non-embedded split dicts disappear. Copies become combined, and managed-dict
types with items (tuple and int subclasses, which have no inline values,
typeobject.c:9390) get ordinary dicts instead of `new_dict_with_shared_keys`
(dictobject.c:1007). Those instances lose key sharing; in return the shared-key
locking in `insert_split_key` and `shared_keys_usable_size` goes.

## 5. Inline cache and guards

**5.1 The type guard stays.** A site can see any object, and the shape sits at a
type-dependent offset, so reading it before the type check could read past the
end of a smaller object.
A data descriptor added to the class after specialisation must also win
(test_opcache `TestLoadAttrCache.test_descriptor_added_after_optimization`);
adding one modifies the type, not its instances. The shape guard replaces the
`tp_basicsize` load, the `valid` test and the NULL test.

**5.2 Feedback entry.** Note 01's `_PyFbAttr` grows from 40 to 48 bytes: `_pad`
becomes `shape_offset` (the owner's `tp_basicsize`), the `descr` union gains
`shape_to`, and a `PyShape *shape` field follows. `_PyFbPoly` gains
`PyShape *shape[4]`, with `aux[i]` holding the shape offset and `index[i]` the
slot offset. Before typed feedback lands, the 9-unit `LOAD_ATTR` cache fits a
shape load, but the 4-unit `STORE_ATTR` cache cannot hold a transition; hence
step 5 waits for note 01's step 2.

| Specialisation | `type_version` | `shape` | `shape_offset` | `slot_offset` | `descr` union |
|---|---|---|---|---|---|
| `LOAD_ATTR_SHAPE` | yes | yes | yes | yes | |
| `LOAD_ATTR_SHAPE_POLY` | per entry in `_PyFbPoly` | per entry | per entry | per entry | |
| `STORE_ATTR_SHAPE` (replace) | yes | yes | yes | yes | |
| `STORE_ATTR_SHAPE_TRANSITION` (add) | yes | from | yes | yes | `shape_to` |
| `LOAD_ATTR_METHOD_WITH_VALUES`, `..._NONDESCRIPTOR_WITH_VALUES` | yes | | yes | | `descr` |

**5.3 Loads.**

```c
op(_GUARD_SHAPE, (shape_offset/1, shape/4, owner -- owner)) {
    PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
    PyShape *s = FT_ATOMIC_LOAD_PTR_ACQUIRE(*(PyShape **)((char *)owner_o + shape_offset));
    EXIT_IF(s != (PyShape *)shape);
}

op(_LOAD_ATTR_SHAPE_SLOT, (slot_offset/1, owner -- attr, o)) {
    PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
    PyObject **slot = (PyObject **)((char *)owner_o + slot_offset);
    PyObject *attr_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(*slot);
#ifdef Py_GIL_DISABLED
    DEOPT_IF(attr_o == NULL);                   /* a racing delete */
    DEOPT_IF(!_Py_TryIncrefCompareStackRef(slot, attr_o, &attr));
#else
    assert(attr_o != NULL);                     /* the shape guarantees presence */
    attr = PyStackRef_FromPyObjectNew(attr_o);
#endif
    o = owner;
    DEAD(owner);
}

macro(LOAD_ATTR_SHAPE) =
    unused/1 + _GUARD_TYPE_VERSION + _GUARD_SHAPE + _LOAD_ATTR_SHAPE_SLOT +
    POP_TOP + _PUSH_NULL_CONDITIONAL;
```

`LOAD_ATTR_SHAPE_POLY` checks up to four (type version, shape) pairs in order
and loads at the matching slot offset. It exists because shared keys keep this
pattern monomorphic today:

```python
class Token:
    def __init__(self, kind, value=None):
        self.kind = kind
        if value is not None:
            self.value = value
        self.pos = 0
```

Here `pos` has slot 2 in one shape and slot 1 in the other. A monomorphic shape
cache misses whenever consecutive tokens differ and falls to the
292-instruction generic path.

Method loads cannot guard an exact shape, since methods are called on objects
of every shape of a class. `LOAD_ATTR_METHOD_WITH_VALUES` instead guards the
type version and `shape != &_PyShape_DictMode` (`_CHECK_SHAPED`, replacing
`_CHECK_MANAGED_OBJECT_HAS_VALUES`). It is correct because `names` holds every
name in any shape of the type, the specialiser requires the method's name to be
absent from it, and adding a new name modifies the type, as `insert_split_key` does
today. `names` starts with `__static_attributes__`, so ordinary classes pay no
invalidations. Past 64 names the type stops getting this specialisation.

**5.4 Stores.** `STORE_ATTR_SHAPE` guards type version and shape, and allows a
materialised dict without watchers, because replacing a value changes neither
`ma_keys` nor `ma_used`. `STORE_ATTR_SHAPE_TRANSITION` guards type version,
`from`, no dict and `slot < capacity`, writes the slot, then stores `shape_to`
with release order. Both run under `LOCK_OBJECT`, as `STORE_ATTR_INSTANCE_VALUE`
does (bytecodes.c:3109).

**5.5 Lifetime.** Fast paths compare cached shape pointers and never
dereference them. Heap-type version tags are never reused within an
interpreter, so after a type dies its version matches no live object, and the
shape compare is never reached. `shape_to` belongs to the same tree as `from`,
so a passing guard proves it alive. Tier 1 may embed shapes under the same rule
that note 01 sets for `descr`.

**5.6 Free-threaded build.** Shapes are immutable once published. Transitions
are created under `_PyTypeShapes.mutex` and published with release stores.
Slot and shape writes stay inside the object's critical section. Per the
coordinator's decision on note 02, this build gets correctness, not tuning.

## 6. Slot kinds

Phase 1 writes only BOXED, and the kind byte costs nothing on fast paths,
because a guarded shape fixes every slot's kind. The byte suffices for Phase 2:

- **Storage.** A slot is eight bytes and holds `PyObject *`, a raw IEEE double
  or an int32 sign-extended to 64 bits (`_PyShapeSlot`, section 2).
- **F64.** Holds an exact, non-NaN `float`. A load builds the stack value with
  `PyStackRef_FromDoubleNotNaN` (note 02, 2.1) and allocates nothing. Storing a
  non-float or a NaN takes a KIND edge to BOXED. NaN stays boxed so that
  `o.x is o.x` keeps holding under note 02's value identity (12.2).
- **I32.** Holds an exact `int` within int32, matching note 02's inline ints.
- **GC.** Traversal visits BOXED properties only. `_Py_SHAPE_HAS_UNBOXED` lets
  all-boxed shapes keep a plain loop.
- **C API.** Before a dict is built (`__dict__`, `_PyObject_GetDictPtr`,
  `__getstate__`), every slot is generalised to BOXED; while the dict exists,
  transitions create only BOXED properties. Dict code and borrowed-reference
  calls such as `PyDict_GetItem` never see a raw slot. Slow-path reads box.

Phase 2 must add one mechanism: after a KIND edge, the old shape is deprecated
with a forwarding pointer, so a site does not see both shapes (V8's map
deprecation). It adds a field to `PyShape`. Nothing in Phase 1 depends on it.

## 7. `__slots__` classes

Keep them on `tp_members`. Slot members already sit at fixed offsets and are
monomorphic per type; `LOAD_ATTR_SLOT` costs 32 instructions against 35 for
instance values. Member descriptors are visible API: `C.a` is a
`member_descriptor`, `__getstate__` returns a `(dict, slots)` pair
(typeobject.c:7827), and `copyreg._slotnames` drives pickling. A slots-only
instance has no inline block, so unification would add 16 bytes to each.
Classes with both keep slots before `tp_basicsize` and the shaped block after
it. One cost: the `float` benchmark's `Point` uses `__slots__`, so it gains
nothing from F64 shapes. Typed members are the route there; the `Py_T_DOUBLE`
member type already exists (question 4).

## 8. GC and memory

| Four-attribute instance, steady state (capacity 5) | Today | Shapes |
|---|---|---|
| Weakref and dict words | 16 | 16 |
| `PyGC_Head` | 16 | 16 |
| `PyObject` header | 16 | 16 |
| Inline header | 8 | 16 |
| Slots | 40 | 40 |
| Insertion-order bytes | 8 | 0 |
| Total (mimalloc bin) | 104 (112) | 104 (112) |
| First instance of a new class (capacity 29) | 328 | 296 |
| `sys.getsizeof` | 48 | 48 |

`PyObject_VisitManagedDict` (dictobject.c:7775) visits the dict if one exists,
else slots `0..nslots-1` rather than `0..capacity-1`. It must never visit both,
or the GC subtracts an aliased reference twice (the GH-130327 rule;
test_dict `test_overwrite_managed_dict`). `gc.get_referents(obj)` therefore
returns what it returns now: slot values or the dict, plus the type. Shapes
are not objects and never appear. `sys.getsizeof` keeps omitting the inline
block, as it does today (question 3).

Per type, today's keys table costs 768 bytes (test_sys formula). A single chain
of n properties costs `48(n+1) + 8n(n+1)` bytes: 400 for n = 4 and 1,632 for
n = 11, plus `names`. V8 shares descriptor arrays along a chain to make this
linear; we defer that until step 5's RSS check asks for it.

## 9. Compatibility and ABI

| Observer | Change |
|---|---|
| `Py_TPFLAGS_INLINE_VALUES` (Include/object.h:472), `tp_dictoffset == -1` | none; the flag now means "has a shaped block" |
| `_PyObject_GetDictPtr`, `PyObject_GenericGetDict`, `PyObject_GenericSetDict` | none visible; they return a shape-backed dict |
| `PyObject_VisitManagedDict`, `PyObject_ClearManagedDict` (Include/cpython/object.h:390) | same API; the implementation reads the shape |
| `PyDictObject.ma_values` (Include/cpython/dictobject.h:32) | still "non-NULL means split"; it points at the owner's block |
| `PyHeapTypeObject.ht_cached_keys` | replaced by `ht_shapes`, a pointer of the same size |
| `_Py_DebugOffsets` `ht_cached_keys`, `ma_values` (pycore_debug_offsets.h:169, :198) | add shape offsets and bump the version |
| Tools/gdb/libpython.py:513-531 | reads `PyDictValues` and `ht_cached_keys`; must read shapes |
| `type.__sizeof__` (typeobject.c:7066) | counts the tree instead of the keys |
| `_testinternalcapi.get_object_dict_values`, `has_split_table` | redefined; their test_opcache and test_class expectations change |

Ports, searched in `build/ports/work/cpython-3.16-refcnt-v0`: numpy 2.5.4 sets
positive `tp_dictoffset` on its own types (`ufunc_object.c:6976`,
`arrayfunction_override.c:763`) and calls `_PyObject_GetDictPtr` only in
pythoncapi-compat fallbacks for Python below 3.13 (`pythoncapi_compat.h:923`,
`:934`). cffi 2.1.1 sets a positive offset on `LibObject` (`lib_obj.c:624`).
Cython 3.3 output (pyyaml's `_yaml.c`; numpy uses the same Cython) sets
`Py_TPFLAGS_MANAGED_DICT` on `CyFunction` (`:32664`), so it gets shapes, and uses
only `_PyObject_GetDictPtr`, `PyObject_GenericGetDict` and the managed-dict
visit and clear calls (`:31856`, `:32287-32353`). Its dict-version code that
reads `tp_dictoffset` (`:29006`) is compiled out for 3.12 and later
(`:409-410`). No port reads `ma_values`, `ht_cached_keys`,
`MANAGED_DICT_OFFSET` or memory past `tp_basicsize`. No patch and no `abi_id`
change are needed. In the tree, asyncio's Future and Task
(Modules/_asynciomodule.c:1749, :2945) and the typing TypeVar types
(Objects/typevarobject.c:941, :1512, :1839) get shapes through
`type_ready_managed_dict`.

## 10. Measurement

Census of attribute insertion orders, recorded at every `__init__` return and
over all live instances at the end of a reduced run of each benchmark:

| Benchmark | Instance classes | Longest chain | Orders per class | Phase 1 | Phase 2 kinds |
|---|---|---|---|---|---|
| richards | 11 | 8 | 1 | neutral | I32 only |
| raytrace | 9 (Vector, Point: x, y, z floats) | 6 | 1 | neutral | F64: largest candidate |
| deltablue | 9 | 7 | 1 | neutral | I32 only |
| chaos | 3 (GVector: x, y, z floats) | 10 | 1 | neutral | F64: moderate |
| go | 5 (Square, Board, UCTNode) | 11 | 1 | neutral | I32 only |
| float | `Point` has `__slots__` | | | none | none without typed slots |
| nbody | none: lists of floats | | | none | none |

Each class has one insertion order, so shapes add no polymorphism beyond the
per-type polymorphism that sites such as a shared base `__init__` already have.
The interpreter gains little, and a model with gcc -O2 on x86-64 shows how
little:

| Hit path, guards to incref | Instructions | Dependent loads before the last guard |
|---|---|---|
| Today: version, `tp_basicsize`, `valid`, NULL | 16 | 3 |
| `LOAD_ATTR_SHAPE` | 15 | 2 |
| Tier 1: `ob_type == T` under a type watcher, then shape | 12 | 1 |

One instruction of a 35-instruction handler will not show on pyperformance.
Phase 1's gate is therefore neutrality: richards, raytrace, deltablue, chaos and
go each within 1 percent, and RSS within 1 percent (`boil/tools/rss.py`). The
gains come later. In tier 1, `self.x * o.x + self.y * o.y` needs two guards
instead of a type check, a `valid` check and a NULL check per field, because
presence follows from the shape. In Phase 2, F64 slots remove the float box on
every store in raytrace and chaos, which note 02 leaves boxed. Loads on objects
with a materialised dict, such as copies and unpickled objects, cost 35
instructions instead of 292.

## 11. Sequence

| # | Step | Tests | Benchmarks |
|---|---|---|---|
| 1 | Harness: section 1 rows in `boil/tools/micro_loops.py`; `boil/tools/shape_census.py`; `_testinternalcapi.get_shape_info` | the tools reproduce sections 1 and 10 | baseline JSON in `boil/bench/` |
| 2 | `--with-shapes` defines `Py_SHAPES`. `Objects/shapeobject.c`: roots, ADD and DELETE edges, limits, `names`, freeing with the type, the free-threaded mutex. No instance uses them yet | new `Lib/test/test_shapes.py` through `_testinternalcapi`: same order gives the same shape; delete-last gives the parent; holes; the 30-slot and 64-shape caps; interning of non-interned names; refleak runs freeing trees | none |
| 3 | Instances under the flag: the `{shape, capacity}` block, root at allocation (`_PyType_AllocNoTrack`, typeobject.c:2482; `_PyObject_GC_New`, Python/gc.c:2047), slow get, set and delete through shapes, the sentinel in place of `valid`, GC visit and clear. Interim: `__dict__` enters dictionary mode; instance-value specialisations are off; `_CHECK_SHAPED` replaces the method guard | full suite, default and free-threaded, with the flag: test_class, test_descr, test_dict, test_gc, test_copy, test_pickle, test_weakref, test_sys; test_shapes covers every row of section 3's table and insertion order after holes. Tests that pin shared keys get flag-aware expectations | slower under the flag; not gated |
| 4 | Shape-backed `__dict__`: lazy `shape->keys`, insert, delete, clear, iterate, copy, detach | test_dict `test_splittable_*`, `test_split_table_*_str_subclass`, `test_clear_reentrant_*`, `test_overwrite_managed_dict`; test_descr `test_set_dict`, `test_set_class`, `test_copy_setstate`, `test_dir`; test_class `TestInlineValues`; test_opcache `TestInstanceDict`; test_copy; test_pickle; test_free_threading.test_races | deepcopy |
| 5 | Specialisation, after note 01 step 2: `LOAD_ATTR_SHAPE`, `LOAD_ATTR_SHAPE_POLY`, `STORE_ATTR_SHAPE`, `STORE_ATTR_SHAPE_TRANSITION`; `_PyFbAttr` to 48 bytes; `_PyFbPoly.shape[4]` | test_opcache: every LOAD_ATTR and STORE_ATTR case, `TestRacesDoNotCrash`, descriptor-after-optimisation; test_dis; test_generated_cases; test_shapes: the `Token` pattern stays specialised | `z = o.a` no worse than today's 71 instructions; the five benchmarks within 1 percent; RSS within 1 percent |
| 6 | Tools: gdb's libpython, `_Py_DebugOffsets`, `_testinternalcapi` helpers | test_gdb, test_external_inspection; `ports check` for all seven ports | none |
| 7 | Flip: shapes on by default; `--without-shapes` survives one step | full suite on default, debug and free-threaded builds; pyperformance against `boil/bench/default.json`; RSS | as step 5 |
| 8 | Delete `ht_cached_keys`, shared-key insertion, order bytes, `valid` and `embedded`, `SHARED_KEYS_MAX_SIZE`, non-embedded split values, `_CHECK_MANAGED_OBJECT_HAS_VALUES`, `_GUARD_DORV_NO_DICT`, the `*_INSTANCE_VALUE` instructions and the flag | full suite | neutral |

## 12. Open questions for the coordinator

1. **Interpreter polymorphism.** Note 01 keeps the interpreter monomorphic.
   This note adds `LOAD_ATTR_SHAPE_POLY` so that classes with optional
   attributes do not fall from 35 to 292 instructions. Accept the exception?
2. **Free-threaded build.** Ship shapes there too, with the coarse locking of
   5.6, or keep inline values in that build until plan step 2c? The second
   choice delays step 8 until then.
3. **`sys.getsizeof`.** Keep omitting the inline block, as today, or count
   header and capacity? Counting is accurate and changes test_sys values.
4. **Typed `__slots__`.** Should Phase 2 give member slots the same kinds, so
   the `float` benchmark gains? Shapes alone cannot reach it.
5. **The specialiser fix.** Land the specialize.c:719-725 change for loads
   on objects with a dict first, on today's code, so that shapes are not
   credited with its gain?

## Coordinator decisions (2026-10-23)

1. **Polymorphic handler.** Allowed. The feedback entry already carries a
   four-entry polymorphic table, and `LOAD_ATTR_SHAPE_POLY` is the one
   handler that reads it. Note 01's monomorphic rule is amended to "one
   polymorphic handler per feedback kind, at most".
2. **Free-threaded build.** Keeps inline values. Shapes are a default-build
   feature. Step 8 (removing the old code) waits for plan step 2c, when the
   free-threaded build is dropped.
3. **`sys.getsizeof`.** Unchanged in Phase 1.
4. **`__slots__` kinds.** Yes, in Phase 2. Members declared through
   `__slots__` get the same kind byte so struct-of-floats classes gain
   without a rewrite to shapes.
5. **The `specialize.c` gap** (objects whose `__dict__` has been
   materialised never specialise) is fixed first, on today's code, as part
   of the interpreter-mechanics batch (plan item 1d). Shapes are not
   credited with it.

Priority change: because the interpreter gain is neutral, shapes move to the
end of Phase 1, after the IR interpreter (note 01 step 4) and typed feedback
exist, so the first consumer of shape guards is the code that benefits. The
Phase 1 gate for shapes is neutrality on richards, raytrace, deltablue, chaos
and go, within 1 percent on time and RSS.
