# 6c rung 2: the lookup plan

Status: implemented, 2026-10-27; "As built" records where the code
differs from the design. Revised after its review
(work/reviews/6c-rung2-review.md) and against note 09 as revised twice
(70ca9ece1a5). Specification: note 09 section 4 and section 9 item 2.

Rung 1 put the shape in the header. The instance layout is unchanged
until rung 3: inline values indexed by the shared keys, holes allowed,
an aliasing split dict possible. So this rung's plans describe today's
layout through a storage adapter (below), and rung 3 changes only the
adapter, the offsets and the handlers.

## What changes

### 1. Dispatch and the generic protocol (note 09 section 4.1)

`getattr_object_value`, `PyObject_GetAttr` and `PyObject_SetAttr`
dispatch on `tp_getattro` and `tp_setattro`, as now. The generic protocol
is three functions:

- `generic_getattr_value` (behind `PyObject_GenericGetAttr` and
  `_PyObject_GenericGetAttrWithDict`);
- `_PyObject_GetMethodValue` and `_PyObject_GetMethod`, the call-pair
  form;
- `generic_setattr_value`.

Each becomes "plan, then execute", and none of them calls a type slot
for the attribute. A hook (`_Py_slot_tp_getattr_hook`, `module_getattro`,
`object.__getattribute__` called explicitly) that runs the generic
protocol executes the same plan. The recursion the review found cannot
occur: no plan kind dispatches.

### 2. Plans (new `Include/internal/pycore_lookup.h`, `Objects/lookupplan.c`)

- **`_PyLookupPlan`** as in note 09 section 4.3: kind, fallback,
  slot_kind, flags (CACHEABLE, MEMBER), type_version, offset, value; 24
  bytes. `value` is always the MRO result.
- **`_PyLookupFacts`**, the output of phase 1:
  - `descr`, the MRO result;
  - `cacheable`;
  - the type's version;
  - for the specialiser only, the hook facts: the Python
    `__getattribute__` function or NULL, `has_getattr`, and whether
    `tp_getattro` and `tp_setattro` are generic.
- **`_PyLookup_ClassifyDescr(descr, op)`** classifies `descr` by its
  type's current slots. It returns DATA_DESCR (for a load, get and set;
  for a store, set), PROPERTY, METHOD, DESCR, CLASS_VALUE or none. It is
  the one descriptor classifier, used by instance plans and by the
  specialiser's class-receiver path, and it replaces
  `classify_descriptor`.
- **`_PyShape_ResolveLoad(shape, name, &facts)`** and
  **`_PyShape_ResolveStore(shape, name, &facts, op, value_kind)`**,
  pure.
  - They read the shape, `shape->type` and `descr`'s type.
  - In this rung, the index of a name comes from `ht_cached_keys` with
    `_PyDictKeys_StringLookupSplit`, which runs no code on an
    exact-`str` name.
  - `value_kind` is BOXED throughout this rung (kinds come in rung 4).

### 3. Phase 1 and the cache: `_PyObject_LookupPlan(obj, name, &plan, &facts)`

- **The type is ready**: it is readied first, as the generic path does.
- **A cache hit** on (shape, name) gives the plan and facts.
- **On a miss**, the root entry, or `find_name_in_mro`, gives `descr`.
  Then classify, and insert if the result may be cached.
- **The result may be cached** only if all of these hold:
  - the name is an interned exact `str`;
  - the type's version is not zero (it is assigned if possible, as now;
    exhaustion leaves it zero);
  - every class dict on the MRO has a `DK_IS_UNICODE` keys table, so the
    lookup ran no user code;
  - `descr`'s type is immutable.
- **Otherwise** the plan is used once and not cached. On that path,
  phase 1's intermediate results are rooted in `_PyValueRoot`s, as
  `generic_getattr_value` roots `descr` now. On the cacheable path phase
  1 allocates no object and runs no code, so its facts cannot change
  under it.
- **A name that is not an exact `str`** is classified the same way and
  not cached. Data descriptors keep their precedence, and the storage
  adapter materialises the dict only when execution reaches instance
  storage, as now.

### 4. The plan cache (`Python/typecache.c`)

- **Entries** are {shape, name, plan}: 40 bytes, against {name, value}
  at 16. The hash is the name's hash xor `(uintptr_t)shape >> 4`.
- **The root entry** holds `plan.value`, the MRO result.
  `_PyType_LookupValueAndVersion` and `_PyType_LookupRefAndVersion`
  probe (root shape, name) and return it, so their callers and contract
  are unchanged.
- **Root entries are filled only by `find_name_in_mro`.** Hook facts are
  root entries of `__getattribute__` and `__getattr__`. `plan.value` is
  never the hook.
- **Validity**: an entry is used only if its version is nonzero and
  equals the shape's version. The whole table is cleared by
  `type_modified`, as now. A stale entry is replaced in place, which is
  safe because the free-threaded build no longer exists (note 06 section
  3.4).

### 5. Execution and the storage adapter of this rung

**Loads.**

- SLOT reads the inline value at `offset`. The shape guarantees that
  the values are valid and the slot is filled.
- DICT executes today's instance storage access: the inline values
  through `_PyObject_TryGetInstanceAttributeValue` if they are valid,
  otherwise the managed dict, otherwise materialisation for a
  non-exact-`str` name. Then the fallback.
- In this rung a dictionary-mode shape does not imply that a dict
  exists: after the shape limit the inline values stay valid without a
  dict, and a materialised dict may alias them through a split table.
  This adapter is rung 2's only knowledge of that. Rung 3 deletes it,
  when dictionary mode means "slot 0 holds the dict".

**Stores.** A new entry point, `_PyObject_LookupStorePlan(obj, name, op,
value, &splan)`, takes SET or DELETE. Its dispatch on `tp_setattro` is
separate from the load dispatch, so a class with independent read and
write overrides is handled. Execution has two parts.

Prepare. Nothing about the object changes here:

1. Find the name's shared-keys index, inserting the name into the
   type's shared keys if it is new. That is type state, which today's
   `insert_split_key` also changes; it may stay if a later step fails.
2. Check the index against the values' capacity. If it does not fit,
   the outcome is LIMIT, and the store becomes TO_DICT.
3. `_PyShape_Prepare` creates or finds the transition. `_PyShape_Add`
   is changed to report NOMEMORY (with MemoryError set) and LIMIT (64
   shapes) as distinct results, where it returns NULL for both today.

Commit:

1. Write the value.
2. Update the insertion-order bytes and size.
3. Write the header.

TO_DICT builds the dict, stores the value in it, then publishes it, as
today's overflow path does. REMOVE uses rung 1's `shape_after_delete`
as its preparation; MISSING raises; NO_STORAGE raises AttributeError, as
now. This is today's `store_instance_attr_lock_held`, reordered into
prepare and commit.

**Members** (note 09 section 4.3). A direct load or store needs all of
these:

- `_Py_T_VALUE_EX`;
- `PyType_IsSubtype(shape->type, d_type)`;
- no `_Py_AFTER_ITEMS`;
- an offset inside `tp_basicsize`;
- for a load, no `Py_AUDIT_READ`;
- for a store, no `Py_READONLY`.

Any other member runs through its descriptor. Deleting an unbound member
keeps `structmember.c`'s AttributeError.

### 6. The specialiser over plans

`_Py_Specialize_LoadAttr` takes the load plan and the hook facts;
`_Py_Specialize_StoreAttr` takes the store plan. Only cacheable plans
specialise. Today's handlers are the targets:

| Plan or outcome | Handler | Condition |
|---|---|---|
| SLOT | `LOAD_ATTR_SHAPE` | one shape of the type seen at the site |
| SLOT | `LOAD_ATTR_INSTANCE_VALUE` | another shape of the type seen (presence-checking) |
| DICT, values valid | `LOAD_ATTR_INSTANCE_VALUE` | the name is in the shared keys |
| DICT, combined dict | `LOAD_ATTR_WITH_HINT` | the name is in the dict |
| DICT, split dict | decline | |
| MEMBER | `LOAD_ATTR_SLOT` / `STORE_ATTR_SLOT` | |
| PROPERTY | `LOAD_ATTR_PROPERTY` | not with `__getattr__` |
| METHOD, call-pair | `LOAD_ATTR_METHOD_WITH_VALUES`, `_NO_DICT`, `_LAZY_DICT` | `_WITH_VALUES` keeps today's precondition that the name is absent from the shared keys, guarded by the keys version |
| CLASS_VALUE | `LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES`, `_NO_DICT` | the same precondition |
| ABSENT | cooldown, as now | |
| hook: Python `__getattribute__`, no `__getattr__` | `LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN` | the function from the hook facts |
| REPLACE | `STORE_ATTR_SHAPE`, or `STORE_ATTR_INSTANCE_VALUE` when polymorphic | |
| ADD | `STORE_ATTR_SHAPE_ADD`, or `STORE_ATTR_INSTANCE_VALUE` when polymorphic | the specialiser prepares a new transition |
| DICT store | `STORE_ATTR_WITH_HINT` | as now |
| anything else | decline | |

A type whose hook is `__getattr__` with the default `__getattribute__`
specialises only SLOT, METHOD, CLASS_VALUE and DICT whose fallback is
one of those (note 09 section 4.2). Modules keep
`specialize_module_load_attr`, and classes keep
`specialize_class_load_attr`, which classifies through
`_PyLookup_ClassifyDescr`. `_testinternalcapi.lookup_plan(module, name)`
returns the module's DICT plan.

### 7. `LOAD_ATTR_PROPERTY` reads the getter at execution

The entry holds the property object (`descr`) instead of a borrowed
`fget`. The handler loads `prop_get` and checks that it is a function
with the recorded version; NULL or a non-function fails the guard.
The layout's meaning and the handler change, so the magic goes to 3722.

### 8. Inspection

- `_testinternalcapi.lookup_plan(obj, name)` returns `(kind, fallback,
  slot_kind, cacheable, member, type_version, offset, value)`, with the
  kinds as strings.
- `_testinternalcapi.lookup_store_plan(obj, name, value_or_NULL)`
  returns `(outcome, offset, has_shape_to)`.
- `_testinternalcapi.shape(obj)` returns `(type, index, slots)`: `index`
  is the shape's index in its type's table, or None in dictionary mode,
  and `slots` is a tuple of `(name, kind)`.
- `dis` with `show_caches` prints an ATTR entry's `type_version` and
  `shape`, the latter as a hex address that is never followed.

## Invariants

- Classification allocates nothing and runs no user code: it reads only
  the shape, its type, the type's shared keys and `descr`'s type. Code
  review checks this, since the functions call nothing that allocates.
- The executors give the results, exceptions and side effects of today's
  generic paths. Plans add no semantics.
- A cached plan is used only while its nonzero version equals its
  shape's.
- Executors root the receiver, the descriptor, the name, the incoming
  value and any provisional dict in `_PyValueRoot`s across descriptor
  calls and allocation.
- Prepare changes no object state; commit cannot fail.

## Deletions

- In Python/specialize.c: `analyze_descriptor_load`,
  `analyze_descriptor_store`, `classify_descriptor`, `instance_has_key`
  and the `DescriptorClassification` enum (stats use plan kinds).
- The resolution logic in `generic_getattr_value`,
  `_PyObject_GetMethod`, `_PyObject_GetMethodValue` and
  `generic_setattr_value`.
- The {name, value} cache entry.
- The borrowed `fget` in the ATTR entry.

## Decisions note 09 left open

1. **Where the code lives.** Plans and executors in
   `Objects/lookupplan.c`; shapes in `Objects/shapeobject.c`; the cache in
   `Python/typecache.c`.
2. **A fallback on every plan** except data descriptors and properties,
   so that a caller's explicit dict needs no second resolver.
3. **Cache key and contents.** The key is (shape, name), per type, with
   the hash above. The cache holds load plans only; ABSENT is cached, so
   a `hasattr` miss is one probe. Store plans are computed from the root
   entry and the shape tree.
4. **Polymorphic shapes** keep today's rule in this rung: a site that has
   seen another shape of the type uses the presence-checking handlers.
   Rung 3 replaces this with `LOAD_ATTR_SHAPE_POLY`.
5. **No plan for `__class__`.** It is a getset data descriptor on
   `object`, so its plan is DATA_DESCR, which is not specialised, as now.
6. **No free-threaded gate.** That build was removed (note 06 section
   3.4).

## Risks

- **Semantic drift in the generic paths**: audit-read members, set-only
  descriptors, `suppress`, the AttributeError suggestion context,
  `str`-subclass names, lazily readied types. Mitigation: the executors
  transcribe today's paths in plan order, and the full suite runs on both
  builds.
- **Cache size**: entries grow from 16 to 40 bytes, and there is an entry
  per non-root shape and name. Recorded below.
- **Generic-path cost**: a cache probe where today it is `_PyType_Lookup`
  plus `_PyObject_TryGetInstanceAttributeValue`. Recorded below.

## Tests

New `LookupPlanTests` in test_shapes. Each plan kind is asserted through
`lookup_plan`, with the behaviour asserted beside it through `getattr`
and through `_testcapi`'s value API.

- **Kinds.**
  - Slot and dictionary mode, including shape exhaustion without
    materialisation, and split-dict aliasing.
  - Members: set, unbound, deleted twice, transplanted (`B.x = A.x`
    raises TypeError), on a variable-size subclass, and native members.
  - Property, with its getter replaced by a function, by None and by a
    non-function.
  - A set-only C descriptor (`_testcapi`), read and written.
  - Methods: extracted (`f = o.m` is bound), shadowed on one instance
    and not on another, `staticmethod`, `classmethod`, a custom
    `__get__`.
  - Class values, shadowed and not.
  - Missing attributes, with and without `__getattr__`.
  - A Python `__getattribute__`, a delegating one
    (`object.__getattribute__(self, n)`), and explicit
    `object.__setattr__`.
  - A class with independent read and write overrides.
  - A `str` subclass name over a property.
- **Raw lookup.** `_PyType_Lookup` beside ordinary lookup on a class with
  `x = 42` and a Python `__getattribute__`, populated in both orders.
- **Modules.** `lookup_plan(module, name)` is DICT. One site alternates
  two modules with different insertion orders, then deletion and
  reinsertion; a module `__getattr__`; a dict entry named `__class__`.
- **Mutation.**
  - A descriptor class gaining `__set__` after warm-up and inside a hot
    loop: the plan is uncacheable from the start, and the behaviour
    follows the change.
  - `p.__init__(new_getter)` after warm-up, with `gc.collect()` before
    the next load.
  - A getter that calls `gc.collect()`.
- **Lifetime.** Transient types created, specialised and dropped. The
  specialised code is kept alive, a weak reference checks that each type
  is reclaimed, and the old site is then run with a new type.
- **Cache.** In test_type_cache: collisions, resize, negative (ABSENT)
  entries, one name under several shapes, version exhaustion
  (uncacheable, still correct), non-interned names. In test_descr:
  `test_type_lookup_mro_reference` extended with a reentrant `__eq__` on
  a class-dict key.
- **Stores.** Replace, add, remove, missing, member, dictionary mode,
  data descriptor. `_testcapi.set_nomemory` during preparation leaves
  values, order and header unchanged.
- **dis.** test_dis shows `type_version` and `shape` with `show_caches`.

Ladder: gate 0 after every build. Gate 1 before each commit, with
test_shapes, test_opcache, test_descr, test_class, test_dict,
test_property, test_type_cache, test_capi, test_dis, test_sys, test_gc,
test_weakref, test_super, test_module, test_inspect and test_copy. Gate 2
at the specialiser milestone; gate 3 at the end.

## Measurements (recorded, not gated)

Instructions per loop, before (rung 1) and after: richards, go, raytrace,
deltablue, chaos and json_loads; the micro benchmarks attr_load,
attr_load_poly, attr_store, method_call, property_load, class_attr_load,
slots_load, new_instance, attr_load_after_dict and attr_store_after_dict.
Boxes per loop for class_attr_load and property_load. Plan-cache bytes
per type for the richards classes. A cold measure (first call on a new
class) beside the warm loop.

### Results

Measured on 2026-10-27 with `boil/bench/run.py --metric instr`. Before
is b65f081239d (rung 1 and the gate fixes), built in a temporary
worktree; after is this commit. Both are default builds (`CC=gcc`).

| benchmark | before | after | change |
|---|---:|---:|---:|
| richards | 530.62M | 514.04M | -3.13% |
| go | 1,086.92M | 1,086.90M | -0.00% |
| raytrace | 2,740.55M | 2,748.80M | +0.30% |
| deltablue | 41.57M | 41.80M | +0.55% |
| chaos | 568.85M | 570.14M | +0.23% |
| json_loads | 7.749M | 7.750M | +0.01% |
| attr_load | 41.0 | 41.0 | -0.03% |
| attr_load_poly | 206.6 | 186.0 | -10.00% |
| attr_store | 44.0 | 44.0 | -0.00% |
| method_call | 302.0 | 302.0 | -0.01% |
| property_load | 276.0 | 281.0 | +1.81% |
| class_attr_load | 35.0 | 35.0 | +0.00% |
| slots_load | 41.0 | 41.0 | -0.09% |
| new_instance | 1,755.7 | 1,755.6 | -0.00% |
| attr_load_after_dict | 44.0 | 44.0 | -0.01% |
| attr_store_after_dict | 88.0 | 88.0 | -0.03% |

Kernels are instructions per loop and micro benchmarks instructions per
iteration. The kernel geomean is 0.997 and the micro geomean 0.993.

What causes the changes:

- **attr_load_poly, -10%: the generic path.** The site's
  `LOAD_ATTR_SHAPE` misses on the second shape and falls back to
  `LOAD_ATTR`. `perf record -e instructions:u` shows how that load runs:
  - before, a type lookup, a shared-keys probe
    (`unicodekeys_lookup_unicode`, 7%) and
    `_PyObject_TryGetInstanceAttributeValue` (6%);
  - after, one plan-cache probe (`_PyShape_LookupPlan` with
    `_PyTypeCache_Lookup`), then a direct read at the SLOT offset.

  richards' -3.1% likely has the same cause, at its polymorphic sites.
- **property_load, +5 instructions per load: the handler.** It now reads
  `prop_get` from the property and checks that it is a function, which
  is section 7's fix for the borrowed getter.
- **The others** are within 0.6%.

Other measurements:

- **Boxes per iteration** (`--metric boxes`, pystats builds of both) are
  unchanged, effectively zero (0.001), for class_attr_load,
  property_load, attr_load and method_call.
- **Plan-cache bytes after richards has run** total 15,888 over its 14
  classes. The four Task subclasses have 2,576 bytes each, Packet and
  TaskState 1,296, and the rest 656 or less. The before build has no
  size helper; its entries were 16 bytes against 40 now.
- **Cold beside warm** (/tmp script, median of 5, wall time, noisy).
  - Cold, a new class with its first loads of an attribute, a class
    value, a method and a miss: 7.8 to 8.1 us after, against 7.6 us
    before, about 5% slower. Class creation dominates; the extra cost is
    filling per-shape plans.
  - Warm, the same four loads on a fresh instance: 174 to 218 ns after,
    against 184 to 186 ns before, which is inconclusive.

## As built

Where the code differs from the sections above, and why.

- **Names and signatures.**
  - Phase 1 is `_PyShape_LookupPlan(shape, name, &plan)` and
    `_PyShape_LookupStorePlan(shape, name, op, &splan)`, in
    Objects/typeobject.c beside the MRO lookup. There is no
    `_PyLookupFacts` struct. The MRO result travels in `plan.value`, and
    the hook facts are computed by the specialiser alone
    (`lookup_hook_facts` in Python/specialize.c), from the root entries of
    `__getattribute__` and `__getattr__`.
  - The classifier is `_PyLookup_ClassifyLoad(descr, &mutable)`. Store
    classification needs one test (`tp_descr_set`), so it is inline in
    `_PyShape_ResolveStore`.
  - The executors stay in Objects/object.c, next to the generic paths
    they replace; Objects/lookupplan.c holds phase 2 only.
  - `lookup_store_plan(obj, name, delete=False)` returns `(outcome,
    cacheable, member, offset, has_shape_to)`.
- **Store preparation is not reordered in this rung.** `_PyShape_Add`
  still returns NULL for both the limit and an allocation failure, and
  either way the object goes to dictionary mode with its inline values
  still valid, which is a correct state and raises nothing. The one
  allocation that can raise is materialising the dict when the shared
  keys are full, and today's overflow path stages that before
  publishing. `test_store_out_of_memory` checks that the values, their
  order and the header are unchanged on that path. The prepare and
  commit split, with NOMEMORY and LIMIT as distinct results, comes in
  rung 3, where a store can need a shape that no fallback can stand in
  for.
- **`instance_has_key` stays** as the storage adapter's precondition for
  the `_WITH_VALUES` handlers (review issue 5). Rung 3 deletes it with
  the adapter.
- **Stores keep today's refusal** to specialise a name that has a
  non-data class attribute.
- **The set-only descriptor test uses a Python class.** Classification
  reads only `tp_descr_get` and `tp_descr_set`, which a Python class
  fills the same way as a C type. The Python class is uncacheable, so the
  test covers precedence but not a specialised site.
- **The specialiser still makes the transition** for
  `STORE_ATTR_SHAPE_ADD` with `_PyShape_Add`, as in rung 1. This changes
  type state (the shape tree), never the object, and if it fails the
  site uses `STORE_ATTR_INSTANCE_VALUE`.

## Review

The review (work/reviews/6c-rung2-review.md, verdict revise) raised seven
blocking issues; each is addressed above.

| Issue | Change |
|---|---|
| 1. GENERIC recursion | dispatch and the generic protocol are separate; no plan kind dispatches (1; note 09 section 4.1) |
| 2. `plan.value` | always the MRO result; hook facts come from phase 1 (2, 4) |
| 3. Cache eligibility and phase-1 consistency | interned names, a nonzero version, Unicode-keyed MRO dicts, immutable descriptor types; uncacheable results rooted; tests in test_type_cache and test_descr (3) |
| 4. Store path | a separate store entry point; prepare and commit, with NOMEMORY and LIMIT distinct; the specialiser prepares new transitions (5, 6) |
| 5. Storage guarantees | the rung-2 storage adapter, the handler conditions, and the shared-keys precondition kept for the method and class-value handlers (5, 6) |
| 6. Members | the six conditions; other members through their descriptor (5) |
| 7. Non-exact names | descriptor precedence kept; materialised only at storage; set-only descriptors (3; note 09 section 4.2) |

Non-blocking points taken: the free-threaded gate is gone; "classify
twice" is replaced by the construction rule; test_type_cache is on the
ladder; the lifetime tests keep the code alive and use weak references;
boxes and cold costs are measured. Answers to its questions:
the specialiser prepares a missing transition; `lookup_plan(module,
name)` is DICT, executed under `module_getattro`; a stale entry is
replaced in place (4).
