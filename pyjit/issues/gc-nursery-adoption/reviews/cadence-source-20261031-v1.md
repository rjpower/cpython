# Independent exact cadence source review — 2026-10-31 v1

Disposition: **ACCEPT exact source for separately reviewed native feedback. No source blocker found.** This is neither native activation nor cadence performance, qualification, or landing acceptance. All collection/fault/resource observations on this source remain unexecuted.

## Exact source and inputs

Reviewed clean isolated `build/worktrees/nursery-adoption` HEAD `c8cc7718ef3037500aa3902ba5c31acc2b8d7c60`, parent `ef0af95904b87d888eee4a98b6a745835ff95085`. The frozen capsule is `build/nursery-cadence-source/capsule.json`, SHA `648cf340d6d9582d5a18d520ce1ebbee21ee662a4a36a2a2b47bd63689eda30b`; exact binary diff SHA `3aa1966e523223565a337c98af9780a721b25d5e150cfdabe68e34ad5ca868ff`; handoff SHA `7969f29b990e45cbc03d9784d5e6f67cff332c55bfce61aecd973ee0666f23bd`. The seven changed source/document paths and all capsule artifacts match their hashes. The accepted design and earlier causal plan/launch/completion remain unchanged. No allocator, constructor, barrier, ledger-retirement, or runtime-policy switch was added.

I read the entire seven-file diff, relevant surrounding commit/dispatch/queue/mark code, all allocation-probe/deadline consumers, and every focus child and nominated negative patch. Source freeze preceded the 23:12:29 checkpoint. The original overall 04:18:11 UTC deadline remains unchanged.

## Runtime contract review

M/H are the only new state fields; P remains the existing allocation/retry probe. New addition saturates and halfway arithmetic avoids an overflowing increment. Init and fini establish inactive sentinels; threshold configuration arms relative deadlines; successful majors retain the existing `max(threshold, live_bytes)` budget and reset debt. Successful minors, including zero-death passes, close H while preserving D/M. Both transitions occur only at the existing `!failed` lifecycle commit. Mark and preparation failures preserve M/H and do not reset debt; a future finite retry changes P only. No statistics, public death count, accepted-entry return, or queue-completion count is misused as success.

The shared selector applies due-major priority and the current main/GIL/mimalloc capability guard. An empty N0 halfway probe retires H without a mark, and later admission cannot reopen it. Both N0 dispatch paths retain D≥P before selecting, so queue-only breaker work cannot bypass an OOM retry. Stress count advancement/reset remains ahead of the collecting/sweeping guard; only eligible Nth inline opportunities select kinds. The breaker drains queued work without an additional stress P=0 transaction. Threshold zero suppresses automatic selection, while explicit minors/admission remain available.

The new `collect_inline()` is explicitly precise and preserves the previous no-Python collection/queue-scheduling body. Full/emergency/injected C entries still pass major directly; only automatic dispatch chooses cadence kinds. No opaque-frame exception or audit-tool change was introduced. Existing queue ownership, full explicit entries, and bounded lifecycle follow-up logic remain intact. Linked-image precision is still a native-build obligation.

Enable rearms only an actual disabled→enabled transition; a no-op enable preserves retry suppression. Disable itself changes no deadlines. Stress changes rearm stored deadlines, and threshold1/2 retain their storage behavior. Start-callback configuration is used by the entered transaction's commit; later stop/queue configuration is not restored over. Nested rejected collection cannot consume H. The state getter captures D/P/M/H/pending before building its result under the collecting guard, then restores its prior probe. Allocating observation may increase D or admit young objects; the snapshot is not a policy setter or an exact post-allocation debt report.

## Child and negative scope

All twenty previous NurseryTests IDs remain; seven are added. The focused supported-debug roster is thirteen parent methods and twenty-two fresh children, with the KILL child omitted on release. I independently reconstructed each complete child from the actual method AST, f-string values, shared preludes, mode/stress checks, and cleanup. All twenty-two strings match the frozen child files/hashes and compile with host Python. The unchanged settrace control is correctly treated as direct `assert_python_ok`, not as a `_queue_child` invocation. Future telemetry must preserve that distinct startup/flag/seed path.

The source contains actual-generation, fixed-budget, young-death/live-payload, old-cycle, zero-death, empty-retirement/no-reopening, due-major, threshold-zero, no-op-enable, both stress-kind, fault-consumption/backoff, and callback-order assertions. These are pending witnesses, not successful behavior inferred from compiling templates. Existing failed-major promotion/debt tests remain unchanged outside the narrow focus; later coherent GC feedback must preserve their coverage. Dynamic stress assertions do not supply per-allocation telemetry, and inline no-Python paths do not imply Python callback execution.

The three nominated patch files apply cleanly to exact c8cc and match their capsule bindings:

1. Force only automatic minor selections to major: the OFF sequence child must fail the named `automatic kinds` assertion. Manual collection remains unchanged.
2. Add RETRY_BYTES to M after every successful minor, including repeated explicit minors: the frozen finite-major child must exhaust its original bound and fail `manual minors starved the automatic major`. The early debt oracle cannot substitute.
3. Close H after start callback but before a young mark: minor trace-fault stage3 must fire and fail `failed attempt consumed minor opportunity` at the ordinary outside-callback assertion.

One factual labeling correction belongs in future metadata: trace stage1 is the diagnostic-ledger merge, stage2 is live-object inventory, and stage3 is early protected heap/domain preflight before page inventory (`gc_trace.c:3609`). The handoff calls stage3 an inventory fault. This label does not change the actual patch, selected child, fault-consumption check, or valid premature-H causal test; preserve the frozen handoff and use the accurate name in successor records.

No negative was executed here. Each still needs a baseline positive first, exact temporary source/product bindings and fresh products, the nominated actual assertion/child return, root/mode witnesses, and cleanup. Crashes, missing faults/kinds, or a different failure are not expected-negative credit.

## Evidence and limits

Independent source/capsule/child audit is retained at `build/nursery-adoption-review/cadence-source-20261031-v1.py` and its JSON receipt. Source-only precise-root checks independently report zero errors for all three changed C files. This does not replace the linked inline audit, fresh native build, actual focused cases/negatives, coherent full-GC feedback, or elapsed/memory/pause decision.

The elapsed-first design interpretation remains: compare cadence against adopted automatic majors; report historical pre-adoption separately. Hardware instructions are currently unavailable and must be declared so before scores, without a proxy veto or security-setting change. Current source has no performance result. Coordinator helper review and explicit activation remain required before any native command.
