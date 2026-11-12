# Bounded rescope: direct construction at native owning boundaries

Reviewed proposal, later implemented and rejected; see the [final result](native-construction-result.md).

Proposed against a fresh golden51-derived
`BASE/worktrees/activation53-native-construction`; root owns creation/configuration.
Full A's initial two pairs improve generators but lose on coroutines and logging;
it is not admitted. This is one bounded architectural rescope, not another IR
guard/forwarding rescue queue. Root retains all rejected-A evidence and confirms
material losses separately.

## Why this boundary

Exact IR calls already overlap thread argument homes and enter the existing
evaluator through direct handler transfers. Full A replaced their short inline
construction route with a normal C helper, repeated admission checks and a new
result terminal. Its 242,784 coroutine IR constructor hits establish execution,
not benefit: two pairs still lose about 3.3%. Native providers instead assemble
an owning argument array, push a temporary frame and enter a C evaluator solely
to execute RETURN_GENERATOR. Their direct constructor removes that actual
creation-only evaluator activation. The observed 100,000 native generator hits
and generator improvement justify testing this boundary on its own; the paired
ratios do not establish which subchange caused either result.

The coordinator's six-pair confirmation retains that decision: full A takes
0.929631 times golden51 generator time, 1.027712 times coroutine time and
1.244130 times logging time. A separate stable matched instruction diagnostic
finds coroutine instructions at 0.985784 and logging at 1.008263. Fewer
instructions do not establish throughput improvement; duplicate guards are not
a proven cause of the losses. Native-only construction is a different bounded
work-removal hypothesis, not an established causal repair.

The public object-vector thunk and canonical-value vector API share the same
owning storage boundary. No benchmark names, source locations or persistent chain state select
the optimization. IR CALL/SEND/RETURN, specialization, metadata, continuations,
binding plans and the generic frame-returning API remain exactly golden51.

## Shared constructor

Implement `_PyGen_TryMakeFromFunction(tstate, function_home, code, args, nargs)`
in `Objects/genobject.c`, declared in `pycore_genobject.h`. It accepts an explicit
selected executing code and exact positional description. There is no feedback
plan argument, kwargs/default binder or IR caller. It returns `_PyStatusValue`:
MISS=0 and ERROR=-1 leave every input home unchanged; SUCCESS=1 NULLs the moved
function/argument homes and returns one canonical generator/coroutine owner.
It publishes no caller recipient, instruction cursor or continuation.

Admission requires exact argument count, no variadics or keyword-only parameters,
optimized code, standard Python provider, existing executable and physical word
zero itself RETURN_GENERATOR with its constructor-validated IR_NO_DST recipient
and a real physical successor. Check pristine and current executing first words.
Async generators and every creation prefix decline. Do not skip MAKE_CELL,
COPY_FREE_VARS, GET_ITER/GET_AITER, RESUME or wrappers. Code flags alone do not
authorize bypass. The local immutable-shape check lives in this constructor;
no code, executable or frame metadata is added.

Keep the current conservative observer/origin admission from reviewed A:
decline PEP523, trace/profile hooks, code monitoring, nonzero global monitoring
version, origin tracking, enabled DTrace and LLTrace. Debug LLTrace lookup must
remain nonescaping through a unicode-key globals dictionary or decline. Decline
free-threaded builds. Retain Python recursion remaining and thread capacity
admission; the native adapter declines near C recursion limits to the original
provider/evaluator. These checks are needed at a native boundary where no IR
function/version/recursion prefix has established those facts. No poll occurs
inside the admitted allocation/commit segment; GIL GC allocation schedules GC
and does not synchronously call Python.

Pin code before allocating and pass it explicitly to the common static shell
allocator's type/slot sizing. Refactor that allocator to accept code, updating
its existing callers without changing their behavior. The shell is CLEARED with
valid name/qualname, exception and executable fields; coroutine origin is NULL.
There is no general CONSTRUCTING state. Allocation failure occurs before moves
and releases only the temporary code pin while caller inputs remain owned.
Initialize every physical heap home NULL, use the shared frame initializer once,
then move function and exact arguments and NULL their source homes. No fallible
step or callback-capable release follows the first move before sealing.

Seal previous=NULL, owner=GENERATOR, state=CREATED, the actual word-one successor
and word-zero incoming recipient. The final frame separately owns function and
code; release the temporary code pin after that owner exists. The helper keeps
the object allocation/final NULL initialization, argument moves, code/function
owners and constant owners. It removes the temporary thread reservation/header,
whole-frame move, source scratch clears, transient current-frame and recursion
churn, RETURN_GENERATOR transition and native creation-only evaluator entry.

## Native boundary and remaining common cost

`_PyFunction_ValueVectorcall` in Objects/call.c tries the constructor after
building its existing canonical owned array. `_PyFunction_Vectorcall` already
converts public object vectors through its existing thunk into that same value
provider; no separate object adapter is necessary. Only no-keyword,
no-explicit-locals calls enter the constructor; other shapes retain the old
frame API. `_PyEval_Vector` and its legacy explicit-locals callers remain
unchanged because their existing in-tree calls cannot enter this bounded path. Keep one explicit function owner. On MISS pass it and unchanged args
to `_PyEvalFramePushAndInit`. On ERROR close those intact owners once with the
original exception preserved, free only scratch and propagate. On SUCCESS free
scratch and return the canonical owner, taking its owning object view only at
the object API boundary. No callback-selected IR PC protocol is introduced.

Remaining common costs are the code/flag/shape branch at the native provider,
the existing argument owner-array construction and function duplicate. Additional
eligible-arm guards and the temporary code pin are real costs. Moving the helper
to genobject.c removes full A's new common IR code and known-plan terminal tests;
it does not guarantee that native-only code layout or ordinary native-call
branching is free. Native inspection and unchanged-workload timing decide.

## Files and validation

Handwritten runtime files: `Objects/genobject.c`, `Objects/call.c` and
`Include/internal/pycore_genobject.h`. Add focused native constructor tests.
Python/ceval.c, bytecodes, generators, specializer, feedback, IR headers and all
generated products stay exactly golden51. Reuse the corrected A shell/home
implementation, removing every binding and caller-publication component.

Root owns builds, runtime checks, hit probes and measurement. Each native path
claimed as covered needs successful constructor evidence after warming its
executable, including coroutines as well as generators; a helper attempt or
semantic fallback pass is insufficient. In this selected runtime
`_PyFunction_Vectorcall` is an object-to-value thunk, so both map and ordinary
`pyobject_vectorcall` tests reach `_PyFunction_ValueVectorcall`. Current direct
in-tree `_PyEval_Vector` callers pass explicit locals and cannot use this path.
The coordinator therefore omits an `_PyEval_Vector` adapter and any artificial
internal API used only to reach that dead branch. Legacy EvalCode calls with
explicit locals retain their existing behavior and receive no new hit claim.
Use fresh test processes before hook/origin/LLTrace cases because
the conservative global monitoring-version guard persists after tool removal.
Test the native value provider and its public object-vector thunk, exact mortal arguments, GC/frame escape,
pre-first-send close/throw and initial non-None send, coroutine warnings/origin,
prefix and changed-flag fallback, hook installs and allocation failure. Existing
ordinary-call/observer suites remain regressions, not proof of new native hits.

Inspect actual native provider branches and verify creation hits omit frame push,
frame copy and C evaluator entry. Confirm golden51 IR handler source/generated
products stay unchanged. Screen against actual golden51, confirm material gains
and losses, and require breadth/direct upstream evidence before promotion.
