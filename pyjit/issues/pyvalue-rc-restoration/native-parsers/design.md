# Clinic parser owners and complete call preparation: v3

Accepted design; current owners and active scope are in [implementation grants](implementation-grants.md).
V1 design.md and v2 design-v2.md remain preserved. This v3 is the complete
selected proposal, incorporating both required changes in independent-review-v2.md.
It retains the accepted two ownership roles, shared engine and production family;
it closes early preparation and replaces the unported FASTCALL witness route.

## Selected lifetime model and proposed contract amendment

Publish a coherent IN_CALL state before any native preparation or callout. Its
logical PC identifies the call, every evaluated input home owns a valid canonical
value (or is explicitly absent), the output has not replaced a live input, and
the frame/code/argument window stays alive. Snapshot the operation's numeric
current/next word offsets, window bounds and destination before allocating.
Do not retain decoded IR/feedback or movable array addresses across a callout.

There are two ownership roles within this ONE transaction:

| Owner | Retirement rule |
| --- | --- |
| Final selected invocation inputs, caller's displaced output, staged scalar result box | Retain through result/error publication; then drain and refresh |
| Callee-local parser boxes, selected keyword owners, buffers and native locals | Retain through their last callee use, including converter cleanup; may close while IN_CALL remains coherent |

This distinction is essential: ordinary native bodies can Py_DECREF their own
local objects, release buffers or call arbitrary Python before returning. Local
cleanup is another part of executing that body, not consumption of the caller's
argument homes. A parser lease may be the last owner of a heap value removed
from kwargs; its finalizer is allowed to reenter during IN_CALL, just like a
local object created and destroyed by the implementation. It need not wait for
a result that the native body has not returned yet.

Proposed amendment to extension-api.md: replace the blanket requirement to defer
ALL parser resources until caller result publication with the two ownership
roles above. Require coherent IN_CALL publication before ANY parser allocation,
boxing, converter, buffer-release or destructor callback. Keep the existing
strict final-invocation-input/result publication and subsequent epoch/frame reload rule.
Resources that escape a wrapper must instead have an explicit longer-lived
owner; no hidden current-scope/TLS or wrapper-stack pointer may escape. This is
one revised contract for every caller, not an optional legacy lifetime policy.

## Why IN_CALL is a valid reentry boundary

The current generated handlers set frame->instr_ptr at handler entry. Ordinary
call windows are promoted by _PyValue_OwnArray before native work. An executing
frame owns its code through f_executable; normal RC/GIL instrumentation changes
handler/feedback data, not the lifetime of the active frame. Current IR source
constructs _co_exec in _PyCode_GetExec and frees it only in code deallocation via
_PyCode_ClearExec; the active frame's code owner prevents that deallocation.
Callee-local reference counts pin objects independently of Python frame scans.
The wrapper can therefore execute multiple nested callbacks without decoding
any caller instruction or depending on a mutable kwargs entry it once read.

These facts are necessary but insufficient in the CURRENT callers. The generic
fallback calls native_vector_take before invocation, then lets
_Py_VectorCall_ValueSteal consume argument owners before publication. The EX
handlers close tuple/kwargs inputs before installing the returned value. Cold
and instrumented generic paths also use the old consuming helper. They do not
establish the full selected invariant. The first E2E increment must repair these
edges; the parser bridge will not knowingly rely on them.

After native return (including all wrapper-local cleanup), verify frame/code
identity and reacquire execution data from captured word offsets BEFORE touching
cached window/IR data or installing results. Stage result ownership without
release. Move final selected invocation inputs to caller-owned retirement slots, publish a valid
canonical result or error state and actual PC/live homes nonallocatingly, then
retire invocation owners. Refresh frame/epoch/words/feedback again after that
cleanup before decoding the next handler. No call may be replayed after its body
or a converter has executed. Whole-handler redispatch is permitted only before
any possible user effect; a NOT_INVOKED adapter decline after preparation can
choose another invoke adapter inside the same record, never replay the handler.

## Actual production route, including constructor dispatch

The selected production family uses tuple/kwargs constructor wrappers, NOT
registered Cfunction vectorcall bodies. For an ordinary call:

    cold CALL/_DO_CALL or warmed CALL_NON_PY_GENERAL
    -> revised caller-owned generic native transaction
    -> PyObject_Vectorcall(type, real owned object vector, kwnames)
    -> type's actual vectorcall OR _PyObject_MakeTpCall
    -> PyType_Type.tp_call / type_call
    -> type->tp_new(type, canonical argument tuple, kwargs)
    -> generated Clinic wrapper -> implementation
    -> wrapper-local converter cleanup, then argument lease cleanup [IN_CALL]
    -> optional type->tp_init, maintaining the same IN_CALL invariant
    -> returned owned object -> stage -> caller publication
    -> invocation input/displaced/result-box retirement -> refresh

If a builtin has its own constructor vectorcall, the precise registered entry
must be inventoried; do not assume type_call or a Clinic helper is always used.
The selected seven constructors have no dedicated constructor vectorcall in
current source; specialization chooses CALL_NON_PY_GENERAL. Keep existing
constructor/slot registration and exact C prototypes; do not function-pointer
cast a method to a new ABI. type_call itself may release native locals on an
init error; that is permitted by the same callee-local rule.

For expanded calls, both generic/instrumented _DO_CALL_FUNCTION_EX and warmed
_CALL_FUNCTION_EX_NON_PY_GENERAL must use the same transaction state machine:

    coherent IN_CALL frame + owning callable/tuple/kwargs homes
    -> caller-owned tuple-call scope -> PyObject_Call
    -> _PyObject_Call -> type_call -> tp_new/wrapper[/tp_init]
    -> local cleanup while IN_CALL -> return/stage
    -> result/error publication -> invocation cleanup -> refresh

The scope retains the actual tuple and kwargs container owners; no second boxed
argument tuple or snapshot of every mutable keyword is required for frame
validity. A wrapper independently owns each selected keyword result. Removing
an entry may end its container ownership but cannot invalidate the wrapper's
selected output, and its eventual local finalizer sees coherent IN_CALL state.

A Cfunction witness follows cfunction_call -> registered generated wrapper
instead of type_call. It uses the same generic transaction and final publication;
it is not claimed to be the production constructor route. Direct C-API callers
must likewise keep their own borrowed inputs live and have a coherent surrounding
VM state before calling Python; they receive no implicit TLS lifetime engine.

## Reuse the accepted engine

Extend _PyNativeScope with object-call entry adapters, not a second engine:

- BeginObjectVector: accept a generic object callable; pin it/kwnames, capture
  independent canonical input snapshots, and produce an owning PyObject vector
  with checked storage and NO OFFSET prefix. Share capture/boxing internals with
  BeginVector while preserving its existing Cfunction-specific contract.
- BeginTupleCall: pin callable plus the canonical args tuple and kwargs object;
  these are invocation inputs, retained through publication. Do not reinterpret
  tuple storage as pointers or box all tuple contents into another tuple.
- InvokeObjectVector/InvokeTupleCall: use the proper existing object call API,
  consume its owned result through existing StageResult, then use the existing
  Publish/Abort/End transaction and exception-preserving cleanup.

Existing eligible Cfunction direct invocation remains an adapter into the SAME
scope/result machine. Unsupported flags or custom vectorcall targets use the
correct object-call adapter before invocation, never after a completed body.
Never loosen End's Publish/Abort requirement or introduce fake publication.

## Transaction entry includes all preparation

The generator creates ONE instruction-local call record at entry to every
relevant CALL/CALL_KW/EX handler, BEFORE the first potentially reentrant step.
After decoding immutable call shape into numeric fields, pure OwnArray promotion
and pure IN_CALL publication precede monitoring boxes, method retirement,
iterable conversion or any allocating scope Begin. The record contains captured
frame/code, current/next offsets, window base/count/destination, route kind,
preparation progress and pre-reserved displaced-preparation-owner slots. Its lifetime spans all
uops in the handler, rather than beginning inside the final invoke uop.

Named bytecode ops in scope:

- `_MAYBE_EXPAND_METHOD` and `_MAYBE_EXPAND_METHOD_KW`: publish new callable/self
  homes nonallocatingly while still retaining the displaced bound-method owner.
  Only after both FINAL callable/self homes are valid may the obsolete method
  wrapper retire as a preparation local. Refresh on success/error before reading
  decoded state. A retirement error stops preparation and publishes error; do not
  invoke. This applies to Python-inlined calls too; no C-only owner escapes.
- `_MONITOR_CALL` and `_MONITOR_CALL_KW`: capture is already active before
  `_PyValueTemps_Object` can box. Maintain an owned argument view through the
  CALL event. Refresh after boxing, the event and local-view cleanup on success
  AND error before using `this_instr`, `window`, argument views or decoded fields.
- `_MAKE_CALLARGS_A_TUPLE`: all four EX homes (callable/absence/iterable/kwargs)
  are independently owning before `_PyValueTemps_Object`, `_Py_Check_ArgsIterable`
  or `PySequence_Tuple`. Pin the original iterable across conversion. A successful
  tuple result is prepared, then moved to the actual callargs home without
  allocation while retaining the displaced original iterable owner. After FINAL
  tuple input publication, that obsolete iterable is a preparation local and may
  retire; refresh success/error before further decoding. A partial tuple
  internal to a failed conversion is a callee/converter local and may close while
  the original homes remain valid. Refresh every success/error callback edge.
- `_DO_CALL`, `_DO_CALL_KW`, `_CALL_NON_PY_GENERAL`, `_CALL_KW_NON_PY`,
  `_DO_CALL_FUNCTION_EX`, `_CALL_FUNCTION_EX_NON_PY_GENERAL`: consume that SAME
  entry record; do not recapture from stale pointers or restart preparation.
  Preserve Python-frame inlining as the existing transfer of FINAL owning inputs
  to the new frame. Finish and drain obsolete preparation locals BEFORE transfer,
  while IN_CALL is coherent. Check cleanup error before creating/entering the
  new frame. Once inputs transfer, no owner or pointer to a C-only prelude record
  remains; DISPATCH_INLINED keeps its current frame ownership protocol.
- CALL/CALL_KW/INSTRUMENTED_CALL/INSTRUMENTED_CALL_KW, CALL_FUNCTION_EX/
  CALL_EX_PY/CALL_EX_NON_PY_GENERAL/INSTRUMENTED_CALL_FUNCTION_EX macros carry
  this entry/finish behavior, including error exits and monitoring preludes.

The EX instrumented event in `_DO_CALL_FUNCTION_EX` remains AFTER iterable
conversion, matching current ordering. For ordinary instrumented calls, method
expansion still precedes CALL monitoring, and CALL monitoring precedes invocation.
C_RETURN/C_RAISE remain after the completed body. Do not move events or reexecute
`__iter__`/`__next__` simply to reuse a final-call helper. If specialization or
JIT preparation can call out, entry ownership/capture precedes it too. The admission target is the
JIT-disabled interpreter; no current runtime or JIT admission is claimed.

## Effects, refresh and retirement rules

Before the FIRST possibly effectful preparation operation, latch progress in
the call record. Conservatively count boxing/allocation hooks, monitoring,
iteration/conversion and cleanup as possible user effects. Pure specialization
can redispatch before that latch. After it, no whole-handler replay is allowed.
If instrumentation changes, reacquire execution data using captured offsets and
continue the already-started logical call at its recorded phase, using captured
operand indices and any completed tuple conversion. Validate compatible logical
call shape/bounds; if invalid, publish a coherent error and drain once. Never
retry conversion, CALL monitoring or invocation after its effects. A
NOT_INVOKED native-adapter decline may switch invoke adapters within this call
record using the prepared owners; it does not redispatch completed preparation.

A preparation error has the same real error-publication boundary as an invoke
error: the CURRENT raw/prepared owning homes remain held until the frame/PC/live
homes describe that error; only then drain those owners. Already retired obsolete
preparation locals are never replayed or resurrected.
No allocation is allowed to append a displaced owner during publication. Reserve
fixed preparation slots and variable invocation storage before their moves,
while current owning homes still retain the corresponding values.

Callee-local temporaries created by a converter (temporary object views, partial
iteration products, buffers, parser lookup owners) may retire after their last
use while the currently published IN_CALL homes remain coherent. In contrast,
the FINAL selected invocation inputs remain caller-owned through final result/
error publication. A displaced raw iterable/method wrapper becomes an obsolete
preparation local ONLY after the final replacement input homes are published
coherently while its owner is still retained. It may then close and reenter,
followed by refresh; if cleanup fails, do not invoke. This is the literal
amendment to v2's overly broad “original inputs” wording. It closes EX
close-before-coherent-input-publication without a new frame-carried owner list.

Refresh on EVERY potentially reentrant return and cleanup error edge before
using cached frame/IR/feedback/window data. During a callee's internal callbacks,
the C wrapper never dereferences caller decoded state and the published frame
remains valid. After return, the caller reacquires state; it then publishes
result/error, retires invocation owners, and refreshes again. Retain the first
primary error across all cleanup. No fake Publish callback, TLS current scope,
second invocation engine or alternative lifetime policy is introduced.

## Concrete shared interfaces and ownership

Proposed private C interfaces (names may be adjusted only within review):

```c
int _PyNativeScope_BeginObjectVector(
    _PyNativeScope *scope, PyThreadState *tstate, PyObject *callable,
    const PyValue *owning_args, Py_ssize_t positional_count,
    Py_ssize_t total_count, PyObject *kwnames);
int _PyNativeScope_BeginTupleCall(
    _PyNativeScope *scope, PyThreadState *tstate, PyObject *callable,
    PyObject *args_tuple, PyObject *kwargs);
int _PyNativeScope_InvokeObjectVector(_PyNativeScope *scope);
int _PyNativeScope_InvokeTupleCall(_PyNativeScope *scope);

int _PyArg_OwnerScope_BeginTuple(
    _PyArg_OwnerScope *scope, PyObject *args_tuple,
    Py_ssize_t keyword_owner_budget);
PyObject *const *_PyArg_OwnerScope_Objects(const _PyArg_OwnerScope *scope);
PyObject *const *_PyArg_UnpackKeywordsOwned(
    _PyArg_OwnerScope *scope, PyObject *kwargs,
    struct _PyArg_Parser *parser, int minpos, int maxpos,
    int minkw, int varpos, PyObject **argsbuf);
int _PyArg_OwnerScope_End(_PyArg_OwnerScope *scope);
```

BeginObjectVector borrows callable/kwnames and a stable already-owning argument
window until independent capture completes; it pins metadata and duplicates all
snapshots BEFORE any boxed result is exposed. BeginTupleCall borrows and pins
the callable, actual canonical args tuple and optional kwargs; it does not expose
an element array. Both return 0/-1 with initialized partial scopes on every exit.
Invoke uses only that scope's owned views, invokes at most once and stages the
owned result in `scope->result`, returning 0/-1. The caller still performs real
`_PyNativeResult_Publish/Abort` and `_PyNativeScope_End`; Begin/Invoke never consume
caller homes or publish caller result state themselves. Preserve the existing
BeginVector/CallScoped adapters and share capture internals without silently
widening their old eligibility contract. No writable vector prefix/OFFSET flag.

The owner-scope record is defined in pycore_modsupport.h, independent of the
invocation-scope record. Zero initialization and failed Begin are safe for one
End; all bookkeeping slots initialize before their counts become visible.
BeginTuple is deliberately CAPTURE-ONLY: validate/pin the immutable source tuple,
record positional count and keyword budget, and initialize cleanup state. It
performs no positional-array allocation, no element scan or boxing. Do not
materialize an arbitrarily long invalid tuple just to discover an arity error.

UnpackKeywordsOwned derives nargs from that pinned tuple and pins kwargs before
any parser callout. Preserve the old no-keyword fastguard and parser-init order,
then use the shared unpack algorithm's cheap count/shape checks and original
error priority BEFORE positional materialization. After those checks succeed,
allocate checked storage, capture tuple borrows into independent canonical
snapshots, and box them. For the fixed-one family, excess arguments fail before
reading/boxing any positional value; valid capture has at most one positional
slot. Do not duplicate a second divergent arity/keyword-error algorithm. Keyword
lookup/conversion and duplicate/default handling keep existing sequencing.

The scope now reaches MATERIALIZED state. Its object array is stable through End;
Objects returns a borrow only in MATERIALIZED state (assert that precondition).
Callers may not free/realloc or retain it beyond End. A valid empty view avoids
NULL arithmetic. Capacity/size overflow is checked, and keyword owner slots are
reserved before lookups so retaining GetItemRef owners cannot allocate. No raw
canonical array can accidentally enter an object-vector parser API. Selected
keyword owners transfer to the scope BEFORE borrows appear in argsbuf. Return
a borrow into scoped objects or caller argsbuf, or NULL with error; missing
optionals are NULL. Partial Begin/preflight/materialization paths all End safely.

Keyword metadata is scoped too. The first family uses dict kwargs and no kwnames;
FASTCALL keyword tails are a later distinct adapter, not an unimplemented promise
in this tuple-only signature. End drains all local parser owners after generated
converter cleanup, preserving entry or first cleanup error; returns 0 only with
no error, otherwise -1. It cannot end a native invocation scope or retire its
inputs/result. Begin pinning survives parser initialization and storage allocation;
no tuple-slot address crosses either callout.

There is NO cross-lane lifetime pointer parameter, registry/TLS lookup or wrapper
ABI change: the two scopes share ownership rules, not storage. Callee locals
close inside the caller's already-published IN_CALL state. Lane A owns the native
header/functions; lane B owns modsupport/getargs/Clinic. Root merges those exact
interfaces before admission checks.

## Exact native producer functions

`Tools/cases_generator/ir_generator.py:generate_handler` owns entry-record
creation and pure owning-window publication. Extend its current CALL/CALL_KW
family normalization/promotion to CALL_FUNCTION_EX and its instrumented/specialized
EX members. Use `external_io(inst)` for the complete external input span, not a
hard-coded subset; assert EX's four inputs and absence sentinel handling.
Generate entry capture before the existing JIT preparation block and before uop
emission. Include EX/instrumented handlers in the mutable `this_instr` treatment
needed for refresh; do not leave their old const/stale pointers.

`IREmitter.finish_value_operation` and `IREmitter.reader_reload` own refreshed
`this_instr`, `ir_word`, slots and window reacquisition. `write_uop` and
`IREmitter.error_no_pop`/`goto_error` own success/error uop transitions and must
not flush stale locals over newly published homes. Add narrowly named call-entry,
call-prelude-refresh and call-abort emission helpers if needed; enumerate their
new DSL markers explicitly. Reuse the existing error-phase discipline for
shared JIT helper products rather than subtracting old instruction pointers.
`IREmitter.ir_window_taken` retains its explicit input-consumption responsibility;
prelude publication must not mark the whole window consumed before invocation.

`Python/ceval.c` owns `value_operation_begin`/`value_operation_refresh` and the
`native_vector_capture/prepare/take/retire` coordination plus new bounded call
preparation helpers. Factor the common call record once so monitors/EX and final
invocation cannot diverge. Existing direct-native engine publication semantics
remain; old generic consuming helpers must no longer be used by these selected
ordinary/EX routes. No unrelated binary-op/provider hunks are included.

Regenerate native products from their producers together. At minimum review
`Python/generated_ir_cases.c.h` and `Python/generated_jit_helpers.c.h`; any changed
opcode metadata/targets/labels are included only if actual generation changes
them. Preserve all producer output differences, not hand-edited fragments.

## Parser lease and selected production family

Use an explicit wrapper-owned _PyArg_OwnerScope plus an owned keyword-unpack
entry point as in the original ownership analysis. Initialize before any goto;
pin tuple/count without materializing; preserve shared cheap preflight/error
priority; then reserve cleanup slots, capture independent heap-safe snapshots
and box into initialized owning object slots. O
and O! outputs borrow those slots through implementation and generated cleanup.
A selected PyDict_GetItemRef result transfers into a reserved keyword owner slot
BEFORE argsbuf exposes its borrow. Check cheap count/shape errors before materializing invalid argument windows;
zero arguments must use a valid empty view without NULL arithmetic.
Original object-vector APIs remain explicitly
object-taking; they do not silently accept canonical arrays. Primitive converters
can later bypass boxing without changing the lifetime model.

Drain generated buffer/converter resources before the lease. Keep any successful
implementation result owned through both cleanups. Preserve a primary exception,
retain a new cleanup error, and never return a non-NULL object with an exception
set. On failure retire that local result under the primary error; this is legal
callee cleanup in IN_CALL. Integer init results become -1 if cleanup fails.
All success/error/partial-initialization paths converge on the generated common
exit. No output borrow or pointer to scope storage escapes.

First structural rule: non-limited, non-FASTCALL, non-vectorcall-helper tuple/kwargs
constructors with exactly one required positional-or-keyword object parameter,
no optionals, keyword-only parameters, aliases, varargs or recursive formats.
Include matching one-object init wrappers so an actual constructor does not
immediately enter an unported paired parser. Current source census (inputs-v2.json: nine wrappers in seven products) finds:

| Input producer | Existing generated wrappers |
| --- | --- |
| Objects/memoryobject.c | memoryview |
| Objects/descrobject.c | mappingproxy_new |
| Objects/typevarobject.c | paramspecargs_new, paramspeckwargs_new |
| Modules/posixmodule.c | os_sched_param |
| Modules/_zoneinfo.c | zoneinfo_ZoneInfo |
| Modules/_struct.c | Struct and paired Struct___init__ |
| Modules/_remote_debugging/module.c | one-object BinaryReader.__init__ |

The last init is a structural sibling, not a named special case. Confirm this
census by producer metadata before editing and include ALL products it selects.
These wrappers contribute actual production macro failures. The four testclinic
array wrappers from v1 become a later structural-family increment; do not add
their converter support merely to make this production slice look broader.

Struct has mandatory handwritten closure: tp_new is s_new, whose normal
single-positional route currently Py_NewRef(PyTuple_GET_ITEM(...)). Port that
read to an owned GetItemRef boundary; preserve its subclass delegate/warnings
and release the owner on every existing exit. In particular, the current
`tp_alloc == NULL` failure return after format acquisition leaks format: close
that owner under the allocation exception. Audit all set_format/warning exits
for the same owned cleanup and preserve their existing primary errors. tp_init s_init may delegate to
Struct___init__, hence the paired generated init is in scope. No full typeobject
rewrite is needed: type_call's actual dispatch is inventoried, and the ordinary
constructor path does not enter its unrelated type(x) tuple getter branch.

## Scope and useful implementation layers

The first E2E slice comprises these coordinated layers under one accepted design:

1. Amend extension-api.md's lifetime contract and document IN_CALL, local vs
   invocation ownership, replay prohibition and refresh requirements.
2. Add generic vector and tuple-call scope adapters and call transaction
   ownership/publication to cold/warm/EX/instrumented non-Python callers.
3. Add the parser lease/scoped unpack provider and the production Clinic family
   above, preserving kwargs owners through local cleanup.
4. Add real registered constructor fixtures and actual caller/publication
   witnesses; independently review code/results and commit the coherent slice.

Useful dependency commits within the slice are allowed, but layer 3 must not
land with an intentionally invalid caller lifetime or await a different future
policy. Merge validation binds all layers. An unlinked source checkpoint is
still not runtime qualification.

Expected shared files: Include/internal/pycore_native_scope.h,
Python/native_scope.c, Python/ceval.c, Python/bytecodes.c, the native caller
producer helpers in Tools/cases_generator/ir_generator.py only if needed, and
regenerated generated_ir_cases.c.h/generated_jit_helpers.c.h/metadata products
actually changed by those producers. Caller design review must enumerate exact
functions before edits. No whole Objects/typeobject.c staging: it has separately
held unrelated scalar/cache changes.

Parser files: Include/internal/pycore_modsupport.h, Python/getargs.c,
Tools/clinic/libclinic/parse_args.py (structural selection, owner setup/exit and
argument expressions), Modules/_struct.c (s_new boundary), all seven Clinic
products corresponding to the production source inputs above, plus normal
Clinic input checksum changes. converters.py is not needed for a fixed O-only
starter; its varpos array migration is deferred. Reachable keyword metadata
must use proven Unicode views; unexpected-key errors need owned keys/a canonical
key-list snapshot across callbacks. Leave the other four tuple parser families
and recursive scope propagation visibly unfinished rather than fake a TU pass.

Witness files: Modules/_testclinic.c and its generated header, Lib/test/test_clinic.py
and a focused native lifetime test, plus the existing native_scope fixture only
for actual caller publication/cleanup observation if its current hooks suffice.
Use typed new/init fixtures matching the production family and normal slot
registration; avoid direct calls to static _impl workers. Review any needed
fixture hook separately and keep its event observations independent of the
implementation being tested. Generator fixture products follow their inputs.

## Evidence that can admit this slice

Cheap gates first: exact named source scope; host producer tests; selected Clinic
regeneration twice with identical second output; native caller producer and ALL
changed generated products regenerated together; AST and whitespace checks.
Use the NEW diagnostic build's configured debug pyconfig and real W6 headers,
not the old interim config. Focused actual TU failures retain unrelated named
old consumers; none may be reported as a clean TU pass. No cast/fake declarations.

Candidate tests must use ordinary constructor calls (cold and warmed), positional,
keyword and *args/**kwargs EX forms, instrumented calls and C-API routes to the
REGISTERED wrapper. Cover memoryview's actual buffer-exporter callbacks,
ParamSpecArgs/Kwargs scalar/object origin retention, mappingproxy argument errors,
os.sched_param canonical scalar field, Struct direct/subclass paths, and applicable
ZoneInfo/BinaryReader construction without requiring unrelated environment data.

A same-family registered constructor fixture receives one context object and
calls its callback. The callback changes monitoring and inspects the suspended
caller frame. Another REGISTERED constructor fixture's native body owns a target
constructor and the actual kwargs dict, then calls
`PyObject_VectorcallDict(target, NULL, 0, same_kwargs)` from within the already
coherent ordinary/EX IN_CALL transaction. The target is the real registered
one-object tp_new wrapper, not a static `_impl` call. Its callback closes over
that SAME dict and removes the selected heap object after parsing. Observe the
local object's finalizer and subsequent actual caller publication/retirement.
This replaces the `_testcapi.pyobject_fastcalldict` witness entirely: no new
CALL_BUILTIN_FAST or `_Py_BuiltinCallFast_Value` dependency is admitted.

Drive the OUTER fixture constructor with ordinary positional/keyword calls,
cold and warmed, and EX forms. Test nested callback/error paths. Both the outer
constructor and registered inner wrapper obey the same IN_CALL invariant;
`PyObject_VectorcallDict(..., NULL, 0, kwargs)` forwards the dict to MakeTpCall /
type_call without a duplicate argument tuple or a separate mutable kwargs copy.

Required observations distinguish two times: parser-local finalization sees the
coherent call-site PC/owning input state with no premature result publication;
invocation-owner cleanup sees actual canonical result/error publication. A
narrow `_testinternalcapi.native_scope_frame_window` METH_O fixture may inspect
the passed suspended frame's current call word and bounded canonical window.
It validates code/format/bounds, copies up to the selected call window's bounded
observation fields into independent owners BEFORE any allocation, then reports
them. It neither mutates a caller frame nor trusts a producer-maintained test
"published" flag. Use only real owned getters/canonical snapshots; no old
VARARGS parser and no TLS owner context. This tests the actual registered route
and actual home contents, not only the synthetic scope's event list.

Prelude witnesses are mandatory: CALL monitoring that changes instrumentation
and raises; a non-tuple EX iterable with observable `__iter__`/`__next__`, nested
callbacks, allocation churn and a marker exception; successful conversion that
changes monitoring before invoke; and method expansion/free-hook reentry where
the existing allocator harness can provide a real route. Assert exact conversion/
monitor/invocation counts, preserved event order and exact exception identity.
Observe displaced original iterable destruction only after coherent FINAL input
publication; partial conversion-local destruction may occur earlier while raw
input homes remain coherent. Final invocation owners retire only after actual
result/error publication. Verify both boundaries separately.
All success/error paths reload the caller before the next instruction.

Allocation-failure and cleanup-error cases preserve the exact primary exception
and exactly one result owner. Use existing faults where possible; unexercised
internal branches remain source reasoning. No test may replay a conversion or
monitor callback and then treat the repeated final value as sufficient evidence.

Host interpreter results are reference controls only. Candidate tests remain
unexecuted until a coherent candidate links. If a real invariant witness fails,
fix the selected transaction; do not switch back to a wrapper-exit-only policy.
After the coordinator's merged validation, migrate further production tuple/kwargs
families, varpos arrays and recursive parsers on this SAME lifetime contract, then
replace primitive boxing with value-native conversion and extend opaque APIs.

## Reviewed implementation boundaries

| Lane | Exclusive files/functions | Dependency and review boundary |
| --- | --- | --- |
| A: caller transaction and engine | pycore_native_scope.h; native_scope.c object adapters/shared internals; ceval.c named call/value-operation helpers; bytecodes.c named call/prelude ops; ir_generator.py named entry/refresh/error producers; actual generated native products; `_testinternalcapi/native_scope.c` observer and its Clinic product; focused call-transaction tests | One owner for all shared VM/generator edits. Review source and generated products together. It may precede B as a useful dependency checkpoint with ordinary/EX constructor witnesses; no parser admission claim yet. |
| B: scoped parser and production constructors | pycore_modsupport.h; getargs.c lease/scoped unpack plus reachable metadata/error helpers; Clinic parse_args.py selection/setup/common exit; seven production Clinic inputs/products; only s_new handwritten body change in _struct.c; `_testclinic.c` registered constructor witnesses/product; focused Clinic tests and applicable clinic.test.c output | Disjoint from A. Begin implementation only after complete design acceptance and named source boundaries are granted. Depend on A's accepted contract; merge with A before E2E admission. No converters.py/varpos array work. |
| Coordinator | extension-api.md contract amendment/current status; integration receipts; shared index/commits/build freezes | Merge accepted A+B endpoints, verify cross-lane header identities, then candidate checks. No whole-file inclusion of separately held object/type/abstract changes. |

The seven production source inputs may receive checksum-only Clinic changes plus
selected generated outputs; s_new is the only handwritten production constructor
body edit. Any newly required helper/provider or caller family outside this table
returns for design review. The coordinator has authorized the linked disjoint grants. Stable-input native
checks remain serialized/coordinated.
Review design -> implement -> focused checks -> independent code/result review ->
commit; coordinator validates the merged slice. Dependency commits can remain
unlinked, but they use this final invariant and require no later lifetime rewrite.
