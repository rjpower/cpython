# Completed diagnostic evidence review: `6a0e4c12c17`

Independent review on 2026-11-06 accepts the completed evidence and the
[checkpoint report](../../bench/results/checkpoint-6a0e4c12c17.md) as diagnostic
results. There are no blocking collection or arithmetic findings. This does
not admit R27 or close the interpreter checkpoint. The full census and strict
startup-inclusive kernel gate remain failed.

This review read retained JSON, journals, raw sampling records, ELF notes,
source and reports. It ran no interpreter, build, test, benchmark or sampling
workload, and changed only this receipt. Collection and runtime integration
remain coordinator-owned.

## Evidence and identity

All directories below are under the external build root
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/checkpoint-closure/`:

- `measure-6a0e4c12c17-diagnostic-v3/`
- `pyperformance-6a0e4c12c17-diagnostic/`
- `rss-6a0e4c12c17-diagnostic/`
- `profiles-6a0e4c12c17-diagnostic/`

The measurement stages record unchanged before/after build identities and no
preflight/postflight errors. Source revisions, executable revisions, ELF IDs,
module origins and rosters agree with upstream `3a440b65695`, interim
`eba351fee7d`, and candidate `6a0e4c12c17`. The companion source manifests
are unchanged. Diagnostic eligibility remains false as intended. Timing, RSS
and sampling were separate stages; retained journals place RSS after pyperf,
and profile command-file timestamps follow the RSS completion.

## Kernel elapsed time

All 261 reports reconcile: three rounds, 29 kernels and three interpreters,
with the declared rotation and no reused samples from the two failed attempts.
Each report contains five measured calls and one warmup; its median matches
the saved round vector. Loop counts agree across roles. Every displayed
kernel median, ratio and round spread matches the raw reports.

The invocation journal retains complete load tuples and quiet-wait records.
Terminal wait loads stay below three; child-report loads stay below four
(maximum 3.4722). Nineteen invocations waited, totaling 990.04 seconds. The
journal spans 1,276.95 seconds, within the 45-minute window.

| Equal-weight 29-kernel geomean | Upstream | Interim |
| --- | ---: | ---: |
| Baseline elapsed / candidate elapsed | 0.777139 | 0.929947 |
| Candidate elapsed / baseline elapsed | 1.286771 | 1.075330 |

The candidate beats upstream in three rows and interim in eight. Six candidate
round-median spreads exceed 10%: unpack sequence 22.3%, deepcopy 22.0%,
deepcopy memo 20.3%, logging format 18.2%, JSON loads 12.9%, and unpickle
10.8%. Small differences in these rows do not support a significance claim.

## Pyperformance subset

All 24 invocations reconcile with the declared eight modules and rotated role
order. Each role has 12 named results, each with six measurement workers,
three measured values per worker, and one warmup per measurement worker;
calibration is separate. The commands and recorded executable metadata agree
with the launchers, release interpreters, pyperf 2.10.0, hash seed zero and the
intended `PYTHONPATH`. All commands exit zero without overlap.

Independent arithmetic over the saved 18-value vectors reproduces every
reported mean and sample coefficient of variation, including the coordinator's
post-collection `aggregate.json`. The baseline/candidate geomeans are
0.912045 upstream and 1.095214 interim; their inverses are 1.096437 and
0.913063. Collection takes 487.91 seconds, within 30 minutes. The maximum
command-start load is 3.7105 and maximum recorded worker load is 3.84.

Pyperf emits insufficient-sample precision warnings for 35 of 36 results:
the sample count does not establish variation within 1% at 95% certainty.
Candidate SciMark LU has the largest sample CV, 7.97%; its modest improvement
needs that qualification. The aggregate weights 12 results equally, including
five SciMark outputs. It is neither the complete suite nor an equal-weight
eight-module score.

## Peak RSS

All three interpreters have the same 31 rows: 29 kernels, startup and imports.
Every row has three positive GNU-time peak-RSS samples, and its median agrees
with the report. Identities and source manifests are unchanged. The maximum
recorded child-harness load is 3.7178.

The warming receipts contain 78/78/79 current-ABI extensions plus the respective
interpreter: 79/79/80 distinct files. File sizes and names match `fincore`;
every listed file has all pages resident at that check. Historical ABI files
are excluded from the current roster.

The independently recomputed 29-kernel candidate/baseline peak-RSS geomeans
are 1.259197 upstream and 1.061971 interim. Startup medians are
10,372/12,036/13,300 KiB and imports 11,932/14,492/15,692 KiB, in
upstream/interim/candidate order. All 31 report rows reconcile. Peak RSS
includes resident mappings; it is not retained heap, PSS or an anonymous-memory
breakdown.

## Statistical profiles and report review

All six upstream/candidate cases exit zero and retain depth-one raw PCs,
nonempty maps and matching executable-load offset/virtual-address checks.
Retained interpreter ELF IDs match the measured builds, and the retained
sampler matches the preloaded artifact. Loop counts derive from the fresh
medians and target roughly two seconds after warmup. Raw counts agree with
both mapping receipts and the report:

| Kernel | Upstream samples | Candidate samples |
| --- | ---: | ---: |
| deltablue | 2,193 | 2,026 |
| logging_silent | 1,885 | 2,006 |
| comprehensions | 1,966 | 1,946 |

The sampler arms its 1 ms SIGPROF timer only after preparation and warmup.
These are statistical leaf-PC observations, with sampling overhead, not caller
stacks, hardware instruction counts or causal patch costs. Candidate tail-call
handlers have separate symbols; upstream's large evaluator contains much of
the corresponding work. Their symbol percentages are not directly comparable.
The symbolizer chooses the nearest preceding symbol without checking its end
address. Stripped external-library labels, including `_dl_mcount_wrapper`,
therefore remain opaque and do not prove execution cost in the named function.
PyValue-related symbols likewise do not establish boxing cost.

The checkpoint report's kernel, pyperf and RSS table cells match independent
recalculation. Its profile observations match the retained symbol reports,
and its external-symbol, precision and qualification caveats are appropriate.
The observations support bounded attribute, call/frame, iteration and
dictionary experiments; they do not identify a corrective patch or clear any
ownership/accounting gate. Use the [follow-up queue](follow-up-6a0e4c12c17.md)
for those gates and preserve its ctypes/LRU implementation holds.
