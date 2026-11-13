# Outlined constructor shim ownership

2026-11-12 design before edits. Source: integrated constructor route around
`Python/bytecodes.c:_ALLOCATE_OBJECT/_CREATE_INIT_FRAME` and generated
CALL_ALLOC_AND_ENTER_INIT, plus the private binder in ceval.c. Root owns
integration/validation. Frames owns the concurrently changing provider region;
freeze a small ceval helper insertion boundary before source edits.

## Actual source and gaps

The specialized route checks type version, Python eval-hook absence, recursion
and combined init-code + InitCleanup capacity. `_ALLOCATE_OBJECT` loads borrowed
cached init/function code, allocates the instance, duplicates the function and
replaces the callable/self window homes. `_CREATE_INIT_FRAME` then pushes an
unchecked two-home shim, duplicates self into shim0, and calls the consuming
generic binder with init and `args-1` (self plus arguments). On success it sets
the caller destination/continuation and accounts for the shim's recursion depth;
the next `_PUSH_FRAME` publishes the init child. On failure it clears shim
scratch before M2 frame retirement.

Two real arena authorities are absent during construction/failure: shim0's
self owner and any remaining shim homes/code; the not-current child already
has the new binder stage, but an outer scope must retain both until disposition.
Failure's `_PyFrame_ClearTemporaries(shim,0)` can itself run the self finalizer
before M2 registers anything. The shim must already be staged through that
close; a record added only inside frame pop is too late.

There is also an alias/edge issue. Generated code calls the consuming binder
on the caller window, and executes `IR_CLEAR_SLOTS(window,2+nargs)` only after
the call returns. A generator/heap caller's traversal can see those words as
counted container edges after their counts moved into child homes or were
closed on an error. Rooting the shim does not repair duplicate heap-edge
subtraction or a dangling parent home. The ordinary fallback binder instead
duplicates evaluated caller owners and detaches the parent only at commit.

`PyType_GenericAlloc` normally schedules GC rather than running Python, and
its reference-tracer contract forbids allocating Python objects or changing
exceptions. Nevertheless move the existing cached-function DUP before the
allocation so the borrowed cache entry is never used afterward without a pin.
Release that acquired owner on allocation failure. Select/pin the executing
code independently for the actual child binder; do not use a preallocation
borrowed code size as lifetime or post-callback geometry authority.

## Guard facts and the shared call contract

No ownership policy is selected yet. A duplicate-then-detach adapter is a
correct reference if its refresh/commit is complete, but adds a DUP and close
per evaluated argument solely to retain an implementation window. It should
not become the default constructor cost without comparison.

`specialize.c:function_kind` admits optimized code without varargs, varkw or
keyword-only parameters and prepares its executing image. The class cache
does not require the actual supplied argument count to match. In particular,
`test_opcache.test_push_init_frame_fails` deliberately specializes a failing
call, and `test_assign_init_code` replaces the cached function's code with a
varargs image. Type-version validation does not validate mutable `__code__`.
The current image, argument shape, ready executing image and reservation must
therefore be rechecked at the actual entry boundary.

For current simple code with exactly `co_argcount == nargs+1`, the existing
`_PyFrame_PushUnchecked` typed function/header initializer plus positional owner
transfer can prepare the child without calling the generic binder. This is the
same narrow entry contract used by existing property/getitem entries: consume
the function owner, establish initialized NULL homes, move supplied owners into
their final argument homes, and publish before callbacks. It does not implement
defaults, keyword matching or errors again. Rechecked misses use the existing
generic binder, including valid calls requiring defaults. A shared positional
transfer primitive is preferable to a second constructor binding engine.

Do not use overlapping-call reservation blindly here. The child's logical
previous frame is the shim, while arguments physically belong to the caller;
an extent beginning in that caller window can clobber shim storage. Preserve
the existing separate child reservation and argument moves. Ordinary exact
CALL's zero-copy geometry is unchanged.

## Proposed slow interface and move lifetime

Use a reached `Py_NO_INLINE _PyEval_CreateInitFrameSlow` beside the private
binder, declared in pycore_ceval.h. Only a rechecked generic-binding case enters
it; scoped 40-byte records and owner workspace stay out of the callback-free
exact constructor entry and ordinary CALL/SEND. The helper receives the actual
caller, canonical site/continuation, window geometry and destination. Its
preparation failure and consumed-window result must be explicit, so allocation
failure cannot accidentally make the handler forget still-owned inputs.

1. Validate the current site/window and allocate any required native owner
   workspace while original caller homes still own their counts. Allocation
   failure leaves those homes intact for existing error unwind. Reserve/init
   the shim before detaching the window; no potentially failing reservation
   may sit between detach and establishment of its staged observer state.
2. Initialize the shim, install its existing self DUP, and link a full initialized
   NULL/value STAGED authority covering its homes and CODE before the first
   callback-capable bind or close. Publish the explicit parent CALL_STAGED
   observer state described below, then move init/self/argument bits into
   workspace and NULL their caller homes in a callback-free sequence. This
   transfers ownership without DUP/close pairs. Workspace follows the selected
   native conservative policy and is never another container edge bank. The
   shim has NULL function/mapping and no generator exception state.
3. Pass moved init and argument owners plus independently pinned selected code
   into the mandatory-stage private binder, with shim as `previous`. Do not
   close workspace entries after the binder consumes them. Its child stage is
   nested and retained on success; failure transfers child authority to M2
   while the outer shim stage remains linked.
4. Keep the shim stage through all failure self closes, not just the later frame
   pop. Preserve the pending error, NULL each shim home before its one-pass
   close, then transfer to M2 STAGED_FAILURE without a callback gap. Keep the
   outer node linked with only its parent CALL_STAGED metadata through the M2
   closes; zero its arena authority before the inner retirement record takes
   over. Its shim pointer remains valid through those callbacks and the final
   PopFrame has no callback before clearing/popping this outer node. Full-range
   staging remains necessary if callbacks write an already visited home; a
   shrinking remaining-prefix record is not equivalent. Pop records before
   releasing their owned reservations; this metadata-only outer node owns no
   reservation after M2 returns. Do not reallocate the instance or reread the
   function's replaced code after callbacks.
5. On success, release only genuinely independent native/code owners while both
   arena stages remain live. Set the caller destination/continuation and shim
   recursion accounting, then disarm/pop child followed by shim in a
   callback-free handoff to existing `_PUSH_FRAME` publication. Clear the parent
   staged observer state in that same handoff. No argument-buffer owner close
   is required after a consuming bind.

The handler must distinguish preparation failure from a consumed window and
mark DEAD/SYNC state accordingly. Generated code must not close those owners a
second time. Its current `value_temps=NULL`, NULL displaced-owner initialization
and direct `_PUSH_FRAME` publication make the successful pop-to-publication gap
callback-free; preserve that as a generator contract. DTRACE/LLTRACE run after
publication. Do not place a record in a wrapper that inlines into every handler.

## Observable state during staged binding

Moving scratch owners does not change Python named-local bindings: f_locals,
frame GetVar and normal debugger local names cover `[0,nlocalsplus)`, not the
private evaluated argument window. Existing consuming error paths already
release extra arguments during binding. Exact destructor timing and an
incidental extra temporary count are not the Python model being preserved.
Native owners still count and remain roots under the selected policy.

Redirect is the material observer boundary. The current setter reads immutable
CALL/CALL_KW identity and can derive a pending-call recipe from an ancestor;
the executing specialized opcode alone does not reject a constructor. Nor does
an absent/OTHER observer context guarantee rejection. A moved window must not
be authorized as an available argument bank by those recipes.

Propose one explicit `caller_staged` bit in the existing transition-record
padding byte, set only on this reached shim stage. Its frame's stable `previous`
identifies the blocked parent. A cold observer lookup rejects redirect of that
parent before the setter's current-line identity shortcut or pending-call
reconstruction. It does not reject f_locals mutation, collection or unrelated
nested calls. This adds no frame field, record size, count or per-argument store;
the constructor stage already needs a chain head/store. No generic handler
decoder or ordinary exact-call branch is introduced. Root/frames must freeze
this actual producer/consumer interface before edits; an explicit new record
kind is an alternative if preferred, not a reason to infer state from shim
geometry. M4 can later admit redirects only with an explicit staged owner recipe.

## Capacity, mutation and static-image obligations

The shim's unchecked push needs a **current** shim-sized capacity proof. The
child uses checked `_PyThreadState_PushFrame` with the independently pinned
selected code and may use another chunk; the old combined check is not a proof
that a later mutable init image still has the same size. Either retain and
prove the immediate shim guard/reservation invariants, or use a checked shim
reservation with shared trampoline initialization. Do not duplicate an ordinary
function initializer for the NULL-function static shim. Any callback during
preparation must restore caller/reservation state and pass actual refresh
before dereferencing an unchecked reservation address.

The cached function pin must span instance allocation; the child's execution
code pin must span preparation/binding independently of function replacement.
Type-version changes cannot send this already-allocated instance back through
tp_new. Preserve actual recursion accounting: one extra shim decrement on
success and the child decrement in `_PUSH_FRAME`, none for an unentered failed
construction. Keep the existing return-destination mutation semantics.

The trampoline initializer currently NULLs borrowed globals/builtins only in
debug builds. Active GIL trace walking visits those fields on THREAD frames,
including a published shim. Initialize them to NULL unconditionally in this
rare producer, or supply an explicit nonordinary-header reader contract before
publication; release reused storage is not a valid borrowed environment. The
prepublication CODE-only shim mask does not repair its later active reader.

InitCleanup has its own static executable words and nullable function header;
its phase proof is separate from ordinary code plans. M4 must certify shim
entry/self, pending child, EXIT_INIT_CHECK result validation, error unwind and
return, plus caller/child alias partition and full clobber capacity. Development
full initialized NULL/value staging is not permanent static authority. No arena
scan or NULL-store removal follows from this adapter.

## Proof, file boundaries and costs

Fixtures must prove the actual specialization was reached, including the existing
test_opcache changed-`__init__.__code__` and failed-init-frame cases. Add precise
root observations in binding failure/excess-argument finalization while shim
self remains live; successful init must see normal active authority after
handoff. Exercise generator/heap caller edge ownership with recursive cyclic GC,
init code replacement with larger geometry, small-capacity/chunk growth, nested
constructor callbacks, observer named-local mutation/ancestor jump policy and
complete record/reservation exhaustion. No malformed-code execution or relaxed
jump admission can stand in for a reached constructor fixture.

GC owns bytecodes/prototype and the agreed small outlined helper region;
frames peers its private binder/provider interface and avoids concurrent writes
there. If the region is still changing, root applies the reviewed helper hunk
after provider integration. Root owns generation, role builds, tests and native
measurements. Compare at least these source costs before selecting the route:

| Route | Argument ownership traffic | Arena/observer work |
| --- | --- | --- |
| Current generic constructor | A child writes, delayed A source NULL stores; unsafe aliases during bind | No shim stage; child binder stage |
| Duplicate reference adapter | A DUPs, A child writes, A original closes and source NULL stores | Two scoped nodes; refresh/commit across callbacks |
| Move slow adapter | A native buffer writes, A source NULL stores, A child writes; no argument DUP/close pairs | Two scoped nodes; explicit parent staged barrier and cold redirect lookup |
| Rechecked exact entry | A final child writes and A source NULL stores; no native argument buffer | Callback-free typed initialization/publication; no scoped records |

A includes self and explicit arguments; init-function and the existing shim
self owner are accounted separately. Buffer allocation, helper call/ABI,
current-image guards, reservation checks, selected-code pins and failure cursor
work must also be counted. These are source operations, not measured costs.
No large record enters the ordinary CALL_ALLOC handler or unrelated exact call.
