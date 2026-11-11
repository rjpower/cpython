# Round 51: five interpreter experiments

Review complete, 2026-11-11. This is the one broad architecture review for
this round; Sol owns bounded proposals, implementation and independent code
review. No runtime changes or new measurements were made for this review.

The source inspected is `bf810ec1a33` in
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108`.
Paths and line numbers below refer to that runtime, not this documentation
checkout. Native evidence is the matching `execution-boundaries-release/python`,
including retained `native50final` disassembly and fresh read-only `objdump`
inspection of ADD_INT, FOR_ITER_LIST and frame clearing. The
[round-50 report](../deltablue-call-model-20261110/report50.md) and
[tables](../deltablue-call-model-20261110/benchmark-tables50.md) own measurements.
The CPython AI policy and canonical IR, interpreter, frame and value contracts
were consulted. Historical value-API inventories are not current source evidence.

The architectural opportunity is to preserve existing facts at their consumers:
layout at attribute guards, canonical values at publication, reserved storage
at activation, and empty scratch at completion. None requires a second evaluator,
owner ledger, alternate compiler or benchmark-specific behavior.

| ID | Bounded enhancement | Expected reach and benefit | Confidence / risk | Suggested order |
| --- | --- | --- | --- | --- |
| 1 | Cache the guarded inline-values displacement | Small per attribute hit; DeltaBlue, richards, comprehensions, generator tree traversal | High mechanism confidence; low–medium invalidation risk | 3 |
| 2 | Publish specialized sequence iteration directly from canonical homes | Moderate potential in list/tuple loops; comprehensions, DeltaBlue and other container workloads | High native evidence; medium ownership/observer risk | 1 |
| 3 | Give exact activation one reservation phase and one header initialization | Potential across Python-call workloads, including recursive coroutines; size of gain uncertain | Medium mechanism confidence; medium storage risk | 5 |
| 4 | Preserve canonical integer arithmetic results through overflow joins | Small cost on very frequent arithmetic; spectral_norm, coroutines, richards, DeltaBlue | High native evidence; medium proof risk | 2 |
| 5 | Carry normal-return empty-scratch proof into generator cleanup | Modest potential per completed generator/coroutine; generators, coroutines, comprehensions | High source evidence; medium lifetime risk | 4 |

These are potential improvements, not promised speedups. IDs are stable subtree
identities; suggested order ranks evidence and breadth against implementation
risk. Samples at exact CALL 13.39%, shape-local 9.28%, frame clear 5.68% and
RETURN 4.40% locate work; they do not establish removable fractions. DeltaBlue's
35.3% excess instructions make structural instruction removal useful, but do not
explain logging's measured equal-instruction cycle regression.

## 1. Cache the inline-values displacement under the existing type guard

**Evidence.** `bytecodes.c:2688` computes `_PyObject_Shape(owner)` after the
type-version guard. `pycore_object.h:978` computes inline values as
`obj + Py_TYPE(obj)->tp_basicsize`. Selected native
`_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` loads `tp_basicsize` at `0x3670f0`,
then reads the shape through that dependent address at `0x3670f4`. Its value
publication is already canonical and its local receiver needs no close.

**Experiment.** Implement the unimplemented displacement proposal B from the
previous round: record the inline-values displacement when specialization
establishes the type/layout fact. Use the existing `_PyFbAttr._pad` storage
where its full producer/copy/reset contract permits it. The shape/validity
reader uses the cached displacement only after the existing type-version
guard. Keep shape, descriptor, dictionary and mutation guards. Begin with the
shape-load family, including its existing local/temp variant; update every
payload copier required for that family. Do not invent a new global shape ID
or remove the type guard: shape indices are local to a type.

**Boundary.** `Include/internal/pycore_feedback.h`, relevant specialization and
polymorphic snapshot code in `Python/specialize.c`, shape uops in
`Python/bytecodes.c`, and their generated products. Audit overflow when converting
`tp_basicsize` to cache width. Decline specialization when it cannot fit. Preserve
48/120-byte feedback geometry; check whether a semantic serialization change
requires a magic update even if geometry is unchanged.

**Proof and admission.** Test descriptor insertion, base/MRO or class mutation,
`__dict__` materialization/replacement, add/delete transitions, polymorphic sites,
and cache exhaustion. Native success must replace the type-dependent basicsize
load with the cached offset without extra admission work. Confirm DeltaBlue and
one other attribute workload. Reject if the offset load merely substitutes
equally expensive work with no repeatable application benefit. This is a small
layout fact improvement; it is not an explanation of the full attribute sample.

## 2. Make the specialized iteration body a canonical output operation

**Evidence.** `bytecodes.c:3849` reads a canonical list home and increments a
tagged cursor. Generated `FOR_ITER_LIST` at `generated_ir_cases.c.h:9688`
nevertheless emits `IR_PUBLISH_TEMP(window[1], cursor)`,
`IR_PUBLISH_TEMP(window[2], next)` and a subsequent move to A. Selected native
code contains publication/type tests and `_PyValue_Canonicalize` arms for
both cursor and item. It also calls the out-of-line `PyList_GET_VALUE`, whose
body (`Objects/listobject.c:254`) only borrows the already-guarded array cell.
The tuple variant has the same producer structure.

**Experiment.** Give exactly the list/tuple specialized body edge a checked
canonical-window output contract. Store the internal cursor unchanged, acquire
the item owner once, and publish the item to its actual recipient. For a local
or discarded A, eliminate the intermediate B+2 store/clear when the existing
edge contract proves no observer can see it. For a temporary A, preserve the
required B+2 identity. Use a small core inline typed home accessor if needed to
remove the demonstrated getter call; do not weaken public getter semantics.
Keep generic iteration, range overflow and exhaustion completion outside this
first proof. Cursor index is an internal permitted word, not a Python value.

**Boundary.** Sequence iteration uops, `Tools/cases_generator/ir_generator.py`,
the shared publication macros only if necessary, and `pycore_list.h` for a
bounded internal accessor. Regenerate output/observer metadata from producers.
Extend the positive proof to this window shape; do not relax the existing
scalar canonical-output checker globally or certify by handler name alone.

**Proof and admission.** Commit cursor and item before any displaced-local or
discard finalizer runs. Preserve iterator ownership, last-loop-local value on
exhaustion, branch/LINE events, exception attribution and callback redirects.
Test list mutation during a loop, tuple/list subclass fallback, aliased values,
local/temp/discard destinations, finalizers enabling observation, empty loops,
break and exceptions. Add negative generator cases for a changed producer,
callback between borrow and acquisition, and changed output geometry. Native
body hits must lose normalization and the redundant item round trip; remaining
owner acquisition and displaced-owner work are required. Confirm comprehensions
plus a second loop-heavy kernel. Do not infer a spectral_norm gain merely from
its use of `enumerate`, which need not execute FOR_ITER_LIST.

## 3. Separate exact-call reservation from one frame initialization

**Evidence.** `_INIT_CALL_PY_EXACT_ARGS` (`bytecodes.c:4331`) selects overlapping
or separate storage, with both branches constructing a header. The selected
native handler is 1,410 bytes and has a `0x50` C stack reservation. It contains
two header-initialization/constant-fill regions and an initialized-frontier
`memset` branch carrying numerous saved live values. Repeated source-level
function/code conversions mostly collapse in native code; passing typed pointers
alone is not the experiment. Ordinary argument overlap and initialized storage
already work and must remain.

**Experiment.** Refactor the existing exact path into reservation, one header
initialization, and owner commit. Compute the final base/saved-top from caller
ownership, reserve through the same frontier rule, then initialize once with
the selected function/code. Keep the ordinary initialized overlap case compact;
put actual frontier growth and the non-overlap argument move behind narrow
helpers where this reduces live state. The helper may not reselect mutable
function code or consume owners on a precommit miss. This is one implementation
of the existing ABI, not a new CALL flavor or method-to-CALL refinement.

**Boundary.** `pycore_interpframe.h` reservation/initialization helpers and the
exact initialization uop. Update affected callers of any changed helper interface.
No operand format, owner ledger, source-map path or leaf certificate changes.
Keep the known-Python binder's always-inline policy unless its own native
evidence justifies changing it; a blanket outlining change is out of scope.

**Proof and admission.** First inspect release output: require fewer executed
reservation/setup instructions or spills on the ordinary path, not just a smaller
symbol. Existing native CSE is explicit counterevidence to a pointer-threading
proposal. Exercise self/no-self calls, generator-owned callers, deep recursion,
chunk growth, frontier truncation/reuse, constant homes including mortal replaced
constants, frame escape, exceptions and monitoring. Both capacity and initialized
storage must be established before header writes. Benchmark DeltaBlue, richards
and coroutines; reject if a helper call taxes the normal path or reproduces the
rejected adjacent-call experiment's application losses. The main uncertainty is
whether GCC already makes the practical hot path as good as this factoring can.

## 4. Give integer arithmetic a proved canonical result boundary

**Evidence.** Guarded ADD_INT/SUBTRACT_INT (`bytecodes.c:652–687`) join scaled
inline construction and an out-of-inline-range `_PyValue_BoxInt64` result.
`pyvalue.c:155` asserts that box helper's range precondition. The current
canonical-result recognizer excludes these conditional producers, so generated
ADD_INT ends in `IR_STORE_DST`. Selected native ADD_INT executes pointer-space
publication checks even on the inline result and includes integer/float type
checks and `_PyValue_Canonicalize` arms after the join. Its guarded input closes
already disappear in native code; do not claim those as additional savings.

**Experiment.** Establish a small canonical owned-result API for guarded inline
integer addition/subtraction, or an equally bounded whole-producer proof. Both
success arms must produce canonical owners; NULL remains the allocation-error
outcome. Preserve scaled arithmetic and its overflow test. The generator carries
that fact through unchanged input consumption to the existing canonical final
store. Start with ADD and SUB only. Multiplication, division and other constructors
require their own proof and are not needed to answer this experiment.

**Boundary.** `pycore_value.h` if an arithmetic helper makes the proof clearer,
the two arithmetic uops and `ir_generator.py`'s producer validation. Keep encoding
inside the value boundary. Do not whitelist `_PyValue_BoxInt64` as unconditionally
canonical at arbitrary call sites: its precondition is essential.

**Proof and admission.** Test both inline endpoints, both overflow directions,
negative results, local/temp/discard destinations, mixed/subclass fallback,
allocation failure where supported, and reentry when replacing a mortal local.
Negative generator tests must reject altered overflow predicates, changed helper
arguments, callbacks, output reassignment and missing error propagation. Require
normalization removal in native local and temporary result paths. Confirm
spectral_norm and coroutines, with richards/DeltaBlue as independent screens.
Keep remaining input-slot classification and displaced-local handling visible;
this proposal does not remove them or authorize a general SSA/type system.

## 5. Clear only locals on proved normal generator completion

**Evidence.** `_RETURN_VALUE` (`bytecodes.c:1479`) takes the canonical result,
asserts temporaries clear, unlinks the dying frame and invokes
`_PyEval_FrameClearAndPop`. Thread cleanup already uses only `co_nlocalsplus`.
But `clear_gen_frame` (`ceval.c:2954`) calls `_PyFrame_ClearExceptCode`, whose
non-thread branch (`frame.c:229`) invokes `_PyFrame_ClearLocals` over *all*
slots. Thus normal generator/coroutine completion scans and rewrites scratch
already proved NULL. This is distinct from the absent full-temp scan on ordinary
thread return and from generator suspension, which may own scratch values.

**Experiment.** Carry an explicit normal-completion capability from RETURN_VALUE
into shared frame cleanup. The common implementation keeps frame-object takeover,
locals/function ownership and code lifetime handling in one place, but uses the
locals limit when the caller proves scratch empty. A narrow named completed-frame
entry point can supply this fact; do not infer it merely from generator ownership
or FRAME_CLEARED, and do not store another per-frame ownership flag. Preserve the
canonical return owner in C throughout retirement and existing caller publication.

**Boundary.** Normal return uop, `pycore_ceval.h`/`pycore_interpframe.h` declarations,
`ceval.c` frame-clear plumbing and `frame.c` shared clear body. The generator's
arbitrary clear/deallocation path in `Objects/genobject.c:172` retains full cleanup.
Escaped frame ownership transfer remains complete. First scope is normal return;
exception unwind, early entry failure, throw and close gain no permission by
analogy. Update the generator's exact return-body proof when changing its call.

**Proof and admission.** Add a debug assertion at the explicit capability boundary.
Test normal generator/coroutine return, returned values aliasing locals, escaped
frames, retained locals, closure cells, finalizers/GC reentry, pending redirects,
and observation enabled during retirement. Separately verify suspended close,
throw, cancellation and exception failures still release every scratch owner.
Native normal completion must clear `co_nlocalsplus`, not total slot count, while
generic clear retains its full bound. Confirm generators and coroutines; use
comprehensions as a third application. Reject if saved slots are too few to
produce a repeatable gain or extra routing costs erase it.

Do not expand this into direct generator construction this round. The existing
RETURN_GENERATOR copy has a different proof: generator-expression GET_ITER can
run before it and retain scratch, so a blanket locals-only entry copy is unsound.

## Shared experiment and integration gates

Use six unchanged hot kernels: **deltablue, richards, comprehensions, generators,
coroutines, spectral_norm**. They cover object/call traffic, control/arithmetic,
container iteration, suspension/creation/completion, and numerical calls. Run
**logging_silent** as a separate mandatory target guardrail. This is six hot
kernels plus one guardrail, not a new composite benchmark. The all29 screen
retains output logging, XML processing, native boundaries and other workloads.

Each subtree starts from selected50. First inspect its native mechanism and run
focused release/debug semantic tests and relevant generator negatives. Measure
two balanced pairs on the hot set and logging to screen, then six pairs on the
claimed winners and suspicious regressions. Use the existing fixed GCC, affinity,
hash, JIT-off and quiet measurement protocol. Obtain separate post-warmup
instruction diagnostics for the claimed mechanism; elapsed time decides admission.
No padding experiments, counter-informed benchmark changes or busy-build timings.

Promotion should show a repeatable benefit on at least two unchanged kernels,
or explicitly remain an unadmitted narrow experiment. Preserve logging's upstream
parity and report direct selected50 losses even where upstream headroom remains.
Any clear repeated regression needs a causal explanation and coordinator decision;
an aggregate mean cannot silently waive it. The two-pair all29 screen is a warning
detector, not enough to dismiss a material loss. Compare admitted combinations
directly with selected50 and immutable golden39/upstream. Do not multiply ratios
from separate campaigns. Full diagnostics and the strict value API gate belong
at the coherent merged checkpoint, with established failing-case limits reported.

## File overlap and likely combinations

IDs 1 and 3 are mostly disjoint: feedback/specialization versus frame reservation.
IDs 2 and 4 both edit generator proof/publication code but establish different
facts: a container body edge versus an arithmetic result join. Give them separate
helpers/tests and merge the producer changes before regenerating once. Their
combination is attractive after independent wins; neither depends on the other.

ID 5 shares frame declarations with ID 3 and return-body generator validation
with IDs 2/4. Agree the explicit normal-completion interface before merging it;
do not copy cleanup bodies into RETURN_VALUE. IDs 3 and 5 can compose because
reservation and retirement preserve the same NULL-storage invariant, but run
reentry/frontier and escaped-generator tests on the merged tree. Every option
touching `bytecodes.c` owns a separate uop region; generated output is an
integration artifact, not independent source ownership.

Prefer the measured combination of IDs 1, 2 and 4 first, then add a winning ID 5.
ID 3 has the largest native-code uncertainty and should stay isolated until its
own evidence is convincing. None of these options changes source-independent
execution, removes observer semantics, or revives the rejected adjacent method
refinement/mini-leaf evaluator. Final combinations require direct measurement;
individual speedups and code-size reductions do not add mechanically.
