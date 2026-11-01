# VM0 revision 3 process-count clarification review

2026-11-01 05:09 UTC. Coordinator accepts the separate
[count clarification](../compiler-backend-v3-count-clarification-20261101.md),
SHA256 `5958c9746ee3e6039581b8a8f59cd6ddaa887c4a4877b96335be3db6b6e80313`.
The five stale rows each invoke one target process containing six controls and
100 iterations per control: five physical children, thirty case executions,
3,000 iterations. Host wrappers and other focused-test children remain separate.

This corrects reporting units without changing exposure. Frozen revision 3 and
its original review remain intact; helper manifests must bind this correction.
Host preparation through 05:35 remains authorized. Compilation, targets and
timing require later independent helper review and a fresh serial activation.
