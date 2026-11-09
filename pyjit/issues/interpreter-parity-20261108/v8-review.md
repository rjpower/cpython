# Interpreter review against V8

2026-11-08. Independent source/design review of runtime `0953757f4e7`.
No runtime source, build, workload or commit was changed for this review.
The recommendation decisions below are review recommendations; the coordinator
owns acceptance, implementation and measured admission.

## Finding and target

The register IR is adequate for the next interpreter increment. Its fixed
word and owning windows impose real constraints, but they do not require the
repeated provider discovery, scalar canonicalization of code objects, and
duplicate successful refresh checks visible in the implementation. Start with
those losses of already established facts. A wholesale switch to V8 bytecode,
an accumulator or optimizing SSA is not supported by the workload evidence.

Keep `logging_silent` as the first target. The retained final campaign gives
candidate/upstream elapsed **1.743034**, instructions per workload loop
**1.720317**, and selected cycles per workload loop **1.629600**. The previous
instructions ratio was 1.774821. These are different measurements. Richards'
elapsed ratio is 1.252098, but it lacks the same retained detailed profile.
Logging has the stronger diagnostic starting point, though its larger gap
means parity requires roughly a 42.6% elapsed reduction from the current
candidate. No recommendation here predicts that reduction.

The actual benchmark issues ten `logger.debug(message)` calls per loop.
`Logger.debug(self, msg, *args, **kwargs)` then calls `isEnabledFor`, normally
using a warmed dictionary entry. Its fresh kwargs dictionary is observable
Python state. Removing that allocation, sharing the dictionary between calls,
or destroying locals at their last future read would change the contract.
The no-kwargs common-call shortcut is therefore a secondary candidate. The
coordinator's fresh `warmed-ir.json` capture confirms ten `CALL_PY_GENERAL`
and ten `LOAD_ATTR_METHOD_WITH_VALUES` sites in the 67-word benchmark body;
the 18-word `Logger.debug` body contains an inner `CALL_PY_EXACT_ARGS`.
The 72-word `isEnabledFor` body has two `LOAD_ATTR_SHAPE` sites and the hot
dictionary read. This confirms that the proposed nonallocating overlap route
does not cover either hot call shape. These are warmed sites, not execution
counts of every branch or provider.

Sources: [benchmark](../../bench/internal/kernels/logging.py), runtime
`Lib/logging/__init__.py:1518,1791`, the
[qualified report](../../bench/results/interpreter-architecture-20261107.md),
retained `B/final-analysis/analysis.md`, and the coordinator's
`interpreter-parity-20261108/warmed-ir.json` in the same evidence storage.
That capture records R5 identity, JIT disabled, CPU15 affinity and 20,000 warmup
loops. In this review, `R` is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`
and `B` is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107`.

## Comparison boundaries

V8 source is pinned to **`63481c19453c72221531058be6d2e94adf3ebc66`**, resolved
from upstream `refs/heads/main` during this review. All source links below use
that revision. The 2016 Ignition and 2021 Sparkplug posts are historical design
descriptions, not measurements or claims about today's exact pipeline.

Ignition is a baseline register interpreter with an accumulator and explicit
virtual register operands. Its generated handlers use low-level compiler
machinery. That does not make its bytecode an optimizing SSA IR or allocate
JavaScript virtual registers directly to hardware registers. Our frame homes,
compiler temporaries and future method/tracing SSA must remain distinct too.
See V8's [Ignition description](https://v8.dev/blog/ignition-interpreter) and
[Sparkplug frame explanation](https://v8.dev/blog/sparkplug).

| Area | Current fork | Relevant V8 mechanism | Classification |
| --- | --- | --- | --- |
| Operands | Fixed 64-bit word; four 12-bit fields; explicit output; some contiguous windows | Accumulator plus typed/scalable operands and register lists | IR/layout choice, with different density/decoding tradeoffs |
| Baseline allocation | Occupancy-aware compactor preserves ownership, stack retirement order and windows | Register-equivalence optimizer materializes observable registers and lists | Compiler policy; neither is optimizing SSA |
| Calls | Exact direct push; common positional overlap; transaction for fallible preparation | Separate receiver/arity call forms and generated call builtins | Specialization and provider implementation |
| Entry | Known-NULL overlap reuse; initialize new tail and constants | x64 entry initializes its local/temp register file with undefined | Validity is required in both; skipping arbitrary initialization is unsupported |
| State publication | Canonical owning homes, fault PC, refresh after possible reentry | Explicit dispatch interface and call-aware bytecode-offset saves | Effect/observation protocol and code generation |
| Retirement | Reference-count releases can execute Python; exposed locals retain owners | Tracing collection follows stack roots | Python semantic/selected memory-management cost |

Primary comparisons: [operand types](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-operands.h),
[register optimizer](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-register-optimizer.cc),
[x64 interpreter entry](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/builtins/x64/builtins-x64.cc#L983),
and [V8 collection model](https://v8.dev/blog/trash-talk).

The following are already implemented and are not missing optimizations:
thread frame stacks; common positional argument overlap; defaults/empty
variadic filling; reuse of known-NULL overlap; locals-only ordinary thread
frame clearing; consuming-fallback outlining; direct immortal empty tuple;
first-output publication and the narrow canonical shape-source fact. The
current interpreter also already folds local loads/stores, discards call
results and fuses selected compare branches. Existing effect metadata includes
raises, escapes, Python execution, polls, allocation and frame switches. The
remaining issue is using facts at the right phase, not inventing those flags
again.

## Ranked recommendations

### 1. Carry specialized Python-call facts into common preparation

**Accept design; warmed IR confirms the target; obtain route counts before
implementation. Implementation opportunity, not an encoding constraint.**

`R/Python/generated_ir_cases.c.h:7288` checks PEP 523, exact function type,
function version and recursion, then invokes the generic vector transaction
with `CALL_PREPARED` unset. `ceval.c:3287` consequently considers method
expansion and rediscovers function/provider/hook facts before
`call_transaction_overlap_python`. The unbound specialized route has no
escape between its guards and that invocation. The generic transaction loses
a fact the handler just proved. Bound-method expansion is different: retiring
the wrapper can reenter and requires refresh.

`specialize.c:2140` only installs this specialization for standard vectorcall.
`Objects/funcobject.c:472` clears the function version before changing
vectorcall through `PyFunction_SetVectorcall`. This supports carrying that
provider proof through the guarded nonescaping span; post-allocation entry
must still revalidate it. Tests must exercise that public setter, not assume
that a function's provider never changes.

V8's [call handlers](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-generator.cc#L1446)
carry receiver/arity knowledge into generated call entry. Apply that principle
through one internal shared prepare/commit implementation; do not copy V8's
JavaScript calling convention.

The concrete design below skips method preparation/provider rediscovery only
while its proof remains valid. After `PyDict_New`, retain callable, code,
vectorcall, hook, executable, capacity, defaults and self-offset guards. It
removes pre-allocation generic branches and some transaction marshalling;
it cannot remove the fresh kwargs allocation or promise an elapsed gain.
Admission: exact/general/expanded/generic and declined routes; mutated code
and defaults during allocation; monitoring once; PEP 523; errors, generators,
keywords/EX and free-threaded fallback; release/debug generated-code review.

### 2. Preserve the known code-object type at frame initialization

**Accept a small independent slice. Implementation opportunity.**

`R/Include/internal/pycore_interpframe.h:283` installs the executable through
`PyValue_DUP(PyValue_FromObjectBorrow(code))`. The generic constructor
(`pycore_value.h:761`) checks exact float/int canonicalization, although the
parameter is a code object. Retained R5 GCC output
`B/r5-gcc/call_transaction_overlap_python.txt`, addresses
`0x334328–0x334374`, contains those scalar-type tests and ownership work.
The exact R5 ELF build ID is `9d98a70694382c4650742915f0560f12346c85bf`.

Use one explicitly constrained canonical nonnumeric-object constructor or
typed code-home wrapper, sharing immortal tagging and duplication rules with
the ordinary constructor. Audit/update all applicable frame executable
initializers, including trampoline entry. `_PyValue_PointerSteal` is currently
documented for boxes and canonical constructors; using it ad hoc at the frame
site would violate that boundary. Keep the eight-byte canonical home and
executable owner. V8's [typed dispatch descriptors](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/codegen/interface-descriptors.h#L2432)
illustrate preserving type facts at generated boundaries.

Expected work removed: impossible scalar canonicalization branches, not a
reference count or code lifetime. Test mortal/immortal executable ownership,
ordinary/overlap/generator/trampoline creation, exception clearing, frame
exposure and `code.replace` constants. Verify actual GCC removes the checks;
measure target and call/non-call controls before keeping performance-only
API complexity.

### 3. Remove the duplicate successful call-refresh validation

**Accept a small independent slice. Implementation defect.**

`R/Python/ceval.c:1422` calls `value_operation_refresh`, then immediately
`value_frame_is_current`. Successful refresh already checked the current frame,
executable, words and PC range, and no callback occurs between these checks.
An offline disassembly of the retained R5 ELF shows the duplicate executable
load/null/words checks at `0x333a3e–0x333a55`, after the successful PC store at
`0x333a38`; GCC has not removed them.

Return immediately on refresh success. Preserve the existing failure-path
second validation and error handling, because constructing an error can
escape. This leaves all actual post-escape validation and fault-PC publication
intact. V8's [call-aware state publication](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-assembler.cc#L751)
is the relevant comparison: publish where required, without redoing an
unchanged successful check. Test allocation/reentry and existing transaction
failure cases; inspect release GCC and count refreshes separately from calls.

### 4. Use phase contracts to reduce handler state traffic

**Accept bounded proposal work. Emitter/implementation opportunity; Python
observation remains a constraint.**

`R/Tools/cases_generator/ir_generator.py:2534–2605` flushes and reloads around
call preparations; `finish_value_operation` rereads word/slots after cleanup.
`IRStack.store:2039` and the R4/R5 result-phase work already distinguish
temporary publication from a folded-local store. The retained shape report
identifies two extra companion-slot address calculations on a local route.
Fix that known emitter dataflow first, using the shared flush preparation.

Then audit one hot helper sequence for an already-published PC/home or
unchanged decoded field with no intervening escape. Express the proven phase
in the shared checked contract and consume it locally. Keep the existing
whole-handler effects conservative; specialization can miss, and a folded
local's old-owner release can run Python. Do not infer purity merely from a
specialized opcode. V8's
[assembler](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-assembler.cc#L23)
tracks call invalidation and saves offsets according to call behavior.

Expected work removed: redundant address calculation, stores/reloads or
refreshes in a proved phase. No publication fact crosses a callback unless
the existing contract explicitly guarantees it. Admission must include
temporary/local/discarded destinations, object/scalar payloads, local writes
by callbacks, error ordering, line/monitor events and the generic path.

### 5. Make compiler equivalence ownership-aware before adding an accumulator

**Accept a bounded inventory; defer wider compiler changes pending evidence.
Compiler/IR policy constrained by Python ownership.**

`R/Python/regcfg_passes.c:240` folds single-reader borrowed local loads and
kills pending loads across possible Python execution or local writes. It
preserves source-line boundaries. `regcfg.c:898` also tracks dead-but-owned
temporaries and contiguous windows. These constraints explain why simply
borrowing caller locals or applying last-read liveness is unsafe.

V8's [equivalence optimizer](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-register-optimizer.cc#L322)
distinguishes materialization and observable registers, and materializes
register lists. Inventory the warmed logging IR's surviving moves and owner
duplicates by reason: semantic snapshot, window placement, observer boundary,
or avoidable compiler limitation. Start with a same-line, nonescaping region
whose immutable inline values or existing owning homes prove equivalence;
remove one transfer without changing destruction time. Do not weaken the
family-wide reentry kill or retain mortal borrowed locals across argument
evaluation.

Expected work removed only after such a proof: a dispatch or slot transfer,
possibly its duplicate count. Test local mutation through frames, argument
evaluation order, exceptions, line jumps and dead-but-owned loops. A new
cross-handler accumulator requires explicit roots/recovery and is deferred.

### 6. Assess operand-class and call-window costs with dynamic denominators

**Accept measurement; defer encoding changes. IR/layout constraint.**

`R/Include/internal/pycore_ir.h:20–106` reserves all 64 bits; fields identify
homes but do not encode local/temporary class. `ceval_macros.h:505–636`
therefore performs class-dependent consumption/publication. Calls also require
evaluated owning temporary windows. V8 has
[receiver/arity-specific bytecodes](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecodes.h#L293)
and lists; its smaller forms do not justify losing Python argument snapshots.

Count executed class tests, window-producing transfers and decoded operands
per hot semantic operation, separating temporary/local destinations and call
shapes. Compare with actual GCC to exclude tests already folded by the
generator. Only then design an encoding or handler refinement that eliminates
material work without multiplying a general call ABI. A new class bit would
trade slot range or another field; specialization/instrumentation currently
share operand layouts. Require an explicit range/large-function, branch,
instrumentation and generic-fallback migration plan before such a revision.
This is deferred behind recommendations 1–4.

### 7. Measure dispatch/footprint before changing the handler backend

**Accept evidence collection; defer backend and word-width rewrites.
Implementation and layout tradeoff.**

`R/Python/ceval_macros.h:64–106` passes five state arguments under GCC's
selected tail-call convention; it does not explicitly assign machine
registers. V8's [dispatch descriptor](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/codegen/interface-descriptors.h#L2432)
and [tail dispatch](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-assembler.cc#L1286)
make that machine interface explicit. Our fixed eight-byte word gives simple
decoding but spends space on absent operands; V8's
[operand scaling](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-operands.h#L59)
takes a different tradeoff.

For the warmed target, retain bytecode/feedback size, handler transitions and
GCC spill/reload sequences at hot helper boundaries. Normalize instructions
and branches by semantic operations and dispatches, with code-cache counters
treated cautiously. First remove a demonstrated spill or helper boundary
within current C/GCC generation. Introduce compact words or a custom handler
backend only if this evidence identifies a material remaining limit. Do not
change compiler or claim V8's generated-handler architecture proves C cannot
perform adequately. Test all dispatch, instrumented, stats and error targets
for any eventual interface change.

### 8. Keep native recovery separate and reuse baseline homes deliberately

**Accept architectural boundary; defer implementation to the selected JIT
workstream. Not a JIT-off parity optimization.**

The baseline already supplies physical canonical homes and fault PCs; future
optimizing code needs effect-prefix, owner and resume-state recovery beyond
those words. Follow the reviewed
[recovery contract](../../design/jit-e2e-system.md) rather than treating
baseline liveness as a root or deoptimization map. V8's historical
[Sparkplug design](https://v8.dev/blog/sparkplug) shows why compatible baseline
frames can simplify native entry and observation; it is not a reason to make
the baseline itself optimizing SSA.

The smallest later slice remains prepared owners plus shared once-only
transfer commit and forced native exits. Its work removal would be native
decode/dispatch elimination; it cannot count toward a JIT-disabled result.
Admission requires actual allocation failures, callback invalidation,
exceptions, exactly-once effects and monitoring during recovery. Preserve the
current interpreter as the semantic provider while this separate work proceeds.

## Concrete common-call preparation design for recommendation 1

Implement the smallest unbound specialized Python route first. Keep existing
word fields, return destinations, transaction status and generic provider.
The one conceptual division is **guarded facts → fallible preparation → final
guards → shared infallible commit**.

1. At `CALL_PY_GENERAL`, the function/version and PEP 523 guards establish an
   exact Python function before any escape. Mark method preparation complete
   using the existing `CALL_PREPARED` meaning; no method expansion is needed.
   Carry the exact-function fact to the private preparation entry. Do not
   infer it from the opcode name inside a generic provider or preserve it on
   a deopt/fallback edge that did not execute the guards.
2. Enter the same common positional preparation through a private checked
   function entry, using scalar arguments or a transaction record that stays
   in the out-of-line helper's C frame. Avoid taking a handler-local address.
   Generic CALL still discovers its provider and uses the same preparation
   routine after proving the required entry facts. Bound-method preparation
   keeps its existing refresh; its pre-expansion function version cannot be
   reused after wrapper retirement.
3. Check positional shape, executable and capacity before allocating. Retain
   the pinned selected code through allocating kwargs preparation. The initial
   type/hook/provider proof only avoids rediscovery up to this escape. For
   `CO_VARKEYWORDS`, allocate the required fresh empty dict and refresh the
   actual caller once. On error, preserve the existing owner/error path.
4. Revalidate all mutable entry facts after allocation: actual window callable
   and code identity, standard vectorcall, hook, executable/capacity, defaults
   coverage and self offset. Use a fresh view of the owning caller window.
   A guard decline releases prepared resources, refreshes, then enters the
   existing fallback once. A preparation/refresh failure stays an error.
5. Both entry routes reach one private nonescaping commit using the checked
   code and argument mapping. Reuse the existing overlap primitive and NULL
   proof. Transfer callable and arguments once; fill default owners, empty
   varargs and prepared kwargs; publish return slot and current child before
   any escaping release. No allocation, deopt, callback or failure may occur
   from first detachment to clearable child publication. Retain current
   recursion/RESUME, monitoring and child-not-started cleanup.

The implementation may first land only the existing `CALL_PREPARED` proof for
this specialized route, then remove provider rediscovery in a second reviewed
slice if GCC and dynamic counts justify it. Share the commit rather than
duplicating overlapping frame setup. The no-kwargs shortcut can reuse this
same commit later, but it is not the primary logging proposal.

The coordinator's warmed IR confirms `bench_silent`, `Logger.debug` and
`Logger.isEnabledFor` as described above. Next obtain separate stats-only
counts: direct exact, general specialized, expanded method,
accepted allocating/nonallocating overlap, each decline, consuming binder,
native, monitoring and error. Reconcile mutually exclusive outcomes. Source
predicts call shapes; samples do not establish their frequencies.

Expected work removed is finite and inspectable: unnecessary method tests,
duplicate pre-escape type/provider/hook tests, and any marshalling/spills that
the private entry actually eliminates. The post-allocation guards and frame
ownership work stay. Require fresh GCC disassembly, focused semantic tests,
balanced target/control timing and independent cold source/result review.
Keep the change only on correctness and measured benefit.

## Measurement review and limits

The coordinator's [plan](plan.md) has the right target, fixed workload and
JIT-off constraints. Two fresh six-round campaigns with GM at most 1.00 and
each round at most 1.02 are an empirical parity admission rule. Before capture,
fix the estimator as each process's arithmetic mean of all seven timings,
paired candidate/upstream ratio per round, then GM of six ratios. Balance
three upstream-first and three candidate-first rounds; retain the same fixed
loop count across both campaigns. Assert native CPU15 affinity immediately
before timed work in both roles. Retain all raw values, failures, identities
and load observations. Restart a rejected campaign into a new evidence
directory; do not replace selected rows. Report spread and both campaigns'
raw ratios. Passing this rule is repeatability evidence, not a statistical
proof that every execution is equivalent.

Review-only verification: read actual runtime source and retained GCC output,
checked R5 ELF identity, compared pinned primary V8 sources and read the
coordinator's warmed-IR capture. This reviewer ran no new native
workload, test, build or performance capture. Existing profiles cannot
assign elapsed cost to source checks or prove that these changes reach parity.
