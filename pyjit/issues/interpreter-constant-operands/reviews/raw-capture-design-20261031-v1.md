# Raw instruction capture design review, 2026-10-31 v1

Disposition: **ACCEPT for the bounded twenty-minute host implementation only.**
This accepts design SHA256
`770b5feaec0f4e4af8380235821a0ac3fcaa7af09d199123c927add6b93bc7cb`.
It grants no target, perf, timing, compatibility, qualification or landing
acceptance. Runtime source and images remain frozen; performance remains open.

The reviewer inspected the actual primary runner, SHA256
`7ec1cdf621835b84b273219bbde1966b711032c81effbb15268b377af614eaf8`,
its existing test harness and benchmark contract. `perf_instructions` currently
reuses scratch/perf.txt, `measure_instr` keeps only normalized differences, and
TemporaryDirectory removes the original files. The proposed evidence addition
addresses that loss without changing the measured work, process order, loop
counts, median, normalization, skeleton subtraction or scalar count interface.

The initial design review identified four concrete details, all incorporated
in the accepted revision: exclusive journal creation before build_info invokes
the target; rejecting a journal/report path alias; preserving complete streams
and perf text before the shared truncated error formatter, including explicit
spawn/missing-output failures; and linking ordered raw integer low/high pairs
to numbered invocations while retaining the existing skeleton `raw` meaning.
The mock tests use a separate module because importing existing test_run.py
executes perf availability probes in decorators.

A start record is closed before each perf child and its finish record is
closed after return. Nonzero exit, invalid text, missing output and stale
scratch cannot yield an inherited count or a successful report. An interrupted
attempt may leave a start without a finish; that is retained evidence, not a
sample. Failure records survive scratch cleanup and refusal preserves an
existing journal. No general callback, cache policy or provenance framework is
needed: a shared subprocess result-error formatter can retain current reporting
while the perf caller records the complete CompletedProcess first.

Implementation review must check these behaviors against meaningful mock-only
tests, including invocation order/IDs and unchanged normalization. No target,
perf subprocess or measurement was run during this design review. The eventual
quiet manifest must bind the reviewed runner revision, journal hashes, workload
bytes and both source/image bridges before measurement authority is considered.

Reviewed UTC: 2026-10-31T22:45:41Z.
