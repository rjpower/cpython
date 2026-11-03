# Cadence cost campaign: first failure outcome

Disposition: **INCOMPLETE; no cost acceptance or collector rejection**. The campaign stopped at the first failed retention validation. Preserve all partial durations unscored; this review authorizes no repeat, bound change, workload reset or additional sample. A revised oracle or campaign requires a separately reviewed causal design and explicit root grant.

The exact master is `build/worktrees/nursery-adoption/build/nursery-cadence-cost-native-v1`. Materialized plan SHA-256 is `89c15bf3fda2c0635cdb8d78921493ac97526d66155be5d91f99fa3acf3b086f`; launch is `f90dd8a15e64bf5fbe458e02ca0394388380ac0d14bfcfe55428e77c17bea526`; activation is `9241a53787c854cfdc774ca26ae3c49c2ba11ba2e6cdeb61534920267949e229`. Root confirmed session 29523 exited one. The original window is 00:55:00.325366–01:55:00.325366 UTC on 2026-11-01. It was not reset.

The first rejected row is `score-r0-richards-0-historical`, PID 1217851, historical source e9 and the accepted historical image. Its native child returned zero, without timeout, after one timed call and three unmeasured retention calls with explicit majors. The host failed at the unchanged `paired.normalize` assertion that `retention_bound_passed` must be true:

| Raw post-major observation | First | Second | Third | Final minus first |
| --- | ---: | ---: | ---: | ---: |
| Tracked objects | 12,112 | 12,168 | 12,224 | 112 |
| Live bytes | 3,181,888 | 3,185,088 | 3,188,288 | 6,400 |

The object growth exceeds the fixed 32-object bound; byte growth remains within 16 KiB. The raw retention flag is false. The child is OFF/N0 with threshold 33,554,432, enabled GC and no callbacks. The recorded 60,091,021 ns hot duration remains raw partial evidence, not an accepted campaign score. There is no `measurement.json` for this rejected row and no final `score.json`.

This is not evidence of cadence-specific retention: failure occurred on the historical control. Source gives a concrete mutator explanation requiring a different oracle. `boil/bench/internal/kernels/richards.py:181` links each new task to the previous global `taskWorkArea.taskList`; line 192 installs it as the new head. `Richards.run` resets only the two scheduler counters, not that list. Thus earlier task graphs remain strongly reachable across calls. Each iteration creates six tasks, six task records, eight packets and eight packet-data lists; at two loops this is consistent with the observed 56 additional objects per batch. The source and raw growth establish why a universal steady-object-count expectation is unsuitable for this unchanged control. They do not prove the absence of any unrelated leak or authorize subtracting a guessed allowance from a new score.

Actual process evidence is narrower than the planned 132-process campaign:

- Twelve origin processes completed, with zero timed or retention calls. Their actual native/library/Python traces replay through the accepted parser and match their recorded source/workload/image identities.
- Forty-five scored processes completed validation: five fresh samples for each arm on each of coroutines, generators and comprehensions in round zero. Their original raw records and retention checks remain intact.
- One additional Richards historical process finished natively but was rejected by the host validator. No later planned process ran.

There are 58 actual command receipts, 57 completed rows including the origins, and 46 raw timed calls. The scored children report 138 completed retention calls and 138 explicit majors in total, including the rejected row. These are actual wrapper records, not full-campaign credit. I replayed normalization for all accepted rows and reproduced failure on the first rejected row. All commands match the frozen literal order, environment, cwd and image. The 115 quiet checks all passed; the maximum observed load is 0.7734375 under the fixed 1.5 limit. The final rejected row has a before check, but no successful post-row validation/quiet credit.

The outer supervisor correctly propagates gate return one. Its `failure` field is null because it received a nonzero child return rather than raising its own error; `post_cleanup_verified` is false. The locked failure receipt and traceback explicitly retain the assertion. This is not a successful campaign merely because terminal cleanup succeeded. Launch-result SHA-256 is `e6fa7d0fe916468d5810b9852587afd8afbe48e6f029a2a0a720c7cdb2554217`; terminal-cleanup is `484a642827f2a7abcf98bcdf30d6f7072b5133ac94d0fe48e682cb6c9297ead0`.

All 317 launch-result artifact bindings rehash correctly. The materialized source, configuration, tools, product inventories and native-library inputs still pass the accepted read-only verifier. Before/after/post-cleanup/terminal inventories agree for all three images. Actual inherited admission/two-slot/all-three-image locks and CPU 0 affinity are recorded. Terminal cleanup reports all three targets absent, all three build locks held, and completion before the original deadline. I separately acquired those locks and found no live target. The original held snapshots and prior preparation/native failures remain preserved.

Independent host evidence is `build/nursery-adoption-review/cadence-cost-outcome-20261101-v1.py`, its complete log, and receipt `cadence-cost-outcome-20261101-v1.json`, SHA-256 `431865a4e8fb0a55089609a55cb947742d0ca1ad2d7fddf2b87086058ffcf4fc`. Two reviewer-only lock-label assertions failed before any row replay; both original attempts are retained, and the corrected audit checks the supervisor's actual historical/adopted/cadence lock names. No target, compiler, workload, probe, bound or source was changed during review. No qualification, landing, full-round ratio, memory-envelope or net GC-cost claim follows from this incomplete exposure.
