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

## Full observer correction, 10:33 PDT

The first full run reports native SUCCESS but remains qualification FAIL.
Global addSkip captures67 extra skips from inner meta-test result objects;
the original rows lack identity sufficient for retrospective classification.
No threshold or module exception repairs that evidence. Preserve the attempt,
original helpers and c9e plan without changing their outcome.

Independent peer and coordinator clear the separate full-only v3 observer:
exact scheduled load/runner objects before import, nesting guards, actual
returned outer result and weakref-backed monotonic identity tokens. It does
not keep inner result objects alive. Exact outer skip multisets and statistics
must match returned results; all raw and outer protected skips still fail.
The new phase binds module/configuration/selection/source/native fields to
the actual worker. Counter domains are checked before summing, and every
driver/worker needs a main-interpreter record.

Final plan SHA256
`14739d4144d25df076bded0fa5a174a8ffbcb737fbd70058d30d07b3ab7c6827`;
proposal `74d50d8b7fcb275d1c944dd616388b40542bca9c893b67d9feee1ab2c7a453fd`;
validator `7a5c69a7a96f951d3a895fa562f11a5063f9b3a506fe2036a841f93bbe4f1945`.
Four observer controls,45 validator negatives and legitimate class-skip,
normalization/nested-result positives pass. Prior versions remain preserved.
Both564-module rosters, filters, ceilings and four matrix helpers are unchanged.

The read-only release640 supplement99e1c5ca checks every retained state/root
hash,1,480 required PID occurrences and9,141 records for main0/domain validity.
This adds no runtime coverage. Apply the equivalent supplement after debug640.

Fresh release-full v3 passes native and audit:51,130tests/3,458outer skips,
67 separate inner events,565 actual driver/workers,63,597collections and zero
fallback. Receipt195c64bee179667ab1b71c08f7b5c122da5c8cf9ce4dfbee1b0e178092d2903d
matches coordinator inspection. Release stress11 subsequently passes3,061tests/
100skips,44,828collections and zero fallback; receipt68feb053300b33f745379cc5fe59e6436e4c82139b61eedbfbe3d2089175cbf4.
Unpinned totals remain report-only;13 stress-driver visits are not exactly
attributed and must not all be labeled interpreter swaps.

The reviewed debug armer waits for exact640/17a8 completion and its main0
supplement, then runs only normal full, kill full and stress11 under14739d,
with candidate-after20 explicit. Its guard checks frozen armer/replay hashes
before waiting and before execution. Stop on any failure. Full100k remains
after debug fulls; no overlapping full suites or new filter is authorized.

This qualifies build-owned full tools only. Shared verify_launch.py's global
skip observer and future run/gate orchestration still need this semantic
correction and their own qualification; the landed targeted component is not
a shared full-profile result.
