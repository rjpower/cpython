# ROOTS2b: final stress100k triage

Coordinator checkpoint, 2026-10-29. Source `ce2ac8991ee`, frozen full plan
`7d40ccb2`. The final full is still finishing; its returned failure/ENV_CHANGED
block landing. Preserve original logs, helpers, options, source and native
products through terminal cleanup. Do not retry, add filters or relabel old
receipts. This record is source/evidence analysis, not an execution grant.

The other six profiles pass: release/debug/debug-kill fulls51,132 each;
release/debug stress11 sets3,062 each; ASan16-kill3,976. All have zero
fallback within their exact recorded filters/modes. Tail make/strictstale pass;
Walk and gate3 extras never launch because their all-seven-PASS prerequisite
is unsatisfied. Final landing review and fresh merged gates remain.

## Interpreter test: distinguish missing collection from retention

`test_interpreters.test_api.ListAllTests.test_destroyed_by_gc` fails at305:
interpreter34 remains in[0,34]. Worker3667921 reports179 tests/one failure/
eight skips. The source creates an interpreter in an otherwise unreachable
cycle, then lowers the threshold to1 around list_all(). It does **not** call
gc.collect(). The stress100k worker actually runs modeoff/interval100000.

`Python/gc_live.c:_PyGC_LiveOnThreshold` handles positive stress intervals
first, increments its stress counter, resets alloc_limit to0 and returns
before the ordinary scheduling path. The public threshold change updates
regular threshold/limit. Independent source reads agree this can suppress
the test's intended collection. Whole-module collection totals cannot prove
that the crucial window collected. Do not call this confirmed stale-root
retention, finalizer failure or a Park regression yet.

After terminal cleanup, review/grant the smallest original-selector comparison
and a separate critical-window diagnostic. Preserve the original unmodified
failing selection. Observer/counter allocations can change collection timing;
record their effects and keep diagnostic scope separate from the reproducer.
Choose the public-threshold/stress scheduling contract before a native repair;
do not insert a collection merely to make the assertion pass or skip the test.
A causal old-fail/new-pass control and invariant-specific negative should
determine affected requalification, with no automatic20 campaign.

## HTTP response: separate lifecycle hypotheses

`test_httpservers` worker3667375 returns ENV_CHANGED:103 tests/no failures/
eight skips, with two HTTPResponse unraisables at addresses0x35558e984a0
and0x35558e98340. HTTPResponse.close calls IOBase.close, whose flush invokes
HTTPResponse.flush; self.fp is non-None but already closed. This establishes
that observed state, not the cause. Both tests/modules pass on each of the
three fresh normal/kill full runs. HTTP source is unchanged from primary.

Distinguish explicit/test/connection closure, a missing traverse/mark edge
across collections, and finalizer execution/completion order. Existing raw
witnesses do not give per-object creation/close/thread timelines. A bounded
case-local diagnostic, if authorized after cleanup, must identify actual
response/referent instances and the first close transition. A clean stderr
rerun is not proof. Do not assume a common cause with interpreter scheduling.
If evidence points at the already-held Queue2 completion work, preserve that
handoff and hold; do not retry its probes or implementation via another agent.

## Immediate sequence

1. Let the original profile finish and retain/audit its exact failure closure.
2. Review focused original-selector and diagnostic plans; no native/source
   mutation during the current run. Root owns execution and contract rulings.
3. Fix demonstrated causes with bounded changes; rerun relevant levels based
   on the actual delta. Keep six earlier successes at their true source IDs.
4. Resume tail/extras only under an explicitly reviewed successor plan after
   current failures are resolved/dispositioned; never infer all-seven-PASS.
5. Complete exact-candidate landing review, merge and fresh integration gates.

An optional early wall-time diagnostic may bind the frozen release candidate
that passed its normal full, with these stress failures explicit. It is not
correctness, landing, parity or2× qualification. It may occupy a bounded quiet
window after terminal cleanup/focused diagnosis, while source plans continue;
a ready GC fix or noisy machine must not be delayed for it.
