# Round51 qualification and measurement proposal

Preserve selected `bf810ec1a33cacb94102c73779d015c18f85aa8d` as the frozen
comparison. Each experimental subtree has its own source and release/debug build
directories. The coordinator owns creating controls, builds and all execution.

Add a focused/full runner adapted from the existing execution-boundaries runner,
with explicit `--source`, repeatable `--build ROLE=PATH`, and `--evidence` paths.
Keep native revision, magic and source-Lib preflight, sanitized Python environment,
hash seed zero, durable command/log journals, unique tags and failure exit status.
Support focus/full only; builds/regeneration/lint stay with established commands.
Do not add another build wrapper or change the historical runner.

Default qualification excludes only
`test.test_gdb.test_pretty_print.PrettyPrintTests.*` using regrtest's existing
`--ignore` option. `--include-gdb-pretty-print` restores those diagnostics.
Final50 release/debug logs identify this single-class module as failing with
16/21 failures; quiet logs do not identify individual failing methods. Excluding
its whole class is the narrowest supported rule from this evidence. The other
four GDB groups passed and remain live. Journal the exact quarantine and label
results as qualified with skipped diagnostics, never fixed or unfiltered green.
`test_sys_settrace` and `test_lltrace` remain live with their known outcomes.
No runtime test-source change is required.

Use a thin `measure.py` entrypoint to the existing DeltaBlue measurement runner.
It already accepts arbitrary candidate/control build paths and explicit golden
control commit, derives source identity from each Makefile, rejects dirty/stale
or incorrect module identities, and records before/after snapshots. Keep its
unchanged kernels, correctness/warmup behavior, balanced pairs, CPU15, hash0 and
JIT-off gates. Document exact frozen-bf810 and upstream commands rather than
introduce another baseline selector. Root will supply the frozen control paths.

Acceptance: root reviews this proposal before implementation. Host-only syntax,
help and command-construction checks may follow if permitted; this lane launches
no native tests, builds or timings. Root validates actual candidate focus/full
and matched measurements. Scope cut is the single GDB rule and explicit reusable
commands before any additional convenience.
