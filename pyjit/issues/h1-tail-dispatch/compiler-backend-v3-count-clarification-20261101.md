# VM0 revision 3 stale-process count clarification

2026-11-01. This clarifies the physical process count in frozen revision 3
`compiler-backend-plan-20261101-v3.md`, SHA256
`ecaa2e25798e8039fe17bf22a8aa921045a2b15f60da1ae80dfb7c8a64964d06`.
The accepted plan and its review remain unchanged. Exposure is unchanged.

The existing canonical `raw_stale_probe.py` starts **one target child per stale
row**. Its CHILD executes all six dead_* controls, each100 times inside the
original sorted-key callback placement. Thus the five proposed stale rows mean:

- **5 actual target children**, each running six controls;
- **30 control-case executions** and **3000 total internal iterations**;
- separate unscored host wrappers, which receive no target/test-method credit.

“30 child controls” in the plan means control executions inside children; it
must not be reported as30 physical children. Earlier author messages using
that physical-child label were wrong. Exact CHILD/function bytes, source hashes,
actual PID/state/root witnesses and retained streams remain mandatory. Other
focused-test inner children are counted only from actual telemetry, separately
from these five stale target processes and ordinary driver/workers.

This correction changes no method, source assertion, row, loop count, runtime,
configuration, deadline or execution authority. Future helper manifests and
their README must bind this clarification and use these units explicitly.
