# Constructor shim: frames design peer

2026-11-12 architecture GO for the current
[init-shim proposal](init-shim-proposal.md), with the caller-staged interface
condition below. Root selects rechecked exact transfer plus outlined generic
move-and-NULL. Source post-review and native qualification remain required.
Duplicate-then-detach is a correct cost reference, not the chosen hot policy.

The exact path can reuse `_PyFrame_PushUnchecked` and
`call_transfer_exact_args(child->base, window, nargs)`: the mover transfers self
from window1, then explicit arguments from window2 onward, and NULLs those
source homes. Move/NULL the function home into the typed initializer in the same
callback-free commit. Recheck current optimized/simple flags, actual argument
count, ready executing image and combined shim/child capacity before the
separate reservations. Do not overlap caller argument storage with a child
whose predecessor is the shim. No defaults/keyword/error binder is duplicated.

A standard-function-provider guard belongs at the preallocation deopt boundary.
A custom vectorcall miss must use the ordinary class path before an instance
exists; it cannot go through the consuming direct binder after allocation.
Pin the cached function before allocation. Select executing code independently
for a generic child, while the checked allocation handles changed geometry.

The slow move policy preserves named-local bindings but invalidates the private
caller argument bank. Publish its explicit redirect barrier before moving and
NULLing any input. Native workspace carries counted C owners under the selected
policy, not duplicate heap-container edges. Keep nested shim and child authority
through partial binding and all callback-capable closes; preparation failure
must explicitly leave the original window owned. Success consumes it once and
publishes the child before any further callback.

Freeze `caller_staged:1` in byte21, set only on the shim scope. Cold observer
lookup requires the explicit flag, nonNULL frame and stable shim.previous equal
to the target parent; reject before line-identity shortcuts and pending-call
recipes. Named-local mutation and unrelated nested calls remain allowed.
Provider `handoff_staged_frame` must **skip flagged nodes**: a metadata-only shim
node is not an ordinary consumable provider token, even though kind is STAGED.
Otherwise generic handoff could NULL its frame and lose the redirect barrier.
NULL-frame visitor handling and Pop must assert caller_staged is clear.

During failure, zero shim arena authority before inner M2 retirement but retain
its flagged metadata node through every close. The old shim pointer is read only
while its reservation is live. After M2 pops storage, clear flag/frame and pop
the metadata node without a callback or poll in the gap. Only the shim helper
owns this disposition; do not call the ordinary provider-token retirement API
on the flagged scope. Child/provider stages remain unflagged.

Current generated CALL_ALLOC_AND_ENTER_INIT has value_temps=NULL and
ir_displaced=NULL before `_PUSH_FRAME`. That operation directly publishes the
child→shim→caller chain; DTRACE/LLTRACE follow publication. A generator invariant
must preserve the callback-free scope-pop→publication gap. This is distinct from
DISPATCH_INLINED, which has a bridge cleanup check.

Unconditional NULL globals/builtins in the static trampoline initializer repair
its actual active tracing reader; a prepublication CODE-only stage cannot make
release header garbage a valid borrowed environment. Preserve static InitCleanup
phases, recursion accounting and destination publication semantics. Complete
map/alias/clobber proofs remain M4 gates. The existing reached-specialization,
changed-code/failing-bind, heap-caller GC, nested finalizer and redirect-barrier
fixtures are necessary. No candidate/native execution ran for this review.

The helper insertion before call_transaction_overlap_python is disjoint from
the frozen provider component except for the agreed shared matcher flag check.
Root should integrate the reviewed small hunks over provider be36adec43a.
# Source postreview of revised producer

2026-11-12. GO for the eight-file GC-lane producer and three focused fixture
methods, subject coordinator generation/build/runtime gates. The checked exact
path reuses the typed initializer and positional mover in separate child/shim
reservations. Slow binding moves inputs to native owners and NULLs actual
caller homes before callbacks; its initialized shim and child stages stay
nested correctly. Preparation failure leaves actual homes for ERROR_NO_POP.
The error branch now precedes DEAD, preventing generated successful slot clears
from discarding intact preparation-failure inputs.

The explicit caller_staged flag blocks parent redirects before the identity
shortcut while permitting named-local mutation. The provider matcher excludes
flagged shim nodes; NULL visitors and pop require the flag removed. Failure
retains the metadata barrier through self retirement and M2 final closes, then
clears it immediately after PopFrame in a callback-free tail. The saved-error
restore is callback-free under the selected deallocator exception-preservation
contract; an explicit no-raised-error assertion documents that requirement.
It does not retain a flag pointing to freed shim storage during callbacks.
Trampoline globals/builtins are now unconditionally NULL, including release.

The fixtures exercise specialized exact active authority, a failing heap caller
with more than eight arguments and a precise shim self root, nested actual LINE
redirect rejection with named-local mutation, and changed init code with larger
checked geometry. They observe actual specialization rather than assuming it.
Root must inspect generated ERROR-before-DEAD ordering, current/shim/child
publication and native stack/spill costs. No native execution was done here.
