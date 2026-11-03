# Value-aware regex code-word extraction

2026-11-01. **Design only; implementation is held pending current site evidence
and independent review.** This is the native track's one selected candidate,
following [the Fable native audit](../fundamental-runtime-design/fable-native-audit-20261101.md).
No runtime/test patch, target, compiler, census or timing ran. Shared stats
preparation belongs to the qualification owner; this design creates no helper
or competing census. Prefix/allocator work remains held.

Hypothesis: `_sre.compile` unnecessarily converts nonnegative inline int32 code
words to PyLong objects before immediately extracting their unsigned value.
Reading those words directly can remove conversion calls and, outside the
small-int cache, temporary allocations. It may improve the registered regex
compiler consumer; neither current site dominance nor elapsed benefit is proved.

## Actual path and scope

The maintained source inspected here is HEAD
`129131574a8de0bff1c1590666ca9ea258f6e4d6`. Its source layers are:

1. `Lib/re/_compiler.py:395–416` generates a code list, constructs group maps,
   and calls `_sre.compile(pattern, flags, code, groups, groupindex, indexgroup)`.
   The registered `regex_compile` kernel repeatedly purges caches and compiles
   its fixed corpus; it does not call a synthetic numeric microbenchmark.
2. `Modules/_sre/clinic/sre.c.h:1110–1206` parses six arguments. Flags already use
   `PyLong_AsIntValue`; groups use `PyNumber_IndexValue` then
   `PyLong_AsSsize_tValue`. Code must be a list, groupindex a dict and indexgroup
   a tuple; subclasses are admitted by those checks.
3. `_sre_compile_impl:1764–1807` takes `n = PyList_GET_SIZE(code)`, allocates a
   variable-sized PatternObject, initializes its traversed object fields to
   NULL, and fills its code array. Every iteration currently calls
   `PyList_GET_ITEM(code, i)` then `PyLong_AsUnsignedLong`.
4. `Include/cpython/listobject.h:39–57` stores/reads PyValue. GET_ITEM invokes
   `_PyValue_ObjectOrAbort`, requesting a numeric box only for an inline value.
   `Objects/longobject.c:714–766` then reads an integer object's digits.
5. The loop preserves a distinct conversion-error check, SRE_CODE store and
   narrowing check. After tracking the PatternObject, it returns any conversion
   error before pattern/buffer checks, sets remaining metadata and calls
   `_validate`. `SRE_CODE` is unsigned Py_UCS4; validator failure is RuntimeError.

No maintained `PyLong_AsUnsignedLongValue` was found. Existing signed value
converters in `Objects/abstract.c:2185–2204` do not supply the required unsigned
negative/overflow contract. The selected loop has not already become value-aware.

## Minimum candidate

Change only the code-word extraction inside `_sre_compile_impl`:

```c
PyValue item = PyList_GET_VALUE(code, i);
unsigned long value;
if (PyValue_IsInlineInt(item) && PyValue_Int32(item) >= 0) {
    value = (unsigned long)PyValue_Int32(item);
}
else {
    PyObject *o = PyList_GET_ITEM(code, i);
    value = PyLong_AsUnsignedLong(o);
}
/* Preserve the existing conversion-error, store and narrowing checks. */
```

This is design pseudocode, not an applied patch. All supported C unsigned-long
widths can represent nonnegative int32. Keep the shared check for the valid
`ULONG_MAX` sentinel versus an actual exception. Preserve the exact SRE_CODE
round trip and error text rather than substituting an unsigned mask conversion.

Only **nonnegative inline int32** is admitted. Negative inline ints, inline
floats, bool, int subclasses, NaN and larger boxed integers take the literal old
object-access/conversion route. Re-reading the slot on this fallback introduces
no intervening callback or allocation before the old read. It avoids duplicating
negative/type/overflow behavior and avoids a new general unsigned-long API.
No Clinic regeneration, slot typedef, method convention, object layout or
extension ABI change is required. Future source scope is `Modules/_sre/sre.c`,
focused `Lib/test/test_re.py` tests and issue evidence only.

The old box in the admitted case does not escape the scalar converter. Removing
it does not change an observable language value or a retained C object's
identity. It intentionally removes that allocation/failure opportunity; it
cannot preserve a MemoryError/abort caused solely by making an unnecessary
temporary box. All remaining fallible allocation and fallback paths retain
their current behavior and rooting obligations.

## Index conversion, mutation and roots

There is **no `__index__` fallback for code-list elements** today.
`PyLong_AsUnsignedLong` rejects a non-int with TypeError before invoking an
index hook. An int subclass is accepted from its stored digits without calling
its overridden `__index__` or `__int__`. Introducing `PyNumber_IndexValue` in
this loop would widen acceptance and create callback/list-mutation hazards.
It is explicitly excluded from the candidate.

There is real index reentry in the Clinic arguments **before** the implementation:
flags may call an index conversion, and groups explicitly do so after the code
argument has been checked. Such a callback may mutate the code list or raise.
`n` is sampled only after parsing finishes, so the implementation must observe
the updated list and preserve argument/error order. Do not hoist its size,
items pointer or element values into the wrapper or before that callback.

List subclasses retain direct macro semantics: neither `__len__` nor
`__getitem__` is called by the old loop or the proposed read. The successful
integer conversion path runs no Python code and releases no GIL. Allocator/OOM
collection may occur on fallback boxing and error construction; the input list,
the actual boxed fallback object and the partially constructed PatternObject
must remain live under the existing opaque-C-frame/argument rooting contract.
The candidate adds no precise-frame declaration, C root span or lifetime waiver.
Do not keep an `ob_item` pointer across a collecting call or replace a raw box
root with a canonical inline value. Read each element by index as currently.

The PatternObject fields used by traversal are initialized before the loop;
tracking, error cleanup and later metadata initialization remain in their
existing order. No list snapshot, owner registry or new reclamation policy is
needed because no new element callback is introduced. If later design demands
arbitrary index-like elements, its reentry/shrink/reallocation contract is a
different change requiring separate review.

## Behavior and causal oracles

Use direct `_sre.compile(None, 0, code, 0, {}, ())` calls, not just `re.compile`.
Build minimal valid code arrays from `re._constants.LITERAL` and `SUCCESS`;
the literal argument is an unsigned code word and the current validator imposes
no character-range check there. Match ordinary valid Unicode literals, and
only compile the intentionally out-of-Unicode-range numeric boundary arrays.
Do not execute such arrays as regex matching tests.

| Case family | Required observation |
|---|---|
| Cached-range inline ints: 0, 1, 1024 as literal payloads | Same compiled pattern/matching behavior; old conversion requests may reuse a singleton, so no fresh-allocation saving is claimed |
| Uncached inline ints: 1025, 65535, 0x10ffff | Same matching; source-bound counters prove this exact old read requested boxes and candidate bypassed them |
| INT32_MAX | Compile-only equality of acceptance; fast path's cast and SRE_CODE storage remain exact |
| INT32_MAX+1, SRE_CODE_MAX and native ULONG_MAX | Object fallback remains; test architecture-dependent acceptance versus narrowing/long overflow with native widths derived from existing test helpers |
| SRE_CODE_MAX+1 and 2**128 | Preserve the first applicable unsigned conversion or SRE narrowing OverflowError; do not overwrite a conversion error |
| Negative -1 and -6 | Old cached/uncached negative routes keep the existing negative-to-unsigned OverflowError |
| Bool and an int subclass whose index/int hooks raise | Underlying digits accepted where the resulting code is valid; hooks remain uncalled |
| A non-int object whose `__index__` mutates the list and returns a valid int | TypeError; hook and mutation must remain uncalled |
| Non-int index hook that raises, strings, None, float +/-0, infinity and NaN | Retain rejection and exact first-error behavior; no new implicit index or float coercion |
| List subclass with hostile len/getitem overrides | Overrides remain uncalled; direct storage semantics unchanged |
| Flags/groups index callback mutates or clears code | Callback executes in the existing parsing order, before size capture; compare valid updated code or invalid-code error rather than retaining a stale length |
| Flags/groups index callback raises or an earlier/later argument has a wrong container type | Preserve which error occurs first, before any code-word conversion |
| Early invalid code item followed by another failure and invalid pattern | First conversion error wins, extending existing `test_dealloc` coverage for the overwritten-TypeError regression |
| Constructor and remaining fallback/error OOM plus forced collection | No dangling list/item/PatternObject; actual fallback objects remain raw-rooted, partially initialized traversal remains safe; no blanket OOM skip |

The integer/no-index expectations are source contracts, not new behavior to
grant. The existing `test_re.test_dealloc` already checks huge unsigned overflow,
wrong argument types and preservation of a string item's TypeError. Keep that
coverage and add only the focused missing matrix. Exact method/child IDs and
fault mechanism must be frozen during a later source grant; no tests ran here.

The causal source arm restores the exact old extraction for every element,
with the same wrapper, corpus, tests and common checks. This is a temporary
source ablation with independently bound fresh products, never a permanent
runtime flag. Require current native inspection to show direct inline
extraction in the candidate, the old boxing call on the ablation's nominated
path, and unchanged fallback/narrowing/error exits. The generated evaluator
must remain byte-identical at source; avoid a coupled interpreter experiment.

For known uncached inline input, software counters should show the old box-site
request and its disappearance on the candidate. Cached cases distinguish removed
conversion work from removed heap allocation. Output/error/root tests cannot
be substituted with a lower total box counter. If native code already eliminates
the selected box, the proposed mechanism has failed before timing.

## Evidence required before an implementation grant

Historical regex totals (991,905 conversions/loop at b3b30c90830) support choosing
this census, not claiming that the selected loop dominates. Current raw box
sites and loop input categories have not yet been observed. Implementation is
therefore **not ready** on existing evidence alone.

The shared stats owner must bind an actual current Py_STATS source/config/ELF
and loaded `_sre` origin; preserve complete warmup/low/high dumps with hashes,
actual commands, loops, PID/origin records and handler coverage metadata. Resolve
the conversion return PC to the actual `_sre_compile_impl` list-access line,
using the loaded image's offset/load bias and file identity, rather than the
symbol name alone. A built-in versus extension-module placement changes which
image supplies that PC. Retain unresolved and 64-slot overflow cases explicitly.

The census must separate this loop's nonnegative inline reads inside/outside
the -5..1024 cache from object and invalid-type reads. Existing global counts
and return PCs may identify the site but do not by themselves establish fresh
allocation counts. The qualification owner can use the narrow Py_STATS-only
input-category follow-up described in the native audit if existing data cannot
resolve that distinction; this design does not independently implement it.

Before source work, establish that the selected route executes in the registered
timed consumer and is a leading fresh-capable inline-int conversion family
there. If attribution remains unresolved, the site is cold, or requests are
mostly cached, do not claim an allocation-removal opportunity; reject or restate
the hypothesis explicitly. Counts cannot establish CPU dominance. An actual
matched source/elapsed experiment must still decide profitability, including
allocation/GC and memory effects.

If the evidence passes, propose one separate 45-minute source/review checkpoint
and bounded debug/release/ablation feedback plan. Freeze the unchanged regex
workload as primary and json/unpickle plus pure-Python controls before timing;
elapsed improvement must exceed the declared noise rule without unresolved
control or memory regressions. No sample count, performance threshold, new
build or activation is implied here. All work must receive an explicit clock
and leave the required qualification before the existing integration boundary.

## Source bindings

| Input | SHA256 |
|---|---|
| `Modules/_sre/sre.c` | `3891f3503cb0ab91baea68f1b285e1e37b958f8eed6a6969663d9e9746e22a71` |
| `Modules/_sre/clinic/sre.c.h` | `42c6c480d68d75d1290fd4c276577d4cb3a0afc939a387ee4d684599a07305a0` |
| `Objects/longobject.c` | `a15845b5c07f06428262dd91f939815a24fac0d1efc8af776e5b197d2523c7b1` |
| `Objects/abstract.c` | `d6f5b630b4cb390fbd7210cb0b69f57bb621cecece15dd0a57c8e7a71e765ca6` |
| `Include/cpython/listobject.h` | `21ed11abbd3c69e77776e0204741f8a9f326d7f14bc83f109f8bbde94a42bafc` |
| `Lib/test/test_re.py` | `f729d116ab5fffeab1f76bb7d1cadadf950e5bf834b73f6c47558ada6aa661a3` |
| Registered `regex_compile.py` | `8e20fdb86c9510a540c39d36fbb2cf82b6ea652fd179260343368e949967c5a6` |
