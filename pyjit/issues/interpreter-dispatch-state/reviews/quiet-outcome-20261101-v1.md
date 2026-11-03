# Slot-base elapsed outcome

Decision: **REJECT this increment on the predeclared elapsed primary**. The completed fixed campaign fails the required coroutine improvement in both rounds. No added samples or further timing are authorized by this result. This rejects source `3ddc437679ebbb9f8629c1670655c58776075dfc` as the proposed performance increment; the earlier bounded correctness results remain valid.

| Workload | Round 0 candidate / parent | Round 1 candidate / parent |
| --- | ---: | ---: |
| Coroutines, primary | 1.119789 | 1.109957 |
| Generators | 1.261330 | 1.267107 |
| Unpack sequence | 1.477540 | 1.527735 |
| Spectral norm | 1.110383 | 1.088041 |

Ratios above one are slower. The primary is about 12.0% and 11.0% slower; it fails both the 1% improvement and descriptive adequacy rules. Each control also exceeds the declared 2% regression threshold in both rounds. These are observations on the four selected workloads under the frozen environment, not general speed estimates. Several control vectors have substantial spread, which remains preserved rather than filtered. Static frame/IP traffic and text growth are diagnostic; this review attributes no measured change to caches, register allocation, or a particular native instruction sequence.

The master is `build/interpreter-slot-base-quiet-v1`. Plan SHA-256 is `7682690de1669e4f2ce99afeb729eb844304d2ef80fdeba0c901bff9b7c3799e`; activation is `99f4be9e34916d07dbe64c09983245cbacde177affa2564148b346a730231c2c`; launch result is `935c45613bb737a9f04fc9359ba98b0b9ce117d841b563412efa4717f62a580d`; score is `f3c06723bff76bcd31ed975ae096aadb76ee2be526ffab57fe8c65a47ae08eb1`. Root confirmed session 30844 exited zero. The fixed window began at 00:39:23.851891 UTC on 2026-11-01 and ends at 01:24:23.851891; all recorded work, final evidence and cleanup finished before it.

Independent host replay rehashed all 191 launch-result artifacts, exact reviews, source/product/tool/native-library bindings and current inventories. Before/after/post-cleanup/terminal inventories agree for both images. The e9 baseline and 3dd candidate remain the matched images from the accepted preparation. The actual outer command, environment, inherited admission/two-slot locks, both image locks and CPU 0 affinity are recorded. Gate return is zero; post-cleanup verification is true; failure is null. Terminal cleanup records both process lists empty and no deadline overrun. A separate current observation acquired both image locks and found no matching target processes.

All eight actual zero-sample origin processes were replayed through the unchanged source/library parser, including each workload's source, its own json source/extension, native program event and library trace. They precede all sixteen scored processes in the exact declared order. Origins use one unmeasured one-loop warmup; scoring processes use the registered-loop warmup. The original raw stdout vectors contain exactly eighty positive finite durations. Every child returned zero without timeout under the exact OFF/N0 environment. I independently recomputed normalization, medians, spread, MADs and every leave-one-out ratio from those raw durations, matching the sealed score. The forty-eight before/after quiet checks all passed; maximum observed load was 0.70361328125 under the fixed limit 1.5. No counter probe or replacement samples ran.

Independent evidence is `build/interpreter-dispatch-state-review/quiet-outcome-20261101-v1.py`, its log, and receipt `quiet-outcome-20261101-v1.json`, SHA-256 `85f7bca35d369723717ca1f066171ebad0ea576de8388bb6d4c18a3ec1f60270`. The audit executed no target, compiler, workload or timing call. This result supplies no qualification, landing, or joint GC/IR performance credit. Any further source work requires a new concrete hypothesis and its own bounded decision.
