# Source26 context postreview

Approved as a bounded engineering checkpoint. This is not full behavior or
performance admission. All runtime and tests remained frozen during this review.

The actual source uses one canonical raw EH reader for construction and cold
analysis. The request-owned sparse graph covers real normal successors,
pre-success exception ingress and post-outcome source-callback errors. Backward
owner requirements preserve that phase separation. Saved-previous exception
origins follow actual physical PUSH/POP/COPY/SWAP roles; unrelated transports
lose the proof. Same-caught plans preserve their checked owners through a bounded
no-reentry slice. Every plan uses detach-all, commit continuation/generation, then
close. No target or callback handled-exception link is written.

Coordinator receipts `ownership26-small-focus-checks.json` and the release/debug
small-focus logs show six modules and 131 tests passing in each build, including
all three new protected inherited/same-caught/refusal methods and existing NULL,
FOR, identity, source-error, entry and completion coverage. Both builds import
117 modules. The corrected profiling selector passes 28 tests per build. The
50-row completion payload probes have zero mismatches; their older failing
receipts remain evidence. These are executed results, not claims inferred from
source review.

The wider receipt retains four actual failing modules: monitoring, pdb,
peepholer and sys_settrace. The obsolete profiling selector was an orchestration
error, separately corrected. In sys_settrace, 65 errors remain owner/context/
suspension refusals; 21 failures concern refusal diagnostics, and one is the
known class-init LINE epoch duplicate. The earlier five extra implicit-None
exception-event failures are absent. No failure is waived by the targeted pass.

Current explicit gaps include source-only protected destinations, current
surplus-owner retirement at ordinary observed boundaries, forward same-caught
moves, region/loop exits, comprehension saved-local restoration and suspension.
Caught exits need a resolver for the actual target exception restoration owner
through callback frames. The matched upstream caught-callback probe mutates the
callback exception incorrectly; copying that behavior would not satisfy our
preservation contract.
