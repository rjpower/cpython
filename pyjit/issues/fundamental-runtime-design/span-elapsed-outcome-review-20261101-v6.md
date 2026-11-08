# Integrated span elapsed/RSS pilot outcome, v6

2026-11-01. **Reject the current integrated span candidate on measured elapsed time.** Correctness feedback remains accepted within its focused scope. This pilot gives no parity, full qualification or landing credit and does not decide all tracing-GC architectures.

Actual source `5a457c738310f9dd762c88743af9042258841c74`, release image `3a2b9aa1ee98390bb2c35734d531d7da78466642ceed4a782a9a07dc30a922b1`; qualified tracing parent image `289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`. The third arm is the exact pinned upstream `eval-nogcse` image recorded in the plan. Plan SHA256 `4fd95b8bed9bbbcdfd3e6ed7bae06edfe97618289454145cea4ae5ca93974359`. Root confirmed supervisor 6706 terminal exit 0 at 05:28:15 UTC; the fixed window began 05:26:52.815728 and ended 06:11:52.815728 UTC.

The predeclared primary requires coroutines candidate/parent ≤0.99 each round; measured ratios are 1.50744 and 1.51074 (50.74% and 51.07% slower). The four-row candidate/parent geometric means are 1.14789 and 1.10257; both fail the ≤1.0 gate. Every coroutine candidate sample exceeds every parent sample within its round, and all ten leave-one-out ratios remain above 1. This is a clear loss, not a borderline gain requiring more samples. No samples or retries are authorized.

Ratios below divide candidate elapsed by the named reference; lower is better. RSS is whole-process maximum in KiB, ordered candidate / tracing parent / pinned upstream.

| Round | Workload | Elapsed / parent | Elapsed / upstream | Peak RSS KiB: C / P / U |
|---|---|---:|---:|---:|
| 0 | coroutines | 1.5074 | 2.4554 | 71,016 / 66,956 / 14,376 |
| 0 | generators | 1.0309 | 1.6920 | 72,200 / 71,824 / 27,512 |
| 0 | comprehensions | 1.1213 | 1.3333 | 86,264 / 86,400 / 17,012 |
| 0 | richards | 0.9963 | 0.5230 | 30,712 / 30,684 / 14,608 |
| 1 | coroutines | 1.5107 | 2.5504 | 71,192 / 66,860 / 14,596 |
| 1 | generators | 1.0316 | 1.6463 | 71,520 / 73,448 / 27,568 |
| 1 | comprehensions | 0.9759 | 1.2486 | 86,928 / 86,204 / 16,944 |
| 1 | richards | 0.9716 | 0.5207 | 30,916 / 30,688 / 14,508 |

The four-row candidate/upstream elapsed geometric means are 1.30466 and 1.28538; parent/upstream means are 1.13658 and 1.16580. Four workloads cannot establish the required all-29 parity result. The generator median regresses about 3.1% in both rounds; comprehension results change direction, and Richards improves modestly. Serial spreads are substantial in some control vectors (candidate generator spread reaches 32.9%). Those limits do not rescue the separated coroutine loss or let another row compensate for it.

Coroutines peak RSS also rises: candidate/parent 1.06064 and 1.06479, candidate/upstream 4.93990 and 4.87750. Across four workloads the geometric means of process-peak RSS ratios are candidate/parent 1.01598 / 1.01307 and candidate/upstream 3.42871 / 3.42953. These are resource diagnostics, not a percentage veto. GNU time records the entire process maximum, including interpreter setup and registered warmup; it does not measure per-loop live storage, slot occupancy or persistent leaks. No new occupancy census was inserted.

Independent audit replayed the accepted schedule, raw normalizer, origin parser and three-runtime source/product/ELF binding checks. All 24 scored fresh processes completed five serial values after the registered warmup: 120 retained durations. All 12 distinct workload/image origin witnesses executed zero scored samples and one separate one-loop warmup. Recorded timestamps match the fixed rotating order without overlap, argv/environment/cwd match the plan, and every RSS scalar matches its original raw file. Three arm medians, all ratios and geometric means were independently recomputed; score serialization matches the unchanged summarizer.

All 243 outer artifact bindings verify. Current source, interpreter/library/extension/tool identities and native inventories agree with their sealed manifests and before/after/terminal inventories. CPU affinity is [0]; all 72 load/policy checks passed, maximum observed one-minute load 0.90039 against the predeclared maximum 4, with CPU0/sibling8 policy unchanged. Actual inner lock receipts bind both machine slots and all three build locks. Gate/inner/outer returned zero; final cleanup records all three targets idle under those locks before deadline, with no failure. Later coordinator-authorized native work is separate from this completed quiet window.

The original v5 attempt remains failed before targets/gates/scores; v6 binds only its reviewed empty-lock inventory correction. No prior scores were appended. Reviewer-only adaptation initially retained two old IR assumptions (maximum load 1.5 and an explicit empty GC-verifier variable); the retained host audit failures were corrected to the already-frozen storage plan (maximum load 4 and exact natural release environment). Neither measurement inputs nor runtime protocols changed; no target or benchmark reran.

This experiment measures the net candidate change, including common allocator lookup effects. It proves no particular allocator, conservative-root, compiler-layout or cache cause. Instruction counters are not required for this elapsed rejection. Preserve the full candidate, correctness evidence and raw vectors for a separately scoped causal question; do not adopt this source or infer composition gains from isolated mechanisms.

Evidence: [independent receipt](../../../build/span-elapsed-pilot-review-20261101-v6/receipt.json), with all raw bindings, per-round ratios, process peaks, sample spreads and leave-one-out results.
