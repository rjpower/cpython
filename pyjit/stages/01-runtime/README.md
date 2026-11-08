# Stage 1: coherent runtime integration

State: integrated and qualified at `0267d428808`; selected batch kept after
performance admission. The broad upstream screen remains 1.2412× and parity
is not established. See the [checkpoint report](../../bench/results/interpreter-parity-20261108.md)
and [golden next plan](../../issues/golden-20261108/next-steps.md).
Owner role: coordinator; role names are not an active assignment list.

Deliver one runtime with canonical eight-byte PyValues across frames,
containers and attributes; a register interpreter; RC with cyclic collection;
and correct owning native boundaries. The selected baseline profile is Linux
x86-64/GIL. Do not reopen the closed tracing-GC/nursery work.

The table records earlier integration packets. Their dated pages are history;
the qualified report and golden next plan own current admission work.

| Workstream | Responsibility | Historical next step |
| --- | --- | --- |
| [Consumer ports](workstreams/consumers.md) | Core/startup and selected bundled container consumers | Core, stdlib, system and test modules ported; fix what suite triage finds |
| [Type system](workstreams/type-system.md) | Value-aware lookup, slots, mutation, cache and guard contracts | Type projection committed (checkpoint57); close lookup/version packets |
| [Native boundaries](workstreams/native-boundaries.md) | Per-thread borrow stack, getargs, Clinic | P1 to P3 landed; finish P4 and the post-review |
| [Integration](workstreams/integration.md) | Exact composition, commits and merged build | Qualified golden baseline complete; follow the cold review's experiment plan |

The selected runtime completed release and debug full-suite qualification:
54,394 tests each with zero failures. The checkpoint report records the matched
measurements, evidence limits, and accepted spectral tradeoff.

See [current status](../../STATUS.md), [handoff](../../HANDOFF.md),
[architecture](../../design/runtime-architecture.md) and
[runtime experiment records](../../issues/pyvalue-rc-restoration/status.md).
