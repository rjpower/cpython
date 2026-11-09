# Value API step V2a-1a: value slots of int, float, bool and None (proposal)

Status: proposal, revision 2, 2026-11-04, after
[review 1](../reviews/value-api-v2a1a-review-1.md) ("accept with changes")
and the coordinator's rulings on it. It specifies parts i and ii of V2a-1a
([value-api-v2a0.md](value-api-v2a0.md) §1.4) and sketches part iii, which
waits for M3, as an interface note (§9).

Base: the shared runtime tree `build/worktrees/pyvalue-rc-universal` at
A5b-3 (`c71a3ca6aa2`: no tagged mortal borrows; `_Py_VALUE_ARG(o)` is
`PyValue_FromObjectBorrow(o)`, inline, which unboxes exact ints and floats),
with V2a-0 part 2 (`build/pyjit-link-wave1/v2a0-notes.md`, `T_2f`), which the
coordinator is integrating there. Names are W1a's (`PyValue_FromObjectSteal`).
Counts and line numbers are from that tree. V2a-1a starts when V2a-0 part 2
lands (§1.7's T7).

### Changes after review 1

| # | Finding | Change | Section |
|---|---|---|---|
| 1 | Digit view is a second form | Replaced by digit spans (`_PyLongSpan`, GMP's `mpn` layer, V8's `Digits`) and a result builder (`_PyLongOut`). No `PyLongObject` header is ever built for an inline int. The word path widens to heap ints of up to 2 digits | D2, D3, §1.2, §1.3 |
| 2 | Slot bodies call thunks; the entry primitive can recurse | Every object-extractor and object-slot call in the converted bodies is routed to a typed kernel or value form (table §2.2). The entry primitives call only kernels. The new extractor thunks are marked now in the step's own units | D13, §2.2 |
| 3 | Path length; D9 against D14 | Rule R-inline: a dispatcher may call int's or float's value function directly for an inline operand. Applied in hash, `RichCompareBool` and `WriteRepr`; `compare_inline` deleted; path lengths restated | D14, §2.5, §5 |
| 4 | Extractor rows incomplete | 20 rows, 18 from one macro, including the unsigned, mask, `AsLongLongAndOverflow`, fixed-width, `AsNativeBytes` and `AsVoidPtr` forms; `AsLongLong` and `AsInt64` separate. Rows that require an int are named `_PyValue_IntAs…` | §2.4 |
| 5 | `PyValue_FromBool(int)` truncates | `PyValue_FromBool(bool)` | §2.3 |
| 6 | Collisions | M3 uses `PyValue_FromInt64` (coordinator); M3b after V2a-1a on the `class int`/`class float` Clinic lines; the order V2a-1a, M1b, M2 extends to `abstract.c` and `object.c`; four files leave W1b for W2; direct slot calls in four modules recorded as T boxes | §5, §7 |
| 7 | Estimate | Recomputed: 15.5 agent-hours, lands near T14; I1 to about T15 | §8 |
| 8–10 | Notes | Box table: `pow` allocations, `str(x)` split, warm-up; `object.__eq__` returns `NotImplemented`; the NoneType pair row dropped (object's slot becomes `_Py_BaseObject_RichCompare`); 52 `hold()` call lines; the contract line in `pycore_value_ops.h` | §3, §2.1, §2.5 |

## 0. Decisions

| # | Decision |
|---|---|
| D1 | int's value slots dispatch on representation: the **word path** when both operands have a word (an inline int, or a heap int of at most 2 digits, below 2^60: `bool`, subclass instances, the range 2^51 to 2^60); the **span path** otherwise; `NotImplemented` for a non-int, as `CHECK_BINOP` |
| D2 | **Digit spans.** Digit kernels take `_PyLongSpan {const digit *d; Py_ssize_t n; int neg}`. A heap int presents its `ob_digit`; an inline int a local `digit[2]`. A span is not an object, so nothing can escape |
| D3 | About 10 leaf kernels take spans; results are built by `_PyLongOut`, which creates a heap int only when the result's size bound exceeds two digits or the value leaves the inline range (R27's too-large int). Upstream's object entry points become one-line wrappers; int's object slots become `_Py_VALUE_THUNK_*` thunks of the value slots |
| D4 | Mixed int and float: int's slots return `NotImplemented` for a float; float's slots read an int as a double: an inline int exactly, a heap int by the span kernel `_PyLong_SpanAsDouble` (upstream's `OverflowError`) |
| D5 | `bool` is never inline. It inherits int's value sides through the pass; its own `&`, `|`, `^`, `~` get value bodies that call int's value functions |
| D6 | `NoneType` defines no value slot (finding 6 of review-v2a0-1). `object`'s compare thunk is renamed `_Py_BaseObject_RichCompare`, the pointer NoneType and the sentinel type already name, so the pass gives them object's value side; no pair row |
| D7 | `object` gets value sides for `tp_richcompare` and `tp_str`; `str` gets its class-N compare side (`_Py_VALUE_NOTIMPL_RICHCMP`, one line), pulled forward from W2 |
| D8 | `value_ops.c`: `hold()` is dropped (every caller owns, §2.5); the arms become R-inline calls |
| D9 | Word kernels are `static inline` in `pycore_long.h`, shared by int's slots and, from I1, the typed handlers; overflow goes to span kernels. `_PyValue_MultiplyInt64` and `_PyValue_LshiftInt64` move to `longobject.c` and stop calling `PyNumber_Multiply`, a thunk, on heap temporaries |
| D10 | Constructors: `PyValue_FromInt64`, `PyValue_FromDouble` (exist); new `PyValue_FromUInt64`, `PyValue_FromBool(bool)`, `_PyValue_FromDoubleIntegral`. No `PyValue_FromSsize_t` (M3 uses `PyValue_FromInt64`) |
| D11 | Extractors: one macro, one row per object extractor, each keeping that function's contract and text; inline words read directly, heap ints through span kernels; the object extractors become thunks of their rows |
| D12 | **Rule "no thunk from the step's own bodies."** No converted body and no entry primitive calls a compatibility thunk. A unit opt-in, `Py_VALUE_MARK_NUMBERS`, marks the new thunks `Py_VALUE_COMPAT` in `longobject.c`, `floatobject.c`, `boolobject.c` and `pyvalue.c` now, so the compiler lists any such call; W2 marks them everywhere and deletes the opt-in |
| D13 | **Rule R-inline.** An inline value's type is int or float, fixed by the encoding. A dispatcher may call that type's value function directly for an inline operand, through the `static inline` definition that the type's `tp_value` also stores: one implementation, no second guard form. No other inline arm exists in a dispatcher |
| D14 | `_PyValue_BinaryOps[]`, the dispatcher table that I1's generic `BINARY_OP` indexes by `oparg`, is defined here in `abstract.c` |

## 1. How an operand reaches int's and float's slots

### 1.1 Operand pairs

| Operands | Slot | Path | Boxes |
|---|---|---|---|
| two words (inline, or heap below 2^60) | int | word kernel; a result outside the inline range is a new heap int | 0 |
| inline int, heap int ≥ 2^60 | int | compare and shift counts by sign and size alone; arithmetic by span kernels over a word span and the heap span | 0 |
| int (any), float (inline, NaN, subclass) | int: `NotImplemented`; then float | float reads both as doubles; `PyValue_FromDouble` | 0 |
| float, heap int beyond 2^1024 | float | `_PyLong_SpanAsDouble` raises `OverflowError`; compare keeps `float_richcompare`'s exact path (§2.2) | 0 |
| int or float, anything else | `NotImplemented`; the other type's slot | value slot if set, else the adapter | the adapter's, until W2 |

A NaN result is a heap float, the float with no inline form (R27). The
typed handlers (`BINARY_OP_ADD_INT`, `_MULTIPLY_FLOAT` …) still take two
inline operands before any dispatch.

### 1.2 Digit spans

```c
/* Include/internal/pycore_long.h */
typedef struct {
    const digit *d;     // magnitude, least significant first; d[n-1] != 0
    Py_ssize_t n;       // digit count, 0 for zero
    int neg;
} _PyLongSpan;

static inline _PyLongSpan _PyLong_SpanOf(const PyLongObject *op);            // any PyLong_Check
static inline _PyLongSpan _PyLong_SpanOfWord(int64_t v, digit buf[2]);        // |v| < 2^60
static inline _PyLongSpan _PyLong_SpanOfValue(PyValueBorrow v, digit buf[2]); // an int value

/* Results. bound is the kernel's size bound in digits. */
typedef struct { digit small[2]; PyLongObject *heap; digit *d; } _PyLongOut;
static inline digit *_PyLong_OutReserve(_PyLongOut *out, Py_ssize_t bound); // small if bound <= 2,
                                                       // else _PyLong_New(bound); NULL: MemoryError
PyValue _PyLong_OutValue(_PyLongOut *out, Py_ssize_t n, int neg);   // normalise; inline if it fits
PyLongObject *_PyLong_OutObject(_PyLongOut *out, Py_ssize_t n, int neg); // working objects only
void _PyLong_OutAbandon(_PyLongOut *out);
```

`_PyLong_OutValue` returns an inline int when the normalised result fits;
otherwise the heap int, created from `small` only if the value lies between
2^51 and 2^60 (a too-large int). A reserved heap result that normalises into
the inline range (`2**70 - (2**70 - 1)`) is freed: a working allocation sized
by the bound, as upstream's, never a conversion of an inline value.
`_PyLong_OutObject` gives the digit layer's internal working objects
(`long_pow`'s accumulators, `_PyLong_GCD`'s copies), which never become
values.

Leaf kernels (static in `longobject.c`):

| Kernel | Span signature | Note |
|---|---|---|
| `x_add` | `Py_ssize_t x_add(_PyLongSpan a, _PyLongSpan b, _PyLongOut *out)` | magnitudes |
| `x_sub` | `Py_ssize_t x_sub(_PyLongSpan a, _PyLongSpan b, _PyLongOut *out, int *neg)` | |
| `x_mul` | `Py_ssize_t x_mul(_PyLongSpan a, _PyLongSpan b, _PyLongOut *out)` | `k_mul` stays on objects; an inline operand has 2 digits, so `k_mul` always sends it here |
| `divrem1` | `Py_ssize_t divrem1(_PyLongSpan a, digit n, _PyLongOut *q, digit *rem)` | |
| `x_divrem` | `int x_divrem(_PyLongSpan v, _PyLongSpan w, _PyLongOut *q, _PyLongOut *r)` | its working copies stay internal |
| `long_bitwise` | `Py_ssize_t long_bitwise(_PyLongSpan a, char op, _PyLongSpan b, _PyLongOut *out, int *neg)` | |
| `long_compare` | `Py_ssize_t long_compare(_PyLongSpan a, _PyLongSpan b)` | |
| `long_true_divide` digit path | `double _PyLong_SpanTrueDivide(_PyLongSpan a, _PyLongSpan b)` (-1.0 and an exception) | |
| `long_pow` reads | `PyValue long_pow_spans(_PyLongSpan a, _PyLongSpan b, const _PyLongSpan *c)` | copies the base into a working object, as upstream allocates per multiply; a modulus of at most 2 digits builds the result in `small` |
| decimal string | `long_to_decimal_string_internal(_PyLongSpan a, PyObject **str, _PyUnicodeWriter *w, …)` | |

`_pylong`'s paths (`int_divmod`, `int_to_decimal_string`) stay on objects:
their thresholds are 300 and 1,000 digits, which an inline operand never
reaches. Readers with spans (`_PyLong_SpanAsDouble`, `_PyLong_SpanNumBits`,
`_PyLong_SpanAsInt64AndOverflow`, `_PyLong_SpanAsNativeBytes`) serve §2.2
and §2.4.

**Later readers reach digits the same way.** M3b's Clinic `self` of
`class int` (27 blocks) is a `PyValueBorrow`; `int.bit_length`,
`to_bytes`, `__format__` and `__round__` take `_PyLong_SpanOfValue(self, buf)`
and a span kernel (`_PyLong_SpanNumBits`, `_PyLong_SpanAsNativeBytes`, the
formatter). `math.gcd`, `isqrt` and `comb` (W4) and `%d` (W2) do the same.
Workers in those waves meet only spans; no stack object exists to escape.

Prior art. GMP has both `mpz_roinit_n` (a read-only `mpz` over limbs, for
callers that do not own the kernels) and the `mpn` layer (spans); we own the
kernels, so we take `mpn`. V8's `src/bigint` algorithms take
`Digits`/`RWDigits` spans; Smis and BigInts never mix in JavaScript. LuaJIT
has no big integers. Rejected: int64 variants of each kernel (a second
implementation) and heap temporaries (core boxes).

### 1.3 Word kernels (D9)

```c
/* pycore_long.h: |a|, |b| < 2^60, so add and subtract cannot overflow int64.
   Result owned; PyValue_NULL with MemoryError or the operation's error. */
static inline bool _PyLong_WordOperands(PyValueBorrow v, PyValueBorrow w,
                                        int64_t *a, int64_t *b);
static inline PyValue _PyLong_WordAdd(int64_t a, int64_t b);       // PyValue_FromInt64
static inline PyValue _PyLong_WordSubtract(int64_t a, int64_t b);
static inline PyValue _PyLong_WordMultiply(int64_t a, int64_t b);  // overflow: x_mul over word spans
static inline PyValue _PyLong_WordFloorDivide(int64_t a, int64_t b); // ZeroDivisionError
static inline PyValue _PyLong_WordRemainder(int64_t a, int64_t b);
static inline PyValue _PyLong_WordLshift(int64_t a, int64_t n);    // ValueError n < 0; overflow: span shift
static inline PyValue _PyLong_WordRshift(int64_t a, int64_t n);
/* And, Or, Xor, Negative, Invert, Absolute, Compare, Hash, Bool. */
```

`_PyValue_MultiplyInt64` and `_PyValue_LshiftInt64` keep their names and
their two `bytecodes.c` callers and move to `longobject.c` over word spans.

## 2. Interfaces and per-type changes

### 2.1 Value slots per type

| Type | Rows (functions) | Object side |
|---|---|---|
| `int` | 24 (22): the 18 number rows `nb_add` … `nb_float`; `nb_positive`, `nb_int`, `nb_index` (one function); `richcompare`, `hash`, `repr` | 22 thunks; `tp_repr` becomes `long_repr`, the thunk |
| `float` | 17 (16): 14 number rows (`nb_positive` and `nb_float` share one), `richcompare`, `hash`, `repr` | 16 thunks |
| `bool` | 4: `nb_and`, `nb_or`, `nb_xor`, `nb_invert` | 4 thunks; `bool_repr` keeps value side 0 |
| `object` | 2: `richcompare`, `str` | 2 thunks: `_Py_BaseObject_RichCompare` (now `object`'s own slot, replacing the static wrapper `object_richcompare`), `object_str` |
| `NoneType`, sentinel | 0 own | inherit `object`'s compare pair by the pass: same object slot |
| `str` | 1: `richcompare`, class N | `PyUnicode_RichCompare` unchanged |

int by row group (word path / span path): `+ - *` scaled or int64 arithmetic
/ `x_add`, `x_sub`, `x_mul`; `// % divmod` C division with the floor
correction, `divmod` a tuple of two values / `divrem1`, `x_divrem`; `/` both
below 2^53 `(double)a / (double)b` / `_PyLong_SpanTrueDivide`; `**` two
operands with exponent ≥ 0 by squaring with an overflow test, a negative
exponent to float's value `nb_power` (`_PyFloat_ValuePower`), three operands
`long_pow_spans`; `<< >>` with `ValueError` for a negative count / sign and
size, or `long_lshift1` over spans; `& | ^ ~ - abs` / `long_bitwise`;
`+x int() index` `PyValue_DUP` / `_PyLong_OutValue` of the span for a
subclass; `bool float hash` `!= 0`, exact double, `i` with -1 → -2 / span
readers; compare by int64 / sign and size; `repr` over a word span.

float: `float_operand(PyValueBorrow, double *)` gives 1 (inline float or
int; heap float; heap int by `_PyLong_SpanAsDouble`), 0 (`NotImplemented`)
or -1. Two inline operands take `PyValue_FloatOperandValues` first.
`float_rem`, `float_floor_div`, `float_divmod`, `float_pow` and `float_repr`
become double kernels. `nb_int` is `_PyValue_FromDoubleIntegral(trunc(d))`
(its `ValueError` and `OverflowError`); `hash` is `_Py_HashDouble(NULL, d)`
for an inline float, the object for a NaN.

`object`'s compare, upstream's rules exactly: `==` gives `True` for the same
word (`PyValue_Is`), otherwise `NotImplemented`. `!=` calls the receiver
type's compare pair with `Py_EQ` (through `object.c`'s `call_richcompare`,
exported internally) and inverts a result that is not `NotImplemented`
through `_PyValue_IsTrue`; with no `tp_richcompare` it gives
`NotImplemented`. Other operators give `NotImplemented`. `object_str_value`
calls the type's repr pair.

### 2.2 Calls inside the converted bodies (D12)

| Body | Call today | Route |
|---|---|---|
| float's slots, a heap int operand (`CONVERT_TO_DOUBLE`) | `PyLong_AsDouble` | `_PyLong_SpanAsDouble(_PyLong_SpanOf(i))`, the kernel of `PyLong_AsDouble` |
| `float_richcompare` (`floatobject.c:404–500`) | `PyLong_GetSign`, `_PyLong_NumBits`, `PyLong_AsDouble`, `PyLong_FromDouble`, `PyObject_RichCompareBool(vv, w, op)` (`:431`, `:491–495`) | span sign and `_PyLong_SpanNumBits`; `_PyLong_SpanAsDouble`; `_PyValue_FromDoubleIntegral(intpart)`; `_PyValue_RichCompareBool(vv, w, op)`, so an int subclass `w` still gets its reflected `__lt__` |
| bool's four (`boolobject.c:83`, `:90`, `:98`, `:106`) | `PyLong_Type.tp_as_number->nb_and(a, b)` and the rest, thunks after D3 | `_PyLong_ValueAnd`, `_PyLong_ValueOr`, `_PyLong_ValueXor`, `_PyLong_ValueInvert`, declared in `pycore_long.h` |
| `long_float` (`longobject.c:5912`) | `PyLong_AsDouble` | `_PyLong_SpanAsDouble` |
| int's `nb_power`, negative exponent | `PyFloat_Type.tp_as_number->nb_power` | `_PyFloat_ValuePower` |
| `_PyObject_ValueIdentity` (`pyvalue.c:159`), behind every entry primitive | `PyLong_AsLongLongAndOverflow(op)`: once a thunk, it recurses (thunk → `_Py_VALUE_ARG` → `_PyObject_ValueIdentity` → thunk) | `_PyLong_SpanAsInt64AndOverflow(_PyLong_SpanOf(op))`. Rule: entry primitives call only typed kernels |
| `_PyValue_MultiplyInt64`, `_PyValue_LshiftInt64` (`pyvalue.c:168–207`) | `PyLong_FromLongLong` temporaries, `PyNumber_Multiply`, `_PyLong_Lshift` | span kernels into `_PyLongOut` |
| `_PyValue_Float`, `_PyValue_AsSsize_t` (`abstract.c`) | `PyLong_AsDouble`, `PyLong_AsSsize_t`, `PyFloat_FromDouble` into `_PyValue_FromObjectResult` | `_PyValue_IntAsDouble`, `_PyValue_IntAsSsize_t`, `PyValue_FromDouble` |

### 2.3 Constructors (ii)

In `pycore_value.h` (R26). Owned value, or `PyValue_NULL` with `MemoryError`.

| Value form | Replaces (core + `Modules/` calls) | Out of range |
|---|---|---|
| `PyValue_FromInt64(int64_t)` (exists) | `PyLong_FromLong` 157 + 763, `FromSsize_t` 84 + 170, `FromLongLong` 12 + 68, `FromInt64` 3 + 22, `FromInt32` 1 + 9 | `_PyValue_BoxInt64`: a heap int |
| `PyValue_FromUInt64(uint64_t)` | `PyLong_FromSize_t` 10 + 42, `FromUnsignedLong` 16 + 131, `FromUnsignedLongLong`, `FromUInt32`, `FromUInt64` | a heap int |
| `PyValue_FromDouble(double)` (exists) | `PyFloat_FromDouble` 54 + 139 | a NaN: a heap float |
| `PyValue_FromBool(bool)` | `PyBool_FromLong` 82 + 166; C converts any scalar to `bool` as `!= 0`, so `flags & (1L << 40)` stays true | – |
| `_PyValue_FromDoubleIntegral(double)` | `PyLong_FromDouble` in value contexts | a heap int; `ValueError` for NaN, `OverflowError` for infinity |

### 2.4 Extractors (ii)

`_Py_VALUE_INT_EXTRACTOR(name, ctype, min, max, mode, index, kernel)` in
`longobject.c`, declared and exported in `pycore_long.h`. A row reads an
inline int in range directly; out of range it raises the object function's
`OverflowError` text, or wraps for a mask row. A heap int goes to the span
kernel that is the object function's digit loop. A non-int goes through
`_PyValue_Index` if the row accepts `__index__`, else raises the object
function's `TypeError`. An inline float is never boxed for a message. Rows
that require an int are named `_PyValue_IntAs…`, so `_PyValue_AsSsize_t`
(index) and `_PyValue_IntAsSsize_t` (int only) differ by a word.

| Value form | Object function (its thunk) | `__index__` | Core + `Modules/` |
|---|---|---|---|
| `_PyValue_AsInt` | `PyLong_AsInt` | yes | 81 + 570 |
| `_PyValue_AsLong` | `PyLong_AsLong` | yes | 46 + 116 |
| `_PyValue_AsLongLong` | `PyLong_AsLongLong` ("int too big to convert") | yes | 12 + 21 |
| `_PyValue_AsLongAndOverflow(v, int *)` | `PyLong_AsLongAndOverflow` | yes | 21 + 34 |
| `_PyValue_AsLongLongAndOverflow(v, int *)` | `PyLong_AsLongLongAndOverflow` | yes | 4 + 23 |
| `_PyValue_AsInt32(v, int32_t *)`, `AsInt64`, `AsUInt32`, `AsUInt64` | `PyLong_AsInt32`, `AsInt64`, `AsUInt32`, `AsUInt64` ("… to C int64_t") | as the object function | 6 + 17 |
| `_PyValue_IntAsUnsignedLong`, `IntAsUnsignedLongLong` | `PyLong_AsUnsignedLong`, `…LongLong` | no | 10 + 36 |
| `_PyValue_AsUnsignedLongMask`, `AsUnsignedLongLongMask` | the two mask forms (wrap, no overflow) | yes | 2 + 15 |
| `_PyValue_IntAsSsize_t`, `IntAsSize_t` | `PyLong_AsSsize_t`, `PyLong_AsSize_t` | no | 64 + 182 |
| `_PyValue_IntAsDouble` | `PyLong_AsDouble` | no | 8 + 9 |
| `_PyValue_AsNativeBytes(v, void *, Py_ssize_t, int)` | `PyLong_AsNativeBytes` | per flags | 10 + 57 |
| `_PyValue_AsVoidPtr` | `PyLong_AsVoidPtr` | yes | 5 + 80 |
| `_PyValue_AsDouble` (own body) | `PyFloat_AsDouble`: inline float or int directly; else the `nb_float` pair, then `nb_index` | `__float__`, `__index__` | 25 + 83 |
| `_PyValue_AsSsize_t(v, exc)` (exists) | `PyNumber_AsSsize_t` | yes | 33 + 25 |

20 rows, 19 new, 18 of them from the macro; the fixed-width rows keep the
object functions' out-parameter shape. No handler calls a row with an out-parameter (A3's R-a).

### 2.5 `value_ops.c` and R-inline (D8, D13)

| Function | V2a-0 | V2a-1a |
|---|---|---|
| `_PyValue_Hash` | inline int and float arms (second implementation), `hold()`, port | R-inline: an inline int calls `_PyLong_ValueHash`, an inline float `_PyFloat_ValueHash`; else the `PyObject_Hash` port |
| `_PyValue_RichCompareBool` | identity, `compare_inline` (doubles), `hold()` ×2, dispatch | identity; two inline operands: the left type's value compare, then the right's reflected only on `NotImplemented` (int against float), no recursion guard since neither runs Python; else dispatch. `compare_inline` deleted |
| `_PyValue_RichCompareObject` | arm, `hold()` ×2 | dispatch and exit; deleted in W2 |
| `_PyValue_WriteRepr` | `hold()`, `_PyValue_Repr`, write | R-inline: an inline int writes its word span through the decimal kernel, an inline float through float's repr kernel; upstream's None and heap exact-int arms; else `_PyValue_Repr` |

`binary_op1` has no arm: after I1 the typed handlers cover its inline
callers.

**Owners.** `hold()` is dropped, not converted to `PyValue_DUP`: the
dispatcher contract (v2a0 §4.6, the caller holds the owner) holds at every
caller (review 1, finding 10, checked list, dict, set, cell, sort and repr
sites). There are 52 call lines (list 14, dict 17, tuple 7, set 6, slice 6,
structseq 1, cell 1). The contract line moves into `pycore_value_ops.h`
beside the dispatchers, where W2's workers read it.

## 3. The boxes V2a-0 added, and where each goes

`x = 1000`. "Before" is the remeasure before V2a-0, "V2a-0" review-v2a0-2's
measurement; "derived" rows follow the same code paths; later columns are
expected. C core, S slot adapter, T thunk, A call adapter.

| Operation | Before | V2a-0 | V2a-1a | After I1 | After W2/W4 | How |
|---|---:|---:|---:|---:|---:|---|
| `x == 'a'` | 1 | 3 (1 C, 2 S) | 1 C | 0 | 0 | int's and str's value sides return `NotImplemented`; the C box is the generic `COMPARE_OP` handler's |
| `'a' in [1000, 2000, 'a']` | 2 | 4 | 0 | 0 | 0 | the same two sides per mixed compare (2 without D7's `str` line) |
| `str(x)`, unspecialised | 1 | 2 | 1 A | 1 A | 0 | `object_str_value` calls int's repr; A is `str`'s call adapter (M3, W2) |
| `str(x)`, specialised (`CALL_STR_1`) | 1 | 2 | 1 C | 0 | 0 | I1 converts `CALL_STR_1` |
| `divmod(x, 7)` | 2 | 4 | 2 A | 2 A | 0 | word `divmod`; A: an object-form builtin (W2) |
| `pow(x, 2, 7)` | 3 | 7 | 3 A + 1 T | same | 0 | `long_pow_spans` builds the result in `small` (modulus of one digit): no allocation; T is the `PyNumber_Power` thunk's exit |
| `operator.add(x, 2000)` | 2 | 5 | 2 A + 1 T | same | 0 | word add; `_operator` in W4 |
| `repr([1000, 2000])` | 2 C | 2 S, a str per item | 0, no str | 0 | 0 | R-inline in `_PyValue_WriteRepr` |
| `x == None`, `x == object()` (derived) | 1 | 3 | 1 C | 0 | 0 | int's side, then `object`'s |
| C caller `PyNumber_Add(box1000, box2000)` (derived) | 0 | 3 (2 S, 1 T) | 1 T | 1 T | – | Q8's exit box, permanent for object-API callers |

Allocations. The T box of `pow` and `operator.add` stands where upstream's
`long_pow` or `long_add` allocated the result object: one allocation for
results from 257 to 2^50, as upstream, since `_PyLongOut` builds an in-range
result without one. `long_pow`'s working copies are upstream's.
`test_box_kinds` warms each statement up (or runs it with specialisation
off) before it asserts exact counts.

## 4. Interaction with I1 and the typed handlers

V2a-1a edits no handler, generator file or `ceval.c` (A3 step 2 owns them).

| Handler | Today | After V2a-1a | I1 |
|---|---|---|---|
| Generic `BINARY_OP` | `value_binary_operation`: boxes inline operands, calls `_PyEval_BinaryOps[oparg]` | the `PyNumber_*` thunk unboxes, a value slot answers, the exit boxes an inline result (T) | D14's `_PyValue_BinaryOps[]` with borrows: no box. `NB_SUBSCR` stays for I3 |
| `_COMPARE_OP` | two inline operands: `_PyValue_RichCompareObject`; else boxes and `PyObject_RichCompare` | the inline call reaches the value slots; `True`/`False` never box | `_PyValue_RichCompare`, then `_PyValue_IsTrue` for `oparg & 16` |
| `TO_BOOL`, `UNARY_NOT`, `POP_JUMP_IF_*` | `_PyValue_Truth` (`ceval_macros.h:533`), five arms | unchanged | `_PyValue_IsTrue`; `_PyValue_Truth` deleted (5 uses) |
| `_UNARY_NEGATIVE`, `_UNARY_INVERT` | inline arms then `PyNumber_Negative`; generated box then `PyNumber_Invert` | invert: C and T (V2a-0 added an S) | `_PyValue_Negative`, `_PyValue_Invert` |
| Typed int and float handlers | guards, word arithmetic, overflow helpers | overflow helpers are `longobject.c`'s (same names, no thunk) | bodies call D9's word kernels; I1 checks on the object code that the scaled single-overflow-test form survives inlining |

Dispatchers take borrows and return a `PyValue` or an `int` (R-a).

## 5. Box counters and path length

| §1.2 family | V2a-1a | Then |
|---|---|---|
| Generic `BINARY_OP` (1.47M, 10.2%) | S boxes from int and float go; C stays, plus one T per inline result | I1: 0 |
| `COMPARE_OP` mixed (0.76M, 5.2%) | S boxes go for int, float, bool, None, str and object-compare types; C stays | I1: 0 |
| Repr of numbers | 0 for int, float and None items | – |
| Kind S | the types still without value sides (W2; Python classes: iii) | W2's assertion |
| Kind T | 3 → 1 per object-API call with exact ints (Q8). Direct slot calls stay T until W4: `_json.c:1540`, `:1601`, `:1697`, `_decimal.c:7920–7923`, `mathmodule.c:1595`, `mathintegermodule.c:462` | W2, W4 |
| Uncounted temporaries | `_PyValue_MultiplyInt64`'s two heap ints per overflowing multiply: 0 | – |

Path length, read from the code, not measured:

| Path | Instructions |
|---|---|
| `_PyValue_Hash`, inline key (every `d[i]`, `i in s`, `s.add(i)`; the handlers specialise on the container, not the key) | about 5: a tag test and the inlined `_PyLong_ValueHash`, as V2a-0's arm |
| `_PyValue_RichCompareBool`, two inline ints (`list.index`, mixed sorts, tuple `in`) | about 10: identity, two tag tests, the inlined compare |
| the same, inline int against inline float | about 15: int's function returns `NotImplemented` (a tag test), float's compares |
| `_PyValue_RichCompareBool`, other operands | 40–60, two direct calls and one indirect call: upstream's shape (`_PyValue_RichCompare` with its recursion guard, `do_richcompare`, the slot, `_PyValue_IsTrue`) |
| Generic `BINARY_OP` with `bool` or mixed operands | one more indirect call for the `NotImplemented` round trip, as upstream |

## 6. Tests

Existing: `test_long`, `test_int`, `test_float`, `test_bool`, `test_index`,
`test_binop`, `test_pow`, `test_richcmp`, `test_compare`, `test_hash`,
`test_math`, `test_fractions`, `test_decimal`, `test_statistics`,
`test_builtin`, `test_unary`, `test_operator`, `test_enum`, `test_repr`,
`test_list`, `test_set`, `test_dict`, `test_json`, `test_capi`
(`test_long`, `test_float`, `test_number`), `test_value_slots`,
`test_pystats`, `test_sys`, `test_tools.test_check_value_api`.

New, in `test_value_slots.py` (helpers in `_testinternalcapi/value_slots.c`):

| Case | Expectation |
|---|---|
| `tp_value` of `int`, `float`, `bool`, `NoneType`, `object`, `str` | §2.1's rows set; bool's inherited rows equal int's; NoneType's and the sentinel's compare equal `object`'s |
| Extension static subtype of `int`: nothing / `nb_add` overridden | int's sides / `nb_add` 0, the rest int's |
| Every operator over {0, ±1, ±2^29, ±2^30, 2^50 − 1, −2^50, ±2^51, ±2^59, ±2^60, ±2^62, ±2^63, ±2^64, ±2^200} squared, both orders | equal to `_testinternalcapi.int_span_path(op, a, b)`, which runs the span kernels on both operands; exact type `int`; inline exactly when in range |
| `divmod(5, 2**70)`, `-5 % 2**70`, `pow(5, 1, 2**70)`, `I(2**40) // 3`, `2**70 - (2**70 - 1)` | upstream; the last is inline |
| `-2**50 // -1`, `-(-2**50)`, `abs(-2**50)`, `1 << 1000`, `-1 >> 100`, `x << -1`, `x // 0`, `pow(0, -1)`, `pow(2, 3, 0)`, `pow(x, -1, 7)`, `5 << 2**70` | upstream values and messages |
| `True + 5`, `5 & True`, `~True`, `True == 1`, `hash(True)`, `I(3) + 1` | upstream; `type(...) is int` where upstream |
| `1 + 2.5`, `2**60 + 2.5`, `10**400 + 1.0`, `2**53 + 1 == float(2**53)`, `I(2**53 + 1) < 2.0**53` with `I.__gt__` logged, `1000 < float('nan')`, `1 + F(2.0)` with `F.__radd__` | upstream; the reflected method runs |
| hashes at the edges, `hash(3) == hash(3.0)`, NaN | upstream |
| `repr`, `str` at the edges; `repr` of lists of them | upstream text |
| Extractor parity: each §2.4 row against its object function through `_testcapi`, on inline and heap ints, inline floats, out-of-range values, `__index__` and `__float__` objects, strings | same value, or the same exception type and text |
| Constructors at ±2^50, ±2^51, `INT64_MIN`, `2**64 − 1`, NaN; `PyValue_FromBool(1L << 40)` | as specified; `True` |
| `object.__eq__(1000, 1000)`, `object.__eq__(1000, 'a')`, `object.__ne__(1000, 'a')`, `None == 5` | `True`; `NotImplemented`; `NotImplemented`; `False` |
| `make check-value-api` with `Py_VALUE_MARK_NUMBERS` | no marked call in the four units |

`test_pystats.test_box_kinds` gains §3's rows after warm-up.

## 7. Files and collisions

Files: `Objects/longobject.c` (spans, `_PyLongOut`, kernels, 22 value
functions and thunks, extractor rows, moved helpers), `floatobject.c`,
`boolobject.c`, `typeobject.c` (`object`'s sides, the renamed compare),
`object.c` (`call_richcompare` exported internally), `unicodeobject.c` (one
line), `value_ops.c`, `abstract.c` (`_PyValue_Float`, `_PyValue_AsSsize_t`,
`_PyValue_BinaryOps[]`), `Python/pyvalue.c` (entry kernel, two helpers
out), `Include/internal/pycore_long.h`, `pycore_value.h`,
`pycore_value_ops.h`, `Tools/build/check_value_api.py` (the opt-in);
`Modules/_testinternalcapi/value_slots.c`, `Lib/test/test_value_slots.py`,
`test_pystats.py`.

| Work in flight | Shared | Resolution |
|---|---|---|
| V2a-0 part 2 | `typeobject.c`, `abstract.c`, `object.c`, `value_ops.c` | V2a-1a starts after it lands |
| M1b, M2 | `typeobject.c`, `abstract.c`, `object.c` | integrate in the order V2a-1a, M1b, M2 on all three |
| M3 | emits `PyValue_FromSsize_t` | uses `PyValue_FromInt64` (coordinator) |
| M3b | the `class int` and `class float` Clinic lines in `longobject.c`, `floatobject.c` | V2a-1a first; M3b then regenerates. Clinic `self` readers use spans (§1.2) |
| M3a | none | iii waits for it |
| W1b (ends about T9) | `longobject.c`, `floatobject.c`, `boolobject.c`, `unicodeobject.c` | these four leave W1b; W2 sweeps them |
| A3 step 2 | none | I1 takes D9's kernels and D14's table |
| W4 | `_json`, `_decimal`, `mathmodule.c`, `mathintegermodule.c` call the int and float object slots | T boxes until W4 (§5) |

## 8. Steps and estimate

Counts: about 10 span kernels and 4 span readers; `_PyLongOut`; about 15
word kernels; 22 int and 16 float value functions; 4 bool, 3 `object` and
`str`; 44 thunks; 9 routed call sites (§2.2); 19 new extractor rows over 6
digit loops; 3 constructors; 4 `value_ops.c` functions. Rates (v2a0 §1.6):
10 value-semantics functions an hour, 15 wrappers; the lead's pattern work
at about 5.

| Diff | Content | Who | Agent-h |
|---|---|---|---:|
| 1a-0 | interface note: span, `_PyLongOut`, word kernels, `float_operand`, extractor macro, names | lead | 0.5 |
| 1a-1 | span type, `_PyLongOut`, `x_add` as the pattern; word kernels; int skeleton with 4 rows (`+`, `//`, compare, `**`); entry kernel | lead | 2 |
| 1a-2 | the other 9 span kernels and 4 readers (13 / 10 = 1.3, plus `x_divrem`'s copies) | Sonnet | 1.5 |
| 1a-3 | int's other 18 value functions and 22 thunks (18 / 10 + 22 / 15 ≈ 3.3, less the skeleton's macros) | Sonnet | 2.0 |
| 1a-4 | float's 16 and its routed calls (1.6 + 0.3); bool's 4 bodies, `str`'s line (0.3) | Sonnet | 2.2 |
| 1a-5 | constructors; extractor macro, 18 macro rows and `_PyValue_AsDouble`, 6 loops as span kernels, thunks, the opt-in (0.5 + 37 lines / 30 + 6 / 10) | Sonnet | 2.3 |
| 1a-6 | `object`'s sides and rename, `value_ops.c` with R-inline, `abstract.c` reads, `_PyValue_BinaryOps[]` | lead | 1.25 |
| 1a-7 | §6's cases, `test_box_kinds` | Sonnet | 1.75 |
| Review of worker diffs; suites (GCC and clang debug, release, stats) | | lead | 2 |
| **Total** | lead 5.75, Sonnet 9.75 | | **15.5** |

Critical path: 1a-0 (T7–T7.5); 1a-1 (T7.5–T9.5), with 1a-2 beside it;
1a-3 after both (T9.5–T11.5), while the lead does 1a-6; then worker review
and suites (T11.5–T13.5). V2a-1a i–ii lands near T14, as review 1 computed. I1 (T8–T14) integrates after it, at about
T15. v2a0 §1.7 needs that edit; W2's pilot and core Clinic regeneration
(M3b's forms, 1.5 hours) still start at T12.

## 9. Part iii, after M3 (interface note)

| Item | Interface |
|---|---|
| Functions | Value sides of about 40 `slot_*` functions (13 binary by `SLOT1BIN`, `nb_power`, 7 unary, 13 in-place, `nb_bool`, `tp_richcompare`, `tp_hash`, `tp_repr`, `tp_str`), with the table's signatures. `slotdef_values[]` already has a value function per slotdef (0 so far); iii fills it, so `update_one_slot` installs both sides unchanged |
| Unary sides too | A Python receiver is never inline, but a dunder's result can be: the object form boxes it at the object vectorcall's exit |
| Call | `vectorcall_method_value(PyObject *name, const PyValue *args, size_t nargsf)` and `vectorcall_maybe_value`, static in `typeobject.c`: the special-method lookup on `PyValue_TYPE(args[0])` (not M3's `_PyValue_VectorcallMethod`, which looks up on the instance), then M3a's `_PyValue_Vectorcall`. M1b-d's five and M2-d's three value `slot_*` reuse it |
| `SLOT1BINFULL` | Keeps its two-sided logic; "the other type's slot is this function" compares object slots (v2a0 D8) |
| Results | `__bool__` must return a bool; `__hash__`'s inline int is its own hash (-1 → -2), a heap int goes through int's value hash; `__index__`, `__int__`, `__float__` keep the dispatchers' checks (R17) |
| Object sides | `slot_*` object functions become thunks by `_Py_VALUE_THUNK_*`, keeping their names |
| Round trip (R9) | A `_testcapi` test pins `SetAttr`/`GetAttr`, `PyList_Append`/`GetItemRef`, a thunk to the slot adapter, and `l[0] is l[0]` |
| Needs, estimate | M3a's `_PyValue_Vectorcall` and value frames; 2.5 agent-hours |

## 10. Risks and open questions

| # | Question or risk | Recommendation |
|---|---|---|
| Q1 | `_PyLongOut` frees a heap result that normalises into the inline range | A working allocation sized by the bound, as upstream's; no inline value is ever converted. Exact bounds would need a second pass |
| Q2 | R-inline admits arms in three dispatchers | Each calls the type's own value function; the rule names why (the encoding fixes the type). Further uses need the rule cited |
| Q3 | The word kernels take int64 up to 2^60, the handlers' scaled int51 form uses one overflow test | I1 measures both on the object code; if the scaled form is faster, the kernels take it and the 2-digit heap ints use the span path |
| Q4 | `_Py_BaseObject_RichCompare` becomes `object`'s own slot | One initialiser and odict's comment; no identity test names `object_richcompare` |
| Q5 | `object.__eq__` on two equal heap ints from C now returns `True` | Value identity (R9) |
| Q6 | 19 extractor rows keep the object API's per-type shape | Messages are behaviour; the rows come from one macro |
| R1 | Word and span edges (floor signs, shift counts, `NotImplemented` order) | The span-path parity sweep and the full suite |
| R2 | 13 span-kernel signatures change at once | `x_add` first by the lead as the pattern; the worker's diff is reviewed against it; `test_long` per kernel |
