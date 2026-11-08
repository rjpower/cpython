# Pyperformance prelaunch quiet wait

Proposed 2026-11-06. The completed kernel campaign is separate and remains
unchanged. The first v3 pyperformance subset stopped before launching its
ninth command because the instantaneous prelaunch load exceeded 4. Restart
only the subset in a fresh directory, excluding the partial subset from final
aggregation.

For timed pyperformance commands, `run_command` starts a monotonic deadline
from its supplied remaining timeout. Before launching, record each full load
tuple and timestamp in the existing JSONL journal, waiting at most ten seconds
between observations until one-minute load is below 3. The existing
`load_guard()` remains the final prelaunch check; if load rises to 4 or above
at that point, record the rejection and return to the wait within the same
deadline. If the deadline expires, journal the timeout and raise without
launching. Pass only the remaining time to `communicate`, so waiting cannot
extend the caller's fixed 30-minute window. No command is retried after it
starts; execution, result validation, and failure semantics remain unchanged.
RSS callers supply no timeout and retain their existing immediate guard.

Fixtures should simulate an initial busy host followed by quiet load and check
journal observations, final guard, exactly one launch, and reduced timeout;
check expiry during the wait launches nothing and records all observations.
Also test a load spike between quiet sampling and the final guard. No native
benchmarks or builds are needed for these fixtures.
