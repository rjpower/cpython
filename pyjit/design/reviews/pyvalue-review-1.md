# Review 1: PyValue value representation

Document: [`../pyvalue.md`](../pyvalue.md) (uncommitted, 2026-11-03).
Code: `build/worktrees/pyvalue-rc-universal` at `271f4e435e5`. Uncommitted edits
by other agents in that tree were ignored. Paths below are relative to that tree.

**Verdict: accept with changes.** The document's technical claims are almost all accurate. Its
comparison with other runtimes is sound, and its gap list finds the right weak points. Its
recommendations are too timid in three places, and it misses one important comparison
(CRuby) and several observable-behaviour and ABI consequences. Nine of the 37 checked claims are
stale, incomplete or wrong; none of them invalidates the design.

## 1. Accuracy

### 1.1 Claims checked

| # | Claim (doc section) | Code | Result |
|---|---|---|---|
| 1 | `PyValue` is `struct { uintptr_t bits; }` (1.1) | `Include/pytypedefs.h:19` | Holds |
| 2 | `#error` unless 64-bit GIL (1.1, 1.12) | `Include/pyvalue.h:10` | Holds |
| 3 | Constant table (1.2) | `pycore_value.h:173-190, 459` | Holds |
| 4 | `pyvalue.h` repeats region constants as literals (1.2) | `pyvalue.h:PyValue_Borrow`, `IsObject`, `IsLanguageValue` | Holds |
| 5 | Bit layout, NaN image ranges, wrap of negative NaNs (1.3) | Arithmetic from `_PyValue_DoubleBits` | Holds |
| 6 | `IsObject`, `IsTaggedIndex`, `IsInlineFloat`, `IsInlineInt` definitions (1.4) | `pyvalue.h:36`, `pycore_value.h:192-196` | Holds |
| 7 | `BothInlineInt` AND trick relies on `0xFFFF` unused (1.4) | `pycore_value.h:198` | Holds |
| 8 | `IsLanguageValue` rejects unused int-region patterns (1.3, 1.4) | `pyvalue.h:PyValue_IsLanguageValue`: `0xFFFE` with bits 32-47 set and `0xFFFF` decode as NaN | Holds |
| 9 | `IsNone/IsTrue/IsFalse` are exact compares with bit 0 (1.4) | `pycore_value.h:473-475` | Holds |
| 10 | `IsInlineAt` reads the top 16 bits in place (1.4) | `pycore_value.h:_PyValue_TopBitsAt` | Holds |
| 11 | `FromPyObjectSteal` tags from `ob_flags` (1.6) | `pycore_value.h:PyValue_FromPyObjectSteal` | Holds |
| 12 | `FromPyObjectNew` tags immortals (1.6) | `_PyValue_FromPyObjectNew` uses `_Py_IsImmortal` (the refcount), not `ob_flags` | Partly: different immortality test from Steal (see 1.3) |
| 13 | `DUP` of a borrow stays a borrow (1.6) | `pycore_value.h:PyValue_DUP` | Holds |
| 14 | `Close` uses `Py_DECREF`; `CLOSE` uses `Py_DECREF_MORTAL` (1.6) | `pyvalue.h:PyValue_Close`, `pycore_value.h:PyValue_CLOSE` | Holds |
| 15 | `AsPyObjectBorrow` only debug-asserts pointer space (1.6) | `pycore_value.h:PyValue_AsPyObjectBorrow` | Holds; `AsPyObjectSteal` has the same release hole |
| 16 | `FromObjectBorrow` canonicalises via `_PyObject_ValueIdentity` and `PyLong_AsLongAndOverflow` (1.5) | `Python/pyvalue.c` | Holds |
| 17 | `PrepareOwned` returns the immediate and hands back the box (1.5) | `pyvalue.c:PyValue_PrepareOwned` | Holds |
| 18 | `_PyValueTemps_Publish` tests exact int or float on every pointer result (1.5) | `pycore_value.h:_PyValueTemps_Publish` | Holds |
| 19 | `PyTuple_SET_ITEM`, `SetItem`, `PyDict_SetItem`, `PyList_Append` canonicalise (1.5) | `tupleobject.c:(PyTuple_SET_ITEM)`, `dictobject.c:PyDict_SetItem`, `listobject.c:_PyList_AppendTakeRef` | Holds |
| 20 | Value-form setters "trust their input" (1.5) | `tupleobject.c:(PyTuple_SET_VALUE)`, `PyTuple_SetValue`, `_PyTuple_FromValueStealOnSuccess` call `PyValue_MakeHeapSafe` | Partly: they promote borrows; only canonical form is trusted |
| 21 | `PyValue_Is` XORs, masks bit 0 if `a` is a pointer, else calls out of line (1.7) | `pycore_value.h:742`, `pyvalue.c:_PyValue_IsSameValue` | Holds |
| 22 | `id()` and `operator.is_` use `_PyObject_ValueIdentity` (1.7) | `bltinmodule.c:builtin_id_impl`, `_operator.c:715,729` | Holds |
| 23 | `LOAD_CONST` is `NewRef(PyTuple_GET_VALUE(consts))` (G1) | `bytecodes.c:271` | Holds |
| 24 | `_PyValue_Hash`: int maps -1 to -2; float uses `_Py_HashDouble(Py_None, d)` (1.7) | `Objects/value_ops.c` | Holds. Inline and boxed forms hash alike, and so do equal int and float values |
| 25 | `compare_inline` is exact for int32 (1.7) | `value_ops.c:compare_inline` | Holds |
| 26 | `compare_generic`: `PyValue_Is`, then hash, then `_PyValue_RichCompareBool` (1.7) | `dictobject.c:1176` | Holds |
| 27 | `tuple_hash` uses `_PyValue_Hash` (1.7) | `tupleobject.c:453` | Holds |
| 28 | `unsafe_float_compare` reads inline floats (1.7) | `listobject.c:2589` | Holds |
| 29 | `_Py_VISIT_VALUE` visits objects only; `_PyGC_VisitValue` skips `visit_decref` when bit 0 is set (1.9) | `pycore_value.h:1134`, `gc.c:466` | Holds |
| 30 | `traverse_slots` calls `visit` directly (1.9) | `typeobject.c:2981` | Holds, but it also asserts `PyValue_IsHeapSafe` (see 1.2) |
| 31 | `init_value_slots` walks every base's `tp_members` on every allocation (G9) | `typeobject.c:2876` | Holds |
| 32 | `_PySlotKind` has only `_Py_SLOT_BOXED` (G8) | `pycore_dict.h:305` | Holds |
| 33 | Empty set entries are `PyValue_NULL` with hash 0 (1.8) | `setobject.c:253` | Holds |
| 34 | `RefcountOnObject(ERROR)` is true, so `CLOSE(ERROR)` decrefs address 2 (G3) | `pycore_value.h:Py_OWNED_MASK`, `PyValue_CLOSE` | Holds; the `_POINTER` variants have the same hole (see 1.2) |
| 35 | Native calls run `MakeHeapSafe`, then `AsObject` with `PyErr_Occurred` per argument (1.10) | `ceval.c:_PyObjectArray_FromValueArray` | Holds |
| 36 | The trace JIT encodes results as `+ Py_INT_BASE` and tests the slot's top 16 bits (G12) | `jit_backend_x86.c:270-280` | Holds |
| 37 | Stale `PySequence_Fast_GET_ITEM`; `_ctypes/callbacks.c` assigns `PySequence_Fast_ITEMS` to `PyObject **` (G11) | `Include/cpython/abstract.h:96`; `callbacks.c:146,441` at HEAD | Holds |

Call-site counts in 1.5 and G5 match within one (158, 53, 13 and 72; `PyValue_NewRef` is now 58).
I did not check the JSC `BigInt32` history in 2.1. Cite a source or remove the claim.

### 1.2 Stale, incomplete or wrong

1. **Stale header and G1.** The source-of-truth line names `5e977ec8cbb`, and the code is now six
   commits newer. The §1.6 `NewRef` row and G1 describe the bug fixed in `271f4e435e5`. Mark G1
   fixed and move its remaining items into a new G1' (section 1.3).
2. **G1's invariant cannot hold.** "An immortal reference always carries bit 0" fails for objects
   made immortal after a reference to them already exists (interning, `_Py_SetImmortal`). The
   `pycore_value.h:PyValue_CheckValid` comment says so. The invariant that can hold is that
   *statically allocated* immortals always carry bit 0, which is what `PyValue_IsNone` and the
   other exact tests need. `PyValue_CheckValid` and debug `Py_DECREF_MORTAL` already assert it in
   part.
3. **The G6 "silent leak" bullet is wrong.** Skipping `visit_decref` for a reference that owns no
   count is the correct accounting, and upstream's `_PyGC_VisitStackRef` does the same. A stored
   borrow is a use-after-free hazard, not a GC leak.
4. **§1.9 does not say which traversal is wrong.** `traverse_slots` asserts `PyValue_IsHeapSafe`.
   Under that invariant the only bit-0 values it visits are immortals, and `_Py_SetImmortal`
   untracks those, so `visit_decref` does nothing to them. If the invariant breaks, `traverse_slots`
   over-subtracts and can free reachable objects early. Use `_Py_VISIT_VALUE` there.
5. **G3's fix is incomplete.** `_PyValue_PointerOwnsCount` (used by `_PyValue_CLOSE_POINTER`,
   `_DUP_POINTER` and `_AsPyObjectStealPointer`) tests bit 0 alone, so it also treats `ERROR` and
   `Wrap` values as owned.
6. **§1.5 overstates the setters.** Value-form setters promote borrows through `MakeHeapSafe`; they
   trust only the canonical form (row 20).
7. **§2.2 says no runtime has wider inline ints.** CRuby, the closest analogue to Python, is
   missing (section 2.1).

### 1.3 Is the G1 fix correct and complete?

The new `Include/pyvalue.h:PyValue_NewRef` is semantically correct for the core. It INCREFs a
mortal object and clears bit 0; it tags an immortal one. Remaining sources of an untagged immortal
(U) or a tagged mortal owner (T):

| Path | Can yield | Severity |
|---|---|---|
| `PyValue_NewRef` in a public header uses `_Py_IsImmortal`, which `refcount.h:125` hides under `_Py_OPAQUE_PYOBJECT`. `Python.h:81` includes `pyvalue.h` unconditionally | Compile error for any extension that defines `Py_TARGET_ABI3T` (`pyabi.h:66`). The refcount layout is also inlined into every extension | **Bug, introduced by the fix** |
| Different immortality tests: `FromPyObjectSteal` uses `ob_flags & 1`; `FromPyObjectNew`, `NewRef` and `IsHeapSafe` use the refcount sign. A count that saturates at 2^31 makes an object immortal without the flag (`refcount.h:44`) | U from Steal, tagged from New and NewRef: two encodings of one object | Low (inherited from upstream). Use one test, `_PyValue_TagFor(obj)`, everywhere |
| `PyValue_FromPyObjectStealMortal` in `BUILD_TUPLE`. `_PyTuple_FromValueStealOnSuccess(…, 0)` returns the static empty tuple | U, a static immortal. Debug builds assert; release `CLOSE` runs `DECREF_MORTAL` on it | Latent. Only reachable if `BUILD_TUPLE 0` is emitted; upstream folds it to `LOAD_CONST ()`. Assert `oparg > 0` or use `Steal` |
| Dynamic immortalisation after a reference exists | U | Legitimate. Exact-bit tests are only valid against static immortals; say so |
| `MakeHeapSafe`, `DUP`, `XSETREF` | Keep their input's tag; they create neither | OK |
| `PyValue_Borrow(PyValue_ERROR)` | Gives `3`, which is neither ERROR, NULL nor an object | Low. Only set bit 0 when bit 1 is clear |
| `IR_STORE_DST` and `IR_MOVE_TO_DST` to a local store `V` as given (`ceval_macros.h:684-719`) | T, if a handler passes a borrowed operand through to a local | Not shown to occur. Add a debug `PyValue_IsHeapSafe` assert on local destinations |
| `PyValue_FromPyObjectBorrow` and `DUP` of a borrow passed to a value setter | Setters promote them (row 20) | OK in containers; frames are unchecked (previous row) |

## 2. Design critique

### 2.1 Is this the right representation?

Yes, in outline. The best fit for Python is a JSC-style word: doubles stored with an offset, a
48-bit pointer space and value identity for numbers. Python has bignum overflow, refcounting and C
extensions that hold raw pointers. Those rule out V8 pointer compression (a cage, 32-bit fields)
and PyPy's box-everything-then-trace approach.

The comparison the document lacks is **CRuby**. It has bignum overflow like Python, C extensions
see its `VALUE` word, and it chose the other trade-off: 62-bit Fixnums with a low-bit tag, plus
"flonums" that rotate the exponent to hold doubles from about 1e-77 to 1e77 inline and box the
rest. OCaml (63-bit ints) and Chez Scheme are the other fixnum precedents. Section 2 should cite
CRuby, and G5 should treat it as the real alternative to int32.

### 2.2 Per gap

| Gap | Doc says | Recommendation | Evidence | Settling measurement |
|---|---|---|---|---|
| **G2** canonical form | Convention now; checker first; bit-compare `is` "afterwards, measured" | **Go further.** Remove non-canonical construction from the core: `FromPyObjectSteal`, `New` and `Borrow` canonicalise, and no "Raw" variants remain. Make `PyValue_Is` a pure bit compare for all core values. `id()` and `operator.is_` keep `_PyObject_ValueIdentity` for C objects. Make the int and float guards `IsInlineInt` and `IsInlineFloat` only. Do this before baseline: it is a representation rule | `_PyValueTemps_Publish` already loads the type of every pointer result, so canonicalising at construction adds no new cost. Every false `is` on objects (`x is None` as a value, `IS_OP`) and every probe of a dict collision chain calls `_PyValue_IsSameValue`, which loads two types | Debug checker in every home store and in `_PyGC_VisitValue`, run over the suite, should find no violations. A pystats counter of `_PyValue_IsSameValue` true results should read 0 after the change. Microbenchmark: `IS_OP` false path and dict misses on collision chains |
| Release contract (inside G2 and G4) | G4.2: remove per-release exception save/restore, "changes a safety convention" | **Decide before baseline.** Exact int and float boxes have no finaliser, and allocator hooks must not run Python or raise. Then canonicalisation frees the box at once, which deletes the `retired` out-parameter, the `_PyValueTemps` retire path and `close_object`/`close_value` save/restore (`value_ops.c:8-21`), and with them `_PyValue_RestoreException`'s cycle walk | The memory contract (`gc-roots-and-lifetimes.md`) already requires deallocators to preserve the exception. `free` has no error channel. Upstream already releases objects in arbitrary interpreter states | Audit the in-tree allocator hooks (tracemalloc, debug hooks, `_testcapi` hooks) and add debug asserts in the hook wrappers |
| **G3** owned mask | Widen the mask to `Py_TAG_BITS` | Agree. Also fix `_PyValue_PointerOwnsCount` | Row 34, 1.2 item 5 | None |
| **G4** boundary cost | Counters before baseline; contract change; the call ABI goes to native boundary | Agree. Add the **type-slot boundary** (section 2.3), which is wider than vectorcall. Concrete regressions to quote: `lst.append(1000)` allocates and frees a box where upstream INCREFs; `PyDict_GetItemRef` does 3 INCREFs, a canonical borrow, hashing with an exception save, and a box on every inline hit (`dictobject.c:dict_getitemref_owned`). `_PyValue_Hash` saves the exception, INCREFs and closes with a second save/restore for **each** tuple element (`value_ops.c:_PyValue_Hash`) | Code as cited | Per-site box counters; microbenchmarks against upstream for `list.append`, `dict[k]` and `hash(tuple)` |
| **G5** int width | Measure at baseline; prefer int51; decide before the method JIT | **Decide before baseline**, since by the document's own rule representation changes go there. The measurement does not need the linked tree: instrument **upstream** CPython. Default to int51 in `0xFFF8<<48..2^64-1`: the float and pointer regions stay as they are, every test stays one compare, the `BothInlineInt` AND trick holds without the "0xFFFF unused" caveat, hashes equal values, and comparison stays exact. int51 still misses `time.time_ns()` (about 2^60.6), 64-bit masks and hashing (FNV, xorshift64) and `hash()` results. Only a CRuby-style 62-bit fixnum with ranged flonums covers those, and it boxes floats outside about 1e±77 | Scaled int51 add is `shl; shl; add; jo; shr; or`, against int32's `add; jo; or`: about 3 ALU operations under the cost of dispatch. Every int32 overflow becomes a heap `PyLong` that stays boxed in its home, so the loop leaves the specialised path for good | Histogram of int results (BINARY_OP int paths and `PyLong_From*`) by bit width ≤31 / 32-51 / 52-62 / >62, and of float results by exponent (inside or outside about 2^±255), over pyperformance and the in-tree benchmarks. Rule: choose int51 if the 32-51 bucket is material and 52-62 negligible; evaluate the CRuby scheme only if 52-62 is material and out-of-window floats are rare |
| **G6** ownership bit | Assert heap safety; later use bit 2 for borrows; maybe a typed borrow | **Make borrowing static, not a bit.** The register IR already knows statically which operands are borrowed (`IR_IS_LOCAL(F)` in `IR_LOAD_BORROWED`). Rule: no home (frame slot, heap field) ever holds a borrow, and bit 0 in a home means immortal. Borrows live only in C locals, as a distinct C type (`PyValueRef`) that store APIs reject. That removes `MakeHeapSafe` from setters and makes bit 2 unnecessary. If bit 2 is kept anyway, note that `Borrow` must leave a tagged immortal unchanged, or `PyValue_IsNone` breaks on a borrowed `None`. Also merge or rename the `DUP`/`NewRef` and `CLOSE`/`Close` pairs: one name per meaning | 1.2 item 3; `ceval_macros.h:610-640` | Debug assert "tagged implies immortal" in `_PyGC_VisitValue` (heap) and on local stores in `IR_STORE_DST`; list the producers of borrows in homes and drive them to zero |
| **G7** strategies | Reject PyPy strategies; optional V8-style kind word for the JIT | Agree, and **demote to a non-goal** with a trigger to revisit. Inline tagging already gives PyPy's Integer and Float strategies at 8 bytes per element; Python has no holes; `list.sort`'s O(n) pre-scan is negligible beside an O(n log n) sort. A kind word is only trustworthy if `ob_item` is private, and today it is exposed to C (`PySequence_Fast_ITEMS`, `PyList_SET_VALUE`) | Code as cited | Revisit only if a JIT profile shows per-element guards as a material share of list loops |
| **G8** `_PySlotKind` | Rename or remove | Remove it: it is a placeholder abstraction | `pycore_dict.h:305` | None |
| **G9** sentinels | Keep `NULL = 1` | **Challenge.** Upstream chose `NULL = 1` for stack refs, which are never zero-filled. PyValue now lives in every heap object, where zero-fill is the norm (`PyType_GenericAlloc`, `PyObject_Calloc`, extension `tp_alloc`). `NULL = 0` deletes `init_value_slots` and the explicit NULL fills, and makes zeroed memory valid, at the cost of one more compare in `XCLOSE` and the owned test. The encoding currently has three empty-like states (0 invalid, 1 NULL, 2 ERROR) | `typeobject.c:init_value_slots`; `PyValue_CheckValid` asserts `bits != 0` | Microbenchmark `CLOSE` with the extra test, plus a count of allocations of types with value fields |
| **G10** 48-bit check | Fatal per-arena check | Agree | None checked in `FromPyObject*` | None |
| **G11** dead branches | Delete | Agree. Also the `_WIN32` macro forms of `DUP`, `CLOSE` and `Borrow`, which have drifted (`PyValue_Borrow`'s `_WIN32` macro ends in `;`) | `pycore_value.h:505-509, 617-619` | None |

NaN policy: I agree with boxing every NaN. Canonicalising NaN and keeping it inline would make two
distinct NaN objects the same dict key, which CPython programs can observe.

### 2.3 Missing from the document

| Topic | Finding | Evidence | Action |
|---|---|---|---|
| **Stable and limited ABI** | `Misc/stable_abi.toml` and `PC/python3dll.c:680` still export `PyTuple_GetItem`, `PyList_GetItem` and `PyDict_GetItem`, which have no definitions. abi3 extensions get unresolved symbols, and `test_stable_abi_ctypes` and the Windows `python3.dll` link will fail. `Python.h` includes `pyvalue.h` even under `Py_LIMITED_API` | `Lib/test/test_stable_abi_ctypes.py:740` | State the policy (limited API unsupported, or shimmed), guard `pyvalue.h` with `#ifndef Py_LIMITED_API`, and update the manifest |
| **Type-slot boundary** | `nb_*`, `tp_richcompare`, `tp_hash`, `sq_item`, `mp_subscript`, `tp_iternext` and `tp_getattro` are all in object form. Every generic operation on an inline value boxes, and every int above 256 or float returned by a slot is allocated and then freed at publication | §1.10 lists `tp_iternext`, `sq_item` and generic subscripts as a cost, but no gap proposes anything | Add a G item: either value-form slots for the core types (range, list, tuple and dict iterators; list and tuple subscripts) or a measured decision not to have them |
| Observable behaviour changes | `gc.get_referents(container)` omits inline elements (`gcmodule.c:referentsvisit` via `_Py_VISIT_VALUE`); upstream returns them, and memory profilers (pympler, objgraph) rely on that. `sys.getrefcount(x)` reports a fresh box's count. `id()` is no longer an address, so `ctypes.cast(id(x), py_object)` crashes for inline values. `x is y` holds for equal computed numbers | Code as cited | List the expected test-suite differences, so failures are triaged against intended changes |
| Out-of-process tools | `_Py_DebugOffsets` exports no encoding constants. In-tree `_remote_debugging` is aware of PyValue (`asyncio.c:remote_value_object_addr`), but py-spy, austin and memray would misread frames, tuples and dicts | `pycore_debug_offsets.h` | Add `Py_INT_BASE`, `Py_DOUBLE_OFFSET`, `Py_PTR_LIMIT` and the tag mask to the debug offsets |
| Threads | GIL only. Aligned 8-byte stores make tearing a non-issue. The document never says whether free threading is a non-goal or what would change (bit 0 would then also mean deferred, and inline values would help free threading) | `pyvalue.h:10` | One paragraph: non-goal, or the deltas |
| Subinterpreters | Inline values are interpreter-independent, and `PyValue_None` and the other singletons are global statics. Objects immortalised dynamically are per interpreter, so exact-bit tests must use only static immortals (1.2 item 2) | n/a | One sentence |
| marshal and `.pyc` | Checked, no issue. Fresh boxes have refcount 1 and are never `FLAG_REF`ed. `w_ref`'s comment now says the output does not depend on that shortcut, so output is deterministic | `marshal.c:w_ref` | Record it as checked |
| Hash and equality proofs | Hold today (rows 24-25). Any G5 change must keep hash equal to the value and comparison exact; int51 does, a 62-bit scheme needs `_PyHASH_MODULUS` reduction | `value_ops.c` | State these as invariants, not just observations |

## 3. Required edits, in order

1. Update the source line to `271f4e435e5`. Mark G1 fixed and add G1' from section 1.3: the
   public-header `_Py_IsImmortal` problem under `_Py_OPAQUE_PYOBJECT`, one immortality test for
   Steal, New and NewRef, the `StealMortal` empty-tuple case, and the invariant restated as
   "*static* immortals always carry bit 0".
2. Strengthen G2: canonical construction everywhere in the core, no raw constructors, a
   bit-compare `PyValue_Is` and inline-only guards, all before baseline. Move G4 item 2 (the
   release contract) next to it as the change that deletes the `retired` and `_PyValueTemps` retire
   machinery.
3. Move the G5 decision before baseline, with the upstream histogram as its measurement. Add CRuby
   to section 2 and the int51 and CRuby schemes to G5, with the time_ns and 64-bit hash coverage
   gap stated.
4. Rewrite G6 around static borrowing: no borrows in homes, bit 0 in a home means immortal, typed
   C borrows. Fix the "silent leak" bullet. Add the debug asserts on GC traversal and local stores.
5. Add the stable and limited ABI policy, and the manifest bug, to section 1.11.
6. Add a gap for the type-slot boundary, or point to the native-boundary page if it already owns
   slots.
7. Fix G3 to cover `_PyValue_PointerOwnsCount`. Fix §1.9 (use `_Py_VISIT_VALUE` in
   `traverse_slots`; the divergence is harmless only under the heap-safe invariant). Correct §1.5
   on setters.
8. Add a G9 option evaluating `NULL = 0`, with its measurement.
9. Demote G7 and G8 to non-goals or deletions.
10. Add a section on observable behaviour (`gc.get_referents`, `getrefcount`, `id()` not an address,
    `is` on numbers), debug offsets for external tools, and one-line positions on threads and
    subinterpreters.
11. Cite a source for the JSC `BigInt32` history, or remove it.

## Code bugs found

| Bug | Where | Fix |
|---|---|---|
| Public `PyValue_NewRef` calls `_Py_IsImmortal`, which is undefined under `_Py_OPAQUE_PYOBJECT`; `Python.h` includes `pyvalue.h` unconditionally | `Include/pyvalue.h:51`, `Include/refcount.h:125`, `Include/Python.h:81` | Export `PyValue_NewRef` as a function for extensions, keep an inline core variant, and guard `pyvalue.h` |
| Stable-ABI manifest exports three removed functions | `Misc/stable_abi.toml` (`PyTuple_GetItem`, `PyList_GetItem`, `PyDict_GetItem`), `PC/python3dll.c:680` | Remove them, or provide shims |
| `_PyValue_PointerOwnsCount` and `Py_OWNED_MASK` treat `ERROR`/`Wrap` as owned | `pycore_value.h:190, 757` | Test `Py_TAG_BITS` |
| Different immortality tests (flag versus refcount) across constructors | `PyValue_FromPyObjectSteal` versus `_PyValue_FromPyObjectNew`, `PyValue_NewRef` and `PyValue_IsHeapSafe` | One helper |
| `traverse_slots` applies `visit_decref` to references that own no count | `typeobject.c:2993` | `_Py_VISIT_VALUE` |
| `BUILD_TUPLE` uses `StealMortal` on a result that may be the static empty tuple | `bytecodes.c:2332`, `tupleobject.c:552` | Assert `oparg > 0`, or use `Steal` (latent) |
| `PyValue_Borrow(PyValue_ERROR)` yields `3` | `pyvalue.h:31` | Set bit 0 only when bit 1 is clear (low) |
