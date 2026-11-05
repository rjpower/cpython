# Review 1: step V2a-1a, value slots of int, float, bool and None (proposal)

Reviewed: [value-api-v2a1a.md](../proposals/value-api-v2a1a.md) revision 1
(417 lines, uncommitted, 2026-11-04), against the common brief
(`build/pyjit-link-wave1/VALUEAPI-STEP-PROPOSAL-COMMON.md`),
[value-api.md](../value-api.md) (R1, R9, R17, R26, R27, §1.3–1.5),
[value-api-v2a0.md](../proposals/value-api-v2a0.md) (§1.4, §1.7, §4.6) and the
sibling proposals in flight (`value-api-m3.md`, `value-api-m1b-m2.md`). Code:
`build/worktrees/pyvalue-rc-universal`, read-only, at `c71a3ca6aa2` with
V2a-0 part 2 uncommitted in its index. Line numbers below are from that tree.
Nothing was built or run; path lengths are read from the code, not measured.

## Verdict

**Accept with changes.** The per-type plan holds: representation dispatch
inside int's slots, `NotImplemented` for floats (D4), bool never inline (D5),
None with no slot (D6), the `str` and `object` sides pulled forward (D7, D8),
the box table and the `hold()` removal (D9). Two changes are needed before
implementation:

- **The digit view is a second object form of the small int**, introduced to
  keep upstream's `PyLongObject *` kernel signatures, and the proposal never
  weighs the alternative that removes it: kernels over a digit span
  (finding 1). Revision 2 should respecify §1.2–1.3 and §8 for spans; only
  those sections need a second look.
- **The step's own slot bodies call compatibility thunks** that D13 creates
  in the same step, and one of those calls can recurse (finding 2).

## Finding 1 in brief: the digit view against the alternatives

| Option | What a digit kernel receives for an inline operand | Escape | Kernel edits | Verdict |
|---|---|---|---|---|
| Digit view (D2; GMP's `mpz_roinit_n`) | A `PyLongObject` header (type, immortal count, tag) and 2 digits in C locals: an exact int object in the inline range, the non-canonical form that R8 removed | Prevented by convention and a debug assert on direct results only | none | reject |
| **Digit span** (GMP's `mpn` layer; V8's `src/bigint` `Digits`/`RWDigits`) | `{const digit *d; Py_ssize_t n; bool neg}`; a heap int presents `ob_digit` the same way, an inline int a local `digit[2]` | Impossible: a span is not a `PyObject *` | about 9 leaf kernels | **adopt** |
| int64 variants for mixed operands (GMP's `_si`/`_ui`) | the word | none | one more implementation of each operation | reject |
| Word kernels plus a heap temporary for mixed operands | a heap int in the inline range | none | none | reject: a core box (R27). It is today's `_PyValue_MultiplyInt64` (`pyvalue.c:184–207`) |

V8 never meets this case: Smis and BigInts do not mix (`1n + 1` throws), and
its BigInt algorithms already take digit spans. LuaJIT has no big integers;
its integer/number split widens to double, so it has one form per range and
no bridge. GMP has both of the first two rows. `mpz_roinit_n` exists for
callers that do not own the kernels. We own them, so the kernel layer is the
right one to change.

## Findings

| # | Severity | Finding | Evidence | Fix |
|---|---|---|---|---|
| 1 | must-fix | **§1.2, D2, D3, Q1–Q2: replace the digit view with digit spans.** (a) *Second form.* A view is an exact `PyLongObject` holding an int51: the object form of an inline int that canonical values exclude. It has automatic storage, but it is still a box. It exists only so that the kernels keep upstream's signatures (D3 cites R1, but R1 asks for a typed concrete position, which a span also is). (b) *Containment fails.* "Only between a value slot and a typed kernel" will not hold. M3b changes the Clinic `self` of `class int` and `class float` to `PyValueBorrow` (27 blocks), so `int.bit_length`, `to_bytes`, `__format__` and `__round__` on an inline `self` need digits. So do `math.gcd`/`isqrt`/`comb` and `%d` in W2 and W4. Sonnet workers would then write stack objects that must not escape. (c) *Escape is wider than §1.2 says.* Operands come back through out-parameters and early returns as well as direct results. The kernels can also run Python code: they call `_pylong`, which is safe here only because a view has two digits and the thresholds are 300 and 1,000 digits. The debug assert checks direct results only. (d) GC and debug headers are not a problem: ints are not GC-tracked, increments and decrements of an immortal return early, and `_PyObject_IsUniquelyReferenced` is false for an immortal, so the in-place paths (`_PyLong_Negate`, `_PyLong_GCD`) copy. Those paths are safe only because of the immortal count | (c) `long_divrem` `*prem = long_long(a)` for \|a\|<\|b\| (`longobject.c:3200`), likewise `:3263`; `long_pow` `z = Py_NewRef(a)` (`:5100`); `long_rshift1` (`:5300`); `pylong_int_divmod` passes both operands to `_pylong.int_divmod` (`:4373`), and repr passes its operand to `_pylong.int_to_decimal_string` (`:2010`). (d) `:333`, `:5801` | Define `_PyLongSpan` and `_PyLong_SpanOf(PyLongObject *)`, plus `_PyLong_SpanOfWord(int64_t, digit buf[2])` in `pycore_long.h`. Convert the leaf kernels to take spans: `x_add`, `x_sub`, `x_mul` (an inline operand has at most 2 digits, so `k_mul` always sends it to `x_mul`; Karatsuba stays object-only), `divrem1`/`x_divrem` (they only read digits and allocate their own working copies), `long_bitwise`, `long_compare`, the digit path of `long_true_divide`, `long_pow`'s base and exponent reads (it copies the base into its working object, as upstream allocates on every `MULT`), and `long_to_decimal_string_internal`. The object entry points become one-line wrappers. Results stay new `PyLongObject`s, canonicalised at the slot's exit. Widen `_PyLong_WordOperands` to heap ints of up to 2 digits, which fit int64. An inline operand then meets only heap ints of magnitude at least 2^60, so compare and shift counts are decided by sign and size, with no kernel. Cost: about 2 agent-hours more than the view (the lead sets the type and converts `x_add` as the pattern; a worker converts the rest). Tests: `int_digit_path` runs the span kernels; add `divmod(5, 2**70)`, `-5 % 2**70`, `pow(5, 1, 2**70)` and `I(2**40) // 3` |
| 2 | must-fix | **§2.1, D4, D5, D13, Q9: the step's slot bodies call thunks.** D13 makes the object extractors thunks, but float's slots read a heap int "by `PyLong_AsDouble`" (D4, §1.1, §2.1). This breaks the standing ruling that core code never calls a compatibility thunk. Three more cases. `float_richcompare`'s exact path calls the `PyObject_RichCompareBool` thunk on a `PyLong_FromDouble` temporary: that call, not the allocation Q9 discusses, is the defect. bool's four sides forward to int's object slots, which become thunks. `long_float` calls `PyLong_AsDouble`. Worst: if D13's "and the rest" covers `PyLong_AsLongLongAndOverflow`, the entry primitive recurses: `thunk → _Py_VALUE_ARG → _PyObject_ValueIdentity → thunk` | `floatobject.c:431`, `:270`, `:491–495`; `boolobject.c:83`, `:90`, `:98`, `:106`; `longobject.c:5912`; `pyvalue.c:159` (`_PyObject_ValueIdentity` → `PyLong_AsLongLongAndOverflow`); D13 names no row for that function | Add a table to §2.1 listing every object-extractor and object-slot call inside the converted bodies, each routed to a typed kernel or value form: float operands via the `_PyLong_AsDouble` kernel; `float_richcompare` via `_PyValue_RichCompareBool` on `PyValue_FromObjectSteal(vv)` (an int subclass `w` still gets its reflected `__lt__`, as upstream); bool via int's value functions; `_PyObject_ValueIdentity` via a typed kernel, never a thunk. Mark the new extractor thunks `Py_VALUE_COMPAT` in V2a-1a's own three files now, so that the compiler lists these call sites; leave the rest of the marking to W2 |
| 3 | should-fix | **D14, §5, R2: path length and the inline arm.** (a) "About 15 instructions" fits hash. For compare it is about three times too low. `_PyValue_RichCompareBool` with two inline ints now runs the identity test; the out-of-line `_PyValue_RichCompare` (thread-state fetch, recursion guard); `do_richcompare` (two `PyValue_TYPE`); `call_richcompare` (field load, indirect call); int's slot; the `NotImplemented` test; `_PyValue_IsTrue` on the result; and `XCLOSE`. That is about 40–60 instructions, two direct calls and one indirect call. The right baseline is upstream, which has the same shape. (b) After I1 the typed handlers do not cover the hash path. They specialise on the container, not the key, so every `d[i]`, `i in s` and `s.add(i)` with an inline key dispatches `_PyValue_Hash`. Compares reach the dispatcher from `list.index`/`count`/`remove`/`in`, tuple `in`, mixed sorts (`unsafe_object_compare` sends inline keys there), and `heapq`, `bisect`, `min` and `max` through the thunk until W4. (c) D9 keeps an exact-int arm in `_PyValue_WriteRepr` because upstream has one, while D14 forbids arms; the two decisions contradict each other | `object.c:1182–1199`, `:2128–2137`; `value_ops.c:63–79`; 12 `_PyValue_Hash` sites in `dictobject.c`, 3 in `setobject.c`; `listobject.c:2473` | Adopt one named rule: *an inline value's type is int or float, static and immutable, so a dispatcher may call that type's value-slot function directly.* Apply it in `_PyValue_Hash`, `_PyValue_RichCompareBool` and `_PyValue_WriteRepr`. The arm calls the function stored in `PyLong_Type.tp_value` (declared `static inline` in `pycore_long.h` beside D10's kernels), so there is one implementation and no second guard form. `compare_inline`, a second implementation (compare as doubles), is deleted. `binary_op1` does not need the arm: after I1, the typed handlers cover its callers |
| 4 | should-fix | **D12, §2.3, Q6: the extractor rows are incomplete, and one merged row breaks its own rule.** Generating the rows from one macro is one form; replacing them with two extractors would push range checks and messages into more than 600 callers, so keep the rows. But "one row per object extractor" is not met. There is no row for `PyLong_AsUnsignedLong`, `…LongLong`, the two `…Mask` forms, `AsLongLongAndOverflow`, `AsInt32`/`UInt32`/`UInt64`, `AsNativeBytes` or `AsVoidPtr`: about 30 core and 230 `Modules/` calls, so W4 has no target for them. Merging `PyLong_AsLongLong` and `PyLong_AsInt64` into one row cannot keep both error texts or both contracts | `rg` counts (definitions included): unsigned family 14 + 51, `LongLongAndOverflow` 4 + 21, `NativeBytes` 10 + 57, `AsVoidPtr` 3 + 79. "int too big to convert" (`longobject.c:1222`, via `_PyLong_AsByteArray`) vs "Python int too large to convert to C int64_t" (`:6779`), and `int PyLong_AsInt64(obj, int64_t *)` | Add rows for the unsigned and mask families and `AsLongLongAndOverflow` (each is one macro line), or list which extractors stay object-form and who converts their callers. Give `AsInt64` its own row or record the message change. Rename `_PyValue_AsSsize_t(v, exc)` or `_PyLong_AsSsize_tValue` so that two functions with different semantics do not differ only in prefix |
| 5 | should-fix | **D11, §2.2: `PyValue_FromBool(int)` truncates under script class R.** The script maps `PyBool_FromLong(long)` to it by name. A `long` argument whose low 32 bits are zero becomes `False` | e.g. `PyBool_FromLong(flags & (1L << 40))`; 82 + 166 calls | Declare `PyValue_FromBool(bool)`: C converts any scalar to `bool` as `!= 0` |
| 6 | should-fix | **§7: collisions not listed.** (a) M3 emits `PyValue_FromSsize_t`, which D11 drops. (b) M3b edits the `class int` and `class float` Clinic lines in `longobject.c` and `floatobject.c`, yet the table says M3 touches neither file. (c) 1a-4 edits `abstract.c` and `object.c`, which M1b and M2 also edit; the integration order V2a-1a → M1b → M2 is stated for `typeobject.c` only. (d) W1b ends near T9, so its "last batch" cannot wait for V2a-1a (T12.5 or later). (e) `_json`, `_decimal`, `mathmodule.c` and `mathintegermodule.c` call int and float object slots directly. After this step those slots are thunks, which box inline results until W4 | (a) `value-api-m3.md:360`, `:447`. (b) m3 §2.2, "27 blocks". (e) `_json.c:1540`, `:1601`, `:1697`; `_decimal.c:7920–7923`; `mathmodule.c:1595`; `mathintegermodule.c:462` | (a) M3 uses `PyValue_FromInt64`; coordinator's call. (b) List M3b and order it: V2a-1a first, then M3b regenerates. (c) Extend the order to `abstract.c` and `object.c`. (d) Take `longobject.c`, `floatobject.c`, `boolobject.c` and `unicodeobject.c` out of W1b; W2 sweeps them. (e) Record these as T boxes in §5, owned by W4 |
| 7 | should-fix | **§8: estimate arithmetic.** 1a-1 divides 22 functions by the lead's 8 per hour and then subtracts "the shared skeleton", which the rate already discounts. The row also carries about 15 word kernels, the moved helpers and 22 thunks, none of them counted. Thunks total 22 + 16 + 4 + 2 = 44, not 43. `_PyValue_AsSsize_t` exists, so there are 8 new rows | §8 rows 1a-1, 1a-3; §2.1 | 1a-1: about 3.5 h with the view, 5.5 h with spans (finding 1), of which a worker takes 1.5 h. Lead total about 7 h, so V2a-1a i–ii lands near T14. I1 (T8–T14), which integrates after it, slips to about T15. Say so in §8 and in v2a0 §1.7 |
| 8 | note | **§3: box table.** Traced from the code: `x == 'a'` 1 C, `'a' in […]` 0, `str(x)` 1 A, `divmod` 2 A, `pow(x, 2, 7)` 3 A + 1 T, `PyNumber_Add` 1 T. The counts hold, and `pow`/`operator.add` are correctly owned by W2 (`pow` is a Clinic builtin) and W4 (`_operator`). Two corrections. The claim "not in allocations" (§3, Q7) fails on the digit path: three-operand `long_pow` allocates its result, `FromObjectSteal` frees it, and the thunk exit allocates again, two allocations against upstream's one for results from 257 to 2^50. `str(x)` after I1 is 0 when specialised, because I1 converts `CALL_STR_1` | v2a0 §1.4 I1 row | Correct Q7. Split the `str(x)` row into specialised and unspecialised. Make `test_box_kinds` warm up, or disable specialisation, before asserting exact counts |
| 9 | note | **D7, §2.1, Q4: `object`'s compare.** Pin upstream's semantics. `==` gives `True` for the same word and otherwise **`NotImplemented`**, not `False`. `!=` calls the receiver type's compare with `Py_EQ` and inverts a non-`NotImplemented` result through `_PyValue_IsTrue`; with no `tp_richcompare` it gives `NotImplemented`. Q4: the pair row is redundant. Once `object`'s initialiser sets its value field, the MRO walk gives it to NoneType and the sentinel, because their object slot is `object`'s | `typeobject.c` `_Py_BaseObject_RichCompare` (`:8088`); pass `type_ready_inherit_value` at `:10039` | State both rules in §2.1 and test `object.__eq__(1000, 'a') is NotImplemented`. Drop the pair row, unless the coordinator prefers to follow V2a-0, which listed `PyObject_GenericHash` in the pair table |
| 10 | note | **D9: `hold()` removal holds on the sample.** Checked: list `contains`, `index`, `count`, `remove`, `richcompare` and repr (each `PyValue_DUP`s); sort keys (owned by the sort's saved array); cell compare (owners from `_PyCell_GetValue`, released with `XCLOSE`); dict `:1093` (`Py_INCREF`); `dict_equal` (owners from `_PyDict_NextValue` and `_PyDict_GetValue_KnownHash`); dict-items `contains` (`DUP`); the frozenset lookup and `frozendict_pair_hash` (immutable homes); the thunks (C-API contract). The site count is 52: slice has 6, not 5 | `listobject.c:497`, `:528`, `:3005`, `:3031`, `:3058`, `:3113`; `cellobject.c:146–153`; `dictobject.c:1092`, `:4287–4290`, `:6381–6390`; `setobject.c:201–210` | Keep the contract line of v2a0 §4.6 in `pycore_value_ops.h` beside the dispatchers, where workers in W2 will read it |
| 11 | note | **Correctness checks that pass.** `binary_op1` treats `slotw == slotv` as one slot, so `True + 5` and `5 + True` call int's side once. `5 & True` reaches bool's side first through the subtype test. A negative exponent goes to float's value `nb_power`. Inline-int and inline-float hashes agree with heap ints and floats (\|i\| < 2^50 < 2^61 − 1; -1 maps to -2 in both; `_Py_HashDouble` gives integral floats the int's hash). Mixed compares are exact: an int51 converts exactly; a heap int keeps `float_richcompare`'s exact path; a NaN, always a heap float, compares false. `-2**50 // -1` and `-(-2**50)` leave the range [-2^50, 2^50 − 1]. D4's reason fits heap floats only, since inline floats are exact `float`; the choice stands because the typed handlers take inline mixtures | `abstract.c:1135–1186`; `pycore_value.h:194–270` | – |

## Scope (item 3)

- **`object`'s compare and str sides (D7) and `str`'s class-N line (D8):**
  accept. Together they are three functions and one line, and they remove the
  slot-adapter boxes from the commonest mixed compares and from `str(x)`.
  Waiting for W2 would leave the dispatchers adapting every
  `x == None` and `x == 'a'` for a whole wave.
- **Dropping `PyValue_FromSsize_t` (D11):** accept. One signed and one
  unsigned constructor are exact for every C integer type of at most 64
  bits. Reconcile the name with M3 (finding 6a).
- **Nine rows against two (Q6):** see finding 4. Keep rows generated by one
  macro, make the set complete, and keep upstream's per-type messages, which
  the rows provide at no cost.

## What holds

- Representation dispatch inside the slot, with `NotImplemented` for foreign
  operands and the reflected order unchanged, ports upstream's
  number protocol exactly. bool and None need no special case in the
  dispatchers.
- D10: kernels shared by the slots and I1's typed handlers give one
  definition of each word operation. Whether inlining gives the handlers'
  current scaled form, which uses one overflow test, is for I1 to check, as
  §4 says.
- Moving `_PyValue_MultiplyInt64` and `_PyValue_LshiftInt64` off
  `PyNumber_Multiply` and heap temporaries removes an uncounted box path.
- The tests are well chosen. The parity sweep against a digit-path oracle and
  the extractor parity against `_testcapi` find exactly the edge errors that
  R1 warns of.

## Review 2 (revision 2)

Reviewed: revision 2 of the proposal, rewritten in place (436 lines), against
the same tree. I checked §1.2–1.3 (spans and `_PyLongOut`), §2.2, D13
(R-inline), §2.4–2.5 and §8, and answer the author's Q1, Q3 and Q4.

**Verdict: accept. Implementation can start.** Review 1's findings 1–7 are
resolved:
- spans replace the view, with the wider word path;
- §2.2 routes the calls, and the entry primitive calls only kernels;
- R-inline is one named rule, and `compare_inline` is gone;
- the extractor set is complete, and `AsLongLong` and `AsInt64` are separate;
- `PyValue_FromBool(bool)`;
- the collisions are listed;
- the estimate adds up: lead 5.75 + Sonnet 9.75 = 15.5, landing near T14.

One new must-fix remains. It does not block 1a-0 or 1a-1, and the interface
note 1a-0 settles it before the worker's span diff (1a-2) starts.

| # | Severity | Finding | Evidence | Fix |
|---|---|---|---|---|
| 12 | must-fix | **§1.2, D3, §3: `_PyLong_OutObject` lets the heap temporary back in.** "`long_pow` copies the base into a working object", and so do `_PyLong_GCD`'s copies. Each puts an inline operand into a `PyLongObject`: the object form that row 1 of the change table says is never built, and the row that review 1's table rejected as a core box. Upstream copies nothing at these points; it takes a reference. §3's "no allocation" for `pow(x, 2, 7)` also rests on an unspecified "builds the result in `small`" path inside `long_pow_spans`, although its intermediate products need up to 4 digits | `z = Py_NewRef(a)` (`longobject.c:5100`), `table[0] = Py_NewRef(a)` (`:5127`); §1.2 `long_pow_spans` row and the `_PyLong_OutObject` comment | State the rule in 1a-0: *an out holds computed results only, never a copy of an input span.* `long_pow_spans` keeps the base, and the window table's first entry, as spans. `MULT` takes spans after 1a-2, so accumulators are multiply results. Name the three-operand word path in D9: `_PyLong_WordPowMod` for a modulus below 2^60, with products below 2^120 in `unsigned __int128` (GCC and Clang on 64-bit targets). Then `long_pow_spans` runs only for a modulus of 3 or more digits, and the `small` special case goes. For `_PyLong_GCD` in W4: reduce the heap operand by the word first (`divrem1`), then use a word gcd, with no copy |
| 13 | should-fix | **§2.2, D12: the table lists less than the opt-in will flag.** Marking the new extractor thunks in the four units also flags these calls. First, two converted bodies the table omits: the `PyLong_AsInt64` shift counts in `long_rshift` and `long_lshift_method`. Second, three bodies that stay: `_PyLong_GCD` (four calls), `PyLong_Export` (two) and float's `is_integer`, `__round__` and `from_number` (`PyFloat_AsDouble`). That is 11 call lines beyond §8's 9 routed sites | `longobject.c:5368`, `:5468`, `:5873–5877`, `:6854`, `:6857`; `floatobject.c:839`, `:1036`, `:1638` | Add them to §2.2. Shift counts go by word, or by sign and size, as §2.1 does. GCD and `Export` go through span readers. The float methods use `PyFloat_AS_DOUBLE` after their type check. Add about 0.3 h to 1a-4 and 1a-5 |
| 14 | note | **`_pylong` switches.** `long_to_decimal_string_internal` holds the switch at more than 1,000 digits, and that switch needs the object; with a span signature the kernel no longer has one. `l_divmod`'s switch must also stay above `long_divrem` | `longobject.c:2107–2108`, `:4468–4476` | Say that every `_pylong` switch lives in the object wrapper, before the span kernel. A span from a word never reaches one |
| 15 | note | **Q4's thunk.** `_Py_VALUE_THUNK_RICHCMP` emits a `static` function, but `_Py_BaseObject_RichCompare` is extern and named by two other files | `pycore_typeobject.h:149`; `object.c:2386`; `sentinelobject.c:217` | Write the thunk by hand, in three lines, or give the macro a linkage argument. Do not add a static thunk plus an exported wrapper: that makes two pointers again |
| 16 | note | **Small corrections.** (a) The heap word range starts at 2^50, not 2^51: the inline range is [-2^50, 2^50 − 1] (D1, §1.2). Add `2**50` and `-2**50 - 1` to the edge set. (b) `PyLong_AsVoidPtr` sends a non-int to `PyLong_AsUnsignedLong`, which raises `TypeError`: it accepts no `__index__`, so the row is `_PyValue_IntAsVoidPtr`. (c) R-inline's mixed pair (left's compare, then right's reflected) is the whole dispatch order for two inline values, because no subclass relation or identity default can apply; pin it with `sorted([3, 1.5, 2])` and `[1, 2.5].index(2.5)`. (d) 1a-2, 1a-3 and 1a-5 are three Sonnet diffs in one file, `longobject.c`. Give each a disjoint set of functions and merge them in plan order, or the wall clock slips | `longobject.c:1497–1506`; §6; §8 | – |

**Answers to the author's questions.**

- **Q1 (`_PyLongOut` frees a heap result that falls into the inline range):**
  accept. It is a result allocation sized by a bound, not a conversion of an
  inline value. But `x_sub`, the main case, needs no second pass. Upstream's
  `x_sub` finds the highest differing digit before it allocates, so
  reserving after that scan gives the exact size. The proposal's own example,
  `2**70 - (2**70 - 1)`, then builds in `small` and allocates nothing. The
  other kernels keep their bounds.
- **Q3 (int64 word kernels against the handlers' scaled int51 form):** keep
  int64. It is the representation-independent input. The scaled
  single-overflow test is a property of the encoding (R26), and the compiler
  cannot derive it from int64 operands, so expect one or two more
  instructions in the typed handlers. Record that in I1; it is not a gate. If
  I1 adopts the scaled form, it replaces the kernel for both callers, so there
  is still one kernel. Two-digit heap ints then take the span path, as Q3 says.
- **Q4 (rename `_Py_BaseObject_RichCompare` as `object`'s own slot):** agree.
  Review 1's note 9 was wrong. In the tree, `object`'s slot is the static
  `object_richcompare` (`typeobject.c:8044`, `:9162`), while NoneType and the
  sentinel name the exported wrapper `_Py_BaseObject_RichCompare` (`:8088`).
  The pointers differ, so the MRO walk would not have matched them. Making
  the exported function `object`'s only compare slot gives one function, drops
  the wrapper, and lets the pass supply the value side. That is better than
  either a pair row or my suggestion. Mind the macro linkage (finding 15).
