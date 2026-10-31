# Interpreter status and prospective improvements

Source review, updated2026-10-30 after supported GC integration **75e8a1e0c01**.
Qualified4da/runtimeDAC6/ordinary native3c6/ASan nativef945 is integrated;
final12f1ea31 and postmergef56e0b6b bind all qualification and both1,935-case
smokes. The analysis below separates source evidence from the new elapsed
diagnostic; neither is a performance implementation grant.
H3 is landed; H1 frame passing and inert H2 remain isolated.

The owned upstream eval-nogcse comparator has passed configure/make and
independent build review (23c9/c518),18-module correctness and29 workload
origins. Fork29 origins also pass. The [post-GC elapsed diagnostic](pre-I-post-GC-timing.md)
completed at0.824299 upstream/fork; independent review passed.
No numeric NOARGS ABI or
other proposed interpreter change is implemented here. Roughly2× remains soft guidance.
Supported callback/buffer lifetime is qualified for Linux/main interpreter/
ordinary threads/shared GIL; foreign/adoption, platform and precision work
remain separate. Original failed runs and measurement limits stay preserved.

The [measured GC plan](gc-architecture-next.md) now has implementation/landing
authorization and active parallel owners. Selective-death O3 coroutine throughput
improves1.309× in the isolated topic; whole29 remains unchanged. Keep32MiB after
the completed budget experiment. Native-only cleanup and direct scanning are
now composed with the warning-context repair as4d8e690. Candidate33944 adds proven
decoder/libc effect declarations after two retained linked-audit failures.
O1/O3/ASan/tail builds and all early runtime checks pass. Broad release full
passes51,255 tests; debug normal full is active. Final qualification and landing
remain before any refreshed performance claim.
The
[new independent interpreter review](reviews/interpreter-holistic-review-post-gc.md)
checks actual source and ranks five clean cuts for post-qualification attribution.
Storage/nursery interfaces follow the union's measured result. The code organization review explains
current roots and the new writable-slot/space/publication/commit boundaries.
These status updates supersede pending-budget and nonmoving-first sequencing
below; the other interpreter opportunities remain conditional on attribution.

The core architecture is coherent: compile once into register IR, carry one
canonical value representation, generate handlers from one DSL, and keep
feedback separate from immutable code. The largest design gaps are the
unfinished boundaries between those layers: native self/slot interfaces,
frame transitions and initialized root spans, and complete final-word effects.
Those deserve shared solutions before adding unrelated fast paths. The
[first-slice decision tree](pre-I-first-slice-decision.md#evidence-based-decision-tree)
prioritizes selective collector death processing from current cost evidence;
budget sensitivity remains a diagnostic, with numeric NOARGS and producer
facts conditional on their own attribution. H1 reopens only for an active
Clang-tail target. NOARGS is not automatically
first; an ABI expansion needs conversion attribution and measured benefit. Existing
typed handlers are useful implementation structure, but do not yet constitute
one orthogonal parameterized operation model.

## What is implemented

| Layer | Actual implementation | Important remaining boundary |
| --- | --- | --- |
| Compiler | `compile.c` obtains the optimized stack CFG; `regcfg.c:1404` runs swap renaming, copy propagation, store elimination, pop-top and truth folding, then physical slot compaction. `assemble.c` emits words, feedback initializers, source locations and exception tables. | No runtime lowering from ordinary bytecode. The compiler still starts with stack-shaped semantics; the final physical IR is not yet the authority for complete H2 liveness. |
| IR | `pycore_ir.h:12`: one64-bit word,16-bit handler and four12-bit fields with generated slot/immediate/feedback roles; CD can hold wider data. Code words are immutable; `ir.c:245` lazily creates mutable execution words and feedback. | Four fields are not four interchangeable operands. Folded inputs, call windows, relaxed jumps and exception successors have distinct real semantics. |
| Handlers | `bytecodes.c` declares operations, guards, macro compositions and specialization families. `ir_generator.py` generates executable cases; IR metadata describes formats, properties and instrumentation relationships. | There is one source of generated truth, but some complex effects still need explicit semantics before maps or ownership proofs can consume it. |
| Values and objects | Canonical64-bit `PyValue`: inline exact int32/non-NaN doubles, otherwise object pointers; bool, large integers, subclasses and NaN stay objects. Core frames/containers/cells/instance slots use values. Shapes describe positional attributes; materialized dictionaries enter terminal dictionary mode. | Object-pointer APIs still box; raw native fields are not automatically value storage. This is an intentional object/ABI migration, requiring rebuilt ports. |
| Calls and slots | Value vectorcall, value arguments/results for METH_O/FASTCALL/METHOD, value `tp_call` and attribute/getter results. Generic and specialized call routes coexist. | Native self remains an object; NOARGS retains object result; many numeric/sequence/mapping/wrapper slots still take objects. Bound-method object storage and VARARGS tuples are separate allocation sources. |
| Dispatch | Generated computed-goto/switch and preserve_none/musttail backends exist. Current tail handlers pass slots/thread/IP/word, plus stats sequence state, and reload frame from the thread state. | A backend's existence is not a measured win. H1's passed-frame convention remains isolated; changing diagnostic IP publication is outside that experiment. |
| Roots and memory | Full-heap nonmoving tracing, no reference counts, explicit precise value/raw spans and checked precise C frames, conservative opaque stack/PyMem-carrier roots. Frame clearing remains active. | No active H2 maps, generational barriers/minors or moving collector. Current frame pointers, initialized slots and hybrid coverage cannot be removed by implication. |

Source anchors: [register pass driver](../../../Python/regcfg.c),
[IR/feedback interface](../../../Include/internal/pycore_ir.h),
[operation DSL](../../../Python/bytecodes.c),
[value representation](../../../Include/pyvalue.h),
[native method conventions](../../../Include/methodobject.h),
[dispatch macros](../../../Python/ceval_macros.h), and
[root contract](../11-precise-roots.md). These common interpreter sources
are shared with the integrated candidate; the GC composition is
separately reviewed in [the union source review](reviews/gc-simple-union-source-review.md).

## Existing common and typed families

The source already shares meaningful machinery. BINARY_OP groups int arithmetic,
division/remainder, bitwise/shift operations, float arithmetic, Unicode addition,
subscription and generic extension. Shared guards admit inline or compact exact
integers. Int/float/string comparisons, list/tuple/dict/string subscription,
sequence unpacking and list/tuple/range/generator iteration have specializations.
Generic value APIs retain overflow, subclass and user-code fallback semantics.

LOAD/STORE_ATTR shares shape/version/transition and polymorphic plans, alongside
module, slot, property and dictionary-mode routes. Method loading can forward a
descriptor and value receiver without constructing a bound method first. CALL
has Python exact/general, bound-method, builtin and descriptor conventions.
SEND/YIELD inline generator/coroutine transitions while retaining exception,
return-slot, frame and IP state. These are common families, not an absence of
specialization or a need to add a separate arithmetic interpreter.

Several dimensions remain entangled in target names and adapters: semantic
operation, receiver admission, stored representation, result representation,
call convention, monitoring and available feedback. A shape proof that a slot
is filled does not automatically prove its value is int32. A non-NaN float
representation does not admit numeric subclasses. A predicted entry is not an
ordinary counted dispatch entry. A clean parameterized design must preserve
those distinctions rather than assign one ambiguous "type" parameter.

## Landed, isolated and proposed work

**H3 domain/storage/statistics is landed** through qualified5a45→d6a23 and is
present in3c6. Its302-entry domain is capacity, not an optimization threshold.
Generated dispatch/pristine/adaptive/specialized/instrumented/NOFB properties
control eligibility. Monitoring has reviewed full-domain storage interfaces.
Py_STATS counts actual target entries/pairs before predicted labels and tracks
activation-local sequence state even when aggregation is disabled. Readers
validate metadata and distinguish entries from semantic/site counts. Later
transport/renumbering and typed operations remain. See
[H3 source/results](H3-1-stage2b3-implementation.md).

**H1 passed-frame is isolated**, native62456/tool-test94ab. Source/audits,
runtime checks and strict stale controls exist; old combined harness receipts
have an informational platform-header limitation. Static code changes are
mixed, and no elapsed benefit is measured. Its matched Clang experiment is
selected for archival for the current GCC milestone. See [H1 record](H1-1.md) and
[passed-frame readiness](H1-passed-frame-readiness.md).

**H2 is isolated and inert**, selected for archival rather than a merge without
a consumer; native5df336 with test-only parser bridgeb3e6847.
Its generated scalar-temporary decoder supports27 pristine handlers and
explicitly rejects126 unsupported ones. Corrected release/debug-kill affected
validation exists; original parser failure remains preserved. It changes no
runtime root reader, clearing policy, code/frame layout or register reuse.
Complete effects/CFG/fixedpoint, final-word verification, independent compiler
projection and frame-reader/initialized-span closure remain before activation.
See [H2 source and evidence review](reviews/H2-root-effects-source-review.md).

Generations and alias-safe linear string growth are designs, not implemented
speedups. JIT and movement remain deferred. No isolated branch is silently
included in this candidate's implementation or performance claims.

## Ranked opportunities for the next review

Three source examples make the distinction between cleaner operation families
and faster execution concrete:

- **`i += 1`.** Existing register passes fold eligible local loads and the
  final store. Warm `BINARY_OP_ADD_INT` checks both compact integer operands,
  decodes them and checks overflow. An ordinary int32 result allocates nothing;
  overflow takes the boxing path. Constant `1` still uses the general operand
  guard because its kind is not propagated into this consumer. Generated C
  temporaries alone do not establish machine-level moves or their cost.
- **`o.x + o.y`.** The existing attribute-sum test specifies four words: two
  loads, add and return. A shape load guards owner/version and copies one
  `PyValue`; it does not box the result. Shape slot kinds already promise I32,
  F64 or any value, and generalization invalidates the version; these are
  value promises rather than different layouts. There is no string slot kind.
  A typed LOAD_ATTR name alone saves no payload work. A consumer would need
  to use the promised result kind, with a valid path when the producer falls
  back to generic behavior.
- **`if i < n`.** The compiler already folds away the separate TO_BOOL for
  comparisons used as conditions. An integer comparison still writes a
  True/False singleton value and dispatches to a BOOL branch that checks and
  consumes it. This creates no boolean allocation. Shared result contracts
  may expose redundant work, but removing a guard requires proof for every
  entry, including instrumentation and debugger control transfers.

See [existing IR expectations](../../../Lib/test/test_ir.py),
[shape kind promises](../../../Include/internal/pycore_shape.h),
[specialization feedback](../../../Python/specialize.c),
[truth folding](../../../Python/flowgraph.c) and the operation DSL above.
An initial design experiment can ask whether existing producer facts can be
reused by consumers without adding another type system or changing instruction
boundaries. This is a feasibility question; matched attribution must establish
whether the saved work is worth an implementation.

This ranking weighs reuse, existing evidence and proof readiness. It is not a
speedup forecast or approval to begin seven parallel implementations. With
supported GC qualified, choose a small coherent source slice and measure it
on matched nonstats release products.

| Area | Current symptom | Clean shared direction | Required proof or falsifying evidence |
| --- | --- | --- | --- |
| 1. Close value/native boundaries | `binaryfunc` remains object/object→object; native self boxes in generic and specialized numeric methods/getsets. Other slot wrappers, Clinic residue and native fields add distinct conversions. | Extend declared callback/slot conventions in place, with matching wrappers, Clinic generation and specialized dispatch. Preserve values until an actual object consumer requires a box. A numeric NOARGS receiver slice is finite; bound-method storage/getsets remain explicit separate closures. | All generic/specialized/escaped routes must agree on ABI, wrong-receiver/error order and roots. Preserve raw box roots when the callback actually needs the box. Recount actual box sites/allocations, then elapsed work; no bit_length-only intrinsic. [Receiver audit](pre-I-inline-receiver-audit.md), [value ingress](pre-I-value-ingress-audit.md), [note12](../12-object-slots-on-values.md). |
| 2. Simplify frame/call/resume transitions | Historical generator samples concentrate in SEND/YIELD/dispatch; coroutine work includes frame traversal and return-slot movement. Python calls, native calls, throw/close and suspended frames have related but distinct handoffs. | Treat create/enter/resume/return/unwind as one explicit initialized-state protocol, then remove redundant adaptation or allocation only where that protocol proves it unnecessary. Keep specialized direct frame entry and generic fallback coherent. | Current frame/exception/IP publication, escaped frame introspection, PEP523, monitoring, recursion and error spans must remain correct. Measure frame creation versus hot resume separately. Do not optimize a required state store away because a source sample landed there. [Handler contract](../14-handler-contract.md), [attribution](pre-I-evaluator-source-attribution.md). |
| 3. Make operation families orthogonal | Existing macros reuse guards/bodies, but semantics, receiver/storage/result facts and feedback mode are not one explicit family contract. Adding more one-off targets risks repeating dispatch and root logic. | Define operation plus independent admission/representation/call-mode dimensions in the existing DSL and derive concrete targets/metadata from that one declaration. Retain the existing word format unless actual constraints justify changing it. LOAD_ATTR int32/float/string/object modes must state whether they mean stored representation, checked result or observation. | Each admitted combination needs exact guard/fallback/error/monitoring/root behavior; unsupported combinations are rejected. Compare generated coverage and native code size. Parameterization should reduce duplicated semantics, not generate every Cartesian combination or a second opcode/effect inventory. H3 properties are the starting point, not a numeric handler quota. |
| 4. Complete physical liveness before retiring clearing | Scalar effects are partial; frame clearing/root scans still cover live state. Removing stores based on virtual registers or ordinary CFG alone would miss physical rewrites and transition windows. | For a scheduled usable slice, reuse archived H2 work to finish generated final-word effects/CFG/liveness, validate against compiler projection, then migrate readers/publication one contract at a time. | Exception edges, aliased physical slots, relaxed SEND/FOR_ITER, replacement handlers, created/suspended/escaped frames and raw call windows must all be covered. No all-slots "map" success, unsupported-opcode acceptance or activation of incomplete maps. Only then compare clearing/scanning cost and RSS retention. [H2 review](reviews/H2-root-effects-source-review.md), [note14](../14-handler-contract.md). |
| 5. Test dispatch and feedback costs coherently | Profiles include evaluator work; Richards includes feedback addressing/shape matches. Current tail handlers reload frame. | Inspect current-backend feedback/addressing and call boundaries where measured. Reopen same-Clang H1 comparisons only for an active tail-backend decision. Prefer one convention over permanent dual machinery. | Match compiler/flags/backend/root contracts, retain diagnostic IP publication, inspect spills/code size and elapsed distributions. GCC non-tail versus Clang tail confounds compiler and topology. A sample on a landing pad is not its exact instruction cost. No local fusion solely to improve one row. [H1](H1-1.md), [offline join](pre-I-evaluator-source-attribution.md). |
| 6. Make proven private string appends linear | Flat immutable strings plus no refcounts leave local repeated append without a refcount-based uniqueness test. | Use a certified nonescaping local append epoch and geometric capacity while preserving ordinary flat string representation. Publication/aliasing seals the epoch. Reuse the proposed code-owned proof/activation owner, not ropes or speculative universal uniqueness. | Complete escape/join/exception/overwrite/final-word admission and owner-generation proof; preserve lazy exec and all storage observers. Initial shared prefixes need a fresh private result; in-place continuation needs the same valid transaction, not any old pointer. Scaling and alias negatives decide value. [Design](pre-I-linear-string-concat.md), [proof/owner prerequisite](pre-I-linear-string-proof-owner.md). |
| 7. Reduce shared allocation/GC work | Historical coroutine/comprehension/generator windows spend substantial time collecting and processing dead objects, while Richards has no hot collections. | First test the existing allocation-budget control on unchanged qualified source with a declared memory allowance; also scrutinize shared frame/allocation churn. Consider nonmoving generations only after real owner/store/barrier coverage. Reuse existing queue/root contracts rather than add another resource scheduler. | Old→young barriers, partial constructors, opaque roots, promotion/reentry and an independent full-young oracle must close. Record candidates, survival, death work, RSS and pauses. Generations do not remove inevitable dead-object bookkeeping or C ABI boxes. [Generations design](7-nonmoving-generations-design.md) and its store ledgers remain prerequisites, not implementation. |

## A cheaper GC question before generational barriers

The current collector already scales its post-collection allocation budget:
`max(configured_floor, live_object_bytes)`, with a default floor of32MiB.
Automatic heap-triggered collections do not routinely run a second pass;
the bounded extra pass concerns explicit collection/finalization progress.
A fixed tiny threshold or automatic double collection is not established as
our problem. See [trigger policy](../../../Python/gc_live.c) and
[default budget](../../../Include/internal/pycore_interp_structs.h).

There is a measurement question about accounting. Allocation pressure counts
gross requested object and PyMem bytes before allocation; realloc charges the
full new size, including requests that shrink in place or fail. Frees do not
subtract from that counter. The live-byte budget counts object blocks rather
than all retained PyMem payloads or RSS. These quantities serve different
purposes; their difference is not itself a correctness bug or a measured
performance problem. See [allocation accounting](../../../Objects/obmalloc.c)
and [live object accounting](../../../Python/gc_trace.c).

On the qualified source, test the existing budget control before committing
to barriers. Declare the sensitivity
comparison and memory allowance in advance, and measure elapsed time,
collection/root/death work, RSS and pauses. A policy that amortizes root and
live-heap work may be a simpler improvement if that experiment supports it;
larger budgets cannot remove inevitable per-object death work. Root-aware
heap expansion has a well-known precedent in the
[BDWGC allocation policy](https://hboehm.info/gc/gcdescr.html). This is a
proposed controlled question, not a threshold recommendation or speedup claim.

## What the measurements actually establish

Fresh [cost attribution](pre-I-gc-policy-and-experiment-decisions.md) records
timed GC shares47.1%/54.1%/29.7%/0% for coroutines/comprehensions/generators/
Richards. Mark/root closure is8.0%/3.7%/10.8%/0% of wall time and already
included in GC. A whole-process coroutine profile implicates heap
classification, sweeping and repeated lifetime eligibility checks; the first
code candidate is selective death processing, with all existing semantics
preserved. No source optimization or budget change has run.

The new matched eval-nogcse [29-row diagnostic](pre-I-post-GC-timing.md) reports
**0.8242992363 upstream/fork**,21 slower rows and eight faster. Coroutines0.4489,
comprehensions0.5097 and generators0.5737 remain large deficits; float2.0394,
Richards1.8655 and nbody1.6681 favor fork. All290 values, actual source/native
identities and terminal cleanup are retained; independent review passed.
Five serial calls in one fresh process per runtime/row are correlated samples,
not independent trials or formal confidence intervals. Fork spreads are often
10–40% using `100 × (max/min − 1)`, including84.26% for logging_silent and51.14%
for float. Near1 rows and float's proximity to2 do not establish stable ranks
or a threshold win. The broad deficits are a planning issue; the elapsed score
alone does not identify the mechanism. Current RSS/pauses were not measured.

The historical owned-upstream-versus-ce2ac diagnostic reported a29-kernel
geomean of0.828243 for upstream/fork elapsed time:22 slower rows and seven
faster. Coroutines0.365, comprehensions0.528 and generators0.592 contrasted
with float1.923, Richards1.897 and nbody1.603. Those five-sample initial
estimates used an unlanded failing-stress fork and disclosed fork-only
`-fno-gcse`, fork-driver origin and SDK limits. They are **not a3c6 score**,
correctness qualification or a present parity claim. Every row remains in the
[timing record](pre-I-first-quiet-timing-plan.md).

The same historical ce release's four-row profiling measured hot GC fractions
54.0%/47.0%/30.8%/0% for coroutines/comprehensions/generators/Richards. Root
time is part of GC; callback-pause accounting can undercount delivery. The
sampled evaluator shares were27.3%/21.1%/51.1%/79.5%, with no lost samples
but unresolved libc/ancestry. Death/candidate totals establish per-object
bookkeeping, not an allocation-age distribution or a generation speedup.
The offline source join identifies common frame/feedback/dispatch sites, but
skid, inlining and compiler line attribution prevent exact instruction-cost
claims. See [shared costs](pre-I-shared-cost-attribution.md) and
[source join](pre-I-evaluator-source-attribution.md).

Do not attribute the aggregate difference from historical ce2ac to GC: both
fork source and the upstream ceval-flag cohort changed, and the old fork was
unqualified. The [next bounded proposal](pre-I-first-slice-decision.md#bounded-next-question-existing-gc-budget)
compares existing32/64MiB allocation budgets on unchanged qualified products,
with collection/death/root work, RSS and pauses. It remains a useful diagnostic;
the fresh profile makes selective death processing the first source-backed
code candidate. No generation or numeric ABI is selected. Roughly2× remains a soft
guide. Keep a proposed change only when
its shared design is simpler or its measured benefit pays for its complexity;
preserve a clean negative result instead of adding exceptions until it wins.
