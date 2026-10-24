# 02: Tagged values in `_PyStackRef` (Phase 1) and the path to `PyValue` (Phase 2)

Status: design, 2026-10-23. Covers plan items 1a and 1b, and the encoding
constraint they place on idea 5. Line numbers refer to this branch at
`321925c92c9`.

## Decisions

1. `_PyStackRef.bits` adopts the final Phase 2 layout now: JavaScriptCore-style
   NaN-boxing. Pointers keep their raw value (top 16 bits zero), doubles are
   `raw + 2^49`, inline ints are `0xFFFE_0000_<int32>`.
2. Inline ints are int32. Inline floats ship in Phase 1 (option c). NaN
   results stay boxed.
3. Interpreter-internal indices (loop indices, `lasti`) move to their own
   region, `0x0001_<int48>`.
4. `Py_TAG_REFCNT` keeps its meaning inside pointer space only. Inline values
   own nothing and cannot be deferred.
5. Boxing happens at the borrow: the cases generator boxes a stack input in
   place before its first `PyObject*` conversion in any instruction that is not
   tagged-aware. Guards use `PyStackRef_TYPE` and never box.
6. Python `is` and `id()` use value identity for exact int32 ints and exact
   non-NaN floats. C `Py_Is` stays pointer equality in Phase 1.
7. The free-threaded build keeps building and passing tests, with no
   performance work.
8. Tier 2 and the copy-and-patch JIT are not maintained from step 2.

## 1. What the code does today

`_PyStackRef` is a `uintptr_t` (`Include/internal/pycore_structs.h:66-72`).
The two low bits hold three tags (`Include/internal/pycore_stackref.h:53-58`):
`Py_TAG_REFCNT` (1), `Py_TAG_INVALID` (2) and `Py_INT_TAG` (3), with the int
payload shifted left by 2 (`:445-466`). NULL is `0x1` and ERROR is `0x2`
(`:386`, `:479`).

The tag scheme works because tagged ints have bit 0 set. `PyStackRef_CLOSE`
and `PyStackRef_DUP` test only `bits & 1` (`:533-536`, `:638-645`, `:699-705`),
so tagged ints skip refcounting at no extra cost. `PyStackRef_Borrow` ORs in
bit 0 unconditionally (`:546-549`). `PyStackRef_Is` masks bit 0 (`:742`).

Tagged ints never reach Python code. Their only producers are the
virtual-iterator index (`Python/ceval.c:1145`, `:3821-3830`;
`Python/bytecodes.c:3737`, `:3901-4015`) and `lasti` during unwinding
(`bytecodes.c:6481`, consumed at `:1926`). The GC skips them
(`Python/gc.c:465`, `:479`; `Python/gc_free_threading.c:271`, `:440`, `:842`,
`:1834`).

The `Py_STACKREF_DEBUG` variant (`pycore_stackref.h:60-382`,
`Python/stackrefs.c`) replaces pointers with table IDs. IDs start at 20 and grow
by 4. Tagged ints keep the same `(i << 2) | 3` form there (`stackrefs.c:271-298`).

Compact ints have one 30-bit digit, so |v| < 2^30
(`Include/cpython/longintrepr.h:117-142`). `_PyCompactLong_Add` returns a
stackref and allocates from the int freelist (`Objects/longobject.c:328-353`,
`:3867`). The small-int cache in this tree covers -5..1024, not -5..256
(`Include/internal/pycore_runtime_structs.h:97-98`). Floats allocate from a
freelist on every result (`Objects/floatobject.c:126-138`).

Measured on `build/baseline-default` (gcc 15, -O3, no PGO) with
`perf stat -e instructions:u`, as user-mode instructions per iteration of an
inner `for i in range(1000)` loop. Rows after the first two subtract the
80-instruction skeleton:

| Loop body | Instructions today |
|---|---|
| empty loop skeleton (`FOR_ITER_RANGE`, `STORE_FAST`, `JUMP_BACKWARD`) | 80 |
| same, with i ≥ 1025 so `FOR_ITER_RANGE` allocates | 143 (whole loop) |
| `z = x + y`, ints, result not cached | 164 |
| `z = x + y`, ints, cached small result | 100 |
| `z = a + b`, floats | 130 |
| `z = x < y`, ints | 93 |
| `z = a < b`, floats | 79 |
| `z = l[j]`, list of floats | 88 |
| `z = x; z = y` | 57 |

An int or float allocation plus its later free costs about 60 instructions.

## 2. Encoding

### 2.1 Layout

| Top 16 bits | Low 48 bits | Phase 1 meaning | Phase 2 `PyValue` meaning |
|---|---|---|---|
| `0x0000` | `0x000000000001` | NULL | NULL (see Q1) |
| `0x0000` | `0x000000000002` | ERROR | ERROR |
| `0x0000` | ptr, bit 0 = 0 | owned reference | object |
| `0x0000` | ptr, bit 0 = 1 | borrowed, immortal or deferred | object (bit 0 ignored) |
| `0x0000` | ptr \| 2 | `PyStackRef_Wrap` internal pointer | same |
| `0x0001` | int48 | interpreter-internal index | same |
| `0x0002`–`0x7FF2` | | +0.0 to +inf, stored as `raw + 2^49` | same |
| above `0x7FF2_0000_0000_0000` to `0x8001` | | unused (image of positive NaNs) | canonical NaN `0x7FFA_0000_0000_0000`, if adopted |
| `0x8002`–`0xFFF2` | | -0.0 to -inf | same |
| above `0xFFF2_0000_0000_0000` to `0xFFFD` | | unused (image of negative NaNs) | unused |
| `0xFFFE` | `0x0000` then int32 | inline int | inline int |
| `0xFFFF` | | never used | never used |

```c
/* pycore_stackref.h, SIZEOF_VOID_P == 8 */
#define Py_TAG_REFCNT     ((uintptr_t)1)                 /* pointer space only */
#define Py_TAG_INVALID    ((uintptr_t)2)                 /* pointer space only */
#define Py_PTR_LIMIT      ((uintptr_t)1 << 48)
#define Py_INDEX_BASE     ((uintptr_t)1 << 48)           /* 0x0001_... */
#define Py_DOUBLE_OFFSET  ((uintptr_t)1 << 49)           /* 0x0002_... */
#define Py_INT_BASE       ((uintptr_t)0xFFFE << 48)
#define Py_OWNED_MASK     (((uintptr_t)0xFFFF << 48) | Py_TAG_REFCNT)

#define PyStackRef_IsPointerSpace(r)  ((r).bits <  Py_PTR_LIMIT)
#define PyStackRef_IsInline(r)        ((r).bits >= Py_DOUBLE_OFFSET)  /* int or float */
#define PyStackRef_IsInlineInt(r)     ((r).bits >= Py_INT_BASE)
#define PyStackRef_IsInlineFloat(r)   ((r).bits - Py_DOUBLE_OFFSET < Py_INT_BASE - Py_DOUBLE_OFFSET)
#define PyStackRef_BothInlineInt(a,b) (((a).bits & (b).bits) >= Py_INT_BASE) /* needs 0xFFFF unused */
#define PyStackRef_RefcountOnObject(r) (((r).bits & Py_OWNED_MASK) == 0)

static inline _PyStackRef PyStackRef_FromInt32(int32_t i)
{ return (_PyStackRef){ .bits = Py_INT_BASE | (uint32_t)i }; }
static inline int32_t PyStackRef_InlineInt(_PyStackRef r)
{ return (int32_t)(uint32_t)r.bits; }
static inline _PyStackRef PyStackRef_FromDoubleNotNaN(double d)
{ uint64_t u; memcpy(&u, &d, 8); assert(!isnan(d)); return (_PyStackRef){ .bits = u + Py_DOUBLE_OFFSET }; }
static inline double PyStackRef_InlineFloat(_PyStackRef r)
{ uint64_t u = r.bits - Py_DOUBLE_OFFSET; double d; memcpy(&d, &u, 8); return d; }
static inline _PyStackRef PyStackRef_TagIndex(int64_t i)          /* -1 .. 2^47-1 */
{ return (_PyStackRef){ .bits = Py_INDEX_BASE | ((uint64_t)i & (Py_PTR_LIMIT - 1)) }; }
static inline int64_t PyStackRef_UntagIndex(_PyStackRef r)
{ return (int64_t)(r.bits << 16) >> 16; }
static inline _PyStackRef PyStackRef_Borrow(_PyStackRef r)
{ return (_PyStackRef){ .bits = r.bits | (uintptr_t)PyStackRef_IsPointerSpace(r) }; }
```

Float negation is `bits ^ (1ULL << 63)`. Adding 2^49 never carries into bit 63
for a non-NaN double, so the sign flip commutes with the offset.

### 2.2 Why this variant of NaN-boxing

The plan describes NaN-boxing with raw doubles and pointers in the NaN payload.
That layout cannot contain Phase 1: a raw pointer such as `0x00007f12_34567890`
is a subnormal double there. Phase 1 must keep raw pointers, because the C ABI
is untouched and `PyStackRef_AsPyObjectBorrow` must stay one AND
(`pycore_stackref.h:539-543`). The double-favouring layout would add a mask or
an OR at about 600 conversion sites. The offset variant keeps pointers raw and
charges doubles one add to encode and one subtract to decode. Low-bit tagging
(V8, OCaml) has no room for a 64-bit double.

### 2.3 Int range

| Range | Overflow check | Phase 2 subset | Boxed form | Verdict |
|---|---|---|---|---|
| 62-bit (today's spare range) | `jo` on 64-bit add | no: the NaN-box holds 32 bits | up to 3 digits | rejected |
| 31-bit (compact, \|v\| < 2^30) | range compare after add (2 to 3 instructions) | yes, but Phase 2 range differs | always compact | workable |
| int32 | `add r32; jo`, `imul r32; jo` | identical to Phase 2 | compact, or 2 digits for \|v\| ≥ 2^30 | **chosen** |

Int32 matches Phase 2, so overflow code written now survives, and the hardware
flag is the cheapest check. Every compact int fits. The int guard accepts any
exact int whose value fits in int32, including 2-digit ones, so
"inline-representable" means one thing throughout.

### 2.4 Floats: option (c)

| Option | Effect | Verdict |
|---|---|---|
| (a) no inline floats | ints could keep bit 0 set (payload in bits 16-47), leaving `CLOSE`/`Borrow` unchanged; costs three shifts per int op forever (subset rule) and defers all float wins to Phase 2 | fallback if the step 2 gate fails |
| (b) index into a per-thread float table | a slot needs a lifetime, which means refcounts or a scan; escapes still box; no Phase 2 use | rejected |
| (c) full NaN-boxing now | Phase 1 is the Phase 2 layout minus NaN; subset is trivial | **chosen** |

The price of (c) is paid in Phase 1 only, since Phase 2 deletes `CLOSE`, `DUP`
and `Borrow`. No layout with full doubles can make one OR-able bit mean "no
refcount": non-NaN doubles alone need more than 2^63 patterns. From gcc -O2 on a
model of the helpers:

| Operation | Today (x86-64) | Proposed |
|---|---|---|
| `PyStackRef_CLOSE`/`DUP` test | `test $1,%dil; je` | `movabs; test; je` (+1; `movabs` is shared within a handler) |
| `PyStackRef_Borrow` | `or $1` | `movabs; mov; or; cmp; cmovb` (+3; on arm64 `orr; tst #imm; csel`, +2) |
| `PyStackRef_AsPyObjectBorrow` | `and $-2` | unchanged after the box check |

### 2.5 Subset property

Phase 1 uses only `0x0000` (pointers, NULL, ERROR), `0x0001` (indices), the
non-NaN double images, and `0xFFFE` with bits 32-47 clear. Phase 2 reads each
of these patterns the same way; pointers decode through `bits & ~1`, as
`BITS_TO_PTR_MASKED` does now (`pycore_stackref.h:477`). Phase 2 adds patterns
only in unused rows: a canonical NaN, or new kinds under `0xFFFE` with nonzero
bits 32-47 (which requires `IsInlineInt` to compare bits 32-63). So Phase 1's
set is a strict subset of Phase 2's, with identical meanings.

It breaks in three places. The first two are constants behind existing
macros; the third is a platform restriction:

- **NULL** is `0x1` so that `XCLOSE` equals `CLOSE` (`pycore_stackref.h:479`,
  `:721-728`). If Phase 2 picks `0x0`, only `PyStackRef_NULL` changes.
- **None, True, False** are static-object pointers with bit 0 set
  (`:483-485`). If Phase 2 makes them immediates, Phase 1's forms become a
  second encoding that `is` must normalise. Keep them as objects (Q1).
- **32-bit hosts** have a 32-bit `_PyStackRef`. Phase 1 disables inline values
  when `SIZEOF_VOID_P < 8` and keeps `(i << 2) | 3` indices there.

The layout assumes heap addresses below 2^48. That holds on Linux x86-64 (LA57
included, absent a mmap hint) and stock arm64 Linux and macOS, but not under
arm64 top-byte tagging (Android Scudo, HWASan). Debug builds assert it in
`_PyObject_Init`.

## 3. Semantics

**Overflow.** Add, subtract and multiply use `__builtin_{add,sub,mul}_overflow`
on int32. On overflow the exact result is computed in int64 (|a·b| ≤ 2^62) and
boxed with `PyLong_FromLongLong`. `-INT32_MIN` and `INT32_MIN // -1` take the
same path. Inline is a representation; Python ints stay unbounded.

**Types.** Only exact `int` and `float` are inline. `True` and `False` stay
pointers and fail the int guards, as they do now. `PyStackRef_TYPE` returns
`&PyLong_Type` or `&PyFloat_Type` for inline values (`pycore_stackref.h:748-754`).

**Identity.** A value boxed into a list and the same value inline in a frame
are one object to the program. Hence, in order:

- `l[0] is x` must be True, so `is` compares by value when a side is inline.
- `id(x) == id(l[0])` must hold, so `id()` must derive from the value, not from
  whichever box exists.
- `is` must agree with `id()`, so value identity covers every exact int32 int
  and exact non-NaN float, boxed or inline.

The rule: `x is y` is True if they are the same object, or both are exact ints
with equal int32 values, or both are exact non-NaN floats with equal bits
(`0.0 is -0.0` stays False). `id()` of such a value returns its encoding, which
is at least 2^49 and cannot collide with an address. `_IS_OP`
(`bytecodes.c:3293`) pays only when bits differ; `operator.is_`/`is_not`
(`Modules/_operator.c:711-716`) follow. `Py_Is` stays pointer equality, since C
sees only boxes in Phase 1. The language reference allows this for immutable
types, and PyPy uses the same rule. Today the case never arises: tagged ints
are internal, and `PyStackRef_Is` masking bit 0 (`:742`) is harmless for
`(i << 2) | 3`. Under the new layout it would merge adjacent inline ints, so it
must mask pointers only.

**`sys.getrefcount`.** The baseline already returns 1 for a computed local int
(`LOAD_FAST_BORROW`). A fresh box also returns 1; -5..1024 box to the immortal
cache, as now.

**Hashing** runs on the box, so values are unchanged. A future inline-key dict
lookup must reuse `v mod 2^61-1` and `_Py_HashDouble`.

**NaN and signed zero.** NaN results stay boxed, at the cost of one
`ucomisd; jp`. This keeps NaN identity (`nan in [nan]`, NaN dict keys, the
identity hash at `Python/pyhash.c:93-97`) and payloads, with no
canonicalisation. Encoding an arbitrary NaN through the offset is unsafe:
negative NaN images land on ints, pointers and indices. `-0.0` is inline as
`0x8002_0000_0000_0000`.

## 4. The boxing boundary

**Rule.** A stack input is boxed in place, meaning the stack slot is
overwritten with an owned reference to a new box, before its first conversion to
`PyObject*` in any instruction that is not tagged-aware. The existing input
cleanup then closes the box. Boxing values in -5..1024 returns the cached
object.

Boxing can fail with `MemoryError`, so the generated code takes the
instruction's error exit. Ints and floats are not GC-tracked, so boxing runs no
Python code and cannot start a collection. The generator can therefore treat it
as non-escaping, like `PyStackRef_AsPyObjectBorrow` (`Tools/cases_generator/analyzer.py:546`),
and avoid spilling the stack pointer.

If a guard later fails, the instruction re-executes with a box in the slot. A
box is the same value, so this is correct.

| Escape point | Location | Class |
|---|---|---|
| Vectorcall to C | `_PyObjectArray_FromStackRefArray` (`ceval.c:1075-1093`); `CALL_BUILTIN_*`, `CALL_METHOD_DESCRIPTOR_*` | (a) box; `CALL_LEN`/`CALL_ISINSTANCE` could be (b) |
| Call to a Python function | `_INIT_CALL_PY_EXACT_ARGS` (`bytecodes.c:4573-4580`), `initialize_locals` (`ceval.c:1807`) | no box; `*args` (`ceval.c:1857`) and `**kw` (`:1957`) are (a) |
| `RETURN_VALUE` / `YIELD_VALUE` to a Python frame | `bytecodes.c:1574`, `:1852`; `_MAKE_HEAP_SAFE` passes inline values | no box |
| Return or yield to a C caller | `INTERPRETER_EXIT` (`bytecodes.c:1550`); generators via `genobject.c:283` | (a) |
| `STORE_ATTR*` | e.g. `_STORE_ATTR_INSTANCE_VALUE` (`bytecodes.c:3090`) | (a); (c) with shape slot kinds |
| `STORE_SUBSCR`, `_DICT` | `bytecodes.c:1410`, `:1454` | (a) |
| `STORE_SUBSCR_LIST_INT` | `bytecodes.c:1423` | index (b), value (a), boxed before the lock |
| `STORE_GLOBAL`, `STORE_NAME`, `STORE_DEREF` | `bytecodes.c:2164`, `:2001`, `:2404` | (a) |
| `BUILD_LIST/TUPLE/SET/MAP`, `LIST_APPEND`, `SET_ADD`, `MAP_ADD` | `listobject.c:3366`, `tupleobject.c:472`, `ceval.c:956` | (a) |
| `f_locals`, `locals()`, tracebacks, pdb | `frameobject.c:51`, `:2301` | (a), fresh box per read; writes store boxes (`:311`) |
| Monitoring events | `_RETURN_VALUE_EVENT` (`bytecodes.c:1596`), `_YIELD_VALUE_EVENT` | (a) |
| Generic `BINARY_OP`, `COMPARE_OP`, `CONTAINS_OP`, `FORMAT_*`, slices | | (a); slices and f-strings could be (c) |
| `BINARY_OP_EXTEND` | descriptors take `PyObject*` (`specialize.c:2249`) | (a); float/int entries move to (b) in step 6 |
| `IS_OP` | `bytecodes.c:3293` | (b), value identity |
| Exceptions | unwinding calls `XCLOSE` (`bytecodes.c:6464-6478`); exception values are objects | none |
| GC | `gc.c:465`, `:479`; `gc_free_threading.c:271`, `:440`, `:842`, `:1834`; `_Py_VISIT_STACKREF` (`pycore_stackref.h:862`) | skip; no box |
| Generator frames at rest | `gi_iframe`, visited by `_PyFrame_Traverse` | stay inline |
| Remote debugger, gdb | `_remote_debugging/object_reading.c:37`, `Tools/gdb/libpython.py:110`, `:157` | decode, never dereference |

`PyStackRef_AsPyObjectBorrow` appears on 488 lines of non-generated source.
Bytecodes.c accounts for 342 lines (348 calls):

| Bucket (bytecodes.c) | Calls | Treatment |
|---|---|---|
| Type and version guards (`_GUARD_*`, `_CHECK_*`, `_ITER_CHECK_*`, `_RECORD_*`) | 106 | `PyStackRef_TYPE`, no box |
| Containers and subscripts | 63 | generator box-on-borrow; index operands become aware |
| Calls | 53 | box-on-borrow; the callable and `self_or_null` slots are object-only |
| Attributes, globals, cells, imports | 36 | box-on-borrow for values; owners via guards |
| Frames, generators, exceptions, iterators, functions | 35 | object-only, debug assert |
| Generic and non-numeric operators | 34 | box-on-borrow |
| Specialised int and float arithmetic | 21 | rewritten as tagged-aware |

| Other files (146 lines) | Lines | Treatment |
|---|---|---|
| `Python/ceval.c` | 24 | call helpers box; `Borrow(x) != NULL` tests (`:1660`, `:1970`, `:2012`, `:2024`) become `PyStackRef_IsNull` |
| `Python/specialize.c` | 21 | `PyStackRef_TYPE` |
| `Objects/typeobject.c` | 19 | type-lookup results, object-only; zero-argument `super()` (`:12781`, `:12814`) boxes |
| `record_functions.c.h`, `ceval_macros.h` | 17, 13 | tier 2, deleted |
| `Objects/frameobject.c` | 8 | box |
| `Objects/call.c` | 6 | method/self pairs; `self` may be inline, box |
| all others | ≤ 3 each | case by case |

`AsPyObjectSteal` (45 lines) and `AsPyObjectNew` (8) get the same treatment,
with fallible boxing.

## 5. Specialised instructions in Phase 1

| Instruction | Change |
|---|---|
| `LOAD_FAST*`, `STORE_FAST*`, `COPY`, `SWAP`, `POP_TOP` | no code change; they inherit the new `DUP`/`Borrow`/`CLOSE` |
| `LOAD_SMALL_INT`, `LOAD_CONST` | quickening rewrites them to inline-int and inline-float variants for exact int32 and non-NaN float constants |
| `BINARY_OP_{ADD,SUBTRACT,MULTIPLY}_INT` | accept inline or boxed int32, produce inline, box on overflow |
| `BINARY_OP_{ADD,SUBTRACT,MULTIPLY}_FLOAT`, new tier-1 `TRUEDIV_FLOAT` | accept inline or boxed floats, plus an int32 operand when the other is a float; result inline unless NaN |
| `COMPARE_OP_INT`, `COMPARE_OP_FLOAT` | accept both encodings |
| `TO_BOOL_INT` (`bytecodes.c:521`) | inline: `(uint32_t)bits != 0` |
| `UNARY_NEGATIVE` (`:455`) | becomes a family with `_INT` and `_FLOAT` |
| `BINARY_OP_SUBSCR_{LIST,TUPLE,STR,USTR}_INT`, `STORE_SUBSCR_LIST_INT` | index read directly from an inline int |
| `FOR_ITER_RANGE` (`:4053-4066`) | inline result for int32 values instead of `PyLong_FromLong` |
| `FOR_ITER_LIST/TUPLE/VIRTUAL`, `GET_ITER_VIRTUAL`, `RERAISE` | index moves to `PyStackRef_TagIndex` |
| `IS_OP` | value identity (section 3) |
| `UNARY_NOT` | unchanged; operates on bools only |

The fast path, written as one instruction (tier 1 only):

```c
inst(BINARY_OP_ADD_INT, (unused/5, left, right -- res)) {
    int32_t a, b, sum;
    if (PyStackRef_BothInlineInt(left, right)) {          // and; cmp; jb
        a = PyStackRef_InlineInt(left);                   // low 32 bits, free
        b = PyStackRef_InlineInt(right);
    }
    else {                                                // one side boxed
        DEOPT_IF(!_PyStackRef_AsInt32(left, &a));         // inline, or exact int in int32
        DEOPT_IF(!_PyStackRef_AsInt32(right, &b));
    }
    STAT_INC(BINARY_OP, hit);
    if (__builtin_add_overflow(a, b, &sum)) {             // add r32; jo
        PyObject *big = PyLong_FromLongLong((long long)a + b);
        ERROR_IF(big == NULL);
        res = PyStackRef_FromPyObjectSteal(big);
    }
    else {
        res = PyStackRef_FromInt32(sum);                  // mov r32; or r64
    }
    PyStackRef_CLOSE_SPECIALIZED(left, _PyLong_ExactDealloc);   // mask test: no-op when inline
    PyStackRef_CLOSE_SPECIALIZED(right, _PyLong_ExactDealloc);
}
```

A tagged int plus a boxed compact int (for example `x + 1000` with a boxed
constant) misses the AND test. `_PyStackRef_AsInt32` then costs about 8
instructions per boxed operand: pointer test, `ob_type` compare and `lv_tag`
compare. That replaces today's two guards and the call to `_PyCompactLong_Add`.

Float add checks both operands with two range compares each, subtracts the
offset, runs `addsd`, uses `ucomisd; jp` for NaN, and adds the offset back. That
is about 17 instructions, with no allocation and no freelist traffic.

`COMPARE_OP_INT` is the AND test, `cmp r32`, and a select between the
`True`/`False` constants: about 11 instructions. `FOR_ITER_RANGE` replaces the
out-of-line `PyLong_FromLong` with a range check and an OR.

Instructions per loop iteration, as in section 1. The proposed column is an
estimate from the gcc -O2 model plus measured dispatch and load overhead; steps
4 and 5 replace it with measurements.

| Loop body | Today | Proposed |
|---|---|---|
| empty loop skeleton | 80 | ~70 |
| skeleton with i ≥ 1025 | 143 | ~70 |
| `z = x + y`, ints | 164 (100 if cached) | ~50 |
| `z = a + b`, floats | 130 | ~55 |
| `z = x < y`, ints | 93 | ~45 |
| `z = a < b`, floats | 79 | ~50 |
| `z = l[j]` | 88 | ~85 |
| `z = x; z = y` (no arithmetic) | 57 | ~62 |

The last row is the cost on code without numbers. Step 2 measures that cost on
its own.

## 6. Deferred refcounting (plan item 1c)

`Py_TAG_REFCNT` keeps its meaning: bit 0 of a pointer-space value means the slot
does not own a count. The reason can be a borrow, immortality, or deferral. 1c
changes which objects get the bit in the default build and adds a thread-stack
root scan to `gc.c`, modelled on `gc_free_threading.c:450`. That scan uses the
same `PyStackRef_IsObject` predicate this design introduces.

Inline values and indices have no owner, so "deferred" means nothing for them.
`Py_OWNED_MASK` excludes them from `CLOSE` regardless of bit 0. Fresh boxes are
plain owned references and are never deferred. The two items share one
predicate and touch no common code paths. Either can land first.

## 7. Free-threaded build

The free-threaded build uses bit 0 for deferred references
(`pycore_stackref.h:596-600`). It masks bit 0 in `IsNone`, `IsTrue` and
`IsFalse` (`:489-491`), scans all thread stacks (`gc_free_threading.c:271`,
`:440`, `:842`), and has `_Py_TryIncrefCompareStackRef` (`pycore_stackref.h:828-840`). All of
this lives in pointer space. A masked comparison cannot match an inline value,
because inline values are at least 2^49. Boxing uses the per-thread freelists.
Inline values cross threads only inside containers, where they are boxed, so
the change adds no new races.

Recommendation: keep `Py_GIL_DISABLED` building and passing the test suite at
every step, and do no free-threading performance work. The cost is about 10
free-threaded-only sites, mostly `IsNullOrInt` becoming `!IsObject`, plus one
extra build and test run per step. Revisit at Phase 2e, where the tracing GC
replaces both refcount schemes.

## 8. Tier 2, which is dropped

The following become dead code:

- the symbolic compact-int reasoning in `optimizer_bytecodes.c:209-240`
- the in-place int ops (`bytecodes.c:721-773`, `ceval_macros.h:651-680`)
- the in-place float ops (`bytecodes.c:852-950`, `:467`)
- the tier-2-only `_BINARY_OP_TRUEDIV_FLOAT` (`:907`)
- `_GUARD_BIT_IS_SET_POP`, which reads address bits of `True` and `False`
  (`:6054-6072`)
- `record_functions.c.h`
- the JIT stencils

Inline values make the in-place tricks unnecessary, because the result does not
allocate. The type lattice in `optimizer_bytecodes.c` stays as reference
material for the tier-1 JIT. Step 2 deletes the tier-2-only uops rather than
audit them. Plan item 1z, the register IR, is not affected: its frame slots are
`_PyStackRef` too.

## 9. Debugging

- **`Py_STACKREF_DEBUG`** uses the real encoding for inline values and indices
  (table IDs stay below 2^48). `_Py_stackref_get_object` fails with "inline
  value borrowed without boxing at file:line". Boxes get table entries naming
  the boxing site, so leak reports show it.
- **Frame checker.** `_PyFrame_CheckValues(frame)` walks
  `localsplus..stackpointer`: pointers must reach an object with refcount above
  0 and a type-typed `ob_type`; ints must have bits 32-47 clear; floats must
  decode to non-NaN; ERROR, unused rows and pointer-space `...11` are
  rejected. `_testinternalcapi.check_frame_values()` exposes it.
- **Switch.** `PYTHON_TAGGED_VALUES=0` (`-X tagged_values=0`) stops the
  specialiser and quickener from emitting value-producing variants, so no
  inline Python values exist; hot paths pay nothing. `=check` (debug builds)
  runs the checker at every `RESUME`, frame pop and collection. Bisect
  correctness with the switch, performance against the parent commit.
- **Injection hook.** `_testinternalcapi.inline_locals()` re-encodes the
  caller's eligible locals as inline values, so step 2 can test the whole
  boxing boundary before any producer exists.
- **Statistics.** A `Py_STATS` counter of boxes per opcode finds double boxing.
- `Tools/gdb/libpython.py` and `Modules/_remote_debugging` learn to decode
  inline values in step 2.

## 10. Implementation sequence

| # | Step | Tests | Benchmarks, expected |
|---|---|---|---|
| 1 | Split internal indices from ints: rename `TagInt`/`UntagInt`/`IsTaggedInt`/`IncrementTaggedIntNoOverflow` to `*Index`, keeping the old encoding. Users: `ceval.c:1145`, `:3821-3830`; `bytecodes.c:1760-1793`, `:3737`, `:3845-4015`, `:6481`, `:1926`, `:4146`. | full suite on default, free-threaded and `Py_STACKREF_DEBUG` builds | none, neutral |
| 2 | New layout, predicates, `Box`/`BoxInPlace`, generator box-on-borrow, guards via `PyStackRef_TYPE`, GC, gdb and remote-debugger decoding, frame checker, switch, injection hook. No producers. Delete tier-2-only uops. | new `Lib/test/test_tagged_values.py`, driven by `inline_locals()`: calls, attribute and subscript stores, `f_locals`, tracebacks, generators across `gc.collect()`, `MemoryError` during boxing via `_testcapi.set_nomemory`; full suite on all three builds; `test_generated_cases` | **overhead gate**: richards, go, nqueens, chaos within 1.5% geomean; otherwise fall back to option (a) |
| 3 | Identity: `IS_OP`, `id()`, `operator.is_`/`is_not` value identity | `l[0] is x`, `id` stability, `0.0 is not -0.0`, NaN identity, small-int `is`; `test_builtin`, `test_operator` | neutral |
| 4 | Inline ints: `LOAD_SMALL_INT`/`LOAD_CONST` quickening, `ADD/SUB/MUL_INT`, `COMPARE_OP_INT`, `TO_BOOL_INT`, subscript indices, `STORE_SUBSCR_LIST_INT`, `FOR_ITER_RANGE` | boundary values (2^31-1 + 1, -2^31 - 1, multiply overflow, mixed boxed and inline, 2-digit boxed int32), `bool` and subclass exclusion, `sys.getrefcount`; `test_long`, `test_int`, `test_sys`, `test_dis`; suite with `PYTHON_TAGGED_VALUES=0` | microbenchmarks above; nqueens and go neutral to up; richards neutral |
| 5 | Inline floats: `ADD/SUB/MUL/TRUEDIV_FLOAT` with int32 promotion, `COMPARE_OP_FLOAT`, `UNARY_NEGATIVE_{INT,FLOAT}`, float constants | NaN never inline (`inf-inf`, `0*inf`, NaN dict keys), `-0.0`, `struct` round trip of NaN payloads, float subclasses take the slow path, division by zero; `test_float`, `test_math`, `test_statistics`, `test_fractions` | nbody up strongly (allocations per pair drop from about 24 to 8); float and chaos up modestly (attribute stores still box) |
| 6 | Wider coverage, guided by box statistics: int32 `//`, `%`, `&`, `\|`, `^`, `<<`, `>>`, moving the compact-long and float-long entries out of `BINARY_OP_EXTEND`; float `**` with positive base | Python floor and modulo sign rules, shift limits; `test_long` | spectral_norm up (`eval_A` uses `//` and `1.0 / int`) |
| 7 | Optional, if statistics show double boxing: flowgraph marks `LOAD_FAST` whose consumer is not tagged-aware, and those loads box the local in place, so later loads reuse one box | box counters before and after | float and chaos up; others neutral |

## 11. Open questions for the coordinator

1. **Phase 2 constants.** Do None, True and False stay static-object pointers,
   and NULL stay `0x1`? If so, the subset holds exactly; immediates or
   `NULL == 0` are one-constant changes that break it for those patterns. The
   C API owner (item 2a) should decide.
2. **Identity semantics.** Do we accept value identity for `is` and `id()` on
   exact int32 ints and non-NaN floats? It is allowed but visible, and
   upstreaming Phase 1 would need a PEP-level discussion.
3. **Overhead gate.** If step 2 costs more than 1.5% on non-numeric benchmarks
   on x86-64, do we accept it (Phase 2 deletes `CLOSE` and `Borrow`) or switch
   to option (a)?
4. **Free-threaded CI.** Keep it through Phase 1, as recommended, or drop it
   now?
5. **NaN in Phase 2.** Keep NaN boxed, or adopt one canonical inline NaN that
   gives all NaNs a single identity and hash?

## 12. Coordinator decisions (2026-10-23)

1. **Phase 2 constants.** None, True and False stay static immortal objects
   and NULL stays `0x1`. A compare against a static pointer costs the same as
   a compare against an immediate, so immediates buy nothing that justifies
   breaking the subset property. Recorded as a constraint on the Phase 2 C
   API design (plan item 2a).
2. **Identity.** Accepted: `is` and `id()` use value identity for exact
   int32 ints and exact non-NaN floats. PyPy has shipped the same semantics
   for years. Document it in `boil/design/` and in What's New for the fork.
3. **Overhead gate.** 1.5 percent geomean on richards, go, nqueens and chaos
   is the target. Up to 3 percent is accepted with a note, because Phase 2
   deletes `CLOSE`, `DUP` and `Borrow`. Above 3 percent, fall back to option
   (a) and revisit floats in Phase 2.
4. **Free-threaded build.** Keep it compiling and passing the test suite
   through Phase 1, with no performance work. It is dropped at plan step 2c
   when the encoding flips.
5. **NaN in Phase 2.** Preference for one canonical inline NaN. Final call
   belongs with the C API design, since it fixes NaN hashing and identity for
   extensions too. Phase 1 keeps NaN boxed as this note proposes.

Sequencing change: tier 2 and the copy-and-patch JIT are removed as their own
step before step 2 of section 10, so the new encoding never has to be taught
to code we are deleting.

## 13. Coordinator decisions after measurement (2026-10-23, later)

Step 2 was built twice and measured in instructions and time against the
step 1 binary on richards, go, nqueens and chaos with no inline producers:

| encoding | instructions, `-l 1` | per loop | time geomean |
|---|---|---|---|
| NaN-box (section 2.1) | +5.8% | +7.7% | 1.047 |
| low-bit int tag, floats boxed | +2.5% | +3.3% | not yet timed |

The NaN-box cost is structural while refcount borrows are dynamic:
`PyStackRef_Borrow` cannot be `or $1` when bit 0 of an inline float is a
mantissa bit, and that macro runs on every `LOAD_FAST_BORROW`. The low-bit
encoding keeps `Borrow`, `CLOSE`, `DUP` and `Is` at their step 1 cost; what
remains is box-in-place sites that take the address of a local and force
spills, guard exits (`test $4; jne`), and code growth defeating inlining.

Decisions:

1. **Phase 1 encoding is the low-bit int tag.** Low 3 bits: `000` owned
   pointer, `001` borrowed or immortal pointer, `010` error, `011` index
   (`(i << 3) | 3`), `101` inline int32 in bits 32 to 63. No inline floats in
   Phase 1 (option (a) of section 2.4). Inline floats return when borrows are
   static (the register IR's borrow bit) or in Phase 2.
2. **The subset property is relaxed** to: every encoding fact lives behind
   the predicates in `pycore_stackref.h`, so the Phase 2 re-encoding is a
   header change plus re-measurement, not a rewrite of handlers.
3. **Before landing step 2:** emit boxing by value (`x = PyStackRef_Box(x)`)
   so no address is taken, mark the inline branch unlikely, and re-measure.
   The gate stays at 2 percent instructions with 3 percent accepted.
4. **Guards** stay generated. With producers, an int guard becomes the inline
   test itself and gets cheaper than today's `ob_type` load and compare.

## 14. Correction (2026-10-23, review)

Section 13 decided too much on too little. The step 2 measurement exercised
a refcounting interpreter with dynamic borrow flags, no inline producers and
no IR, which is the worst case for any tagging scheme and says nothing about
the cases where NaN boxing pays: floats held in slots across iterations,
floats in containers and attributes once those hold `PyValue`, and the JIT
materialising doubles only at escapes. The dominant NaN-box cost, the
pointer-space test in `PyStackRef_Borrow`, exists only because borrow status
is a runtime bit; the IR makes it static.

Revised decisions:

1. **The encoding decision is deferred to IR step 4** (the 1:1 IR
   interpreter with static borrows), where both encodings are re-measured
   on equal footing with the first inline producers in place. NaN boxing
   remains the Phase 2 target and the default candidate.
2. **Section 13 decision 1 is withdrawn.** `boil/tagged-step2-wip` (NaN-box)
   and `boil/tagged-lowbit` (low-bit) are both kept as measured data.
3. **Section 13 decision 3 stands** (by-value boxing, unlikely branches,
   inlining control): it applies to either encoding.
4. **Conversion reduction work precedes the decision**: static borrow bit,
   feedback-driven slot kinds so boxing checks are emitted only where inline
   values can flow, slot-level `known_object` across instructions, and
   `inline_ok` on the numeric handlers.

## 15. Final decision (2026-10-23, after review)

NaN boxing, as in section 2.1, developed incrementally on the main branch.
The pre-IR overhead is accepted and recorded; inline ints (step 4) and
floats (step 5) pay it back on numeric code, and the IR's static borrows
remove the `Borrow` cost everywhere else. Sections 13 and 14 are history;
the prototype branches are deleted once their generator improvements are
carried over. No further encoding prototypes.
