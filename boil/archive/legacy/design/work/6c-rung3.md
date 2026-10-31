# 6c rung 3: the instance layout

Status: design, 2026-10-27. Specification: note 09 section 6, section 4.5
(the store plan) and section 9 item 3. Before: rung 2 (80ba2edf94c).
Coordinator rulings already in force:

- option A for `__dict__`;
- capacity `min(30, max(6, S + 2))`;
- a computed dict offset for variable-size managed-dict types;
- DICT plans cached with a keys hint;
- `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` stay for members.

From rung 2 this rung also inherits two items:

- the store prepare and commit split, with NOMEMORY and LIMIT as
  distinct results;
- the refusal to specialise a store over a non-data class attribute,
  which stays, with a test that the generic path takes it.

Rungs 1 and 2 put the shape in the header and made lookup a plan over
it, but the storage underneath is still CPython's. An instance has a
two-word pre-header holding its dict pointer. Its inline values sit
behind a `PyDictValues` header (capacity, size, embedded, valid, then
the insertion-order bytes), and they are indexed by the type's shared
keys, so a slot number is a key index, not a shape position. A dict may
alias the values. Rung 2's storage adapter hides all of this. This rung
replaces that storage, deletes the adapter, and makes the shape the only
description of an instance.

## What changes

### 1. The type: capacity and flags

- **Capacity** is fixed when a heap type with inline values is readied:
  `min(30, max(6, S + 2))`. S is the number of distinct names in the
  `__static_attributes__` tuples of the classes on the MRO, so S is 0
  for a type from `PyType_FromSpec` or `type()` with no such tuple. It
  is stored in the type's shape table (`_PyTypeShapes.capacity`, a
  `uint8_t`). Every instance is allocated with exactly that many slots,
  and the number never changes.
- **`Py_TPFLAGS_INLINE_VALUES`** keeps its bit, with a new meaning:
  "instances have `capacity` shape slots at `tp_basicsize`". It is set,
  as now, for a `Py_TPFLAGS_MANAGED_DICT` type with `tp_itemsize == 0`.
  That covers Python classes and the C heap types that ask for a managed
  dict: TypeVar, ParamSpec, TypeVarTuple, `_asyncio.Future` and `Task`,
  and the `_testcapi` types.
- **`Py_TPFLAGS_MANAGED_DICT`** stays the public request "the runtime
  places my dict". With inline values the dict is slot 0 in dictionary
  mode, and `tp_dictoffset` stays -1, which now means "managed: slot 0
  in dictionary mode" instead of "the pre-header". Code that reads
  `tp_dictoffset` without testing the flag already has to treat -1 as a
  sentinel, so nothing outside the core sees a different contract.
  `Py_TPFLAGS_PREHEADER`, which is the same bit today, is deleted.
- **A variable-size type with a managed dict** is section 5.
- **`ht_cached_keys` is deleted** from `PyHeapTypeObject`, together with
  `_PyDict_NewKeysForClass`, its traverse and its removal, the
  shared-keys bump of the type version in `insert_split_key`, and the
  keys' share of `type.__sizeof__`. `__static_attributes__` is used only
  for S above.

### 2. The layout and allocation

- **An instance of an inline-values type** is the header word, then the
  native base's fields, then any `__slots__` members (all before
  `tp_basicsize`), then `capacity` slots of `PyValue`. Slot i is at byte
  `tp_basicsize + 8*i`. There is no pre-header, no `PyDictValues`
  header, no capacity byte, no order bytes and no `valid` flag.
- **In shape mode** slot i holds the shape's i-th field, or NULL for a
  hole. Slots at or past `shape->nslots` are NULL. **In dictionary
  mode** (the type's dictionary-mode shape) slot 0 holds the instance
  dict or NULL (empty), and the other slots are NULL.
- **Allocation.** `_PyType_AllocNoTrack` and `_PyObject_GC_New` add
  `capacity * sizeof(PyValue)` and zero it. Both write the root shape,
  as now. The two pre-header words, `presize`, and the mimalloc
  `GC_PRE` heap tag go, along with the code that knows about them:
  - `gc_alloc`, `_PyObject_GC_Resize` and `PyObject_GC_Del`;
  - `obmalloc`'s `gc_block_object` and the `GC_PRE` object-heap case;
  - `gc_trace.c`'s `heap_offset` and `tag_offset`, and
    `_PyGCDeadObject.presize`;
  - the sweep's `op - presize`;
  - tracemalloc's two steps back by the pre-header size;
  - `_PyObject_AssertFailed`'s traceback lookup.

  The address map becomes one rule. An object's address is its
  mimalloc block's address plus the allocator's `debug_offset`, which is
  0 without the debug hooks and two words with them (`mimalloc_debug`,
  or a release build with `PYTHONMALLOC=debug`). The rule holds only in
  the two object heaps (OBJECT and GC). `tag_offset` stays, with
  `debug_offset` for those two tags and -1 for MEM and NOSCAN, so a
  conservative pointer into a MEM or NOSCAN page is never read as an
  object. Freeing passes the allocator the object's address, which is
  the user pointer the debug hooks expect; the hooks find their own
  header from it. The sweep's block address and the dead record's
  `presize` go.
- **Alignment.** The pre-header was two words, so that putting it in
  front of an object kept the object at the block's alignment (ctypes'
  `CDataObject` holds long doubles). Without a pre-header the object
  starts at its block, so it has exactly the block's alignment, and
  nothing replaces the padding. test_ctypes is in the ladder.
- **Sizes.**
  - `object.__sizeof__` adds `capacity * 8`. The native `__sizeof__`
    overrides that start from `_PyObject_SIZE(Py_TYPE(self))` (list,
    dict, set, bytearray and the like) start instead from one helper,
    `_PyObject_InstanceSize(self)`, which adds the slots. It does not count inline
    values today, so `sys.getsizeof` of an instance has been too small.
  - `sys.getsizeof` drops the pre-header term.
  - `_testinternalcapi.SIZEOF_MANAGED_PRE_HEADER` and
    `SHARED_KEYS_MAX_SIZE` are deleted, and the tests that use them are
    rewritten.

### 3. Shapes

- **Slots are positions.** A slot is the field's position in its shape,
  no longer a shared-key index. `_PyShape_PrepareAdd(from, name, &to)` follows or
  creates the (ADD, name) edge. The new field is slot `from->nslots`, so
  the same insertion order gives the same shape. Slot names are interned
  exact `str`s, held by the shapes and visited by the type's traverse
  (rung 1: through `edge_name`, since every slot name is an ancestor's
  edge name).
- **Deletes** follow note 04 section 3.
  - Deleting the field the last edge added returns to `parent`.
  - Any other delete follows a (DELETE, name) edge to a shape of a new
    kind, `_Py_SHAPE_DELETE`, with that slot's name NULL. It has the
    same `nslots` and its parent is the shape the delete started from.
  - A re-added name is appended as a new slot, which keeps dict order.
  - Holes count against capacity.
- **Lookup of a name in a shape** is `_PyShape_SlotOf(shape, name)`. It
  scans `slots` by identity, which suffices for an interned name, and by
  `unicode_eq` for a non-interned exact `str`. It runs no user code. A
  name that is not an exact `str` never takes a shape slot. Its store
  converts to dictionary mode first, as today's `insert_split_key`
  refusal does. Its load is uncached and classified DICT; when the
  executor reaches instance storage with it and the object is in shape
  mode, it converts first (as CPython materialises the dict today), then
  looks the name up in the dict, so a subclass's `__hash__` and `__eq__`
  run as they do now. A data descriptor still wins before storage is
  reached. A conversion failure raises MemoryError from the load and
  leaves the object unchanged.
- **Limits.** A transition is refused (LIMIT) at `capacity` slots
  (holes included) or at 64 shapes per type. Capacity is at most 30, so
  the 30-slot cap is the capacity cap. Hole shapes count among the 64.
- **Preparation** (section 4.5 of note 09) is
  `_PyShape_PrepareAdd(from, name, &to)` and
  `_PyShape_PrepareDelete(from, slot, &to)`. Each returns OK, LIMIT, or
  NOMEMORY (with MemoryError set). They are the only functions that
  allocate a shape. `_PyShape_Add(from, slot)` (rung 1's signature) and
  `slot_name` go.
- **Kinds** stay BOXED (rung 4 adds I32 and F64).
- **The dictionary-mode shape** stays embedded in the table. A type
  without a table has only its root shape.

### 4. Storage: execution of plans

The rung-2 adapter (`load_from_storage`'s DICT branch, `instance_has_key`,
`_PyObject_TryGetInstanceAttribute[Value]`, `_PyObject_StoreInstanceAttribute`)
is deleted. Plans now describe the storage exactly.

- **Phase 2.**
  - `_PyShape_ResolveLoad` gives SLOT when `_PyShape_SlotOf` finds the
    name. The offset is `tp_basicsize + 8*i`.
  - `_PyShape_ResolveStore` gives:
    - REPLACE (offset), when the name is in a slot;
    - ADD, with the index `nslots`, the offset and `shape_to` if the
      edge exists;
    - REMOVE, with `shape_to` (the parent or the hole shape) if it
      exists;
    - MISSING;
    - DICT for a dictionary-mode shape or a non-exact name;
    - DATA_DESCR or NO_STORAGE, as now.
- **Loads.** SLOT reads the slot. On an inline-values type, DICT reads
  slot 0's dict (NULL means a miss). On other types it reads the dict
  word (section 5).
- **Stores** execute as prepare, then commit:
  - REPLACE writes the slot.
  - ADD prepares the edge. OK: write the value into slot `nslots`, then
    the header. LIMIT: TO_DICT. NOMEMORY: return -1. In every case the
    object is unchanged until the commit.
  - REMOVE prepares the delete edge. OK: clear the slot, then the
    header. LIMIT: TO_DICT, then delete from the dict. NOMEMORY: -1.
  - MISSING raises AttributeError, as now.
  - DICT stores into or deletes from slot 0's dict. A SET with no dict
    creates one.
- **TO_DICT** (conversion) builds a dict from the slots in slot order,
  skipping holes, so insertion order is kept. It applies the triggering
  store to that dict, then publishes: it clears slots 1 to nslots-1,
  writes slot 0, then writes the header. Publication is those stores
  only: no call, no allocation. A failure before publication drops the
  provisional dict and leaves the object unchanged.
- **Rooting during conversion** follows note 09 section 4.7. The
  receiver and the name are held by the caller. The incoming value and
  the provisional dict are each held in a `_PyValueRoot` from entry and
  from creation until publication; the values copied from the slots are
  held by the object, which the collector traverses until the header
  changes. A test hook (`_testinternalcapi.set_conversion_collect`)
  makes conversion run a full collection after the triggering store is
  applied to the provisional dict and before publication, so the tests
  collect at the one point where a missing root would matter.
- **NOMEMORY at entry points that do not raise.** The specialiser
  prepares an ADD edge speculatively: if preparation fails it clears the
  MemoryError and leaves the site unspecialised, so the generic store
  that follows prepares again and reports the failure itself.
  `_PyObject_GetDictPtr` keeps its contract: on a conversion failure it
  clears the error and returns NULL, with the object unchanged.
- **Entering dictionary mode.** This is conversion without a triggering
  store. It happens on:
  - `obj.__dict__` (`subtype_dict`, `PyObject_GenericGetDict`);
  - `vars(obj)`;
  - `_PyObject_GetDictPtr`, which keeps its writable contract by
    returning `&slot0`;
  - the default `__getstate__` when the object has fields;
  - `__class__` assignment;
  - a store with a non-exact name;
  - a limit.

  `obj.__dict__ = d` writes slot 0 and the dictionary-mode shape and,
  from shape mode, clears the rest without copying, since the old
  attributes are discarded. On an object already in dictionary mode it
  writes slot 0 only, since the other slots are NULL already and, after
  a `__class__` assignment, may not exist (next bullet). `del obj.__dict__` leaves slot 0 NULL in dictionary mode.
  A Py_STATS counter records each entry by reason.
- **`__class__` assignment** between inline-values types keeps today's
  `compatible_for_assignment` (equal `tp_basicsize`, equal flags). The
  object converts under its old type, and the dict stays in slot 0,
  which is at the same offset in both types. Then the header becomes the
  new type's dictionary-mode shape. The allocation keeps the old type's
  capacity, which may be smaller than the new type's, so from then on no
  path may touch a slot past 0: dictionary mode is terminal, every write
  in dictionary mode (a store, `__dict__ = d`, `del __dict__`, clearing
  by the collector or the C API) touches slot 0 only, and none returns
  the object to a shape. `__sizeof__` reports the current type's
  capacity, which after such an assignment can differ from the
  allocation by up to 24 slots; `sys.getsizeof` is an estimate already,
  and recording the allocated capacity would cost a field per object.
- **The import fast path** (`import.c`'s `spec._initializing` through
  `_PyObject_TryGetInstanceAttribute`) uses `_PyShape_SlotOf` on the
  object's shape.

### 5. Variable-size subclasses and other dict-bearing types

- **A Python subclass of a variable-size type** (`int`, `tuple`,
  `bytes`, a variable-size extension type) is given a dict word of its
  own and no longer gets `Py_TPFLAGS_MANAGED_DICT`. Where the word goes
  depends on where the items are:
  - when the items are at a fixed offset in the base's struct (int,
    tuple, bytes, and any base without `Py_TPFLAGS_ITEMS_AT_END`), the
    word follows the items, as before 3.12: `type_new_descriptors` adds a
    word to `tp_basicsize` and sets `tp_dictoffset = -sizeof(PyObject *)`;
  - when the base has `Py_TPFLAGS_ITEMS_AT_END` (`type` itself, and
    `_testcapi.HeapCCollection`), the items are at the final
    `tp_basicsize` (`PyObject_GetItemData`), so the word goes in the
    fixed part before them: `tp_dictoffset` is the base's
    `tp_basicsize`, and `tp_basicsize` grows by a word, which moves the
    items after it. (Subclasses of `type` inherit `type`'s own dict
    offset and are not affected.)
- **The dict word is found from the end of the object.**
  `_PyObject_ComputedDictPointer` keeps its formula
  (`_PyObject_VAR_SIZE(tp, n) + tp_dictoffset`), with n taken from one
  new function, `_PyObject_VarItemCount(obj)`. It returns `|Py_SIZE|`,
  except for `int` subclasses. Their `ob_size` is `lv_tag`, so n is the
  digit count, with a minimum of 1, which is what `long_subtype_new`
  allocates. int is the one variable-size builtin whose `ob_size` is not
  the item count, and the reason 3.12 moved these types to the
  pre-header.
- **A C type that asks for a managed dict and has `tp_itemsize != 0`**
  is converted the same way in `type_ready`, before any instance exists:
  the flag is cleared, a word is added and the offset is set, by the
  same two cases. Its `PyObject_VisitManagedDict` and
  `PyObject_ClearManagedDict` calls still work, through the next
  bullet.
- **The managed-dict C API** keeps its contract:
  - `PyObject_VisitManagedDict` visits the instance's attribute storage,
    which is the shape slots, slot 0 in dictionary mode, or the computed
    dict word.
  - `PyObject_ClearManagedDict` clears that storage. On an
    inline-values object in shape mode it clears the named slots and
    writes the root shape; in dictionary mode it clears slot 0 and stays
    in dictionary mode (section 4, `__class__` assignment).
- **`BaseException` and C types with their own dict field** are
  unchanged: their plans are DICT through `tp_dictoffset`.

### 6. The collector

- **`subtype_traverse`** visits, on an inline-values type, slots `0` to
  `nslots-1` in shape mode or slot 0 in dictionary mode, never both. On
  other types it visits the dict word, as now. Then it visits members
  and the type, as now. `gc.get_referents(obj)` follows from that.
- **`subtype_clear` and dealloc** go through `PyObject_ClearManagedDict`:
  from shape mode they clear the slots and write the root shape, and in
  dictionary mode they clear slot 0 and keep the dictionary-mode shape.
  Either way a resurrected or finalised object is a valid empty instance
  and no shape claims a slot that was cleared.
- **No new roots.** The slots are inside the object block, as the
  inline values are today, and are reached only through traverse.

### 7. Handlers and the specialiser

Guards. Every shape handler guards `_GUARD_SHAPE` (masked header ==
shape) and then `_GUARD_SHAPE_VERSION`, which compares the version read
from the live shape, `shape->type_version`, with the entry's. That is
note 09 section 2's guard: two dependent loads where
`_GUARD_TYPE_VERSION` has three. `_GUARD_TYPE_VERSION` stays for the
handlers that have no shape: class receivers, modules and the `NO_DICT`
forms.

| Plan | Handler | Change |
|---|---|---|
| SLOT, one shape at the site | `LOAD_ATTR_SHAPE` | new guards |
| SLOT or METHOD, two to four shapes | `LOAD_ATTR_SHAPE_POLY` | new |
| METHOD, call-pair, inline-values type | `LOAD_ATTR_METHOD_SHAPE` | new; replaces `METHOD_WITH_VALUES` and `METHOD_LAZY_DICT` |
| CLASS_VALUE, inline-values type | `LOAD_ATTR_NONDESCRIPTOR_SHAPE` | new (decision 2); replaces `NONDESCRIPTOR_WITH_VALUES` |
| METHOD, CLASS_VALUE, no dict storage | `LOAD_ATTR_METHOD_NO_DICT`, `LOAD_ATTR_NONDESCRIPTOR_NO_DICT` | unchanged |
| DICT | `LOAD_ATTR_WITH_HINT`, `STORE_ATTR_WITH_HINT` | the dict word from the entry's `dictoffset` (a new field, below): slot 0, a fixed `tp_dictoffset`, or computed; shape guard |
| MEMBER | `LOAD_ATTR_SLOT`, `STORE_ATTR_SLOT` | shape guard instead of the type version |
| REPLACE | `STORE_ATTR_SHAPE` | new guards |
| ADD, edge exists or prepared | `STORE_ATTR_SHAPE_TRANSITION` | renamed from `STORE_ATTR_SHAPE_ADD`; writes the slot, then `shape_to` into the header; no order bytes, no capacity check |
| PROPERTY, hooks, classes, modules | as in rung 2 | |

- **`LOAD_ATTR_SHAPE_POLY`** reads the site's poly table, which today
  records observations only. Its rows are matched by shape and version.
  A row's `handler` says what it is, as note 09 section 7 requires for
  polymorphic method calls: a `LOAD_ATTR_SHAPE` row reads the slot at
  its `index` (the offset); a `LOAD_ATTR_METHOD_SHAPE` row, at a
  call-pair site, pushes its `descr` and the receiver. A site becomes
  POLY when its second shape is recorded and every row's handler is one
  of those two. More than four shapes make it MEGA, and MEGA is the
  generic path through the plan cache. This is the one polymorphic
  handler of the ATTR kind (note 04 coordinator decision 1).
- **The ATTR entry's dict location.** `hint` and `dictoffset` share a
  union today, and `WITH_HINT` needs both. `dictoffset` moves to the
  entry's spare word (`_pad`), as an `int32_t`: positive is a fixed
  offset (slot 0 is `tp_basicsize`, or a C type's `tp_dictoffset`), and
  negative means computed from the end. The entry stays 40 bytes; the
  layout change is in the magic bump below.
- **Stores at polymorphic sites** take the generic path. There is no
  store poly handler (note 09 lists none), and today's
  `STORE_ATTR_INSTANCE_VALUE` goes. `attr_store_poly` records the cost.
- **Deleted handlers:**
  - `LOAD_ATTR_INSTANCE_VALUE`;
  - `LOAD_ATTR_METHOD_WITH_VALUES` and `LOAD_ATTR_METHOD_LAZY_DICT`;
  - `LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES`;
  - `STORE_ATTR_INSTANCE_VALUE`.

  Their uops go with them: `_CHECK_MANAGED_OBJECT_HAS_VALUES`,
  `_GUARD_DORV_NO_DICT`, `_CHECK_ATTR_METHOD_LAZY_DICT` and
  `_GUARD_DORV_VALUES_INST_ATTR_FROM_DICT`. So do the non-escaping entries
  for `_PyDictValues_AddToInsertionOrder`, `_PyObject_InlineValues`,
  `_PyObject_GetManagedDict` and `_PyObject_ShapeAfterAdd` in
  `Tools/cases_generator/analyzer.py`.
- **The specialiser** follows from the plan, with no storage logic of
  its own:
  - `specialize_dict_access_inline` and `instance_has_key` go;
  - `specialize_shape_access` takes the plan's offset;
  - ADD prepares the edge, so that a new transition can be specialised;
  - SLOT at a site with another shape of the type in its poly table
    gives `LOAD_ATTR_SHAPE_POLY`;
  - a store over a non-data class attribute still declines.
- **Magic.** 3723 in the commit that changes the handler set and the
  entry layout, and again in any later commit that changes either.

### 8. Deleted

Commit 1 deletes every instance use of `PyDictValues` below. The struct
itself (not `PyDictValues_Type`, the `dict.values()` view) stays until
commit 3, because the split-table code still dereferences `ma_values`;
commit 3 deletes both.

- The instance uses of `PyDictValues`:
  - `_PyObject_InlineValues`, `_PyInlineValuesSize` and
    `_PyObject_InitInlineValues`;
  - the order bytes and `_PyDictValues_AddToInsertionOrder`;
  - `shape_after_delete` and `shape_matches_order`;
  - `make_dict_from_instance_attributes`;
  - `_PyObject_MaterializeManagedDict[_LockHeld]`;
  - `detach_dict_from_object`, `set_dict_inline_values` and
    `clear_inline_values`;
  - `ensure_managed_dict`'s aliasing branch and
    `ensure_nonmanaged_dict`'s CACHED_KEYS branch;
  - `_PyObject_ManagedDictPointer`, `MANAGED_DICT_OFFSET` and
    `_PyObject_GetManagedDict`;
  - `_PyObject_InlineValuesConsistencyCheck`;
  - `store_instance_attr_lock_held` and `store_instance_attr_dict`;
  - the dead `_PyType_NewManagedObject`.
- Shared keys for instances (section 1).
- The pre-header (section 2).
- `_testinternalcapi.get_object_dict_values`, `has_inline_values` and
  `has_split_table`. Tests use `get_object_shape` and
  `_testinternalcapi.shape`.
- Tools/gdb/libpython.py's inline-values reader becomes a shape-slot
  reader. `pycore_debug_offsets.h` drops `ht_cached_keys`.
- The docs that describe the pre-header (Objects/object_layout.md,
  InternalDocs/values.md, and the `PyObject_VisitManagedDict` and
  `tp_dictoffset` notes in Doc/c-api) are updated.
- **Split tables** (decision 3).

### 9. Commits within the rung

1. **The storage.** Shapes keyed by name, with delete edges and
   preparation (section 3), and in the same commit:
   - the layout, allocation and pre-header deletion (sections 1, 2, 4,
     5 and 6);
   - the handler set (section 7);
   - the tests' new expectations.

   The pieces cannot be split. Today's slots are shared-key indices, so
   positional shapes need the new layout. The old handlers read the old
   layout, so they cannot outlive it. The commit is large because the
   old layout's readers are everywhere; gating it on both builds is the
   mitigation.
2. **`LOAD_ATTR_SHAPE_POLY`.**
3. **Split tables**, `ma_values` and the `PyDictValues` struct
   (decision 3).
4. **Measurements**, and this document's "As built".

Each commit is gated to level 1 on both builds and level 2 on release;
level 3 runs at the end.

## Invariants

- A shape-mode object's slot i is non-NULL exactly when the shape names
  slot i.
- A dictionary-mode object has NULL in every slot but 0.
- A shape's `nslots` is at most its type's capacity, so every slot that
  any shape names exists in every instance of the type.
- Prepare changes no object. Commit cannot fail. The header is written
  last.
- Insertion order is slot order without holes, in shape mode, and the
  dict's order in dictionary mode. Conversion preserves it.
- The executors give today's results, exceptions and side effects,
  except for the deliberate changes listed under Tests: dictionary mode
  is terminal, and `__dict__` does not alias.

## Decisions for the coordinator

1. **Variable-size subclasses drop `Py_TPFLAGS_MANAGED_DICT`.** They
   get a computed dict word instead (section 5), and so do C
   variable-size types that ask for a managed dict. The alternative,
   keeping the flag with a negative offset, gives the flag two layouts.
2. **`LOAD_ATTR_NONDESCRIPTOR_SHAPE` is added.** Note 09 section 9
   deletes `NONDESCRIPTOR_WITH_VALUES` without naming its replacement.
   Without one, a class value read through an instance (`o.k`, the
   `class_attr_load` micro benchmark, constants and defaults on classes)
   takes the generic path. The handler is `LOAD_ATTR_METHOD_SHAPE` that
   pushes only the value. If you would rather have one handler with a
   flag, say so.
3. **Split tables are deleted in this rung**, as commit 3. Once shared
   keys go, nothing creates a split dict, but dictobject.c keeps about
   150 references to them (`ma_values`, `DICT_KEYS_SPLIT`, the split
   insert, resize and copy). Deleting them removes `PyDictObject.ma_values`
   from a struct in Include/cpython, which ports must follow.
   Ruled: if the rung passes its deadline by more than half a day, the
   commit moves to 6c's last rung, not to 6d; 6c lands with no
   `ma_values` and no `PyDictValues`.
4. **Stores at polymorphic sites are unspecialised** (section 7). The
   alternative is a store poly handler, a second polymorphic handler for
   the ATTR kind.
5. **The managed-dict C API is reinterpreted, not deprecated**
   (section 5).

## Risks

- **Blast radius.** The pre-header and `PyDictValues` reach the
  allocator, the collector, tracemalloc, ctypes alignment, gdb and about
  60 tests. Commit 1 is gated with the full suite on both builds before
  it lands. ASAN is not available on this build.
- **C extension types with managed dicts** (asyncio's Future and Task,
  typing's TypeVar family) change layout. test_asyncio and test_typing
  are in the ladder.
- **Dictionary mode is terminal.** copy, pickle, `vars()` and
  `logging` put objects there for good. Rung 5 records
  `attr_load_after_dict`, `attr_store_after_dict`, deepcopy and logging,
  and the entries by reason.
- **int subclasses.** The computed offset depends on
  `long_subtype_new`'s digit rule. A test covers int subclasses of zero,
  one digit and many digits with attributes, collected and
  `getsizeof`-ed.
- **Capacity waste.** A class whose attributes are set from outside its
  methods gets six slots. Rung 5 records slots allocated against slots
  used.

## Hazards found

- **test_buffer, `test_memoryview_get_contiguous`** (gate 2 on release
  for 798977ee556). Line 4115, `self.assertEqual(nd[1], 1)`, raised
  `UnicodeDecodeError: 'utf-8' codec can't decode byte 0xa0 in position
  1: invalid start byte`. Afterwards, test_buffer alone passed 10 of 10
  times on release and once on debug, under `gate.sh 1`. The failure was
  real, not timing. `_testbuffer`'s ndarray had no `tp_traverse`, so the
  collector took it to refer to nothing. A view such as `nd[::-1]` holds
  its source array only in its base buffer's `obj`, so after
  `nd = nd[::-1]` a collection freed the source, and the view read a
  freed format string. With a `gc.collect()` between the slice and the
  read, the failure reproduced 200 times in 200. The ndarray now visits
  its exporter. `test_ndarray_view_keeps_exporter` covers the case. The
  ports README already lists the general rule: a C type that holds
  references must traverse them.

## Tests

- **test_shapes** gains:
  - layout: slot offsets are positions; `getsizeof` includes the
    capacity; capacity from `__static_attributes__` over the MRO (S = 0,
    S = 3, S > 28, inheritance);
  - transitions: delete edges and holes, insertion order after a delete
    and a re-add, and the 64-shape and capacity limits converting with
    order kept;
  - injected allocation failure (`_testcapi.set_nomemory`) during
    preparation of an add, a delete and a conversion: values, order and
    header unchanged, and MemoryError raised;
  - dictionary mode:
    - `__dict__` identity, mutation through the dict, `__dict__ = d`,
      `del __dict__`;
    - a dict retained after its owner dies;
    - `vars`, `__getstate__`, `__class__` assignment with different
      capacities;
    - `_PyObject_GetDictPtr` through `_testcapi`;
  - variable-size subclasses (`int` with 0, 1 and many digits, `tuple`,
    `bytes`) with attributes, collection and `getsizeof`;
  - C managed-dict types: `_testcapi.HeapCTypeWithManagedDict`, and the
    visit and clear C API (`clear_managed_dict`);
  - `gc.get_referents` in both modes;
  - collection during conversion, through the test hook of section 4:
    object-valued attributes that nothing else references, a freshly
    created incoming value, and inline ints, signed zero, infinities and
    a boxed NaN; after the store every value is intact and in order. Run
    under the default allocator and, in a subprocess, under
    `PYTHONMALLOC=mimalloc_debug`;
  - non-exact names: loads, stores and deletes through a `str` subclass
    with its own `__hash__` and `__eq__`, from Python and through
    `PyObject_GetAttr`/`SetAttr` in `_testcapi`, with a data descriptor
    of the same name on the class, and with allocation failure during
    the load's conversion;
  - `__class__` assignment from a 6-slot type to a 30-slot type and
    back, each followed by `_testcapi.clear_managed_dict`, stores,
    `__dict__ = d`, `gc.collect()` and `gc.get_referents`, repeated;
  - `_testcapi.HeapCCollection` subclasses with attributes and zero, one
    and several items: the items are intact after attribute stores,
    `clear`, collection and deallocation;
  - allocation failure in the specialiser's speculative preparation (a
    warmed store whose next shape has no edge yet): no exception leaks
    from specialisation and the generic store raises MemoryError; and
    `_PyObject_GetDictPtr` under failure returns NULL with no exception;
  - conservative roots and sweep under `mimalloc_debug`: test_gc and
    test_shapes in a subprocess with `PYTHONMALLOC=mimalloc_debug` on the
    release build;
  - transient types with many shapes, created and dropped.
- **Specialisation:**
  - `LOAD_ATTR_SHAPE_POLY` over 2, 4 and 5 shapes (MEGA), with SLOT rows,
    METHOD rows and a mix;
  - after warming each of POLY, METHOD_SHAPE, NONDESCRIPTOR_SHAPE,
    SLOT (member) and WITH_HINT: a class mutation, and version
    exhaustion; and specialised code kept alive while transient types
    die and their addresses are reused;
  - `LOAD_ATTR_METHOD_SHAPE` and `LOAD_ATTR_NONDESCRIPTOR_SHAPE`,
    including an instance attribute shadowing the method on a second
    shape;
  - `STORE_ATTR_SHAPE_TRANSITION` from a prepared edge;
  - `WITH_HINT` on dictionary-mode, int-subclass and `BaseException`
    subclass receivers;
  - a store over a non-data class attribute: the site stays
    `STORE_ATTR` and the generic path stores.
- **Rewritten expectations:**
  - test_class `TestInlineValues`;
  - test_opcache `TestInstanceDict` and the handler tests named in the
    survey;
  - test_sys (sizes);
  - test_capi `test_type` and `test_misc` (flags, the managed-dict
    layouts);
  - test_dict's split-table tests (they become ordinary-dict tests, or
    go with commit 3);
  - test_type_cache `test_one_name_several_shapes` (offsets now
    differ);
  - test_shapes' rung-1 and rung-2 aliasing tests.
- **Ladder.**
  - Gate 1 on both builds, with test_shapes, test_opcache, test_class,
    test_descr, test_dict, test_sys, test_gc, test_capi, test_copy,
    test_pickle, test_weakref, test_type_cache, test_asyncio,
    test_typing, test_ctypes, test_enum, test_dataclasses, test_inspect,
    test_functools, test_dis, test_import and test_tracemalloc.
  - Gate 2 per commit; gate 3 at the end.

## Measurements (recorded, not gated)

Before is rung 2 (80ba2edf94c), after is the rung's last commit.

- **Instructions per loop:** richards, go, raytrace, deltablue, chaos,
  nbody, json_loads, deepcopy and logging; the attribute micro benchmarks
  (including `attr_load_poly`, `attr_store_poly`,
  `attr_load_after_dict`, `attr_store_after_dict`), `new_instance` and
  `new_instance_empty`.
- **Memory:** RSS for one million four-attribute instances, and
  `sys.getsizeof` of a small instance, against both before and
  `results/step5c.json`.
- **Wall time** for the same kernels (gate 4), since instructions do not
  show the latency of the dependent loads.
- **Shape and plan-cache memory:** `_PyShape_TypeShapesSize` and the
  plan-cache bytes over the benchmark set, and for a loop that creates
  and drops 10,000 transient classes with instances.
- **Dictionary-mode entries by reason**, over the benchmark set and the
  test suite (Py_STATS).
- **Slots allocated against slots used**, over the benchmark set.

### Results

Measured on 2026-10-27 with gate 4 (`boil/bench/run.py --set all
--metric instr,rss`). Before is 80ba2edf94c (rung 2), built in a
temporary worktree. After is dba354f43ec, the last commit before the
storage-marking fixup. Both are default builds (`CC=gcc`). The files are
`results/6c-rung2-before.json` and `results/6c-rung3.json`. Wall time
was not measured (see "As built").

| benchmark | before | after | change |
|---|---:|---:|---:|
| richards | 515.55M | 355.50M | -31.05% |
| go | 1,086.84M | 1,349.97M | +24.21% |
| raytrace | 2,748.03M | 2,698.66M | -1.80% |
| deltablue | 41.80M | 42.33M | +1.27% |
| chaos | 570.23M | 575.55M | +0.93% |
| nbody | 687.72M | 723.87M | +5.26% |
| json_loads | 7.750M | 7.641M | -1.41% |
| deepcopy | 3.783M | 3.754M | -0.76% |
| logging_format | 683.4K | 756.5K | +10.70% |
| logging_simple | 617.8K | 706.7K | +14.40% |
| logging_silent | 16.59K | 16.64K | +0.28% |
| attr_load | 41.0 | 42.0 | +2.39% |
| attr_load_poly | 185.9 | 60.0 | -67.74% |
| attr_store | 44.0 | 44.1 | +0.17% |
| attr_store_poly | 280.9 | 263.1 | -6.36% |
| attr_load_after_dict | 44.0 | 78.1 | +77.60% |
| attr_store_after_dict | 88.0 | 92.0 | +4.61% |
| attr_load_dataclass | 41.0 | 42.0 | +2.53% |
| attr_store_dataclass | 496.1 | 522.1 | +5.25% |
| new_instance | 1,755.7 | 1,626.0 | -7.39% |
| new_instance_empty | 1,457.1 | 1,339.1 | -8.10% |
| empty_loop | 58.7 | 60.6 | +3.32% |

Kernels are instructions per loop, and micro benchmarks instructions per
iteration. Over the whole set the kernel geomean is 1.004 (29
kernels), the micro geomean 1.012 (45) and the memory-set instruction
geomean 0.954 (6).

What causes the changes:

- **richards, -31%, and attr_load_poly, -68%: `LOAD_ATTR_SHAPE_POLY`.**
  Before, 52 `LOAD_ATTR_SHAPE` sites missed on their second shape and
  took the generic path. After, 22 of them are POLY, and the generic
  stores fall from 18 to 10.
- **go, +24%: polymorphic stores are generic (decision 4).** Before,
  22 store sites were `STORE_ATTR_INSTANCE_VALUE`, which wrote through a
  shared-keys index and did not depend on the shape. Section 7 deletes
  that handler. After, those sites are plain `STORE_ATTR`:
  - `Square.move`: color, reference, ledges (five sites) and used;
  - `Square.remove`: removestamp, ledges and color;
  - `Square.find`: reference.

  Squares have several live shapes: `Board.reset` gives every square
  color and used, `move` adds reference and ledges on a square's first
  move, and `Board.useful` adds temp_ledges. So each of these sites sees
  more than one shape of `Square`. In the profile, the generic store
  path (`generic_setattr_value`, `PyObject_SetAttrValue`,
  `_PyShape_SlotOf`, `_PyShape_ResolveStore`,
  `_PyType_LookupValueAndVersion`) accounts for about 10% of go's
  instructions, against none before. Load sites are POLY (48) or SHAPE
  (92); the 35 generic loads are in cold code (`Board.check`, `reset`,
  `__repr__`). This is the cost the decision named, not a defect. A
  store poly handler is the hand-off item.
- **logging_simple, +14%, and logging_format, +11%: conversion to
  dictionary mode.** The formatter reads `record.__dict__`, which
  converts each new `LogRecord`. With call graphs, `convert_to_dict` is
  9.0% of logging_simple's instructions, about 207M. Before, rung 2
  materialised a split dict that shared the type's keys: 0.35%, about
  7M. That is about 200M of the 240M increase. The copy is note 09's
  design (`__dict__` does not alias, and dictionary mode is terminal).
  Within it, `dict_from_slots` starts from an empty dict and resizes as
  it inserts (`dictresize`, 3%); presizing is on the hand-off list.
- **attr_load_after_dict, +34 instructions per load:
  `LOAD_ATTR_WITH_HINT`.** Before, a read of `__dict__` left the values
  inline and aliased, so the site stayed `LOAD_ATTR_INSTANCE_VALUE`.
  After, the object is in dictionary mode, and the site is
  `LOAD_ATTR_WITH_HINT`, as note 09 section 6 prescribes. Its checks
  are all needed: slot 0 may be NULL or a dict subclass, and the hint
  needs the keys kind, the length and the key. attr_store_after_dict is
  +4 instructions, for the same reason.
- **About +2 instructions per iteration on every micro benchmark: a
  register spill in `_PyEval_EvalFrameDefault`.** empty_loop goes from
  62.4 to 64.4 instructions per iteration (`perf stat`, 20M iterations).
  The source of `FOR_ITER_RANGE`, `STORE_FAST` and `JUMP_BACKWARD` is
  the same in both builds. In the compiled `FOR_ITER_RANGE`, rung 2
  keeps the frame's register-file base in `%r13`. Rung 3 spills it to
  `-0x398(%rbp)` and reloads it twice in the handler: `mov
  -0x398(%rbp),%rcx` before the guard and again before the store of the
  result. Otherwise the two handlers are the same, instruction for
  instruction. The handler set changed (POLY added, the `_WITH_VALUES`
  and `LAZY_DICT` handlers removed), and gcc allocates registers over
  the whole function, so every handler that indexes the register file
  pays the reload. This is not attributable to any one handler, and no
  rung-3 source change can remove it without tuning the interpreter
  loop. It is on the hand-off list.
- **new_instance, -7%, and new_instance_empty, -8%:** there is no
  pre-header, `PyDictValues` header or order bytes to initialise.
- **nbody, +5% (36M instructions)** is not attributed. The register
  spill is a candidate, but I have not verified it. It is on the
  hand-off list.

Memory:

| measure | before | after | change |
|---|---:|---:|---:|
| instances_1m peak RSS | 127,536 KiB | 118,520 KiB | -7.07% |
| dicts_200k peak RSS | 66,036 KiB | 62,764 KiB | -4.95% |
| tuples_10m peak RSS | 859,420 KiB | 746,320 KiB | -13.16% |
| startup peak RSS | 13,872 KiB | 13,744 KiB | -0.92% |
| `sys.getsizeof` of a four-attribute instance | 24 | 56 | (see below) |
| richards' 14 classes: shape tables | 6,528 B | 6,224 B | -4.7% |
| richards' 14 classes: plan cache | 15,888 B | 15,888 B | 0 |
| 10,000 transient classes with instances, RSS growth | 31,644 KiB | 33,944 KiB | +7.3% |

The RSS geomean is 0.997 over the kernels and 0.968 over the memory set.
Three runs per build agree within 0.3%, except one collector_live512
run.

- **instances_1m, -7%:** there is no 16-byte pre-header and no
  `PyDictValues` header. The six fixed slots (capacity `S + 2`) take
  the place of rung 2's decaying allocation.
- **`sys.getsizeof`:** before, 24 did not count the inline values or
  the pre-header. After, 56 is the whole object: the header word and
  six slots.
- **dicts_200k, -5%:** `PyDictObject` is 8 bytes smaller without
  `ma_values`.
- **Transient classes, +7%:** each class's shape table holds the
  capacity and the shapes. This is not attributed further.
- **Not attributed:** tuples_10m, -13% (113 MB, stable over three runs;
  tuples are untouched by this rung, and the one allocator change is
  that instances lost their separate pre-header heap); coroutines RSS
  +9.4%; raytrace RSS +4.4%; collector_live512 +3.5%. These are on the
  hand-off list.
- **Dictionary-mode entries by reason and slots allocated against slots
  used** were not measured (see "As built").

## As built

Where the code differs from the sections above, and why. The rung is
eight commits:

- f9685f65468, the storage;
- bb5ff82e6e9, `LOAD_ATTR_SHAPE_POLY`;
- 798977ee556, the review fixes;
- e7adfa36b13, dataclass capacity;
- 49580514d90, the `_testbuffer` traverse (under "Hazards found");
- dba354f43ec, split tables;
- 26d218d54a9, the collector marks attribute storage;
- this one, the measurements and this section.

- **A declined site records its row.** When `set_shape_guard` declines
  because the site has seen another shape, it still records the new
  shape's row in the poly table (`fb_record`). Without that record, a
  site that alternates between two shapes lost the first one on every
  re-specialisation and oscillated between them without reaching POLY.
- **`LOAD_ATTR_SHAPE_POLY`** is `unused/1 + _GUARD_SHAPE_POLY + unused/8 +
  _LOAD_ATTR_SHAPE_POLY`. The guard is its own op so that the generator
  checks the owner in pointer space and does not box it.
  `_PyFb_ShapeRow` matches the live shape and its `type_version` against
  each row. A slot row reads `poly->index[row]`, and a method row pushes
  the row's descriptor and the owner. A site becomes POLY when every row
  is `LOAD_ATTR_SHAPE` or `LOAD_ATTR_METHOD_SHAPE`, and rows of
  different types may mix. The megamorphic test warms for eight
  cooldowns: the site reaches MEGA after about a hundred iterations.
- **Members guard on the version only.** `LOAD_ATTR_SLOT` and
  `STORE_ATTR_SLOT` use `_GUARD_SHAPE_VERSION` without `_GUARD_SHAPE`. A
  member's offset is fixed by the type, so every shape of the type is
  valid, and comparing the live shape's `type_version` checks the type
  in two loads.
- **The lazy-dict method path is lost.** With
  `LOAD_ATTR_METHOD_LAZY_DICT` gone, method calls on objects whose dict
  is reached through `tp_dictoffset` stay generic. That covers exception
  instances, functions, int and tuple subclasses, and C types with a
  computed dict word. It costs performance only. Note 09 has no handler
  for it.
- **Sibling layouts with a dict word.** Two variable-size subclasses
  whose only difference is the computed dict word have the same layout
  (`dict_word_added`, used in `shape_differs` and `same_slots_added`).
  `_PyObject_GC_NewVar` zeroes the dict word, and `_PyObject_GC_Resize`
  moves it with the items, for types without `MANAGED_DICT`.
- **The shape hook collects with `_PyGC_LiveCollectHere`**, the
  allocation-triggered collection that runs no Python code.
- **Dataclass capacity** (note 09's capacity paragraph, amended
  2026-10-27). A type that has never allocated an instance
  (`_PyTypeShapes.allocated`, set by `_PyType_AllocSlotsSize`) and has
  no subclasses recomputes its capacity when `__static_attributes__` is
  assigned. dataclasses assigns the field names there before the class
  can be instantiated, so an 8-field dataclass has 10 slots.
- **Split tables went in this rung** (decision 3) as dba354f43ec. A dict
  keeps its values in its keys object only.
- **gdb is not installed** on the build machine, so test_gdb skips. The
  printer changes (shape slots, dictionary mode, no `ma_values`) are
  checked by a parse only.
- **No wall-clock numbers.** Other agents ran gates on the machine during
  the measurements, so only instructions and RSS were recorded (gate 4,
  `--metric instr,rss`). Both are insensitive to load.
- **Dictionary-mode entries by reason and slots allocated against slots
  used** need counters that this rung does not add. Rung 5 records them,
  as the Risks section says.
- **Hazards.** test_buffer's failure in the fixup's gate 2 was a missing
  traverse in `_testbuffer`, not a flake (see "Hazards found"). The
  traverse audit found that `_testcapi.HeapCTypeWithDict` and its
  variants never visit their dict. The coordinator ruled that the
  collector marks an instance's attribute storage itself
  (26d218d54a9; note 09 section 6, amendment of 2026-10-27).

### Hand-off

Each item is a measured cost that this rung records and does not tune.

1. **A store poly handler.** go is +24% because the polymorphic store
   sites of `Square` are generic (decision 4); about 10% of its
   instructions are in the generic store path.
2. **Presize the dict in `dict_from_slots`.** `dictresize` is 3% of
   logging_simple, inside the conversion's 9%.
3. **The register spill in `_PyEval_EvalFrameDefault`.** It costs
   about +2 instructions per iteration on every micro benchmark: the
   register-file base moves from `%r13` to `-0x398(%rbp)`.
4. **Not attributed:**
   - nbody, +5% of instructions;
   - tuples_10m, -13% peak RSS;
   - coroutines, +9.4% peak RSS;
   - raytrace, +4.4% peak RSS;
   - collector_live512, +3.5% peak RSS;
   - 10,000 transient classes, +7.3% RSS growth.

## Review

Reviewed by `boil/tools/review.sh` (codex, xhigh) against note 09, note 04
and the rung 2 doc; the full text is reviews/6c-rung3-review.md. Verdict:
revise, with six blocking issues, all addressed above.

1. **Clearing after a cross-capacity `__class__` assignment** could reset
   an object to the new type's root shape and let later stores write past
   its allocation. Fixed in sections 4, 5 and 6: every write in
   dictionary mode touches slot 0 only, clearing keeps dictionary mode,
   and only a shape-mode object returns to its root. A test changes class
   from 6 to 30 slots and back, then clears, stores and collects.
2. **A load with a non-exact name** found slot 0 holding an attribute,
   not a dict. Fixed in section 3: the load converts before the dict
   lookup, after descriptor precedence, as CPython materialises today.
   Tests cover `__hash__`/`__eq__` overrides, the C API and allocation
   failure.
3. **The computed dict word overlapped `Py_TPFLAGS_ITEMS_AT_END`
   items.** Fixed in section 5: such types get a fixed dict offset
   before the items; negative offsets are kept for items at a fixed
   struct offset. Tested with `HeapCCollection` subclasses.
4. **The debug allocator's offset** was missing from the address map.
   Section 2 now states it: object = block + `debug_offset`, in the
   OBJECT and GC heaps only, and the allocator is given the user pointer.
   Tests run test_gc and test_shapes under `PYTHONMALLOC=mimalloc_debug`.
5. **NOMEMORY at non-raising entry points.** Section 4: the specialiser
   clears its own MemoryError and leaves the site unspecialised, and
   `_PyObject_GetDictPtr` keeps NULL-without-exception. Both are tested.
6. **The collection test did not collect during conversion.** Section 4
   adds a test hook that collects between the provisional dict and
   publication, and roots the incoming value and the dict separately.
   Publication makes no call and no allocation.

Non-blocking items taken: native `__sizeof__` overrides share one size
helper; the deletion schedule is one sequence (commit 3 removes split
tables, `ma_values` and the `PyDictValues` struct; commit 1 removes the
instance uses); wall time, shape memory, plan-cache memory and transient
types are measured; the handler tests cover class mutation, version
exhaustion and address reuse; conversion tests use inline ints, signed
zero, infinities and a boxed NaN. The gdb reader's int digit-count rule
and a test_gdb run go with section 8's reader change.

Questions answered:

- `__sizeof__` after a cross-capacity `__class__` assignment reports the
  current type's capacity (section 4).
- `LOAD_ATTR_SHAPE_POLY` serves SLOT and METHOD rows (section 7), as
  note 09 section 7 asks.
- `dictoffset` moves out of the `hint` union into the entry's spare
  word, so the entry stays 40 bytes (section 7).
