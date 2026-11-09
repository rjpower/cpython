# Round-wrap navigation and benchmark coverage audit

The canonical entry path is sound: `pyjit/README.md` leads to status, the active
plan, architecture, golden baseline and handoff. The five current IR,
interpreter, call/frame, handler/observer and debugger design pages describe the
integrated 38 production model. Focused 39 debugger test ports do not change
that model; the deferred coroutine and retained-owner drafts must not be
presented there as implemented capabilities.

At this inspection, `STATUS.md` still says the reviewed debugger ports await
native validation. `HANDOFF.md` links the thirteenth report while status links
the fourteenth and still assigns now-finished source work. The active plan's
work sequence starts with the completed canonical-home packet. Update those
three current-facing passages from the coordinator's final 39/40 qualification
and benchmark receipts. Preserve the dated reports and failed attempts as
evidence. `GOAL.md` contains older paragraphs that call superseded packets
active; mark those decisions historical or shorten them when the current
handoff is refreshed. The design pages need no protocol rewrite for a
test-only 39 change.

The Loom space has current `goal`, `plan` and
`interpreter-performance-report` artifacts. Six older one-revision artifacts
(`golden-checkpoint`, `golden-v8-review`, `interpreter-alternatives`,
`interpreter-mechanisms`, `v8-interpreter-review`, `v8-review-checkpoint`)
describe historical selections. Keep their revision history; remove or archive
their active-space visibility after the three current artifacts mirror the
final canonical pages. Deleting them would discard that history.

`pyjit/bench/run.py` registers 29 kernels, 65 micro rows and eight memory rows.
The two RSS-only memory rows are `startup` and `imports`; the six timed rows
are `tuples_10m`, `instances_1m`, `dicts_200k`, and
`collector_live{16,128,512}`. The unchanged runner already provides four
low/high instruction pairs plus skeleton subtraction for micro, and five
timed calls after warm-up for memory. It records raw instruction counts and
perf output in an exclusive journal. The issue-scoped supplemental wrapper
`measure-supplemental-benchmarks.py` checks native/source/module identities,
the exact roster and loop shapes, hashes inputs before and after, and retains
failed commands and postflight errors. Its single serial sweep is diagnostic
coverage, not a balanced elapsed-time comparison or a new aggregate parity
metric. Full 29-kernel balanced timing and the two six-round logging campaigns
remain separate final gates. No RSS-only workload or benchmark input was
changed for this coverage.

The timed memory driver makes one warm-up and five measured `run(1)` calls in
one process per row. `tuples_10m` is the largest instantaneous object count;
`collector_live512` retains a nominal 512 MiB heap while each call allocates
a fixed 1 GiB cumulatively in short-lived batches. These are source-defined
counts, not measured RSS. The runner samples load between workloads, but does
not impose a child timeout or memory cap; the wrapper has generous one-sweep
timeouts and retains a failed-command record. Schedule the sweep serially on
the reserved CPU with enough free RAM and no competing native work.
