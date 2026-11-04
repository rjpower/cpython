# Review 1 of `proposals/pyvalue-representation-packet.md`

Reviewed 2026-11-03 against `build/worktrees/pyvalue-rc-universal` at
`e704ab282d9` plus the uncommitted native-boundary P3 edits present at 17:10
PST. Upstream reference: `3a440b65695`. Read-only; no source was changed.

## Verdict

**Revise before implementation.** The target encoding is sound and the step
order is mostly right. Four points would break the build or crash a release
build if implemented as written: F3, F5, F6 and F13. Three inventories are
too small: release helpers (F7), class P (F8) and allocation-side reentry
(F11). Step 5 deletes `ceval.c` revalidation that guards more than free hooks
(F12). Step 8 assumes that no caller of `PyValue_FromPyObjectSteal` uses the
result as an object, and one caller does (F14). None of these changes the
design; each is a correction to a step.

## What was checked and holds

- **Regions.** int51 `[0xFFF8<<48, 2^64)` is disjoint from the non-NaN
  floats (`0x0002<<48` to `0xFFF2<<48`), indices (`0x0001<<48`), pointers
  (below 2^48), NULL (0) and ERROR (2). Every pattern in the int51 region
  decodes to exactly one value.
- **Int arithmetic.** `(int64)(bits << 13) >> 13` and `((u64)s >> 13) | base`
  round-trip. `BothInlineInt` is exact: the AND keeps all 13 top bits set only
  if both operands have them. Scaled add, subtract and multiply (scaled ×
  unscaled) overflow int64 exactly when the result leaves [−2^50, 2^50).
  Negation overflows only at −2^50.
- **Other predicates.** `IsInlineAt` (`cmpw $2, 6(p)`) and `IsInlineFloat`
  keep their form.
- **Hash and compare.** `_PyValue_Hash` equals `long_hash` because
  2^50 < 2^61 − 1, and it equals the float hash for integral values. Every
  int51 converts to a double exactly, so `compare_inline` stays exact.
- **NULL = 0.** `IsObject` rejects 0–3. `IsHeapSafe` without its NULL term
  is correct, because `(0 & 3) != 1`. `IsValid` (`bits > 2`) is unchanged.
  The owned test needs `bits != 0`. Code that gives zero another meaning is
  limited to the three places the proposal names: `pystate.c:702`,
  `typecache.c:16` and `_testcapi/list.c:164,166`.
- **ERROR and indices.** Both encodings are unchanged. ERROR (2) and every
  index fail the owned mask. Index 0 is `0x0001_0000_0000_0000`, never 0.
- **Release contract (I9) for exact boxes.** `_PyFloat_ExactDealloc` and
  `_PyLong_ExactDealloc` either push the box on the freelist or call
  `PyObject_Free`. Neither runs Python or touches the exception.
  - tracemalloc's free hook only removes a trace, and its reftracer returns
    at once for DESTROY (`tracemalloc.c:1405`).
  - The debug hooks can only abort.
  - The `_testcapi/mem.c` free hooks pass through.
  - Some `_testinternalcapi` fixtures violate the contract (F11).
- **Releases outside the contract.** Most class A and B sites, and the
  helpers `type_mutation_close`, `drop_failed_value`, `_PyObjectArray_Free`,
  `_PyValue_CloseStack`, displaced locals, `free_keys_object` and
  `_PyDict_NotifyEvent`, release arbitrary objects, not exact int or float
  boxes. Removing their save and restore rests on the upstream contract:
  tp_dealloc keeps the exception (gh-89373), and finalisers, weakref callbacks
  and dict watchers report errors as unraisable. In this tree,
  `slot_tp_finalize` and `PyObject_ClearWeakRefs` still save and restore. That
  contract justifies dropping exception save and restore. It does not justify
  dropping revalidation, because these releases can run Python (F12).
- **Sweep spot checks.** The mechanical rewrite gave the right result at all
  of these sites:
  - Class A: `_operator.c` 1075, 1082, 1089, 1096 and 1102;
    `itertoolsmodule.c` 243, 549, 556, 563 and 4655; `genericaliasobject.c`
    137 and 685; `codecs.c` 283; `_abc.c` 442.
  - Class B: `typeobject.c` 2097, 3457, 3468, 3484, 3499, 3510, 3522 and
    13393.
  - Class P: `codecs.c` 281, `itertoolsmodule.c` 4637 and 4680,
  `typeobject.c` 5098.
  - Helpers: `dict_xdecref_object`, `close_object`, `marshal_close_object`.

  The exceptions are `dict_close_value` (F6) and the shapes that rules 3 and 6
  do not cover (F8, F9).

## Findings

1. **§1.3, last paragraph — style.** The stated reason for using two tests is
   wrong. `(bits − 1) & Py_OWNED_MASK` does not misclassify index 0
   (`0x0000_FFFF_FFFF_FFFF & mask` is 3). It fails for every owned pointer,
   because `p − 1` has its low bits set. The conclusion, two tests, stands.
   — *Fix the sentence.*
2. **§1.2 — style.** The table lists negative-NaN images only up to
   `FFF7_FFFF_FFFF_FFFF`. They continue through the int51 region and wrap into
   the pointer and index regions, which is why no NaN is ever inline. As
   written, the table implies that the int51 region holds no NaN images.
   — *Say where the remaining NaN images fall.*
3. **§1.4 I5, step 3 — wrong.** `PyValue_CheckValid` asserts
   `PyValue_IsPointerSpace` and, for tag 0, loads the object.
   - Called from `PyValue_CLOSE` and `XCLOSE`, it fires on every close of an
     inline value or an index.
   - After step 4 it dereferences NULL in `XCLOSE(PyValue_NULL)`.
   - The close half of G1' item 6 already exists: `Py_DECREF_MORTAL` asserts
     `!_Py_IsStaticImmortal` in debug builds (`pycore_object.h:412`, `446`).

   — *Check static immortals only in `_PyValueTemps_Publish` and the home
   stores, on tag-0 objects only. Leave `CLOSE`, `XCLOSE` and `CheckValid`'s
   pointer-space assert alone. In step 4, make `CheckValid` return early for
   0.*
4. **Step 4 — missing.** `IR_BORROW_KNOWN(REF, F)` (`ceval_macros.h:609`,
   80 uses in `generated_ir_cases.c.h`) ORs bit 0 into an operand that is
   "known pointer" and read through a local. Pointer space includes NULL, so
   an empty local becomes `1`, which I1 forbids and `PyValue_IsNull` misses.
   — *Add it to step 4: assert `!PyValue_IsNull(REF)` or use
   `_PyValue_BorrowBitAt`. Test an unbound local through comprehension
   inlining (`LOAD_FAST_AND_CLEAR`) and through `del`.*
5. **Step 6 — wrong (build break).** Step 6 "ends by deleting
   `_PyValue_RestoreException`", but the class C, D, E, F and U sites (about
   130) call it until step 7. — *Delete it at the end of step 7. Step 5's text
   ("until step 6 deletes its callers") changes to match.*
6. **§3.2 rule 5 — wrong.** The body of `dict_close_value` is
   `PyValue_XCLOSE` (`dictobject.c:270`), not `CLOSE`. Mapping it to
   `PyValue_CLOSE` adds a debug assert. Because step 4 makes NULL 0 before step
   6 runs, a NULL argument also becomes `Py_DECREF(NULL)` in release builds.
   — *Map each helper to the exact release in its body, and have the script
   derive the mapping from the body.*
7. **§3.1, helper table — missing.** The table lists about 510 helper calls.
   These save/release/restore wrappers are not in it:

   | Helper | Calls |
   |---|---|
   | `typeobject.c:type_mutation_close` | 349 |
   | `_testinternalcapi/dict.c:dict_fixture_close` | 108 |
   | `abstract.c:abstract_close` | 18 |
   | `ceval.c:drop_failed_value` | 17 |
   | `_testclinic.c:owner_fixture_close` | 14 |
   | `typeobject.c:collect_slot_error` | 13 |
   | `object.c:attribute_close` | 11 |
   | `_testinternalcapi/native_scope.c:close_destination` | 9 |
   | `_testinternalcapi/dict.c:dict_public_close_value` | 7 |
   | `call.c:call_pop_value` | 6 |
   | `call.c:call_free_buffer` | 5 |

   Also missing: `clear_value_arguments`, `check_value_bridge_result`,
   `call_transaction_retire`, `capture_cleanup_error`, `_PyObjectArray_Free`,
   `setiter_dealloc`, `type_dealloc_common`, `_PyType_Modified_Unlocked` and
   `compile.c:dict_lookup_arg`. That is about 560 more calls, so the total is
   near 1,070. — *Have the script find helpers by the shape of their bodies,
   print the list, and size step 6 from that list.*
8. **§3.1, class P — missing.**
   - `X == NULL || PyErr_Occurred()` accounts for 371 of the 783
     `|| PyErr_Occurred()` terms. The rest include `status < 0 ||
     PyErr_Occurred()` (168), other `||` forms such as `_operator.c:2492`, and
     standalone `if (PyErr_Occurred())` tests after a release or store
     (`itertoolsmodule.c` 4670, 4684; `typeobject.c` 3463, 3505;
     `genericaliasobject.c` 691).
   - Per file, the tree has 5 to 100 times upstream's count of
     `PyErr_Occurred()`: `typeobject.c` 488 against 49, `_operator.c` 98
     against 0, `genericaliasobject.c` 128 against 1.
   - For the 37 `Py_NewRef` producers, the rewrite leaves a dead
     `X == NULL` branch.

   — *Widen P to any `<failure test> || PyErr_Occurred()` that follows a
   call whose failure the left term already reports. List the standalone
   tests for step 7. Delete the `X == NULL` branches after
   `Py_NewRef`/`Py_XNewRef`. Record the per-file `PyErr_Occurred()` count
   against upstream as the measure of what remains.*
9. **§3.2 rule 3 — style.** Most class A and B sites detach before the save
   (`typeobject.c:3457`, `genericaliasobject.c:137`, `slot_tp_finalize`).
   Rule 3 folds a detach only between the save and the restore, so these sites
   keep a three-line detach inside a bare `{ }` block. — *Also fold a detach
   that immediately precedes the save. Unwrap a bare block that becomes one
   statement. Drop the stale `// _PyValue_RestoreException()` include
   comments.*
10. **Step 5, I9 — wrong.** The gh-89373 check is in `py_dealloc` only.
    Interpreter frees of scalar boxes bypass it:
    `PyValue_CLOSE_SPECIALIZED(…, _PyFloat_ExactDealloc / _PyLong_ExactDealloc)`
    (51 uses in `generated_ir_cases.c.h`) and ceval's `Py_DECREF` override
    (`Python/ceval.h:76–105`) call tp_dealloc directly. An assert in "the
    debug allocator hooks" sees only frees below those hooks, and a hook
    installed later with `PyMem_SetAllocator` sits above them. —
    *Put the debug check (exception unchanged, no Python frame entered) in
    `PyObject_Free`, `PyMem_Free` and `PyMem_RawFree` (`obmalloc.c` 1356,
    1809) and in `_PyReftracerTrack`. Name reftracers (`PyRefTracer_SetTracer`)
    in the contract; upstream's documentation already forbids them from
    raising or creating objects.*
11. **Step 5 — missing.** D2 says allocator hooks never run Python, and that
    applies to malloc as well as free. These fixtures test the old
    "allocation may reenter" model:
    - `_testinternalcapi/dict.c:dict_reenter_malloc` runs `PyDict_Clear`
      inside an allocation (`dict_value_pop_boxing_aba`).
    - `tuple.c:tuple_reentry_*` (`tuple_value_boxing_reentry`).
    - `native_scope.c:vector_change_*` and `end_free`.
    - `set.c:set_cleanup_free` calls `PyErr_FormatUnraisable`, which can run
      a Python `sys.unraisablehook`.

    The defensive code they exercise (revalidation after boxing, ABA checks)
    is not in the inventory. — *State the malloc half of the contract. List
    these fixtures for deletion in step 5. Inventory the revalidation that
    follows boxing, and remove it in step 7 or keep it with a named reason.*
12. **§3.3 F, step 5 — risk.** `ceval.c:value_operation_refresh` (line 1603)
    revalidates frame, code and executing copy after a bridge operation and
    after `drop_failed_value`. `drop_failed_value` releases arbitrary results,
    and the native call before it can run any Python. The comment says "free
    hooks", but the check guards more than that. — *Classify each of the 17
    `ceval.c` sites by what precedes it. Delete the check only where the only
    work in between is a scalar-box free. After native calls and non-scalar
    releases, keep it or replace it with an upstream-style check of the
    instrumentation version at dispatch. Tests: a `__del__` triggered by
    rebinding a local that calls `sys.monitoring.set_events` or
    `sys.settrace`; a native call that does the same.*
13. **Step 8 — wrong.** `Tools/cases_generator/generators_common.py`'s
    `POINTER_CONSTRUCTORS` treats `PyValue_FromPyObjectSteal`, `New` and
    `Borrow` as always returning pointer space. For their results the
    generator emits `_PyValue_CLOSE_POINTER`, `_DUP_POINTER` and
    `_AsPyObjectStealPointer`, which test only `Py_TAG_BITS`. Once these
    constructors canonicalise, a release build decrefs an inline int whose low
    two bits are `00` as if it were a pointer. — *Step 8 removes them from
    `POINTER_CONSTRUCTORS`, regenerates, and checks generated code for
    `_POINTER` operations on their results. The JIT needs the same audit.*
14. **Step 8 — missing.** "The call sites keep their names" assumes that no
    caller uses the result as an object. `typeobject.c:3453` says otherwise:
    "Successful method/self outputs retain owned object views for native
    consumers, including WITH_EXCEPT_START; do not canonicalize them here."
    The code then calls `PyValue_AsPyObjectBorrow(*method)`. — *Audit the
    224 call sites for a later `AsPyObjectBorrow`/`Steal`, `_POINTER`
    operation or `Py_TYPE` on the result. Those sites keep a `PyObject *`
    in a C local and convert it at the home store. Tests: `(5).bit_length()`,
    `(2.5).hex()`, and `with` on an object whose `__enter__` returns an int.*
15. **Step 2 — wrong (against G10).** This tree uses mimalloc for the raw,
    mem and obj domains (`obmalloc.c:500–507`). A check in
    `_PyObject_MiMalloc`/`_PyMem_MiMalloc` therefore costs a compare per
    object. G10 accepted one compare per arena, and step 2's own cost line
    says "per large allocation". — *Check where memory comes from the OS:
    `Objects/mimalloc/prim/unix/prim.c:_mi_prim_alloc` (one line in vendored
    code, per segment), `obmalloc.c:new_arena`, and `_PyMem_RawMalloc`,
    `_PyMem_RawCalloc` and `_PyMem_RawRealloc` (for `PYTHONMALLOC=malloc`).
    Keep the debug asserts in the constructors.*
16. **Step 9a — risk.** The `len` path (`bytecodes.c:4729`) and
    `FOR_ITER_RANGE` (line 3901) "encode int64 directly", but
    `len(range(2**55))` and ranges that cross 2^50 leave int51. — *Use the
    range-checked `PyValue_FromInt64` at both; test the boundaries.*
17. **Step 9a — missing.** These tests pin the int32 encoding and are not
    listed:
    - `Lib/test/test_capi/test_list.py:35` hard-codes
      `(0xfffe << 48) | (value & 0xffffffff)`.
    - The comment at `test_tagged_values.py:52` ("would alias the inline
      int 1") is about the int32 encoding.

    — *Add both to the step 9a list.*
18. **Steps 9a/9b — risk.** Step 9a disables int traces.
    `Lib/test/test_jit_method.py` (1,148 lines) asserts
    `jit_method_compile(...)` and `*_seen` witnesses; any case over int locals
    fails between 9a and 9b. — *In step 0, find the cases that depend on int
    traces. Land 9a and 9b as one validation point (two commits, one suite
    record), unless no JIT test depends on int traces.*
19. **Step 9a — missing tests.**
    - Overflow at the boundaries: `abs(-2**50)`, `-(-2**50)`,
      `-2**50 // -1`, `divmod(-2**50, -1)`, `(2**50-1) + 1`, `1 << 50`,
      `-1 << 50`, `(2**50-1) << 1`, `~(-2**50)`.
    - List and tuple indexing with int51 values: an `IndexError`, not a
      truncated index.
    - Round trips at the boundaries: `struct.pack('q')`, `array('q')`,
      `ctypes.c_int64`.
    - Hash and equality: `hash(2**50-1) == hash(float(2**50-1))`, and dicts
      and sets that mix `2**40` with `2.0**40`.
    - Constants and memo keys: int51 constants in `co_consts` through
      `marshal` and `.pyc` determinism (`test_py_compile`,
      `test_compileall`); `copy.deepcopy` memo keys, since `id()` results
      are now inline.
    - Unchanged behaviour: `sys.getsizeof(2**40)` and `(2**40).bit_length()`.
20. **Step 6 — risk.** Step 6 makes about 686 direct, 1,070 helper and 371 to
    780 class P edits across about 60 files, as one commit. One commit is too
    large to review. — *Split it into 6a (class A and B direct sites), 6b
    (helpers) and 6c (class P). Each is a commit of per-file batches that
    builds and passes. Commit or attach the script and its list of refused
    sites.*
21. **Step 5 — risk (churn).** Changing `PrepareOwned` (32 sites) removes the
    `retired_*` variables, and with them class A blocks such as
    `itertoolsmodule.c:238–247`. — *Say that step 5 deletes those blocks,
    and recount the classes after step 5.*
22. **Step 4 — risk.** In release builds, `PyValue_CLOSE(NULL)`,
    `_PyValue_CLOSE_POINTER(NULL)` and `DUP(NULL)` were no-ops when NULL was
    1. With NULL = 0 they call `Py_DECREF(NULL)`. Only the existing debug
    asserts detect such a call, and only on paths the suite runs. — *After
    step 4, run the full debug suite, not only the step-0 pass set.*
23. **§1.4 I4, steps 3 and 9a — risk.** `_PyValue_CheckHome` uses the int32
    range until 9a and int51 after it. If it reads a constant of its own, it
    will drift from the constructors. — *Derive both from one helper,
    `_PyValue_IntFitsInline(int64_t)`.*
24. **§4 — missing.** There is no record after step 7. Steps 5–7 (release
    contract) and step 8 (canonical construction) affect different
    benchmarks: `hash_tuple`, `dict_subscript` and native calls on one side,
    `is_op_false` and `dict_miss_chain` on the other. — *Also record after
    step 7, including code size. Add a `build_tuple_small` micro for the
    open choice below.*
25. **Step 1 — style.** "3,294 lines including the file below" contradicts
    leaving `gc_free_threading.c` (2,904 lines) in place. — *Give the count
    without that file.*

## Open choice: `_PyValue_FromNonNumberSteal` or the canonical constructor

**Recommendation: delete `StealMortal` and `NewMortal`, use the canonical
`PyValue_FromPyObjectSteal` at all 16 sites, and add no non-number variant.**

- One operation per meaning. A precondition-checked twin is a second
  constructor that release builds cannot check, and misuse is silent: `is`
  gives a false negative.
- The owner's rule forbids micro-optimising within a step. The cost is one
  type load, from the same object header that `_Py_IsImmortal` already
  reads, and two compares, all in handlers that have just allocated. Record
  it with `build_tuple_small`; do not optimise it.
- The generator loses its pointer proof for these results (finding 13). After
  D5 and G3, `CLOSE` and `CLOSE_POINTER` are each one test, so the loss is
  negligible.
- Revisit only if the post-baseline measurement shows the type test.

## Required changes, in order

1. Step 3 / I5: check static immortals only on tag-0 objects at publish and
   home stores; do not call `CheckValid` from `CLOSE` or `XCLOSE` (F3).
2. Step 4: add `IR_BORROW_KNOWN`, make `CheckValid` accept 0, and run the full
   debug suite after the step (F3, F4, F22).
3. Step 2: put the 48-bit check where memory comes from the OS, not in each
   mimalloc wrapper (F15).
4. Step 5: check the contract in the `PyObject_Free`, `PyMem_Free` and
   `PyMem_RawFree` entry points and in the reftracer; state the malloc half;
   list the fixtures to delete; say that `PrepareOwned` call sites lose their
   retired blocks (F10, F11, F21).
5. Steps 5 and 7: classify the 17 `ceval.c` revalidation sites by what
   precedes them; keep revalidation after native calls and arbitrary
   releases (F12).
6. Step 6: find helpers by the shape of their bodies (about 1,070 calls); map
   each helper to its exact release (`dict_close_value` → `XCLOSE`); widen
   class P; fold detaches that precede the save; split the step into 6a, 6b
   and 6c (F6–F9, F20).
7. Delete `_PyValue_RestoreException` at the end of step 7, not step 6 (F5).
8. Step 8: remove the canonicalising constructors from `POINTER_CONSTRUCTORS`;
   audit Steal, New and Borrow results that are used as objects
   (`typeobject.c:3453`); use the canonical Steal at all 16 `StealMortal` sites
   (F13, F14, open choice).
9. Step 9a: range-check `len` and `FOR_ITER_RANGE`; add the missing test pins
   and boundary tests; derive the home checker and the constructors from one
   range helper (F16, F17, F19, F23).
10. Steps 9a and 9b: validate them together unless no JIT test depends on int
    traces (F18).
11. §4: add a record after step 7 and the `build_tuple_small` micro (F24).
12. Text fixes: §1.3 reason, §1.2 NaN rows, step 1 line count (F1, F2, F25).
