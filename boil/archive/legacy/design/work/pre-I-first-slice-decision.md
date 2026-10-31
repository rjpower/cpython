# Choosing the next shared interpreter slice

Author recommendation for review, updated2026-10-30. Supported GC source4da
is integrated75e8 after final12f1 and postmergef56e qualification; runtimeDAC6,
ordinary native3c6 and ASanf945 retain their actual identities. Current work
qualifies the matched cohorts: upstream eval-nogcse configure/make,
18-module correctness and both29 workload origins passed. The [post-GC elapsed
diagnostic](pre-I-post-GC-timing.md) completed at0.824299 upstream/fork;
independent review passed. No new interpreter ABI is implemented. This packet
uses the [status report](pre-I-interpreter-status-and-opportunities.md),
[receiver audit](pre-I-inline-receiver-audit.md) and
[value-ingress audit](pre-I-value-ingress-audit.md). Its recommendations remain
conditional on current shared-cost evidence and measured benefit; roughly2×
is soft guidance, not an acceptance threshold.

**First source-backed code candidate: selective death processing.** Fresh
[counter windows and a coroutine profile](pre-I-gc-policy-and-experiment-decisions.md#why-the-fork-is-slower)
show substantial GC cost outside mark/root closure, including repeated
eligibility checks and passes over unrelated dead objects. Refine actual work
eligibility and stage subsets before introducing generations or a numeric ABI.
The existing-budget sensitivity below remains a diagnostic of amortization
and memory cost, not the selected explanation or a forecasted fix.
The [architecture plan](gc-architecture-next.md) separates the active first
slice from native-only cleanup, allocator-aware reclamation and a common movable
nursery with stable native space.
Source 788695 passes independent review and its O1 debug build; an initial
13-case run passed all 12 existing controls and failed the new resurrection
oracle. Independent comparison with pinned upstream 321925 confirms that
callback-free weakrefs intentionally survive finalizer resurrection. The
test-only correction 69c34 asserts survivor identity and eventual clearing.
Reusing unchanged native788 products, focused13 passes without skips and all
four affected modules pass (599 tests,46 skips), with successful cleanup and
zero fallback. The whole-module run has one report-only unpinned record;
the original failed run is preserved. No runtime fix or speedup is established
by correcting the test. Subsequent O3 candidate10d877 measurements now show
1.309×/1.062×/1.078×/0.985× parent/candidate throughput on coroutines,
comprehensions, generators and Richards. Only coroutine sample ranges are
nonoverlapping; full qualification and landing review remain. The architecture
plan includes the independent replay and finite13 native profiles.
The [pinned upstream rationale](https://github.com/python/cpython/blob/321925c92c9e67864cafa1566840927f6f07e90a/Python/gc.c#L779-L787)
explains why callback-free references remain usable during finalization.

The finite NOARGS closure remains a candidate whose ABI cost needs a current
conversion budget. H1 is archived for the current GCC milestone;
its matched comparison becomes relevant only if Clang tail dispatch is an
active target. None is an automatic
implementation or experiment grant.

## Shared interpreter choices after the collector cost question

| Choice | Source-backed opportunity | Smallest coherent boundary | Why choose or defer |
| --- | --- | --- | --- |
| Numeric NOARGS value receiver/result | Generic descriptor calls box inline self in `method_check_args`; specialized NOARGS calls explicitly root a new object box, then canonicalize the object result. Stored bound methods have a separate object-self storage box. | One declared value-self/value-result NOARGS callback convention, all18 int/float instance entries, generic/unbound/specialized/escaped call routes, Clinic and shared implementation adapters. Getsets, classmethods, other argument conventions and bound-method storage stay separate. | Choose only if current receiver conversions materially contribute. Eighteen is the actual finite entry set, not an optimization quota; the ABI closure must earn its cost. |
| Shared frame/call/resume transitions | Generator/coroutine profiles expose evaluator and frame movement costs alongside GC. Direct Python frame entry already exists. | Select a bounded current-GCC call/frame intervention from actual profiles; H1 is a separate archived Clang-tail experiment. | Do not redesign transitions or delete required IP/exception/root stores from source-line samples. Separate frame creation, copying and resume cost before choosing the slice. |
| Generated producer/consumer facts | A shape load already copies one64-bit value. Slot kinds promise I32/F64/any-value, but following numeric guards do not consume that promise. Specialized int/float comparisons produce canonical bools; generic rich comparison need not. | First prove a one-use canonical-BOOL producer/consumer contract with unchanged two-word/IP/monitoring boundaries; no general typed IR or H2 activation. | Choose if repeated guard work is material and all entries, generic fallback, exceptions, debugger jumps and instrumentation establish the fact. Typed LOAD_ATTR alone saves no payload load or boxing. Avoid dependent unchecked consumers before that proof. |

The first column identifies mechanisms; any elapsed benefit remains a
hypothesis. ABI, canonical-value, root and entry-state contracts are mandatory
correctness requirements rather than optional performance assumptions.

## Evidence-based decision tree

| Current matched evidence | Smallest next question | Minimum attribution |
| --- | --- | --- |
| GC cost is largely outside mark/root closure | Actual cleanup eligibility and selective stage traversal, with budget sensitivity as a control | Preserve weakrefs, finalizer ordering/resurrection, buffer readiness and OOM rollback; measure reclamation work and elapsed cost. |
| Repeated tracing work or collection frequency is material | Existing allocation-budget sensitivity before new barriers | Collection/root/death work, elapsed time, RSS and pauses within a declared memory allowance. The current budget is already `max(configured_floor, live_object_bytes)`, with a32MiB default floor; larger budgets cannot remove required death processing. |
| A non-GC generator/coroutine deficit remains | A bounded current-backend call/frame investigation before redesign; H1 only for an active Clang-tail target | Separate frame creation/allocation from hot resume/dispatch. Required exception, return-slot and IP stores are not redundant merely because they appear in samples. |
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

## Existing GC budget: completed diagnostic

The finite13 experiment is complete; the architecture plan contains the results.
Keep32MiB: doubling the floor saves2–7% instructions on the slow rows for
52–84% more peak RSS. The remaining paragraphs describe the bounded design
used as input to that experiment, not pending work. Reuse the existing
[four-row phase probe](pre-I-shared-cost-attribution.md): coroutines,
comprehensions and generators, with Richards as the historical no-hot-GC
control. Use the qualified fork O3 product unchanged. One paired comparison
per row is enough for this initial sensitivity question; balance which setting
runs first, retain every result, and stop rather than extend until a win appears.

The only setting change is32MiB versus64MiB through existing
[`gc.set_threshold(threshold0)`](../../../Modules/gcmodule.c), whose first
argument here is allocated bytes. There is no budget environment variable.
Apply the setter to **both** arms at the same point before setup, retain the
other threshold tuple fields and restore the original tuple in `finally`.
Keep GC enabled, actual verifier off/extras0/stress0, and no forced collection.
The setter initially sets `alloc_limit = alloc_bytes + threshold0`; after a
collection [the collector](../../../Python/gc_live.c) uses
`max(threshold0, live_object_bytes)`. Thus a large live set can make the two
floors equivalent; a null result would not refute every GC-cost hypothesis.

Reuse the probe's counter-only setup/warm/hot boundaries and existing collector/root
snapshots, with `/usr/bin/time` peak RSS outside the child. Report elapsed,
collection count/time, candidates/collected/freed bytes, root work and pause
observations. Root time is included in GC time; a lifetime maximum pause or
RSS high-water mark is not a hot-window distribution. Preserve callback/drainer
delivery limits. Allocation pressure counts gross requested object/PyMem bytes,
including full realloc requests; frees do not subtract, while live-object bytes
exclude retained PyMem payloads and RSS. Inspect those quantities before
concluding repeated old-live scanning is the cost.

Propose eight fresh processes total, existing registered workloads and probe
hot-call budget,150s per process and10min total under normal admission/quiet
coordination. A provisional post-pair comparison allowance is baseline peak
RSS plus128MiB per row; report any excess. It cannot stop a64MiB-first arm
before baseline RSS is known. Declare any absolute process ceiling separately
before launch, using existing resource controls if needed. These are diagnostic
resource bounds for review, not a new correctness gate or default-heap change.
No perf-event campaign, collector instrumentation, new runner or build is
needed. Any helper adaptation and literal vectors still require review.

Fewer collections alone is not success: assess elapsed benefit against RSS,
pause and delayed-reclamation costs. If collection amortization is material
within the allowance, consider the simple policy before barriers. If it is
immaterial, retain the result and resolve creation/resume/dispatch or measured
receiver conversions next. No generation, NOARGS ABI or H1 implementation is
selected by the29-row aggregate alone.

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
