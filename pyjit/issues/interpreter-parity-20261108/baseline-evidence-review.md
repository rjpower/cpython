# Independent review of target baseline evidence

2026-11-08. Host-only audit of retained `baseline-target/` data and collector;
no measurements were run. Twelve raw stdout files each contain exactly the
seven positive timing values and metadata recorded in `commands.json`; all
children returned zero, stderr is empty, each used 60,000 loops, CPU 15,
JIT-disabled metadata and fixed hash seed. Execution order alternates
upstream-first and candidate-first across six rounds. The collector takes the
arithmetic mean of each seven-value set, then forms candidate/upstream ratios.
Independent recomputation exactly reproduces 1.648208468, 1.665204586,
1.636743761, 1.646092266, 1.658995259 and 1.689002917 and their geometric mean
**1.657289423**. The `summary.json` min and max also match. This supports the
reported baseline, not parity or a statistical significance claim.

The generated native driver retains the original module loader, setup, warmup,
seven timed `module.run` calls and `perf_counter_ns` boundaries; it adds
pre-timing CPU/JIT/method-admission assertions and metadata. `logging.py`
selects the real silent logging workload and checks the stream is empty on
each invocation. The collector removes inherited `PYTHON*` and `LD_PRELOAD`,
sets `PYTHONHASHSEED=0` and `LD_BIND_NOW=1`, records load and enforces its
stated load thresholds. Pinning and CPU checks do not prove absence of
interference; load averages are coarse. The candidate and upstream are separate
sequential processes, not simultaneous measurements.

Before/after identity snapshots are byte-equivalent as parsed JSON, have no
reported errors, and identify clean candidate `0953757f4e7` and upstream
`3a440b65695`, GCC 15.2.0 release builds, JIT unavailable/disabled and recorded
module origins/build IDs. Every recorded driver-source SHA-256 matches the
current corresponding file. Retained candidate and upstream executable copies
match current build executables by SHA-256, and their retained ELF build IDs
match the snapshot. This is a recorded build/source identity check, not an
independent reproducible-build proof or a hash of every dynamically loaded
library. The snapshot's historical module-roster logs report 117/116 modules;
they are not fresh test runs for this campaign.

Only one six-round baseline is present here. The two-campaign parity rule is
not met or being tested; the factor does not identify a cause or establish a
performance benefit for any proposed mechanism. Six paired ratios span
1.636744–1.689003, and the first candidate sample set is visibly slower than
later ones; retain the full values and order for interpretation. No native
workload, build, or source mutation was performed for this review.
