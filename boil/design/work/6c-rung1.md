# 6c rung 1: the shape in the header

Status: implemented; written after the fact as the record of the rung's
decisions (PROCESS.md came into force while the rung was in flight; the
coordinator waived its review). Specification: note 09 sections 2, 3 and 9
item 1. The decisions below were approved by the coordinator on
2026-10-27 before the code was written.

## What changes

- `struct _PyShape` (Include/object.h, because `Py_TYPE()` reads it):
  `type`, `flags`, `type_version`, `nslots`, `shape_kind`, `edge_slot`,
  `parent`, `edge_name`, `children`, `sibling`, `slots`; 64 bytes. The
  shape tree's helpers are in Include/internal/pycore_shape.h and
  Objects/shapeobject.c.
- `ob_header` holds the shape in bits 0-47 and the collector's flags in
  48-63. `Py_TYPE(o)` is `_PyObject_SHAPE(o)->type`; `Py_SET_TYPE()` stores
  the type's root shape and keeps the flags.
- The `PyXxx_Check` macros that test a `*_SUBCLASS` bit read
  `shape->flags` (`_PyObject_FastSubclass()`); the exact checks of static
  types are `_Py_IS_ROOT_SHAPE(ob, type)`, a compare of the masked header
  with the type's root shape.
- `_PyValue_Shape(v)`: the root shape of int or float for an inline value,
  the header's shape otherwise.
- `--with-shapes` and `Py_SHAPES` are deleted: step one's shapes are
  always on, and the header is their one record (the shape byte in
  `PyDictValues` is gone).
- `_PyFbAttr` grows from 40 to 48 bytes with a `_PyShape *shape` field;
  `shape_to` is a pointer in the descr union; `_PyFbPoly` gains
  `shape[4]`. Magic 3721.

## Decisions (each goes to note 06 section 20)

1. **Where a static type's root shape lives.** Every type embeds its root
   shape as the last field of `PyTypeObject`, `tp_root_shape`. The header
   of a static object is then an address constant,
   `&type->tp_root_shape` plus the flags (`_Py_STATIC_HEADER_INIT`); a
   NULL type gives a NULL shape, which `PyType_Ready()` replaces. The
   positional initializers of static types leave the embedded shape zero,
   so `_PyTypes_InitRootShapes()` fills type, flags and version of every
   static builtin type and static exception at the start of
   `_PyRuntimeState_Init()`, before any static object is used;
   `type_ready()` fills them again for every type. A static type of an
   extension gets its root at `PyType_Ready()`, which the API already
   requires before its objects are used. Amendment (after gate 2 found
   segfaults in test_code and test_monitoring): some core static types are
   never readied (`_PyBranchesIterator` in Python/instrumentation.c, made
   through `PyType_GenericAlloc`), so `_PyObject_Init` fills a type's root
   shape at its first object when it is still empty. A debug build asserts
   in `Py_TYPE()` that the header has a shape and the shape a type, so an
   object of a type with no root shape at all fails there instead of
   crashing later. `Py_TYPE()` is not NULL-safe: the
   four sites that asked whether a type was set yet (`type_ready_set_type`,
   structseq, `_PyErr_NoMemory`, the extension loader) test
   `_PyObject_SHAPE(o) == NULL`.
2. **`shape->flags` is a copy of `tp_flags`**, not a pointer: a pointer
   costs the two loads of `Py_TYPE(o)->tp_flags`. `type_set_flags()`, the
   one writer in typeobject.c (`_PyType_SetFlags()` now goes through it),
   rewrites it in every shape of the type; `type_ready()` and the early
   pass copy it. The Check macros rely only on bits fixed when the type is
   readied.
3. **`shape->type_version` mirrors `tp_version_tag`**: `set_version_unlocked()`,
   the one writer, writes it into every shape of the type.
4. **The header is the one record.** In the default build no object
   carried a shape before this rung (`Py_SHAPES` was off), so "the shape
   byte and the header agree" is realised by deleting the byte and making
   step one's tracking unconditional. A heap type with inline values owns
   a table (`PyHeapTypeObject.ht_shapes`): the root first, at most 64
   shapes, and one dictionary-mode shape that stands for step one's
   `_Py_SHAPE_NONE` (a materialised `__dict__`, invalid inline values,
   past the limits, after `__class__` assignment). Dictionary-mode shapes
   are per type, not shared, because `shape->type` must be the type. A
   slot is still the attribute's index in the shared keys, so a shape can
   have holes. Non-root shapes are PyMem blocks with their slot table
   after them, freed with the type; the type's traverse visits their edge
   names (also held by the shared keys in this rung).
5. **Exact test macro.** `_Py_IS_ROOT_SHAPE(ob, type)`; valid for a type
   without `Py_TPFLAGS_INLINE_VALUES`, which every static type is.
   Applied by rewriting every `Py_IS_TYPE(op, &Py..._Type)` exact check in
   Include/ (py_curses.h left alone: its type comes from a capsule).
6. **Header layout** (coordinator ruling): note 09's layout stays for 6c.
   Every handler's shape guard is the one form "masked header ==
   immediate". Note 08's row moves to step 7 (side bitmaps beside the
   object-start map).
7. **Readers outside the interpreter.** `_Py_DebugOffsets` gains a
   `shape` section (size, offset of `type`); `_remote_debugging` reads the
   type through it; gdb's libpython.py follows header, shape, type.
8. **`type.__sizeof__`** counts the shape table and the non-root shapes.

## Invariants

- `shape->type == Py_TYPE(o)` for the shape in `o`'s header, always
  (`_PyObject_SetShape()` asserts it).
- An object of a type with inline values whose values are valid and that
  has no `__dict__` has the shape of its insertion order; otherwise it has
  the type's dictionary-mode shape. A dict that aliases the inline values
  implies dictionary mode.
- Shapes outlive every object and feedback entry that holds them; a
  feedback entry's shape is compared only after the type version guard.

## Risks

- A static type outside the early lists whose static instances are used
  before its `PyType_Ready()` would make `Py_TYPE()` return NULL; the
  suite on both builds is the check.
- `Py_SET_TYPE()` on an object with attribute slots resets its shape to
  the root; `object_set_class` sets dictionary mode after it.

## Tests

test_shapes (previously skipped without `--with-shapes`) runs on both
builds; test_opcache expects the shape handlers unconditionally; test_sys
sizes for the embedded root shape, `ht_shapes` and the counted table;
test_gdb's corrupt-type expressions follow the shape. Gate 2 on
dev-default, gate 3 on dev-debug.

## Measurements (recorded, not gated)

Before is b6fb7543511 (rung 0), after is this rung; release builds,
`boil/bench/run.py --metric instr`, user-mode instructions per loop, four
runs each (spread under 0.01%).

| | before | after | change |
|---|---|---|---|
| richards | 526.53M | 530.62M | +0.78% |
| go | 1071.93M | 1086.89M | +1.40% |
| json_loads | 7.705M | 7.749M | +0.58% |

Attribution: the change mixes the extra dependent load of `Py_TYPE()`
(`shape->type`) with step one's shape tracking, which was off by default
before this rung and is now always on (shape writes on every attribute
add and delete, the shape handlers at monomorphic sites). The suite does
not separate the two; rung 3 replaces the tracking.

Sites: `Py_TYPE(` 1,836 and `Py_IS_TYPE(` 185 in Objects/, Python/,
Include/, Modules/ and Parser/; in Python/bytecodes.c, 57 `Py_TYPE` and
26 `Py_IS_TYPE` or `PyValue_Type`. Header masks (`movabs
$0xffffffffffff`) in `_PyEval_EvalFrameDefault`: 269 before, 278 after;
the function has 28,245 instructions before and 27,680 after.
