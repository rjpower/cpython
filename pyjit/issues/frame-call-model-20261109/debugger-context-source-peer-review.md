# Derived context source peer review

Source-only review for native26; native acceptance remains coordinator-owned.
The first liveness draft redeclared handler as both an EH entry and an int in
one block. B repaired it to real_handler; no remaining concrete blocker found.

Verified actual runtime contracts: PUSH_EXC_INFO writes its saved previous
exception in window B, POP_EXCEPT reads explicit A, COPY's new tail is BASE+arg,
and SWAP permutes its two endpoints. Malformed SWAP crossing named locals now
makes the context unavailable and clears origin facts. No inferred transport
creates an owner or context. Parent ambiguity/cycles and repeated producers are
refused; active inherited context is not assumed to mean None.

Ordinary EH incoming requirements join after normal fresh-definition kills;
source callback requirements use each actual post-operation outcome before
its source unwind. Context origins likewise use pre-success ordinary errors
and post-outcome source errors. Canonical EH factoring retains prior marked,
bounded, sorted and nonoverlapping geometry. The actual setter preserves the
same caught node and ancestor saved owners and never writes target exc_info or
callback exception links. Reentry generation and detach-all/commit-before-close
remain in the application path.

The sparse refinement is approved: checked capacity3N+R covers two normal edges,
one ordinary handler and one possible source-error edge per source record.
Forward/reverse worklists replace all-pairs adjacency queries. Source records
are decoded once and indexed by predecessor. Error edges retain their actual
unwind floor, including source callbacks; a kept owner cannot cross that floor.
Named-local role checks apply also to pending-child analysis without NULL facts.
No source event/schema, runtime dispatch or ordinary frame field was added.

B's focused tests and the coordinator's merged compiler/native/debugger gates
remain required. This approval covers the bounded inherited/same-caught rung,
not context exits, suspended frames or target exception-link restoration.
