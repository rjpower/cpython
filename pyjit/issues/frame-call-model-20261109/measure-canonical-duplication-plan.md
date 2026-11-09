# Canonical duplication component measurement

Proposal for a coordinator-run driver; no candidate execution or runtime edit.
Adapt measure-r4-r5.py's retained-executable identities and the current
execution-boundaries measure.py timing protocol, using checkpoint-closure's
existing identity/probe helpers. Do not change the benchmark registry, kernels,
loop counts, warmup or timing boundaries.

CLI: before/after executables, before/after full revisions, runtime source,
baseline manifest, sealed baseline identity, before/after module logs, output
directory, cases (default nbody,logging_silent,pickle),
four balanced rounds and CPU15. Resolve both executables to distinct inodes in
the same build directory. The retained before must match baseline.json's path,
source_commit, SHA256 and ELF notes; the manifest's complete 213-DSO path/hash set
must still match that shared build directory. Do not compare the manifest's old
`python` path hash to the newly rebuilt after executable.

Preflight probes both native revisions against their explicit CLI revisions,
requires clean source HEAD exactly equal to after revision, JIT disabled and
method-JIT unavailable, release/debug configuration and compiler/config equality,
and identical successful import origins. Require native GCC version equality;
record CONFIG_ARGS/CC/CFLAGS/OPT and the current Makefile/configuration files.
The sealed seventh-engineering-upstream identity records the actual 417
candidate configuration. Require its source/native revision and build ID to
match the retained baseline, then compare both current probes to that recorded
configuration. Preserve the 117-module, zero missing/failed-import gate for both
roles using the existing module-log helper. Hash both receipt inputs and logs.

Snapshot before and after: both complete ELF identities, baseline file/hash,
clean source identity, every runtime Lib Python file, every benchmark Python
file, all shared extension DSOs, native import probe manifests, driver/helper
hashes and build configuration inputs. The benchmark child also reports its
complete imported file/hash manifest after untimed work. Check equal imported
inputs between roles for each case and stable identities across rounds. Do not
hash modules inside a timed interval.

Use the unchanged internal driver loader/setup, one warmup call and seven timed
calls. Add CPU15, hash seed0, JIT-off and native revision gates immediately before
each timing boundary and after timing. Pin the complete native child with
taskset. Alternate before/after then after/before per round, four rounds. Keep
the existing quiet-load entry<3/exit<4 and process-group timeout/deadline guards.
Retain raw stdout/stderr, commands, seven durations and per-round medians.

Summaries contain explicit before and after median ns/loop for every round,
paired after/before ratios, median-of-round-medians and ratio of those medians.
Keep all raw timing rows for later PMU comparison. Failed preflight, native gate,
import/hash drift, load/timeout guard or postflight invalidates the campaign.
Always write rejection/status evidence; never report a successful summary after
a failed identity check. No PMU/FIFO implementation is included in this driver.
