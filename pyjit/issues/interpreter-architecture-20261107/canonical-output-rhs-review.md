# Canonical producer expression refinement

2026-11-07. Proposal refinement only; no runtime edits. **Accept
`attr = IR_CANONICAL_HOME_DUP(found);` in place of the statement-form
`IR_CANONICAL_HOME_DUP(attr, found)` in the
[bounded proposal](canonical-output-boundary-proposal.md).**

The ordinary assignment remains visible to existing `assigns_to`, format,
ownership and R4 single-assignment checks. A statement intrinsic would need a
second way to tell those consumers that it assigns the output. The RHS form
therefore preserves the smaller interface.

Recognize only the exact direct, top-level assignment with one simple
identifier argument. Record its canonical-source fact on the existing checked
publication record for that result and producer. The first temporary flush
may consume that fact; the final folded-local publication and all other
outputs retain their existing behavior. No mutable stack cache or C pointer
provenance parser is needed.

The expansion may use `(assert(_PyValue_IsHome(found)), PyValue_XDUP(found))`
in debug and `PyValue_XDUP(found)` in release. Requiring a simple identifier
avoids double evaluation of an expression with side effects. The assertion is
fatal and adds no recoverable error or ownership path. Reviewed writer/home
invariants remain the premise for the source fact; the RHS syntax does not
prove where `found` came from. R5 still requires its own implementation,
generated-code review, native validation and measurement.
