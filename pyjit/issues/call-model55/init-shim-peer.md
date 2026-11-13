# Constructor shim: frames design peer

2026-11-12 design GO for [init-shim-proposal.md](init-shim-proposal.md), subject
to root's duplicate-policy choice and source post-review. Duplicate-then-detach
retains caller observation and independent counted edges until commit; consuming
caller aliases while their heap traversal still reports the old words is unsafe.
A move-and-NULL alternative needs its observer/redirect proof before selection.

Keep shim and partial child stages nested through all binder/provider releases.
The shim is a NULL-function static executable: its stage covers initialized
homes plus code, not an invented function pin. On failure its self home closes
while the shim stage is still linked; then callback-free handoff reaches M2
before reservation pop. Duplicate C workspaces retain the native policy and are
not additional heap-container edges. Pin the cached init function before
instance allocation and select the executing code independently for the checked
child allocation.

Current generated CALL_ALLOC_AND_ENTER_INIT sets value_temps=NULL and initializes
ir_displaced=NULL. `_PUSH_FRAME` directly publishes the child, allowing the
child→shim→caller predecessor chain; it has no DISPATCH_INLINED bridge call.
DTRACE/LLTRACE follow publication. Thus a checked generation invariant can
protect the callback-free staged-pop→actual-publication gap. Do not assume the
invariant if future generator changes insert workspace/owner cleanup there.

Use a current capacity proof for the unchecked static shim and checked
allocation for mutable child geometry. Existing call refresh/revalidation must
cover callback-time caller changes; do not rerun tp_new or cache a parent-window
pointer across callbacks. Preserve recursion and destination publication order.
InitCleanup phases and complete alias/clobber geometry remain M4 gates.

The helper can be inserted before call_transaction_overlap_python, after the
exported EX constructor. Frames' concurrent provider increment changes the
shared stage init/disarm helpers and distant evaluator/provider regions, so root
should integrate this later as a small reviewed hunk. No candidate code or
native validation ran for this review.
