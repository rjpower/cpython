# M3 staged arena owners and native policy

Status: revised bounded proposal under the selected [collector policy](collector-policy.md).
This replaces the earlier global CValue/CObject registration proposal. NULL homes
and arena scans remain until every staged arena/map reader gate closes. Root
owns integration and execution; no completed native-precision migration is claimed.

## Selected boundary

GIL cyclic RC starts from actual counts and subtracts container edges once.
Counted native C owners already remain external in that accounting. The tracing
verifier has a different discovery policy: M4 removes its **arena** scans while
retaining native stack/register/PyMem conservative discovery. This explicitly
includes live exact numeric boxes. Their lack of outgoing GC edges does not mean
they are not roots. `_PyValueTemps` remains their bounded native workspace.

Do not add unconditional CValue/CObject heads, registration stores or larger
records across the sixteen existing CValue workspaces solely for rare verifier
runs. The rejected blanket-chain option adds links/head/unlink/NULL stores and
C-stack bytes without changing GIL RC correctness. GC's narrow cleanup instead
removes the unproduced FT head/loop and documents counted native workspace
semantics. Source peer GO covers that four-file cleanup; FT qualification remains
separate.

Explicit transition descriptors are needed for actual arena owners outside
active mapped frames: staged binding homes, initialized partial outputs,
unlinked retirement reservations and ownership transferred away from a heap
frame's counted edge set. Adding a C root cannot repair double subtraction of
an old heap edge. Native C counts, logical values, arena authority and container
edges must remain distinct in the shadow census.

## Callback/owner ledger

`IR_CONSUME_INPUT` moves scratch ownership into a C local and NULLs its home;
locals remain mapped owners. `TAKE` transfers a scratch owner or duplicates a
local. The generator already tracks consumed/spent owners, object views, stale
views, outputs and escaping statements. Carry those facts into the callback
ledger. A borrowed PRE input that remains mapped requires no new record.

| Boundary | Selected authority and remaining work |
| --- | --- |
| `finish_value_bridges_ex`, readers, attributes and displaced locals | Actual counted C owners/results retain native policy. Remove moved owners from heap/frame edge authority once; publish mapped outputs before callbacks and reload destinations after mutation. |
| Native call transaction and `_PyNativeScope` | Parent window remains mapped until detach; C callable/monitor/result/native scope fields retain counted native policy. Detached arrays in PyMem/native storage retain native discovery. Actual arena output publication and any staged arena window need verified phases; don't blanket-root borrowed vectors. |
| Generic/default/keyword/EX binders | Unpublished child homes are arena owners not reached by current-frame maps. Their record must start before the first binding callback and survive EX/provider cleanup until child publication or complete retirement. An inner binding-only record is insufficient if caller cleanup can reenter after it pops. |
| Generic unpack | Output homes are a growing initialized arena subset while iteration/errors/cleanup can reenter. An outer consumption/cleanup adapter must keep their prefix until the active frame accepts output authority. |
| Generated OWNER/TAKE, observer terminal paths and exceptions | Describe whether the value is still mapped, moved to native counted authority, transferred to a heap edge, or staged in arena storage. Saved exceptions and raw C results stay under explicit native policy; geometry/phase publication is required before observers or heap traversal. |

This ledger is not a full census. Every callback and early exit must eventually
have a concrete authority row; retaining native policy does not excuse missing
arena owners or stale heap/container edges. No scanner is weakened in this step.

The first adapter has GC peer GO and is implemented for source post-review in
`call55-pins`. It is not tested or integrated evidence. Generic unpack now
consumes the sequence owner and keeps its initialized suffix through iterator
and sequence cleanup. The reached function is explicitly noinline. Exact
specializations and native conservative policy are unchanged. Five focused
methods cover simple/starred cleanup, short/excess/raised prefixes, nested
unpack and traced ancestor redirect rejection with named-local mutation.

Current observer proof is bounded: ordinary `f_lineno` mutation outside a trace
is rejected; a trace inside iterator/destructor code sees UNPACK as an active
ancestor operation, while the pending-child redirect recipe admits CALL/CALL_KW
only. The focused fixture checks this rejection rather than changing admission.
M4 still needs verified POST handoff before prefix disarm and a complete
callback/observer phase matrix.

## First real arena adapter: generic unpack

Use the existing reached outlined `_PyEval_UnpackIterableValue`, not a universal
handler guard. Its two actual bytecode callers already transfer a sequence to an
owned `seq_o`. Change the helper contract to consume that owner on every exit
and remove the caller's later DECREF. This closes the cleanup gap where output
homes would outlive an inner descriptor before sequence destruction could call
Python. Exact tuple/list unpack specializations remain unchanged.

Reuse `_PyTransitionPrefix` from the observer adapter. For total output capacity
N, set `base=original_sp-N`, `begin=end=N`. Each backwards write installs a value
before decreasing begin. Every iterator callback sees exactly the initialized
suffix `[begin,end)`. On failure advance begin and NULL the home before closing
its owner. Keep the descriptor linked through iterator, surplus result and
sequence cleanup, then pop before returning; native scalar/iterator owners
remain covered by the selected native policy.

On success the complete initialized output region transfers callback-free back
to the current NULL-authoritative frame. M4 must replace that development handoff
with verified POST publication **before descriptor disarm**; do not silently
assume PRE maps include new outputs. Folded local unpack and observer mutation
need their own exact handoff proof. Preserve existing redirect admission and
post-provider refresh behavior; no new jump recipe is admitted by this adapter.

The record has zero frame-home/header/result authority and only the output
prefix. It neither counts another owner nor creates extra heap subtraction
edges. Code/geometry is computed before callback; the prefix visitor never
reconstructs it. This is one coherent first consumer, not complete M3 coverage.

Focused fixtures inspect precise IDs during the second `__next__`, excess-item
close, iterator destruction and sequence destruction, including simple/starred
success and every partial-failure prefix. Exercise recursive collection, nested
unpack, named-local mutation and chain exhaustion. Root executes candidate
release/debug runs. A precise fixture must fail without prefix registration;
plain object survival under conservative policy is insufficient evidence.

## Typed function owner transfer

The M1 initializer consumes a canonical owned function value. Its generic
`PyValue_AsKnownObjectSteal` currently includes borrowed/deferred acquisition
outside this contract. Add `_PyValue_FunctionOwnerSteal` with debug assertions
for nonNULL canonical home and actual function type. Pointer extraction belongs
inside the value representation helper; release code returns the typed strong
pointer without an ownership fallback or added RC. Immortal functions remain
valid strong pointers with immortal-aware later decref.

Audit every initializer source together: `PyFrame_New` explicitly DUPs its new
function; generic/EX initialization consumes its owning function argument;
unchecked and overlapping pushes consume owned callable values; exact CALL
transfers its owning callable window only after guards. Follow exported binder
callers and provider duplicates to prove the public internal entry contract.
A borrowed/deferred mortal input must be repaired at its producer, not silently
upgraded. Executing code remains pinned independently of mutable function code.

Completed source producer proof for the implemented accessor:

| Initializer route | Owning producer and commit |
| --- | --- |
| `Objects/frameobject.c:init_frame` | Explicit DUP from newly constructed function; direct heap homes initialized before admission. |
| Generic `eval_frame_push_and_init_code` | Consumes `func`; exported vector providers in `Objects/call.c` and `_PyEval_Vector` explicitly DUP the callable; fallback binder duplicates parent callable before consuming it. |
| EX binder | Its contract consumes `func`; fallback EX caller DUPs the parent callable. Tuple/kwargs cleanup does not convert that transferred function owner to a borrow. |
| `_PyFrame_PushUnchecked` | `getitem` cache produces DUP; property and overridden-getattribute paths explicitly DUP the guarded function. |
| `_PyFrame_PushOverlapping` | Known positional/default/keyword routes move `homes[0]` after all guards, NULL it and complete child publication without callback. |
| Exact CALL bytecode | Guarded callable window owns the value; callback-free initialization transfers it and clears window slot0 before dispatch. Heap-caller argument moves use the same callable owner. |
| Init trampoline caller | Init callable is an owning result/duplicate consumed by the exported binder; the code-backed trampoline itself has no function pin. |

The helper asserts canonical home and function type in debug, including the
actual immortal requirement for a tagged function, and removes generic object
bridge classification from release initialization. A focused internal helper
checks that mortal function count transfer into/out of the frame adds no second
count. Runtime/immortal/finalization qualification remains root-owned; source
assertions do not distinguish a borrowed mortal by tag alone.

## Implementation boundaries and gates

Frames lane owns unpack/helper/bytecode changes and the owned-function accessor;
coordinate its value.h region with GC's comment cleanup. Root owns generator
regeneration and shared integration. GC peers prefix authority and collector
policy, and owns root visitors and the narrow dead-FT-loop cleanup. Subsequent
binder staging APIs require a separate caller-lifetime proposal before edits.

Record the source costs: one prefix/transition record only in generic outlined
unpack, link/bounds/cursor stores at actual production/retirement boundaries,
sequence-owner consumption and helper ABI. Count actual native stack frames,
spills, branches and removed ownership classification; no speedup is assumed.
All home clearing remains. A complete callback/arena/heap-edge matrix, phased
shadow ownership comparison and every lifecycle reader remain prerequisites to
M4. The bitmap rival must use the same call, placement and boundness policies.
