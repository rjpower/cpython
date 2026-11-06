# PyValue/register interpreter diagnostic, 2026-11-06

Candidate runtime: `6a0e4c12c17`. This is an elapsed-time diagnostic of the
implemented interpreter, with the JIT off. Full release and debug suites pass
54,351 tests each, but strict R27 admission fails. Performance does not close
that gate. See the [census](../../issues/checkpoint-closure/full-census-6a0e4c12c17.md)
and [follow-up plan](../../issues/checkpoint-closure/follow-up-6a0e4c12c17.md).

## Builds and collection

| Role | Source | Release ELF build ID | Module roster |
| --- | --- | --- | ---: |
| Upstream | `3a440b65695` | `ff51c387c04ccd5a2664e5a572bc68e491ec0db1` | 116 |
| Interim RC | `eba351fee7d` | `6b25bff25ab86337e0eb1284483cdd6350186b9a` | 116 |
| Universal PyValue | `6a0e4c12c17` | `df7d4401cc45f00e9dabead7cb8fc8f3b06bd00c` | 117 |

All use GCC 15.2, `-O3` and mimalloc, without PGO or LTO. Upstream uses
computed-goto dispatch; the fork uses register-IR tail-call handlers.
Interim and candidate enable the fork's shapes. These are matched release
controls, not upstream's best PGO/LTO configuration. Source trees, interpreter
and imported extension identities are recorded before and after collection.
All worker builds and tests stopped for measurement; stages run sequentially.

Retained raw artifacts share this project-managed root:
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/checkpoint-closure/`.
Kernel results are in `measure-6a0e4c12c17-diagnostic-v3/`, including all 261
invocation reports, `round-medians.json`, load waits and pre/postflight records.
The two earlier attempts (`measure-6a0e4c12c17-diagnostic/` and `-v2/`) failed
the load guard and remain retained; none of their partial samples is reused.

The accepted v3 protocol waits for one-minute load below three before every
invocation, rejects child measurements at load four, and retains all three
load averages. Waits count toward the 45-minute window. The fixed three-round
kernel/interpreter rotation is unchanged. The completed run's identities are
unchanged and pre/postflight report no errors. Small spreads remain noise;
there is no statistical-significance claim. Hardware counters below are a
separate collection on three selected kernels.
The [harness review](../../issues/checkpoint-closure/measurement-harness-review-v2.md)
records collection fixes and limitations.
The independent [completed-evidence review](../../issues/checkpoint-closure/measurement-evidence-review-6a0e4c12c17.md)
reconciles every timing, pyperformance and RSS table cell and the profile records.

## Kernel elapsed time

For each kernel, take each invocation's five-call median, then the median of
its three independent rounds. Aggregate ratios with the geometric mean.
The candidate takes **1.2868 times upstream elapsed time** and **1.0753 times
interim elapsed time**. It wins 3/29 kernels against upstream and 8/29 against
interim. Float, nbody and spectral norm are the upstream wins. Deltablue and
silent logging are the largest upstream regressions. Six candidate rows have
more than ten percent max/min spread across rounds; keep that noise visible.

Qualification: diagnostic. DIAGNOSTIC ONLY — NOT R27 ADMISSION.

Ratios are baseline elapsed / candidate elapsed; above 1 favors candidate.

| Kernel | Candidate ns/loop | Upstream/Candidate | Interim/Candidate | Candidate spread |
| --- | ---: | ---: | ---: | ---: |
| richards | 77,870,383 | 0.741 | 0.842 | 7.6% |
| go | 170,978,134 | 0.669 | 0.876 | 2.5% |
| nbody | 87,690,341 | 1.140 | 1.436 | 9.2% |
| float | 67,927,983 | 1.221 | 1.391 | 3.1% |
| spectral_norm | 97,960,887 | 1.113 | 0.946 | 5.9% |
| raytrace | 358,821,756 | 0.894 | 1.019 | 1.3% |
| deltablue | 6,920,096 | 0.527 | 0.701 | 7.5% |
| chaos | 70,975,739 | 0.964 | 1.126 | 5.0% |
| nqueens | 111,936,646 | 0.811 | 1.101 | 3.7% |
| fannkuch | 508,466,113 | 0.776 | 0.979 | 1.0% |
| generators | 46,329,362 | 0.688 | 1.031 | 7.0% |
| comprehensions | 26,651 | 0.667 | 0.902 | 0.7% |
| coroutines | 35,982,860 | 0.720 | 1.104 | 1.7% |
| unpack_sequence | 19,229 | 0.695 | 1.461 | 22.3% |
| deepcopy | 439,402 | 0.679 | 0.840 | 22.0% |
| deepcopy_reduce | 5,123 | 0.713 | 0.862 | 1.0% |
| deepcopy_memo | 40,882 | 0.732 | 0.970 | 20.3% |
| json_dumps | 15,984,423 | 0.701 | 0.764 | 3.1% |
| json_loads | 740,663 | 0.792 | 0.794 | 12.9% |
| pickle | 300,443 | 0.916 | 0.911 | 4.8% |
| unpickle | 448,353 | 0.797 | 0.802 | 10.8% |
| regex_compile | 213,400,465 | 0.826 | 0.986 | 3.2% |
| logging_format | 106,206 | 0.721 | 0.831 | 18.2% |
| logging_silent | 2,187 | 0.554 | 0.622 | 1.2% |
| logging_simple | 96,039 | 0.820 | 0.855 | 2.0% |
| xml_etree_generate | 172,592,454 | 0.751 | 0.859 | 1.7% |
| xml_etree_iterparse | 141,031,873 | 0.778 | 0.816 | 1.5% |
| xml_etree_parse | 184,282,199 | 0.863 | 0.889 | 1.6% |
| xml_etree_process | 125,827,465 | 0.677 | 0.794 | 0.5% |
| Geomean | | 0.777 | 0.930 | |

## Pyperformance subset

The pinned pyperformance 1.14.0 subset uses pyperf 2.10.0: eight modules,
12 named results, six worker processes and three measured values per worker
(18 per result), one warmup and 0.1-second minimum time. Roles rotate by
module; result means below use all measured values through pyperf’s loader.
Collection completed all 24 invocations. Build identities and cached benchmark/
pyperf source manifests stayed unchanged; pre/postflight report no errors.
Raw results and commands are in `pyperformance-6a0e4c12c17-diagnostic/`.

The candidate takes **1.0964 times upstream elapsed time** and **0.9131 times
interim elapsed time** on the 12-result geometric mean. SciMark contributes
five equally weighted rows; this is a declared subset, not the complete suite.
Pyperf warned for 35 of 36 results that the sample count does not establish
precision within one percent at 95% certainty. No significance test is claimed.
CV is sample standard deviation / mean; LU has 8.0% candidate CV.

| Result | Candidate mean ms | Upstream/Candidate | Interim/Candidate | Candidate CV |
| --- | ---: | ---: | ---: | ---: |
| hexiom | 7.631 | 0.855 | 1.075 | 2.2% |
| pyflate | 460.355 | 0.922 | 1.129 | 1.7% |
| scimark_fft | 221.003 | 1.728 | 1.959 | 2.3% |
| scimark_lu | 135.543 | 1.065 | 1.076 | 8.0% |
| scimark_monte_carlo | 80.751 | 0.906 | 1.265 | 3.6% |
| scimark_sor | 148.816 | 0.897 | 1.265 | 1.3% |
| scimark_sparse_mat_mult | 4.342 | 1.345 | 1.560 | 1.5% |
| regex_v8 | 22.565 | 0.955 | 0.926 | 1.4% |
| mdp | 2184.522 | 0.697 | 0.860 | 2.7% |
| async_tree_none | 531.347 | 0.667 | 0.763 | 4.3% |
| unpickle_pure_python | 0.368 | 0.698 | 0.959 | 1.0% |
| typing_runtime_protocols | 0.249 | 0.674 | 0.800 | 3.9% |
| Geomean | | 0.912 | 1.095 | |

## Peak RSS

RSS is secondary to steady-state throughput for this project.

RSS is separate from elapsed timing: GNU time peak resident set, median of
three fresh processes per row. Before collection, the helper touched every
page of each executable and all current-ABI extensions (78/78/79), then
retained `fincore` residency and the exact file roster. Source and benchmark
identities remained unchanged; pre/postflight report no errors. Raw samples
are in `rss-6a0e4c12c17-diagnostic/`. This measures peak RSS, not anonymous/
file-backed breakdown, PSS, retained heap size or an application memory model.

Across the 29 kernels, candidate/baseline peak-RSS geometric means are
**1.2592 versus upstream** and **1.0620 versus interim**. Startup is 13.0 MiB
versus upstream 10.1 MiB; imports are 15.3 versus 11.7 MiB. Float is a memory
win (20.8 MiB versus upstream 27.1 MiB); the overall kernel aggregate is larger.

| Workload | Upstream KiB | Interim KiB | Candidate KiB | Candidate/Upstream | Candidate/Interim |
| --- | ---: | ---: | ---: | ---: | ---: |
| richards | 10,368 | 12,312 | 13,188 | 1.272 | 1.071 |
| go | 12,020 | 13,984 | 15,196 | 1.264 | 1.087 |
| nbody | 10,456 | 12,316 | 13,596 | 1.300 | 1.104 |
| float | 27,740 | 29,488 | 21,292 | 0.768 | 0.722 |
| spectral_norm | 10,308 | 12,084 | 13,288 | 1.289 | 1.100 |
| raytrace | 11,564 | 13,568 | 15,016 | 1.299 | 1.107 |
| deltablue | 11,336 | 13,208 | 14,372 | 1.268 | 1.088 |
| chaos | 12,096 | 14,096 | 15,332 | 1.268 | 1.088 |
| nqueens | 10,360 | 12,048 | 13,244 | 1.278 | 1.099 |
| fannkuch | 10,388 | 11,964 | 13,264 | 1.277 | 1.109 |
| generators | 23,416 | 25,244 | 23,616 | 1.009 | 0.936 |
| comprehensions | 12,676 | 15,472 | 16,476 | 1.300 | 1.065 |
| coroutines | 10,336 | 11,916 | 13,292 | 1.286 | 1.115 |
| unpack_sequence | 10,168 | 12,412 | 13,404 | 1.318 | 1.080 |
| deepcopy | 11,752 | 14,260 | 15,428 | 1.313 | 1.082 |
| deepcopy_reduce | 11,736 | 14,064 | 15,336 | 1.307 | 1.090 |
| deepcopy_memo | 11,620 | 14,032 | 15,360 | 1.322 | 1.095 |
| json_dumps | 12,504 | 15,700 | 16,568 | 1.325 | 1.055 |
| json_loads | 12,972 | 15,364 | 16,836 | 1.298 | 1.096 |
| pickle | 12,736 | 14,968 | 16,572 | 1.301 | 1.107 |
| unpickle | 12,528 | 14,628 | 16,284 | 1.300 | 1.113 |
| regex_compile | 14,096 | 17,588 | 18,676 | 1.325 | 1.062 |
| logging_format | 15,528 | 18,684 | 19,916 | 1.283 | 1.066 |
| logging_silent | 13,836 | 17,004 | 17,836 | 1.289 | 1.049 |
| logging_simple | 15,052 | 18,572 | 19,548 | 1.299 | 1.053 |
| xml_etree_generate | 19,520 | 23,120 | 24,004 | 1.230 | 1.038 |
| xml_etree_iterparse | 19,312 | 23,064 | 24,552 | 1.271 | 1.065 |
| xml_etree_parse | 18,212 | 22,468 | 23,708 | 1.302 | 1.055 |
| xml_etree_process | 19,040 | 23,332 | 25,448 | 1.337 | 1.091 |
| startup | 10,372 | 12,036 | 13,300 | 1.282 | 1.105 |
| imports | 11,932 | 14,492 | 15,692 | 1.315 | 1.083 |
| Kernel geomean (29) | | | | 1.259 | 1.062 |

## Statistical profiles

After all timing and RSS collection, depth-one SIGPROF sampling captured
about two seconds of warmed hot work per interpreter/kernel. Loop counts
come from the fresh timing medians. Raw PCs, maps, exact executable copies,
ELF notes, commands, return codes and symbol reports remain in
`profiles-6a0e4c12c17-diagnostic/`. All six child processes exited successfully;
executable PT_LOAD offset/virtual-address checks passed for symbolization.

| Kernel | Upstream samples | Candidate samples | Candidate leaf-PC observations |
| --- | ---: | ---: | --- |
| deltablue | 2,193 | 2,026 | `LOAD_ATTR_SHAPE` 14.86%; `CALL_PY_EXACT_ARGS` 8.69%; `LOAD_ATTR_METHOD_WITH_VALUES` 5.28%; `_Py_type_getattro_impl` 4.59% |
| logging_silent | 1,885 | 2,006 | `eval_frame_push_and_init_code` 8.47%; `call_transaction_push_python` 8.37%; frame clearing 7.63%; method attribute handler 7.63% |
| comprehensions | 1,966 | 1,946 | `FOR_ITER_LIST` 6.47%; dictionary lookup 5.86%; `LOAD_ATTR_SHAPE` 5.86%; method descriptor call handler 4.47% |

These samples prioritize attribute lookup, Python-call/frame transactions,
iteration and dictionary work for bounded experiments. They do not establish
the cost of an individual patch or allocation. Candidate tail-call handlers
are separate symbols while much upstream execution is inside
`_PyEval_EvalFrameDefault`; their symbol percentages are not directly
comparable. Leaf samples provide no caller stacks or hardware instruction
counts. Nearest-symbol labels in stripped external libraries remain opaque;
the stripped-libc `_dl_mcount_wrapper` label is not proof of mcount work.

## Hardware counters and native call stacks

After the user enabled `perf_event_paranoid=2`, collection completed on an
AMD Ryzen 7 3800X (eight cores, 16 SMT threads, boost enabled). The matched
upstream and candidate source/build identities remained unchanged, including
the imported extension roster; the final source/module postflight has no
errors or differences. No runtime changes separate these counters from the
elapsed-time diagnostic. Access history and the user-only call-chain scope
are recorded in the [access receipt](../../issues/checkpoint-closure/perf-access-6a0e4c12c17.md).

Counters cover three rotating rounds, two interpreters and three event pairs:
cycles/instructions, branches/branch misses, and cache references/misses.
All **54/54 runs** are valid, with 100% counter running time. Perf starts
disabled; setup and warmup finish before an acknowledged enable, then an
acknowledged disable follows the hot loop. Counts include the small control
bracket outside the benchmark's own hot clock. All 18 matched empty-control
runs are valid; median empty instruction counts are below 0.00036% of the
corresponding full hot counts. Nothing is subtracted. Hot runs lasted
1.852–2.144 seconds and used at least 99.39% process CPU/wall time.

Raw commands, counter vectors, control acknowledgments, calibration and
return codes are retained in `perf-stat-6a0e4c12c17/` and
`perf-empty-6a0e4c12c17-v2/`. The following values use median per-loop
counts from three rounds; CPI is median cycles divided by median instructions.
`postcollection-counter-summary.json` retains every normalized round vector,
median, spread and ratio next to the raw stat records.
Ratios here are **candidate/upstream**. These are selected regressions, not
an instruction-count aggregate for all 29 kernels.

| Kernel | Cycles ratio | Instructions ratio | Upstream CPI | Candidate CPI | Branches ratio | Upstream branch-miss rate | Candidate branch-miss rate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| deltablue | 1.929 | 2.047 | 0.395 | 0.372 | 2.299 | 0.686% | 0.312% |
| logging_silent | 1.939 | 1.903 | 0.343 | 0.349 | 2.147 | 0.089% | 0.021% |
| comprehensions | 1.491 | 1.628 | 0.380 | 0.348 | 1.723 | 0.347% | 0.345% |

For these cases, increased retired instruction work is the strongest measured
explanation for the cycle gap. CPI is comparable or lower, while absolute
branch-miss counts change by 1.047x, 0.511x and 1.715x respectively. This
does not support treating worse branch prediction as the principal common
cause. Cycles and elapsed ratios come from separate runs; boost, sampling and
ordinary run variation prevent treating them as identical.

Cache counts are too variable for a causal conclusion. For example, upstream
logging cache references range from 2,733,880 to 68,709,624 at the same
1,652,262 hot loops (25.1x). Generic cache events do not identify L1 instruction
cache behavior, DRAM latency or the cost of a particular load. Retain the raw
vectors and investigate only with a specific follow-up hypothesis.

Six separate five-second captures use `cycles:u`, 499 Hz, user-only DWARF
call chains and an 8 KiB stack dump. All succeed; no lost samples are reported.
Upstream/candidate sample counts are 2,591/2,542 for deltablue, 2,412/2,524
for logging and 2,484/2,478 for comprehensions. Raw perf data, scripts,
self/caller reports, command receipts and exact hit-library identities are in
`perf-record-6a0e4c12c17-v2/`. The executable copies are shared with the
earlier statistical captures; matching libc and both select modules are
retained beside these perf results. The independent
[hardware evidence review](../../issues/checkpoint-closure/hardware-perf-review-6a0e4c12c17.md)
checks counter arithmetic, calibration, lost samples and unwind limits.

Shares below are weighted by sampled cycle period, rather than unweighted
sample counts. Inclusive shares overlap and must not be added.

| Candidate workload | Native profile evidence | Implication |
| --- | --- | --- |
| deltablue | Shape attribute handler 15.27% self; generic attribute handler 26.48% inclusive; exact-args call 9.99% inclusive | Investigate generic attribute/type lookup admission as well as shape publication and call preparation. |
| logging_silent | Generic Python-call handler 30.30% inclusive; exact-args handler 7.05% inclusive; unknown libc leaves 7.22%, all under frame initialization | Frame initialization affects both call paths. Generic binding work remains a separate target. |
| comprehensions | List iteration 6.76% inclusive, including list getter 0.82% self; dictionary lookup 5.92% self | A thin accessor experiment is bounded; it cannot alone close the 1.499x elapsed gap. |

The logging libc routine has a verified retained build ID and a memset-shaped
instruction sequence, but no resolved function symbol. Its immediate native
caller is `_PyFrame_Initialize`: 3.40% of total cycle period arrives through
overlapping exact-argument frames and 3.82% through generic frame preparation.
Removing this entire bucket would cap speedup at about 1.078x if all other
work stayed fixed. This is an optimistic bound, not an implementable forecast.
The earlier SIGPROF libc capture lacks a library identity and stays opaque.
Tail dispatch erases preceding native handler frames, and DWARF inline frames
are not a logical Python stack. Successful unwinding supports current C-helper
ancestry, not complete Python caller histories. There is one recording per
role/case; sampling skid and recording variability limit site precision.

## Prioritized throughput experiments

The interpreter is **not yet competitive with upstream overall** on these
matched configurations. Kernel parity would require a uniform 22.3% reduction
in candidate elapsed time; the 12-result subset would require 8.8%. These are
aggregate illustrations, not targets that one small patch can promise.
No optimization below has a measured speedup on this source yet. The
[source/disassembly review](../../issues/checkpoint-closure/steady-state-source-review.md)
records exact PCs, ownership constraints and falsification checks.

1. **Calibrate the compiler and handler convention.** Run the existing A3
   R1/Q2 experiment with installed Clang 21.1.8, comparing both candidate and
   upstream against the GCC pair with matched hardening and frame-pointer
   flags. Current GCC helpers spill interpreter state; Clang `preserve_none`
   may change that cost. Keep the normal-ABI entry trampoline: an earlier
   GCC miscompile makes its removal unsafe. This measures the compiler and
   convention together. Actual handlers already tail-jump correctly.
2. **Reduce bounded attribute and accessor work.** Trace deltablue's generic
   attribute/type lookups and shape admission before changing guards. Review
   repeated shape-output publication across generated flushes, preserving
   ownership and callback visibility. Separately inline the private exact-list
   borrowed getter where the GIL/index guards and immediate owning duplicate
   already hold; preserve external and free-threaded entry behavior. The
   getter's 0.82% perf share places a small ceiling on that change alone.
3. **Specialize common Python-call preparation.** Logging uses varargs and
   kwargs; exact-argument calls already have an overlapping-frame fast path.
   Propose a no-keyword varargs lane that completes allocations and callbacks
   while parent arguments remain rooted, then transfers ownership only in a
   non-reentrant commit. Preserve fresh kwargs dictionaries, error order,
   recursion, monitoring and tracing. Do not remove generic binding globally.
4. **Reduce frame initialization and retirement only with a proof.** Current
   code initializes register homes/constants and clears frame slots. A smaller
   live set needs explicit liveness, GC visibility, reentry and resume rules;
   deleting clears based on leaf percentages would be unsound. Review small
   duplicate refresh checks independently and measure their limited benefit.

Each experiment needs a proposal, peer review, minimal relevant tests and
post-review; the coordinator owns matched before/after counters and timing,
then merged regression and admission gates. Keep source and compiler changes
separate. Startup RSS and speculative IR encoding changes are lower priorities.

## Admission and next work

The functional baseline passes, but full PyValue integration is incomplete.
Strict R27 still finds 11,028,818 core/core-pending debt boxes, identity/
accounting gaps and 11 static findings. Eighteen of 29 kernels are raw-zero;
all 29 have zero loop deltas. Neither timing nor profiling changes these gates.
The [reviewed follow-up queue](../../issues/checkpoint-closure/follow-up-6a0e4c12c17.md)
starts with accounting and caller inventory, keeps ctypes/LRU implementation
held, and uses these profiles to propose later performance experiments.
