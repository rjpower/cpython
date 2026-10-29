# ROOTS2b full-suite harness review

Coordinator review, 2026-10-29, of build-owned plan SHA256
`62988ee16b00a38502e7d833cd53f8f2d3e06a7ab78ab886a7e52d7f0aaf2df8`.
Decision: revise the new full-run validator before execution. No change to
the running matrix, frozen8ce source or its four helpers is requested.

The 564-module release/debug rosters match current default findtests and
Regrtest.find_tests with no selected tests, exclusions, PGO or fromfile.
Full normal/kill commands retain the existing remote selector alone. Stress
commands use the explicit approved policy. Existing driver/worker adapter,
timeouts, locks, caches, exact PID/config/seed results, positive collections
and zero-fallback requirements are appropriate. The qualification registry
diff changes only the approved full-stress availability/applicability.

Two coverage gaps remain:

1. The validator permits a PASSED module with absent statistics, or one
   silently empty module among nonempty ones. Require valid statistics for
   PASSED/ENV_CHANGED. Reject absent statistics or zero tests and zero skips.
   Preserve legitimate whole-module resource skips and recorded setUpClass
   skips; a zero tests_run count alone does not prove a module was omitted.
2. Skip events are retained but not checked against the 32 protected
   reclamation selectors. Reject exact/base-subtest skips and whole-module
   skips covering a protected case known available on this source/build.
   Normalize default module names when matching the inventory. Do not apply
   the narrow matrix's 2,041-case count to the whole suite.

Add negative replays against the actual qualified witness for both gaps,
then preserve the first plan and freeze a new plan/validator hash. Full runs
still wait for each build's complete repeated qualification. O1 full suites
are candidate qualification, not prior global adoption. Cext, regeneration,
traversal and optional-extension checks remain separate gate3 obligations.

Follow-up09:33 PDT: revised plan SHA256
`c9e05359d7afd3a1d98f35541b66a0065d83886155455ae7ded8cc4441b3b505`
is cleared for the approved profiles after each build's repeated qualification.
New validator checks present/nonempty statistics with recorded fixture-skip
accounting, all32 protected cases/subtests/setup containers, and whole-module
skip scope with normalized names. Twenty negative replays and the class-skip/
name-normalization positives pass against retained evidence. Plan/helper
hashes and both564-module rosters match. Prior plan/helpers remain preserved.
The four running matrix helpers and native8ce source remain unchanged.
