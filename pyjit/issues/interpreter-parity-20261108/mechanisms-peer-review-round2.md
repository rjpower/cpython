# Mechanisms peer review: second round

2026-11-08. Reviewed the revised `mechanisms-proposal.md` against R5 source,
retained warmed IR and the existing-stats audit. The original review remains
historical. This accepts the corrected proposal as a comparison of candidates,
not implementation authorization or a completed semantic proof.

The prior multi-owner concern is addressed: M1 now checks each owner
immediately before its ordered close, and sends the first possible deallocation
and all remaining owners through the escaping helper. That matches the
sequential pattern in `ir_generator.py:retire_published`; its alias test is
necessary. The proposal also retains result publication, local ordering and
generic failure fallback. The actual dictionary primitive, generated
control-flow proof, and exact ordinary lookup ordering remain to be reviewed
before a runtime implementation is accepted.

The POP reconciliation is sound: `warmed-ir.json` contains ten static POPs,
but one follows pre-loop `truncate_stream`; the first nine loop calls have
POPs, and the tenth has `IR_NO_DST` followed by the loop back edge. The R5
stats interval's nine POP dispatches per loop agrees. M2 therefore has an
upper bound of nine hot POP dispatches, not ten, before observer/result guards;
the pre-loop site does not scale with loop count. Source identity and dynamic
aggregate counts support this account, but the stats interval still does not
measure the cost of a saved dispatch or the share passing future guards.

M1b is a meaningful distinct comparison: its checked temporary destinations
could specialize owner/publication machinery for message and None producers.
It must retain the local snapshot owner increment and prove the operand class
using the actual compiler/generator interface. Existing global handler counts
are consistent with its ten-per-loop opportunities but do not attribute every
execution to a specific site. The proposal correctly holds PC-store removal
for a separate asynchronous-inspection, error and observer proof. Compare
actual uninstrumented GCC before assuming the local-destination path survives.

M2b is a credible narrower boundary experiment: R5's ordinary NOP publishes
its PC before dispatch, and the false branch's hot continuation reaches a NOP.
Retaining the word and passing instrumented targets to the existing wrapper
preserves a route to observer semantics, but the described check is still a
proof obligation: establish current instrumentation, event/branch reporting,
failed instrumentation, asynchronous observation and changed-PC behavior on
both edges after any possible escaping retirement. A boolean's immortal close
on the specific specialized route is not a proof for all future branch forms.
Count/select an explicit branch edge; avoid adding a next-word test to every
dispatch. Ten global NOPs per loop plus the warmed IR support an opportunity,
not a measurement of guard eligibility or savings.

M2's post-child-clear observer predicate and NULL temporary return-home proof
remain open as the proposal states. M3's structural-version and no-callback
probe-prefix invariants also remain open; tests and a full mutation audit must
precede acceptance of any actual diff. Neither is a defect in existing R5 or a
reason to reject the ideas from comparative diagnostics. M1, M1b, M2, M2b,
M3 and fact-carrying call entry overlap in covered work; compare their actual
paths, fallback rates, GCC code and bounded target/control effects separately.
No runtime edits, builds, tests or native measurements were performed.
