# VM0 compiler/backend plan: independent coordinator review

2026-11-01. **ACCEPT revision 2 for held host-helper preparation.** This is
independent of the plan author. It grants no compiler, target, benchmark or
native activation. Exact plan SHA-256:
`eb51a54a0a56fbadfffeda4e22901157f6664d3ecf5c462de8ef06db6810a6b7`.

The unchanged-source GCC-computed/Clang-computed/Clang-tail contrasts distinguish
compiler plus maintained source-selected policy from the same-compiler backend
effect. Keeping the current four-state tail ABI makes this an informative
control before the separate frame-argument change. No current GC source,
handler semantics, publication rule or workload changes in VM0. Source
inspection confirms the named runtime/build inputs are unchanged between e9
and 417; actual complete product/configuration/tool bridges remain mandatory.

Revision 2 resolves both earlier findings without changing exposure:

- Historical policy is PIE/ET_DYN without shared libpython, not a fully static
  executable. Preserve the observed native interpreter/library closure and
  effective per-compiler flags rather than treating requested configure flags
  as proof. Clang must not be admitted under a GCC-only toolchain validator.
- The stats method deletes its TemporaryDirectory. Capturing its original
  child's one raw file after typed rc0 and before returning the same
  `_PythonRunResult` is a valid observation point. `script_helper._assert_python`
  returns that typed object. Exact case/code/directory binding, original result
  identity, capture before cleanup and raw mode1/handler-ID/counter checks must
  be demonstrated by host controls and the actual TD worker. Configuration or
  a successful parent alone earns no dispatch-counter witness.

All twenty-one focused method IDs exist in the frozen test source. Across four
images they give eighty-four invocations, plus the six actual TestPyStats
methods only on TD: ninety invocations. The tail-walk method's actual source
skips only when no precise tail section exists; the two nominated computed-arm
skips do not count as tail coverage. T and TD must execute it. The unchanged
six stale bodies across G/C/T OFF and TD OFF/KILL give thirty cases and 3,000
internal iterations, with actual worker/child root and cleanup evidence still
required. Source-derived child manifests must be reviewed, not guessed from
outer counts.

Three fresh Clang builds and reuse of the exact historical GCC image are
reasonable finite feedback. A reuse mismatch stops that arm. The shared
sixty-minute window includes all rows; row ceilings do not add extra time.
Unknown linked outlines, missing annotations, skips or native failures stop
rather than relaxing root audits. A selected configuration still needs full
landing qualification. The separate thirty-minute quiet plan retains the
original four workloads, twenty-four scored processes/120 serial calls and
twelve one-loop origin witnesses. Its two-round primary/noise/regression rules
are explicit; no PGO, flags search, text-size veto or automatic retiming is
authorized. O3 results do not settle the separately documented PGO question.

Preparation may proceed while GC diagnosis/source work uses separate files.
Before native activation, the coordinator must fit actual remaining work plus
qualification and integration into the stated boundary, or defer VM0 explicitly.
The structural GC prototype and oracle correction are not bundled into this
comparison. All future helpers require exact independent review before use.

Host-only evidence: `build/vm0-plan-review-20261101-v2/receipt.json`. It binds the
plan, source IDs, relevant test/ABI/build/helper inputs and finite counts. No
compiler, target, kernel import or workload call ran during this review.
