# Proposal: PyValue representation packet (D2, D3, D5, G1', G3, G10, G11)

Status: proposal, revision 2, 2026-11-03. Reviewed in
[review 1](../reviews/pyvalue-representation-packet-review-1.md); all 12
required changes are accepted.
Design reference: [pyvalue.md](../pyvalue.md) §1 and §3 (decisions D2, D3, D5).
Source tree: `build/worktrees/pyvalue-rc-universal`, branch
`boil/pyvalue-rc-universal` at `e704ab282d9` plus the uncommitted
native-boundary P3 edits present at 17:25 PST (Clinic regenerated, the
`_PyArg_OwnerScope` family gone). Counts are `rg` counts over
`Objects Python Modules Include Parser` and include generated files
(`generated_ir_cases.c.h`, `generated_jit_helpers.c.h`, `Python-ast.c`)
unless stated. Recount at the start of each step.

Out of scope: static borrowing and the typed borrow (G6/D4, decided with the
register IR's G2), the IR v2 encoding revision (it follows this packet,
register-ir.md IR-D1), the native-boundary packets, boxing counters by site
and the type-slot boundary (G4, G13).

## Changes after review 1

1. Step 3 checks static immortals only on tag-0 objects at publication and
   home stores; `CLOSE`, `XCLOSE` and `PyValue_CheckValid`'s pointer-space
   assert are unchanged (review F3).
2. Step 4 adds `IR_BORROW_KNOWN`, makes `PyValue_CheckValid` return early for
   0, and runs the full debug suite after the step (F3, F4, F22).
3. Step 2 checks addresses where memory comes from the OS, not per object
   (F15).
4. Step 5 checks the release contract in `PyObject_Free`, `PyMem_Free`,
   `PyMem_RawFree` and the reftracer hook, states the allocation half of the
   contract, lists the reentry fixtures to delete, and says that the
   `PrepareOwned` call sites lose their retired blocks (F10, F11, F21).
5. The `ceval.c` revalidation is classified by what precedes it (§3.4). Every
   check is preceded by an operation that can run Python or by the release of
   an arbitrary object, so step 5 keeps all of them and deletes only the
   exception save and restore inside them (F12).
6. Step 6 is split into 6a (direct sites), 6b (helpers, found by the shape of
   their bodies and mapped to the exact release in each body) and 6c (class
   P, widened). The rewrite also folds a detach that precedes the save and
   unwraps bare blocks (F6–F9, F20).
7. `_PyValue_RestoreException` is deleted at the end of step 7 (F5).
8. Step 8 removes the canonicalising constructors from the generator's
   `POINTER_CONSTRUCTORS`, audits the 224 constructor call sites for later
   object use, and uses the canonical `Steal` at the 16 `StealMortal` sites
   with no non-number variant (F13, F14, coordinator decision on the open
   choice).
9. Step 9a range-checks `len` and `FOR_ITER_RANGE`, adds the missing test
   pins and boundary tests, and derives the home checker and the constructors
   from one range helper (F16, F17, F19, F23).
10. Steps 9a and 9b form one validation point unless no JIT test depends on
    int traces (F18).
11. §4 adds a record after step 7, with code size, and a `build_tuple_small`
    micro (F24).
12. Text fixes: §1.3 owned-test reason, §1.2 NaN images, step 1 line count
    (F1, F2, F25).

## 0. State of the tree before the packet

Already done in the tree:

| Item | Evidence |
|---|---|
| G3 mask | `pycore_value.h`: `Py_OWNED_MASK` is `(0xFFFF << 48) \| Py_TAG_BITS`; `_PyValue_PointerOwnsCount` tests `Py_TAG_BITS` |
| G1' item 4 | `pyvalue.h:PyValue_Borrow` sets bit 0 only when `PyValue_IsObject` |
| G1' item 2 | `PyValue_FromPyObjectSteal`, `_PyValue_FromPyObjectNew` and `PyValue_NewRef` all use `_Py_IsImmortal` |
| G1' item 1 (build break) | `Include/Python.h:81` includes `pyvalue.h` only under `#ifndef Py_LIMITED_API` |
| `traverse_slots` | `typeobject.c:traverse_slots` uses `_Py_VISIT_VALUE` |
| G11 `_WIN32` forms | No `_WIN32` left in `pycore_value.h` |
| G1' item 6, close half | `Py_DECREF_MORTAL` and `_Py_INCREF_MORTAL` assert `!_Py_IsStaticImmortal` in debug builds (`pycore_object.h:412`, `446`) |

Still open, and taken by this packet:

| Item | Where |
|---|---|
| G1' item 3: `BUILD_TUPLE` uses `PyValue_FromPyObjectStealMortal` on a result that may be the static empty tuple | `bytecodes.c:BUILD_TUPLE`, `tupleobject.c:_PyTuple_FromValueStealOnSuccess` |
| G1' item 5: no heap-safe assert on local stores | `ceval_macros.h:IR_STORE_DST`, `IR_MOVE_TO_DST` |
| G1' item 6, publication half | `pycore_value.h:_PyValueTemps_Publish`, value-form setters |
| G11: no-inline branch; 12 `Py_GIL_DISABLED` and 7 `Py_VALUE_INLINE_VALUES` uses in `pycore_value.h`; 20 `Py_VALUE_INLINE_VALUES` uses in 8 files; `_PySlotKind` (7 uses, 2 files); 4 region literals in `pyvalue.h` | |
| G10: no address check | `obmalloc.c:new_arena`, mimalloc's OS layer, raw allocator |

If the separate G1' packet lands first, drop its items from step 3.

## 1. Target encoding

### 1.1 Constants

Defined once, in `Include/pyvalue.h` (with a `_Py_VALUE_` prefix) and used by
name in both headers:

| Constant | Value |
|---|---|
| `Py_TAG_REFCNT` | `1` |
| `Py_TAG_INVALID` | `2` |
| `Py_TAG_BITS` | `3` |
| `Py_PTR_LIMIT`, `Py_INDEX_BASE` | `1 << 48` |
| `Py_DOUBLE_OFFSET` | `1 << 49` |
| `Py_INT_BASE` | `0xFFF8 << 48` (was `0xFFFE << 48`) |
| `Py_INT_SHIFT` | `13` (new: 64 − 51) |
| `Py_INT_MIN`, `Py_INT_MAX` | `-(1 << 50)`, `(1 << 50) - 1` (new) |
| `Py_OWNED_MASK` | `(0xFFFF << 48) \| Py_TAG_BITS` (unchanged) |
| `PyValue_NULL` | bits `0` (was `1`) |
| `PyValue_ERROR` | bits `2` (unchanged) |

One inline helper decides the inline int range, and every other test uses it:
`_PyValue_IntFitsInline(int64_t v)`. It is int32 until step 9a and
`Py_INT_MIN <= v && v <= Py_INT_MAX` after. Its users are the constructors,
`_PyObject_ValueIdentity`, `PyValue_FromInt64` and the debug home checker.

### 1.2 Bit layout after the packet

| Bits (hex) | Region | Meaning |
|---|---|---|
| `0000_0000_0000_0000` | sentinel | `PyValue_NULL`: absent, empty slot, end of iteration. Zero-filled memory is NULL |
| `0000_0000_0000_0001` | unused | Old NULL. Never produced; the debug check rejects it |
| `0000_0000_0000_0002` | sentinel | `PyValue_ERROR` |
| `0000_0000_0000_0003` | unused | Never produced; the debug check rejects it |
| `0000_xxxx_xxxx_xxxx`, low 2 bits `00`, value ≥ 8 | pointer | Owned strong reference |
| same, low 2 bits `01` | pointer | No count owned: borrow or immortal (G6 later narrows this to immortal in homes) |
| same, low 2 bits `10` | pointer | Debug builds only: `PyValue_Wrap` of a non-object pointer |
| `0001_xxxx_xxxx_xxxx` | index | int48, −1 to 2^47 − 1, frame temporaries only (unchanged) |
| `0002_0000_0000_0000` – `7FF2_0000_0000_0000` | float | IEEE bits + 2^49, +0.0 to +inf (unchanged) |
| `7FF2_0000_0000_0001` – `8001_FFFF_FFFF_FFFF` | unused | Images of positive NaNs |
| `8002_0000_0000_0000` – `FFF2_0000_0000_0000` | float | −0.0 to −inf (unchanged) |
| `FFF2_0000_0000_0001` – `FFF7_FFFF_FFFF_FFFF` | unused | Never produced |
| `FFF8_0000_0000_0000` – `FFFF_FFFF_FFFF_FFFF` | int51 | `Py_INT_BASE \| (v & (2^51 − 1))`, v in [−2^50, 2^50) |

Images of negative NaNs start at `FFF2_0000_0000_0001`, run through the whole
int51 region, and wrap modulo 2^64 into `0000_0000_0000_0000` –
`0001_FFFF_FFFF_FFFF`, which holds NULL, ERROR, pointers and indices. That is
why no NaN is ever inline. Every pattern in the int51 region is a valid int;
the old unused patterns (`0xFFFE` with bits 32–47 set, and `0xFFFF`) are gone.

### 1.3 Operations on the encoding

| Operation | Definition | Instructions |
|---|---|---|
| `PyValue_IsInlineInt(v)` | `bits >= Py_INT_BASE` | one compare |
| `PyValue_BothInlineInt(a, b)` | `(a & b) >= Py_INT_BASE` (exact: the AND keeps the top 13 bits set only if both have them) | and, compare |
| `PyValue_IsInlineFloat(v)` | `bits - Py_DOUBLE_OFFSET < Py_INT_BASE - Py_DOUBLE_OFFSET` (unchanged form) | sub, compare |
| `PyValue_IsInline(v)`, `IsInlineAt(p)` | `bits >= Py_DOUBLE_OFFSET`; `cmpw $2, 6(p)` (unchanged) | one compare |
| `_PyValue_IntScaled(v)` (new) | `(int64_t)(bits << 13)` = value × 2^13 | shift |
| `_PyValue_FromIntScaled(s)` (new) | `((uint64_t)s >> 13) \| Py_INT_BASE` | shift, or |
| `PyValue_InlineInt64(v)` (renamed) | `_PyValue_IntScaled(v) >> 13` (arithmetic) | two shifts |
| scaled add and subtract | `a_s + b_s`; int64 overflow is exactly int51 overflow | add, `jo` |
| scaled multiply | `a_s * b` (b unscaled) | imul, `jo` |
| `PyValue_RefcountOnObject(v)` | `bits != 0 && (bits & Py_OWNED_MASK) == 0` (D5) | two tests |
| `_PyValue_OwnsCountNotNull(v)` (new; `CLOSE`, `DUP`, `AsPyObjectSteal`, with a debug NULL assert) | `(bits & Py_OWNED_MASK) == 0` | one test |
| `PyValue_XCLOSE(v)` | `if (RefcountOnObject(v)) Py_DECREF_MORTAL` | two tests |
| `_PyValue_XCLOSE_POINTER(v)` | `bits != 0 && (bits & Py_TAG_BITS) == 0` | two tests |
| `PyValue_IsNull(v)` | `bits == 0` | one test |
| `PyValue_IsObject(v)` | `bits > 2 && bits < 2^48 && !(bits & 2)` (unchanged) | |
| `PyValue_Is(a, b)` | `((a ^ b) & ~IsPointerSpace(a)) == 0`, no fallback (D2). The bit-0 mask stays until G6 removes borrows from homes | xor, compare |
| `PyValue_IsLanguageValue(v)` | object, or `bits >= Py_INT_BASE`, or a non-NaN float image | |

With NULL at 0, the owned test needs two tests. No one-test form works: with
`m = (bits - 1) & Py_OWNED_MASK`, the test `m == 0` rejects every owned
pointer (`p - 1` has its low two bits set), and `m == Py_TAG_BITS` accepts
owned pointers but also index 0 (`0x0001_0000_0000_0000 - 1` gives
`0x0000_FFFF_FFFF_FFFF`, and `& mask` is 3). D5 accepted the second test.

### 1.4 Invariants and their debug checks

| # | Invariant | Debug check that enforces it | Step |
|---|---|---|---|
| I1 | Zero is NULL. Patterns `1` and `3` never occur | `PyValue_CheckValid` returns at once for 0 and asserts `bits != 1 && bits != 3`; its pointer-space assert and tag-0 object load are unchanged | 4 |
| I2 | Object addresses are below 2^48 | Fatal check where memory comes from the OS (step 2) in all builds; `assert((uintptr_t)obj < Py_PTR_LIMIT)` in every `PyValue_FromPyObject*` | 2 |
| I3 | No NaN is inline; nothing is produced in the unused ranges | `_PyValue_CheckHome` rejects float images that decode to NaN; `frame.c:is_well_formed` keeps its NaN test | 3 |
| I4 | Canonical form in every home: an exact `int` for which `_PyValue_IntFitsInline` holds, or an exact non-NaN `float`, is never held as a pointer in a frame slot, container element, field, cell, dict key or value, or set key | `_PyValue_CheckHome(v)`: report-only counter in step 3, assert from step 8; called from the value-form setters, `IR_STORE_*`, `_PyGC_VisitValue` and `frame.c:is_well_formed` | 3, 8 |
| I5 | Static immortals always carry bit 0 | Publication and home stores: `_PyValueTemps_Publish` and `_PyValue_CheckHome` assert `!_Py_IsStaticImmortal` for tag-0 objects only. Closes are already covered by `Py_DECREF_MORTAL`'s assert | 3 |
| I6 | Indices only in frame temporaries; NULL, ERROR and indices never in heap homes | `frame.c:is_well_formed` (exists); `PyValue_IsLanguageValue` asserts in setters (exist) | — |
| I7 | Locals hold heap-safe values | `assert(PyValue_IsHeapSafe(v))` in the local branch of `IR_STORE_DST` and `IR_MOVE_TO_DST` | 3 |
| I8 | `_PyValue_Hash(v) == hash(PyValue_AsObject(v))`, and `compare_inline` agrees with the boxed comparison | Tests over the int51 boundaries and int/float pairs (step 9a list) | 9a |
| I9 | Release contract: freeing an exact `int` or `float` box, allocator hooks (allocation and free) and reftracers never run Python, never raise and leave the current exception unchanged | In debug builds, `PyObject_Free`, `PyMem_Free`, `PyMem_RawFree` and `_PyReftracerTrack` record the thread's current exception and current frame on entry and assert both unchanged on exit. This covers interpreter frees that bypass `py_dealloc`'s gh-89373 check (`PyValue_CLOSE_SPECIALIZED` with `_PyLong_ExactDealloc`/`_PyFloat_ExactDealloc`, 50 uses in `generated_ir_cases.c.h`; the `Py_DECREF` override in `Python/ceval.h`) and hooks installed later with `PyMem_SetAllocator`. The allocation entry points get the same check | 5 |
| I10 | Every pattern in `[Py_INT_BASE, 2^64)` decodes to one int and every int in range has one encoding | Static asserts on the constants; round-trip tests at −2^50, 2^50 − 1 and their neighbours | 9a |

`_PyValue_CheckHome(v)` = `PyValue_CheckValid` + not index + not ERROR + not
`1`/`3` + (pointer ⇒ not an exact int for which `_PyValue_IntFitsInline`
holds and not an exact non-NaN float) + (tag 0 ⇒ not a static immortal). It
is compiled only under `Py_DEBUG`.

## 2. Steps

| Step | Content | Kind | Agent-h |
|---|---|---|---|
| 0 | Preconditions, JIT test survey, "before" measurements | measurement | 1.5 |
| 1 | G11: one profile, one set of constants, delete `_PySlotKind` and dead free-threaded blocks | mechanical | 1.5 |
| 2 | G10: 48-bit address checks at the OS boundary | judgment, small | 1 |
| 3 | G1' remainders, the range helper and the debug home checker (report-only) | judgment | 2 |
| 4 | D5: `PyValue_NULL` = 0 | judgment | 3.5 |
| 5 | D2 core: release contract, delete `retired` and the retire path | judgment | 5 |
| 6a | D2 sweep: direct release-only save/restore sites | mechanical (script + Haiku batches) | 2.5 |
| 6b | D2 sweep: release helpers | mechanical with a derived mapping | 2.5 |
| 6c | D2 sweep: class P (`<failure> \|\| PyErr_Occurred()`) | mechanical | 2.5 |
| 7 | D2 residue: judgment sites, reentry defences; delete `_PyValue_RestoreException` | judgment | 5 |
| 8 | D2 canonical construction and bit-compare `PyValue_Is`; home checker becomes an assert | judgment | 4 |
| 9a | D3 int51 in the runtime and interpreter, numeric guards inline-only | judgment | 7 |
| 9b | D3 int51 in the trace JIT | judgment | 3 |
| 10 | "After" measurements | measurement | 1 |

Total about 42 agent-hours, plus review.

Forced orders:

- **P3 before 6a** (step 0): P3 rewrites files the sweep would touch.
- **5 before 6a–6c:** until step 5 replaces `_PyValue_Prepare` and the
  temps retire path, a box can be released while a canonical value is still
  unpublished, and the sweep would remove protection still in use. Step 5 also
  deletes the class A blocks around `retired` variables.
- **6a–6c before the end of 7:** `_PyValue_RestoreException` has callers
  until step 7 has handled the judgment sites.
- **8 before 9a (churn):** step 8 removes the boxed int and float paths from
  constructors and `PyValue_Is`; step 9a rewrites the numeric handlers once,
  with inline-only guards and int51 together.
- **9a with 9b (validation):** see step 9a.

Not forced: step 1 first saves later steps editing two definitions in
`pycore_value.h`; step 4 before 5 because it is smaller, fixes a known crash,
and changes `XCLOSE`'s definition, not its call sites.

### Step 0. Preconditions and "before" measurements

- The suite runs (bring-up). Record the pass/fail set of the debug and
  release builds; every later step keeps it.
- Native-boundary P3 is committed. At 16:58 the tree had 59 restore calls
  after `_PyArg_OwnerScope_End` and 74 in 29 stale Clinic headers; P3's
  working-tree edits removed all of them by 17:08. P3 also rewrites
  `getargs.c`, `call.c`, `descrobject.c`,
  `_operator.c:methodcaller_vectorcall`, `exceptions.c`, `Python-ast.c`
  (through `Parser/asdl_c.py`) and `native_scope.c`.
- JIT survey: list the cases in `Lib/test/test_jit_method.py` (1,148 lines)
  and the other JIT tests that need int traces (they assert
  `jit_method_compile(...)` or `*_seen` witnesses over int locals). Step 9a
  uses this list.
- Take the section 4 measurements on the release build and on upstream
  `3a440b65695`.

### Step 1. G11: one profile, one set of constants

- `Include/internal/pycore_value.h`: delete the `#else  // No inline values`
  branch (`PyValue_IsTaggedIndex` with `Py_INDEX_TAG`, the stub
  `PyValue_InlineInt`, `PyValue_TagIndex`, `PyValue_Box`), the
  `#ifndef Py_VALUE_INLINE_VALUES` `PyValue_FromInt32`, the non-inline
  `_PyValue_CLOSE_POINTER` aliases, and the `Py_GIL_DISABLED` arms of
  `PyValue_FromPyObjectSteal`, `_PyValue_FromPyObjectNew`,
  `PyValue_IsHeapSafe`, `PyValue_IsNone/IsTrue/IsFalse`,
  `_PyThreadState_PushCValue/PopCValue`, `_Py_TryIncrefCompareValue` and
  `_Py_TryXGetValue` (12 `Py_GIL_DISABLED` and 7 `Py_VALUE_INLINE_VALUES` uses).
- Delete `Py_VALUE_INLINE_VALUES` and its other 13 uses
  (`_testinternalcapi.c` 4, `pyvalue.c` 3, `_testinternalcapi/native_scope.c`
  2, `jit_record.c`, `ir.c`, `frame.c`, `_remote_debugging/frames.c` 1 each),
  keeping the inline arm.
- Move the region constants to `Include/pyvalue.h`; use them in
  `PyValue_IsObject` and `PyValue_IsLanguageValue` instead of 4 literals.
- Delete `_PySlotKind`/`_Py_SLOT_BOXED` (`pycore_dict.h` 4, `dictobject.c` 3).
- Delete the free-threaded-only blocks that use PyValue or deleted getters:
  13 blocks, 351 lines, in 8 files outside `pycore_value.h` (`codeobject.c` 3,
  `specialize.c` 2, `bytecodes.c` 2 and their 2 generated copies,
  `odictobject.c`, `genobject.c`, `pycore_structs.h`, `pycore_tstate.h`).
  `Python/gc_free_threading.c` is not compiled; leave it to a separate
  decision on free-threaded sources.
- Leave `PySequence_Fast_*` to native-boundary.

Test: build debug and release; the step-0 pass set. No new tests. Risk: low;
a block removed by mistake fails to compile.

### Step 2. G10: 48-bit address check

The tree uses mimalloc for the raw, mem and object domains
(`obmalloc.c:500–507`), so a check per mimalloc call would cost a compare per
object. Check where memory comes from the OS instead:

- `Objects/mimalloc/prim/unix/prim.c:_mi_prim_alloc`: one line in vendored
  code, once per segment. Mark the line so a mimalloc update keeps it.
- `Objects/obmalloc.c:new_arena` (pymalloc, selectable at run time).
- `Objects/obmalloc.c:_PyMem_RawMalloc`, `_PyMem_RawCalloc`,
  `_PyMem_RawRealloc` (`PYTHONMALLOC=malloc`); one compare per raw
  allocation, which that mode accepts.
- Each check is fatal (`Py_FatalError`) in all builds if
  `address + size > Py_PTR_LIMIT`.
- Debug assert in `PyValue_FromPyObjectSteal`, `_PyValue_FromPyObjectNew`,
  `PyValue_FromPyObjectBorrow` and `PyValue_FromObjectBorrow`.

Test: step-0 set under the default allocator and under `PYTHONMALLOC=malloc`
and `PYTHONMALLOC=pymalloc`. Risk: low. Cost: `new_instance`,
`tuples_10m` (expected unchanged).

### Step 3. G1' remainders, range helper, debug home checker

- `bytecodes.c:BUILD_TUPLE`: use `PyValue_FromPyObjectSteal` (the result may
  be the static empty tuple).
- `ceval_macros.h:IR_STORE_DST`, `IR_MOVE_TO_DST`:
  `assert(PyValue_IsHeapSafe(V))` in the local branch.
- Static immortals (I5): in `_PyValueTemps_Publish` and inside
  `_PyValue_CheckHome`, assert `!_Py_IsStaticImmortal(obj)` for tag-0 objects
  only. Leave `PyValue_CLOSE`, `PyValue_XCLOSE` and `PyValue_CheckValid`
  unchanged: `CheckValid` asserts pointer space, so calling it from a close
  would fire on every inline value and index.
- Add `_PyValue_IntFitsInline` (int32 range for now) and use it in
  `_PyObject_ValueIdentity`.
- Add `_PyValue_CheckHome` (§1.4) in report-only mode: it increments a debug
  counter readable as `_testinternalcapi.pyvalue_home_violations()` and
  records the first violating site. Call it from the value-form setters
  (`PyTuple_SET_VALUE`, `PyList_SET_VALUE`, `PyDict_SetValue`,
  `_PyMember_SetValue`, `_PyCell_SwapValue`, set insertion), the `IR_STORE_*`
  macros and `_PyGC_VisitValue`.

Test: step-0 set, plus a debug-suite run that reports the violation count
per test module; that list is the input to step 8. Risk: the static-immortal
assert can fire where things work today (Clinic `_kwtuple` items built from
raw `&_Py_ID(...)`, bring-up finding at 16:45); each firing is a real second
encoding and is fixed or listed.

### Step 4. D5: `PyValue_NULL` is 0

`pycore_value.h` unless noted:

- `PyValue_NULL_BITS` 0 (5 uses in 2 files); `PyValue_IsNull` is
  `bits == 0` (524 uses, unchanged text).
- `PyValue_RefcountOnObject` adds `bits != 0` (18 direct uses in 8 files;
  read each for a possibly-NULL argument). Add `_PyValue_OwnsCountNotNull`
  for `PyValue_CLOSE`, `PyValue_CLOSE_SPECIALIZED`, `PyValue_DUP` and
  `PyValue_AsPyObjectSteal`, which assert non-NULL.
- `PyValue_XCLOSE` (216 uses), `PyValue_CLEAR` (48), `PyValue_XSETREF` (2):
  definitions only; `XCLOSE`'s `assert(ref.bits != 0)` goes.
- `_PyValue_XCLOSE_POINTER` (44 uses in 3 files) adds `bits != 0`; its
  comment "PyValue_NULL owns no count, so this also serves as XCLOSE" is now
  false. `_PyValue_CLOSE_POINTER` (75) keeps one test.
- `_PyValue_BorrowBitAt` returns 0 for zero bits; otherwise
  `PyValue_BorrowAt(NULL)` gives `1` (15 uses of `BorrowAt`/`BorrowBitAt`
  in 6 files).
- `ceval_macros.h:IR_BORROW_KNOWN(REF, F)` (line 609; 80 uses in
  `generated_ir_cases.c.h`) ORs bit 0 into a "known pointer" operand read
  through a local. An empty local would become `1`. Assert
  `!PyValue_IsNull(REF)` where the generator proves the local bound, and use
  `_PyValue_BorrowBitAt` where it does not.
- `PyValue_IsHeapSafe`: drop the `ref.bits == PyValue_NULL_BITS` term (NULL
  now fails `(bits & 3) == 1`).
- `PyValue_CheckValid`: return at once for 0; assert `bits != 1 && bits != 3`.
- Code that gives zero a meaning other than NULL: `Python/pystate.c:702`
  (`ref.bits != 0 && !PyValue_IsNull(ref)`); `Python/typecache.c:16`
  `EMPTY_ENTRY` (becomes `{0}`); `Modules/_testcapi/list.c:164,166` (test
  `bits == 1`; also `_testcapi.list_unpublished_nulls`).
- Delete `typeobject.c:init_value_slots` and its call in
  `_PyType_AllocNoTrack`.
- NULL-fill loops after allocation: about 30 `for` loops writing
  `= PyValue_NULL` (`dictobject.c` 6, `pycore_interpframe.h` 3, `ceval.c` 3,
  `tupleobject.c` 2, `structseq.c` 2, `bytecodes.c` 2 plus generated copies,
  `listobject.c`, `getargs.c`, `Python/native_scope.c`,
  `_testinternalcapi/native_scope.c` 2, `itertoolsmodule.c`,
  `pycore_value.h`), and `IR_CLEAR_SLOTS` (27 uses in 3 files). Delete a loop
  where the memory came from a zeroing allocator; use `memset` otherwise.
  Stores that clear a slot for GC visibility (`_PyValue_CloseStack`) stay.
- Set tables: `setobject.c` tests `entry->hash == 0 && PyValue_IsNull(entry->key)`
  (lines 186, 325), so a zeroed table is a valid empty table; table
  allocation can use `PyMem_Calloc`.

Test: the **full debug suite**, not only the step-0 set: in release builds
`PyValue_CLOSE(NULL)`, `_PyValue_CLOSE_POINTER(NULL)` and `DUP(NULL)` were
no-ops when NULL was 1 and now call `Py_DECREF(NULL)`; only the debug asserts
find such calls, and only on paths that run. `test_pyvalue_datetime_readers`
(aborts today on a zero-filled tuple subclass from `tp_alloc`) must pass. New
tests: a tuple subclass and a `__slots__` class allocated by
`PyType_GenericAlloc` read as empty without `init_value_slots`; an extension
type allocated with `PyObject_Calloc` with a `_Py_T_VALUE` member; an unbound
local read through comprehension inlining (`LOAD_FAST_AND_CLEAR`) and after
`del`, in functions that also use the local as a call receiver or attribute
base (the `IR_BORROW_KNOWN` paths). Risk: medium.

### Step 5. D2 core: release contract and the retire path

- **Contract.** State in `pycore_value.h` ("Value identity") and in
  [gc-roots-and-lifetimes.md](../gc-roots-and-lifetimes.md): freeing an exact
  `int` or `float` box never runs Python and never changes the current
  exception; allocator hooks, for allocation as well as free, and reftracers
  (`PyRefTracer_SetTracer`) never run Python, never raise and never change the
  current exception. Upstream's documentation already forbids reftracers from
  raising or creating objects.
- **Debug enforcement (I9).** Checks in `obmalloc.c:PyObject_Free`,
  `PyMem_Free`, `PyMem_RawFree`, the matching allocation entry points, and
  `_PyReftracerTrack` (`pycore_object.h:111`). An assert in
  `_PyValue_Canonicalize` checks the same around the box free.
- **Fixtures that test the old model.** Delete these and their Python tests:
  `_testinternalcapi/dict.c:dict_reenter_malloc` (runs `PyDict_Clear` inside
  an allocation; `dict_value_pop_boxing_aba`), `_testinternalcapi/tuple.c`
  `tuple_reentry_*` (`tuple_value_boxing_reentry`),
  `_testinternalcapi/native_scope.c` `vector_change_*` and `end_free`,
  `_testinternalcapi/set.c:set_cleanup_free` (calls `PyErr_FormatUnraisable`,
  which can run `sys.unraisablehook`). Their Python tests are in
  `test_pyvalue_dict.py` and `test_pyvalue_tuple.py` (and the native-scope and
  set modules). gc-roots-and-lifetimes.md: "Do not weaken that contract to
  make a test inject an error."
- **`retired` out-parameter.** `PyValue_PrepareOwned(obj, &retired)` becomes
  `PyValue_PrepareOwned(obj)` and frees an inline-capable box at once (32
  uses in 15 files; public, so `pyvalue.h` changes). The call sites lose
  their `retired` variables and the class A blocks that release them (for
  example `itertoolsmodule.c:238–247`). `_PyValue_Prepare(source, &retired)`
  (3 uses in 2 files) becomes `_PyValue_Canonicalize(PyValue)`.
- **Retire path.** `_PyValueTemps_Publish` canonicalises and frees the box in
  place; delete `_PyValueTemps_PublishObject` (3 uses).
  `_PyValueTemps_RetireLocal`/`_CommitLocal` (224 uses in 5 files, almost all
  generated) keep the displaced local until the handler's publication point,
  because releasing a local can run `__del__`: replace the `pending_local`
  slot with one handler-local `PyValue ir_displaced` released where
  `_PyValueTemps_Close` runs today. This is a change to
  `Tools/cases_generator/ir_generator.py` (11 uses of `_PyValueTemps`, 5 of
  `value_boxes`).
- `_PyValueTemps` then holds only input boxes (`_PyValueTemps_Box`,
  `_PyValueTemps_Object`, 46 uses in 7 files). `_PyValueTemps_CloseNonempty`
  becomes a loop of `Py_DECREF` with no exception handling. Handlers that box
  nothing drop `value_temps` (register-ir.md G9 item 3).
- **Revalidation in `ceval.c`.** Keep every check (§3.4). Delete only the
  exception save and restore inside `value_operation_refresh` and the other
  revalidation functions, and rewrite the "Free hooks can change
  instrumentation" comment (`ceval.c:1615`) to name what can run Python
  there: the operation, and the release of operands, results and the
  displaced local.
- `value_ops.c`: delete `close_object` and `close_value`; simplify
  `_PyValue_Hash`'s object arm, which saves the exception around
  `PyObject_Hash` (upstream `PyObject_Hash` needs no save).
- `_PyValue_RestoreException` keeps its signature until the end of step 7.
  Its cycle walk goes now: it becomes `PyErr_SetRaisedException(saved)` after
  asserting that no new exception was raised.
- Regenerate `generated_ir_cases.c.h` and `generated_jit_helpers.c.h`.
- Recount the section 3 classes after this step; 6a–6c start from the new
  counts.

Test: step-0 set; `test_pyvalue_native_call_transaction`,
`test_pyvalue_repr_cleanup`, `test_pyvalue_call_bridges`. New tests:
`del` of a local holding an object with a raising `__del__` inside an
`except` block keeps the handled exception; a `__del__` triggered by
rebinding a local that calls `sys.monitoring.set_events` or `sys.settrace`
takes effect at the next instruction; a native call that does the same.
Risk: medium; the generator change touches every handler.

### Step 6a. Sweep: direct release-only sites

Classes A and B of §3.1 (688 sites at 17:25; recount after step 5), with the
rewrite in §3.2 rules 1–4. One file per batch for the large files
(`_operator.c`, `itertoolsmodule.c`, `genericaliasobject.c`,
`_datetimemodule.c`, `typeobject.c`), groups of small files otherwise;
`Python-ast.c` through `Parser/asdl_c.py` and regeneration. Each batch: run
the script, read the diff, build debug and release, run the module's tests.
Commit or attach the script with its list of refused sites.

### Step 6b. Sweep: release helpers

The helpers of §3.3 (about 1,100 calls). The script finds them by body shape
and derives each replacement from the body (§3.2 rule 5). Helpers that return
a status (`set_close_key`, `set_close_object`, `_PyTypeMutation_End`,
`collect_slot_error`) have a status rule. One helper per batch for the
large ones (`type_mutation_close` 349 calls, `dict_xdecref_object` and
`dict_close_value` 110 each, `dict_fixture_close` 108, `value_operation_close3`
81, `set_close_object` 74).

### Step 6c. Sweep: class P

`<failure test> || PyErr_Occurred()` after a call whose failure the left term
already reports (§3.5 rule 6), and the dead `X == NULL` branches after
`Py_NewRef`/`Py_XNewRef`. Standalone `if (PyErr_Occurred())` tests after a
release or store are listed for step 7, not rewritten.

Test for 6a–6c: step-0 set after each batch. No new tests: behaviour must not
change. Risk: low; the script refuses anything it does not recognise.

### Step 7. Residue, reentry defences, and the restore helper

- Classes C, D, E, F and U (128 sites at 17:25) and every site the scripts
  refused. Each site loses its save and restore and any revalidation that
  existed only for a raising release, or keeps it with a comment that names
  what can run Python there (a dict watcher, a weakref callback, a `__del__`
  before published state). Upstream practice is the reference.
- Standalone `if (PyErr_Occurred())` tests after a release or store
  (`itertoolsmodule.c` 4670, 4684; `typeobject.c` 3463, 3505;
  `genericaliasobject.c` 691 and similar): delete where the release cannot
  raise under the contract.
- Defensive code exercised by the fixtures deleted in step 5: revalidation
  after boxing and ABA checks (`dictobject.c` around value pop,
  `setobject.c` iterator repair around line 1357, the tuple boxing paths).
  Delete it, or keep it with a named reason. Comments that mention reentry,
  allocator hooks or ABA: 8 in `dictobject.c`, 11 in `typeobject.c`, 3 in
  `setobject.c`, 4 in `ceval.c`.
- Delete `_PyValue_RestoreException`, its declaration and the stale
  `// _PyValue_RestoreException()` include comments.
- Record the per-file `PyErr_Occurred()` count against upstream (§3.5) as the
  measure of what remains.

Test: step-0 set; `test_dict`, `test_set`, `test_weakref`, `test_gc`,
`test_finalization`, `test_exceptions`, `test_sys_setprofile`,
`test_monitoring`. Risk: medium; these are the places the old code guarded
against reentry.

### Step 8. Canonical construction and bit-compare `is`

- `PyValue_FromPyObjectSteal`, `PyValue_FromPyObjectNew` and
  `PyValue_FromPyObjectBorrow` (224 uses in 22 files together) canonicalise:
  an exact `int` for which `_PyValue_IntFitsInline` holds, or an exact
  non-NaN `float`, becomes inline (`Steal` frees the box; `New` and `Borrow`
  leave it).
- **Generator.** Remove these three, and `StealMortal`, `NewMortal` and
  `PyValue_FromPyObjectImmortal` (no call sites), from `POINTER_CONSTRUCTORS`
  in `Tools/cases_generator/generators_common.py:118`. Otherwise the
  generator emits `_PyValue_CLOSE_POINTER`, `_DUP_POINTER` and
  `_AsPyObjectStealPointer` on their results; these test `Py_TAG_BITS` only,
  and a release build would decref an inline int whose low two bits are `00`.
  Regenerate, and check generated code for `_POINTER` operations on these
  results. Audit the JIT's helpers (`jit_helpers_generator.py`) the same way.
- **Call-site audit.** For each of the 224 sites, look for a later
  `PyValue_AsPyObjectBorrow`/`Steal`, a `_POINTER` operation or `Py_TYPE` on
  the result. `typeobject.c:3453` is one ("Successful method/self outputs
  retain owned object views for native consumers, including
  WITH_EXCEPT_START; do not canonicalize them here", followed by
  `PyValue_AsPyObjectBorrow(*method)`). Such sites keep a `PyObject *` in a C
  local and convert it at the home store.
- Delete `PyValue_FromPyObjectStealMortal` (16 uses in 4 files) and
  `PyValue_FromPyObjectNewMortal` (2) and use the canonical `Steal`/`New`.
  No non-number variant (coordinator decision); the cost is recorded with
  `build_tuple_small`.
- `PyValue_Is` becomes the bit compare of §1.3. Delete
  `_PyValue_IsSameValue` and `value_identity` (`pyvalue.c`).
  `_PyObject_ValueIdentity` stays for `id()` (`bltinmodule.c:builtin_id_impl`)
  and `operator.is_`/`is_not`. Fix `genericaliasobject.c:tuple_index`, which
  compares a raw `PyValue_FromPyObjectBorrow` against tuple elements.
- `_PyValue_CheckHome` becomes an assert.

Test: step-0 set with zero home-checker violations; `test_tagged_values`
identity tests; `test_pyvalue_dict`, `test_pyvalue_set`. New tests:
`(5).bit_length()`, `(2.5).hex()`, `with` on an object whose `__enter__`
returns an int; a false `x is y` on distinct objects (record the step-3
counter of the removed fallback's true results before deleting it). Risk:
medium; a home store that bypasses every constructor shows up as a checker
assert.

### Step 9a. D3 int51 in the runtime and interpreter

- `pycore_value.h`: constants (§1.1), `_PyValue_IntFitsInline` to int51,
  `_PyValue_Int32Bits` (5 uses) becomes `_PyValue_IntBits(int64_t)`, scaled
  helpers. Rename `PyValue_FromInt32` (72 uses in 13 files) to
  `PyValue_FromInlineInt(int64_t)` (asserts the range) and
  `PyValue_InlineInt` (55 uses in 18 files) to `PyValue_InlineInt64`, so the
  compiler finds every caller that kept a 32-bit variable.
  `_PyValue_BoxInt64` (19 uses) becomes `PyValue_FromInt64`, inline when
  `_PyValue_IntFitsInline` holds and boxed otherwise.
- Numeric guards inline-only (D2): delete `PyValue_IsCompactInt` and
  `PyValue_CompactIntValue` (112 uses in 5 files: `bytecodes.c` 29,
  `specialize.c` 14, generated 60); delete the boxed arms of
  `PyValue_FloatOperandsCheck`, `PyValue_FloatCheckExact`,
  `PyValue_FloatValue`, `PyValue_NumberAsDouble` (57 uses in 6 files).
  `specialize.c` tests `IsInlineInt`/`IsInlineFloat`.
- `bytecodes.c` int handlers: add, subtract and multiply on scaled int64 with
  `__builtin_*_overflow`; negation, floor division, modulo and `abs` handle
  −2^50; `_LSHIFT_INT` (`_PyValue_LshiftInt32`, 4 uses) and `_RSHIFT_INT`
  use int64; `LOAD_SMALL_INT_INLINE` uses `PyValue_FromInlineInt`.
  `FOR_ITER_RANGE` (line 3901) and the `len` path (line 4729) use the
  range-checked `PyValue_FromInt64`, because `len(range(2**55))` and ranges
  that cross 2^50 leave int51. Compare-int handlers compare scaled values.
- `pyvalue.c:_PyObject_ValueIdentity` uses `PyLong_AsLongLongAndOverflow` and
  `_PyValue_IntFitsInline`; `_PyValue_BoxInlineValue` uses
  `PyLong_FromLongLong`.
- `value_ops.c:_PyValue_Hash` (int64; −1 maps to −2) and `compare_inline`
  (every int51 converts to double exactly; fix the comment).
- `pyvalue.h:PyValue_IsLanguageValue`: the int test becomes
  `bits >= Py_INT_BASE`.
- Compiler users of inline ints (`compile.c` 5, `assemble.c` 4,
  `symtable.c` 3, `codegen.c` 3, `flowgraph.c` 2): read each for an int32
  assumption.
- `frame.c:is_well_formed`: the inline-int arm is `true`.
- Out-of-process: `Tools/gdb/libpython.py:_PyValue_InlineValue` (lines
  153–165) decodes int51. Add `Py_INT_BASE`, `Py_INT_SHIFT`,
  `Py_DOUBLE_OFFSET`, `Py_PTR_LIMIT`, `Py_TAG_BITS` and the NULL bits to
  `_Py_DebugOffsets` (`pycore_debug_offsets.h`) with a version bump.
- Trace JIT: if the step-0 survey found JIT tests that need int traces, land
  9a and 9b as two commits with one suite record. Otherwise 9a makes
  `jit_record.c` refuse int traces until 9b.
- **Tests that pin int32:** `test_tagged_values.py` (boundaries, and the
  comment at line 52, "would alias the inline int 1", which is about the
  int32 encoding); `test_capi/test_list.py:35`, which hard-codes
  `(0xfffe << 48) | (value & 0xffffffff)`; the `test_pyvalue_*` modules that
  use 2^31 (`slots`, `set`, `call_bridges`, `small_readers`, `marshal`,
  `typecache`, `abstract`, `type_new`, `pyvalue`); `test_capi/test_getargs.py`
  checked.
- **New tests:**
  - Round trips at −2^50 − 1, −2^50, 2^50 − 1, 2^50.
  - Overflow at the boundaries: `abs(-2**50)`, `-(-2**50)`, `-2**50 // -1`,
    `divmod(-2**50, -1)`, `(2**50-1) + 1`, `(2**50-1) * 2`, `1 << 50`,
    `-1 << 50`, `(2**50-1) << 1`, `~(-2**50)`.
  - `len(range(2**55))`, iteration over `range(2**50 - 2, 2**50 + 2)`.
  - List and tuple indexing with int51 values raises `IndexError`, not a
    truncated index.
  - `struct.pack('q')`, `array('q')` and `ctypes.c_int64` round trips at the
    boundaries.
  - `hash(2**50-1) == hash(float(2**50-1))`; dicts and sets that mix `2**40`
    with `2.0**40`.
  - int51 constants in `co_consts` through `marshal`, and `.pyc` determinism
    (`test_py_compile`, `test_compileall`).
  - `copy.deepcopy` memo keys (`id()` results are now inline).
  - Unchanged behaviour: `sys.getsizeof(2**40)`, `(2**40).bit_length()`.

Test: step-0 set, the new tests, `test_int`, `test_long`, `test_float`,
`test_math`, `test_hash`, `test_marshal`, `test_pickle`, `test_struct`,
`test_array`, `test_ctypes`, `test_copy`, `test_gdb` where available. Risk:
high; it is the largest semantic change (section 5).

### Step 9b. D3 int51 in the trace JIT

- `Python/jit_x86.h` (38 int32 references) gains 64-bit add, sub and imul
  emitters. `jit_backend_x86.c` (11), `jit_record.c` (18), `jit_ir.c` (14),
  `jit_baseline.c` (10), `jit_runtime.c` (15), `pycore_jit_ir.h` (11) and
  `pycore_jit.h` (16) move the int LIR to scaled int64: read with `shl 13`,
  operate with one `jo`, write with `shr 13; or Py_INT_BASE`.
  `jit_record.c` lines 348 and 365 use `PyValue_FromInlineInt`.
- Regenerate `generated_jit_helpers.c.h`; re-enable int traces if 9a
  disabled them.

Test: the JIT test modules and the step-0 set with the JIT on; the step-9a
boundary tests under the JIT. Risk: medium, contained to the JIT.

## 3. The save/restore sites

### 3.1 Direct sites

`_PyValue_RestoreException(` with an argument:

| Snapshot | Calls | Files |
|---|---|---|
| 16:58 PST | 983 | 100 |
| 17:08 PST (P3 working-tree edits: Clinic regenerated, `OwnerScope` gone) | 820 | 53 |
| 17:25 PST | 820 | 53 |

The classifier (a scratch script; step 6a commits or attaches its successor)
pairs each call with the nearest preceding `X = PyErr_GetRaisedException()`
for the same variable, at most 60 lines back, and classifies the lines
between them. At 17:25, 816 calls were paired:

| Class | Meaning | Calls | Rule |
|---|---|---|---|
| A | Only `Py_DECREF`/`Py_XDECREF`/`Py_CLEAR` of `PyObject *`, optionally with a detach | 561 + 22 generated (43 + 2 files) | 6a |
| B | Only releases, at least one `PyValue_*` close, optionally with a detach | 105 (19 files) | 6a |
| C | A loop or array release (`_PyObjectArray_Free`, `_PyValueTemps_Close`, `_PyValue_CloseStack`) | 18 (7 files) | 7 |
| D | A release plus other statements (revalidation, `PyErr_Occurred` tests, iterator repair) | 24 + 1 generated | 7 |
| E | Nothing between save and restore | 7 (2 files) | 7 |
| F | No release in between (`free_keys_object`, `free_values`, `_PyDict_NotifyEvent`, `box_operands`, `ceval.c` revalidation) | 74 + 1 generated (20 + 1 files) | 7 |
| U | No matching save within 60 lines | 3 | 7 |

The generated sites are 21 in `Python/Python-ast.c` (from `Parser/asdl_c.py`,
21 matches), 2 in `generated_ir_cases.c.h` and 1 in
`generated_jit_helpers.c.h` (from `jit_helpers_generator.py`).

Largest files: `_operator.c` 107 (A 95), `itertoolsmodule.c` 104 (A 85,
B 15), `genericaliasobject.c` 89 (A 87), `typeobject.c` 74 (B 42, A 21),
`_datetimemodule.c` 63 (A 62), `ceval.c` 53 (F 22, B 10, A 9, C 7),
`_testinternalcapi/native_scope.c` 37, `Python-ast.c` 21, `codecs.c` 19,
`_abc.c` 17.

### 3.2 Mechanical rewrite rules (6a, 6b)

A Python script works per file:

1. Find each `S = PyErr_GetRaisedException();` whose restore,
   `_PyValue_RestoreException(S);`, is in the same brace block.
2. Accept only these lines between them: blank lines, comments, a detach
   (`T *r = x;` then `x = NULL;`, or `PyValue r = x;` then
   `x = PyValue_NULL;`), and release calls (`Py_DECREF`, `Py_XDECREF`,
   `Py_CLEAR`, `PyValue_CLOSE`, `PyValue_XCLOSE`, `PyValue_CLEAR`,
   `PyValue_Close`, `_PyValue_CLOSE_POINTER`, `_PyValue_XCLOSE_POINTER`). A
   detach that **immediately precedes** the save is treated the same way.
   Anything else: leave the site and list it for step 7.
3. Rewrite: delete the save and restore. A detach of `x` into `r` followed
   by `Py_XDECREF(r)` or `Py_DECREF(r)` becomes `Py_CLEAR(x)`; a `PyValue`
   detach followed by `PyValue_XCLOSE(r)` becomes `PyValue_CLEAR(x)`, and
   followed by `PyValue_CLOSE(r)` becomes `PyValue_CLOSE(x); x = PyValue_NULL;`.
   A bare `{ }` block that is left with one statement is unwrapped; an
   `if (…) { … }` keeps its braces (PEP 7).
4. If `S` was declared at the save, the declaration goes; if it was declared
   earlier and has no other use, that declaration goes too.
5. Helpers (6b): replace each call with the release in the helper's body,
   **derived from the body**, not from the name. `dict_close_value`'s body is
   `PyValue_XCLOSE(value)` (`dictobject.c:270`), so it maps to
   `PyValue_XCLOSE`, not `PyValue_CLOSE`. A helper that detaches through a
   pointer (`type_mutation_close(PyObject **slot)`,
   `call_close_object(PyObject **owner)`) maps to `Py_CLEAR(*slot)` or the
   value equivalent. Status helpers: `return set_close_key(k, s);` becomes
   `PyValue_CLOSE(k); return s;` only when the helper's only failure source
   was the release; otherwise list the call for step 7.

Before and after, detach before the save (`typeobject.c:3457` shape):

```c
{
    PyValue retired = found;
    found = PyValue_NULL;
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(retired);
    _PyValue_RestoreException(saved);
}
```
```c
PyValue_CLEAR(found);
```

Before and after, detach inside (`_operator.c`):

```c
if (item != NULL) {
    saved = PyErr_GetRaisedException();
    PyObject *retired = item;
    item = NULL;
    Py_XDECREF(retired);
    _PyValue_RestoreException(saved);
}
```
```c
if (item != NULL) {
    Py_CLEAR(item);
}
```

Before and after, helper (`dictobject.c`):

```c
dict_close_value(old_value);
```
```c
PyValue_XCLOSE(old_value);
```

Why this is safe: under the release contract, freeing a scalar box cannot
raise. For any other object, a deallocator must leave the current exception
unchanged (the gh-89373 check in `Objects/object.c:py_dealloc`), and
finalisers, weakref callbacks and dict watchers report their errors as
unraisable (`slot_tp_finalize` and `PyObject_ClearWeakRefs` save and restore
in this tree). That is the upstream contract under which upstream error paths
do `Py_DECREF(x); return NULL;`. It justifies dropping the exception save and
restore. It does not justify dropping revalidation, because these releases
can run Python (§3.4).

### 3.3 Release helpers (6b)

Found by body shape: functions of at most 20 body lines that contain a save
and a restore, whose remaining lines are releases, detaches and simple
control flow. Pure wrappers (18 helpers, 852 calls at 17:25, including
generated calls):

| Helper | Calls | Body release |
|---|---|---|
| `typeobject.c:type_mutation_close` | 349 | detach `*slot`, `Py_XDECREF` |
| `dictobject.c:dict_xdecref_object` | 110 | `Py_XDECREF` |
| `dictobject.c:dict_close_value` | 110 | `PyValue_XCLOSE` |
| `_testinternalcapi/dict.c:dict_fixture_close` | 108 | `Py_XDECREF` |
| `marshal.c:marshal_close_object` | 29 | `Py_XDECREF` |
| `ceval.c:drop_failed_value` | 19 | detach `*result`, `PyValue_XCLOSE` |
| `value_ops.c:close_object` | 18 | `Py_XDECREF` |
| `abstract.c:abstract_close` | 18 | detach `*slot`, `Py_XDECREF` |
| `_testclinic.c:owner_fixture_close` | 14 | `Py_XDECREF` |
| `ceval.c:call_transaction_close_object` | 12 | detach `*slot`, `Py_XDECREF` |
| `object.c:attribute_close` | 11 | detach `*slot`, `Py_XDECREF` |
| `call.c:call_close_object` | 11 | detach `*owner`, `Py_XDECREF` |
| `ceval.c:call_transaction_close_value` | 10 | detach `*slot`, `PyValue_XCLOSE` |
| `_testinternalcapi/native_scope.c:close_destination` | 9 | detach, `PyValue_XCLOSE` |
| `value_ops.c:close_value` | 8 | `PyValue_CLOSE` |
| `_testinternalcapi/dict.c:dict_public_close_value` | 7 | `PyValue_XCLOSE` |
| `ceval.c:check_value_bridge_result` | 5 | `Py_DECREF` if an exception is set (class P inside) |
| `codegen.c:codegen_close_value` | 4 | `PyValue_CLOSE` |

Helpers with a status or extra logic (about 250 calls): `value_operation_close3`
81, `set_close_object` 74 (calls `set_close_key`), `set_close_key` 33,
`collect_slot_error` 13, `_PyTypeMutation_End` 12,
`Python/native_scope.c:close_object`/`close_value` 12 (they accumulate a
saved exception), `call_pop_value` 6, `call_free_buffer` 5,
`clear_value_arguments` 4, `_PyObjectArray_Free` 3, `capture_cleanup_error`
3, `call_transaction_retire` 1.

The total is about 1,100 calls. Larger functions that contain a save and
restore (`PyValue_AsObject`, `value_operation_refresh`,
`value_reader_complete`, `_PyValue_Hash`, `_PyValue_RichCompareBool`,
`PyObject_ClearManagedDict`, `dictresize`, `set_table_resize` and others) are
not helpers; their sites are counted in §3.1.

### 3.4 Revalidation in `ceval.c`

Twelve `SystemError` checks ("changed the active frame", "changed executing
words", "changed an input home" and similar) in seven functions, plus the
instrumentation refresh in `value_operation_refresh`:

| Function | Callers | What precedes the check | Decision |
|---|---|---|---|
| `value_operation_refresh` (line 1604) | 8 in `ceval.c` | The bridge operation, `value_operation_close3` (operands), `drop_failed_value` (arbitrary results) and `_PyValueTemps_Close` | Keep. Operands and results are arbitrary objects; the operation runs Python |
| `value_operation_restore1`, `_restore2` | 3, 32 | A dict store or subscript that can run Python | Keep (operands read in place: register IR G2/G6) |
| `value_reader_complete` | 30 | A reader that can run Python | Keep |
| `value_attribute_complete`, method lookup (line 2094) | 3 | Attribute lookup | Keep |
| `call_transaction_refresh` | 24 | A native call | Keep |
| `native_call_finish` (lines 2210–2227) | 3 | A native call, then argument cleanup, whose owners may be last references after a callee rebinds a local | Keep |
| `_PyValueTemps_Close` before a refresh with nothing else in between (line 1679) | 1 | The operation ran before the close ("may have reentered before creating any private boxes") | Keep |

No check is preceded only by a scalar-box free, so step 5 deletes none. It
deletes the exception save and restore inside these functions (class F sites,
22 in `ceval.c`) and fixes the comments. A dispatch-time check of the
instrumentation version, as upstream does, could replace some of them; that
belongs to IR v2.

### 3.5 Class P (6c) and the remaining `PyErr_Occurred()` tests

`|| PyErr_Occurred()` terms at 17:25: **790 in 59 files**, by left term:

| Left term | Count |
|---|---|
| `X == NULL` | 375 (37 of them after `Py_NewRef`/`Py_XNewRef`) |
| `X < 0` | 156 |
| `X == -1` | 14 |
| `!X` | 42 |
| other (`_operator.c:2492` and similar) | 203 |

Rule 6: drop `|| PyErr_Occurred()` when the left term is the failure report
of a call in the previous statement (`X == NULL`, `X < 0`, `X == -1`, `!X`
after a predicate). After `Py_NewRef`/`Py_XNewRef`, delete the `X == NULL`
branch too, since it is dead. List the "other" forms for step 7.

Standalone `if (PyErr_Occurred())` lines: 892 in `Objects Python Modules`
against 286 upstream. `PyErr_Occurred()` per file against upstream
`3a440b65695`: `typeobject.c` 489/49, `Python-ast.c` 185/2, `dictobject.c`
148/11, `genericaliasobject.c` 128/1, `_operator.c` 98/0, `_datetimemodule.c`
94/15, `itertoolsmodule.c` 72/14, `ceval.c` 50/14, `call.c` 28/1, `_abc.c`
27/2. Over files present in both trees: 4,614 against 3,012. This count is
recorded after step 7 as the measure of what remains.

## 4. Measurements (not gates)

Record on the release build (`--with-mimalloc --with-computed-gotos`, -O3)
and upstream `3a440b65695`, with the `pyjit/bench/run.py` contract (`time`,
`instr` when perf access allows, `rss`, `boxes` with pystats). Records: before
the packet (step 0), after step 4, after step 7, after step 8, and after
step 9b.

Kernels (`pyjit/bench/internal/kernels/`): all 29 as the aggregate. Watch:

| Kernel | Why |
|---|---|
| `deepcopy`, `json_dumps`, `pickle` | `id()` memo keys move from boxed to inline (D3); release-wrapper removal in dict paths (D2) |
| `nbody`, `float`, `spectral_norm`, `raytrace`, `chaos` | Float guards inline-only (D2) |
| `richards`, `deltablue`, `nqueens`, `fannkuch`, `go` | Int guards and scaled int arithmetic (D3); `go`'s 52–62-bit Zobrist hashes stay boxed |
| `generators`, `coroutines` | Frame NULL fills become `memset` (D5) |
| `unpack_sequence`, `comprehensions` | Tuple and list construction (D5 fills, D2 constructors) |

Memory workloads (`internal/memory/`): `tuples_10m`, `instances_1m`,
`dicts_200k` (D5 deletes `init_value_slots` and fill loops).

Existing micros (`internal/micro/`): `store_fast_two`, `int_add`,
`int_add_small`, `int_chain`, `int_compare`, `float_add`, `float_chain`,
`float_compare`, `dict_subscript`, `list_subscript`, `tuple_subscript`,
`call_len`, `call_abs`, `call_math_sin`, `new_instance`,
`new_instance_empty`, `empty_loop_large_i`.

New micros, same format as `int_add.py`:

| Name | Body | Measures | Steps it shows |
|---|---|---|---|
| `is_op_false` | `z = a is b`, distinct objects | `IS_OP` false path | 8 |
| `is_op_int` | `z = x is y`, equal computed ints | inline identity | 8 |
| `list_append_int` | `lst.append(1000)`, cleared each outer loop | box at the C boundary | 5–7 |
| `dict_miss_chain` | absent keys that collide | `PyValue_Is` in `compare_generic` | 8 |
| `hash_tuple` | `hash(t)`, 4-tuple of ints and strs | `_PyValue_Hash` without save/restore | 5–7 |
| `int_add_wide` | `z = x + y`, `x = 2**40` | int51 inline vs boxed | 9a |
| `id_memo` | `d[id(o)] = o` over a fixed list | `id()` results inline | 9a |
| `close_xclose` | rebind a local between unbound and an object (`del x; x = o`) | `XCLOSE` with the NULL test | 4 |
| `build_tuple_small` | `t = (a, b)` with object locals | canonical `Steal` on a fresh tuple (the 16 former `StealMortal` sites) | 8 |

Counters (pystats): `inline_ints_created`, `inline_int_boxes`,
`inline_float_boxes`; a counter of `_PyValue_IsSameValue` true results, added
in step 3 and read before step 8 deletes the function; the step-3
home-checker violations per test module.

Code size: `ceval.o` text and `_PyEval_EvalFrameDefault` frame size
(`-fstack-usage`) before step 5, after step 7 and after step 8.

Source: the `PyErr_Occurred()` per-file table of §3.5 after step 7.

## 5. What could go wrong

| Risk | Step | How it is detected |
|---|---|---|
| Code gives zero a meaning other than NULL | 4 | Known sites: `pystate.c:702`, `typecache.c:16`, `_testcapi/list.c:164,166`, `CheckValid`, `_PyValue_BorrowBitAt`, `IR_BORROW_KNOWN`, `_PyValue_XCLOSE_POINTER`. A missed site shows as `CheckValid` rejecting `1`, or `Py_DECREF(NULL)` |
| A path relied on `CLOSE`, `CLOSE_POINTER` or `DUP` ignoring NULL through the mask | 4 | Debug asserts in those operations; the full debug suite after step 4 |
| Static initialisers encode PyValue bits (`EMPTY_ENTRY`; Clinic `_kwtuple`) | 3, 4 | Step 3's static-immortal check; grep initialisers for `.bits =` and `PyValue_NULL_BITS` |
| Persisted encodings | 4, 9a | None found: `marshal.c` writes objects, not bits; the JIT persists no code |
| Extensions built against the old `pyvalue.h` inline the old encoding | 4, 9a | Rebuild every shared module; remove the hand-made symlinks in `build/lib*` (bring-up 16:40) before the step-4 and step-9a builds |
| Out-of-process tools | 4, 9a | `Tools/gdb/libpython.py` (`test_gdb`); `_remote_debugging` includes `pycore_value.h` and masks `Py_TAG_BITS` (`object_reading.c:37`, `_remote_debugging.h:128`), and uses `PyValue_IsObject` (`asyncio.c:remote_value_object_addr`), so a rebuild follows (`test_external_inspection`). py-spy, austin and memray are unsupported; the debug-offset constants let them decode |
| A deallocator, hook or reftracer that raises or runs Python | 5–7 | gh-89373 check; the step-5 checks in the free and allocation entry points and the reftracer; `test_weakref`, `test_gc`, `test_finalization`, `test_capi` memory-hook tests |
| Python runs where a revalidation was removed | 5, 7 | None removed in `ceval.c` (§3.4); elsewhere each removal is a step-7 judgment against upstream practice; the step-5 monitoring and settrace tests |
| A helper mapped to the wrong release | 6b | Mapping derived from bodies; `dict_close_value` maps to `XCLOSE` |
| A constructor result used as an object after it became inline | 8 | `POINTER_CONSTRUCTORS` change, the 224-site audit, the `bit_length`/`hex`/`__enter__` tests |
| A home store that bypasses canonical construction | 8 | `_PyValue_CheckHome` assert in setters, `IR_STORE_*` and GC traversal |
| `x is y` and `id()` change for ints between 2^31 and 2^50 | 9a | Expected (pyvalue.md §1.13); failing `is not` tests on equal large ints are triaged against that list |
| 32-bit truncation of an int51 value in C | 9a | The renames make every caller fail to compile until read; `-Wconversion` on the touched files once |
| `len` or `range` results beyond int51 encoded without a check | 9a | Range-checked `PyValue_FromInt64`; `len(range(2**55))` test |
| Hash or comparison disagreement between inline and boxed forms | 9a | I8 tests; `test_hash`, `test_dict`, `test_set` |
| JIT tests fail between 9a and 9b | 9a, 9b | Step-0 survey; one validation point for both |
| Overlap with other packets (P3, bring-up fixes, IR v2) | all | Step-0 precondition; recounts before each step; IR v2 starts after step 10 |

## 6. Estimate

| Step | Agent-hours | Model |
|---|---|---|
| 0 | 1.5 | Sonnet |
| 1 | 1.5 | Sonnet |
| 2 | 1 | Sonnet |
| 3 | 2 | Opus |
| 4 | 3.5 | Opus |
| 5 | 5 | Opus |
| 6a | 2.5 (script 1, batches 1, review 0.5) | Sonnet script; Haiku batches |
| 6b | 2.5 | Sonnet script; Haiku batches |
| 6c | 2.5 | Sonnet script; Haiku batches |
| 7 | 5 | Opus |
| 8 | 4 | Opus |
| 9a | 7 | Opus |
| 9b | 3 | Opus |
| 10 | 1 | Sonnet |
| **Total** | **about 42** | |

Each step is one commit (6a–6c: one commit per batch group), reviewed with a
release-mode compile, and keeps the step-0 pass set.
