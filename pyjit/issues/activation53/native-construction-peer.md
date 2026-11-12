# Native-only construction: independent proposal review

**Accepted for the bounded implementation and isolated validation described in
[the proposal](native-construction-proposal.md).** No performance approval is
granted. This review read the actual proposal, the prior A source/native review
and the selected native-provider/shell interfaces. It made no runtime edits,
executions, builds or benchmark runs during the coordinator's quiet campaign.

## Architectural scope

The distinction is meaningful. Exact IR calls already transfer argument homes
and enter handlers directly; replacing those paths introduced a C boundary and
extra shared admission work. The native function-value provider instead holds
a private owning argument array and enters the C evaluator solely to produce
a generator object. A final-home constructor there can remove an actual
evaluator activation. The public object-vector API reaches that same provider
through its existing thunk. Selecting by provider/argument/entry contract for
both APIs is not selecting by benchmark identity.

Implement from golden51, retaining its IR CALL/SEND/RETURN and generated products
exactly. Do not carry full A's marker, effect metadata, common exact-call flag
branch, known-plan admission/finished-result tests, caller finisher, public code
shape helper or feedback-plan parameter. The native constructor has no caller
PC or result-recipient authority, so it also does not inherit A's introduced
LINE predecessor defect. Existing native call completion publishes results.

The 100000 Tree native hits and initial generator improvement support the
bounded experiment, not attribution of that improvement to this component.
The 242784 Fibonacci child hits in full A were IR and will be absent here.
Do not predict a native-constructor coroutine-kernel gain without corresponding
native hits. Coroutine breadth remains a regression gate.

## Shared interface and ownership

`_PyGen_TryMakeFromFunction(tstate, function_home, code, args, nargs)` is a clear
owning-array interface. Its explicit selected code is pinned before allocation;
function and supplied arguments remain in their original owning homes until
all fallible admission/allocation succeeds. MISS and ERROR must leave those
homes unchanged. SUCCESS moves them and NULLs their source homes exactly once,
returning one canonical owner. No pointer tagging, mixed frame/object result,
binding-plan union, kwargs/default parser or continuation descriptor is needed.

Keep the helper's private immutable first-word/recipient/successor check and
live executable/provider/observer admission separate in meaning. Generative
flags alone cannot bypass prefixes. Exact supplied count can include functions
whose positional parameters have defaults when every parameter is supplied;
omitted defaults, keyword-only parameters, variadics, keyword names and explicit
locals fall back. Async generators remain excluded.

The reviewed GIL allocation proof carries over: no polling or callback-capable
release before seal; all physical heap homes NULL; one shared initialization
with independently owned code/function; exact arguments moved; previous=NULL,
generator ownership/CREATED and real successor/input recipient installed.
The explicit-code shell allocator can remain static in genobject.c and serve
existing `_Py_MakeCoro` callers without adding an exported allocator API. Its
CLEARED state, executable/names/exception fields and coroutine origin must be
valid before any cleanup/traversal is possible. After the first owner move,
there is no failure before seal. Releasing the extra code pin is safe only
because the frame already owns that code.

The one provider retains a function owner, reuses unchanged arrays on MISS,
retires intact arrays/function once on ERROR with the original exception
preserved, frees only scratch on SUCCESS and returns through its normal value
API (and existing object thunk when called there).
Near C/Python recursion or stack limits it declines to existing evaluator
behavior. Do not drop capacity checks merely because the direct arm needs no
temporary thread frame.

Moving the implementation requires explicit header dependencies for its value
status return and debug dict lookup. Use the canonical PyDict_ContainsValue
API and the existing Unicode-key-table admission proof for LLTrace; do not
reintroduce the debug-only compatibility-call build failure. Keep conservative
monitoring/origin/provider, free-threaded, DTrace and LLTrace declines from the
reviewed proof. Guard duplication, helper ABI cost and native common branches
remain real costs for native inspection and measurement.

## Required evidence before admission

The updated actual proposal is reviewed: only genobject.c, call.c and
pycore_genobject.h plus focused native tests change. Python/ceval.c also stays
exactly golden51. Its internal `_PyEval_Vector` callers supply explicit locals
and would decline the proposed adapter, so omitting that unused eligible arm
is the correct minimal scope. `Objects/call.c:481` shows the public object thunk;
the legacy EvalCode/build_class routes remain outside direct construction.
Do not add a test-only C API to justify a dormant adapter.

Root should inspect the implementation diff against those boundaries, with no
generated products or IR header/effect/specializer changes. Compile release and debug;
then independently inspect native success/MISS/error edges and ordinary native
common-path work, rather than inferring savings from helper placement.

Use fresh processes for direct-path tests before persistent monitoring-version
changes. Demonstrate successful generator and coroutine construction through
the public object-vector and canonical-value API routes after executable warmup; attempts and
fallback semantic passes alone are insufficient. Exercise owning mortal arrays,
frame escape/GC, initial send/close/throw, unawaited warnings, origin/hook/LLTrace
declines, actual creation prefixes, changed flags without RETURN_GENERATOR and
forced allocation failure. Retain existing ordinary call/protocol/observer
regressions. Full A's tests/hits are historical evidence, not passes for this
new candidate. Both APIs share one value-provider body; do not describe those
tests as hits in the omitted internal `_PyEval_Vector` adapter.

Measure native-only A separately against immutable golden51. Confirm material
gains/losses and breadth before considering integration. This acceptance does
not reopen full A's IR helper strategy or establish a throughput result.
