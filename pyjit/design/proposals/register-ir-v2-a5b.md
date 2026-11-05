# Register IR v2 step A5b: the typed borrow (proposal)

Status: revision 3, 2026-11-04. Revision 2 answered
[review 1](../reviews/register-ir-v2-a5b-review-1.md); review 2, in the same
file, accepted it with changes, and revision 3 folds in the coordinator's
rulings on them.
This is phase 1 of step A5b of the
[register IR v2 packet](register-ir-v2-packet.md). It finishes
[pyvalue.md](../pyvalue.md) D4: a borrow is a distinct C type that lives
only in C locals, and no home (frame slot or heap field) holds one.

All counts are at runtime HEAD `5de7bd62ef9`
(`build/worktrees/pyvalue-rc-universal`). They come from
`build/pyjit-link-wave1/a5b/tally.py`, which reads the git blobs, strips
comments and counts call tokens. Where a count comes from `rg` or from the
review instead, the text says so. "Hand" means hand-written C and headers;
"ir" and "jit" mean `Python/generated_ir_cases.c.h` and
`Python/generated_jit_helpers.c.h`.

## Changes after review 1

| Finding | Decision | Handled in |
|---|---|---|
| M1. Clang only warns on pointer-typed misuse | Accepted. Core units build with `-Werror=incompatible-pointer-types` on both compilers, plus `-Werror=incompatible-function-pointer-types` on Clang before 16. The negative units run on GCC and `clang-21`, and each must give its expected diagnostic. `raw-bits` also covers casts to `PyValue *` and `PyValueBorrow *` | §3.6, §5.6, Appendix A |
| M2. `PyValueRead` is a third type with holes | Accepted, in the reviewer's form. Readers take `PyValueBorrow` behind a same-name macro over `PyValue_Borrow`, which is a `_Generic` in C and two overloads in C++. `PyValueRead` and `transparent_union` are gone. All eight misuses fail on both compilers, and the generated code is the same | §3.1, Appendix A |
| M3. L1 conflicts with A3a's record | Accepted, with the coordinator's rule. A C-stack record that lives for one handler activation is a C local and may hold a borrow. A3a's activation record outlives handlers, so it is a home and holds owned values only. A3a removes the scope's borrow field before it moves `_PyNativeScope` into the scratch. A5b lands first, and A3a rebases on its `TAKE`/`OWNER` lines | §2.2, §8.2, §11 |
| M4. The guarantee is stated too strongly | Accepted. §2.3 states what the compiler enforces and what it does not. A5b-3 takes a census of the 71–80 `PyValue x = <home>` copies and retypes them; a `home-copy` lint reports, without gating. The full fix is a field type, `PyValueField`, in value-api V4 and V5 | §0, §2.3, §5.3, §11 |
| S1. The owner reloads the slot | Accepted. The owner is adopted from the word the view already loaded, through a private `_PyIR_AdoptTemp`, not a `PyValue` word local. (Revision 2's proven-inline rule is dropped by R2-S3.) | §4.2 |
| S2. `TAKE` merges two meanings | Accepted. A new DSL form, `OWNER(x)`, covers the 13 receiver sites. `TAKE` stays for values that go to a home | §4.2, §5.2 |
| S3. The fail-closed rule cannot work as written | Accepted. Casts and field-address arithmetic count as views. "Known reader" becomes a checked list. The `borrow_mask` change is measured before landing; the expected figure is given | §4.7 |
| S4. The type cache needs its own type | Accepted. A private `_PyValueGuarded`, a weak reference guarded by the type's version. L1 has no heap exception | §2.4, §5.1 |
| S5. Gaps in the producer list and the tables | Accepted. Added `_PyValue_FromObjectBorrowInline` (merged), the `PyDict_WatchCallback` parameters, the counts that move to callers, the reclassification of `native_scope.c:55`, the stale bit-0 test in `_PyValue_Publish`, the deletion of `AsStrongReference`, and the end of the release check in `native_scope.c`. `_PyObject_ValueIdentity` is removed from the readers | §2.4, §3.2, §5.2, §5.4 |
| S6. H1 fails during finalization | Accepted. The H1 asserts stop once the interpreter begins finalizing; the four paths are named | §5.5 |
| S7. Nothing checks how long a borrow lives | The generator refuses a view of a local that is read after an escaping call. This replaces a debug ledger. Cost: about 1 h, and nothing at run time | §4.8 |
| S8. Generator details not covered | Accepted. Each C variable has one type, so the generator splits variables by role. Pass-through outputs become `TAKE`. Live ranges are measured under A3a's convention | §4.3, §4.6 |
| S9. The estimate is not credible by step | Accepted. Re-estimated per step: 21 h | §8 |
| N1–N4, N6 | Agreed: the mask stays (Q1); the eight close forms wait for a later step (Q7); rule 3's suffix (V4) and `PyValue_Wrap` (G6) are coordinator edits; the C++ declarations are pinned by `test_cext` | §6, §10.2, §11, §3.4 |
| Item 6, (c) and (e) | `PyValue_BorrowAt` is deleted. `PyValue_Borrow` is the one owner-to-view conversion | §3.1, §7 |
| §9 (four corrections) | Applied: upstream frames hold borrows; HPy's lesson is the field type; V8 relies on scopes and a stack-only checker; a Rust row is added | §9 |
| Q1–Q6 | Q1: keep the mask. Q2: replaced by S4. Q3–Q6 accepted as recommended | §10.2 |

## Changes after review 2

| Item | Ruling and change | Handled in |
|---|---|---|
| R2-M1 | `-Werror=incompatible-pointer-types` goes into `CFLAGS_NODIST` in `configure.ac`, beside `:2989`, so core, built-in and shared module units all see it. If the regen container does not run, it goes into `PY_CFLAGS_NODIST` in `Makefile.pre.in`. Third-party extensions built with Clang get warnings only | §2.3, §3.6 |
| R2-M2 | A5b-1 renames the tagging form to a private `_PyValue_BorrowTagged` by script, at its 52 hand-written sites and in `IR_KEEP_BOX`. A5b-2 removes it from generated code, and A5b-3 deletes it. A5b-1 builds and passes on its own | §8.1 |
| R2-S1 | Done in A5b-3, not A3a. The scope drops `argument`. `Begin` puts the borrowed object, or a box, into `objects[0]` at once, and an owned `PyObject *argument_box` (NULL unless it boxed) records the box. No struct field then has type `PyValueBorrow`, so `borrow-field` rejects all of them unless the census finds another (§5.6). +0.5 h | §2.2, §2.4, §5.2, §8 |
| R2-S2 | A3a's record rule governs `PyValue` words. The scope's object pointers are views that `End` releases within the same activation. §8.2 is now what A3a inherits | §8.2 |
| R2-S3 | The proven-inline rule is dropped: the release stays. Recorded as a follow-up with its mechanism | §4.2, §10.2 Q8 |
| R2-S4 | §8.2's call sites corrected. `native_scope_box_oom` installs its failing allocator before `Begin`, which A5b-3 now fixes | §8.1, §8.2 |
| R26 (coordinator, after acceptance) | value-api.md §1.6 adds the representation boundary. In A5b-3, `raw-bits` widens to every `.bits` access and every encoding constant outside §1.6's list. A new `public-value` rule rejects inline decoding in public headers and any word field beyond the five frozen ones. A5b-3 also converts the 21 sites §1.6 lists, through an exact-word equality accessor, a word-as-`uintptr_t` accessor for hashing and debug output, and a named set-dummy sentinel. +1 h | §5.6, §8.1 |
| Notes | Adopted: the consume comes at the first `TAKE` or `OWNER`; the H1 carve-out starts on entry to `finalize_interp_clear`, not at `SetFinalizing`; each reader macro follows its declaration, and out-of-line definitions write `(Name)`; the C++ `BAD` units also fail; Clang's one warning on the probe's good unit is the probe's own unused static | §4.2, §5.5, §3.1 |

## 0. Decisions

| # | Item | Decision |
|---|---|---|
| 1 | Type | `PyValueBorrow`, a second one-word struct that holds its owner's exact bits. A borrow carries no tag of its own; bit 0 means immortal in every word. Borrows live in C locals, including records on the C stack that live for one handler activation, and nowhere else. |
| 2 | API | Readers take `PyValueBorrow`. A same-name macro converts an owner at the call through `PyValue_Borrow` (a `_Generic` in C, two overloads in C++), so no call site changes. Stores, closes, steals and stealing setters take `PyValue` only. `DUP`/`XDUP` (borrow to owner) is the one conversion that takes a count. |
| 3 | Generated code | For each borrowable field, a view `x` (`PyValueBorrow`) and an owner `ir_own_F` (`PyValue`, NULL for a local) adopted from the loaded word. A release is `XCLOSE(ir_own_F)`. A value that goes to a home uses `TAKE`; a value held only for a later retire uses `OWNER`. The conditional releases, `IR_INPLACE_*` and `IR_KEEP_BOX` go. The generator refuses a view of a local that is read after an escaping call. |
| 4 | Heap side | Every borrow producer returns `PyValueBorrow`. The 150 `MakeHeapSafe` uses become 89 `DUP`s, 27 `TAKE`s, 13 `OWNER`s, 1 retyped field and 20 deletions. `MakeHeapSafe`, `IsHeapSafe`, `OwnArray`, `BorrowAt` and `AsStrongReference` go. One home check includes H1 and stops at finalization. The type cache gets a private guarded type. |
| 5 | `PyValue_Is` | Keep the bit-0 mask. Record "tag only static immortals" in G6 as the target form. |
| 6 | Merges and renames | `NewRef` → `XDUP`, `Close` → `XCLOSE`, `FromPyObjectSteal` → `FromObjectSteal`. `FromPyObjectBorrow` and `_PyValue_FromObjectBorrowInline` merge into an inline `FromObjectBorrow`. `FromPyObjectNew(o)` → `DUP(FromObjectBorrow(o))`. One token script does them: 191 hand-written sites and 8 generator strings, plus 104 generated sites by regeneration. |
| 7 | Steps | A5b-1, types and readers (3 h); A5b-2, the generator (8 h); A5b-3, the heap side (7.5 h); A5b-4, merges and renames (1 h); suites on both compilers and review (2 h). Total 21.5 h. Every step runs the negative units on both compilers. |

## 1. Where the plan and the code disagree

The code wins in every row.

| Plan says | Code at HEAD | Effect |
|---|---|---|
| `MakeHeapSafe`: 155 uses in 23 files (packet A5b) | **150 in 24 files**: `dictobject.c` 46, `bytecodes.c` 40, `ceval.c` 14, `tupleobject.c` 7, and 43 in 20 others; 6 more in generated code | §5.2 |
| `_Py_VISIT_VALUE` still to go into `traverse_slots` | Done (`typeobject.c:2808–2809`), under `assert(PyValue_IsHeapSafe(*slot))` | Only the assert changes |
| The typed borrow will catch an object pointer that leaves an in-place read (A5a notes §12, register-ir.md:151, pyvalue.md:223) | The type covers `PyValue` words only; `AsKnownObjectBorrow` returns a plain `PyObject *` | §4.7 |
| Constructor rename counts 155, 13, 53 (value-api.md §1.5) | `FromPyObjectSteal` 95 + 66 generated; `FromPyObjectBorrow` 9 + 2; `FromPyObjectNew` 35 + 18 | §7 |
| The A5a notes call the `ceval.c` pairs `MakeHeapSafe(Borrow(homes[i]))` identities | Each pair takes a count, so it is a `DUP` | §5.2 |
| Packet A3a: the record "holds owned values only … Under D4 it is a home", and its scratch holds `_PyNativeScope` | The scope's `argument` is a borrow of the window's value (`native_scope.c:54–55`) | §8.2, §11 |
| Revision 1: "no C++ file includes the internal headers" | `test_cext`'s `TestInternalCpp` compiles `pycore_frame.h` and `pycore_interpframe_structs.h` as C++ with `-Werror` | §3.4 |

## 2. The type (item 1)

### 2.1 Name

| Candidate | Verdict |
|---|---|
| **`PyValueBorrow`** | **Chosen.** It keeps the `PyValue` prefix. It does not collide with native-boundary.md §2's `PyBorrowScope` and `_PyBorrow_*`, which name scopes and the borrow stack, not values. |
| `PyValueRef` | Rejected: "Ref" means a strong reference in CPython (`Py_NewRef`, `GetItemRef`). |
| `PyValueLocal` | Rejected: "local" already means a frame local (`IR_IS_LOCAL`). |
| `PyValueView` | Rejected: "borrow" is the word the design documents use. |

### 2.2 Representation and invariants

```c
/* Include/pytypedefs.h */
typedef struct { uintptr_t bits; } PyValue;        /* an owner: a home or an owning C local */
typedef struct { uintptr_t bits; } PyValueBorrow;  /* a borrow: C locals only (L1) */
```

| Invariant | Statement | Enforced by |
|---|---|---|
| B1 | A `PyValueBorrow` holds the exact bits of an owner, or an inline value. A borrow never carries a tag of its own. | The producers (§2.4) |
| H1 | In every word of either type, "tagged" implies "immortal". This holds until the interpreter begins finalizing. | Debug asserts (§5.5) |
| H2 | A static immortal is always tagged. `_Py_INCREF_MORTAL` already asserts `!_Py_IsStaticImmortal`. | `PyValue_CheckValid` |
| L1 | A `PyValueBorrow` lives only in a C local. A field of a record on the C stack that lives for one handler activation counts as a C local: a struct declared in a handler body, or in a C function a handler calls, Heap fields, heap arrays, globals, statics and A3a's activation record are homes, and they hold owned values only. After A5b-3 no struct field has type `PyValueBorrow` (R2-S1). | The `borrow-field` lint, with no exceptions (§5.6) |

After A5b, `PyValue_Borrow(v)` is a free type change.

### 2.3 The guarantee (M4)

With `-Werror=incompatible-pointer-types` on both compilers (§3.6). Third-party
extensions do not get the flag: GCC 14 and later reject the pointer cases
anyway, and Clang only warns.

| The compiler enforces | The compiler does not enforce |
|---|---|
| A `PyValueBorrow` never becomes a `PyValue`. It is rejected in an assignment, an initialiser, an argument and a return, and as a `PyValueBorrow *` used as a `PyValue *` (so `CLEAR`, `IR_STORE_DST` and `_Py_dict_lookup` misuse fail too). A closer cannot become a reader pointer. | **That a `PyValue` owns a count.** `PyValue x = *home;` copies an owner's bits into a second owner-typed local. `rg` finds 71 such declarations in 20 files; the review's pattern finds 80 in 22. A5b-3 retypes the non-moves (§5.3). |
| So a value from any borrow producer (§2.4) reaches a store, close, steal or stealing setter only through `DUP`/`XDUP`, or through the generator's `TAKE`. | **How long a borrow lives.** A borrow can outlive its owner. The generator rule in §4.8 covers views of locals in handlers. Runtime C relies on review, as upstream does. |
| | **Casts, unions and `memcpy`.** `raw-bits` lints casts to `PyValue *` and `PyValueBorrow *`. |
| | **Derived `PyObject *` borrows** (§4.7). |

The full fix for heap homes is a field type: HPy's `HPyField`, here
`PyValueField`. Its load returns a `PyValueBorrow`, and its store takes a
`PyValue`. That closes the copy-of-home gap. It does not belong in A5b,
because it retypes every heap field access. value-api V4 (container item
arrays) and V5 (the object fields that become value homes) rewrite those
accesses anyway, so it belongs there (§11).

### 2.4 Where borrows come from

| Producer | At HEAD | After A5b | Uses (hand / ir / jit) |
|---|---|---|---|
| `PyValue_Borrow(v)` | `PyValue`, sets bit 0 on an object | the `_Generic` view: accepts either type, returns `PyValueBorrow` with exact bits | 52 / 0 / 0 |
| `PyValue_BorrowAt(p)` | raw bits | deleted; `PyValue_Borrow(*p)` | 2 / 0 / 0 |
| In-place read in a handler | `IR_LOAD_BORROWED(F)` = `IR_SLOT(F)` | `IR_VIEW(F)` (§4.2) | 1 / 156 / 74 |
| A box for an object input | `IR_KEEP_BOX`: `(REF) = PyValue_Borrow(REF)` | `PyObject *x_o`, owned by `value_temps` (§4.5) | 1 / 95 / 5 |
| `PyValue_FromObjectBorrow(o)` | out of line; bit 0 on every pointer | inline in the core, exported for modules; canonical; tagged if immortal | 140 / 1 / 0 |
| `_PyValue_FromObjectBorrowInline(o)` (`pycore_value_api.h:90`) | inline duplicate | merged into the above | 3 hits in 2 files |
| `PyValue_FromPyObjectBorrow(o)` | raw | merged into the above | 9 / 2 / 0 |
| `_PyValue_FromObjectKnown(o)` | raw tagged bits | `PyValueBorrow`, tagged if immortal | 5 / 0 / 0 |
| `PyTuple_`, `PyList_`, `PyStructSequence_GET_VALUE` | tagged borrow | `PyValueBorrow`, exact bits | 131 / 8 / 4; 41 / 2 / 0; 5 / 0 / 0 |
| `dict_entry_key`, `_PyDict_NextBorrowed`, `_PySet_NextBorrowed` | tagged borrow | `PyValueBorrow` | 5, 6, 3 |
| `_Py_dict_lookup(…, PyValue *value_addr)` | raw `me_value` bits | `PyValueBorrow *value_addr` | 21 in 3 files |
| `PyDict_WatchCallback` parameters (`dictobject.h:93`, "borrow strong snapshots") | `PyValue key, PyValue new_value` | `PyValueBorrow` | 1 type |
| `_PyNativeScope.argument` (`native_scope.c:54–55`) | owned or tagged borrow, chosen by a run-time test | deleted (R2-S1). `Begin` sets `objects[0]` to the borrowed object or a box, and `PyObject *argument_box` owns the box; `End` clears it | 1 field |
| Type cache (`typecache.c:116`, `pycore_typecache.h:19`) | `PyValue_Borrow(value)` in a `PyValue` field | `_PyValueGuarded`, private to `pycore_typecache.h` (S4). It is built from a borrow at insert and read back as a `PyValueBorrow` only after the version check (`typecache.c:209–210`) | 1 field |

`FromObjectBorrow` and `FromObjectKnown` gain a refcount load. It reads the
same header line as the type load that `FromObjectBorrow` already does. B1
needs it, since an untagged `None` would break H2 when it is `DUP`ed.

## 3. The API table (item 2)

### 3.1 Reader form (M2)

```c
#ifdef __cplusplus
static inline PyValueBorrow PyValue_Borrow(PyValue v) { PyValueBorrow b = {v.bits}; return b; }
static inline PyValueBorrow PyValue_Borrow(PyValueBorrow v) { return v; }
#else
static inline PyValueBorrow _PyValue_BorrowOwned(PyValue v) { return (PyValueBorrow){v.bits}; }
static inline PyValueBorrow _PyValue_BorrowBorrowed(PyValueBorrow v) { return v; }
#  define PyValue_Borrow(V) _Generic((V), PyValue: _PyValue_BorrowOwned, \
                                          PyValueBorrow: _PyValue_BorrowBorrowed)(V)
#endif

static inline int PyValue_IsInlineInt(PyValueBorrow v) { return v.bits >= Py_INT_BASE; }
#define PyValue_IsInlineInt(V) (PyValue_IsInlineInt)(PyValue_Borrow(V))
```

- The reader itself takes only `PyValueBorrow`. One macro line per reader
  that owners reach passes an owner, a borrow, `*p` or `slot[i]` through
  unchanged. `_Generic` drops qualifiers, so `const` lvalues work.
- `&PyValue_IsInlineInt` is the borrow-only function. A closer cannot be
  assigned to a reader pointer.
- `PyValue_Borrow` is the one owner-to-view conversion. It is free, and it
  replaces the union's member access.
- Appendix A has the probe, rerun for this revision on GCC 15.2 and Clang
  21.1. Eight misuses fail on both compilers, and `-O2` gives the same
  instructions as a plain `PyValue` reader.
- The same header compiles as C++11 under `g++` and `clang++-21`.
- About 100 header readers get the macro line, by a list-driven script.
  Static readers in `.c` files either take `PyValue_Borrow(x)` at their call
  sites or get the same line, whichever touches fewer lines.

### 3.2 Functions that take `PyValueBorrow`

| Family | Functions (hand / ir / jit uses) |
|---|---|
| Type tests | `IsNull` 357/283/92, `IsObject` 188/14/0, `IsInline` 56/138/9, `IsInlineInt` 76/45/5, `IsInlineFloat` 36/19/5, `IsPointerSpace` 17/43/0, `IsTaggedIndex` 12/3/0, `IsLanguageValue` 67, `IsNone` 16/13/2, `IsTrue` 4/9/2, `IsFalse` 4/9/2, `BothInlineInt` 4, `IsValid` 2, `IsMalformed` 3, `RefcountOnObject` 14, `_MayDealloc` 2/14/0, the `*Check`/`*CheckExact` family (`BoolCheck` 6/21/4, `LongCheckExact` 17/33/0, `FunctionCheck` 5/2/1, 7 others with 1–3 uses), `CheckValid` 5 |
| Accessors | `TYPE` 72/7/0, `InlineInt64` 74/35/4, `InlineFloat` 18/8/4, `_IntScaled` 12/7/0, `UntagIndex` 12/7/0, `NumberAsDouble` 4, `FloatOperandValues` 5/4/0 |
| Object view | `AsKnownObjectBorrow` 554/281/11, `_PyValueTemps_Object` 26/13/2 |
| Identity, truth, hash, compare | `PyValue_Is` 29/1/0, `_PyValue_Truth` 6/5/1, `_PyValue_Hash` 18, `_PyValue_RichCompareBool` 26 |
| Conversions with their own count | `PyValue_AsObject` 41/1/0, `PyValue_AsPyObjectNew` 8/1/0, `PyValue_DUP` 90/12/10, `PyValue_XDUP` (new) |
| Borrowing setters | `PyDict_SetValue`, `PySet_AddValue`, `PyCell_SetValue` 3, `PyCell_NewValue`, `_PyMember_SetValue` 3, the lookup keys |
| Callbacks | `PyDict_WatchCallback`, `check_lookup` (`dictobject.c:1040`), `compare_func` (`setobject.c:91`), `key_compare`, `tuple_elem_compare`: reader parameters typed `PyValueBorrow` |
| Value-form operations | every `_PyValue_<Op>` operand that is read and not consumed |

`PyValue_Is`, `IsNone`, `IsTrue` and `IsFalse` change from macros to inline
readers. `_PyObject_ValueIdentity` takes a `PyObject *`
(`pycore_value.h:377`) and is not a reader.

### 3.3 Functions that take `PyValue` only

| Family | Functions (hand / ir / jit uses) |
|---|---|
| Closes | `CLOSE` 211/50/9, `XCLOSE` 243/37/7, `CLOSE_SPECIALIZED` 6/40/0, the `_POINTER` variants 3/119/4, `CLEAR` 89, `XSETREF` 2, `_PyValue_CloseStack` 2/5/0 |
| Steals | `AsKnownObjectSteal` 24/2/0, `_AsPyObjectStealPointer` 1/6/1 |
| Stealing setters | `PyTuple_SET_VALUE` 38, `PyTuple_SetValue` 5, `PyList_SET_VALUE`, `PyStructSequence_SET_VALUE`, `_PyTuple_FromValueStealOnSuccess` 5/1/1, `_PyList_FromValueStealOnSuccess` 3/1/1, `_PyList_AppendTakeValue` 7/2/0, `_PySet_AddValueTakeRef` 6, `_PyCell_SwapValue` 3/1/0, `_PyDict_SetValue_Take2` 11/1/0, `_PyFrame_SetReturnValue` 6/3/0 |
| Frame and IR macros | `IR_STORE_DST` 108/45, `IR_STORE_TEMP_DST` 35/6, `IR_STORE_TEMP` 32/7, `IR_PUBLISH_TEMP` 127/10, `IR_MOVE_TO_DST` 35/4, `IR_STORE_RETURN` 5/1, `IR_RETIRE_LOCAL`, `_PyValue_Publish` 16 |
| Home lvalues | every `PyValue` field, slot and array element |

### 3.4 Public declarations and C++

- `Include/cpython/` (`tupleobject.h`, `listobject.h`, `dictobject.h`,
  `setobject.h`, `structseq.h`) already declares value APIs.
- After A5b, the `GET_VALUE` accessors return `PyValueBorrow`, and borrowed
  parameters are `PyValueBorrow`. Each of those has the same-name macro.
  Stealing setters keep `PyValue`.
- `PyValueBorrow` goes into `pytypedefs.h`. It is a plain struct, so it is
  valid C++. `test_cext`'s `TestInternalCpp` compiles the internal frame
  headers as C++ with `-Werror` and pins this (N6). The only C++-specific
  code is the two `PyValue_Borrow` overloads.

### 3.5 Signatures

A scan finds about 100 header functions and 136 `.c` functions with a
`PyValue` parameter. A parameter that a function only reads becomes
`PyValueBorrow`; one that it stores, closes or steals stays `PyValue`. The
compiler reports each one in steps 2 and 3. The ABI does not change.

### 3.6 Compiler flags (M1)

| Flag | Where | Why |
|---|---|---|
| `-Werror=incompatible-pointer-types` | `CFLAGS_NODIST`, set in `configure.ac` beside `:2989`, so core, built-in and shared module units all get it (R2-M1); fallback `PY_CFLAGS_NODIST` in `Makefile.pre.in` | Without it, Clang only warns on `PyValue *p = &b`, `CLEAR(b)`, `IR_STORE_DST(F, b)`, `scope->arguments = &scope->argument` and `_Py_dict_lookup` misuse (review M1). GCC 14+ already errors. |
| `-Werror=incompatible-function-pointer-types` | Clang before 16 only | Clang 16+ errors by default |

A5b-3 builds and runs its tests on both compilers, as A3a's qualification
does.

## 4. The generated code (item 3)

### 4.1 At HEAD

After A5a, the handler variable `x` for an input read through a field that
names a local is a `PyValue`. It is owned for a temporary and borrowed for a
local, and only `IR_IS_LOCAL(F)` tells which, at run time.

| Form | ir / jit sites | What it does |
|---|---:|---|
| `x = IR_LOAD_BORROWED(F)`, `x = IR_SLOT(F)` | 156 / 74, 257 / 40 | reads the slot |
| `IR_CONSUME_INPUT(F)` | 252 / 80 | NULLs a temporary's slot |
| `if (!IR_IS_LOCAL(F)) CLOSE(x)` | 131 / 13 | releases a temporary only |
| `IR_INPLACE_HEAP_SAFE`, `IR_INPLACE_OWN`, `IR_INPLACE_OBJECT_STEAL` | 92 / 64, 11 / 0, 3 / 0 | take a count for a local |
| `IR_KEEP_BOX(x)` | 95 / 5 | turns `x` into a tagged borrow of a box |

The 255 instructions with a format split into 175 register formats and 80
window formats. They have 204 scalar inputs: 177 borrowable and 27 owned.

### 4.2 After A5b: view, owner, `TAKE`, `OWNER`

| Variable | Type | Value | For |
|---|---|---|---|
| `x` (the DSL name) | `PyValueBorrow` | `IR_VIEW(F)`, the one load of the slot | every borrowable input |
| `ir_own_F` | `PyValue` | `PyValue_NULL` for a local, else adopted from `x` | every borrowable field that is released, taken or retired |

```c
#define IR_VIEW(F)             ((PyValueBorrow){IR_SLOT(F).bits})
#define IR_CONSUME_INPUT(F, V) (IR_IS_LOCAL(F) ? PyValue_NULL \
                                : (IR_SLOT(F) = PyValue_NULL, _PyIR_AdoptTemp(V)))
#define IR_TAKE(V, OWN)        (PyValue_IsNull(OWN) ? PyValue_XDUP(V) : (OWN))
```

- **Adoption (S1).** `_PyIR_AdoptTemp(V)` is `(PyValue){(V).bits}`. It is
  private to `ceval_macros.h` and is the one count-free conversion from a
  borrow to an owner. It is valid because the word came from a temporary's
  slot, and the same expression clears that slot: the count moves from the
  slot to the C local. It reuses the loaded word and does not reload the
  slot, so after another input's NULL store the compiler still knows the
  value's facts.
- **No `PyValue` local holds a borrow.** The review's `ir_word_F` would have
  been a `PyValue` that holds a borrow for a local. This form avoids that,
  with the same instructions. `raw-bits` allows `_PyIR_AdoptTemp` only in
  `ceval_macros.h`.

| Today | After A5b |
|---|---|
| `x = IR_LOAD_BORROWED(F)` | `PyValueBorrow x = IR_VIEW(F)` |
| `IR_CONSUME_INPUT(F);` | `PyValue ir_own_F = IR_CONSUME_INPUT(F, x);`, at the DSL consume point or the first `TAKE` or `OWNER`, whichever comes first |
| `if (!IR_IS_LOCAL(F)) PyValue_CLOSE(x)` and its variants | `PyValue_XCLOSE(ir_own_F)`, or the variant's X form |
| `held = IR_INPLACE_HEAP_SAFE(x, F)`, `IR_INPLACE_OWN(x, F)`, pass-through `res = x` | DSL `TAKE(x)` → `IR_TAKE(x, ir_own_F)` |
| `IR_INPLACE_OBJECT_STEAL(x, F)` | `PyValue_AsKnownObjectSteal(IR_TAKE(x, ir_own_F))` |
| `PyValue receiver = PyValue_MakeHeapSafe(owner)` for a retire (13 sites) | DSL `OWNER(owner)` → `ir_own_F` (S2) |
| Other `MakeHeapSafe(input)` in bodies (27 sites) | DSL `TAKE(input)` |
| An owned input (window, or not borrowable) | unchanged: `PyValue x`, `CLOSE(x)` |

- **`TAKE` and `OWNER`.**
  - `TAKE` is for a value that goes to a home. It takes a count for a local.
  - `OWNER` is the input's own owner, NULL for a local. A receiver that is
    held only so that `IR_RETIRE_PUBLISHED` can release it after the result
    is published needs no count. The generator allows `OWNER` only where
    `runs_code` shows nothing runs between the view's last read and the
    retire (§4.8).
  - In the review's model, `OWNER` removes an incref and a decref from the
    local path of every `self.x`. GCC: `LOAD_ATTR_INSTANCE_VALUE` 113 → 84
    instructions, `STORE_ATTR_INSTANCE_VALUE` 169 → 115.
- **The consume moves to the first take.** `CONTAINS_OP` stores its result
  into a consumed input's slot (`IR_STORE_TEMP_DST`) before a later uop
  releases that input. So ownership must leave the slot at the consume. If
  a take comes before the consume, the slot and the taken value both hold
  the count until the consume; moving the consume closes that window. The
  review checked that no `DEOPT_IF` or `EXIT_IF` follows a take. Error
  paths after a take release the owner, and A5b-2's tests cover them.
- **The DSL kills an input at its take**, so `IR_TAKE` does not clear the
  owner.
- **Cost against A5a.** These are the review's static models, -O2, all
  paths:

  | Handler model | HEAD | Revision 1 form | This form (S1 + S2) |
  |---|---:|---:|---:|
  | `BINARY_OP_ADD_INT`, GCC / Clang | 76 / 75 | 109 / 121 | 76 / 105 |
  | `LOAD_ATTR_INSTANCE_VALUE`, GCC / Clang | 149 / 147 | 113 / 111 | 84 / 102 |
  | `STORE_ATTR_INSTANCE_VALUE`, GCC / Clang | 168 / 158 | 165 / 105 | 115 / 96 |

  The add's Clang figure keeps one `XCLOSE` test on the fast path. The
  proven-inline rule that would remove it is dropped (R2-S3, Q8). The step
  measures the real handlers with `irv2/hpath.py`, for information only.
- **What does not compile.** These all fail on both compilers:
  - `PyValue_CLOSE(x)`, `PyValue_CLEAR(x)` and `IR_STORE_DST(G, x)`;
  - `slots[G] = x`, `PyValue y = x` and `return x` as a `PyValue`;
  - `PyTuple_SET_VALUE(t, i, x)`.

  The generator rewrites only `CLOSE(<input>)`, `TAKE` and `OWNER`. The C
  compiler rejects every other owner use of a view.

### 4.3 One C type per variable (S8)

The generator declares handler-scope variables that several uops share. In
`STORE_ATTR_INSTANCE_VALUE`, `value` holds field A in `_LOCK_OBJECT` and
field B in `_STORE_ATTR_INSTANCE_VALUE`. `IR_INPLACE_OWN` (11 sites) turns a
borrow into an owner in place. Under A5b:

- each variable has one type. The generator gives a variable one name per
  role (view, owner, output), and refuses a name bound to two roles;
- an output assigned from an input view is a DSL `TAKE`;
- a variable that one uop outputs and the next uop takes as an input
  (`dict_st = nos`) stays a view on both sides.

The C compiler finds every case. The work is in A5b-2's estimate (§8).

### 4.4 Alternatives rejected for the hard case

| Alternative | Why rejected |
|---|---|
| Two handler copies per borrowable field, chosen by class bits | 2ⁿ handlers, and G9 already counts the text at 5.5× upstream's; it is G2's choice, not D4's |
| One "maybe owned" type | Today's hazard under a new name |
| Always count an in-place read | Undoes A5a |
| A bit-0 tag in a `PyValue` | Bits cannot tell a borrow from a late-immortal owner, and the compiler lets it be stored |

### 4.5 Boxes leave input variables

At HEAD, an inline input read as an object is boxed in place
(`x = PyValue_Box(x); IR_KEEP_BOX(x)`). Of the 95 sites, 39 box a borrowable
input, 6 an owned input and 50 a window input.

- **The new form.** The generator emits `PyObject *x_o =
  _PyValueTemps_Object(value_temps, x)`, which `value_temps` owns. It
  rewrites the body's object reads:
  - `AsKnownObjectBorrow(x)` becomes `x_o`;
  - `AsPyObjectNew(x)` becomes `Py_NewRef(x_o)`;
  - a steal becomes `Py_NewRef(x_o)` plus the input's release.

  `x` keeps the canonical value.
- **What goes.** `IR_KEEP_BOX`, the generated `PyValue_Box` on inputs
  (97 ir / 5 jit), and `check_boxed_input()`.
- **Relation to R22.** This is R22's object local, owned by `value_temps`.
  R22 still deletes `PyValue_Box`, including its 6 hand-written uses, in
  V2a-1 (Q6).
- **`_PyValue_Publish`.** The box path into homes is gone, but the
  release-build canonicalisation stays until a census shows it is dead (Q4).
  Its stale bit-0 test, "A borrow: the no-count tag" (`pycore_value.h:79`),
  is fixed in A5b-3.

### 4.6 G9 and live values

| Item | Effect |
|---|---|
| Views, owners, `TAKE`, `OWNER`, `XCLOSE` | By value; no address taken |
| `IR_CONSUME_INPUT` | Writes a frame slot, which G9 allows |
| `&held_*`, `&res`, `window`, `_PyValue_Publish(&(V))` | Unchanged; A3a's debt |
| Live values (S8) | The view and the owner are both live between the consume and the view's last read. For a temporary they hold the same word; for a local the owner is NULL. A5b-2 records register pressure on the hot handlers under A3a's GCC convention (six argument registers, five taken), for information |

### 4.7 Object pointers derived from views (S3)

A5a follows `PyObject *o = PyValue_AsKnownObjectBorrow(x)` through plain
copies and refuses consuming calls. In `bytecodes.c` (review's counts):

- 128 uop bodies derive such a pointer;
- about 85 callees receive one;
- 8 sites compute a field address from one;
- about 200 uses take other forms.

| Use of a followed pointer | Revision 2 rule |
|---|---|
| Casts, `(char *)o + offset`, `Py_TYPE(o)`, field reads | a view of the same object; followed |
| Plain local copies, comparisons | followed |
| An argument of a function on the checked reader list | allowed. The list includes the `*_Check*` family, `Py_TYPE`, `LOCK_OBJECT`/`UNLOCK_OBJECT`, `*_GET_SIZE`, `_PyObject_InlineValues`, `_PyObject_GetManagedDict` and others. It lives in the generator, and `test_generated_cases` checks it |
| A store into a field, array element or `?:` result, or any callee not on the list | fail closed: the input leaves `borrow_mask` |

- **Measured before landing.** A5b-2 reports the `borrow_mask` change per
  instruction, and the kernels' dispatch counts (`irv2` scripts).
- **Expected figure:** at most 5 of the 177 borrowable inputs change, and
  none in `LOAD_ATTR_*` or `STORE_ATTR_*`. If more change, the list grows
  before landing.
- **A5a's deny-list gap** (the 7 stealing APIs whose names do not say so)
  does not reach a view today. Its 4 hits in `bytecodes.c` (`:1814`,
  `:1831`, `:1862`, `:2457`) pass a stolen or freshly fetched exception.
- A `PyObjectBorrow` C type stays rejected. It would need `.ptr` at 846
  uses, and the stealing APIs would need an owned-pointer type.

### 4.8 How long a view lives (S7)

A view of a local dangles if an escaping call rebinds the local (through
`f_locals`, or `del` from a trace function) and the view is then read.

**Chosen:** the generator refuses to read a view of a local after an
escaping call in the same instruction, unless the value was taken
(`TAKE`) or the field's owner was taken (`OWNER`) before the call. It
already knows each escape (`escaping_calls`, `runs_code`).

| | Generator refusal (chosen) | Debug borrow ledger, after `Py_STACKREF_DEBUG` |
|---|---|---|
| Run-time cost | none | debug builds only; a table entry per view |
| Coverage | every handler, at generation time | only the paths a test runs |
| Fit | A view has no close, so the generator alone knows where a view ends | needs a generated "view ends" event at every last use |
| Cost to build | about 1 h in A5b-2 | about 3 h |
| Fix for a refused instruction | `TAKE` before the escape, or the input leaves `borrow_mask` | – |

A5b-2 lists the refused instructions with the S3 report. Most bodies that
escape already take their inputs into `held_*` before the call.

### 4.9 The JIT

`generated_jit_helpers.c.h` uses the same macros and gets the same rewrite.
The emitters move raw words between machine registers and slots, and call
the helpers. They have no ownership logic of their own. The helpers'
stores run the debug frame-home check.

## 5. The heap side (item 4)

### 5.1 Every path by which a borrow reaches a heap setter at HEAD

| # | Path | Example | After A5b |
|---|---|---|---|
| P1 | `Borrow` to a setter | `ceval.c:958` `*self = MakeHeapSafe(Borrow(owner))` | type error; `DUP` |
| P2 | Entry borrow to a setter | `dictobject.c` insertions | type error; `DUP` |
| P3 | Container reader to a setter | `templateobject.c:286`, `:290`, `typeobject.c:8836` `PyTuple_SET_VALUE(…, PyTuple_GET_VALUE(…))` | type error; `DUP` at the caller |
| P4 | Dict lookup out-parameter | `_Py_dict_lookup` callers | `PyValueBorrow *`; `DUP` |
| P5 | In-place handler input to a heap store | `STORE_ATTR_*`, `_STORE_SUBSCR_LIST_INT` | `TAKE` |
| P6 | Box borrow | `IR_KEEP_BOX` | gone (§4.5) |
| P7 | Call arguments into a new frame | `_PyValue_OwnArray(args, total)` | arguments are owned; deleted |
| P8 | Native scope record | `native_scope.c:49` receiver, `:54–55` argument | receiver `DUP`; argument a `PyValueBorrow` field (§2.4) |
| P9 | Raw bits and pointer casts | `pycore_value_api.h:34`, `setobject.c:66`, `ceval.c:2246` | owner-header constructors; `raw-bits` |
| P10 | Type cache read back | `typecache.c:116`, `:209–210` | `_PyValueGuarded` (S4) |
| P11 | Copies of homes | `bytecodes.c:2728` `PyValue found = *value_ptr;` | retyped in A5b-3 (§5.3) |

### 5.2 The 150 `MakeHeapSafe` uses

Each use is decided by the type of its argument. If the argument is a
`PyValueBorrow`, the use took a count and becomes `DUP`. If it is a
`PyValue`, the use was a move and is deleted.

| Class | Meaning | Uses | Files | Action |
|---|---|---:|---:|---|
| S | A setter or stealer promotes its own argument | 7 | 4 | delete |
| N | `_PyNativeScope.argument`, owned or borrowed by a run-time test (`native_scope.c:55`) | 1 | 1 | the field is deleted; `argument_box` owns a box (R2-S1) |
| C | Converts a C-local borrow | 89 | 18 | `DUP` |
| I, take | Keeps an input in a home (`bytecodes.c`) | 27 | 1 | DSL `TAKE` |
| I, receiver | `PyValue receiver = PyValue_MakeHeapSafe(owner)` for a retire | 13 | 1 | DSL `OWNER` |
| W | Keeps a call-window value (`ceval.c`) | 7 | 1 | delete |
| F | `_PyFrame_Copy` | 3 | 1 | delete |
| D | Definitions | 3 | 2 | delete |
| | **Total** | **150** | **24** | 89 `DUP`, 27 `TAKE`, 13 `OWNER`, 21 deleted |

- **S.** `PyTuple_SET_VALUE` (`tupleobject.c:139`), `PyTuple_SetValue`
  (`:164`), `_PyTuple_ReplaceTakeRef` (`:183`),
  `_PyTuple_FromValueStealOnSuccess` (`:559`),
  `_PyList_FromValueStealOnSuccess` (`listobject.c:2980`),
  `_PySet_AddValueTakeRef` (`setobject.c:392`), `_PyCell_SwapValue`
  (`pycore_cell.h:29`).
- **Counts that move to callers.** Deleting class S moves the count to every
  caller that passes a borrow to a stealing setter. Examples are
  `templateobject.c:286`, `:290` and `typeobject.c:8836`. The compiler finds
  all of them, since `GET_VALUE` returns a `PyValueBorrow`. The class split
  counts sites, not the `DUP`s that A5b-3 adds.
- **The 13 receivers.** `bytecodes.c:2011`, `:2674`, `:2732`, `:2763`,
  `:2792`, `:2833`, `:2853`, `:2989`, `:3014`, `:3036`, `:3083`, `:3105`
  and `:3127`.

### 5.3 Copies of homes (M4)

- **Census.** A5b-3 lists every `PyValue x = <field, element or *pointer>;`.
  `rg` finds 71 in 20 files: `listobject.c` 13, `dictobject.c` 10,
  `ceval.c` 7, `bytecodes.c` 7, `tupleobject.c` 6 and others. The review's
  wider pattern finds 80 in 22.
- **Moves and non-moves.** A copy followed by a store to the same home is a
  move and stays a `PyValue`. Every other copy becomes `PyValueBorrow x =
  PyValue_Borrow(home)`.
- **Example.** In `_LOAD_ATTR_INSTANCE_VALUE`, `found` becomes a borrow
  (`bytecodes.c:2728`), so writing `attr = found` instead of `attr =
  XDUP(found)` stops compiling.
- **The `home-copy` lint.** It reports a `PyValue` local initialised from a
  home lvalue with no later store to that home. It reports and does not
  gate.

### 5.4 Other deletions and changes

| Item | Change |
|---|---|
| `PyValue_IsHeapSafe` (46 hand-written uses in 18 files, 1 generated) | Asserts become the home check. The release-mode check at `native_scope.c:93–97` ("native vector arguments must be strong values"), which the A5a notes cite as the release build's fail-closed evidence, goes with it. The argument type replaces it. |
| `PyValue_AsStrongReference` (4) | An identity in the GIL build after A5b; deleted |
| `PyValue_BorrowAt` (2) | Deleted (§2.4) |
| `_PyValue_OwnArray` (2) | Deleted |
| `IR_LOAD_BORROWED`, `IR_INPLACE_*`, `IR_KEEP_BOX`; the generator's `INPLACE_RELEASES`, `INPLACE_OWNERS`, `check_boxed_input()` | Deleted |

### 5.5 Debug asserts (S6)

| Check | At HEAD | After A5b |
|---|---|---|
| Home predicate | `_PyValue_IsCanonicalHome` (heap) and `_PyValue_IsFrameHome` (frame, with H1) | One predicate, `_PyValue_IsHome`: canonical, plus H1 unless finalizing |
| `_PyGC_VisitValue` (`gc.c:466`) | `_PyValue_RecheckHome`, then it skips a tagged value | the merged check |
| `PyValue_CheckValid` | shape check | also H1, unless finalizing, and H2 |
| `IR_STORE_DST` (`ceval_macros.h:724`) | `assert(PyValue_IsHeapSafe(...))` | the type, plus `_PyValue_CheckFrameHome` |
| `traverse_slots` | `assert(PyValue_IsHeapSafe(*slot))` | the home check |
| The writers A5a left unchecked: `STORE_ATTR_INSTANCE_VALUE`, `STORE_ATTR_WITH_HINT`, `STORE_ATTR_SLOT`, `_STORE_SUBSCR_LIST_INT`, the `dictresize` moves, `PyCell_New` | none | `_PyValue_CheckHome` after the store |

**Finalization carve-out.** These paths make immortals mortal again, so
surviving tagged words then break H1:

- `_PyUnicode_ClearInterned` (`unicodeobject.c:15111`,
  `_Py_SetMortal(s, 2)`);
- `clear_tp_bases` and `clear_tp_mro` with `final` set (`typeobject.c:698`,
  `:760`, `_Py_ClearImmortal`);
- `common_constants_clear` (`pystate.c:717`).

`finalize_interp_clear` reaches all of them, and
`_PyInterpreterState_SetFinalizing` runs before it: in `Py_FinalizeEx`
(`pylifecycle.c:2435`, then `:2589`) and in `Py_EndInterpreter` (`:2831`,
then `:2852`). The H1 clause reads "tagged implies immortal, or the
interpreter has entered `finalize_interp_clear`", using a flag set on entry
to `finalize_interp_clear` (`pylifecycle.c:2123`). That keeps the check on
through `finalize_modules` and the last collection. A5b-3 checks the one
other caller of `PyInterpreterState_Clear` (`pystate.c:982`), and runs
`test_interpreters` in a debug build. "Tag only static immortals" (Q1) would make H1 hold
for an object's whole life.

### 5.6 Lint and self-tests (`Tools/build/check_value_api.py`)

| Rule | Fires on |
|---|---|
| `borrow-field` | Any field, array, global or static of type `PyValueBorrow`; no exceptions after R2-S1. If the A5b-3 census finds another field, it is listed and the rule allows only that field |
| `raw-bits` | `(PyValue){`, `(PyValueBorrow){`, `.bits =`, and casts to `PyValue *` or `PyValueBorrow *`, outside the owner headers, `_PyIR_AdoptTemp` and `pycore_typecache.h`. As built (A5b-3, A5b-4), a cast between a word pointer and a byte pointer is allowed everywhere, so `structmember.c` and the `STORE_ATTR_*` offset stores (byte offsets from a `char *`) need no entry; a word pointer made from the address of a non-word object or from a cast of another typed pointer, and a typed pointer made from a cast of a word pointer, fail, so a second cast cannot launder the first (value-api.md §1.6) |
| `home-copy` | §5.3; reports only |
| Negative units | Run under both `gcc` and `clang-21`. Each must fail with its expected diagnostic, matched by a pattern per compiler, as the `Py_VALUE_COMPAT` probe matches "unavailable". A unit that fails for any other reason fails the self-test. The units are the eight of Appendix A, plus `PyTuple_SET_VALUE(t, 0, PyTuple_GET_VALUE(u, 0))`, and `_PyDict_SetValue_Take2(d, k, PyValue_FromObjectBorrow(o))` against the real headers. One positive unit passes owners, borrows, `*p` and `slot[i]` to each reader family |

## 6. `PyValue_Is` and the bit-0 mask (item 5)

At HEAD, `PyValue_Is(a, b)` is
`((a.bits ^ b.bits) & ~(uintptr_t)PyValue_IsPointerSpace(a)) == 0`.
`a.bits == b.bits` would be a correct identity test only if every word that
refers to one object had the same bit 0.

| Source of two bit patterns for one object | Removed by A5b? |
|---|---|
| A tagged borrow next to an untagged owner | Yes (B1) |
| Late immortalisation. `immortalize_interned` (`unicodeobject.c:14814`) runs through `_PyUnicode_InternImmortal`, which is called from `codeobject.c:181`, `dictobject.c:5154`, the parser and codecs. Owners made later are tagged; earlier ones are not | No |
| Finalization making immortals mortal again (§5.5) | No |

**Decision:** keep the mask, in an inline reader. The cost is unchanged, and
upstream's `PyStackRef_Is` also masks both operands (`pycore_stackref.h:742`).

The alternative, tagging only static immortals, saves about three
instructions on `IS_OP` alone, because `IsNone`, `IsTrue` and `IsFalse`
already compare exactly. It is recorded in G6 as the target form, because it
makes bit 0 a function of the object for its whole life (Q1).

## 7. Pair merges and renames (item 6)

| Today | Becomes | Hand (files) | ir / jit | Method |
|---|---|---:|---:|---|
| `PyValue_NewRef(v)` | `PyValue_XDUP(v)` | 41 (11) | 16 / 2 | token rename |
| `PyValue_Close(v)` | `PyValue_XCLOSE(v)` | 9 (4) | 0 / 0 | token rename |
| `PyValue_FromPyObjectSteal(o)` | `PyValue_FromObjectSteal(o)` | 95 (11) | 60 / 6 | token rename |
| `PyValue_FromPyObjectBorrow(o)` | `PyValue_FromObjectBorrow(o)` | 9 (5) | 2 / 0 | token rename |
| `PyValue_FromPyObjectNew(o)` | `PyValue_DUP(PyValue_FromObjectBorrow(o))` | 35 (9) | 18 / 0 | paren-balancing rewrite |
| `_PyValue_FromPyObjectNew(o)` | the same | 2 (1) | 0 / 0 | by hand |
| `_PyValue_FromObjectBorrowInline(o)` | `PyValue_FromObjectBorrow(o)`, now inline | 3 hits (2) | 0 / 0 | in A5b-3, before the rewrite above |
| `PyValue_BorrowAt(p)`, `PyValue_AsStrongReference(v)` | deleted | 2, 4 | 0 / 0 | in A5b-3 |

- **Survivors.** `DUP`/`XDUP` and `CLOSE`/`XCLOSE`, following value-api.md
  §1.5 rule 4 (Q3).
- **`NewRef` → `XDUP` comes after A5b-3.** Only then are there no tagged
  mortals, and an untagged immortal is valid under H1.
- **`FromPyObjectNew`** asserts `o != NULL` (`pycore_value.h:699`), and
  `DUP` keeps that check.
- **`FromObjectBorrow` becomes inline first** (S5). Otherwise the 53
  `FromPyObjectNew` sites, 18 of them generated, would gain an out-of-line
  call.
- **Script.** `build/pyjit-link-wave1/a5b/rename.py` works on tokens and
  skips comments. It touches 154 renames, 35 rewrites and 8 generator
  strings (`analyzer.py` 6, `ir_generator.py` 2). Regeneration then changes
  104 generated sites.

## 8. Steps (item 7)

### 8.1 The steps

Each step is one reviewable diff. It builds and passes its tests on GCC and
`clang-21`, gated on correctness only, and records performance for
information.

| Step | Content | Script or hand | Test | Hours |
|---|---|---|---|---:|
| **A5b-1** Types and readers | `PyValueBorrow`; the `PyValue_Borrow` view (`_Generic` in C, overloads in C++); readers take `PyValueBorrow`, each with its same-name macro; `Is`/`IsNone`/`IsTrue`/`IsFalse` become inline functions; `DUP`/`XDUP` take `PyValueBorrow`; the tagging producer renamed `_PyValue_BorrowTagged` at its sites and in `IR_KEEP_BOX` (R2-M2); `-Werror=incompatible-pointer-types` in `CFLAGS_NODIST`; negative units on both compilers. Nothing produces a `PyValueBorrow` yet. | Script for the reader macro lines and the rename; flags and self-tests by hand | Builds on both compilers; `test_pyvalue_api`, `test_tagged_values`, `test_capi`, `test_cext`; the self-tests: Appendix A's negative units 1–6 and 8 against the real headers, and the positive unit | 3 |
| **A5b-2** Generator | View, adopted owner, `TAKE`, `OWNER`; `_PyValue_BorrowTagged` leaves generated code; variables split by role (S8); boxes as `x_o`; the S3 object rule with its checked list; the S7 escape refusal; deletions (§5.4); reader signatures that views reach | Hand; regenerate | `test_generated_cases` (view and owner emission, `TAKE` before and after the consume, `OWNER` with and without code in between, a refused view after an escape, a boxed owned input); negative unit 7 (`IR_STORE_DST`) against `ceval_macros.h`; the step-0 debug set; the `borrow_mask` and refusal report, with dispatch counts, before landing; `hpath.py` and register pressure for information | 8 |
| **A5b-3** Heap side | Producers return `PyValueBorrow` (§2.4); inline `FromObjectBorrow`; 89 `DUP`s plus the moved caller counts; S, W, F and D deleted; the native scope's `argument_box` (R2-S1) and the `native_scope_box_oom` fix (R2-S4); `_PyValueGuarded`; the home-copy census and retype; `MakeHeapSafe`, `IsHeapSafe`, `OwnArray`, `BorrowAt`, `AsStrongReference` and `_PyValue_BorrowTagged` deleted; the merged home check with the finalization clause; checks at the unchecked writers; `borrow-field`, `raw-bits` and `home-copy` | Hand, driven by compile errors; `tally.py` gives each site's expected class | Debug suite with the new asserts on both compilers; `test_gc`, `test_dict`, `test_set`, `test_tuple`, `test_list`, `test_weakref`, `test_sys`, `test_capi`, `test_interpreters`, `test_tagged_values`; lint clean; the §5.6 header-level negative units | 7.5 |
| **A5b-4** Merges and renames | §7 | `rename.py`; regenerate | Build; `tally.py` finds none of the old names; the step-0 set | 1 |
| Suites and review | Full release and debug suites on both compilers; post-review fixes | – | The 51,571-test suite (2 known `test_capi` refcount failures, open for B5) | 2 |
| **Total** | | | | **21.5** |

- **Order.**
  - A5b-1 is safe alone once the tagging form has its own name (R2-M2).
  - A5b-2 comes before A5b-3, because the generated code calls
    `MakeHeapSafe`.
  - A5b-3 must untag `Borrow` in the same diff that converts
    `MakeHeapSafe(Borrow(x))` to `DUP`. The type makes the compiler find
    every such pair.
  - A5b-4 comes last, for the `NewRef` reason (§7).
- **The estimate.** Against the review's 18–20 h, this adds the
  variable-splitting work (S8) and R2-S1's 0.5 h. A5a's estimate was 5 h
  and it took about 2.7 h, so 21.5 h is an upper budget rather than a
  forecast.

### 8.2 What A3a inherits (M3, R2-S2)

| Item | Resolution |
|---|---|
| The record rule | A3a's activation record outlives handlers, so it is a home and holds owned `PyValue` words only (L1). A C-stack record that lives for one handler activation is a C local. The rule governs `PyValue` words: the scope's object pointers (`objects`, `argument_objects[0]`) are views of values the handler holds, and `End` releases them within the same activation (R2-S2). |
| `_PyNativeScope` in A3a's scratch union | A5b-3 already removes `argument` (R2-S1), so the scope holds no `PyValue` borrow and may go into the scratch. The fixed `Begin` has one runtime caller, `bytecodes.c:4589` and its generated copy, whose `_PyNativeScope_Objects` call is `methodobject.c:543`; the other `Objects` calls (`native_scope.c:164`, `ceval.c:1883`) serve other scope kinds. Its four test callers are in `_testinternalcapi/native_scope.c` (`:119`, `:219`, `:348`, `:429`). Eager boxing adds a box allocation only on the cold error returns before `methodobject.c:543`. |
| `held_*` and `receiver` in A3a's scratch | These are the lines A5b rewrites: `held_dict`, `held_sub`, `held_value`, `held_container`, `release_value` and `retained_dict` at `bytecodes.c:1237–1238`, `:1321–1323`, `:1374–1376` and `:3082`, and the 13 receivers. After A5b they are owned values: `TAKE` results, and `OWNER` results that are NULL for a local. So the record may hold them. |
| Order | A5b lands first. A3a rebases on A5b's `TAKE` and `OWNER` lines, and needs no change to its ownership rule for them. Revision 1's "neither step's diff overlaps the other's" was wrong. |
| What A3a needs from A5b | The record rule; the register-pressure figures from A5b-2; parameter types for its slot-addressed helpers: `const PyValue *` for slots and `PyValueBorrow` for values they only read. |

## 9. Comparison (item 8; this is the scope cut)

| System | What it does | Taken | Rejected or missing |
|---|---|---|---|
| Upstream `_PyStackRef` (`3a440b6`) | A dynamic tag bit. Frame stack slots do hold borrows (`LOAD_FAST_BORROW` pushes `PyStackRef_Borrow`). Generator frames are heap homes, made safe when copied (`_PyFrame_Copy` calls `PyStackRef_MakeHeapSafe`). Heap fields hold `PyObject *`. `PyStackRef_Is` masks both operands. `Py_STACKREF_DEBUG` tracks borrow provenance, and fails when an owner is closed with live borrows. | The mask in `Is`; `DUP` as the one counting conversion | A tag that means "borrowed": the fork's heap holds values, so the type carries ownership. The debug ledger becomes the S7 generator rule. |
| HPy | Two types: `HPy` (a handle, local) and `HPyField` (heap), with `HPyField_Store` and `HPyField_Load`. Debug mode detects use of a closed handle. | The local type | The field type is the lesson that closes the copy-of-home gap (M4). It is recorded for value-api V4/V5 as `PyValueField`. |
| V8 | `Local<T>` is a copyable class. Safety comes from `HandleScope` lifetimes, debug zapping of closed scopes, and static analysis of stack-only types. Object fields hold `Tagged<T>`. | A checker for stack-only types: `borrow-field` | Scopes and zapping: a reference count keeps a borrow's owner alive, not a GC root |
| Rust | `&T` cannot outlive its owner, and an owned non-`Copy` `T` moves. | `ir_own_F` is an `Option<T>`, the principled form for a field with two ownership states | C's `PyValue` is `Copy`, so moves are untracked (§2.3) |
| PyPy cpyext | Keepalive lists; borrows are untyped | – | The model HPy replaced |

## 10. Risks and open questions (item 9)

### 10.1 Risks

| # | Risk | Recommendation |
|---|---|---|
| R1 | A new core warning flag on two compilers | `-Werror=incompatible-pointer-types` is already the default on GCC 14+. The first Clang build in A5b-1 shows what else it finds; fix those findings in that step. |
| R3 | Copies of homes | The census and `home-copy` in A5b-3; the field type in V4/V5 |
| R5 | The refcount load in `FromObjectBorrow` | It shares a cache line with the type load; record it, no gate |
| R6 | Register pressure from views and owners | Measured in A5b-2 under A3a's convention |
| R7 | The S7 refusal pushes inputs out of `borrow_mask` | Reported with the S3 figure before landing; `TAKE` before the escape where that is cheaper |

### 10.2 Open questions (decided after review 1)

| # | Question | Decision |
|---|---|---|
| Q1 | Tag only static immortals, so that `Is` can become `==` | Keep the mask in A5b. G6 records static-only tagging as the target form. |
| Q2 | The type cache | Replaced by S4: `_PyValueGuarded`. |
| Q3 | Survivor names | `DUP`/`XDUP` and `CLOSE`/`XCLOSE`. |
| Q4 | `_PyValue_Publish` becoming an assert | A follow-up after a census. |
| Q5 | The type name | `PyValueBorrow`. |
| Q6 | R22's object locals taken early | Accepted. |
| Q7 | The eight close forms (N2) | Later: one `CLOSE` with `__attribute__((assume))` or `__builtin_assume` hints. Not A5b. |
| Q8 | Dropping the release of an input a guard proved inline (R2-S3) | A follow-up, not A5b. It needs a declared fact (a DSL annotation on the guard's output, such as `value: inline`, checked against the guard's `EXIT_IF`) and `assert(PyValue_IsInline(x))` wherever a release is dropped. |

## 11. Design-document edits for the coordinator

| Document | Edit |
|---|---|
| pyvalue.md D4, G6, §1.6 | The `PyValueBorrow` type and the `PyValue_Borrow` view; B1, H1 (with the finalization clause), H2 and L1; the guarantee as §2.3 states it; static-only tagging as G6's target form; `PyValue_Wrap`'s bare pointer listed as a third meaning of the owner type (N4); `traverse_slots` already done |
| pyvalue.md:223, register-ir.md:151 | The typed borrow does not cover derived `PyObject *`; the generator's checked list does (§4.7) |
| register-ir.md IR-D3, generator section | View and owner per field; adoption; `TAKE` and `OWNER`; the consume at the first take; the escape refusal |
| value-api.md §1.3, §1.5, R22, V4, V5 | `FromObjectBorrow` inline, returning `PyValueBorrow` and tagging only immortals; `NewRef` → `XDUP`; corrected counts; R22's object local owned by `value_temps`; V4 and V5 adopt `PyValueField` for heap homes; V4 drops rule 3's `…Borrowed` suffix (N3) |
| Packet A5b | 150 uses in 24 files and the class split; 21.5 h; both compilers |
| Packet A3a | The record is a home and holds owned `PyValue` words only; C-stack records that live for one handler activation are C locals; the scope's object pointers are views released by `End` (R2-S2). `_PyNativeScope` holds no borrow after A5b-3 and may enter the scratch (§8.2). Rebase on A5b's `TAKE`/`OWNER` lines; the overlap sentence corrected. Slot-addressed helpers take `const PyValue *` and `PyValueBorrow`. |

## Appendix A. The reader-form probe

Run for this revision with GCC 15.2 and Clang 21.1, `-std=c11
-Werror=incompatible-pointer-types`:

- The good unit compiles cleanly under `-Wall -Wextra`.
- Each `BAD=n` unit is an error on both compilers.
- At `-O2`, `via_macro_target` and `via_plain` compile to the same
  instructions on each compiler (GCC: `movabs`, `cmp`, `setb`; Clang:
  `shr $51`, `cmp`, `setae`).
- The file compiles as C++11 under `g++` and `clang++-21`.
- Without the flag, Clang only warns on `BAD=5`, `6` and `7` (review M1).

| BAD | Misuse | GCC 15.2 | Clang 21.1 |
|---|---|---|---|
| 1 | borrow stored to an owner | incompatible types when assigning | assigning from incompatible type |
| 2 | borrow passed to `CLOSE` | incompatible type for argument 1 | passing to parameter of incompatible type |
| 3 | `uintptr_t` passed to a reader | `_Generic` selector not compatible | controlling expression not compatible |
| 4 | closer assigned to a reader pointer | incompatible pointer type | incompatible function pointer types |
| 5 | `PyValue *p = &b` | incompatible pointer type | incompatible pointer types |
| 6 | `PyValue_CLEAR(b)` | incompatible pointer type | incompatible pointer types |
| 7 | `IR_STORE_DST(0, b)` | incompatible pointer type | incompatible pointer types |
| 8 | `return b` as a `PyValue` | incompatible types when returning | incompatible result type |

```c
#include <stdint.h>
typedef struct { uintptr_t bits; } PyValue;
typedef struct { uintptr_t bits; } PyValueBorrow;
#ifdef __cplusplus
static inline PyValueBorrow PyValue_Borrow(PyValue v) { PyValueBorrow b = {v.bits}; return b; }
static inline PyValueBorrow PyValue_Borrow(PyValueBorrow v) { return v; }
#else
static inline PyValueBorrow _PyValue_BorrowOwned(PyValue v) { return (PyValueBorrow){v.bits}; }
static inline PyValueBorrow _PyValue_BorrowBorrow(PyValueBorrow v) { return v; }
#define PyValue_Borrow(V) _Generic((V), PyValue: _PyValue_BorrowOwned, PyValueBorrow: _PyValue_BorrowBorrow)(V)
#endif
static inline int PyValue_IsInlineInt(PyValueBorrow v) { return v.bits >= ((uintptr_t)0xFFF8 << 48); }
#define PyValue_IsInlineInt(V) (PyValue_IsInlineInt)(PyValue_Borrow(V))
void PyValue_CLOSE(PyValue v);
static inline PyValue _PyValue_Publish(PyValue *p) { return *p; }
#define PyValue_CLEAR(REF) do { PyValue *t = &(REF); PyValue o = *t; *t = (PyValue){0}; PyValue_CLOSE(o); } while (0)
#define IR_STORE_DST(F, V) (slot[F] = _PyValue_Publish(&(V)))
PyValue slot[4];
typedef int (*reader_fp)(PyValueBorrow);
int good(PyValue o, PyValueBorrow b, const PyValue *p) {
    reader_fp f = &PyValue_IsInlineInt;
    PyValueBorrow c = PyValue_Borrow(o);
    return PyValue_IsInlineInt(o) + PyValue_IsInlineInt(b) + PyValue_IsInlineInt(*p)
         + PyValue_IsInlineInt(slot[1]) + PyValue_IsInlineInt(c) + f(b);
}
#if BAD == 1
void bad(PyValueBorrow b) { slot[1] = b; }
#elif BAD == 2
void bad(PyValueBorrow b) { PyValue_CLOSE(b); }
#elif BAD == 3
int bad(uintptr_t u) { return PyValue_IsInlineInt(u); }
#elif BAD == 4
void (*bad)(PyValueBorrow) = PyValue_CLOSE;
#elif BAD == 5
void bad(PyValueBorrow b) { PyValue *p = &b; (void)p; }
#elif BAD == 6
void bad(PyValueBorrow b) { PyValue_CLEAR(b); }
#elif BAD == 7
void bad(PyValueBorrow b) { IR_STORE_DST(0, b); }
#elif BAD == 8
PyValue bad(PyValueBorrow b) { return b; }
#endif
__attribute__((noinline)) int via_macro_target(PyValue o) { return PyValue_IsInlineInt(o); }
__attribute__((noinline)) int via_plain(PyValue o) { return o.bits >= ((uintptr_t)0xFFF8 << 48); }
```
