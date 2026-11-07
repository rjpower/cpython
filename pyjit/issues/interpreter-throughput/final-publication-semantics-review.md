# Attribute retirement semantics: test review

2026-11-06. **ACCEPT** the focused `test_ir` source and the final two-line
handler-check addition in `final-publication-semantics.patch`. No remaining
must-fix source finding. Review only: no test or runtime execution by reviewer.

The factory produces fresh mortal receivers and payloads; passing a receiver
as a local argument would not have tested its retirement at the load. The
fixture checks the compiler's actual destination field for temporary versus
folded-local forms, and warms explicit slot/shape cases. Deterministic
finalizer expectations are guarded by `requires_refcounting`.

Receiver destruction reenters Python, reads the caller's old folded local,
and collects garbage while the result must remain alive. The displaced old
local's finalizer observes the newly stored payload. Events are checked after
execution, so the oracle does not rely on assertions that a finalizer could
swallow. The reference list owns only weakrefs. Callback-local frame objects
do not escape; their reads record booleans, not payload owners. Dropping the
returned result and checking that its weakref is dead catches extra retention.

The final addition checks the expected installed handler after the observed
call as well as before it. Together with destination-field assertions, this
is targeted specialization eligibility/installation evidence. It is not an
exact executed-handler counter or proof that no internal guard fell back.
Actual execution and any stronger coverage claim require the coordinator's
recorded results. The test covers SLOT and SHAPE semantic paths, not every
specialized attribute implementation or a measured speedup.
