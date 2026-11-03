# ROOTS2b: final stress100k triage

Coordinator checkpoint, 2026-10-29. Source `ce2ac8991ee`, frozen full plan
`7d40ccb2`. The final full is terminal FAIL: receipt `64448666`, native exit2,
host exit1, 50,722 reported tests and3,418 skips. It has an interpreter
assertion, a test_gc timeout and HTTP ENV_CHANGED. No qualified aggregate root
counter result exists because the timed-out worker has no terminal witness.
Source/native guards remain unchanged after cleanup. Preserve original logs,
helpers, options and products; do not retry or relabel old receipts.

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

The existing contract matters: note11 describes stress as collecting every
Nth inline allocation, and test_gc already marks two byte-threshold tests
ineligible under stress because it replaces that trigger. Enabled/threshold0
still gate collection. This is evidence for a deliberate diagnostic override,
not permission to skip this interpreter test or change production scheduling.

At15:14 PDT root grants the independently reviewed V2 diagnostic plan
`d478fceb` / runner `51df97f7`: original-stress, original-off, window-stress,
window-off, exactly once each as separate guarded commands. Original regrtest
FAIL stays native2; copied unittest FAIL stays native1; none is a qualification
PASS. Normal-mode failures or unexpected state/error/skip stop the sequence.
The copied window observes successful root phases charged to the current
interpreter, not target finalizer completion. Its getters/imports perturb
allocation timing. The bound worker seed is991443983 with explicit
--no-randomize; new hashseed0 does not replay the full's unknown randomized
hash seed or process history. No scheduler change or exclusion is chosen.

### Actual outcomes and coordinator disposition,15:24 PDT

The four once-only diagnostics are terminal; independent review clears their
actual scope (`four-diagnostic-summary.json`, bb50b7d3).

| Diagnostic | Actual result | Successful root phases charged in copied window |
|---|---|---|
| Unchanged original, stress100000 | FAIL/native2, one case/zero skips | Not observed |
| Unchanged original, stress off | PASS/native0, one case/zero skips | Not observed |
| Copied window, stress100000 | FAIL/native1, interpreter remains in[0,1] | 10→10 |
| Copied window, stress off | PASS/native0, interpreter absent from[0] | 16→17 |

The first original failure's diagnostic adapter incorrectly required a
suite-result snapshot that regrtest omits when the suite raises. Preserve its
INVALID adapter receipt86008c1a and nativeFAIL2; offline supplemented54cbe8
validates the exact failure from paired start/end and scheduled/returned
evidence. The original was not rerun. The narrow validator correction permits
only the remaining three commands and changes no full-suite validator.

Root keeps the documented stress override and production scheduler unchanged.
These controls support trigger-dependent test eligibility, not demonstrated
retention after collection. The copied counters have the observer and
interpreter-scope limits above. Root reviews the exact two-line stress
eligibility annotation for this test, matching existing byte-trigger tests;
the body remains unchanged. Application and once-only normal/eligibility
checks wait for the quiet timing window to finish. No broad filter, stress
pause or new hot-path state is introduced. Original fullFAIL remains; HTTP
and the trashcan timeout still require separate disposition.

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

### Actual bounded HTTP diagnosis,15:35–15:40 PDT

The unchanged whole module reproduces both closed-fp HTTPResponse unraisables:
103 tests/eight skips, ENV_CHANGED/native3, receiptc0ca71ff. Paired actual
driver/worker and source/native guards pass; its93 charged collections have
zero fallback/unpinned. It remains a diagnostic environment failure.

The separate ordinary-Python observer reaches its unchanged180s worker
deadline: native2, receipt4ae5e6f9, incomplete worker pairing and no qualified
root aggregate. Preserve all748 partial events and the timeout trace; do not
retry it. One error associates response/fp tokens86/87 with the colorized
logging test_err creation: close entry sees fp open, flush entry sees it
closed, then ValueError. This does not identify the buffer closer, establish
baseline completion ordering or provide complete module coverage. Observer
frames, weak tokens, allocations and event I/O change scheduling; the timeout
is not evidence of a baseline timeout.

Before targets, root source review removed gc.get_referrers from the observer:
in this fork it invokes snapshot(), which explicitly collects. The unexecuted
predecessor is preserved; executed observer has no such introspection or extra
explicit collection. This public-Python diagnostic is a distinct safer scope;
old held forced C/Queue2 probes were not retried. The next source-derived
control is the existing two logging classes/four cases against source-bound
primary052 and ce, once each if the primary products/stdlib can be established.
No native comparison is granted yet; no inherited cause is assumed merely
because HTTP source matches. Eligibility patch stays unapplied while ce guards
are needed for this comparison.

## GC timeout: distinguish queue progress from deadlock

The20-minute test_gc worker timeout is in test_trashcan_threads. Main and two
producer threads appear in finalizer paths that release the GIL. Producers
stop only after main returns from the with-body and requests cleanup;
run_queue_ex currently drains until empty. Continued producer work could keep
main draining before it requests shutdown. This is a liveness hypothesis,
not a demonstrated native deadlock or established shared HTTP cause. The
worker's missing end/returned/suite witnesses correctly invalidate the full
aggregate. The held Queue2 probes and implementation remain held.

## Immediate sequence

1. Preserve the terminal failure closure and six valid earlier profiles.
2. Execute/review the four granted interpreter diagnostics; keep native/source
   frozen and make the stress-contract ruling from actual results.
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
