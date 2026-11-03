# Verification manifest stage 1: coordinator review

2026-10-29. Author: ROOTS lane; reviewer: coordinator. Scope is the
registry, validator, plan display and safety controls. No profile executes
and no numeric gate changes in this stage.

Verdict: LAND, after the two corrections below. Execution remains pending.

The registry preserves full-suite selections, named targeted sets, actual
state requirements, exclusion owners and removal conditions. Unknown
fields, references, modes, empty selections and duplicate JSON keys fail.
Concrete-case validation checks membership and test-source hashes. Display
explicitly says that execution and actual binary-state checks have not
occurred. Historical reclamation coverage remains distinct from the
expanded 31-case, 11-module qualification matrix.

Corrections made before acceptance:

- Narrow the ASan exclusion from the shared method name to
  `*test_capi.test_mem.AddressSanitizerTests.test_buffer_overflow`. QUEUES
  established that the five additional allocator cases sharing the name
  pass. Retain the original unfiltered ASan failure as a failure.
- Document `case-inventory.json` as the normalized `--inventory` input,
  distinct from the raw `reclamation-discovery-final.json` discovery log.
  Passing the raw log fails schema validation, as required.

Independent checks: all seven safety-control groups pass, and validation
against the actual normalized discovery inventory passes for 15 profiles.
Exact commands, output and the four reviewed file hashes are retained in
`build/manifest-20261029/review-stage1.log`. These checks use system Python
through uv and do not claim any CPython runtime coverage.

The proposed execution hook may proceed as a separate implementation.
Keep the existing gate runner's slots, locks, timeouts and cleanup; do not
introduce arbitrary manifest commands or reinterpret narrowed runs as full
suites. The actual driver and workers must check both getters at start
and end, preserve interpreter flags and failures, and retain scheduled
test/filter identities. Missing hooks, getters, end records, counters or
required prerequisites must fail. Validate ignored-environment behavior,
lasting state changes, worker errors and missing evidence before any
profile is treated as an executed gate. Deliberately isolated children
retain their own hazard-specific assertions. Start/end witnesses do not
prove that every transient state within a test was unchanged.
