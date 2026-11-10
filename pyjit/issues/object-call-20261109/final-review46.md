# Candidate 46 final elapsed review

**HOLD performance promotion.** Candidate 46 fixes the guarded-storage API violation and serialized-format/fixture fallout, preserves the focused gains, and improves logging/DeltaBlue versus frozen 39. It still has repeatable coroutine, spectral norm and comprehension regressions. The near-flat all29 aggregate does not waive those costs. Retain the checkpoint and its correctness work; do not claim either a fully green test suite or a general speedup.

Identity: `b8e7e1bee6611eeda320ce163cd3ce89323da877`. Qualification and bounded footprint conclusions are in `current-review46.md`: production-equivalent45 API check clean; 46 builds, disassembly tests and stats smoke pass; retained full45 suites have explicit debugger/frame-jump/trace limitations. The census finds no private attribute-site admission loss in its four matched scopes, with larger metadata totals. It is not a dynamic execution census.

## Audited elapsed evidence

Evidence root `E`: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.

Audited both `measure46-all29-*` campaigns, `measure46-regressions-golden39`, and all four long class/keyword campaigns. All seven have successful status, equal valid identity snapshots and clean expected source/native revisions. Checked every timed raw stdout against journal metadata and seven samples, command/row loop counts, within-pair equal workload, CPU 15/JIT-off/hash-zero conditions, alternating role order, arithmetic means and paired geometric summaries. The two all29 campaigns contain 116 invocations each, the eight-case repeat 96, and each long micro 12.

All ratios below are candidate/control elapsed; lower is better.

| Comparison | Geometric mean | Evidence |
| --- | ---: | --- |
| All29 / frozen 39 | 1.003819 | Two pairs per case; screening aggregate |
| All29 / upstream | 1.181202 | Two pairs per case |
| Long class load / frozen 39 | 0.160125 | Six pairs; about 6.25× faster |
| Long class load / upstream | 1.004341 | Six pairs; range 0.969803–1.080403 |
| Long keyword call / frozen 39 | 0.609568 | Six pairs; about 1.64× faster |
| Long keyword call / upstream | 1.052195 | Six pairs; range 1.020751–1.070182 |

Class uses 20,000 loops and keyword 2,000, unchanged bodies and seven timed samples per process. Their timed intervals are now hundreds of milliseconds rather than the original short class intervals. Class's near-one upstream aggregate is useful, but its pair range does not demonstrate tight parity. Keyword remains slower than upstream in every pair.

The six-pair application confirmation is more useful than selecting a favorable aggregate:

| Case / frozen 39 | Geometric mean | Pair range |
| --- | ---: | ---: |
| Logging silent | 0.973238 | 0.964980–0.978156 |
| DeltaBlue | 0.985848 | 0.977600–0.995200 |
| Generators | 1.011833 | 0.996753–1.025312 |
| nbody | 1.010290 | 0.993485–1.040752 |
| Coroutines | 1.057772 | 1.048273–1.068856 |
| Spectral norm | 1.081081 | 1.046814–1.100653 |
| Comprehensions | 1.094809 | 1.084529–1.106845 |
| Logging simple | 0.957587 | 0.900102–1.007994 |

Coroutines, spectral norm and comprehensions are slower in every pair. Logging silent and DeltaBlue improve in every pair. Generator/nbody differences and logging-simple variability should retain their ranges. These paired observations do not isolate the inline annotations from the complete packet or prove that feedback growth caused any regression.

## Instruction evidence accepted and rejected

`instructions46-micro` is rejected: stable identities alone do not override `complete: false`. Its upstream log records load 3.01 exceeding the 3.0 gate and explicitly says no measurement report was written. Preserve it as a rejected attempt.

`instructions46-micro-v2` is independently audited and accepted: equal valid snapshots, successful completion, three roles,  three expected entries,28 successful perf finishes per role, 100% running coverage, four 200/600-loop pairs per entry, exact raw-record hashes/identifiers and recomputable medians/skeleton subtraction.

| Case | Frozen 39 raw / net instructions | Candidate 46 raw / net | Upstream raw / net |
| --- | ---: | ---: | ---: |
| Class load | 657.189 / 565.188 | 168.999 / 77.007 | 144.749 / 63.938 |
| Keyword call | 1429.001 / 1337.000 | 1080.991 / 988.999 | 719.799 / 638.987 |

Raw is the median differenced instructions per iteration before empty-loop subtraction; net subtracts the same role's empty-loop median. It is not a raw whole-process count or isolated opcode count. Candidate/upstream ratios are class1.167531 raw/1.204412 net and keyword 1.501796 raw/1.547760 net. The focused instruction gains over 39 survive the storage split, but considerable keyword work remains despite its much smaller elapsed gap. This does not assign the gap solely to plan validation.

The application PMU campaigns subsequently completed; the audit below supersedes their earlier pending status. Candidate43 evidence remains historical rather than a substitute for46.

## Completed application PMU audit

The final `closing46-profiles-commands.json` records successful completion of micro-v2, upstream stat 18, record 6, matched39 cycles/instructions 18 and matched39 branches 18. Audited all three stat campaigns and the record integrity: successful complete status, equal manifests, expected native revisions, CPU 15/JIT-off/hash-zero checks before/after each hot region, successful FIFO acknowledgements, positive matching loop counts and no invocation errors. Raw perf event counts, runtime and 100% running fields match parsed records; per-loop normalization recomputes. Stat hot intervals span approximately 1.97–2.09 seconds and record intervals 4.94–5.08 seconds.

The six record files are present, retained ELF SHA-256 matches their manifests, executable build IDs appear in recorded hit lists, and all retained lost-record files are empty. This audit checks capture integrity; offline symbol/profile interpretation belongs to the separate profile report. No sample-share inference is needed for the conclusions below.

### Matched candidate 46 / frozen39

Ratios use each role's median event count per hot loop across three rounds. CPI is median cycles divided by median instructions. Cycles/instructions are grouped in one campaign; branches/misses are grouped in a separate campaign.

| Case | Instructions ratio | Cycles ratio | CPI39→46 | CPI ratio |
| --- | ---: | ---: | ---: | ---: |
| Coroutines | 0.999404 | 1.056255 | 0.336103→0.355222 | 1.056884 |
| Spectral norm | 1.000551 | 1.053766 | 0.248037→0.261229 | 1.053185 |
| Comprehensions | 1.000678 | 1.095617 | 0.339608→0.371828 | 1.094875 |

This now **demonstrates higher aggregate measured CPI against39**, with instruction counts changing by less than 0.07%. Candidate cycle-count ranges do not overlap golden ranges in these three-round captures. The elapsed regressions are therefore not explained by a comparable rise in executed instruction count. Exact spectral cycle and elapsed ratios need not coincide across separately captured campaigns.

| Case | Branches ratio | Branch-misses ratio | Miss rate39→46 |
| --- | ---: | ---: | ---: |
| Coroutines | 1.000000 | 1.000012 | 0.375781%→0.375785% |
| Spectral norm | 1.000004 | 1.001743 | 0.004814%→0.004822% |
| Comprehensions | 1.000506 | 1.013194 | 0.299967%→0.303772% |

Per-loop branch counts are nearly unchanged. Branch-miss ranges overlap between roles in every case; the small comprehension miss-rate movement does not establish a cause for its roughly 9.5% cycle increase. There is no evidence here of a substantial increase in branch-miss counts explaining the regressions. Similar counts do not establish identical branch locations, recovery penalties, instruction mixes or memory dependencies.

The evidence narrows the problem to the cost of executing approximately the same amount of work. Front-end delivery, instruction/data-cache behavior, dependencies, register allocation and code placement remain possibilities—not findings. No cache/stall counters or controlled layout ablation in this packet identify one. The three-round 2/1 order split and single-machine scope also limit generalization. Do not translate “higher CPI” into “proven code-layout regression” or prescribe padding.

### Candidate 46 / upstream

| Case | Instructions ratio | Cycles ratio | CPI ratio |
| --- | ---: | ---: | ---: |
| Logging silent | 1.036363 | 0.873153 | 0.842516 |
| Coroutines | 1.383882 | 1.334123 | 0.964044 |
| DeltaBlue | 1.870575 | 1.657183 | 0.885921 |

Relative to upstream, the larger coroutine and DeltaBlue gaps remain dominated by executing more instructions, while average CPI is lower. That is a different comparison from the newly demonstrated CPI regressions versus39. Keep both statements: fixing current CPI costs and removing structural excess work are independent goals.

### Next bounded diagnostic experiment

The new counters strengthen the HOLD decision. First use the retained disassembly/profile mapping to identify hot code common to the three regressions. A useful controlled successor is an **annotation-only ablation**: retain all lifetime/default/storage/magic fixes and remove only the two frame-inline annotations, under the same compiler/configuration. Preserve46 as control, inspect the actual emitted call graph and spills, then pair the three regressions plus logging and keyword controls with cycles/instructions and elapsed measurements. This is a proposed separately reviewed diagnostic, not authorization to edit or run it now.

If ablation changes the gap, it attributes sensitivity to the inline-policy change and its generated-code consequences; it still does not isolate code placement from register allocation or front-end behavior. If it does not, avoid expanding into padding/flag sweeps: compare the hot emitted paths and obtain targeted hardware evidence for a specific next hypothesis. Do not roll back correctness fixes or conflate this diagnostic with the reviewed keyword-proof optimization.

## Wrapper delta

The golden PMU wrapper now requires `--repeat`, removes it from the forwarded argument vector and sets the imported controller's `REPEAT` before its parser runs. Its default cases are exactly coroutines/spectral_norm/comprehensions. These changes correctly remove the old-repeat/default-five-case hazards; snapshot, interval, events and validators are unchanged. Approve the CLI delta. With those three cases each stat group schedules 18 invocations. Counts remain normalized per hot loop, and three rounds still produce a 2/1 role-order split.

## Five next priorities

1. **Test a specific cause for the measured CPI regressions.** Matched counters establish higher CPI with nearly unchanged instruction and branch counts. Review the annotation-only ablation above against retained hot-code evidence; do not prescribe padding, tags or global compiler flags before attribution. Keep failed captures excluded.
2. **Run the reviewed keyword-plan invariant experiment as a separate successor.** Keep immutable destination proofs at construction, debug assertions at use, and every mutable name/default/provider/observation guard. Actual instruction/elapsed benefit—not removed comparisons alone—decides retention. Preserve 46's sealed evidence.
3. **Census DeltaBlue's remaining lookup routes.** Despite its small gain over 39, upstream gaps remain broad. Match hot sites to generic/type-object/method/shape paths before one bounded cache extension; handler histograms are not dynamic weights.
4. **Complete the existing owning-generator entry proof and eligible-hit census.** Keep its scope narrow, existing MISS/ERROR/COMPLETE outcomes and ordinary return-publication semantics where their preconditions hold. No new general transaction framework or additional entry shapes. Implementation still requires review of the actual ownership/observation contract.
5. **Make promotion's outcome explicit after evidence closes.** Either resolve material regressions or document the checkpoint as a correctness/architecture candidate with measured performance limitations. Preserve strict API/storage and default/lifetime fixes even if a performance component is rejected. Known full-suite limitations remain named; micro wins do not turn them green.

This review performed source inspection and lightweight host parsing only. No runtime edits, builds, native workloads, helper modifications or new perf captures were executed.
