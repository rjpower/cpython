# Independent halfway-cadence design review — 2026-10-31 v1

Disposition: **ACCEPT for bounded source preparation and subsequent exact-source review. No design blocker found.** This review grants neither native execution nor performance/qualification acceptance. The coordinator still owns the source grant and literal feedback/measurement manifests. The frozen design was not changed.

## Inputs and concrete source checks

Reviewed `build/worktrees/nursery-adoption/build/nursery-cadence-design/design.md`, SHA `e78445a86de4bb59ed084185c176988f4ea52e952b64a13420341193a647a1c2`; its receipt SHA is `7a25814434dde2e086f85d82276dafa60b6f340afa36f6968473426d3e5dc83d`. Adopted worktree HEAD remains clean at `ef0af95904b87d888eee4a98b6a745835ff95085`. All seven named source/document hashes, boundary-notes hash `712c701f1ae4740d1104ce41b1c1cdcb63062a0ce7733b5bbb828f080b09606b`, and three preserved causal-plan inputs match. The adopted architecture and accepted manual-minor/debt controls remain the starting point.

Inspected the actual `collect()` success boundary and failures, `collect_from_entry()` follow-up logic, `can_collect()`, both automatic dispatches, stress configuration, init/fini, public enable/disable and explicit-generation entry, threshold setter, allocator probe caller, and read-only state getter. Also checked the existing finite-major test and full inline call sites in `pyvalue.c` and `shapeobject.c`.

## Why this is a coherent increment

The two new deadlines have distinct jobs. M preserves the major budget while P remains movable retry machinery; H identifies one minor opportunity without a separate enabled flag. Threshold changes can intentionally establish a fresh budget relative to existing debt. A failed attempt cannot quietly move M or consume H. Saturated additions and overflow-safe halfway arithmetic are appropriate within the declared scope; the proposal does not require allocator fast-path changes or a global counter rewrite.

The successful commit belongs under the existing `!failed` condition near `gc_live.c:1943`, after marking/inventory and lifecycle preparation. Existing statistics can advance after preparation failure, an accepted inline entry does not imply success, and queue completions describe lifecycle work. The design correctly rejects all three as cadence success signals. A successful zero-death minor still consumes H. Failed preparation may preserve already-promoted survivors without falsely closing H or resetting debt. Explicit minors and required bounded follow-up passes continue to execute regardless of H.

N0 retains the outer D≥P gate, so a queue-only breaker cannot bypass a deferred OOM retry merely because M is due. Major priority applies once a probe is eligible. A no-young halfway probe retires H without a transaction; subsequent admission does not reopen it. Overshooting H in C can legitimately select the major instead. Unsupported minor contexts retain automatic-major behavior and cannot run an early major merely because the old halfway probe became due.

Stress requires its separate opportunity distinction: only the existing Nth inline event selects a cadence transaction; the breaker drains queues without treating P=0 as another byte-triggered request. Stress bypasses the empty-young hint to exercise a real minor, then requests a major after successful consumption. Failure leaves H open. Threshold zero still suppresses automatic requests. The proposed factoring keeps emergency/injected/full inline entries major and preserves no-Python queue delivery.

Configuration is committed in transaction order. A start callback's threshold/stress/disable effects are explicit; the already-selected kind remains fixed, success uses current configuration, and later stop/queue changes are not overwritten by a stale snapshot. Rejected nested calls consume nothing. Before-init/after-close sentinels, actual disabled→enabled rearming, no-op enable preserving retry, stress-off rearming, and the getter's collecting/probe guard are all required source-review checks, not optional implementation details.

## Witnesses and finite scope

The proposed roster derives correctly: seven new methods contribute 15 children; stress contributes two; finite-major/debt/old-boundary contribute three; two unchanged queue/full-entry controls contribute two. Total: **13 parent methods, 22 ordinary fresh children, plus three separately attributed source-negative children**. Other nursery cases remain later coherent feedback, not implicit credit.

The negatives target different causal claims. Forcing only automatic minors to majors must defeat the actual-generation witness. Adding retry debt after **each successful minor, including repeated explicit minors**, must defeat the frozen finite-major bound at its nominated final assertion rather than an earlier equality check. Prematurely closing H must defeat the actual OOM preserved-opportunity witness. Each needs exact-source review and fresh products. Fault stages must actually fire with the selected kind; failed statistics alone are insufficient.

Debt snapshots need their existing allocation-aware interpretation: preserve D means no reset/lost debt, not exact equality across callbacks or getter allocations. M/H can be compared exactly when no explicit configuration change intervenes. The getter must capture all reported fields consistently before building its allocating result. Useful payload, old/resource reclamation, fault consumption, inline queue behavior, and actual stress kinds remain mandatory witnesses.

The 45-minute source checkpoint with a 15-minute status checkpoint is bounded. Literal cases, child counts, negative nominations, product bindings, and downward-clamped native budgets still need acceptance before any execution. The original overall 04:18:11 UTC deadline is not extended.

The factored inline helper also needs the existing precise-frame contract. `Tools/build/gc_inline_audit.py` rejects an outlined collecting static callee of precise callers unless its frame is precise or an existing justified opaque exception applies. Prefer preserving precision. The seven-file list must not force an inaccurate frame classification: permit only a concretely necessary audit-metadata entry, with its actual frame/lifetime reason independently reviewed, if the existing tool requires one. No speculative audit-tool rewrite is justified.

## Elapsed-first decision and remaining preparation

The proposed cost rule follows current user steering: actual elapsed drives the decision; instruction counts and text size explain it. Larger text or more instructions cannot automatically veto a faster candidate. Memory retention/resource correctness remains mandatory, while bounded peak-RSS growth beyond the stated envelope receives an explicit absolute-MiB/elapsed tradeoff decision. There is no hidden return to a nursery runtime switch if this cadence loses.

Before scores, the measurement manifest must name **adopted automatic-major source as baseline and halfway cadence as candidate for the primary ratio**. The historical pre-adoption arm separately diagnoses adoption cost and its >2% regression rule. The stated 3 arms × 4 workloads × 2 rounds × 5 samples gives 120 sample units; the literal launch must distinguish sample units from any runner's nested native processes. Preserve raw elapsed vectors, fixed order, kinds/pauses, memory, and the no-automatic-repeat/inconclusive rules.

The coordinator reports current `instructions:u` collection is concretely denied (`perf true` rc1, paranoid4). Freeze that unavailable diagnostic and its receipt **before scores**; elapsed, memory, pause, and reclamation evidence can proceed through an independently reviewed helper without instruction vectors. No global perf-security change or instruction-proxy veto is needed. This caveat does not authorize measurement now.

Host receipt: `build/nursery-adoption-review/cadence-design-20261031-v1.json`, SHA `9f26f47da9940395563be06f108d0594d72f5514799ff157a6b4964e84d75ece`. Review performed read-only against the frozen design/source; no runtime edit, build, target invocation, or timing occurred.
