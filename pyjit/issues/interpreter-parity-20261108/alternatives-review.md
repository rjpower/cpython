# Architectural alternatives before selecting the next batch

2026-11-08. Independent second pass over runtime `0953757f4e7`, canonical
contracts, the eight V8 recommendations and the coordinator's warmed logging
capture. Source review only: no builds, tests, native workloads, counters or
benchmarks. The user requested this comparison before implementation priority
is selected. Earlier source approval of the small refresh/code-home changes
does not select them, or the specialized call route, as the next architecture.

## Decision to make

The retained target factor is 1.743034, so matching that upstream measurement
would require about 42.6% less candidate elapsed time. Two tiny cleanups and a
future-JIT boundary do not explain how to close that gap. Neither samples nor
source establish a savings budget for any mechanism below. Even a correct
implementation that removes visible checks can be immaterial.

Compare six mechanisms on the same target and call/non-call controls. This is
a choice among changes to the interpreter's work, generated C, compiler and
instruction interface; it is not a choice between universal values and a
return to boxed heap fields. GCC stays fixed, JIT stays off, and reference
counting, observer behavior and canonical owning homes remain requirements.

The warmed capture supplies unusually useful discrimination. Its 67-word
outer body has ten method loads, ten `CALL_PY_GENERAL` sites, ten argument
`LOAD_FAST_BORROW` sites in the repeated body, and ten result `POP_TOP`s.
The 18-word `Logger.debug` body has a method load and `CALL_PY_EXACT_ARGS` on
the silent route. `isEnabledFor`'s first nine words contain two shape reads,
boolean branching, cached dict subscription and returns; most of its 72 words
belong to other paths. Static counts for the whole function must not be used
as dynamic hot-path denominators. Route outcomes, escape counts and native
instructions still need separate diagnostic evidence.

| Mechanism | Logging coverage hypothesis | Broader coverage | First discrimination | Migration risk |
| --- | --- | --- | --- | --- |
| A. Fact-carrying call entry and shared preparation | High at outer calls; does not remove inner exact entry cost | Defaulted/variadic Python calls, both bound and unbound after separate proofs | Reconcile route counts and pre/post-escape checks | Medium |
| B. Phase-aware state publication and leaf paths | High across method/shape loads, cache read, return and call continuation | Attribute/item/arithmetic-heavy code | Count actual publications, refreshes and escapes on the hot prefix | Medium; high if line boundaries are fused |
| C. Ownership-aware equivalence and window construction | Directly targets repeated argument snapshot/window words | Call construction, expressions, unpack/build windows | Explain every surviving transfer in warmed IR | Medium to high |
| D. Operand-class specialization or layout revision | Potentially broad across short handlers; existing window operands are already temporaries | Nearly all register operations | Count surviving class branches in release GCC | Medium for handler forms; high for encoding |
| E. Dispatch interface, fusion and word density | Broad across all hot words, including ten caller POPs | Dispatch-heavy loops and large instruction streams | Normalize state spills and dispatches; distinguish cache pressure from instruction work | Medium for one fusion; high for interface/format |
| F. Frame entry/retirement and constant-home policy | Both Python frames per disabled logging call | All short Python calls and generators | Count actual initialization/clear/constant work by call shape | Medium for one typed field; high for validity-map changes |

These are coverage rankings, not expected-speedup rankings. A, B and F all
touch the same calls and their savings cannot be added as independent shares.
C and E can remove the same dispatch. D can change register pressure and
erase gains elsewhere. Native recovery remains deferred under recommendation
8 because it cannot contribute to a JIT-off result.

## A. Preserve facts through a shared call preparation/commit

Recommendations 1 and part of 4. Source:
`generated_ir_cases.c.h:CALL_PY_GENERAL`,
`specialize.c:specialize_py_call`, `funcobject.c:PyFunction_SetVectorcall`, and
`ceval.c:call_transaction_invoke/call_transaction_overlap_python`.

Competing designs are (1) carry only the already established method-prepared
fact through the existing scalar call interface; (2) add an explicit private
guarded Python entry sharing the preparation/commit implementation with the
generic entry; (3) replace generic preparation with a wider persistent call
plan. The third has greater state and invalidation cost and is unsupported
until the first two identify a material limit. A new general call ABI or
opcode-name table is unnecessary.

Under the held GIL, successful type/version/PEP guards imply standard Python
entry before an escape; the public vectorcall setter clears the cached
version. Provider rediscovery before that escape is optional. After kwargs
allocation, callable/code/provider/hook/executable/capacity/defaults/self
mapping must be revalidated. Fresh kwargs, defaults ownership, recursion,
monitoring and clearable child publication remain real work. The nonallocating
shortcut does not cover `Logger.debug` and the warmed inner call is already
exact, so it is a poor leading logging hypothesis without new route evidence.

Smallest experiment: first count mutually exclusive entry/outcome shapes and
the allocations/refreshes within accepted overlap calls. Inspect GCC for the
specific pre-escape duplication that a private entry would remove. Then
compare one private entry with the smaller prepared-bit variant, keeping
the same shared commit. Test the generated fallback label separately: it must
retain generic entry whenever guards did not complete. Bound-method retirement,
monitoring and free-threaded paths cannot inherit the unbound proof.

Reject as the next major mechanism if the targeted route is uncommon, GCC
already eliminates the duplication, the entry merely moves spills, or the
post-allocation/entry work dominates and the target/control screen shows no
repeatable benefit. Correctness failure on mutation, setter, allocation,
monitoring or fallback rejects the implementation regardless of speed.

## B. Publish state at actual escape phases; make proven leaf paths small

Recommendations 3–4, with a larger scope than one duplicate refresh. Source:
`ir_generator.py:prepare_call/call_reload/reader_reload/IRStack.store`,
`ceval.c:value_operation_refresh`,
`bytecodes.c:LOAD_ATTR_SHAPE/LOAD_ATTR_METHOD_WITH_VALUES`, and the existing
first-output publication contract.

Competing designs are (1) remove individual duplicate stores/checks; (2) have
the shared generator carry checked phase facts such as PC already published,
result canonical in a temporary, and no escape since decoding; (3) generate
paired leaf/escaping continuations for selected handlers. A transitive leaf
proof must include releases and local displacement: a dictionary read or
attribute result may be simple while retiring its receiver can run Python.
Do not replace one conservative whole-handler flag with scattered optimistic
annotations. Use the generated contract and invalidate phase facts at escapes.

The logging hot prefix makes this a real competitor to call entry. It has
method lookup, shape-home results and a warmed dictionary read on every silent
path, including the inner exact-call route that A does not simplify. A result
that is inline or immortal may avoid a release callback, but that is a value
fact to check or prove, not a property of all attribute lookups. Observable
local publication and post-callback refresh remain required. Generic scalar
canonicalization at known-code entry is a narrow instance, not the whole idea.

Smallest experiment: map one actual shape-read → cache-subscription → return
prefix into phases. Count published PC/home stores, actual escaping calls,
receiver/displaced-owner releases and refreshes separately. Inspect GCC to
see which state reloads survive. Propose a single generator rule that removes
only unchanged state traffic, with explicit invalidation and generic fallback.
Compare it with the smallest local C simplification on the same prefix.

Reject if all apparent redundancy follows genuine escapes, if the leaf guard
cost exceeds saved traffic, or if the generated rule needs per-handler hidden
exceptions. Callback mutation, exception ownership, tracing and folded-local
destinations are decisive tests. A tiny isolated cleanup may still be useful,
but cannot serve as evidence for broad phase-contract benefit.

## C. Change compiler equivalence/window construction rather than call entry

Recommendations 5 and part of 6. Source:
`regcfg_passes.c:_PyRegPass_CopyProp`, `regcfg.c:compact_op` and the window
verifier. The compiler already folds borrowable single-reader local loads,
kills pending loads across possible Python execution/local writes, and tracks
occupancy until consumption rather than future-read liveness alone.

Competing designs are (1) improve equivalence inside the current same-line,
nonescaping regions; (2) choose destination windows earlier so producers write
directly into their final owning argument homes; (3) allow calls to gather
arguments from independent registers or locals. The third removes contiguity
as an encoding constraint but loses simple zero-copy child overlap and must
make evaluated owning snapshots before any fallible/reentrant step. A gather
list is not automatically cheaper than today's argument window.

The target retains one message-local-to-window word per outer call. It may be
the necessary snapshot under the current instruction interface, not allocator
waste. The method loader already writes callable/self into the window. There
is no broad copy chain in the warmed nine-word `isEnabledFor` hot prefix; its
many COPY/SWAP words are mostly elsewhere. That weakens a compiler-only route
to parity here while leaving broader call-expression coverage plausible.

Smallest experiment: annotate every surviving load/move as evaluation snapshot,
contiguity, line/observer barrier, retirement-order preservation or avoidable
pass limitation. Identify a producer that could target its final window home
without changing release time, then compare emitted words and GCC. If the
only candidate is the message snapshot, first compare the cost of its current
word with a call form that duplicates that local into the same owner slot
inside the call; this saves a dispatch, not the necessary owner acquisition.

Reject if no avoidable transfers remain on the actual hot path, or if gather/
coalescing adds more ownership work or loses overlap. Frame-local mutation,
argument evaluation order, exception unwind and dead-but-owned temporaries
must remain correct. Last future read never licenses early local destruction.

## D. Encode or specialize operand class

Recommendation 6. Source: `pycore_ir.h`'s full 64-bit budget and
`ceval_macros.h:IR_IS_LOCAL/IR_CONSUME_INPUT/IR_STORE_DST`.

Competing designs are (1) retain words and let generated handlers specialize
the relevant source/destination classes; (2) encode class bits by trading
handler or operand range; (3) use feedback/side metadata to avoid that trade.
The first spends code/handler footprint and specialization complexity; the
second changes assembler, decoder, limits, instrumentation and readers; the
third adds a load and may cost more than the comparison it removes.

Slot class and value ownership tag are different. Class specialization can
remove a branch or address/header load; it cannot remove a mortal count,
canonicalization, local-retirement callback or evaluated call snapshot.
Window temporaries already give stronger class knowledge than arbitrary
register operands, so calls may be a weak direct beneficiary. Short shape,
dict and arithmetic handlers offer wider coverage.

Smallest experiment: choose the most frequent hot handler with a real
remaining local/temporary test in GCC, enumerate its observed class combinations,
and compare a generated fixed-class variant before revising any word bits.
Measure instruction reduction against code footprint and spills. Only a
material result justifies a range/large-function and instrumented-handler
migration proposal.

Reject if tests already fold away, class variants proliferate, new branches/
metadata loads replace the same work, or footprint/control regressions exceed
the target gain. A handler-interface improvement can be kept without implying
that the whole IR format must change.

## E. Reduce dispatch/state-interface cost and assess density separately

Recommendation 7 and the phase contract in 4. Source:
`ceval_macros.h:TAIL_CALL_PARAMS`, `ir_generator.py:call_completion`, and
`regcfg_passes.c:discard_region`.

Competing designs are (1) preserve fixed words and reduce C helper boundaries/
state pressure; (2) fuse common semantic sequences with explicit observation
boundaries; (3) change the dispatch interface or use compact/scaled words.
The current GCC convention passes five state arguments but does not assign
them permanent hardware registers. More inlining can remove an ABI transition
while increasing code size and register pressure. Compact words can reduce
instruction-stream footprint while adding decoding and complicating rewriting;
the small hot logging bodies do not by themselves demonstrate a density limit.

A concrete missed target coverage is important: all ten outer call results
still have `POP_TOP`. Discard support exists, but `discard_region` deliberately
keeps the POP when it ends the call's line. If tracing starts inside the
callee, the first caller line event must still identify that call line. Blind
POP removal is wrong. An observer-aware call-return continuation or fused
call/discard form could represent this boundary explicitly and remove normal
dispatch/result traffic while retaining activation and exceptional behavior.
This is materially broader than the prepared-bit call change, and riskier.

Smallest experiment: count hot dispatches, spills/reloads at call/return and
the actual cost of the retained POP continuation. Write an explicit two-PC/
line-boundary recipe for one call-discard pair, including tracing enabled
inside the callee, before proposing a fused handler. Compare that against
reducing one helper boundary without changing instruction/observer semantics.
Use instruction/cache evidence to decide whether density deserves a separate
experiment; do not infer it from byte width alone.

Reject fusion if it merely hides a second dispatch or needs special logging
knowledge, or if observer activation cannot recover exactly the retained
boundary. Reject a dispatch/interface rewrite if spills move elsewhere or
fixed-GCC controls regress. Density is rejected as the near-term explanation
if the hot stream fits and measured front-end evidence shows no material
pressure; noisy cache counters alone do not establish it.

## F. Revisit frame work through validity and retirement contracts

Recommendation 2, the frame-entry comparison and part of 4. Source:
`pycore_interpframe.h:_PyFrame_Initialize/_PyFrame_PushOverlapping`,
`frame.c:_PyFrame_ClearExceptCode`, and `bytecodes.c:_RETURN_VALUE`.

Competing designs are (1) keep canonical frame homes and specialize typed
fields/known-valid ranges; (2) change constant access/placement to reduce
per-entry hidden-home filling; (3) introduce initialized/owned range metadata
so entry, exposure, GC and unwind need not treat every physical slot as valid.
The third is a genuine architectural migration and cannot be approximated by
using future-read liveness. Locals, GC, frame copying and exception consumers
would all need one coherent validity contract before initialization is skipped.

The current frame stack, argument overlap, known-NULL reuse and locals-only
normal thread cleanup already exist. There is no second full-slot scan to
remove on ordinary return. Frame headers, code/function owners, live argument
owners, fresh kwargs lifetime, constants and observable local destruction
remain work. A typed code constructor removes impossible scalar checks, not
the executable owner. Changing constant placement trades entry stores for
loads/dispatch forms and must include mortal `code.replace` constants.

The constant-placement alternatives deserve their own comparison. Today a
hoisted `CO_FAST_CONST` occupies a hidden frame-local home, filled at entry
and visited by normal local clearing. An alternative can read the existing
code-owned canonical `co_consts` pool directly, with a constant operand class
or a generated handler form; another can retain frame homes but initialize a
proved constant range only at its first required phase. The former trades
per-frame fill/clear and footprint for constant-base decoding/loads, additional
operand forms and borrow-proof obligations. It need not add a competing value
representation or a second constant pool. The latter adds validity state and
observer/exception complexity and may merely move the same stores.

Code ownership is separable from constant placement. A frame's executable
owner can keep its code and `co_consts` alive while a constant operand is
borrowed; the ordinary consumer must still duplicate a mortal value whenever
it needs a new owner. `code.replace` creates another code object and can install
mortal or mutable constant objects. Preserve the active frame's original
code/pool, finalization order, cyclic traversal and hidden-versus-exposed locals
rules. Do not assume constants are immortal, or use an unowned pointer from a
replaceable function code field. A proposal must audit every consumer that
currently treats hidden constant slots as ordinary initialized homes.

Coverage is currently unresolved, and source narrows it sharply. The compiler
`regcfg_passes.c:_PyRegPass_ConstSlots` nominates loads only in backward-jump
loop blocks and returns without hoisting when there are none. Neither warmed
logging callee has a backward jump. The JSON records source `nlocals` values
4 and 3, but omits `co_nlocalsplus`, total slots and `co_nconstslots`; it does
not establish any per-call hidden-constant cost here. Obtain all four counts,
the actual constant-slot table and known-NULL overlap prefix before claiming
that cold regions enlarge entry work. A zero constant-slot count rejects
constant placement as this target's leading explanation, while leaving it a
broader loop/short-call tradeoff. Cold exception regions can instead enlarge
temporary capacity; their physical range still needs the separate validity
proof if entry initializes it unnecessarily.

Smallest experiment: account separately for header stores, newly NULLed tail,
constant-home fills, argument/default owners and actual locals cleared for
the two logging callees, including exact versus general entry. Compare the
typed-field cleanup with a source prototype of direct constant access on one
short nonescaping operation, before considering validity metadata. Inspect
the exposed/generator paths separately; they cannot borrow ordinary thread
assumptions.

Reject a broad frame redesign if repeated NULL work is already absent, if
constants/headers are a small fraction, or if alternate access costs cancel
entry savings. Any change that hides a live owner from GC/frame exposure or
changes finalizer order is invalid. Lazy/shared kwargs and early destruction
of user locals remain outside the admissible choices.

## Evidence needed for selection

The next decision should compare a single diagnostic account across A–F:
semantic calls and returns, mutually exclusive route outcomes, actual escape
phases, class branches, window transfers, initialized/cleared homes, dispatches,
and GCC spills/retired work. Use the real warm path and unchanged controls;
report diagnostics separately from release timings. No large implementation
is required to discover that a mechanism has little target coverage.

Require each candidate proposal to state what work vanishes, where that work
is currently executed, which invariant makes removal valid, and which result
would end the experiment. Select one coherent small discriminating increment
per mechanism under consideration before expanding scope. Keep the reviewed
tiny fixes available, but do not substitute their correctness for an
architectural answer or their source size for measured value. The coordinator
and user retain the batch-selection decision.

## Follow-up evidence: frame coverage resolved

The coordinator's native `frame-layout.json`, read independently after this
draft, identifies R5 `0953757f4e7` and reports:

| Callee | `nlocalsplus` | Temporary slots | Constant homes | Frame words, including header |
| --- | ---: | ---: | ---: | ---: |
| `Logger.debug` | 4 | 5 | 0 | 22 |
| `Logger.isEnabledFor` | 3 | 8 | 0 | 24 |

The header is 13 words by subtraction in both rows. The recorded local-kind
mapping contains no hidden constant homes. **Reject constant-pool relocation
as a logging-parity mechanism on this evidence.** It remains a broader IR
tradeoff for loop functions. This is a useful rejection before implementation,
not a reason to deny the alternative was architectural.

The eight temporary slots in `isEnabledFor` cover the whole function while
its warmed cache-hit prefix uses fewer. That establishes excess capacity
relative to this path, not eight avoidable initialization stores per call:
the current overlap can reuse a valid caller prefix. Account for the actual
new tail before considering validity metadata. The seven total user-local
homes across the two callees remain real owners or NULL; their observation
and destruction obligations do not vanish because the hot path is short.

Evidence storage is the coordinator-owned
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-parity-20261108/`.
The initial field-probe failure is retained there separately; this review
does not relabel it as a successful run. The coordinator also reports a new
six-round target baseline of 1.657289 with stable identities; its smaller
gap still requires about 39.7% candidate time reduction. The source alternatives
do not predict that result, and a baseline report is distinct from the two
campaign parity admission rule.
