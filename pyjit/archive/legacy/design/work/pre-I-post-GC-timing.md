# Post-GC matched interpreter elapsed diagnostic

Completed2026-10-30; **independent review passed**. This records the first
matched eval-nogcse comparison after supported GC integration75e8. All29 rows,
58 measured processes and290 serial values completed in77.043 seconds;
native/outer return codes are0 and final cleanup/product guards passed.
The transaction ended and its quiet reservation was released. This is an
initial performance diagnostic, not additional correctness qualification or
attainment of the roughly2× soft goal.

The geometric mean of upstream/fork median elapsed ratios is
**0.8242992363**: fork elapsed is1.21315× upstream under this aggregation.
There are21 ratios below1 and eight above1. Large recurring deficits remain
a planning problem: coroutines0.4489, comprehensions0.5097 and generators0.5737.
Float2.0394, Richards1.8655 and nbody1.6681 are strong point estimates in the
other direction. Neither near1 rows nor float's proximity to2 establishes a
stable rank, parity or a threshold win from these samples.

## Actual comparison and raw evidence

- Upstream source321925c92c9 uses its own `build/eval-nogcse`, with actual
  make23c9, positive18 correctness and fresh29 workload origins accepted.
- Fork ordinary native3c6 uses its own source/stdlib/extensions: runtimeDAC6,
  checkout4da, integrated primary75e8 with its15 Markdown-only bridge. H3 is
  landed; isolated H1/H2 are absent. Existing qualification composites and
  original failed runs retain their exact identities.
- Both use matched GCC15/O3/`CFLAGS_CEVAL=-fno-gcse`, frame pointers,
  zero-init, shared GIL, nondebug/nonstats/computed-goto/static-libpython,
  no JIT/PGO/LTO/BOLT. The upstream and fork implementations, allocators, GC
  and runtime libraries differ; this does not isolate any one architectural cost.
- The existing driver/29 workloads, registered loops, one warmup and five
  timed calls are unchanged. Each runtime/row gets a fresh process; first
  runtime alternates across rows. Both29 run-mode origin checks plus the two
  existing RUNS0 JSON-tail witnesses bind origin scope. There is no per-row
  time-mode import trace or new consumed-SDK-header proof.
- CPU0 was pinned, sibling8 reserved idle, both machine slots/build locks
  held, seed0, maximum load4, with no governor changes. Recorded AMD Ryzen7
  3800X policy is `amd-pstate-epp`/`powersave`, boost enabled. Affinity and
  admission do not eliminate all host, clock, thermal or allocator variation.

[Closure e5b0a9b2](../../../build/checkpoint-i-qualified-refresh/quiet29-closure.json)
binds393 artifacts and the actual source/product checks.
[Plan](../../../build/checkpoint-i-qualified-refresh/quiet29/plan.json),
[outer receipt](../../../build/checkpoint-i-qualified-refresh/quiet29/launch-result.json),
[inner receipt](../../../build/checkpoint-i-qualified-refresh/quiet29/inner-result.json)
and [machine/sampling metadata](../../../build/checkpoint-i-qualified-refresh/quiet29/run/metadata.json)
retain the concrete transaction.
[Score321aa3e9](../../../build/checkpoint-i-qualified-refresh/quiet29/run/score.json)
contains all normalized vectors and raw nanosecond durations; the
[operator summary fc89bc96](../../../build/checkpoint-i-qualified-refresh/quiet29/row-summary.json)
uses its own explicitly labelled range/median statistic. The table below uses
one different, explicit formula throughout.

[Independent review0f88e376](../../../build/checkpoint-i-qualified-refresh/quiet29-outcome-independent-review.json)
recomputed all raw durations, normalized values and ratios, checked all60
command/environment records and58 pairs of load records, replayed both RUNS0
origin witnesses and confirmed actual source/product/cleanup bindings.

## All29 rows

Ratio is `median(upstream ns/loop) / median(fork ns/loop)`; above1 favors fork.
Score is `exp(mean(log(ratio)))`, weighting each declared row equally.
**Spread here is `100 × (max(samples) / min(samples) − 1)`**, separately for
upstream and fork. It is descriptive range relative to the minimum, not an
error bar or confidence interval. Each U/F link preserves the five raw timed
values in execution order; all corresponding stderr/command/load/measurement
receipts remain alongside it. No value was trimmed or retried.

| Row | Upstream/fork | U spread % | F spread % | Raw vectors |
| --- | ---: | ---: | ---: | --- |
| richards | 1.8655 | 0.36 | 9.37 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/00-richards-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/00-richards-fork/stdout.log) |
| go | 1.0125 | 1.58 | 31.20 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/01-go-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/01-go-fork/stdout.log) |
| nbody | 1.6681 | 9.30 | 0.54 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/02-nbody-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/02-nbody-fork/stdout.log) |
| float | 2.0394 | 4.16 | 51.14 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/03-float-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/03-float-fork/stdout.log) |
| spectral_norm | 1.0572 | 7.93 | 41.67 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/04-spectral_norm-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/04-spectral_norm-fork/stdout.log) |
| raytrace | 1.1394 | 1.86 | 13.45 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/05-raytrace-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/05-raytrace-fork/stdout.log) |
| deltablue | 1.0175 | 6.33 | 40.48 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/06-deltablue-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/06-deltablue-fork/stdout.log) |
| chaos | 1.0053 | 7.85 | 47.97 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/07-chaos-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/07-chaos-fork/stdout.log) |
| nqueens | 0.7260 | 0.73 | 16.04 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/08-nqueens-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/08-nqueens-fork/stdout.log) |
| fannkuch | 0.9447 | 1.67 | 6.75 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/09-fannkuch-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/09-fannkuch-fork/stdout.log) |
| generators | 0.5737 | 1.48 | 26.09 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/10-generators-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/10-generators-fork/stdout.log) |
| comprehensions | 0.5097 | 0.82 | 29.39 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/11-comprehensions-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/11-comprehensions-fork/stdout.log) |
| coroutines | 0.4489 | 0.83 | 11.77 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/12-coroutines-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/12-coroutines-fork/stdout.log) |
| unpack_sequence | 0.7722 | 15.37 | 2.00 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/13-unpack_sequence-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/13-unpack_sequence-fork/stdout.log) |
| deepcopy | 0.6460 | 1.39 | 12.01 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/14-deepcopy-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/14-deepcopy-fork/stdout.log) |
| deepcopy_reduce | 0.5940 | 0.43 | 27.02 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/15-deepcopy_reduce-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/15-deepcopy_reduce-fork/stdout.log) |
| deepcopy_memo | 0.7516 | 1.60 | 9.93 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/16-deepcopy_memo-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/16-deepcopy_memo-fork/stdout.log) |
| json_dumps | 0.8194 | 1.06 | 8.79 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/17-json_dumps-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/17-json_dumps-fork/stdout.log) |
| json_loads | 0.6911 | 0.68 | 12.69 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/18-json_loads-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/18-json_loads-fork/stdout.log) |
| pickle | 0.7192 | 0.43 | 6.54 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/19-pickle-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/19-pickle-fork/stdout.log) |
| unpickle | 0.6568 | 0.38 | 2.20 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/20-unpickle-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/20-unpickle-fork/stdout.log) |
| regex_compile | 0.7186 | 1.37 | 10.34 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/21-regex_compile-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/21-regex_compile-fork/stdout.log) |
| logging_format | 0.7191 | 0.67 | 15.69 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/22-logging_format-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/22-logging_format-fork/stdout.log) |
| logging_silent | 0.6234 | 0.87 | 84.26 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/23-logging_silent-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/23-logging_silent-fork/stdout.log) |
| logging_simple | 0.7818 | 1.40 | 17.61 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/24-logging_simple-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/24-logging_simple-fork/stdout.log) |
| xml_etree_generate | 0.7818 | 7.41 | 12.30 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/25-xml_etree_generate-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/25-xml_etree_generate-fork/stdout.log) |
| xml_etree_iterparse | 0.6108 | 3.17 | 30.83 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/26-xml_etree_iterparse-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/26-xml_etree_iterparse-fork/stdout.log) |
| xml_etree_parse | 0.8840 | 7.99 | 12.98 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/27-xml_etree_parse-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/27-xml_etree_parse-fork/stdout.log) |
| xml_etree_process | 0.8162 | 0.52 | 24.09 | [U](../../../build/checkpoint-i-qualified-refresh/quiet29/run/28-xml_etree_process-upstream/stdout.log) / [F](../../../build/checkpoint-i-qualified-refresh/quiet29/run/28-xml_etree_process-fork/stdout.log) |

## Interpretation and next question

Five calls in one process share warmup, allocation/collection history and
machine conditions. They are correlated serial samples, not five independent
experiments; there is no formal confidence interval. Fork spreads frequently
reach10–40%, with logging_silent84.26% and float51.14%; upstream is mostly
narrower, with exceptions visible above. The current data do not attribute
that asymmetry to GC, scheduling or another mechanism. All values remain.
No automatic repetition or score-driven extension follows.

The broad deficits justify shared-cost work despite uncertainty in exact
ratios and near1 rankings. They do not establish that numeric NOARGS calls,
frame reloads or repeated live-heap scans dominate. Historical ce2ac's0.828243
is retained in the [earlier record](pre-I-first-quiet-timing-plan.md); different
source, eval policy and qualification prevent interpreting the difference
between these two aggregates as a measured GC improvement or regression.
RSS, pause distributions and current hot-phase collection attribution were
not measured by this elapsed transaction.

The [next-slice proposal](pre-I-first-slice-decision.md#bounded-next-question-existing-gc-budget)
is one bounded32-versus64MiB allocation-budget sensitivity using unchanged
qualified products, before generations or numeric ABI work. It reuses the
existing four-row phase probe and GNU time to separate collection amortization
from memory/latency cost. That is a source-only proposal requiring review and
admission; no follow-up experiment or implementation is authorized here.
