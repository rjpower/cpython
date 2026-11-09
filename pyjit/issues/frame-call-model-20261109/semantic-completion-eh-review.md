# Completion EH and throw-target source review

Source-only review after semantic bootstrap failures; no native result claimed.
The earlier assumption that one EH floor proves both outcomes is insufficient.
An anchor below the exception floor can be live, or merely part of the
frontend's preserved stack shape. Those cases need different proofs.

Generator expressions construct their initial iterator before RETURN_GENERATOR
(codegen_comprehension), while codegen_wrap_in_stopiteration_handler inserts
SETUP_CLEANUP only after RETURN_GENERATOR and its initial input pop. Therefore
the PEP479 handler can preserve iterator/cursor at B/B+1 even though its only
operations are STOPITERATION_ERROR and RERAISE1. After exhaustion these homes
may be NULL without losing a semantically read owner: generated RERAISE reads
only its exception and lasti window; nimplicit anchors are liveness inputs,
not additional runtime object reads. Preserve the PEP479 handler if this is
the actual handler. Skipping to an outer handler could turn a StopIteration
raised by a POST monitoring callback into successful generator termination.

This is not general permission to discard handler anchors. POP_ITER/POP_TOP
use PyValue_CLOSE, whose helper asserts non-NULL. Saved outer locals restored
by STORE_FAST_MAYBE_NULL require their actual saved values, including genuinely
unbound NULL values; replacing a bound saved owner with NULL changes behavior.
Inlined comprehension cleanup does SWAP2/POP_TOP, restores saved locals and
reraises. Retain that restoration path whenever its owners remain live.
Actual handler input/definition/implicit-use census must establish which
retired cells are never semantically read before definition or unwind.

Do not select a POST handler solely from the first real continuation. POP_BLOCK
is turned into a zero-word NOP retaining the old handler before flowgraph's
exception stack is popped. A later real instruction can already belong to an
outer scope, while completion callbacks still require in-scope restoration or
PEP479 handling. If a distinct POST recipe is necessary, derive it from the
actual completion ownership boundary and preserve enclosing handlers.

CLEANUP_THROW's successful path must be a semantic jump. The present frontend
can reach shared END_SEND through JUMP_BACKWARD_NO_INTERRUPT; simply deleting
END_SEND leaves completion in a mixed input/post-state jump. The clean repair
is ADDOP_JUMP(CLEANUP_THROW, exit), unchanged frontend input/output shapes,
A completed destination/B four-input base/C-D absolute real continuation.
Reject/reraise runs first; the successful top-level completion marker is
terminal. Analyzer jump metadata and generator jump_always must recognize
that marker; its new argument requires moving no-argument pin 7 to a free
has-argument opcode, with the coordinator's cache-format update. Compiler
matching then proves the target's END_SEND recipient and removes the cleanup
word and intervening execution jump without adding aliases or hot phase state.

Coordinator diagnostic6 confirms the relevant genexpr handler: intrinsic reads
slot6; RERAISE reads lasti5/exception6 and carries slots3/4 only as implicit
unwind anchors. This supports retaining the PEP479 recipe with validated dead
NULL holes, rather than introducing a POST schema solely for this case. B owns
the required semantic-read/constructor proof. A has implemented the true named
CLEANUP_THROW target, including argument-bearing pin87 under the coordinator's
explicit format64414 reuse exception, terminal marker jump metadata and absolute
CD continuation. Root owns magic/digest/regeneration; no native result yet.
