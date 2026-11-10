# Conditional leaf entry and ordinary completion: Sol review

**Approve a bounded contract prototype after the class-scalar/classmethod
increments. Do not enable frame elision until entry observation and committed
completion have executable proofs.** The existing proposal identifies the right
saving, but its completion adapter is still a design requirement, not a reusable
runtime facility. A generic DAG executor and a growing list of getter recipes
are both poor first implementations.

This is a source review of runtime46 `b8e7e1bee6611eeda320ce163cd3ce89323da877`
in `build/worktrees/execution-boundaries-20261108`, under the
`python-boil-the-ocean` checkout. Current coordination and design documents were
read in the canonical `python-jit` checkout. The
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/) was
read. No runtime files changed, and no builds, tests or measurements ran.
Proposed names below are interfaces to implement, not existing APIs.

## What source inspection changes

* `Python/ceval_macros.h:IR_SKIP_RESUME` requires the live handler to be RESUME,
  full eval-breaker equality with the code instrumentation version, TLBC agreement
  where applicable, and Emscripten signal-clock handling. It proves only that
  this RESUME can be skipped. A function can have an uninstrumented RESUME and
  monitored RETURN, LINE, INSTRUCTION or BRANCH words.
* `_PUSH_FRAME` separately executes DTrace and LLTrace entry work. Neither is
  represented by the RESUME predicate. Debug `maybe_lltrace_resume_frame`
  (`Python/ceval.h`) checks the selected function's globals for `__lltrace__`
  and then `PYTHON_LLTRACE`. Checking only the caller's `frame->lltrace` is wrong.
* Generated `CALL_PY_EXACT_ARGS` already overlaps arguments; its prefix checks
  PEP 523, function version, arity, stack space and recursion. The leaf hook
  belongs after this prefix and before `_INIT_CALL_PY_EXACT_ARGS`. Bound-method
  expansion has already released its wrapper and is a different contract;
  exclude that handler initially. Split function/self calls need no expansion.
* Generated RETURN_VALUE holds canonical `temp`, unlinks/clears the child, then
  uses `_PyFrame_SetReturnValue`. Its ordinary branch caches `return_pc` before
  that call and rereads it only if the helper reports a previously pending
  redirect. `_PyFrame_SetReturnValue` samples `pending_redirect` before releasing
  the discard result or displaced destination. Those releases can run Python.
  This is an audit risk to settle with a witness, not evidence that a particular
  public redirect is currently admitted there. A new shared tail must not inherit
  that continuation assumption without proof.
* `value_operation_refresh` restores `instr_ptr` to the operation site. It is a
  preparation/value-operation primitive, not a completed-call refresh. Calling it
  after a committed redirect can erase the state completion must preserve.
* `call_transaction_native` publishes temporary results before retiring inputs,
  and has an optional periodic-at-end check. Ordinary Python return holds its
  result through child retirement. Reusing the native adapter would change both
  contracts. `_PyEval_CompleteStep` supplies a useful two-word terminal shape,
  but its SEND/FOR observer and publication semantics are not ordinary CALL's.

## One representation, with an explicit execution budget

Use one immutable **select of read paths** record on `_PyCodeExec`:

```
read_path = root(argument-index | constant-index | global-word-index)
            followed by zero to two attribute-word indices
predicate = absent | compare(read_path, read_path, comparison-mask)
region    = select(predicate, true-read-path, false-read-path)
```

Use four path records, at most two attribute steps per path, one comparison,
one branch and two terminal returns. Limit certification to 32 original words
and input retirement to eight window homes in the first experiment. These are
reviewable implementation/storage budgets; do not infer them from the target's
word count. Over-budget bodies and arities execute normally. Word indices refer
to final immutable IR and its feedback operands, never cached mutable pointers.

An absent predicate selects the first path. Argument and constant returns thus
need no new runtime recipe. A directly returned comparison uses True/False
literal arms, with literal roots represented by canonical singleton tags if
those constants have no code-constant indices. A single getter is a path with
one step. No per-function C generation, runtime node recursion or instruction-PC
loop is needed: read predicate operands, compare, read the selected result,
acquire it. A bounded loop over homogeneous attribute steps is acceptable; an
operation array containing loads, comparisons, branches and returns is another
interpreter and is outside this recommendation.

This grammar deliberately rejects nested predicates, joins requiring general
phi nodes, computed attribute names and arbitrary arithmetic. Those exclusions
are a size/performance boundary, not a claim that such programs are impure.
Do not add separate cases for their surface spellings. If this grammar cannot
be certified without a second CFG/execution framework, cut the experiment.

Build the record in `Python/ir.c:make_exec`, before execution publication.
Use a pointer in `_PyCodeExec` to a bounded scalar record allocated only for
accepted code; include its bytes in `_PyCode_ExecSizeOf`, free it in
`_PyCode_ClearExec`, initialize `_PyIR_InitCleanup`, and inspect the early
`fb_size == 0` return so constant/argument bodies are not skipped. Classification
rejection is ordinary. Optional allocation failure must leave no pending error
if optimization is declined. Avoid code back-references, Python owners and warm
feedback pointers in this NoScan metadata. No serialized format change follows
from this derived record alone.

## Whole final-IR certificate

The producer must account for every executable word, not search for a return
suffix. Decode immutable handlers/operands with `_PyCode_InstrHandler`,
`_PyCode_InstrOparg` and `_PyIR_Formats`; fused comparison operation/sense comes
from immutable feedback initialization, not the live entry's prediction.
Use generated field roles and effects as conservative rejection facts and a
small positive mapping to the supported semantic probes.

Require an entry RESUME with oparg zero, ordinary function flags, all
arguments supplied, no kw-only arguments, cells/freevars, variadics, handlers or
exception regions. Verify definitions dominate each use, each argument index
is bound, local/copy/constant forms have the claimed provenance, and every
selected path ends in the certified return. Reject cycles, invalid targets,
unrecognized prefixes, stores and additional dead operations. In particular,
do not erase an unused attribute read: it could raise. Initially reject even
unreachable extra executable words rather than inventing an unreachable-code
policy. Track aliases from slot reuse; physical slot equality does not establish
the same definition on both branch arms.

Generic LOAD_ATTR/COMPARE effects remain callback-capable. Their certificate
entries are conditional obligations, discharged only by the shared guarded
probe below. A broad no-write mask or `_PyCode_ReturnsOnlyNone` cannot discharge
them. `code.replace` must derive a fresh certificate from its own final words.

## Shared probes and the protected interval

Proposed internal signatures, with output parameters untouched on failure:

```
bool ir_probe_global(PyFunctionObject *, PyCodeObject *, uint32_t word,
                     PyValueBorrow *out);
bool ir_probe_attribute(PyCodeObject *, uint32_t word,
                        PyValueBorrow receiver, PyValueBorrow *out);
bool ir_probe_compare(uint16_t mask, PyValueBorrow left,
                      PyValueBorrow right, bool *out);
```

These functions perform no allocation, specialization, callbacks, decrements,
event emission or exception creation. MISS preserves every caller input and
the prior exception state. They may use the current executable feedback only
after checking its live handler and typed proof kind. A word whose entry has
been reset, switched to a polymorphic layout, wrapped or deoptimized is a MISS
unless the shared probe explicitly supports that exact representation.

Factor from the guards and raw loads of `_LOAD_GLOBAL_MODULE`,
`_GUARD_GLOBALS_VERSION` plus `_LOAD_GLOBAL_BUILTINS`, `_GUARD_TYPE_VERSION` plus
`_GUARD_SHAPE`/`_LOAD_ATTR_SHAPE`, and the reviewed class-scalar reader.
Ordinary generated handlers call the same primitives and retain their own
acquisition/publication/retirement. Do not call their complete handlers.
Instance-value/slot readers can follow only when their layout/provider proof
is equally shared; shape plus class-scalar coverage is a reasonable first bound.

Globals use the captured function's actual `func_globals`/`func_builtins`.
Validate dictionary/key version/index and fetch the current value. A key version
does not freeze values, and same-code functions do not share environments.
Attribute probes must prove descriptor/getattribute precedence before raw access;
a shape match by itself is insufficient. Class probes validate the current
class/metaclass/MRO proof before loading its guarded canonical word. Do not
infer provider identity from the immutable syntax certificate.

For the first comparison probe accept pairs of inline integers and pairs of
inline floats, using the existing `_COMPARE_OP_INT` scaled comparison and
`_COMPARE_OP_FLOAT` COMPARISON_BIT rules. Share those computations with the
ordinary operations. Reject mixed numeric pairs, bools, heap integers, strings,
subclasses and rich comparison initially. In particular bool is not an inline
integer merely because Python treats it numerically. Preserve NaN and fused
branch sense. Such misses execute the normal comparison once.

The protected interval starts after all cold preparation/allocations and the
final current-target/admission checks. It ends after canonical result duplication
and before the first retirement release. The GIL remains held throughout;
free-threaded and Emscripten builds decline initially. Function/window roots
anchor code and argument paths; the function anchors globals/builtins, which
anchor global paths. Intermediate objects remain reachable through these owner
chains because there is no mutation or callback. No intermediate owner array
is needed. Result duplication uses the canonical-home rule represented by
`IR_CANONICAL_HOME_DUP`, not a fallible object bridge. All code, path, environment
and feedback borrows must be dead before the first release.

## Concrete conservative admission

Implement one `ir_leaf_entry_is_inert(tstate, caller, function, code)` and a
monitoring-owned helper for its observer checks. It should return false without
processing pending work. The first supported configuration requires:

1. GIL build, thread-owned caller, unchanged caller current frame, no pending
   caller redirect; exact captured Python function and normal object/value
   provider. Require a valid cached function version and recheck the selected
   current code, effective self/argument count and executable/certificate.
   `PyFunction_SetVectorcall` clears versions and changes both provider fields;
   check the normal provider directly as well, rather than extending a proof by
   assumption. No bound-wrapper preparation, keywords or missing defaults.
2. Existing stack-space and `py_recursion_remaining > 1` guards. Do not decrement
   recursion because no activation executes, but preserve the admission boundary.
3. No PEP 523 hook, C trace/profile callbacks or tracing recursion. Every entry
   of `interp->monitors.tools` is zero; selected code `_co_monitoring == NULL`.
   This conservative check excludes local-only events and source wrappers and
   may decline permanently after tools were disabled. That coverage loss is
   acceptable initially and should be counted. Check caller instrumentation
   epoch/current handler as required by the uninstrumented CALL producer.
4. Live entry word is plain RESUME and the full atomic `eval_breaker` equals
   the selected code's instrumentation version. This excludes pending signals,
   pending calls, async exceptions, GIL-drop requests and stale instrumentation;
   do not mask away event bits. Recheck this immediately before acquisition/
   commit. If it changed, MISS; never handle an event and reuse old borrows.
5. No enabled DTrace entry **or return** probes. For a strict guarantee against
   externally toggled DTrace probes, initially decline WITH_DTRACE builds;
   checking ENABLED once is not synchronization with external toggling.
   In debug builds, decline if `PYTHON_LLTRACE` is present or globals are not an
   exact unicode-key dictionary with a proven callback-free absence of
   `__lltrace__`. Factor the LLTrace eligibility rule beside its current reader;
   do not run generic dictionary lookup in the protected interval. Until that
   absence helper exists, debug builds conservatively decline elision.

The atomic recheck is a linearization boundary, not immunity to a signal arriving
one instruction later. Newly pending work then waits for the next ordinary poll,
as it can after a successful RESUME skip. The bounded probe interval must contain
no polls or GIL release. Python monitoring cannot change concurrently under this
supported GIL configuration. Callbacks during subsequent retirement can enable
hooks, which apply after this already-completed call; they cannot replay it.

Remote sampling can see the caller instead of an elided pure callee. Decide and
document whether that is the supported optimized-stack contract. If exact remote
logical Python stacks are required, this configuration needs reconstruction or
must decline; no existing per-code observer flag was found that solves it.
Synchronous frame access cannot originate inside the protected interval, because
there is no Python/native callback. Do not present that argument as proof about
asynchronous external stack readers.

## Shared ordinary completion, not a second call ABI

Keep a shared ordinary publication/continuation tail and a leaf-specific input
retirement wrapper. Proposed interfaces:

```
_PyIRStepResult ir_complete_ordinary_return(tstate, caller, owned_result);
_PyIRStepResult ir_try_complete_leaf_call(tstate, caller, site, base,
                                        destination, argc, checked_version);
```

The latter returns MISS with untouched homes before commit, DONE with the actual
next pointer, or ERROR after draining all committed owners. Use named statuses
distinct from SEND/FOR outcomes even if the two-word `{next, status}` layout is
reused. It never returns a fictitious child. The generator consumes DONE/ERROR
as terminal paths: no output flush, delayed `window[0]` move or generic fallback.
MISS continues into the existing exact-frame initializer, not a repeated lookup.

Concretely, add one generator-recognized `IR_TRY_LEAF_CALL()` marker at the
start of `_INIT_CALL_PY_EXACT_ARGS`, enabled only for the plain exact-call
macro. Its emitter calls the out-of-line helper: MISS falls through to the
existing initializer; DONE/ERROR dispatch immediately with refreshed slots and
returned next. The hit edge must bypass both `_SAVE_RETURN_SLOT` and
`_PUSH_FRAME`. Extend positive generator validation, following
`validate_guarded_python_call`, to require the exact PEP523/version/arity/stack/
recursion prefix, unchanged physical window, destination and no earlier effect.
The bound-method macro sharing this initializer must not acquire eligibility
merely through textual reuse. Validate branch-local terminal ownership; do not
globally mark the still-live MISS inputs consumed or invent `new_frame` on DONE.

The leaf wrapper keeps fixed `PyValue retired[8]` storage in its out-of-line C
frame, not in the musttail handler. After result acquisition, detach/NULL the
entire consumed temporary window without callbacks, set the real return slot
and next PC, and retire the detached owners in a documented order. Caller locals
are snapshots' sources, not consumed homes. Hold the result through retirement
for LOCAL, temporary, overlap and discard alike. No hidden periodic-at-end check.
After retirement, invoke the shared tail used by the ordinary RETURN branch.
No selected callee/code/cache view survives into either helper's first release.

The shared tail owns the canonical result on every exit. It finishes a pending
ignored-result redirect only after overlap retirement, publishes/replaces the
current destination once, then releases the current displaced owner or discard
result. It rereads current frame, executable identity, PC and
`frame_obj->f_lineno_generation` after callback-capable cleanup; comparing only
pending_redirect or physical PC is insufficient. A callback may create the frame
object, and a same-PC redirect still changes generation. Do not restore the
captured destination or cached next after such a callback.

Before result publication, a failed refresh closes the held result with the
pending exception preserved and leaves the callback-current LOCAL unchanged.
After publication, the destination owns the result; errors must not close it
again. For a temporary overlapping the input window, detachment precedes the
store. Error branches consume all detached owners and dispatch the caller's real
error path without invoking the body again. Retained caller frames remain valid;
do not use `frame_obj == NULL` as the observation proof.

**Unresolved completion blocker:** deciding the caller's public PC during input
retirement requires a positive owner-phase proof. Ordinary child teardown leaves
the caller at CALL, with return_pc saved. Simply setting `instr_ptr = next`
before leaf retirement can expose a continuation whose folded LOCAL result has
not yet been published; the debugger's existing pending-child proof also has no
real child here. Conversely, leaving it at CALL must not let a callback replay
already-consumed inputs. Audit `Objects/frameobject.c` stopped-state admission
and generated CALL owner facts, then certify the completed-call/held-result phase
or explicitly refuse redirects in that phase. A generic refresh helper cannot
substitute for this proof. Share the selected contract with ordinary return;
do not silently add a second interpretation of CALL completion.

## First bounded implementation and witnesses

First extract/test the ordinary completion tail and prove the producer's
terminal ownership/redirect contract. Then add the complete certificate plus
the single structured evaluator in one bounded experiment, with shared guarded
shape/global/class readers and inline scalar comparison. Constants/arguments
are certificate witnesses in that same implementation, not separate performance
projects. Keep dynamic metadata and rejection counters small and report actual
hits/misses; no speedup follows from the semantic-call ceiling.

Required witnesses, in addition to existing ordinary-return coverage:

| Area | Concrete distinguishing witness |
| --- | --- |
| Whole body | A side effect or unused potentially raising read before an otherwise eligible return rejects; a malformed/extra word or exception region rejects. Copies and reused physical slots must preserve def-use. |
| Branch semantics | Both selector directions; NaN comparison; missing field or descriptor on the unselected arm must not execute. A selected missing field falls back and attributes the error to the real callee. |
| Mutable environment | Two `FunctionType` instances sharing code with different globals; overwrite an existing global key without changing keys; replace a class scalar with a descriptor; change receiver shape, getter provider or metaclass. |
| Provider/binding | Replace `__code__`, custom vectorcall, wrong arity, unsupplied default and keyword call. Each miss executes the ordinary body once. Split self succeeds under the exact contract; wrapped method falls back. |
| Comparison | Substitute an object whose `__eq__` records calls; verify one callback and normal error attribution after MISS. Include bool, large integer and mixed numeric misses. |
| Observation | PY_RETURN-only, LINE-only, BRANCH-only and local-only monitoring with plain RESUME; trace/profile installed between calls; stale epoch and pending eval work; LLTrace selected globals/env; supported DTrace build policy. |
| Ownership | Mortal result reachable only through receiver/constant/global; inline results; aliases among arguments/result; input-overlap, temporary, LOCAL and discard recipients; recursion boundary and stack-space miss. |
| Reentry | Last input/function owner callback reads retained caller locals and changes a LOCAL; displaced destination/discard finalizer installs hooks or requests an admitted redirect, including same-PC generation changes. Assert coherent homes and no body replay. |
| Error cleanup | Force post-retirement revalidation failure through an internal witness; held result drains once, callback-current LOCAL remains intact before publication, published result stays owned after it. |

Use a test-visible admission counter or existing stats mechanism to distinguish
a correct fast hit from a silently passing fallback. Negative tests alone cannot
qualify the optimizer. No builds/tests were run for this design review.

Stop if the entry proof requires general stack reconstruction, completion needs
a general transaction framework, or the evaluator grows into opcode dispatch.
The immediate decision is whether this bounded complete contract can be made
small and measured; the class-scalar/classmethod improvements remain independent.
