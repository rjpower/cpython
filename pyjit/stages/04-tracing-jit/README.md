# Stage 4: tracing JIT

State: selected direction; deferred implementation. It shares compiler machinery
with the method tier rather than introducing a second value or recovery system.

Deliver hot-path compilation through the shared typed IR, optimization and
native backend. Define recording boundaries, loop/side-trace policy, guards,
exits and interactions with method code. Feedback must distinguish receiver,
descriptor/hook and storage behavior; a broad object-kind tag is not enough.

Entry: coherent shared compiler and recovery contracts from the method work.
Exit: trace execution, failure/side exits, frame reconstruction, exceptions,
native boundaries, mutation/invalidation and code lifetime are tested and
measured on representative workloads. Common cases should be fast while opaque
or unsupported paths recover through explicit VM boundaries.

References: [selected JIT system](../../design/jit-e2e-system.md),
[trace research](../../design/jit-tracing-research-20261101.md),
[feedback design](../../design/jit-ir-feedback/design.md),
[current status](../../STATUS.md).
