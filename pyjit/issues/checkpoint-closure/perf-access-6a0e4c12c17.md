# Perf access probe for the steady-state investigation

Both probes ran after the accepted elapsed-time, RSS and initial statistical
profile collections. They did not modify the measured runtime or host policy.
The initial host setting was `perf_event_paranoid=4`.

## Initial denied probes

Hardware counter probe:

```sh
perf stat -e cycles:u,instructions:u,branches:u,branch-misses:u -- /usr/bin/true
```

Exit 1: no supported events; access to performance monitoring is limited by
the kernel's policy and the process lacks the required capability.

Software-clock sampling probe:

```sh
perf record -e cpu-clock:u --call-graph dwarf -o build/perf-access-probe-6a0e4c12c17.data -- /usr/bin/true
```

Exit 255: failure to open `cpu-clock:u` and no events available for recording.
The same kernel access restriction applies. This is an operating-system
restriction, not a tool approval rejection or a failed interpreter workload.

At that point the investigation used the retained depth-one SIGPROF captures,
exact executable identities, sampled instruction addresses, source mappings
and disassembly. It cannot report hardware cycles, retired instructions,
branch misses or runtime caller stacks. Compile-time inline information from
DWARF is not a caller stack. Preserve this limit when estimating benefits.

A future host with perf access can distinguish instruction-count overhead
from cycles per instruction and branch/cache effects. Reuse the same frozen
builds, hot regions and rotating benchmark protocol for that comparison.

## User-enabled access

The user subsequently changed `perf_event_paranoid` to 2. A fresh read confirms
2, and the same hardware probe now exits zero with cycles, instructions,
branches, branch misses and cache references. Cache misses were not counted
in that very short six-event `/usr/bin/true` probe. Actual collection must
split events into supported groups and check enabled/running time and counts;
an uncounted event is not a zero.

Hardware collection subsequently completed: 54/54 counter runs, 18/18 empty
control calibrations and six user-only DWARF recordings. Each counter pair ran
100% of its enabled time; all final identities match. The
[performance report](../../bench/results/checkpoint-6a0e4c12c17.md) records
the findings and limitations. The original denied attempts remain historical
evidence, not current restrictions.

The first DWARF recordings failed before starting their children: despite
`cycles:u`, perf's default call-chain scope requested kernel call chains,
which paranoid 2 disallows. An isolated user-only probe passed. Adding
`--user-callchains` then allowed all six recordings without another host
policy change. The failed recording directory is preserved separately from
`perf-record-6a0e4c12c17-v2/` and supplies no measurements.

The first empty-control run also exposed this host perf's five-byte
`ack\n\0` response rather than the documented four-byte `ack\n` response.
The collector now accepts exactly either complete response; raw response bytes
and acknowledgments are retained. No failed calibration entered the results.
Source/build/module postflight checks are unchanged and error-free. Neither
failure was an automatic approval rejection.
