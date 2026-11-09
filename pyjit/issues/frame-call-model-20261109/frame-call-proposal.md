# Proposed frame and call model

2026-11-09. Proposal only, against integrated `4b3d739d867` in
`build/worktrees/execution-boundaries-20261108`, now branch
`pyjit/frame-call-model-20261109`. No native jobs or runtime edits by this worker.
The candidate remains unpromoted. This proposal needs independent review before
implementation.

## Decision proposed

Use one Python frame ABI with a direct, guarded binding plan for known Python
calls. Frame argument windows transfer owners in place. A result recipient is
an ordinary destination or discard, accompanied by a compact executable
continuation and source-edge metadata where needed. Empty variadic bindings may remain virtual
until Python or a supported observer needs their objects. Physical executable
words represent semantic work, not empty source boundaries or release artifacts.
Retirement is an operation on the frame's owner state, not a sequence of POPs.

The first useful implementation should combine the known-call binder and lazy
empty kwargs. Splitting off another handler alias without removing the generic
preparation work would not test this model. Source-edge mapping is being designed
independently in the same packet; this proposal consumes its continuation
interface rather than retaining tagged POP recipients or a second public
instruction-offset domain.

Borrowing parent locals is an alternative, not the starting ABI. Existing overlap
already avoids argument copies, and a borrowed parent local can be overwritten
through a frame proxy while a child runs. A safe general borrowing ABI needs
anchor promotion at every escape/mutation boundary, not just a different decref.
It is not justified by counting nonexistent binder copies.

## What source and compiled paths establish

- `Python/ceval.c:3069` `call_transaction_overlap_python` handles the logging
  general route. It recomputes offset, count, flags/default bounds, optimized
  code shape, executable presence and stack space. For CO_VARKEYWORDS it pins
  code, calls PyDict_New, refreshes the caller/window, and repeats function,
  code/provider, executable, stack, default and offset validation before commit.
- `call_transaction_run_guarded_python_vector` already starts after function
  version/provider guards, but constructs an address-taken transaction and
  invokes that generic overlap preparation. Logging has no consuming-binder
  fallback, so removing fallback copies cannot explain its current overhead.
- `Include/internal/pycore_interpframe.h:571` `_PyFrame_PushOverlapping` transfers
  argument owners in their existing cells. It calculates overlap/tail bounds,
  builds a complete frame and initializes the new tail. `_PyFrame_Initialize`
  separately owns code because function.__code__ is mutable; the function is
  not a sufficient lifetime anchor for the executing code.
- `Python/frame.c:166` `_PyFrame_ClearExceptCode` handles escaped frame promotion
  and otherwise clears local owners. `clear_thread_frame` additionally releases
  code and pops the stack; RETURN_VALUE keeps its result owned across this work.
- Read-only objdump of the retained release ELF confirms the real allocation
  and refresh/recheck sequence: overlap+0x263 calls PyDict_New, +0x2b0 calls
  call_transaction_refresh, and +0x38e through +0x4a0 repeats guards before
  commit. The success route also saves six callee-saved registers and reserves
  0x58 bytes before its callback-heavy path. This is existing compiled work,
  not a prediction from C line count. `_TAIL_CALL_RETURN_VALUE` contains shared
  result canonicalization, owner cleanup, recipient classification and live POP
  decoding; these are separate costs.

The ELF has another static function named clear_slots belonging to object
member cleanup. Its symbol alone is not evidence about Python frame cleanup;
frame analysis uses `_PyFrame_ClearExceptCode` and its reachable path.

## One frame ABI

Each active frame has its own executable/code owner, function owner, globals and
builtins views, caller link, value base and stack-pop checkpoint. The physical
value array holds canonical PyValues. An initialized ordinary home owns its
counted edge; inline and immortal values need no mortal count. The code's slot
and ownership metadata describes unbound, virtual, ordinary and transferred
states. The first implementation retains existing NULL initialization except
for the explicitly virtual kwargs local; it does not scan uninitialized memory.

The caller owns argument evaluation and its window until a non-reentrant commit.
At commit, function and argument cells transfer to the child; caller maps no
longer count those aliased cells. While the child is current it owns bindings,
overwrites and cleanup. The caller remains suspended with an executable return
continuation, source call position and result recipient. A frame does not acquire
another owner merely because an argument window overlaps it.

Results remain C-owned while child retirement runs, then move to the recipient
or are released for discard. There is no original-result slot retained solely to
recreate a discarded-result POP, and no virtual POP instruction event.
Instruction monitoring sees actual compact semantic instructions;
f_lasti/dis/co_positions use their offsets. Source-edge LINE events can carry
an observed-only source override without inventing another public PC domain.
Native completion uses the same recipient categories.
Generator SEND destinations remain a distinct continuation kind, not a Python
call flag encoded into a slot number.

Canonical ownership is also an ABI fact: a result transferred unchanged from a
canonical frame home does not require canonicalization again on return.
Currently IR_STORE_RETURN calls _PyValue_Publish, whose exact-long/float checks
appear in compiled RETURN_VALUE after child cleanup. `_RETURN_VALUE` itself
only moves the already-canonical input. The generator should preserve this
provenance across the frame switch and invoke canonical return publication;
native/raw producers canonicalize at their actual publication boundary.
Instrumented return and generator/SEND paths need the same provenance audit,
not a global removal of _PyValue_Publish from every result bridge.

Selected cleanup order is a policy, not a Python prompt-finalization guarantee.
We retain it where inexpensive in the first binder, but no proposal is rejected
solely because the exact last decref or destructor f_lasti moves. Live values,
published frames, supported observers, exceptions and language evaluation order
must remain correct. Loops and repeated calls cannot retain an unbounded history
of ignored results: direct completion release or bounded owner-region reuse is
required.

## Stable binding plan and direct commit

Specialization builds a bounded plan in existing per-code feedback storage,
with owning references only where required by that storage's GC contract. Its
guard identity is the callable/function version and actual code/provider;
the plan records input offset/count, supplied positional count, default source
range, variadic local indices, required frame slots and entry continuation.
Code objects are immutable but defaults/provider/function code can change.
`Objects/funcobject.c` clears func_version on vectorcall, code, defaults,
kwdefaults and closure mutation through its Python and C setters. A valid
function-version guard therefore protects the plan's binding shape; still audit
all plan dependencies when implementing it. In-place mutation of a kwdefaults
dict does not require changing that dict's identity, so plans depending on its
contents need a dict-version guard or must read current values. No unchecked
raw default-array pointer survives mutation.

Prefer a scalar-only plan: the existing function-version cache field supplies
guard identity; an immutable code-owned layout holds local indices/flags/frame
size, and per-site scalar fields hold supplied count/default range/input offset.
Read the actual code and defaults from the currently window-owned, guarded
function immediately before the no-callback commit. This avoids adding a strong
code/function/default reference to every feedback site or treating a raw weak
cache pointer as an owner. The executing child still acquires its independent
code home as today.

One guarded handler verifies that identity, recursion/stack capacity, caller
kind and observer/PEP523 admissibility. A hit goes to the direct binder, which
does not construct `_PyCallTransaction`, reread call kind, rediscover layout or
repeat already-proven guards. Ordinary default PyValues are duplicated into
their declared homes. No callback or fallible allocation occurs between the
final guard and publishing the complete child as current. A miss enters the
existing generic transaction before any ownership change. It does not replay
preparation that has already run.

Exact and general calls are two plans for the same ABI, not two runners with
different lifetime rules. Empty variadic bindings require no allocation during
commit. If a nonempty tuple/dict must be built, the first implementation declines
before commit to the generic binder. Later direct binding can commit a valid
child prologue first, then allocate with the child current and the parent window
already transferred; the observer-visible child must never contain arbitrary
uninitialized bindings. That extension needs its own incomplete-frame policy.

## Empty kwargs as a virtual binding

An absent **kwargs binding means a fresh empty dict if used, not an unbound
local and not a shared mutable singleton. Record a pending flag and known local
index in the frame; its physical home is canonical NULL. Empty *args uses the
existing immutable empty tuple, whose identity does not require a fresh object.

The existing register compiler folds loads into direct local input fields and
call windows. Therefore trapping LOAD_FAST alone is incorrect. The compiler
must insert a real semantic materialization operation before every possible
first read of the kwargs binding, including folded input fields, call-window
construction, return, comparison, truth tests and LOAD_FAST_AND_CLEAR. CFG joins
may need a conditional materialization operation; it checks only the specific
pending flag. This is not a generic class check on every local read. Rebinding
or deleting the binding cancels the pending state without allocation. Local
movement/clearing transfers or cancels it according to the actual operation.
The lowering verifier must reject any path reading a virtual home as NULL.

A simple first lowering inserts the materialization operation before kwargs
local loads, before existing load folding runs, and treats it as effectful.
Then direct input/window folding cannot move a read before materialization.
This may conservatively keep a load on paths that actually use kwargs; it adds
no instruction to paths that never read the binding. A later dominance pass can
merge redundant materializations. The operation does nothing when pending is
clear, so a post-delete checked load still raises rather than recreating kwargs.

Captured kwargs initially decline lazy binding: MAKE_CELL/cell accesses would
otherwise require promotion before capture. Generator/coroutine callees also
decline initially. This excludes complex ownership without changing ordinary
synchronous logging coverage. Later compiler-inserted materialization before
MAKE_CELL can expand coverage under the same ABI.

Materialization allocates a fresh dict while the child is current. On failure
the pending binding remains virtual until unwinding cancels it; on success the
canonical home is installed and pending clears before any further callback.
Source narrows the reentrancy proof: PyDict_New uses the dict freelist or
PyObject_GC_New, then initializes and tracks the dict. Python/gc.c
_PyObject_GC_Link schedules collection through eval_breaker rather than running
collection synchronously. No core Python callback occurs in that interval.
Python/frame.c _PyFrame_MakeAndSetFrameObject already relies on the same
non-callback allocation contract explicitly. Materialization must not add a
periodic poll between allocation and installation. Custom allocator contracts
must be assessed against that existing runtime guarantee, rather than silently
adding a new recursive allocator-observation API. If any actual supported
callback remains in the chosen path, install an explicit revalidation protocol
before admitting lazy binding; returning a fake mapping is not an alternative.

Supported observation must materialize before exposing local values. The narrow
first protocol does so before a frame object is created/published, which covers
profile/trace callbacks, locals()/frame proxies, traceback/frame escape and a
child inspecting an ancestor. Existing exposed frames decline lazy entry.
Frame-copy, clear, f_lineno, generator and GC consumers must explicitly understand
the pending state: no absent dict means a missing GC edge, and no RC edge exists
until allocation. Remote/raw frame readers need a documented way to interpret
the virtual binding; they may not report it as unbound. This consumer census is
an admission gate, not completed by the present proposal.

## Retirement and borrowing alternatives

The initial binder still transfers window owners and uses existing frame clear.
A subsequent reviewed owner-map increment can use code/PC liveness plus dynamic
initialized bits for branch-dependent locals, clearing only actual owners.
Every overwrite and exception edge updates that state; GC, frame escape and
debugger local access use the same map. Slots cannot be left as stale owning
bytes and later scanned as though live. Generated fixed clear recipes may help
small stable frames; they must not duplicate a full return handler per shape.

Frame/region retention can postpone releases, but its storage must be bounded
across loops, recursion and nested calls and participate in cyclic traversal.
It trades exact decref work against retention and owner bookkeeping. With
logging's mortal message and transferred windows, merely retaining arguments
until frame exit already matches the existing lifetime; there is no free
elimination of the eventual owning edge.

Borrowed child views can remove caller-load duplication plus child-clear pairs
only when an explicit owner survives the entire view. A mutable parent local
is not that owner without promotion on ancestor mutation. Cells, frame proxies,
callbacks, suspended generators and escaping frames make this a substantial
contract. A dedicated owner region might supply the anchor, but moving the same
count into a region is not a removed count. Measure eligible mortal pairs first.

## Cost ledger and admission

Logging executes 20 Python frames per outer loop and approximately 19,561 native
instructions versus upstream 13,944: the gap is about 5,617. Samples attribute
14.70% to overlap preparation, 10.90% to frame clear, 8.44% to method lookup and
8.19% to return. Sample shares are not removable instruction counts and cannot
be added into a promised speedup.

| Work | Coverage | Budget to verify in actual compiled hit paths |
| --- | --- | --- |
| Direct stable binding | Ten general logging entries, exact route also shares ABI | Remove transaction construction, repeated shape/default/stack checks, code pin/unpin around allocation, refresh and related spills; count direct path instructions before claiming savings |
| Lazy empty kwargs | Ten Logger.debug entries while disabled | Remove ten empty dict allocations/releases and the callback-driven precommit revalidation; allocation savings must be counted once, not again under direct binding |
| Semantic recipient | Ten exact results plus ten general completions | Remove tagged-recipient/live-POP compatibility work and standalone release words; retain real result ownership/observer work |
| Canonical return ownership | Twenty slot-origin returns | Remove redundant exact-number type checks/canonicalization on an unchanged canonical input; raw native publication still canonicalizes |
| Owner-map retirement | Twenty frames | Remove only demonstrated dead-slot scans/NULL stores and duplicate owner pairs; initial direct binder does not claim this budget |
| Method lookup | Separate worker scope | Do not attribute its 8.44% sample share to frame changes |

The gap averages about 281 native instructions per entered frame; this is not
an attributable measured cost of either call route. The compiled generic path
shows dozens of instructions for the post-allocation refresh/spills and repeated
guards alone, before counting the dict allocator and later release. Across ten
general calls these are a credible several-hundred-instruction opportunity;
the allocator contribution remains unmeasured. The exact break-even gate is a
before/after compiled path ledger plus unchanged-workload
hardware counters; no arbitrary claim of all 5,617 instructions is justified.
If the first increment leaves most of the gap, proceed to owner-map and method
lookup work instead of treating fewer handlers as parity.

Focused admission covers default mutations, code/provider changes, misses before
effects, empty/nonempty varargs/kwargs, rebind/delete/clear, folded reads, cells
and generator declines, local identity/mutation/escape, ancestor inspection,
tracing/profile/monitoring enabled after entry, allocation failures, exceptions
and traceback/frame retention. Source verification must reject unmaterialized
reads and invalid ownership transitions. Coordinator owns native builds,
compiled-path checks, matched logging and spectral/unpack/pickle/float/deepcopy/
nbody screens. No permanent runtime switch, benchmark rewrite or alternate
compiler is proposed.

## Proposed completion lowering (not admitted or implemented)

This increment removes the remaining executable ownership-only instructions.
The scalar discard sink is not sufficient: canonical stack-position owners,
shared values, loop exhaustion and delegated-generator completion have different
ownership transitions. `POP_EXCEPT` remains a semantic exception-state operation.

### Iterator completion

`FOR_ITER` has two outcomes. A yielded item publishes its body result and keeps
its iterator/index owners live. Exhaustion detaches iterator/index, holds any
returned value in C, emits the applicable StopIteration observation while its
inputs are valid, and completes directly at the real exhausted target. All
specialized variants obey this contract; neither `END_FOR` nor `POP_ITER` is an
executable successor. A generator child return invokes the same semantic
completion after child retirement, rather than returning to cleanup words.

Before a callback, either the published pre-completion window owns its inputs,
or C owns detached inputs and the window is NULL. Never advertise both owners.
On observation failure, retain the original error, close detached owners, and
unwind from the actual FOR_ITER/source-edge EH state. On success, detach before
closing: finalizers can observe a valid completed frame. A finalizer enabling
observation starts subsequent events; it cannot recreate completed operations.

### Delegation completion

`SEND` needs distinct yielded and completed destinations. Its window's yielded
home remains `base + 2`, preserving receiver/index for resumption. Its explicit
A field names the completed result recipient; its branch field names the real
completion target. B folds the former `END_SEND` output mapping into these
fields before allocation. A generator yield returns to the yielded home; a
return uses the completion recipient after child retirement. Native send and
inlined generator send use the same outcome contract.

For completion, hold the result in C across child retirement and applicable
StopIteration observation. On callback error, release that owner and unwind
with the still-valid receiver state or an explicitly detached NULL window.
On success, detach receiver/index, commit the result to its destination (or
close it for IR_NO_DST), retire detached owners and resume the real target.
No old END_SEND instruction event or historical destruction coordinate remains.
The cold child-completion helper may decode the actual pending SEND/FOR_ITER
operation to select its semantic outcome. Ordinary dispatch does not interpret
a generic release list. Unsupported ancestor redirects must not clear an
active child's overlapping argument homes; pending redirect is committed only
after child retirement.

### Shared or canonical owners

First remove needless sharing in the compiler: a COPY used only by a branch and
its discard should lower to the branch's semantic consuming/retaining outcomes.
Source-edge metadata supplies source lines independently; a historical line
boundary must not force a copy or release word. Unique producer sinks remain
the primary lowering for independently owned unused results.

For genuinely shared owners that outlive their last semantic use, propose a
bounded owning region rather than a hidden per-op cleanup program. Homes have
three ranges: named locals L, internal retained owner homes R, ephemeral
scratch T. Only L is exposed through local names; L+R have the existing
borrow-on-read and replace-on-write ownership behavior. T keeps the current
NULL-before-reuse rule. R is fixed at compilation, not appended at each loop
iteration; each overwrite detaches and releases the old owner. Frame exit
releases all physical owners. This retains a bounded number of values, though
an individual retained value may be large; memory screens are required.

This requires an explicit internal owning-home count in code/execution
metadata, not fictitious Python locals. The compiler may put an owner in R
only when every producer supports replace-on-write and every consumer supports
borrowing or an explicit ordinary argument-window transfer. Multioutput and
window producers need a proven ownership assignment; they cannot silently use
the local-store path. The compiler verifier must prove scratch NULL reuse,
borrow anchors across callbacks, and disjoint owning homes at merges. Exception
unwind clears T and preserves valid R owners until their region/frame retires;
GC visits all actual homes. Escaping frames preserve R just like other physical
owners, while debugger/source states expose only semantically live values.

Normal overwrite installs the new canonical owner before closing the detached
old owner; on callback failure the new state remains valid and unwinds normally.
A read of R creates no new owner until a consumer requires ownership, such as a
call-window copy or return transfer. No local name/proxy exposes a logically
dead retained owner. Simply deleting POP while reusing its old scratch home is
not valid and is not this proposal.

### Integration interfaces and gates

B owns CFG outcome fusion, discard/copy analysis, retained-home allocation and
verification, compact targets, source-edge/EH recipes and observer jump
admission. A owns DSL/generator outcome handling, result/owner transfer helpers,
all specialized iterator/send implementations, return completion and static
init shim. The init shim becomes real EXIT_INIT_CHECK plus RETURN_VALUE, and
its caller saves the real first continuation; no dummy NOP remains. Root owns
merged regeneration, native checks and measurement.

Before implementation, review the exact retained-owner metadata/consumer
census and determine whether compiler consuming branches eliminate the observed
shared POP inventory without R. Do not introduce R merely to solve an absent
case. Tests must cover native/generator exhaustion, send yield/return/error,
StopIteration callbacks that raise or enable tracing, shared mortal owners in
loops, overwrites, frame escape, GC cycles and exception cleanup. Native gates
must also demonstrate no executable NOP, POP_TOP, END_FOR, POP_ITER or END_SEND
in ordinary or static programs, without replacing them with cleanup aliases.
