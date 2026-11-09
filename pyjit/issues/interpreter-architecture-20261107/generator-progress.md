# Generator contract workstream progress

2026-11-07. Owner: Luna medium. Runtime baseline `7780e32e0e2` has been
captured. Root accepted the amended semantic interface and released generator
implementation. Work is in isolated runtime worktree
`interpreter-contract-20261107` at `25ce1654801`; root owns the shared IR
header, `_opcode` reflection/tests, and generated products.

Read-only inspection confirms `IREmitter.retire_published()` currently checks
five handler names to set `IRStack.published_temporary_result`. That fact
selects both local-only failure rooting (`failed_result`) and the final
destination emission; `IRStack.store()` keeps the folded-local store while
skipping a second temporary store. The operation itself flushes first, retires
the receiver, refreshes generated state after callback, and retains error
handling. `IRStack.copy()` inherits the fact through `Stack.copy()`'s shallow
object copy, so branch copies retain it. `IRStack.merge()` does not reconcile
the fact with the other incoming path; the reviewed contract must merge only
matching postconditions or reject conditional-marker joins.
`analyzer.properties.sync_sp` derives synchronization from either
retirement marker; `ir_metadata_generator.py` serializes word-level effects,
roles and extents, but no output-home postcondition.

The final profile samples shape attribute loads for 10.73% deltablue self,
5.44% logging_silent and 4.81% comprehensions; method attribute loads account
for 5.95% deltablue and 6.32% logging_silent. These are symbol sample shares,
not invocation counts or isolated helper costs. In retained candidate GCC output,
`_TAIL_CALL_LOAD_ATTR_SHAPE` calls `retire_published_values` only on its
deallocation/callback path and has separate temporary/local destination paths.
Integration C already removes the duplicate final temporary store. This is a
measured-path justification for expressing that established postcondition,
not evidence for another runtime deletion.

The first implementation changes the five specialized attribute load
producers to `IR_RETIRE_PUBLISHED_RESULT(attr, receiver)`. The generator uses
one checker for emission and metadata, validates top-level unconditional result
and owner assignments, the `ROLE_DST`/zero offset, output ownership, and
effect-free/disjoint suffix positions. Metadata reports
`result_phase = DST_TEMP_CANONICAL_BEFORE_RETIRE` for this marker and the
validated generic completion helper. Tests cover temp/local emission,
conditional and duplicate markers, conditional result assignment, receiver
reassignment, wrong output, suffix escape/read, and generic phase metadata.

Root's first host run of `test.test_generated_cases` found that the receiver
binding check expected `OWNER(receiver)` instead of the top-level
`receiver = OWNER(input)` assignment. Later focused runs exposed a pre-instruction
stack-offset assumption and two test-fixture precedence issues; all were fixed.
The final checker compares the format destination with
`first_new_output(outputs, len(inputs))`, requires a newly produced first
output, and validates the scalar `ROLE_DST` mapping. Suffix checks inspect raw
effect flags despite specializing annotations, reject result references and
stack-position overlap, and permit variable-length array output only in the
terminal suffix uop. v6 passed all 69 `test.test_generated_cases` tests in
5.493 seconds. Sol accepted the frozen source review; native validation remains
with root. A later cold R4 probe found a missing generic-prefix guard: a
producer could close `receiver` or `pending` before `IR_COMPLETE_ATTR_READER`
and then continue to the helper. The real null-result cleanup is valid because
it is a flat branch ending in `IR_ABORT_VALUE_READER()`. The bounded repair
checks protected-argument release/take, mutation and address operations on
continuing paths, permits only that terminal cleanup branch, requires direct
unconditional output assignments, and rejects comma-expression result use.
The expanded negative matrix covers receiver/pending/pending-self close,
reader-close and view-take calls, conditional and loop-header consumption,
member/compound mutation and address escape. An early address-of-local guard
rejects the address case before the new checker; the fixture asserts that
existing diagnostic. v7 ran 73 tests with one assertion mismatch in that
expected diagnostic; the fixture was corrected. R4 landed at `b58036e9718`; the repaired release and
debug generator plus contract suites passed 97 tests per profile, and generated
runtime products were identical. The earlier 1,100-test focused runs and
306-unit lint remain pre-repair evidence, not repeated post-fix checks. The
original v6 acceptance remains a record of the missed path, not the final
qualification. This worker could not use uv because its shared-cache lock
creation failed with EROFS; non-writing AST parsing and `git diff --check`
passed. No generated C body or native runtime performance change was made here.
