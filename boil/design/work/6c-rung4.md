# 6c rung 4: slot kinds as promises, and the `str +=` decision

Status: design, revised after review, 2026-10-27. Specification: note 09
section 5, the GENERALISE row of section 4.5, and section 9 item 4.
Before: rung 3 (a9e02dccada).

Coordinator rulings (2026-10-27):

1. **Versions.** The shape version and `tp_version_tag` are different
   things: one covers a shape's plans, the other the class's attributes.
   All three consumers in A4 are fixed. Type identity comes from the
   shape, not the version. The whole type is never re-versioned.
2. **Kinds.** Kinds live in the feedback entry, with one guard uop and a
   range compare.
3. **`str +=`.** Not in 6c (section B).
4. **Type identity in poly rows.** The live type's table is checked, not
   an 8-byte type pointer in every row. The invariant is stated at
   `_PyFb_ShapeRow` and here (A4.3), and an address-reuse test covers it.

Today every shape slot has kind BOXED (`_PySlotKind` has one value). This
rung adds I32 and F64. The store paths keep the promises, and a store
that breaks one generalises the field in place.

## A. Slot kinds

### A1. Kinds and the value test

- `_PySlotKind` becomes `BOXED = 0, I32 = 1, F64 = 2`.
- `_PyValue_SlotKind(v)` gives I32 for an inline int, F64 for an inline
  float, and BOXED otherwise. BOXED covers a bool, an int subclass, an
  int outside int32, a NaN (always boxed) and every other object.
- `_PyValue_FitsKind(v, k)` is one unsigned range compare,
  `v.bits - lo[k] <= span[k]`, over a static table:
  - BOXED: lo 0, span `UINT64_MAX`;
  - I32: lo `_PyValue_INT_BASE`, span `2^32 - 1`;
  - F64: lo `_PyValue_DOUBLE_OFFSET`, span
    `_PyValue_INT_BASE - _PyValue_DOUBLE_OFFSET - 1`.

  Infinities and signed zeros are inline, so they fit F64. Both
  functions go in `pycore_shape.h`.

### A2. The first store picks the kind

- `_PyShape_PrepareAdd(from, name, kind, &to)` takes the kind. A new
  edge gives its field that kind in the owner shape. `new_shape()`
  copies the kinds from the parent, so every shape below the owner
  inherits the field's kind.
- ADD edges stay keyed by name alone, so an existing edge is returned
  whatever its kind.
- Hole slots and DELETE edges are BOXED, as now.

### A3. Generalisation in place

`_PyShape_Generalise(shape, ix)`:

1. Walk up from `shape` to the field's owner, the ancestor whose kind
   is ADD and whose `edge_slot == ix`. A DELETE edge also has
   `edge_slot`, so the kind test is required.
2. Walk the owner's subtree (`children`/`sibling`, iteratively) and set
   `slots[ix].kind = BOXED` in every shape.
3. Give every shape in the subtree one fresh version, taken from the
   heap-type counter (`NEXT_VERSION_TAG(interp)`), or 0 if the counter
   is exhausted or the type has no version. The helper
   `_PyType_NewShapeVersion(type)` lives in typeobject.c and runs under
   the type lock.

Properties:

- **No allocation, so no failure.** Generalisation is a preparation
  step that cannot fail, and it never moves a value: every slot already
  holds a PyValue.
- **Shared tags are safe.** Every guard and poly row pairs a version
  with a shape pointer or with the type. A tag is unique to one
  subtree of one type, so tags are never reused.
- **Bounded fan-out.** A field generalises at most once, and kinds
  never return, so a type takes at most 63 such tags (one per ADD
  edge).
- **Root and dictionary-mode shapes are never re-versioned**, so they
  always carry `tp_version_tag`.
- **Type changes still win.** `_PyShape_SetTypeVersion()` writes the
  type's new version, or 0, over every shape, so a type change
  overrides a shape's fresh tag as before.

After this, `shape->type_version` is either 0, the type's tag, or a tag
newer than the type's.

Counters: generalisations, and shapes re-versioned, readable through
`_testinternalcapi`. They give rung 5 its fan-out number.

### A4. Consumers that assumed shape version = type version

There are three, found by auditing every macro that guards a version.

1. **The plan cache** (`lookup_plan_facts`, `_PyShape_LookupPlan`).
   - Problem: `cacheable` requires `version == shape->type_version`,
     and insertion requires `tp_version_tag == plan->type_version`. A
     generalised shape would never cache or specialise again, which is
     a defect.
   - Fix: phase 1's returned version must still equal the live
     `tp_version_tag`. This keeps the guard against a lookup that ran
     user code and changed the type. Separately, the shape's version
     must be non-zero.
   - Insertion: under the type lock, insertion checks both
     `tp_version_tag == ` that phase-1 version and
     `shape->type_version == plan->type_version`.
   - Lookup already compares the entry against the shape's live
     version.
2. **`LOAD_ATTR_PROPERTY`.**
   - Problem: it guards `_GUARD_TYPE_VERSION` (`tp_version_tag`) with
     the plan's version, which is the shape's. On a generalised shape
     the guard would fail on every execution.
   - Fix: the specialiser records `tp_version_tag` for the
     type-guarded handlers. The others guard root shapes, which never
     generalise: `*_NO_DICT`, class loads, and
     `GETATTRIBUTE_OVERRIDDEN`, which already uses `ga_version`.
3. **Type identity by version.**
   - Problem: `fb_saw_other_shape()` uses the version to identify the
     type (decision 4: a store site that saw another shape of the type
     stays generic). Across a generalisation it would miss shapes of
     the same type.
   - Fix: a row counts as "another shape of the type" when its shape
     pointer is the address of one of the live owner type's shapes:
     the root, a table entry or the dictionary-mode shape. The
     specialiser compares at most 66 addresses.
   - **Invariant** (stated at `_PyFb_ShapeRow`): a row's shape pointer
     is never dereferenced. It is only compared, in one of two ways:
     - in a handler, with a live object's header word, where the row's
       stored version against the live shape's version makes a reused
       address safe;
     - in the specialiser, with the live type's table, where membership
       proves the shape is that type's.

     Address reuse can only cost a specialisation (a site stays
     generic), never cause a wrong load.
   - Versions are never reused within an interpreter, which is the scope
     of every feedback table. Every non-zero shape version is a tag that
     the per-interpreter heap-type counter issued to that shape's type
     alone, and rows never hold 0.
   - Test: free a type whose shape sat in a poly row, then allocate new
     types until a shape lands on the old address. The site must stay
     correct, generic or re-specialised.

   The same split affects `LOAD_ATTR_SLOT` and `STORE_ATTR_SLOT` for
   members. They guard only the live shape's version, which is
   correct, but may alternate between a generalised and an
   ungeneralised shape of one type. Recorded, with a test that the
   values stay right.

### A5. The store plan and every writer of shape slots

- **The resolver.** `_PyShape_ResolveStore(..., op, value_kind, plan)`
  takes the value's kind, as note 09 section 4.5 specifies.
  - It returns REPLACE when the slot's kind fits, and the new outcome
    `_Py_STORE_GENERALISE` when it does not.
  - `_PyStorePlan` gains `slot_kind` in its pad byte: the field's kind
    for REPLACE and GENERALISE; for ADD, the edge's field kind if the
    edge exists, else the value's kind.
- **Writers.** Each one checks the kind:
  - `generic_setattr_value` (object.c): REPLACE writes;
    GENERALISE calls `_PyShape_Generalise()`, then writes.
  - `_PyObject_ShapeStoreAttr`, which ADD and REMOVE use, on both its
    replace and its add paths. An existing ADD edge whose field does
    not fit is generalised before the commit.
  - `STORE_ATTR_SHAPE` and `STORE_ATTR_SHAPE_TRANSITION`, which are
    guarded (A6).
  - The other slot writes store NULL: clear, delete, `publish_dict`,
    `ClearManagedDict`. `__slots__` members are not shape slots and
    stay BOXED.
  - A grep for `_PyObject_Slots(` and for plan-offset writes found no
    other writer.

### A6. Handlers and feedback

- **Feedback.** `_PyFbAttr` gains `slot_kind`, in a union with
  `dictoffset` (only the WITH_HINT handlers read `dictoffset`).
- **The guard uop.** `_GUARD_SLOT_KIND (slot_kind/0, value, owner --
  value, owner)` does `EXIT_IF(!_PyValue_FitsKind(value, slot_kind))`.
  - `STORE_ATTR_SHAPE` and `STORE_ATTR_SHAPE_TRANSITION` run it after
    the shape guards.
  - Per-kind handlers are not possible, because opcodes 125-127 are
    the only free specialised numbers. One uop also keeps the kind a
    constant for the JIT.
  - Tier 1 does run the vacuous BOXED compare, about three
    instructions. Its cost is recorded.
- **The specialiser.**
  - `_Py_Specialize_StoreAttr` gains the value (it is on the stack at
    `_SPECIALIZE_STORE_ATTR`).
  - It records `plan.slot_kind`, prepares new edges with the value's
    kind, and declines GENERALISE and an ADD whose edge kind does not
    fit. The generic store then generalises, which re-versions the
    shape, and the next attempt sees BOXED.
- **Loads.** The interpreter's load handlers do not change; they never
  test the tag. The plan already carries `slot_kind`. Feedback records
  the kind it observed, because a borrowed shape must never be
  dereferenced (A4.3):
  - `LOAD_ATTR_SHAPE` writes it to `fb->slot_kind`;
  - each poly row gains `slot_kind`.

  A compiler may use this snapshot only behind the runtime shape and
  version guard.
- **Poly rows across generalisation.** When the specialiser records a
  row whose shape pointer is already in the table at an older version,
  it replaces that row instead of appending. The live owner's shape is
  the same pointer, so the old row is obsolete. This keeps
  invalidations from filling the four-row budget and turning a
  two-shape site MEGA.
- **Magic.** The handler set and the feedback layout change, so the
  same commit bumps the pyc magic.

### A7. Tests (test_shapes, through `_testinternalcapi.shape()` kinds)

These are the rung 4 items of note 09 section 9:

- The first store picks I32, F64 or BOXED, for these values: int32
  bounds; an int out of range; a NaN; the infinities; ±0.0; a bool;
  an int subclass; a float subclass.
- Kinds alternating int and float over shared shapes.
- Warmed `STORE_ATTR_SHAPE` and `_TRANSITION` sites given each
  non-fitting value. The value lands, the field is BOXED in the owner
  and its subtree, the versions are fresh, the sibling subtrees and the
  root are unchanged, and the site specialises again.
- Generalisation followed by:
  - an ADD;
  - a delete of the last field and re-add (back through the parent
    to a BOXED owner);
  - a hole deletion.

  These run across shared shapes, including the note's
  `o.x = 1; o.y = 2; o.x = "s"; del o.y` example.
- A specialiser-prepared edge takes the value's kind.
- A two-shape poly load across more than four generalisations stays
  `LOAD_ATTR_SHAPE_POLY`.
- The shape address-reuse test (A4.3).
- A reentrant MRO lookup: a key whose `__eq__` changes the class during
  lookup leaves nothing cached under the new version.
- Generalisation on a type without a version, and with the tag counter
  exhausted (`_testcapi` version hooks): the shapes get version 0 and
  the value still lands.
- A class mutation after a generalisation.
- Incompatible stores through `object.__setattr__`, `setattr` and
  `PyObject_SetAttr`, followed by `gc.collect()` with a boxed referent
  held only in the slot.
- A property load and a poly load on a generalised shape specialise
  and stay correct.
- Values and `is` stay unchanged after generalisation.
- Conversion to dictionary mode copies the slots unchanged.
- `__slots__` members stay BOXED.

Level 1 ladder: the rung 3 list.

### A8. Measurements (recorded in rung 5)

- Store-guard cost: `attr_store` and `attr_store_poly`, in instructions.
- Generalisation fan-out from the counters, on the kernels.
- richards, go, nbody, raytrace, deltablue and chaos per loop.

### A9. Commits

1. The version-pairing fixes (A4, items 1 and 2) with their tests.
   These are correct on their own.
2. Kinds, generalisation, resolver and writers, the guard uop, the
   feedback field, the specialiser, the magic bump and the tests.
   These go together because an unguarded specialised store would
   break a promise.
(`str +=` has no commit in this rung; section B.)

## B. `str +=` (ruled out of 6c)

The coordinator's ruling of 2026-10-27:

- The in-place append was reference counting's, lost at 5c. Its sound
  replacement is an escape proof, which belongs to step 9: a trace
  proves the local unaliased.
- A compiler-emitted builder in which every introspection path seals is
  the same proof done statically. It is the fallback if the trace proof
  does not cover the common loop.
- A rope is out: it changes str's representation, and `PyUnicode_DATA`
  cannot fail.
- 6c accepts the quadratic cost, as PyPy does. `micro/str_concat_loop`
  records it, and note 08 section 3 has the row (ee167b42a18).

For the fallback, the review found two holes in this doc's first sketch,
which the step that takes it must close:

- **Exempt only the successful append.** Only the successful exact-str
  append may skip the seal. A fallback through `__radd__` can retain
  the left operand.
- **A growth contract is needed.** It must cover how capacity is
  recovered, checked geometric sizing, failure and rooting, and
  `s += s`.

## Review

The review (codex, 2026-10-27, `reviews/6c-rung4-review.md`) gave the
verdict "revise". Each blocking issue:

1. **The cacheable check lost the current-MRO guard.** Accepted; A4.1
   keeps `version == tp_version_tag` and adds the reentrant test.
2. **Reading the kind through a borrowed shape.** Accepted: the kind is
   recorded in feedback and in poly rows (A6, "Loads"), and the
   never-dereference invariant is stated (A4.3).
3. **Obsolete rows fill the poly budget.** Accepted: a row for the same
   shape pointer is replaced (A6), with a test.
4. **and 5. The `str +=` builder.** Moot in 6c, by the ruling. Both are
   recorded in section B for step 9's fallback.

Non-blocking:

- **publish_dict.** It writes the dict pointer into slot 0 over a
  possibly I32 or F64 field. That is a commit sequence (slot 0, then
  the header) with no call or safepoint between, and it leaves the
  dictionary-mode shape, which has no kinds. No generalisation is
  needed; a comment says so.
- **The extra failure tests.** Added to A7.
- **A4.3.** Fixed by ruling 4.
- **Tier wording.** It now says "interpreter".

## As built

Commits:

- ee167b42a18: the `str +=` ruling. Adds `micro/str_concat_loop`
  (1000 appends onto a 10,000-character local: 2.4 ms per loop, against
  0.52 ms for `str_concat`, on dev-default) and the note 08 register row.
- 0c0ae6f6fcb: shape and type versions paired by consumer (A4), plus the
  replacement of a poly row of the same shape. Magic 3725.
- e35552a3343: slot kinds, generalisation, the resolver's GENERALISE
  outcome, `_GUARD_SLOT_KIND`, and kinds in feedback. Magic 3726.

Decisions made in the code:

- **No per-kind handlers.** `_PyValue_FitsKind()` reads an `extern`
  range table (`_PySlotKind_Ranges`) rather than branching on the kind,
  so the guard is one form for every kind.
- **Generalisation is a preparation step.** `_PyShape_Generalise()`
  walks the owner's subtree iteratively, through the `children` and
  `sibling` links, and takes the version from
  `_PyType_NewShapeVersion()` under the type lock.
- **A specialiser that sees a misfit declines.** It does not
  specialise and fail its own guard; the generic store generalises,
  and the next attempt finds BOXED.
- **The generator.** `_PyValue_FitsKind` joins its non-escaping list.
- **Test helpers.** `_testinternalcapi.set_next_type_version()` takes a
  PyValue: METH_O passes the value, and a small int is inline.

### Hand-off

- **Members alternate across a generalisation.** `LOAD_ATTR_SLOT` and
  `STORE_ATTR_SLOT` guard only the live shape's version. On a type with
  both `__slots__` members and generalised inline fields, a member site
  alternates between shapes of the type. This is correct and rare;
  guarding the type's version would cost every member site a load.
  Recorded for rung 5's measurements.
- **The BOXED store pays a vacuous compare.** Its cost is measured in
  rung 5 (`attr_store`, `attr_store_poly`).
