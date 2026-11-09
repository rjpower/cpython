# Project status

The frame/call round is frozen at engineering candidate `4d666e1b482`.
Its runtime production changes are `24f4d270ced`; the last commit adds reviewed
debugger tests. The candidate remains unpromoted. The immutable golden is
`0765e9be47f`.

The [closing report](issues/frame-call-model-20261109/closing-report.md) owns
qualification and benchmark evidence. The [plan](issues/frame-call-model-20261109/plan.md)
tracks closure; the [five next priorities](issues/frame-call-model-20261109/next-steps.md)
are proposals for the next round. No new runtime feature is being added now.

Release/debug/stats builds check 117 modules each; stats smoke passes 150 tests.
Final release full diagnostic runs 54,645 tests, with GDB tooling and nine
frame-jump capability errors as its only failed modules. Pdb passes. Debug runs the same 54,645 tests with those two modules plus one
low-level trace-format check failing. The API check passes all 306 units with
zero failing findings or parse errors. The final benchmark matrix is running.
The full suite is not claimed green. GDB's exact format is not a throughput or
Python-semantics gate; unsupported frame jumps remain explicit API limits.

Logging passed the full parity gate earlier in the round. Final-source
confirmation is pending with the full 29-kernel comparison, all 65 micro
instruction workloads and six timed memory workloads. Startup/import RSS is
out of scope. Earlier results remain historical evidence, not final numbers.

The universal PyValue runtime is integrated across interpreter homes, containers,
attributes and rebuilt extensions. Calls use a frame stack, overlapping argument
windows, guarded binding plans, explicit result recipients and real continuations.
Source boundaries live in metadata. Initialized storage and shared semantic
completion preserve owning/GC/reentry invariants. See [IR](design/register-ir.md),
[interpreter](design/interpreter.md), [frames/calls](design/call-and-frame-lifetimes.md)
and [values](design/pyvalue.md).

The native method/tracing JIT remains unfinished. Deferred coroutine ingress,
owner-bundle and GDB drafts are preserved outside the measured runtime; their
task-created worktree is removed. The [handoff](HANDOFF.md) records exact paths.
Golden's [manifest](issues/golden-20261108/README.md) retains its independent
qualification and publication. Historical reports and unrelated local work
remain preserved.
