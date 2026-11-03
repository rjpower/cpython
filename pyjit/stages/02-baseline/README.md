# Stage 2: qualified interpreter baseline

State: blocked by runtime integration. Owner role: coordinator and focused
correctness/performance workers after a coherent candidate exists.

Deliver a trustworthy starting point for optimization: the register interpreter
with universal values, measured with JIT explicitly off. The qualified older
interim image has boxed heap fields and cannot stand in for this result.

Entry: Stage 1 links and executes startup/first focused tests. Then run the
declared value, ownership, container, attribute, parser, mutation, reentry and
teardown cases; broaden to the selected release/debug/extension and stress or
sanitizer gates as warranted. Keep unsupported profiles explicit.

Measure against matched interim RC and upstream controls using the unchanged
29-workload roster. Retain raw workload vectors, build/toolchain identities and
JIT-off evidence. Separate startup, lowering and warm execution; report time
and memory. No arbitrary text-size veto substitutes for performance evidence.

Exit: qualified candidate plus reproducible matched baseline results and a
ranked set of specific next optimization hypotheses. Correct owning C bridges
may remain; direct-value operations, IR, dispatch and allocation tuning can then
be evaluated against that baseline.

References: [gates](../../design/validation.md), [measurement](../../design/measurement.md),
[integration](../../issues/pyvalue-rc-restoration/integration/status.md),
[benchmark registry](../../bench/README.md), [current status](../../STATUS.md).
