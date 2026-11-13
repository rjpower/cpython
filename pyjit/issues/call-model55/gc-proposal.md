# GC and observation interface proposal

Scope: M1 typed consumers and M2 nested roots under NULL authority. Reviewed
target: [model55](../../design/proposals/register-call-model55.md). Implementation
and representation selection remain separate gates.

## Typed header consumers

Retain names/offsets of `f_executable` and `f_funcobj`, changing their types to
`PyCodeObject *` and `PyFunctionObject *`. Both are independent strong pins.
Interpreter/base sentinel code/function are NULL; code-backed trampoline frames
have a code pin and nullable function. Existing owner kind distinguishes the
sentinel before dereference. Heap frame traversal uses ordinary `Py_VISIT`;
clear uses `Py_CLEAR`. Do not borrow executing code through mutable function code.

Remote readers read the typed code pointer directly and recognize interpreter
owner before testing NULL code. Keep range checks for torn/reused samples. GDB
access is already pointer-shaped and needs only sentinel-before-code ordering.
FT deferred-root accounting excludes the new strong pins; optional marking
visits both, rather than classifying them as PyValues. No FT qualification claim.

## Nested transition API

Use a nullable `_PyThreadStateImpl.transition_roots` head. A reached outlined
adapter owns the record storage; no ordinary inline handler/exact-call prologue
initializes a large record. A borrowing helper may keep PRE homes as authority.
The record is collector metadata, never a GC container and never a new owner
by itself; it describes owners already moved to C/retirement authority.

```c
typedef struct _PyTransitionRoots {
    struct _PyTransitionRoots *previous;
    _PyInterpreterFrame *frame;
    int remaining;                     /* initialized NULL-control [0,n) */
    unsigned char header_mask;         /* typed pins, association, gen exception */
    unsigned char result_present;
    PyValue result;
} _PyTransitionRoots;
```

This first real consumer has no unused batch/prefix fields. Add the target's
eight-root batch and dynamic prefix only with their first actual adapters. The
later map iterator uses the frame's pinned code and published PC/literal phase;
do not duplicate site/map keys in this record or add a dependent hot decoder.
Header bits are CODE=1, FUNCTION=2, LOCALS=4, FRAME_OBJECT=8, EXCEPTION=16.
EXCEPTION is valid only for generator-owned storage and marks its still-owned
`gi_exc_state.exc_value` even while the enclosing generator is untracked during
deallocation. The walker derives the generator by frame containment, requiring
no code/map read. Takeover preserves this generator anchor; its bit is removed
before exception close.

The code pin is fixed-last through every home/map read. Only with remaining=0
may its header bit be cleared before code close. The record stays linked with
its result across code deallocation, which can close arbitrary mortal constants
and run Python. Once code is detached, the walker performs no geometry/map
lookup. Heap generators instead retain their code owner and explicitly disarm
that external record bit. Reservation stays
with frame allocator state and is released after this record is popped; scanners
must never infer roots from reservation bytes. Capacity/window validity is
certified at admission. M2 starts with a NULL/full-range shadow implementation
and explicit lifetime assertions; M4 changes only the agreed map iterator.

Proposed functions:

```c
void _PyThreadState_PushTransitionRoots(PyThreadState *, _PyTransitionRoots *);
void _PyThreadState_PopTransitionRoots(PyThreadState *, _PyTransitionRoots *);
int _PyGC_TraceVisitTransitionRoots(PyThreadState *, visitproc, void *);
```

Push/pop are LIFO, callback-free commits; pop asserts no remaining described
owners. The allocator owns the reservation from
first frame unlink through pop; nested growth may append chunks but cannot reset
that saved frontier or free/recycle the containing frame/prefix chunk. Debug push
checks containment and the same reservation state after callbacks. The frames
lane supplies these checks with this consumer; no unused API lands ahead of it.
A record is initialized completely before link. Only authorized entries
are read. Advancing remaining/clearing header bit/removing result authority
precedes close or transfer, so a reentrant collector cannot resurrect the old
edge as a live root. Future prefix adapters install an item before advancing
their end and advance begin before close; an unfinished prefix is not an
eight-slot detached batch. Nested callbacks push independent records.

`_PyGC_TraceVisitFrames` (or a sibling called beside it) marks the complete chain
once per thread. `_PyGC_TraceVisitTransitionRoots` is for external marking, never for
`subtract_refs`. Heap edges moved into a record disappear from that heap frame's
edge authority before publication. Active overlap arguments disappear from the
caller map when child ownership commits. Header globals/builtins are marking
views held by function/mapping authority; do not invent extra subtraction edges.

For GIL cyclic GC, counted external owners already survive subtraction because
their references are absent from container traversal. Explicit chain enumeration
also gives the tracing verifier precise roots. FT deferred treatment must be
specified for any remaining value-tag contracts; strong typed pins are counted.
Never add a visitor that blindly routes every transition root through heap
edge subtraction.

## Mutation and exit obligations

First real consumer handshake (frames/GC/coordinator accepted): thread retirement
uses a result-aware by-value entry; the shared generator clear helper spans
frame and exception cleanup for evaluator completion and direct close/deallocation.
No-result unwind/close callers supply NULL. The
result-aware entry returns the same owner; it must not publish to the caller's
destination before retirement callbacks, which may mutate that destination.
Only a reached noinline continuation allocates/links the record. Fast scalar,
immortal and proven non-last closes stay in the fast function. Count the extra
branch and by-value argument/return traffic; passing a result pointer that forces
hot C-local spills is not a free substitute.

The first may-reenter close can be the detached `frame_obj` association, whose
deallocator clears trace/extra-locals owners, before any value sweep starts.
Other entry points are a last value, `f_locals`, or function pin. The continuation
must resume at that exact stage/cursor, keeping all other header and result
owners registered through the final close. The code pin is fixed-last after the
final map read, while result registration still spans its close. Thread storage reservation cannot end until retirement
and nested callbacks finish. Heap closing paths need the same unique-owner
partition when the generator's public state becomes CLEARED.

Before retiring cursor starts, transfer an externally retained frame into its
separate heap authority using frames lane's takeover interface. Thereafter local
proxy writes target that heap frame, not retired private storage. Active local
overwrites install the new binding before closing the displaced owner, and
reload the current result destination after callbacks. No cursor cache authorizes
ignoring observer writes behind the cursor. Suspended/closing generator storage
has its own admitted phase and exclusion; it is not arena-scanner dependent.

Frame redirect's `retired` heap buffer uses the same initialized-prefix adapter
contract, linked before first close. Any restored values moved between local and
scratch homes transfer map/bitmap authority in the callback-free commit. Helpers
that can reenter during proof preparation must revalidate frame/code/phase and
observer generation before committing, preserving existing behavior.

Required focused evidence: finalizers that recursively call/collect, retain and
mutate departed frames, mutate caller result local, replace function code, raise
or resurrect; failed unpack at every prefix; nested arena growth; heap creation
prefix and constructor allocation failures; suspend/resume/throw/close with
cycles; frame redirect cleanup; inline/immortal/NULL outputs; overlap owner counts.
Root executes these on merged release/debug runtimes and retains results.

M4 must replace conservative arena value/header enumeration and full physical
home interpretation together after all readers/producers pass the census.
M5's rival tracks mortal bits in locals and scratch with identical initialized
value policy; typed header/transition roots remain separate. No stale scratch,
bitmap selection, scanner weakening or hot record cost is part of M1/M2.
C record storage must be in an outlined reached slow function or equivalent
reserved adapter segment; a large automatic variable in hot RETURN does not
satisfy this contract merely because linking happens conditionally.
