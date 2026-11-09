# Current PMU plan after the Native32 broad screen

The clean `488e85fa3a8` two-pair golden screen reports nbody
`1.05754`, generators `1.07753`, and coroutines `1.07886` candidate/golden
geometric ratios. Both generator and coroutine pairs exceed `1.067`. The
screen's before/after source and build identities are stable. These elapsed
ratios warrant confirmation, not attribution. The unchanged unpack, float,
deepcopy-memo and pickle cases are now at or better than the golden control;
the upstream screen is a separate comparison.

1. **Confirm the same three cases with the coordinator's four balanced pairs.**
   Retain the seven raw durations, warmup, workload and executable identities,
   CPU 15, JIT-off/hash-seed gates, quiet-load waits and before/after source
   snapshots. If a loss is not repeatable, do not spend a profile on it.

2. **Collect paired user-mode counts for persistent cases.** Reuse the sealed
   FIFO pause/resume boundary in
   `interpreter-parity-20261108/final-diagnostic-perf.py`: prepare and warm the
   benchmark before enabling `perf stat`, then count only its calibrated hot
   loops. For each case, use current candidate `488e85fa3a8` against golden
   `0765e9be47f`, three alternating role rounds, initially the existing
   `cycles:u,instructions:u` group. Report both counts per benchmark loop and
   their ratios, plus CPI; an elapsed ratio does not say whether extra work or
   stalls dominate. Add the existing `branches:u,branch-misses:u` group only
   if the first counts leave a useful branch question. Preserve each raw perf
   JSON, command, child result, calibration, load wait and identity snapshot.
   Reject an invocation below 99% event running or with a failed FIFO/child,
   and reject the campaign if postflight identities change.

3. **Sample locations only for a persistent, counter-supported loss.** One
   `perf record` per role/case with the same five-second warmed FIFO interval,
   user-mode cycles and retained ELF/build-ID files is enough to identify
   candidate and golden hot symbols. Keep lost-sample and build-ID checks.
   Compare samples within each workload, then inspect the corresponding
   current emitted C/assembly and route counts; a sample share is neither a
   per-handler speed ratio nor proof that Native32 caused the difference.
   Generators recursively use `yield from` over a tree; coroutines repeatedly
   drive recursive `await` through `.send(None)`. Their common suspension and
   frame paths are a hypothesis to test, not an assumed shared cause. Nbody
   remains a separate arithmetic/slot-path question.

The sealed protocol's CLI has fixed cases/roles; its shared invocation function
can use an explicit roster. The older `capture-nbody-perf.py` accepts a dynamic
`--source-head`, but only orchestrates nbody and snapshots a narrower input set.
A coordinator-reviewed case/role orchestration is required before these three
cases can be run. No benchmark kernel, timing boundary,
runtime source, native binary or extension module should change for the
comparison. The prior `c142` PMU and cycle records cannot substitute for
counts or locations at `488e85fa3a8`.
