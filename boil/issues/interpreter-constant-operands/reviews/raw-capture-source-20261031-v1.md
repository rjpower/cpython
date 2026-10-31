# Raw instruction capture source review, 2026-10-31 v1

Disposition: **ACCEPT the exact focused tool change.** No blocking finding.
This clears the coordinator to commit the runner and prepare the fixed quiet
manifest. It grants no native smoke, perf, timing, candidate performance,
qualification or landing acceptance. The runtime candidate remains frozen and
its performance decision remains open.

Reviewed source SHA256:

| File | SHA256 |
| --- | --- |
| boil/bench/run.py | fc1c6931cd5417dced610ad110e9887fe4cb32c3463c460f599dfae411b375b8 |
| boil/bench/tests/test_perf_records.py | 8eadbf87488470e6f06fa5fbc83d6fe7dca7b811a14d43e9506ea4913c78f0ec |
| boil/bench/README.md | ae63dbd808cb1b3d95d0ad299e0e8b3640e0dea01871eccb9b5b35a6648cb046 |

The frozen author capsule is
`build/constant-operands-planning/raw-capture-v1/source.json`, SHA256
`a26ad9df751823adc7f1d4ca1de9e065ac20783e815ca607723aac50b35eb7d8`.
The change implements accepted design770b5f without a new capture framework.
The common subprocess error formatter is factored out; other callers keep
their existing behavior. The perf caller keeps complete captured stdout,
stderr and original perf text before raising the shared error, and its finally
block closes a finish record containing the count or error. Spawn and missing
output errors remain errors with retained records.

The owned scratch perf.txt is unlinked before each invocation, so a failed
subprocess cannot consume an earlier count. A numbered start is closed before
the child runs. The journal is exclusively created before the build probe;
resolved journal/report aliases are rejected. Existing journals are preserved.
Successful report metadata binds the original journal path, digest and number
of invocations. The scalar count API, child argv/environment, warmup/sample
order, loop counts, normalization, median and skeleton subtraction remain
unchanged. Raw integer pairs map to low/high invocation IDs and preserve the
existing skeleton `raw` field.

Independent verification ran only
`uv run --no-project --offline --python /usr/bin/python3 -- python -B -m unittest discover -s boil/bench/tests -p test_perf_records.py -v`.
All nine mock-only methods passed, with log SHA256
`25ec6b90341cb583f2bd21a3ad642c11707ff145591e801d9b4590f4ffd829d0`
at `build/constant-operands-review/raw-capture-v1/mock-tests.log`.
They cover pair/normalization/ID mapping, complete failure streams, invalid and
missing counters, spawn failure with stale scratch, journal survival, the
scalar API, refusal before build probes, and successful metadata binding.
The dedicated module imports run.py directly; the existing test_run.py perf
decorators were not imported. The focused diff check passed and frozen file
hashes still match. No actual target or perf invocation ran during this review.

The journal is evidence capture outside the measured child, not proof that
measurement conditions are quiet or that the candidate is faster. The eventual
manifest must pin this accepted driver, workload bytes, both images/source
bridges and raw journal outputs under separate measurement authority.

Reviewed UTC: 2026-10-31T22:51:22Z.
