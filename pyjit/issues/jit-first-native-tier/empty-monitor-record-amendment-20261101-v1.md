# Empty monitoring records and prefix admission

2026-11-01. Coordinator disposition during the bounded C-stage source attempt;
independent source review confirmed the distinction before implementation.
The original designs and source grant remain unchanged.

`force_instrument_lock_held()` can synchronize fresh code with a newer global
monitoring version while every event is disabled. Its
`update_instrumentation_data()` allocates an empty `_PyCoMonitoringData` header.
Unconditionally disabling the prefix there, or treating the header pointer as
proof of observation, would exclude newly compiled code after an earlier
global monitoring change. Reordering or isolating tests would hide that product
limitation.

Allow this header only when both local and active event arrays are entirely
zero and all five payload pointers are NULL: `tools`, `lines`, `line_tools`,
`per_instruction_opcodes` and `per_instruction_tools`. Keep every other entry
guard, including actual global events, tracing, profiling, tracing depth,
DTrace, lltrace, pending work and the ordinary uninstrumented entry state.
Version equality alone remains insufficient.

Permanently invalidate the descriptor for actual previous or new monitoring
state or retained payload, before the corresponding instruction rewrites.
Check local state as well as active state. Recheck admission after cold work.
An empty epoch synchronization does not itself invalidate the descriptor.
Existing code that was observed remains excluded; this adds no automatic
re-enable mechanism. Monitoring storage and execution-cache teardown remain
owned by the existing code-object lifetime.

The existing monitoring test must cover both sides: fresh code may enter after
global events were disabled; actually observed code remains excluded and keeps
its required instruction events. Retain the same twelve focused method IDs
and feedback cohort. This amendment grants no compilation, execution or timing.

Source anchors: `Python/instrumentation.c:1730` initializes the empty header;
`update_instrumentation_data()` and `force_instrument_lock_held()` distinguish
current and previous event state; `Include/internal/pycore_instruments.h`
defines the local/active arrays and five payload pointers. Concrete source and
test acceptance follows the combined frozen-source review.
