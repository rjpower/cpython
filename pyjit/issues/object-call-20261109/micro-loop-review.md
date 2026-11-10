# Micro loop-count override review

2026-11-10, read-only source review of the optional `--loops` addition to
`measure.py`. Approved: the argument must be positive and is restricted to the
micro group. A single count selected before the role loop is passed to both
native commands and recorded in each result row. Without the override, registry
counts remain unchanged. Benchmark bodies, warmup and timing boundaries are
unchanged. No native tests or result audit were performed in this review.

Longer intervals are justified for checking the reported 1.6–2.0 ms class-load
samples. Retain actual loop counts with the results; treat these as separate
campaigns rather than pooling short and long samples. The proposed 20,000 class
loops and 2,000 keyword loops are measurement parameters, not benchmark-body
changes. Freeze the helper before starting either campaign or the full roster.

The coordinator reports editing this helper during `basic94738`. Because the
helper is included in `driver_sources`, a before/after hash mismatch must reject
that capture for admission. Preserve the failure and raw data as diagnostics;
do not restore an earlier helper to conceal the mismatch. Repeat the affected
campaign after freezing the source. This source review does not independently
validate the previously reported short-interval ratios.
