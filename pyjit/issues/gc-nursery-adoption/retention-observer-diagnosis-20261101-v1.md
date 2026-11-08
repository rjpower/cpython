# Richards retention witness diagnosis

2026-11-01. Host-only source and retained-output analysis; no new target,
workload, timing sample or collector change.

The cost campaign stopped on `score-r0-richards-0-historical`. Its native
child returned zero; `paired.normalize` then rejected
`retention_bound_passed: false`. Recorded tracked counts were
`[12112, 12168, 12224]`; live bytes were `[3181888, 3185088, 3188288]`.
Final-minus-first growth was 112 objects and 6,400 bytes. The object bound
failed; the unchanged 16 KiB byte bound passed. This failure is in the
historical e9 control and is not evidence that cadence leaks.

The exact kernel is `boil/bench/internal/kernels/richards.py`, SHA-256
`c5e9375d4d1070899ab79dbfd9301329435d3918871e762be3468cb565b88a74`.
`Task.__init__` sets `self.link = taskWorkArea.taskList` and then publishes
`self` as the global head. `Richards.run` resets counters, but not that head.
Consequently later iterations keep all prior task chains strongly reachable.
Each iteration constructs six tasks, six task records and eight packets;
each packet constructs a distinct four-element data list. Those 28 explicit
objects are retained through the task/record/packet graph. Temporary
`TaskState` and `Richards` instances are outside that retained graph.

At two iterations per workload call, this predicts 56 additional explicit
graph objects per retention batch, matching both recorded count steps.
Warmup, one scored call and the first retention call total six iterations;
the next retention points total eight and ten. Expected explicit graph sizes
are therefore 168, 224 and 280. These are source-derived expectations, not
newly observed graph identities. The frozen output did not capture typed
graph membership, so it cannot be retroactively accepted by subtracting 112.
The 3,200-byte steps are consistent with live growth; no per-type byte
attribution was captured.

The observer also has an inherited collection asymmetry. Historical e9
`_PyGC_GetObjects` calls `snapshot()` when the age experiment is disabled;
that function invokes a manual collection. Adopted/cadence inspection walks
the heap without collecting. Existing retention has three explicit
`gc.collect()` calls in every scored child, and historical inspection adds
three collection entries after the live-byte readings. Strong global task
roots survive both. This is outside the scored clock and does not explain
the 56-object steps; it must remain explicit rather than being described as
three total collections on all arms.

The host AST/raw audit is
`build/gc-cost-retention-diagnosis-20261101-v1/audit.py`; its receipt is
`build/gc-cost-retention-diagnosis-20261101-v1/receipt.json`, SHA-256
`2aee7de03ff2d83f9129c0a778a71587c8a7c256007a74f558ee998824ffecfb`.
It binds the exact kernel, observer, controller, plan, activation, failed raw
output/return receipt and historical/current inspection implementations.
It imports no workload and uses no timing score.

The campaign remains incomplete. Preserve its 46 timings, including the
rejected row, without performance acceptance. The
[proposed oracle amendment](retention-oracle-amendment-20261101-v1.md)
requires new actual graph witnesses and separate review/authority; it does
not widen a bound or reset the workload's global roots.
