# Generic binder staging: outer lifetime

Status: bounded implementation following the unpack adapter. The
[interface and source peer](binder-staging-peer.md) is GO; root execution
remains pending. Native counted owners retain
the [selected policy](collector-policy.md). No complete M3/M4 gate is claimed.

## Actual missing arena authority

`eval_frame_push_and_init_code` reserves a thread frame and initializes its
header/constants before `initialize_locals`. The child is not in current_frame.
Keyword comparison can execute a str-subclass `__eq__`, kwargs insertion can
call hashing/equality, and argument/error cleanup can run finalizers. Mapped
current frames therefore do not describe the child's installed owners.

The fallback Python-call adapter also returns that child through EX cleanup:
object-vector free, borrow-scope release and callargs/kwargs decrefs happen
before the outer adapter publishes the child. Rooting only initialize_locals
and popping before those releases leaves a real gap. Parent IN_CALL homes stay
owning until the existing final detach; copied C arguments are independent
owners under native policy. A copy of their bits is not a second frame edge.

## Bounded API

Add a mandatory final `_PyTransitionRoots *staged` parameter to the two private
core constructors `eval_frame_push_and_init_code` and its EX form. The caller
owns the storage. Immediately after `_PyFrame_Initialize`, the core initializes
and links the descriptor with child frame, full initialized `[0,N)` homes and
CODE/FUNCTION/LOCALS header bits. Frame association starts NULL. No generator
exception bit applies to this thread-owned not-yet-entered frame.

Full initialized/NULL range is the current control; it is not a static phase
proof. It includes hidden constant owners, supplied/default/kw-only arguments,
varargs/kwargs containers and unbound NULL homes without a store per binding.
All geometry is acquired under the independent code pin. No extra owner count,
arena allocation, exact-call record or per-argument root update is introduced.

Success returns the child **with the caller-owned descriptor still linked**.
The owning caller commits current_frame=child and removes descriptor authority
in one callback-free sequence. It clears remaining/header authority before pop.
Later releases use the now-current child's normal authority. On binding failure
the constructor removes the staged descriptor callback-free, then invokes the
existing M2 retirement path, which registers before its first reentrant close.
No failed-child owner is dropped during that transfer.

The generic fallback is already noinline and owns one descriptor for the whole
attempt, including all EX/provider cleanup. No pointer into parent argument
homes is cached across a callback. Existing parent/child revalidation and final
call-window detach order remain. Existing native/heap argument copies and their
allocation failure paths retain native policy until installed in the child.

Exported constructor wrappers own a descriptor through their own cleanup and
pop immediately before return, preserving their current internal ABI. Mark
those reached wrappers noinline so the record cannot inflate an inline init
handler. Their post-return handoff is a separate explicit contract below; do
not claim this bounded patch supplies an outer record across unknown C callers.

## Complete producer/consumer matrix

| Route | Record lifetime in this increment | Next authority / open gate |
| --- | --- | --- |
| Fallback normal Python call | Outer noinline call adapter; core binding, exec validation, final detach and publication included | Child current before any done-cleanup callback; error transfer into M2 with no callback gap |
| Fallback EX call | Same outer descriptor threaded through EX binding and all object-vector/borrow/callargs/kwargs cleanup | Child publication or M2 failure retirement; parent call window retained until commit |
| Exported `_PyEvalFramePushAndInit` | Wrapper-owned descriptor through binding and final independent code-owner decrement | Actual callers must activate without intervening callbacks; native-provider/PEP523 outer coverage remains open |
| Exported EX constructor | Wrapper-owned descriptor through all EX cleanup and final code-owner decrement | Same post-return activation obligation; no source caller currently uses this exported form |
| `_PyEval_Vector` and `Objects/call.c` function vector provider | Constructor portion covered; argument-buffer free after return is callback-free | Eval entry/hook may call Python before actual child publication: requires a later outer entry adapter, not a claim of native pointer discovery covering arena homes |
| Init trampoline `_CREATE_INIT_FRAME` | Constructor wrapper covers binding; failed child's M2 cleanup covered | Current success handoff is immediate inlined dispatch; prove any intermediate bridge release only exact numeric boxes, and retain hook/frame-entry gate |
| Exact/known positional/keyword/default overlap | No new descriptor | Existing guard→move→initialize→publish commit is callback-free; caller-clobber/survivor maps remain a separate complete-phase obligation |
| Heap caller | Same generic outer descriptor when fallback runs; exact heap path remains callback-free copy | Heap caller still excludes moved aliases exactly once; child thread homes become current authority |
| PEP523 or custom execution provider | No blanket native record added | Full outer hook/provider lifetime and unusual frame consumption remain open M3 gates before arena scan removal |

The final code-owner decrement in exported wrappers is non-last because a
successful child has its independent pin. Nevertheless keep the descriptor
through it rather than relying on that fact to shorten the documented scope.
On failure the child is already retired and no descriptor remains linked.

## Mutation, failure and observation

Callbacks may replace function code/defaults, recurse/collect, mutate caller
locals or raise. Staged geometry uses the original code pin; existing binder and
call transaction validations must remain. A staged child is not an observer
ancestor and cannot be redirected through the active caller chain. Do not expose
it or classify it as executed merely to make traversal find its owners.

Cell/freevar construction has not run before first RESUME; initialized argument
and hidden constant homes remain readable NULL/values. A code-image change
cannot reinterpret those owners. If later staged cell creation is introduced,
its phase requires an explicit new row. Every failure consumes the original
function/locals/args exactly as before and hands installed frame owners to M2.

Five fixture methods use a keyword str subclass whose equality observes
precise transition IDs after an earlier positional/keyword owner is installed.
They cover EX fallback and native function provider entry, collection, nested
partial children, caller-local mutation, prior keyword/default/kw-only/kwargs
paths, equality failure, selected-code mutation and chain exhaustion. All
callbacks must see child function/code/payload roots, independently of arena
conservative scanning. Runtime execution remains root-owned.

EX cleanup lifetime is verified in source. A direct finalizer fixture for the
last successful EX cleanup release is still an open test obligation: the normal
fallback retains independent parent tuple/dict owners until publication, so a
fixture must prove its actual last-owner route rather than assume a provider
release is last. The record nevertheless spans all such releases and wrapper
code-pin cleanup. Parent duplicate owners must not be counted as child container
edges. Native ABI/stack/store review is root-owned.

## Files and cost gate

Frames lane edits private constructor signatures, exported wrapper scopes and
the fallback adapter in ceval.c, plus focused tests. Shared transition layout
and visitors remain unchanged. Root's retirement-reason hooks occupy separate
private helper regions. Freeze source before root copies/compiles.

One record is reached only in these generic outlined adapters; exact calls gain
no per-argument stores. The current 64-bit layout is 40 bytes, kept in the outer
fallback or exported wrapper. Each entry initializes the descriptor and links
the head; publication clears its range/header authority and restores the head.
The private constructor gains a ninth argument, with actual inlining and ABI
spill cost left to native review. Price those stores, wrapper noinline
boundaries and nested stack use against generic call frequency. This is
ownership preparation; no scratch clear removal, static map completeness or
performance gain is asserted.
