# Integration B: overlap initialization

Runtime commit `ba37cc10058`, parent `8a8fdc91368`. Release, debug and stats
builds pass with 117 modules each. The corrected focused set passes 1,013
tests on each release/debug profile (12/11 skips); value API lint checks 306
units with zero failing findings. [Cold review B](cold-b-review.md) accepts
the integrated source with no blocking correctness finding.

The reviewed `frame-overlap-init.patch` reuses the known-NULL non-argument
prefix of an overlapping caller window when initializing child homes. It
captures the previous frame extent before changing stack frontiers, asserts
that the skipped prefix covers transferred arguments, checks skipped
non-argument slots for NULL in debug builds, zeros only the new tail, and
continues filling constants through the normal ownership path. The source
artifact received independent acceptance in
[frame-overlap-init-review.md](frame-overlap-init-review.md); that review did
not establish build, runtime, or performance results.

The patch also carries the two bounded Integration A cold-review corrections:
the provider's decline status is accepted by the assertion, and the overlap
comment names the range above the argument prefix precisely. Allocation
schedules GC collection; it does not synchronously execute
`gc.callbacks` inside `PyDict_New`, so no such callback test is claimed as
coverage of adapter reselection.

The first constant test selected no hidden slot and failed; its corrected
version selects the established hidden `7` and replaces it with a mortal
object implementing `__rsub__`. It checks callback behavior and stable ownership.
The failed run and corrected passes are both retained.

The three-round seven-kernel screen is mixed: B/A candidate elapsed-time
geomean is 1.0034. Logging is 1.0333, Richards 1.0373 and generators 0.9455;
upstream drift and short samples prevent a general performance conclusion.
This removes redundant clearing but establishes no overall throughput gain;
the performance benefit remains unproven.
GCC disassembly retains a conditional tail memset and bypasses it when the
proven prefix covers the child. Non-overlap initialization remains separate.

Raw evidence is under the common experiment root, in `integration-b-basic`,
`integration-b-gcc` and the named build/test/lint logs. The retained GCC ELF
and disassembly permit comparison after C. The full suites and broad throughput
set remain reserved for the coherent final integration batch.
