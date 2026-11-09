# Forward same-caught PRE peer review

**Accept the first within-except increment with one source-proof condition.**
The committed36 diagnostic records a real LINE stop at pc15 and a target at
pc19 for `test_jump_within_except_block`. `PUSH_EXC_INFO` at pc9 establishes
the caught root at scratch home1. The normal 15→16→17→18→19 path uses CALL
windows starting at home2, so it does not read or replace that root. CALL18's
exception edge goes to pc25→26→27 and cannot reach pc19. The sole source event
in this code is before the caught block. The proposal's same positive context,
origin-at-both-rows, actual exception-value, target ALL-demand subset, empty
save columns, and fresh PRE phase gates fit the current `RegisterResumePlan`.
They must remain executable checks; the diagnostic does not itself export
the computed origin or demand matrices.

The proposed shared forward slice must treat edge phase explicitly. Current
`register_caught_slice()` checks an edge's unwind floor and PC direction, but
does not inspect `RegisterResumeEdge.phase`. A floor check alone is not a proof
that an ordinary PRE error and a completed source error carry the same roots.
For this first increment, require every edge in the induced current→target
slice to be normal (`floor < 0`, `phase >= 0`), or add an independently reviewed
phase-aware survival proof. The actual pc15→19 witness meets the normal-edge
condition. Keep the old backward slice's dominance and behavior unchanged.

The retained diagnostic distinguishes the deferred cases. Both finally
examples have an active re-raise payload at home2 beyond the existing
saved-previous origin, and one requested source line maps to distinct normal
and exceptional code regions. The two except-arm requests start at pc18 and
target pc28, but their normal flow exits through `POP_EXCEPT`; no current→target
normal path exists. No lexical-arm prohibition follows. These four cases must
not gain the new permission from the within-except proof.

This is source/design admission only. The original trace test, an exception
identity test inside a distinct callback exception, refusal/unchanged-state
controls, backward-caught and NULL PRE regressions, and root-owned native gates
remain required before an implementation result can be accepted.
