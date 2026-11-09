# Target measurement source review

2026-11-08. Independent review, round 1. Source inspection only; no native
workload, build, benchmark, event capture or commit was run by this reviewer.
Read the CPython AI policy, project guidance, plan, existing measurement
review, collector, unchanged kernel driver and reused checkpoint snapshot code.

**Verdict: reject until M1 is resolved.** The elapsed-time protocol itself
matches the agreed design.

## M1: retain failed identity snapshots before rejecting them

`measure-target.py:74–83` asserts that snapshot errors are empty inside
`snapshot()`, before writing the returned object. Initial snapshot collection
also precedes the `try/finally`. A dirty checkout, wrong executable revision,
failed module probe or module-roster mismatch therefore leaves neither the
failed identity snapshot nor `status.json`. The postflight path has the same
raw-snapshot loss, although it retains an exception string. This defeats the
explicit requirement to retain failed attempts and their identities.

Return and persist the raw snapshot before checking its errors. Put initial
identity validation within protected failure reporting, or explicitly emit
the equivalent preflight failure artifacts. Preserve the detailed postflight
snapshot even when its validation fails. No timing is required to verify
this control-flow fix. A failed attempt must still require a new output
directory; do not replace failed evidence.

## Accepted protocol and bounded provenance

The existing loop alternates AB/BA with three starts in each order for six
rounds. Both roles use the same registry loop count, unchanged module loader,
setup, warmup and timed `module.run(loops)` body. Each process emits seven
durations; its arithmetic mean forms the paired candidate/upstream ratio;
the summary takes the geometric mean of those ratios. Affinity and JIT
assertions occur in native children immediately before each timed call and
after the last one. Metadata and assertions remain outside the measured
interval. The environment removes inherited Python settings and disables
hash randomization. Load tuples, commands, raw stdout/stderr, per-process
metadata and source hashes are retained for completed timing attempts.

The reused snapshot code checks clean source HEAD against executable
`sys._git`, release/JIT state, configured source roots, interpreter and selected
extension ELF IDs, checked module rosters and module-log hashes. Before/after
identity comparison and benchmark/collector/driver hashes detect changes at
those boundaries. This is a bounded provenance check, not proof that every
compiled translation unit derives from the named source or that no artifact
could change and be restored between snapshots. Read the native gates as
runtime checks, not full source-to-binary attestation.

The driver replacement currently matches the exact reviewed driver text.
Its timed-loop replacement is count-checked and the generated file is
compiled before execution. A future driver edit must trigger source review;
adding match-count checks to the other two replacements would make that
failure explicit sooner, but is not a blocker for the current exact source.

The collector produces timing summaries, not a parity decision. The
coordinator must enforce six rounds, fixed loops across two new campaigns,
GM at most 1.00 and every paired ratio at most 1.02, and report raw spread.
Incomplete or failed postflight captures cannot establish parity.
