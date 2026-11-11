# Option 2 implementation review

Disposition: **source approved for release/debug qualification and measurement**.
Reviewed actual source at `0d0dbb5fbc9` and its generated products. No semantic,
ownership or recovery blocker found. Native execution evidence is pending at
this review; this is not final qualification or performance admission.

The new proof matches the complete list/tuple guard, exhaustion and acquisition
bodies, including free-threaded source branches, exact accessor arguments and
tagged cursor increment. It also checks window/destination roles, offset two,
feedback layout, skip geometry, scalar input/output identities and unchanged
passthroughs. A changed producer chain retains ordinary publication. Proof is
independent of handler name and does not broaden the prior scalar proof.

Direct publication is restricted to the final successful window flush, canonical
item and ordinary observer route. Cursor stores first because stack variables
are flushed in stack order, then item reaches the shared canonical destination
transaction. Acquisition precedes replacing a container-aliasing local. The
existing iterator window keeps its independent owner. Local displaced-owner
finalization and redirect/error handling remain the shared implementation.

Temporary destinations retain the window+2 identity. This is an existing
assembler contract: `regcfg.c` verifies consecutive window outputs and maps
temporary results to their window slot; the old IR_MOVE_TO_DST also depended
on it by doing nothing for temporary destinations. The new debug assertion
checks that invariant, and the store writes the same required home.

The final mover is suppressed only after an actual destination store entered
`dst_stores`; canonical cursor marking alone cannot suppress it. The direct
store cannot run during exhaustion or earlier flushes. Exhaustion leaves the
destination binding intact. Observed deferred transactions retain their old
physical home and final mover. No later uop can read the omitted item home
after the successful final flush.

Generated changes affect only list/tuple cursor/item publication and final move:
canonical cursor publication, temporary identity assertion and direct canonical
recipient store replace generic cursor/item publication plus move. Generic,
virtual, range and instrumented paths retain their publication.

The new generator positives and mutations cover source/geometry failure,
ordering, single recipient publication and untouched other routes. Runtime
tests cover canonical/object values, exhausted bindings, aliased container local,
unpacking temporaries, finalizer reentry, mutation/subclass fallback, break/error
and monitoring enable/disable. Root must execute these and existing generator/
monitoring/iteration checks on the candidate, compile release/debug and inspect
native normalization/window traffic before admission. This reviewer performed
read-only source/generated review and no CPU campaign.
