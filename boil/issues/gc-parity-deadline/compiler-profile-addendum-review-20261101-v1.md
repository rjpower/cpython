# Independent compiler-profile amendment review

2026-11-01. **ACCEPT a narrow predeclared amendment, with the selection rule
below fixed before any VM0 elapsed value is obtained.** This is a protocol
ruling, not execution, backend selection or parity acceptance. The coordinator
reports that VM0 has native feedback only and no elapsed scores. Preserve
`budget.json` SHA256
`1bd70fa55b2a0fa26e9b6f27cf8d7cb720090e2286c78ebc6806e9678f9a3f0f`,
measurement protocol `7f8c2769137dd94ca2cec79eb82ec0d537fb881f40c3332ddc236dc3e937fda8`
and VM0 revision3 `ecaa2e25798e8039fe17bf22a8aa921045a2b15f60da1ae80dfb7c8a64964d06`
unchanged. Their separate amendment must name these snapshots. The original
user target concerns the coherent interpreter/collector package; it does not
require attributing parity to GC alone. Allowing a supported compiler/backend
on **both** packages is therefore defensible. Accepting a Clang fork against
GCC upstream, or changing profiles after their parity scores, would not be.

Use this minimal deciding rule:

- Freeze one deterministic selector now: select T only if both T/C and T/G
  clear every already-declared VM0 usefulness/variability/control gate;
  otherwise select C only if C/G clears those gates; otherwise retain G.
  This makes tail earn its incremental complexity and combined deployment
  result. Do not pick a backend by the best individual row or relax a gate.
- A selected Clang profile is fixed for the remaining GC timebox. It admits
  exactly one matched upstream compiler/backend profile, not subsequent
  compiler, flags or baseline shopping after a failed full-29 score. Ordinary
  separately reviewed runtime hypotheses may still change the candidate as
  allowed by the original budget; each requires a fresh coherent binding.
- Build upstream **unchanged** at full commit
  `321925c92c9e67864cafa1566840927f6f07e90a`, with the exact selected Clang
  implementation/dependency hashes and the same computed/tail configuration,
  effective O3, GIL and disabled debug/stats/JIT/tier2/PGO/LTO policy. Its source
  actually supports `--with-tail-call-interp` and checks musttail/preserve_none
  (`configure.ac:7973`, `Python/ceval_macros.h:82`). Support and actual emitted
  configuration still require native verification. Match common optimization
  policy and disclose all expanded arguments; keep architecture-required flags
  in their real package cost. Do not transplant fork ABI/GC code or impose
  fork-only diagnostic interfaces on upstream to create an artificial match.
- Require the selected fork's existing focused debug/release semantic/root
  feedback and the fresh upstream's build/config/import and applicable ordinary
  semantic checks. Bind each image's own stdlib/extensions. No new stats census,
  duplicate GCC campaign or attribution ablation is needed before elapsed.
- Measure the actual coherent candidate directly against that fresh matched
  upstream on the original 29 rows/loops, three fresh-process rounds and five
  serial values per arm/row: 174 scored processes and 870 values. Retain the
  original rotating order, aggregate GM<=1 and each-round GM<=1, variability,
  RSS, correctness, raw-origin and cleanup requirements. Pilot speedups cannot
  be composed with earlier GC results. No subset, retiming or adaptive samples.
- Preserve the original start, milestones, registry, hard cutoff
  **15:14:44 UTC** and RC fallback. Configuration failure, insufficient remaining
  time, missing/invalid/inconclusive data or unresolved critical correctness
  does not establish parity. Final qualification remains required before
  integration; compiler selection grants no clock extension.

The main throughput risk is spending the remaining window qualifying and
measuring several upstream profiles. The one-time selector and one fresh
matched profile avoid that expansion. If this selector is unsuitable, freeze
another equally deterministic rule **before** VM0 timing; do not defer the
choice until scores exist. The coordinator must explicitly publish the
addendum before activation. This review edits neither the budget nor source
and authorizes no build or measurement.
