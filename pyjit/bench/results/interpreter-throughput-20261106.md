# Calls, frame initialization and attribute publication

**Completed and independently reviewed, 2026-11-07.** Runtime `7780e32e0e2`, following `ba37cc10058` and `8a8fdc91368`, starts from `6a0e4c12c17`. This batch uses GCC only and changes neither the PyValue encoding nor the register instruction format. Correctness passes; the interpreter remains slower than upstream overall, and the elapsed results do not establish a repeatable broad speedup from this batch.

## Integrated changes

| Commit | Change | Review |
| --- | --- | --- |
| `8a8fdc91368` | Prepare and revalidate common positional Python calls, then transfer owners into overlapping child homes; retirement owns attribute publication, removing five duplicate pre-retirement stores | [A receipt](../../issues/interpreter-throughput/integration-a.md), [cold A](../../issues/interpreter-throughput/cold-a-review.md) |
| `ba37cc10058` | Initialize only the new child tail outside the caller's proven valid overlap; correct the provider-decline assertion and clarify alias ownership | [B receipt](../../issues/interpreter-throughput/integration-b.md), [cold B](../../issues/interpreter-throughput/cold-b-review.md) |
| `7780e32e0e2` | Reuse the existing attribute completion postcondition to omit five final temporary stores, retaining delayed local replacement | [C receipt](../../issues/interpreter-throughput/integration-c.md), [cold C](../../issues/interpreter-throughput/cold-c-review.md) |

Calls already used a frame stack. The common route now avoids the consuming binder's argument copies after fallible preparation and final guards. It supports positional defaults, empty varargs and a fresh empty kwargs dictionary. Instrumented, keyword/EX, generator-owned caller and other unsupported shapes retain their established providers. Fallback precedes detachment; commit cannot escape or fail.

Initialized homes, owning references and future-read liveness are different facts. The frame experiment reuses only the known-NULL non-argument overlap. Constants still receive ordinary owning duplicates. Ordinary thread retirement already cleared locals only; earlier descriptions of a blanket temporary scan were too broad.

Publication stays before receiver destruction and folded-local replacement stays afterward. There is no new runtime publication cache or validity bitmap. The final five attribute handlers occupy 5,668 bytes versus 6,313 in B, a 645-byte reduction. This is retained GCC code evidence, not measured dynamic cost or a speedup. See the [lifetime contract](../../design/call-and-frame-lifetimes.md) and [holistic review](../../issues/interpreter-throughput/holistic-design-review.md).

## Validation

Release, debug and stats builds each check 117 modules, with none missing or failing import. All candidate build identities match `7780e32e0e2`; controls remain upstream `3a440b65695` and interim `eba351fee7d`, with 116 modules each. Value API lint checks 306 native units with zero failing findings.

Timing uses GCC 15.2.0 release builds with `-O3`, frame pointers and mimalloc,
without PGO/LTO or a JIT. Upstream uses computed-goto dispatch; the fork uses
its register interpreter and tail-call handlers. No second compiler was used.
The candidate ELF build ID is `8c621301ec1ca453bca4c2405f64a80fa187e571`;
upstream is `ff51c387c04ccd5a2664e5a572bc68e491ec0db1`. Pre/postflight
source, executable, module-origin and build snapshots match across stages.

| Integration | Release focused tests | Debug focused tests |
| --- | ---: | ---: |
| A | 939 run, 11 skips | 939 run, 10 skips |
| B | 1,013 run, 12 skips | 1,013 run, 11 skips |
| C | 729 run, 11 skips | 729 run, 10 skips |

All focused sets pass. The changed generator tests and final receiver-retirement semantic test also pass independently. The latter checks slot/shape handlers, actual temporary/local destination classification, finalizer reentry, old/new local observation, result lifetime and absence of extra retention. Installed-handler assertions are not an exact dynamic handler-hit counter.

Full suites ran at the coherent batch boundary:

| Profile | Tests run | Failures | Skips | Duration |
| --- | ---: | ---: | ---: | --- |
| Release | 54,361 | 0 | 2,467 | 3m05s |
| Debug | 54,361 | 0 | 2,405 | 9m11s |

Each reports 537 passing files, 79 skipped files and 11 resource-denied files, out of 627. Early component checks caught an owned/borrowed C type mismatch, a generator test using the wrong fixture helper, and a frame-constant test selecting no hidden slot. All were corrected and their relevant checks passed. Failed logs remain available; those component corrections did not trigger full-suite reruns.

## Integration screens

Each frozen integration has three alternating upstream/candidate rounds, seven timing values per invocation, and seven selected kernels. Ratios below are elapsed factors; lower is better. Control-relative means the change in candidate/upstream factor across campaigns, not a simultaneous parent/child measurement.

| Comparison | Candidate elapsed geomean | Control-relative factor |
| --- | ---: | ---: |
| A / starting 6a | 0.982103 | 1.010531 |
| B / A | 1.003425 | 0.978094 |
| C / B | 0.994414 | 1.002317 |

These mixed screens establish no broad throughput win. Upstream drift changes the interpretation of some rows; normalization is a sensitivity check rather than a correction for noise. B's additional bounds arithmetic has no admitted throughput benefit. Read the independent [admission review](../../issues/interpreter-throughput/admission-review.md) before treating a removed clear as a performance result.

## Final elapsed measurements

The completed 29-kernel campaign gives candidate/upstream **1.263136** and candidate/interim **1.054877**, as equal-weight geometric means of per-kernel median elapsed factors. Lower is better. The register interpreter remains slower overall: about 26.3% against upstream and 5.5% against interim. It wins 4/29 and 10/29 individual kernels respectively. This batch does not establish upstream parity.

| 29-kernel comparison | Factor |
| --- | ---: |
| Current candidate / upstream | 1.263136 |
| Earlier 6a candidate / upstream | 1.286771 |
| Current candidate / earlier candidate | 0.994507 |
| Current upstream / earlier upstream | 1.013115 |
| Current interim / earlier interim | 1.013789 |

The candidate's own geomean elapsed time is about 0.5% lower between campaigns. The improved candidate/upstream ratio is larger because the upstream control also slows. These sequential campaigns do not isolate individual patch effects or establish a repeatable broad speedup. Three-round max/min spreads exceed 10% in 14 candidate, 13 upstream and 11 interim kernel rows; the maximum candidate spread is 29.9%. Treat these as diagnostic results with substantial host variability, not significance estimates. Full vectors and every row remain in the evidence.

The declared 12-result pyperformance subset gives candidate/upstream **1.084920** and candidate/interim **0.902391**. It uses the existing estimator: an equal-weight geometric mean of per-result arithmetic-mean ratios, with six workers and three timed values per worker. Eight modules produce twelve results; five are SciMark results. This is a selected subset, not the full pyperformance suite.

| Subset result | Candidate / upstream | Candidate / interim |
| --- | ---: | ---: |
| async_tree_none | 1.428944 | 1.239406 |
| hexiom | 1.270080 | 0.977440 |
| mdp | 1.466678 | 1.182017 |
| pyflate | 1.122406 | 0.894043 |
| regex_v8 | 1.060772 | 1.073295 |
| scimark_fft | 0.566540 | 0.494557 |
| scimark_lu | 0.889404 | 0.916341 |
| scimark_monte_carlo | 1.012221 | 0.698677 |
| scimark_sor | 1.095678 | 0.784705 |
| scimark_sparse_mat_mult | 0.723085 | 0.659125 |
| typing_runtime_protocols | 1.521286 | 1.257648 |
| unpickle_pure_python | 1.364986 | 1.030265 |

The earlier 6a subset factor was 1.096437 against upstream. Current/earlier elapsed factors are 0.986534 for the candidate, 0.997007 for upstream and 0.998201 for interim. The candidate's roughly 1.3% raw reduction is modest, and these sequential campaigns do not isolate the three changes.
The collector reports precision warnings on 35/36 role/result rows. Its load
guard applies before each module launches, not throughout every worker;
worker metadata reaches 4.54. All declared workers remain included. Quiet
launches do not establish a quiet host for the whole timed interval.

## Hardware counters and profiles

All 54 two-second hot-region counter invocations pass, with 108 event rows at 100% running time and unchanged executable identities. Counts are normalized by actual workload loops, then summarized by the median of three rounds. Setup and warmup precede the acknowledged counter-enable interval. The earlier empty-region calibration uses the same protocol and had instruction overhead below 0.00036%; it was not repeated or subtracted.

| Workload | Instructions C/U | Cycles C/U | CPI C/U | Current / earlier candidate instructions |
| --- | ---: | ---: | ---: | ---: |
| deltablue | 2.001520 | 1.902425 | 0.950490 | 0.977912 |
| logging_silent | 1.774821 | 1.865216 | 1.050932 | 0.932549 |
| comprehensions | 1.605447 | 1.480663 | 0.922274 | 0.985576 |

The batch reduces executed candidate instructions in these selected loops by about 2.2%, 6.7% and 1.4% respectively. The upstream instruction counts change by less than 0.05% between campaigns. This supports reduced executed work in the composition; it does not identify A, B or C individually. Excess native work remains large, while CPI stays within roughly 8% of upstream. For these three cases, instruction volume explains more of the measured gap than a general stall/CPI penalty. Cache counters remain unsuitable for attribution.

These are native instructions **per workload loop**, not per Python bytecode. We did not collect matched dynamic upstream bytecode and candidate IR-operation denominators for these regions. The counters cannot turn the older static IR-size comparisons into a per-bytecode cost measurement.

Six five-second `cycles:u` DWARF captures completed with no lost samples. The physical-symbol self reports put the remaining work here:

| Candidate profile | Selected physical-symbol self shares |
| --- | --- |
| deltablue | shape attribute load 10.73%; exact Python call 8.97%; method attribute load 5.95%; type getattribute 5.78%; type-cache lookup 5.35% |
| logging_silent | `call_transaction_push_python` 16.18%; frame clear 9.38%; method attribute load 6.32%; shape attribute load 5.44% |
| comprehensions | list iteration 7.76%; dict lookup 6.00%; shape attribute load 4.81%; descriptor call 4.47%; exact Python call 4.08% |

These are sampled-period shares from one record per case/role, not isolated operation costs. Inline names in recovered callchains are compile-time ancestry; inclusive shares overlap. The call wrapper includes inlined frame preparation and overlap work. Its logging annotation points to frame setup, guards/revalidation, the remaining NULL-tail loop and empty-varargs handling; focusing only on its prologue would miss most sampled work. See the [call audit](../../issues/interpreter-throughput/call-followup-audit.md).

The logging first-recovered-frame libc bucket is 3.73% of sampled periods, reached through frame initialization, versus 7.22% in the older capture. Exact retained libc identity and bounded disassembly support a memset-shaped routine; the stripped symbol remains unresolved. This is a descriptive share comparison, not proof of B's elapsed benefit. Real new tail homes still need initialization under the current observer contract, and ordinary thread cleanup already clears locals only.

All hit modules have retained matching ELF identities. Actual sampled-IP unknown shares range from 0% to 3.92%; small unresolved kernel-looking addresses account for at most 0.188% despite the requested user event/callchain settings. Recovered user frames can omit those sampled addresses, so their unknown percentages differ. We retain both views and make no claim of complete symbolization. Most recovered stacks reach `_start`, but tail dispatch cannot recover earlier handler history. Exact raw reports, annotation commands, period sums and loss checks remain in the evidence and [independent review](../../issues/interpreter-throughput/final-measurement-review.md).

## Collection and admission decisions

The first 29-kernel attempt was rejected when one-minute load reached 4.04 during candidate `go`; that invocation wrote no report. Source and executable identities remained unchanged. Its partial rows are excluded from final aggregates. A second campaign expired at its initial quiet-host wait without invoking a native benchmark. The completed `final-v3` kernel campaign retained the existing resume-below-3/reject-at-4 guards and retried only exact documented load-guard failures. Each scheduled cell retained its first accepted valid result, with every attempt journaled: 261 accepted cells and one rejected attempt. Other failures remained fatal. The independently reviewed controller change passes eight focused fixtures; it changes no benchmark hot code or runtime binary.

The accepted campaign is `final-v4`: its kernel directory explicitly links the complete, independently accepted `final-v3/kernels` stage. Only the failed subset was restarted, after the separately reviewed `9e349942ff3` prelaunch wait fix passed four focused fixtures. The new subset, 54 counter runs and six captures all completed. The partial `final-v3/pyperformance` attempt is excluded. No accepted cell was rerun to select a faster score.

The coordinator retains A's common transfer and C's publication deduplication. They have reviewed ownership contracts and remove concrete redundant work without another runtime representation or publication cache. The selected-loop instruction reductions support the batch's mechanism, while the elapsed comparisons remain modest and noisy.

B remains a **known-NULL initialization reuse change with unproven throughput benefit**. The independent reviewer prefers removing its clear-elision arithmetic absent specific benefit evidence. The coordinator retains its small, shared initialized-prefix contract: it reuses an existing ownership invariant, introduces no metadata or observer protocol, and makes the skipped range explicit with debug assertions. This is a maintenance judgment, not a measured speedup admission; additional bounds arithmetic may cost more than short clears. The combined campaign cannot isolate B. Do not extend this optimization or use it to justify sparse validity metadata without a focused benefit comparison. Its independent provider assertion and ownership clarification remain useful regardless.

## Remaining work and evidence

The [follow-up plan](../../issues/interpreter-throughput/follow-up.md) names the next packets. One concrete hypothesis is separating the consuming slow provider from the common call route: final GCC still reserves 248 stack bytes and loads a stack canary before the overlap guards. Its dynamic cost is unestablished. Other packets target frequent retained helper work and one generated phase/effect contract, with ownership validity separate from future liveness.

This batch does not complete every value-native C adapter or the optimizing method/tracing JIT. The earlier full-suite conversion census is cumulative migration evidence, not a live-allocation count or hot-loop profile. Its 11,028,818 debt events and accounting limits remain historical; no new zero-box census was run. Ctypes raw-pointer and LRU implementation retain their recorded approval holds. Startup RSS is outside this throughput report.

Raw evidence root: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-throughput-20261106/`. Build/test/lint logs, `final-full-tests.json`, integration-screen vectors, retained GCC executables, module-origin preflight and exact campaign commands are stored there. Excluded earlier attempts are `final-kernels`, `final-v2` and the partial `final-v3/pyperformance`; the selected campaign is `final-v4`, reusing only complete `final-v3/kernels`. Reviewed `analyze-results.py` produces `final-analysis/analysis.json` and `analysis.md`; its independent archived-data self-check is retained in `analysis-baseline-selfcheck`. Historical controls and 6a raw measurements remain under the adjacent `checkpoint-closure` evidence directory.

## Complete kernel elapsed factors

Three rounds per role, five timing values per invocation, with a median for
each round and a median across rounds. Every kernel has equal aggregate weight.
Current/earlier factors are sensitivity comparisons between campaigns, not
individual patch effects. Lower is better in all columns.

| Kernel | Candidate / upstream | Candidate / interim | Current / earlier candidate |
| --- | ---: | ---: | ---: |
| chaos | 0.977819 | 0.824947 | 0.941175 |
| comprehensions | 1.547041 | 1.185885 | 1.001086 |
| coroutines | 1.490996 | 0.992581 | 1.064017 |
| deepcopy | 1.417544 | 1.225047 | 1.032504 |
| deepcopy_memo | 1.207970 | 1.055011 | 0.987526 |
| deepcopy_reduce | 1.316830 | 1.187272 | 0.996591 |
| deltablue | 1.800074 | 1.345953 | 0.956811 |
| fannkuch | 1.262427 | 0.942522 | 0.971613 |
| float | 0.808978 | 0.657319 | 0.933461 |
| generators | 1.508093 | 1.040091 | 1.045662 |
| go | 1.343081 | 1.072779 | 0.893477 |
| json_dumps | 1.373556 | 1.266241 | 0.966735 |
| json_loads | 1.286164 | 1.119402 | 1.007530 |
| logging_format | 1.323950 | 1.073481 | 0.943623 |
| logging_silent | 1.676253 | 1.425471 | 0.914143 |
| logging_simple | 1.209933 | 1.177763 | 0.981978 |
| nbody | 0.850820 | 0.684527 | 1.041873 |
| nqueens | 1.224015 | 0.934193 | 0.991499 |
| pickle | 1.140865 | 0.925749 | 1.000532 |
| raytrace | 1.064650 | 0.893515 | 0.939297 |
| regex_compile | 1.214057 | 0.977613 | 0.978668 |
| richards | 1.256608 | 1.105815 | 0.964206 |
| spectral_norm | 0.983849 | 1.118311 | 1.150484 |
| unpack_sequence | 1.405212 | 0.771278 | 1.111063 |
| unpickle | 1.426350 | 1.408890 | 1.121581 |
| xml_etree_generate | 1.260750 | 1.105108 | 0.981155 |
| xml_etree_iterparse | 1.224382 | 1.193679 | 0.973558 |
| xml_etree_parse | 1.101367 | 1.136126 | 0.992770 |
| xml_etree_process | 1.505688 | 1.276510 | 1.004294 |
| Geomean | 1.263136 | 1.054877 | 0.994507 |
