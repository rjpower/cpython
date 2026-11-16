# Execution73: diagnosis of the rejected72 candidate

The user authorized this bounded diagnosis after round72's protected screen failed.
It explains the failed experiment; it does not reopen selection or measure a round73
implementation. Selected57 remains golden.

Three matched cycles/instructions pairs per workload compare immutable I
`0dcba75bb3df` with rejected G `c6d54fd3b5d`. The unchanged kernels run with
JIT disabled, hash seed 0 and CPU15. Setup and warmup precede FIFO-enabled
counting. One 499 Hz user-cycle recording per role/workload follows. Each native
invocation has a 60-second timeout. All 16 invocations pass protocol checks;
source, ELF, libraries, configuration and kernel identities match before/after.

| G / I, geometric mean of three pairs | Cycles | Instructions | Cycles/instruction |
| --- | ---: | ---: | ---: |
| logging_silent | 1.223496 | 0.975541 | 1.254171 |
| raytrace | 1.293510 | 1.202562 | 1.075629 |

Logging's regression cannot be explained by a larger total retired instruction
count: it executes 2.4% fewer instructions but takes 22.3% more cycles. This
aggregate does not identify dependency stalls, frontend behavior or a code-layout
cause. Both profiles retain the same main method lookup, Python call, branch,
return and frame-clear paths. Their modest 467/564 samples cannot establish a
specific native instruction as the cause. Do not treat a changed percentage of
samples as an absolute cost reduction.

Raytrace executes 20.3% more instructions. Selected samples include the specialized
constructor/init path; rejected samples include generic native transactions,
binding and initialization. Source review finds a concrete mechanism: round72's
physical placement registry omits existing CALL and arithmetic specializations,
so an otherwise legal specialization can be declined solely because the site
uses a carried result. Untimed snapshots of the unchanged warmed kernel confirm
nine ordinary-handler differences across 58 matching code objects: six constructor
return sites lose `CALL_NO_SELF_ALLOC_AND_ENTER_INIT`, one loses the builtin-fast
call, and two lose float arithmetic specialization. The constructor sites are
`Point.__add__`, `Point.__sub__`, `Vector.__add__`, `Vector.__sub__`,
`Vector.cross` and `Vector.scale`. These are actual specialization losses;
snapshots supply neither hit counts nor the fraction of slowdown they explain.

The corresponding logging snapshots have 159 matching code objects and no
ordinary-handler differences. Constructor specialization loss therefore does
not explain the observed logging regression. Its hardware cause remains open.

The corrective implementation starts from selected57 and leaves ordinary CALL
and arithmetic specialization intact. It must remove a complete generator
transfer/consumer boundary, then pass throughput protection. Fewer instructions
or a smaller isolated helper is insufficient evidence of a performance win.

Evidence under `E59`:

- `execution73-diagnostic-driver.py`: fixed protocol driver.
- `execution73-diagnostic/identity-{before,after}.json`: immutable identities.
- `execution73-diagnostic/results.json`: all invocation receipts.
- `execution73-diagnostic/summary.json`: paired counter ratios and aggregates.
- `execution73-diagnostic/record-*-flat.txt`: samples, with no lost samples.
- `execution73-diagnostic/snapshot-*.json`: warmed immutable/adaptive words.
- `execution73-diagnostic/snapshot-commands.json`: four successful untimed runs.

E59 is the established `python-boil-the-ocean/build/pyjit-link-wave1/`
`register-semantics59` evidence directory. No new upstream ratio follows from
this comparison.
