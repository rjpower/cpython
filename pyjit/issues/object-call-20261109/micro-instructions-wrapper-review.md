# Micro instruction capture review

The workload selection, paths, identities, and raw/net interpretation are correct. Before capture, improve the wrapper's durable failure reporting. Before admitting results, verify PMU coverage from the retained raw records; the existing runner does not enforce it. This review ran no native workloads and changed no measurement helpers or runtime files.

## Source checks

`packet.parents[1]` resolves to canonical `pyjit`, so the checkpoint helper and benchmark runner paths are correct. The three build and module-log paths select upstream, frozen golden39, and candidate41. Golden's expected revision and module count are overridden to frozen39/117; candidate must match clean `0a35a382909ca169ce3f381cb38206b1725001b3`. Existing checkpoint snapshots validate native/source revision agreement, release/JIT conditions, representative imports, configured source, executable build ID, and module logs. The wrapper adds executable SHA-256 and hashes itself, the checkpoint helper, and all benchmark Python sources before and after.

The exclusive new output directory prevents overwriting a prior capture. Each role's command runs the host runner under CPU 15 affinity; its native descendants inherit that affinity. Environment handling clears Python overrides and `LD_PRELOAD`, uses the shared uv cache, and sets hash seed zero. These are surrounding identity checks, not metadata assertions emitted by every counted child. The wrapper neither changes nor hashes `object-call-20261109/measure.py`, so creating or editing this separate helper does not change the running timing campaign's input census.

`--only class_attr_load,call_py_kw` automatically includes their shared `empty_loop` skeleton. Each role therefore measures three entries. The existing runner performs one perf availability probe, then one untimed-count warmup and four low/high pairs per entry: 28 perf invocations per role, 84 across three roles, absent failure. The raw JSONL records preserve start/finish entries, argv, child exit status, perf text, counts, and errors. Missing perf output, nonnumeric unsupported counts, or child failures return nonzero and stop the wrapper. The runner's load checks occur before each benchmark and at completion, not before every low/high member.

## Meaning of the numbers

The unchanged micro registry supplies 200 low loops, 600 high loops, and 1,000 iterations per loop. A pair's estimate is `(high_count - low_count) / 400000`. Four such estimates produce a median. For the two requested operations, `raw` is that median **before skeleton subtraction**, not a whole-process count. `value` is `raw` minus the same role's `empty_loop` median. `values` retains the four unsubtracted estimates; `raw_pairs` retains actual whole-process low/high counts and invocation identifiers.

This protocol estimates marginal instructions from separate whole-process runs. It is not the FIFO-delimited PMU interval used for the kernel gap capture. Startup cancellation is an assumption of differencing, and skeleton subtraction is a derived estimate, not a direct opcode instruction count. Report raw and net separately. Do not compare a net instruction ratio directly with whole-body elapsed ratios or label it an elapsed speedup.

## Required failure and coverage handling

1. **Preserve a status record on snapshot failure.** The current initial snapshot and preflight assertions occur outside the protected block. In postflight, `snapshot()` can raise (including its embedded candidate revision assertion or missing executable/source files) before writing `identity-after.json` or `status.json`. These cases fail closed at process level but leave no durable rejection status. Put preflight, commands, and postflight under explicit failure recording; retain the original failure and a separate postflight error, write `complete: false` on either, and preserve any obtainable snapshots. Candidate revision mismatch should be recorded rather than preventing status serialization. Keep this repair within the new wrapper.

2. **Do not infer PMU coverage from `complete`.** The inherited `perf_instructions()` parser accepts a numeric instruction field without checking event enabled/running coverage fields. The JSONL's full `perf_text` permits later verification. Either gate coverage in this wrapper before success or explicitly audit every retained record before admission. Do not silently promote reduced-coverage/scaled counts as fully counted instructions. Also check the expected three result entries, four pairs per entry, and 28 successful perf finishes per role when auditing actual output.

No changes to the frozen timing helper, benchmark bodies, or existing runner are needed for these points. This source review does not certify future raw counts or the eventual completion record.
