# H3 handler-entry producer for the PyValue checkpoint

The 4fffee855c9 stats build does not emit the four handler-entry coverage
declarations required by the benchmark reader. Its counter arrays also stop
at 256, and the dispatch macro skips IR-only targets. The failed 29-kernel
attempt is retained at `build/pyjit-link-wave1/checkpoint-closure/kernels-4fffee855c9-parser2/`.
Adding declarations alone would falsely certify coverage.

The isolated source candidate is `h3-runtime-producer-v2.patch`, based on exact
4fffee855c9833031de188886541c3754c8fe0ba. Its worktree is
`build/checkpoint-h3-emitter`. It adapts the reviewed H3 mechanism from
historical commit d462e8338ce3 to the present 319-entry B6 domain:

- The generated opcode ID header owns `IR_NUM_HANDLERS`, so the public
  `PyStats` layout can size both counter axes. The merge and dump loops use
  that same bound.
- Each generated IR handler counts its own entry before its predicted label.
  Activation-local `lastopcode` advances even while aggregate stats are off.
  The semantic specialization and boxing counters retain their existing
  opcode attribution.
- The metadata generator derives 298 named entries and 285 dispatch targets
  from the pinned ID map. It emits generated flag and canonical-base tables.
  The raw producer prints version 2, dispatch mode 1 (the current interpreter
  always tail-calls), actual domain/count values and an ID/base/flags triple
  for each named entry. It derives `specializable` from specializing DSL work
  and feedback, removing the old cache-width heuristic and its duplicate
  GET_ITER declaration.
- The emitter uses the existing opcode names below IR_FIRST_ONLY_HANDLER and
  the generated IR-only name table above it. The old H3 code indexed opcode
  name/deopt arrays across the whole domain; those arrays stop at 267/256
  here, so that code cannot be copied verbatim.

Source review of the historical implementation is recorded in
`pyjit/archive/legacy/design/work/reviews/H3-stage2b3-source-review.md`;
the accepted contract is
`pyjit/archive/legacy/design/work/H3-1-stage2b3.md`. The current patch has
one focused raw-dump test for complete generated triples and an actual
`BINARY_OP_ADD` entry. The final reader still needs to adopt and validate the
triples before aggregate reports claim full H3 semantics. The current
`pyjit/bench/run.py` uses the four scalar declarations and retains all raw
dump text, so it can measure with this producer once the runtime is rebuilt.

The stats API's fixed specialization-family roster was already stale at
4fffee855c9: `test__opcode.test_specialization_stats` expected
`pop_jump_if_false`, `pop_jump_if_true`, `get_iter` and
`call_function_ex`, while the API still advertised `jump_backward`.
The patch updates only that API roster; raw all-handler entry counts remain
independent.

Validation so far: both generators regenerated their products; the fact
table has 319 domain slots, 298 named entries, 285 dispatch targets and 46
adaptive targets; `clang-21 -fsyntax-only` passed for `pystats.c`,
`ceval.c` and `ir.c` with Py_STATS, and for `pystats.c`/`ceval.c`
without it; `git diff --check` passed. No linked interpreter or dynamic
`test_pystats` result is claimed yet.

Before integration, peer-review the generated base and adaptive flags and
the changed `PyStats` layout. Then rebuild all stats objects, run
`test_pystats.TestPyStats.test_ir_handler_entry_metadata`,
`test_pystats.TestPyStats.test_malloc_debug_with_stats`,
`test__opcode.SpecializationStatsTests.test_specialization_stats` and the
existing stats lifecycle tests. Check one raw dump with the strict H3 reader. Exercise
an IR-only per-operation target, a specialized target, an instrumented target,
and a relaxed jump or executor path; confirm each counted entry/pair against
the observed execution copy. Run the exact `PYTHONMALLOC=malloc_debug` plus
`PYTHONSTATS=1` import repro. Root's gdb trace located the 4fffee crash at
`COMPARE_OP_JUMP_FLOAT`'s `UPDATE_MISS_STATS` with `opcode=306`: its
`STAT_INC(opcode, miss)` indexed the old 256-row array out of bounds.
The full-domain allocation addresses that write, but the rebuilt repro must
pass before the crash is considered fixed.
