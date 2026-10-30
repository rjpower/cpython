# Choosing the next shared interpreter slice

Author recommendation for review, updated2026-10-30. Supported GC source4da
is integrated75e8 after final12f1 and postmergef56e qualification; runtimeDAC6,
ordinary native3c6 and ASanf945 retain their actual identities. Current work
qualifies the matched cohorts: upstream eval-nogcse configure/make,
18-module correctness and29 workload origins passed; fork29 origins also pass.
There is no refreshed score or interpreter ABI implementation. This packet
uses the [status report](pre-I-interpreter-status-and-opportunities.md),
[receiver audit](pre-I-inline-receiver-audit.md) and
[value-ingress audit](pre-I-value-ingress-audit.md). Its recommendations remain
conditional on current shared-cost evidence and measured benefit; roughly2×
is soft guidance, not an acceptance threshold.

**No implementation is automatically first.** The finite numeric NOARGS
closure removes a concrete representation round trip, but its ABI cost needs
a current conversion budget. Existing GC-budget sensitivity or the prepared
matched H1 comparison can precede it. First [refresh the matched interpreter
baseline](pre-I-qualified-timing-refresh.md), then resolve only the attribution
needed to choose one shared intervention. Origin checks establish which code
ran; they do not rank performance. No current measurement establishes which
choice will improve the suite most.

## Three choices

| Choice | Source-backed opportunity | Smallest coherent boundary | Why choose or defer |
| --- | --- | --- | --- |
| Numeric NOARGS value receiver/result | Generic descriptor calls box inline self in `method_check_args`; specialized NOARGS calls explicitly root a new object box, then canonicalize the object result. Stored bound methods have a separate object-self storage box. | One declared value-self/value-result NOARGS callback convention, all18 int/float instance entries, generic/unbound/specialized/escaped call routes, Clinic and shared implementation adapters. Getsets, classmethods, other argument conventions and bound-method storage stay separate. | Choose only if current receiver conversions materially contribute. Eighteen is the actual finite entry set, not an optimization quota; the ABI closure must earn its cost. |
| Shared frame/call/resume transitions | Historical generator/coroutine samples expose dispatch, return-slot movement, traversal and frame transitions. Direct Python frame entry already exists. | The already-isolated H1 passed-frame experiment is finite; broader create/enter/resume/return/unwind consolidation is not yet one bounded patch. | Use H1's matched result first. Do not redesign transitions or delete required IP/exception/root stores from source-line samples. Larger frame/allocation work needs a current shared-cost attribution before choosing its slice. |
| Generated producer/consumer facts | A shape load already copies one64-bit value. Slot kinds promise I32/F64/any-value, but following numeric guards do not consume that promise. Specialized int/float comparisons produce canonical bools; generic rich comparison need not. | First prove a one-use canonical-BOOL producer/consumer contract with unchanged two-word/IP/monitoring boundaries; no general typed IR or H2 activation. | Choose if repeated guard work is material and all entries, generic fallback, exceptions, debugger jumps and instrumentation establish the fact. Typed LOAD_ATTR alone saves no payload load or boxing. Avoid dependent unchecked consumers before that proof. |

The first column identifies mechanisms; any elapsed benefit remains a
hypothesis. ABI, canonical-value, root and entry-state contracts are mandatory
correctness requirements rather than optional performance assumptions.

## Evidence-based decision tree

| Current matched evidence | Smallest next question | Minimum attribution |
| --- | --- | --- |
| Collection remains a large shared cost | Existing allocation-budget sensitivity on unchanged qualified source, before new barriers or ABI work | Collection/root/death work, elapsed time, RSS and pauses within a declared memory allowance. The current budget is already `max(configured_floor, live_object_bytes)`, with a32MiB default floor; larger budgets cannot remove required death processing. |
| A non-GC generator/coroutine deficit remains | Existing matched dispatch/H1 comparison before broader frame/resume redesign | Separate frame creation/allocation from hot resume/dispatch. Required exception, return-slot and IP stores are not redundant merely because they appear in samples. |
| Numeric receiver conversions materially contribute across workloads | Closed18-method NOARGS convention | Current receiver-box/site counts separated from legitimate results and escaped bound-method storage, followed by matched release benefit and code-cost review. |
| Repeated guard work remains material | One canonical-BOOL producer/consumer proof | Eligible producer/consumer pairs and emitted guard instructions, preserving alternate entries and monitoring. |

The inspected [generator](../../bench/internal/kernels/generators.py) and
[coroutine](../../bench/internal/kernels/coroutines.py) kernels do not directly
call the18 numeric methods. Known chaos `bit_length()` traffic is transitive
through `random`; its historical count does not explain the resume-heavy
regressions. Use the already-identified discriminating rows rather than another
broad attribution campaign.

In [the current handlers](../../../Python/bytecodes.c), warmed `i += 1` with
compact operands uses allocation-free int32 addition with checked overflow. Proving constant RHS1
can remove its admission check, but does not prove the loop-carried LHS remains
compact after overflow. Specialized int/float comparisons return canonical
bools; generic rich comparison can return another object. Existing conditional
branches already combine truth conversion and branching, so a valid BOOL fact
removes a guard, not automatically a dispatch.

## Numeric NOARGS candidate and exact caller closure

The proposed logical callback is `PyValue callback(PyValue self)` for a
method with no explicit arguments. It does **not exist yet**. Encode its
calling convention explicitly in the existing method declaration/generation
protocol; do not infer it from receiver type or equal word width. Each
converted method has one implementation and one declared convention, not a
try-value-then-box fallback. Existing object-self conventions remain for
methods that actually declare those conventions. Choose the concrete metadata
encoding and exported ABI/suffix policy in source review before code changes.

The closed method set is10 int entries (conjugate, bit_length, bit_count,
as_integer_ratio, __trunc__, __floor__, __ceil__, __getnewargs__, __sizeof__,
is_integer) and8 float entries (conjugate, __trunc__, __floor__, __ceil__,
as_integer_ratio, hex, is_integer, __getnewargs__). Exact current names and
manual/Clinic definitions are inventoried in the receiver audit.

Required common sites, not optional later fixes:

- [Native method declarations](../../../Include/methodobject.h), MethodDescr
  construction and `method_check_args`/`method_vectorcall_NOARGS` in
  [descrobject.c](../../../Objects/descrobject.c): preserve receiver/subtype,
  keyword and arity validation while retaining self as a value.
- `PyCMethod_New` and `cfunction_vectorcall_NOARGS` in
  [methodobject.c](../../../Objects/methodobject.c): escaped bound methods
  must dispatch through the declared callback ABI too. Keep object `m_self`
  storage and its repr/hash/reduce/traversal contracts unchanged in this slice;
  one storage box may remain when a method escapes.
- `specialize_method_descriptor` in [specialize.c](../../../Python/specialize.c)
  and the existing NOARGS guard/body in [bytecodes.c](../../../Python/bytecodes.c):
  generic and specialized entry must select the same convention. Regenerate
  cases; do not add a bit_length-specific opcode or parallel effect table.
- [Clinic parse/converters](../../../Tools/clinic/libclinic/parse_args.py),
  its self/result/root emission and generated long/float headers, plus the
 18 actual method definitions. Value-self/value-result must be supported
  deliberately; changing a typedef and casting existing callbacks is invalid.
- Shared bodies need explicit adapters: `long_long_meth` calls `long_long`,
  also used by object-self getters; `float___trunc___impl` also supplies
  `nb_int`. Preserve those still-object interfaces through one shared value
  implementation, rather than changing their function-pointer ABI by accident
  or duplicating numeric semantics. Include applicable C/wasm trampolines and
  audit every flag-mask/constructor consumer of the chosen convention.

The argument window, descriptor/callable and actual heap receiver must stay
live across collection, error construction, monitoring and recursive-call
checks. Inline self needs no object box; large ints, subclasses and boxed NaN
still need real roots. Keep canonical semantic values distinct from raw roots
for callbacks that genuinely receive an object pointer. Unknown callbacks
remain collecting. NOARGS is not a purity claim, and this slice changes no
clearing, root map, diagnostic IP or C-frame precision policy.

## User-visible acceptance and critical internal checks

Validate behavior through normal calls first: direct syntax, explicit unbound
descriptor calls, repeated specialized calls and stored bound methods. Check
data and result types for every converted method, including large ints,
INT_MIN, bool/numeric subclasses, signed zero, infinities and NaN. Preserve
tuple/string contents, logical __sizeof__ results and inherited-method
semantics; returning the receiver unchanged is not a general replacement for
`long_long` or float conversion. Wrong receivers, missing/extra arguments and
keywords must report the intended errors in generic and specialized routes.

Use existing profiling/monitoring callbacks that collect, existing call
lifetime tests and allocation-heavy real result methods to exercise liveness
and exception cleanup. Failure injection applies to allocations that remain;
removing a receiver box intentionally removes its former MemoryError point.
Do not preserve an obsolete allocation ordinal by inserting a replacement box.

Internal checks are reserved for the decisive invariants: generated callback
signatures/flag combinations, canonical results, actual receiver-conversion
counts and strict root validation. Reject inconsistent declarations before a
callback is invoked; never execute a deliberately mismatched C function pointer
as a negative control. Existing `test_long`, `test_float`, `test_descr`,
`test_call`, `test_clinic`, `test_monitoring` and call/GC lifetime coverage form
the affected boundary; the concrete native campaign needs its own ownership
and review. An SDK/port smoke must exercise the settled declaration convention;
old wheels or unconverted Cython/cffi emitters are not compatible evidence.

## Decision that can reject the recommendation

Before a large port or ABI rollout, obtain current source-bound conversion
counts and compare matched nonstats release elapsed work on the settled GC
source. Separate receiver boxes from legitimate tuple/string/big-int results
and from escaped bound-method storage. Historical chaos calls/boxes motivate
the question but do not establish today's budget or expected speedup.

Reject or defer the slice if callers cannot close without per-method adapter
exceptions, duplicated numeric bodies or broad unrelated rewrites; if the
conversion budget is immaterial to current workloads; if native code growth
or regressions outweigh its benefit; or if matched measurements do not justify
the ABI/maintenance cost. A successful no-box unit alone is insufficient.
Keep the existing interfaces in that case and use the H1 result or the bounded
BOOL-fact proof to select the next shared question. No promise of2×, fixed
percentage gate or automatic seven-lane optimization campaign follows.
