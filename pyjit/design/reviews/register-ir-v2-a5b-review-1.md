# Review 1: register IR v2 step A5b, the typed borrow (proposal)

Reviewed: [register-ir-v2-a5b.md](../proposals/register-ir-v2-a5b.md) (622
lines, uncommitted, 2026-11-04) and its `tally.py`. Code: runtime tree
`build/worktrees/pyvalue-rc-universal` at `5de7bd62ef9`, clean, read with
`git grep` and `sed`. Upstream control: `3a440b6`, read with `git show`.
Compilers: GCC 15.2.0 and Clang 21.1.8. Nothing was built or timed. The
probes were small C files compiled with `-fsyntax-only` or `-O2 -S` in one
scratch directory, now removed. The units that matter are quoted below.

"Proposal:N" is a line of the proposal. Other paths are relative to the
runtime tree.

## Verdict

**Revise.** The direction is right, and most of the proposal checks out: the
counts, the class split of the 150 `MakeHeapSafe` uses, the step order, and
the view and owner pair for a field with two ownership states. The
`transparent_union` probe reproduces. But the compiler does not enforce the
central claim, that storing or closing a borrow fails to compile, as fully as
the proposal says, and the reader type adds a third form that the compiler
cannot hold to its meaning:

- On Clang 21, every pointer-typed misuse of a borrow is only a warning. That
  includes the proposal's own negative unit `IR_STORE_DST(F, view)` (M1).
- `PyValueRead` lets a reader turn a borrow into an owner (`v.owned`). Its
  function types silently accept a closer (M2).
- L1 conflicts with A3a's activation record (M3).
- The type does not cover a `PyValue` copied out of a home, and it says
  nothing about how long a borrow lives. The proposal's summary claims more
  than the design delivers (M4).

Each must-fix has a small, concrete remedy. With them, and the should-fix
items folded in, I would expect to accept revision 2.

## Findings

### Must-fix

#### M1. On Clang, pointer-typed misuses of a borrow are warnings, not errors

Probe (`holes.c`): the two distinct one-word structs of proposal:59–60
(two typedefs), with the real `PyValue_CLEAR` and `IR_STORE_DST` shapes
(`ceval_macros.h:719`: `IR_SLOT(F) = _PyValue_Publish(&(V))`).

| Misuse | GCC 15.2 | Clang 21.1 |
|---|---|---|
| `slot = b` (a borrow stored to a `PyValue`) | error | error |
| `PyValue_CLOSE(b)` | error | error |
| `PyValue *p = &b;` | error | **warning** |
| `PyValue_CLEAR(b)` (the macro takes `&(REF)`) | error | **warning** |
| `IR_STORE_DST(F, b)` (the proposal's negative unit, §5.5) | error | **warning** |
| `scope->arguments = &scope->argument` (`PyValue *` ← `PyValueBorrow *`) | error | **warning** |
| `lookup(&slots[0])` for `_Py_dict_lookup(…, PyValueBorrow *)` | error | **warning** |
| `*(PyValue *)&b`, `memcpy` | silent | silent |

GCC 14 made `-Wincompatible-pointer-types` an error by default. Clang did
not. `configure.ac` adds `-Werror=implicit-function-declaration`
(`configure.ac:2975`, `:2989`) but not `-Werror=incompatible-pointer-types`.
`check_value_api.py`'s compile probe uses the one compiler in the compile
database (`self_test`, lines 457–505), so a GCC-built tree never sees how
Clang behaves. A3a qualifies on both compilers.

The hole is concrete. `_PyNativeScope` points `PyValue *arguments` at its
own `argument` field (`native_scope.c:22`), and `_PyNativeScope_End` calls
`PyValue_CLEAR(scope->arguments[i])` on it (`native_scope.c:295`). Once
`argument` is a `PyValueBorrow` holding exact bits (§5.1 P8, B1), Clang
compiles the assignment with a warning, and End then releases a count that
was never taken.

**Change.**
- Add `-Werror=incompatible-pointer-types` to `CFLAGS_NODIST` for core units,
  and `-Werror=incompatible-function-pointer-types` for Clang before 16.
- Run the §5.5 negative units under both compilers, and match each one's
  expected diagnostic, as the `Py_VALUE_COMPAT` probe matches "unavailable".
  Otherwise a unit that fails for an unrelated reason, a typo say, passes.
- Extend `raw-bits` to casts to `PyValue *` and `PyValueBorrow *`, outside an
  allowed list (`structmember.c`, the `STORE_ATTR_*` offset stores).

#### M2. `PyValueRead` is a third type, with two holes the compiler does not close

Proposal:113–134 makes readers take a transparent union of the owner and the
borrow types.

1. **A reader can turn a borrow into an owner.**
   `PyValue leak(PyValueRead v) { return v.owned; }` compiles silently on
   both compilers. "`_PyValue_ReadBits(v)` is the only spelling"
   (proposal:127–128) is a convention, and no lint rule in §5.5 enforces it.
2. **Function types with a transparent-union parameter are compatible with
   closers.** `void (*reader_fp)(PyValueRead) = PyValue_CLOSE;` and then
   `reader_fp(b)` both compile silently on GCC 15 and Clang 21. The same
   assignment to `void (*)(PyValueBorrow)` is an error on both. At HEAD,
   five function-pointer types take a `PyValue` by value: the public
   `PyDict_WatchCallback` (`Include/cpython/dictobject.h:93`, whose
   arguments are borrows), `check_lookup`, `compare_func`, `key_compare` and
   `tuple_elem_compare`. Value-api V2a's value slots will add many more.

The principled reader type is the borrow itself. value-api.md §1.4 already
says so: "argument positions take that type and results stay `PyValue`".
What C lacks is an implicit owner-to-borrow conversion at the call. A
same-name generic macro supplies that conversion without a third type:

```c
static inline int PyValue_IsInlineInt(PyValueBorrow v) { return v.bits >= Py_INT_BASE; }
#define PyValue_IsInlineInt(V) (PyValue_IsInlineInt)(PyValue_View(V))
/* C: _Generic over PyValue and PyValueBorrow; C++: two inline overloads */
```

Probe (`gen.c`): with this form, both compilers accept owners, borrows,
`*p` and `slot[i]` unchanged. They reject a borrow stored to an owner, a
borrow passed to `CLOSE`, a `uintptr_t` passed to a reader, and a closer
assigned to a reader pointer. `-O2` gives the same instructions as a plain
`PyValue` function. The same header compiles as C++11 under `g++` and
`clang++-21`. Taking `&PyValue_IsInlineInt` yields the borrow-only function.

**Change.** Readers take `PyValueBorrow`. Each reader that owner call sites
reach gets the one-line same-name macro (about 100 header readers; static
readers in `.c` files can take `PyValue_View(x)` at their call sites). Then
`PyValue_Borrow` and `PyValue_View` are one function (item 6, e). Call-site churn
stays at zero, as with the union. If the coordinator keeps the union
anyway:
- lint `.owned` outside `pytypedefs.h`;
- forbid `PyValueRead` in function-pointer types;
- type callback parameters as `PyValueBorrow`.

#### M3. L1 contradicts A3a's activation record, and the two steps do overlap

L1 (proposal:76–81) allows `PyValueBorrow` fields in C-stack records and
names `_PyNativeScope.argument`. Packet A3a puts `_PyNativeScope` in the
activation record's scratch union ("the largest is `CALL_BUILTIN_O`
(`_PyNativeScope` 152 B …)"). Its rules say "The record holds owned values
only … Under D4 it is a home." Both cannot hold.

Proposal:537–539 says "Neither step's diff overlaps the other's", but it
does. A3a's table moves `held_dict`, `held_sub`, `held_value`,
`held_container`, `release_value` and `retained_dict` into the scratch.
Those are the class-I lines that A5b rewrites (`bytecodes.c:1237–1238`,
`1321–1323`, `1374–1376`, `3082`).

**Change.** Decide now, in both documents. I recommend this: the activation
record is a C-stack record whose handler fields are dead at dispatch and
never scanned, so it is not a home, and it may hold `PyValueBorrow` fields.
Amend A3a's ownership rule to say so, and let `borrow-field` accept the
generated scratch union. The alternative, an owned `argument`, costs a
count pair on every `CALL_BUILTIN_O`. Correct §8's overlap sentence. A3a
rebases on A5b's `TAKE` lines.

#### M4. The guarantee is stated too strongly: copies of homes, and lifetimes

The type proves one thing: a value from a named borrow producer cannot be
stored, closed or stolen. It does not prove that a `PyValue` local owns a
count, or that a borrow's owner is still alive.

- **Copies of homes.** R3 (proposal:558) concedes the gap and defers it to
  V4. Its size: 80 declarations of the form `PyValue x = <field, element or
  *pointer>;` in 22 hand-written files (`dictobject.c` 14, `listobject.c` 13,
  `bytecodes.c` 9, `ceval.c` 7, `tupleobject.c` 6, …). Some are moves; the
  rest are untyped borrows. The hottest handler has one:
  `_LOAD_ATTR_INSTANCE_VALUE` reads `PyValue found = *value_ptr;` and then
  `attr = PyValue_NewRef(found);` (`bytecodes.c:2728–2730`). After A5b,
  writing `attr = found;` there still compiles, and it stores an uncounted
  alias. That is the D4 hazard itself.
- **Lifetime.** Nothing checks that a view of a local is dead before an
  escaping call that could rebind the local (S7).

**Change.**
1. §0, §2.2 and the §11 edits state the guarantee exactly, as above.
2. A5b-3 takes a census of the 80 declarations. It retypes the non-moves as
   `PyValueBorrow x = PyValue_BorrowAt(&home)`. It adds a `home-copy` lint
   rule that reports a `PyValue` local initialised from a home lvalue with
   no later store to that home (a store would make it a move); the rule
   reports and does not gate.
3. G6 records the full closure as an open decision: a field type, as HPy
   uses for `HPyField` (item 8).

### Should-fix

#### S1. `IR_CONSUME_INPUT` must not re-read the slot, and proven-inline inputs need no owner

Proposal:220 defines the owner as `_PyValue_TakeSlot(&IR_SLOT(F))`, which
reads the slot again. After another input's NULL store, which may alias,
the compiler loses what it knew about the value. Model (`handlers.c`, below;
static instructions per function, all paths):

| Handler model | HEAD (A5a) | A5b as proposed | A5b, owner from the loaded word | A5b, receiver as the input's owner (S2) |
|---|---:|---:|---:|---:|
| `BINARY_OP_ADD_INT`, GCC 15 | 76 | 109 | **76** | – |
| `BINARY_OP_ADD_INT`, Clang 21 | 75 | 121 | 105 | – |
| `LOAD_ATTR_INSTANCE_VALUE`, GCC | 149 | 113 | 113 | **84** |
| `LOAD_ATTR_INSTANCE_VALUE`, Clang | 147 | 111 | 108 | **102** |
| `STORE_ATTR_INSTANCE_VALUE`, GCC | 168 | 165 | 169 | **115** |
| `STORE_ATTR_INSTANCE_VALUE`, Clang | 158 | 105 | 102 | **96** |

The attribute handlers shrink mainly because `XDUP` replaces `NewRef` and
`MakeHeapSafe`. On the add, HEAD's compilers prove both releases dead: a
guard showed the value is an inline int. With the owner taken from the
loaded word, GCC still proves it. Clang keeps one `XCLOSE` test sequence of
six instructions on the fast path, because the fact does not survive the
`is_local ? NULL : x` select.

**Change.** Define the owner from the word the handler already loaded:
`ir_own_F = IR_IS_LOCAL(F) ? PyValue_NULL : ir_word_F`, plus the NULL store.
Here `ir_word_F` is a generated `PyValue` local that the DSL body never
sees, and `x = PyValue_Borrow(ir_word_F)`. For an input that a guard has
proved inline, the generator emits neither an owner nor a release; it
already chooses `_POINTER` variants from facts of this kind. Then the cost
matches A5a by construction, not by the compiler's range propagation.

#### S2. `TAKE` merges two meanings; one of them is free under A5b

Of the 40 class-I sites, 13 are `PyValue receiver = PyValue_MakeHeapSafe(owner)`.
They do not keep the receiver in a home: they hold the input's own owner so
that it can be released after the result is published
(`IR_RETIRE_PUBLISHED`). For a local, the generated code takes and drops a
count on every `self.x`. The receiver is `IR_INPLACE_HEAP_SAFE`, so
`retire_published`'s "A local's value is not the handler's to retire" branch
(`ir_generator.py:1904–1909`) never fires. The proposal maps both meanings
to `IR_TAKE`, and keeps the pair.

The owner local `ir_own_F` already is "the input's owner, NULL for a local".
**Change.** Add a second DSL form for this meaning (for example
`OWNER(x)`), lowered to `ir_own_F`. Allow it only where no code can run
between the read and the retire; the generator has `runs_code`. Keep `TAKE`
for a value that goes to a home. In the model this removes an incref and a
decref from the local path: 113 → 84 instructions (GCC) on
`LOAD_ATTR_INSTANCE_VALUE`, and 169 → 115 on `STORE_ATTR_INSTANCE_VALUE`.

#### S3. The fail-closed object rule cannot work as written

Proposal:320–326 allows four uses of a followed `PyObject *`: an argument of
a known reader, a local copy, a comparison, and a `Py_TYPE` or field read.
In `bytecodes.c`:

- 128 uop bodies derive an object pointer from a value;
- about 85 distinct callees (macros included) receive one;
- 8 sites compute a field address with `(char *)owner_o + offset`, in the
  hottest attribute handlers;
- about 200 uses take other forms (casts, assignments and the like).

As written, the rule would take the owner of `LOAD_ATTR_INSTANCE_VALUE`
and `STORE_ATTR_*` out of `borrow_mask`. Each `self.x` would then pay for a
separate `LOAD_FAST_BORROW` dispatch and a count pair. "Expected: few" is
unmeasured.

**Change.**
- Allow casts and field-address arithmetic on the followed pointer, as views
  of the same object.
- Make "known reader" a checked list: the `*_Check*` family, `Py_TYPE`,
  `LOCK_OBJECT`/`UNLOCK_OBJECT`, `*_GET_SIZE`, `_PyObject_InlineValues`,
  `_PyObject_GetManagedDict`, and so on.
- Report the change in `borrow_mask` per instruction, and the kernels'
  dispatch counts, before landing.
- If the list proves too long, keep A5a's deny-list but complete it. Today it
  misses stealing APIs without `Steal`, `Take` or `_SET_ITEM` in their names:
  `PyException_SetCause`, `PyException_SetContext`,
  `PyErr_SetRaisedException`, `_PyErr_SetRaisedException`,
  `_PyErr_ChainExceptions1`, `PyCell_SET` and `PyModule_AddObject`.

I agree that a `PyObjectBorrow` type is not worth it (item 3).

#### S4. The type cache needs its own type, not an exception to L1

`struct type_cache_entry.value` (`pycore_typecache.h:19`) is a non-owning
value. It is valid only while the type's version tag matches, and it is
read only after that check (`typecache.c:209–210`). That makes it a weak
reference guarded by a version, which is a different meaning from a C-local
borrow. If `PyValueBorrow` admits it, the meaning of "borrow" weakens for
every reader of the type. Upstream's type cache likewise stores a borrowed
`PyObject *` value. An owned value would release entries during
`PyType_Modified`, which can run code, so I agree not to make it owned.

**Change.** A private `_PyValueGuarded { uintptr_t bits; }` in
`pycore_typecache.h`, with two operations: build it from a borrow at insert,
and read it as a `PyValueBorrow` after the version check. Then L1 has no
heap exception, and `borrow-field` needs no special case.

#### S5. Gaps in the producer list and the tables

- **Missing producer:** `_PyValue_FromObjectBorrowInline`
  (`pycore_value_api.h:90`). It has the same meaning as `FromObjectBorrow`,
  so merge the two (§6). Make `PyValue_FromObjectBorrow` inline in the core
  and keep the exported symbol for modules. Do this before rewriting
  `FromPyObjectNew(o)` as `DUP(FromObjectBorrow(o))`. Otherwise 53 inline
  sites, 18 of them generated, gain an out-of-line call, and value-api.md's
  "costs the same" is false.
- **Missing producer:** the parameters of `PyDict_WatchCallback`
  (`dictobject.h:89–94`, "borrow strong snapshots"). Type them as
  `PyValueBorrow`.
- **Counts that move to callers:** deleting class S moves counts to the
  callers that pass a borrow to a stealing setter today:
  `templateobject.c:286`, `:290` and `typeobject.c:8836`
  (`PyTuple_SET_VALUE(…, PyTuple_GET_VALUE(…))`). The split counts sites, not
  the `DUP`s that A5b adds.
- **Misclassed site:** `native_scope.c:55` is not a setter. It is the
  dual-state `argument` field (owned or borrowed by run-time tag).
- **Stale bit-0 test:** `_PyValue_Publish` still reads bit 0 as "a borrow"
  (`pycore_value.h:79`).
- **Identity after A5b:** `PyValue_AsStrongReference` (4 uses) becomes the
  identity in the GIL build. Delete it.
- **Wrong signature:** §3.2 lists `_PyObject_ValueIdentity` as a
  `PyValueRead` reader, but it takes a `PyObject *` (`pycore_value.h:377`).
- **Release-build check:** the release-mode check
  `native_scope.c:93–97` ("native vector arguments must be strong values")
  disappears with `IsHeapSafe`. Say so, since the A5a notes cite it as the
  release build's fail-closed evidence.

#### S6. H1 does not hold during finalization

H1 ("tagged implies immortal") becomes an assert in `PyValue_CheckValid`,
`_PyGC_VisitValue` and the home check. But finalization makes immortals
mortal again:

- `_PyUnicode_ClearInterned`: `_Py_SetMortal(s, 2)` (`unicodeobject.c:15111`);
- `_Py_ClearImmortal`: `typeobject.c:698` and `:760`, and `pystate.c:717`.

Tagged references in surviving homes then violate H1. **Change.** Either
exempt the asserts once finalization has begun, or show that no home
survives. Tagging only static immortals (item 5) makes H1 true for an
object's whole life.

#### S7. Nothing checks how long a borrow lives

B1 says a borrow "holds the exact bits of a live owner". The type checks
storage, not liveness. A view of a local that is read after an escaping call
dangles if the call rebinds the local (through `f_locals`, or `del` from a
trace function). The generator has no rule against this; the A5a in-place
rules (`ir_generator.py:520–760`) do not mention escapes. Upstream 3.14
checks it in its debug build: under `Py_STACKREF_DEBUG`,
`_Py_stackref_close` fails with "StackRef … closed with %d borrowed refs"
(`Python/stackrefs.c:117–120` at `3a440b6`).

**Change.** The generator refuses a use of a view of a local after an
escaping call in the same instruction, unless the value was taken first. It
already knows each escape (`escaping_calls`, `runs_code`). Record a debug
borrow ledger as an open question.

#### S8. Generator details the proposal does not cover

- **One C type per variable.** The generator declares handler-scope
  variables shared across uops. In `STORE_ATTR_INSTANCE_VALUE`, `value`
  holds field A in `_LOCK_OBJECT` and field B in
  `_STORE_ATTR_INSTANCE_VALUE`. `IR_INPLACE_OWN` turns a variable from a
  borrow into an owner in place (11 sites). Under A5b each C variable has
  one type, so the generator must split variables by role. Pass-through
  outputs (`res = x`) become `TAKE` in the DSL. The C compiler finds these
  cases, but the work is not in the estimate.
- **Live values.** "The number of live values is unchanged" (proposal:312)
  is not exact. The view and the owner are both live between the consume and
  the view's last read. Measure register pressure under A3a's GCC convention
  (six argument registers, five taken).

#### S9. The estimate is credible as a total, but not by step

A5a was estimated at 5 h. It took about 2.7 h: `progress-irv2-a5a.txt`
runs 06:49–08:49, with post-review work 09:11–09:32. So 14.5 h is a
plausible budget for the proposal as written, but steps 2 and 3 are light:

- step 2 omits the variable splitting (S8) and the reader list (S3);
- step 3 has the compile-driven signature work on two compilers (M1), the
  finalization question (S6) and the first runs of the new asserts.

With the changes above I expect about 18–20 h:

| Change | Extra |
|---|---:|
| M1 | 0.5 h |
| M2 | about 1 h |
| M3 | 0.5 h |
| M4 census | 1.5 h |
| S1 and S2 | 1 h |
| S3 | 1–2 h |
| S7 | 1 h |

### Notes

- **N1.** Keep the bit-0 mask in `PyValue_Is`. I agree with §6 (item 5).
- **N2.** The close family grows to eight names: `CLOSE`, `XCLOSE`,
  `_SPECIALIZED`, `_POINTER`, and their X and combined forms. They are
  variants by precondition, not by meaning. A later step could use one
  `CLOSE` after `__attribute__((assume(…)))` (GCC 13+) or `__builtin_assume`.
- **N3.** value-api.md §1.5 rule 3's `…Borrowed` suffix now repeats what the
  return type says. Keep one marker, the type, and drop the suffix in V4's
  renames (`_PyDict_NextValueBorrowed`).
- **N4.** `PyValue_Wrap` stores a bare non-object pointer in a `PyValue` in
  release builds (`pycore_value.h:388–398`). That is a third meaning in the
  owner type. It is outside A5b's scope; list it in G6.
- **N5.** The corrections to §9 are under item 8.
- **N6.** Proposal:132–133 says "no C++ file includes the internal headers".
  `test_cext`'s `TestInternalCpp` (`Lib/test/test_cext/__init__.py:189`)
  does: it compiles `internal/pycore_frame.h` and
  `pycore_interpframe_structs.h` (`extension.c:12–27`) as C++ with
  `-Werror`. Today those headers reach only type declarations
  (`pycore_structs.h`, `pycore_typedefs.h`), and no inline readers. So the
  consequence is small: the declarations of `PyValueBorrow` (and of
  `PyValueRead`, if kept) must be valid C++ there too. That test pins it.
  The probe shows the proposed C++ struct passes arguments exactly as the C
  union does: identical caller code on GCC and Clang. M2's form needs no
  second spelling.

## Answers to the questions

### 1. The type pair and `PyValueRead`

The pair is right. `PyValueRead` is a dual form, for the reasons in M2: a
third type whose `owned` member and function-type compatibility let a borrow
become an owner without a count.

| Form | Safety | Call-site churn | ABI | C++ | Compilers |
|---|---|---|---|---|---|
| Transparent union (proposal) | Stores and closes rejected; holes M2.1 and M2.2 | none; about 236 signatures | same; probe | needs a second spelling (struct with constructors); same calling sequence (probe), and its declarations reach a C++ test of internal headers (N6) | GCC and Clang only |
| `PyValueBorrow`-only readers, explicit `PyValue_View(v)` at owner sites | strongest; a reader pointer cannot hold a closer | about 1,300 hand-written arguments (below) | same | trivial | any C11 compiler |
| `PyValueBorrow`-only readers behind a same-name `_Generic` macro (recommended) | as the previous row (probe) | none; one macro line per reader that owners reach | same; `-O2` identical (probe) | two inline `PyValue_View` overloads | C11; MSVC-capable |
| One type, ownership only in the slot class | none in hand-written C | none | – | – | – |

The count for the explicit form, outside `pycore_value.h` and
`pycore_pyvalue.h`: 1,805 reader argument positions, of which 496 are
already borrows. The other 1,309 are owners or reads of homes, 355 of them
in `bytecodes.c`. The last row is A5a's state, and D4 rejects it.

**Appendix A, rechecked.** The good unit compiles cleanly. With `-DBAD` each
of the three functions is an error on both compilers. `via_union` and
`via_struct` compile to the same instructions, and `call_u` and `call_s`
are tail jumps. All of that matches the proposal. The probe does not test
pointers, function types or member access, which is where M1 and M2 sit.

### 2. Generated code

- **Cost.** The design handles a field with two ownership states at no
  extra cost, provided the owner comes from the word already loaded (S1).
  Hand expansion for one input on the temporary path:
  - HEAD: one class test and branch at the consume, a NULL store, the same
    test again at the release (common-subexpressioned), then the owned-mask
    test.
  - Proposal: the class test, a reload of the slot, the store, a NULL test,
    then the mask test. That is two more, and the reload defeats range facts.
  - S1's form: the class test, the store, a select, then the NULL and mask
    tests. GCC folds the extra test; Clang keeps it once per handler in the
    add model.

  The model in S1's table gives the totals for three hot handlers. `IR_TAKE`
  for a local is a NULL test plus `XDUP`, cheaper than `NewRef`'s
  immortality load. For receivers, S2 removes the count pair altogether.
- **G9.** The rule holds. Views and owners are passed by value.
  `IR_CONSUME_INPUT` addresses a slot only. Generated handlers take
  addresses only of `window[…]` and `IR_SLOT(…)` for the `*At` readers
  (51 + 31 + 6 + 5 sites), never of an input variable. The existing
  `_PyValue_Publish(&(V))` in the `IR_STORE_*` macros takes the address of
  an owner local. That is A3a's debt; A5b neither adds nor removes it.
- **A3a.** The design is compatible: views and owners are per-handler locals
  and nothing crosses a dispatch. A3a needs two things from A5b:
  - the record rule (M3);
  - the live-range measurement (S8).

  A3a's slot-addressed helpers should take `const PyValue *` for slots and
  `PyValueBorrow` for read-only values.

### 3. Fail-closed for derived `PyObject *`

The claim is right. A C type cannot cover the pointer without retyping the
C API: `AsKnownObjectBorrow` has 846 uses, and its results reach about 85
distinct callees in `bytecodes.c` alone, plus all of `Objects/`. A
`PyObjectBorrow` would need `.ptr` at every use, and it would protect nothing
unless the stealing APIs took a distinct owned-pointer type. I reject it as
the proposal does. Failing closed is the right direction, but the four-form
rule would empty `borrow_mask` on the hottest attribute handlers (S3).

### 4. The heap side

**Split, verified on 28 sites.** The proposal's class matches the code at
each one, except `native_scope.c:55` (S5).

| Class | Sites read |
|---|---|
| C, `DUP` (15) | `dictobject.c:1762`, `:2626`, `:6341`; `sysmodule.c:3453`; `bltinmodule.c:1699`; `structseq.c:257`; `methodobject.c:536`; `typeobject.c:3800`; `ceval.c:958`, `:1746`; `itertoolsmodule.c:2236`; `cellobject.c:25`; `value_ops.c:24`; `structmember.c:79`; `borrow_scope.c:555` |
| S, delete (6) | `pycore_cell.h:29`; `tupleobject.c:139`, `:559`; `listobject.c:2980`; `setobject.c:392`; `native_scope.c:55` |
| W, delete (2) | `ceval.c:1015` (`_Py_VectorCall_ValueSteal`), `ceval.c:3121` ("Consumes func") |
| F, delete (1) | `pycore_interpframe.h:246` (a move, the source slot NULLed) |
| I, `TAKE` (4) | `bytecodes.c:1237` (`held_dict`), `:1358` (list store), `:2732` (receiver: S2), `:2977` (`*value_ptr`) |

**Producer list.** It is complete except for `_PyValue_FromObjectBorrowInline`,
the dict-watcher callback parameters, and home copies (M4, S5).
`_PyCell_GetValue` returns an owner and is correctly absent.
`_PyDict_GetValue_KnownHash`, `_PyDict_NextValue` and
`_PyObject_LookupSpecialMethod` write owners through their out-parameters.

**Type cache (Q2).** Not as a `PyValueBorrow` field. Give it its own private
guarded type, not an owned value and not a `PyObject *`: an inline value
cannot be a `PyObject *` without a box (S4).

### 5. `PyValue_Is` and late immortalisation

The reason is correct. `immortalize_interned` (`unicodeobject.c:14814`)
makes a mortal interned string immortal while untagged owners of it exist.
It is reached from `intern_common` whenever `immortalize` is set, which
covers `_PyUnicode_InternImmortal`, called from:

- `codeobject.c:181` and `dictobject.c:5154`;
- `Parser/action_helpers.c:132` and `Parser/pegen.c:698`;
- `Python/codecs.c:213` and `unicodeobject.c:2108`.

Owners made after that are tagged. The other paths:

- `PyUnstable_SetImmortal` (`object.c:2818`) acts only on a uniquely
  referenced object.
- `typeobject.c:679` and `:728` and `structseq.c:658` run at static-type
  initialisation.
- `long_dealloc` re-immortalises static small ints.

Finalization also goes the other way (S6).

To drop the mask, tag only static immortals. That means:

- testing `_Py_IsStaticImmortal` in `_PyValue_PointerSteal`,
  `_PyValue_FromPyObjectNew`, `XDUP`, `FromObjectBorrow`, `FromObjectKnown`
  and the home predicates;
- accepting that late immortals pay a mortal incref and decref on `DUP` and
  `CLOSE`. Upstream does the same for references made before
  immortalisation, and the counts balance far from the immortal threshold.

The gain is about three instructions on `IS_OP` only, since `IsNone`,
`IsTrue` and `IsFalse` already compare exactly. Not worth doing now. But
record static-only tagging in G6 as the target form: it makes bit 0 a
function of the object for its whole life, so H1 survives finalization. Q1's
measurement decides when.

### 6. Renames and merges

The survivors follow value-api.md §1.5:

- rule 4 for `DUP`, `XDUP`, `CLOSE` and `XCLOSE`;
- the entry names for `FromObjectSteal` and `FromObjectBorrow`.

`NewRef` → `XDUP` is right after step 3, for the proposal's reason, and the
counts check out:
- hand-written: 154 renames, 35 rewrites and 2 by hand;
- generated: 104.

One name per meaning does not quite hold afterwards:

- (a) `FromObjectBorrow` and `_PyValue_FromObjectBorrowInline` (S5);
- (b) `PyValue_AsStrongReference`, an identity (S5);
- (c) `PyValue_BorrowAt(p)`, equal to `PyValue_Borrow(*p)` once `Borrow` is
  a free type change: delete it (2 uses);
- (d) the eight close forms (N2);
- (e) under M2, `PyValue_Borrow` and `PyValue_View` become one function.

The type name is acceptable. Rule 3's suffix is now redundant (N3).

### 7. Steps, order and estimate

Each diff can be reviewed on its own, and the order is right. Step 1 adds
types that nothing produces. Step 2 mixes untagged views with hand-written
tagged borrows, which the mask in `PyValue_Is` covers. Step 3 has to flip
`Borrow` and convert the pairs in one diff. Step 4 comes last.

Step 3 keeps the suite green only if it builds and tests on both compilers
(M1). Moving the consume to the first take is safe for exits: no
`DEOPT_IF` or `EXIT_IF` follows a take in any uop body. Error paths must
release the owner; the step 2 test list covers that.

The estimate: see S9. A3a needs the record rule (M3), the register-pressure
measurement (S8), and a stated parameter type for its slot-addressed
helpers.

### 8. Comparison

| System | The proposal says | The code or design says | Missing lesson |
|---|---|---|---|
| Upstream `_PyStackRef` (`3a440b6`) | Stack refs exist only in frames and C locals; heap fields are `PyObject *`; `Is` masks the tag | Partly. Frame stack slots do hold borrows: `LOAD_FAST_BORROW` pushes `PyStackRef_Borrow`. Generator frames are heap homes, made safe on copy: `_PyFrame_Copy` calls `PyStackRef_MakeHeapSafe` (`pycore_interpframe.h:136–160`). `PyStackRef_Is` masks both operands (`pycore_stackref.h:742`) | `Py_STACKREF_DEBUG` tracks borrow provenance and fails on closing an owner with live borrows (S7) |
| HPy | "Two types, with stores only through the field type", taken | HPy's two types are handle (local) and `HPyField` (heap), with `HPyField_Store` and `HPyField_Load`. The fork has no field type: heap fields are `PyValue`, the owner type | The field type is what closes the copy-of-home gap (M4). Taking "two types" from HPy without it takes the wrong pair |
| V8 | "The C++ type system stops a `Local` from being stored in the heap" | Not by type alone. `Local<T>` is an ordinary copyable class; V8 relies on `HandleScope` lifetimes, debug-mode zapping of closed scopes, and static analysis of stack-only types | A checker for stack-only types (the proposal's `borrow-field` lint is that), and poisoning in debug builds |
| Rust | Not discussed | `&T` cannot outlive its owner; an owned non-`Copy` `T` moves | C's `PyValue` is `Copy`: moves are untracked (R3, M4). `ir_own_F` is an `Option<T>` (the X forms), which is the principled form for a field with two ownership states |
| PyPy cpyext | Rejected | Agreed | – |

## Checked and found sound

- **Recounts.** I reran `tally.py` and recounted by hand.
  - `MakeHeapSafe`: 150 in 24 files, split 89 / 40 / 21.
  - 131 + 13 conditional releases; `IR_INPLACE_HEAP_SAFE` 92 + 64.
  - 204 inputs: 177 borrowable, 27 owned.
  - `IR_KEEP_BOX`: 39 / 6 / 50.
  - The rename totals.
- **Already done.** `traverse_slots` calls `_Py_VISIT_VALUE` under the
  `IsHeapSafe` assert (`typeobject.c:2808–2809`).
- **`ceval.c` pairs.** They are `DUP`s, not identities (proposal §1, against
  the A5a notes §6).
- **ABI.** Union and struct callers are identical on both compilers. C and
  C++ callers of a `PyValueRead` function are identical on both compilers.
- **Step order.** The reasons in proposal:523–533 are correct.

## Probes (removed with the scratch directory)

- **`holes.c`.** The M1 and M2 table: one misuse per `#if HOLE == n`, each
  compiled with `-std=c11 -Wall -Wextra -fsyntax-only` on both compilers.
- **`gen.c`.** The M2 `_Generic` form: one good unit and four `BAD` units,
  plus a C++11 build of the same file.
- **`handlers.c` and `handlers2.c`.** The S1 model. It has slots, a frame
  header with `nlocalsplus`, fields decoded from a word, `decref` calling a
  cold `dealloc`, and cold `deopt`, `retire` and `boxint`. It defines HEAD's
  macros (`NewRef`, `MakeHeapSafe`, the class-tested consume and release)
  and A5b's (view, owner, `TAKE`, `XCLOSE`), and counts instructions with
  `-O2 -fcf-protection=none`. It is a model, not the generated handlers: the
  numbers compare forms, not the interpreter.

## Review 2 (revision 2)

Reviewed: revision 2 of the proposal (751 lines, uncommitted, 2026-11-04),
against the same runtime tree at `5de7bd62ef9`. Compilers: GCC 15.2.0, Clang
21.1.8, `g++` 15.2 and `clang++-21`. The quiet-window rule did not apply: the
checkpoint's last line was the 10:35 re-measure report. Appendix A's probe was
extracted verbatim and run in one scratch directory, since removed. Nothing
was built or timed. Line numbers below are in the runtime tree unless they
are marked as the proposal's sections.

**Verdict: accept with changes.** M1–M4 are resolved in substance. Two small
must-fix items remain: one in M1's flag placement and one in the step plan.
Both are text changes to make before A5b-1, and neither needs a third review.

### The must-fix findings of review 1

| Finding | Resolved? | What remains |
|---|---|---|
| M1 | Mostly. Adding the flags, running the negative units on both compilers with matched diagnostics, and covering casts in `raw-bits` is the right fix. | The flag goes into `PY_CORE_CFLAGS`, which no file under `Modules/` sees. Only the `.c.o` rule uses it (`Makefile.pre.in:3196`). Built-in modules compile with `PY_BUILTIN_MODULE_CFLAGS` and shared ones with `PY_STDMODULE_CFLAGS` (`Modules/makesetup:255`, `:260`), and neither includes it. At HEAD, 18 files under `Modules/` call `PyValue_` APIs, among them `_json.c`, `itertoolsmodule.c`, `_functoolsmodule.c`, `_operator.c` and the `_testinternalcapi` units. On Clang their pointer misuses remain warnings. See R2-M1. |
| M2 | Yes. The probe result is below. | None |
| M3 | Yes, under the coordinator's ruling. The plan is sound and costs nothing on `CALL_BUILTIN_O` (details below). | Should-fix items R2-S1, R2-S2 and R2-S4 |
| M4 | Yes. §2.3 now separates what the compiler enforces from what it does not. The census, the retype, the `home-copy` report and the `PyValueField` closure in V4/V5 are all as recommended. | None |

### The M2 probe, rerun

The probe was Appendix A, compiled with `-std=c11 -Werror=incompatible-pointer-types`.

- **The eight misuses.** All eight `BAD` units fail on GCC 15.2 and on
  Clang 21.1, with the diagnostics in Appendix A's table.
- **Without the flag.** GCC still rejects units 5–7. Clang only warns and
  exits 0, as M1 said.
- **As C++11.** The same file compiles under `g++` and `clang++-21`. All
  eight `BAD` units also fail there, which the proposal does not claim.
- **Code at `-O2`.** `via_macro_target` and `via_plain` are identical:
  - GCC: `movabs`, `cmp`, `setb`, `movzbl`;
  - Clang: `shr $51`, `xor`, `cmp $8191`, `setae`.
- **One small correction.** Under `-Wall -Wextra`, Clang gives the good unit
  one `-Wunused-function` warning, for the probe's own static
  `_PyValue_Publish`. The warning comes from the probe, not the form. A
  header would not trigger it. "Compiles cleanly" is true of GCC only.

### M3: the coordinator's ruling and the author's plan

**Is the plan sound? Yes.**

- *Lifetime.* For an object argument, `objects[0]` borrows the value that
  `_CALL_BUILTIN_O` holds in `s` (`bytecodes.c:4600`) until
  `native_call_finish`, which runs `End` before it clears `s`
  (`ceval.c:2161–2164`). That is the contract the `argument` field has
  today.
- *Consistency.* Converting in `Begin` makes the fixed scope behave like the
  vector scope, which already boxes in `Begin` (`native_scope.c:193`).

**Does it cost anything on `CALL_BUILTIN_O`? Not on the success path.**

| | HEAD | After the plan |
|---|---|---|
| `Begin` | tests `IsHeapSafe`, then tags | tests `IsInline`, then borrows or boxes |
| `Objects` | tests `objects[0]`, tests `IsInline`, then borrows or boxes | finds `objects[0]` set |
| `End` | tests `IsInline(arguments[0])`, then clears a tagged borrow | tests one recorded fact |

- The allocation is the same; it only happens earlier.
- No count pair is added.
- The one new cost is on cold error paths. An inline argument is now boxed
  and freed even on the error returns in `_PyCFunction_CallScoped` that come
  before `_PyNativeScope_Objects` (`methodobject.c:543`): the keyword check,
  the arity check and the recursion check at `:539`.

**Findings on the plan:**

- **R2-S1. Do not record "whether it boxed" as a flag.** A flag makes
  `argument_objects[0]` an object pointer that is owned or borrowed
  depending on the flag. That is §4.4's rejected "maybe owned" form, moved
  from the value to the object. HEAD has the same dual form (`End` tests
  `IsInline(arguments[i])`), and the plan should not carry it forward.
  - Replace `PyValue argument` with an owned `PyObject *argument_box`, NULL
    unless `Begin` boxed.
  - `objects[0]` is then always a borrowed view: of the object, or of the
    box.
  - `End` does `Py_CLEAR(scope->argument_box)`.
  - The struct keeps its size (8 bytes replace 8 bytes), there is no flag,
    and each field has one meaning.
- **R2-S2. Say what the record's rule covers.** Read literally, A3a's rule
  "the record holds owned values only" is broken by the scope's borrowed
  object pointers:
  - `argument_objects[0]` for an object argument;
  - `objects`, which points at that array or at the borrow stack.

  These are derived `PyObject *` borrows (§4.7), not `PyValue` words. The
  packet should state two things. The record's ownership rule governs
  `PyValue` words. The scope's object pointers are views of values the
  handler holds in `held_*`, and `End` releases them within the same handler
  activation. Without this, A3a's reviewer will flag `objects[0]`.
- **R2-S4. Two corrections to §8.2's call sites, and a test that breaks.**
  - §8.2 cites `methodobject.c:543`, `native_scope.c:164` and `ceval.c:1883`
    as `Begin` sites. They are the `_PyNativeScope_Objects` calls, for all
    scope kinds.
  - The fixed `Begin` has one runtime caller, `bytecodes.c:4589` (and its
    generated copy), whose `Objects` call is `methodobject.c:543`. It also
    has four test callers, in `Modules/_testinternalcapi/native_scope.c`
    (`:119`, `:219`, `:348`, `:429`).
  - The test at `:429`, `native_scope_box_oom`, installs its failing
    allocator between `Begin` and `Objects` (`:437–448`). Under eager
    conversion the box is allocated in `Begin`, with the normal allocator.
    `Objects` then succeeds, and the test fails with "argument did not
    allocate a scalar box". A3a must install the failing allocator before
    `Begin` and expect `Begin` to fail.

**For the coordinator (outside A5b): the receiver costs more than the
argument.** For a builtin with a module `self`, `CALL_BUILTIN_O` pays three
count pairs that upstream does not:

- `scope->callable = Py_NewRef(callable)` (`methodobject.c:482`);
- `scope->receiver = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(self))`
  (`:536`);
- `receiver_object = _PyValue_AsObjectAt(...)`, a new reference
  (`native_scope.c:187`).

Upstream passes `PyCFunction_GET_SELF(callable_o)` borrowed. Here the window
owns the callable until `End`, and the callable owns its `self`. This is a
larger cost than the `argument` question, and a candidate for A3a or the
native-boundary work.

### S1 and S3: the author's departures

**S1, sound, and better than my form.**

- *Why it is sound.* `_PyIR_AdoptTemp(x)` moves the count out of a
  temporary's slot. The same expression stores NULL into that slot, and
  `raw-bits` confines the conversion to `ceval_macros.h`.
- *Why it is better.* It holds the bits my `ir_word_F` would have held, so
  the model's counts carry over, and it needs no `PyValue` local that holds
  a borrow.
- *Note.* `OWNER` also reads `ir_own_F`. The consume must therefore come at
  the first `TAKE` *or* `OWNER`; §4.2 names only `TAKE`.

**R2-S3. The proven-inline rule has no mechanism.** The rule is new in
revision 2, and the generator has nothing to build it on.

- *No inline fact today.* `Tools/cases_generator` has none. `inline_ok`
  (`analyzer.py:1001`) marks ops that accept inline values; it proves
  nothing.
- *What the add's ops do.* `_GUARD_TOS_INT` proves inline with
  `EXIT_IF(!PyValue_IsInlineInt(value))` (`bytecodes.c:656–657`).
  `_POP_TOP_INT` still closes with `CLOSE_SPECIALIZED` and asserts only
  `LongCheckExact` (`:349–351`).
- *Why it matters.* A wrong fact drops the release of an object. That is a
  leak that neither the compiler nor the type sees, which makes this the one
  rule in A5b-2 outside the type.
- *Fix.* Specify where the fact comes from, for example a DSL annotation on
  a guard's outputs (`value: inline`) that is checked against the guard's
  `EXIT_IF`. Emit `assert(PyValue_IsInline(x))` wherever a release is
  dropped.
- *The figure.* The add's Clang figure in §4.2 is still 105, the modelled
  form. The proven-inline figure is expected, not measured.

**S3, sound.** A checked allow-list fails closed, and the deny-list could
only fail open. Two caveats:

- *What "checked" can mean.* `test_generated_cases` can check that the
  generator applies the list. Nothing checks that a listed callee neither
  stores nor steals. Each entry needs a one-line reason beside it.
- *The figure.* "At most 5 of 177 inputs change" is a prediction. The
  families named are about ten, against the about 85 callees that review 1
  counted. §4.7 measures it before landing, and that is the right gate.

### Other problems in revision 2

- **R2-M2 (must-fix). `PyValue_Borrow` names two things between A5b-1 and
  A5b-3.**
  - *The conflict.* A5b-1 makes `PyValue_Borrow` the `_Generic` view that
    returns `PyValueBorrow`, and routes every reader through it. But §8.1
    says A5b-3 "must untag `Borrow` in the same diff that converts
    `MakeHeapSafe(Borrow(x))` to `DUP`", so the tagging producer survives
    until A5b-3.
  - *What stops compiling.* Of the hand-written uses at HEAD, 34 are
    `PyValue_MakeHeapSafe(PyValue_Borrow(…))`, 7 assign the result to a
    `PyValue`, and 5 return it as a `PyValue`. `IR_KEEP_BOX`
    (`ceval_macros.h`) uses it at 95 generated and 5 JIT sites. All of these
    stop compiling when the view lands in A5b-1. So A5b-1 is not, as §8.1
    says, "safe alone" with "nothing produces a borrow yet".
  - *Fix.* A5b-1 renames the tagging form by script, to a private
    `_PyValue_BorrowTagged`, at its sites and in `IR_KEEP_BOX`. A5b-2
    removes it from generated code, and A5b-3 deletes it. Each step then has
    one name per meaning. Cost: about 0.3 h.
- **R2-M1 (must-fix). Where the flag goes.** Put
  `-Werror=incompatible-pointer-types` in `CFLAGS_NODIST`, as
  `configure.ac:2989` does for `-Werror=implicit-function-declaration`.
  Every in-tree unit then gets it: core, built-in and shared. §2.3 should
  add that third-party extensions built with Clang get only warnings for the
  pointer cases; GCC 14 and later reject them.
- **Note. The lint cannot see where a `_PyNativeScope` lives.**
  `borrow-field` allows `_PyNativeScope` by type, and a type does not say
  where an instance lives. A scope on the heap, or in a record, would pass.
  Until A3a removes the field, the lint should also reject `_PyNativeScope`
  as a member of any struct, and in a `sizeof` allocation. Once A3a removes
  it, the allow-list is empty and L1 has no exception; the proposal should
  say so.
- **Note. The finalization clause turns H1 off early.** "Unless finalizing"
  turns the check off at `_PyInterpreterState_SetFinalizing`
  (`pylifecycle.c:2435`). That comes before `finalize_modules` (`:2506`) and
  the last collection (`:2533`), where H1 still holds. A flag set on entry
  to `finalize_interp_clear` (`:2123`, called at `:2589` and `:2852`) keeps
  the check over that teardown. Not blocking.
- **Note. Reader macros and later declarations.** A declaration or
  definition that comes after its same-name macro expands it:
  `int PyValue_IsInlineInt(PyValueBorrow v);` becomes a call. The script
  must emit each macro after its declaration. Out-of-line definitions should
  write the name in parentheses, `(Name)`, as CPython does for `Py_INCREF`.

### The estimate and the steps

21 h stands, as an upper budget. Some work is not in it:

- R2-M2, about 0.3 h;
- R2-M1, minutes;
- the proven-inline mechanism, which fits in A5b-2's 8 h.

The step order holds once R2-M2 is applied. The steps cover every finding of
review 1.

### Remaining items

Must-fix, as text changes before A5b-1:

- **R2-M1.** Put `-Werror=incompatible-pointer-types` in `CFLAGS_NODIST`, so
  that built-in and shared modules get it, not only `PY_CORE_CFLAGS` units.
- **R2-M2.** Give the tagging `PyValue_Borrow` a private name from A5b-1
  until A5b-3 deletes it, so that the view and the producer never share a
  name.

Should-fix:

- **R2-S1.** Replace the native scope's `argument` with an owned
  `argument_box`, and keep `objects[0]` always borrowed, with no flag.
- **R2-S2.** State that A3a's "owned values only" governs `PyValue` words,
  and that the scope's object pointers are views released by `End` inside
  the activation.
- **R2-S3.** Give the proven-inline rule a declared source and a debug
  assert.
- **R2-S4.** Correct §8.2's call-site citations, and make A3a's plan move
  the OOM injection in `native_scope_box_oom` before `Begin`.
