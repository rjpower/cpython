# Attribute publication contract: source review

2026-11-07. Review of the frozen R4 diff in runtime worktree
`interpreter-contract-20261107`, before integration. **Source decision:
accept, subject to the coordinator's final generator and native validation.**
This is a first-output success-path fact, not a complete recovery map or a
measured speedup.

The five specialized producers in `Python/bytecodes.c` now declare
`IR_RETIRE_PUBLISHED_RESULT(attr, receiver)`. The same
`result_publication_contract()` in `Tools/cases_generator/ir_generator.py`
feeds both `IREmitter` and `handler_contracts()`; the emitter no longer
decides the local-only failure value or final temp-store omission from a
five-handler name list. `analyzer.py` recognizes the marker's error and
synchronization effects. The bounded `result_phase` field is carried through
`_PyIRFormat`, generated metadata and `_opcode`'s existing contract row.
The source generator's design reference now points to the canonical
`pyjit/design/register-ir.md`.

The checker requires one top-level final marker, a sole unconditional result
assignment and a sole `OWNER()` initializer before it. It verifies that the
first external output is a new, independent scalar result at the format's
`ROLE_DST` offset. That offset is relative to the **pre-instruction** stack
pointer (`first_new_output()`), so the one-input attribute loads use `-1`.
It rejects result use after assignment, ambiguous aliases, suffix reads or
writes of the result position, direct result-name references, releases,
escapes, frame switches, exits and direct frame-slot access. A dynamic-array
suffix output may be terminal, as the existing conditional NULL companion
is; it cannot precede another uop because its zero-length case would make a
fixed-depth proof unsound. The assignment validator is shared between result
and `OWNER`/`TAKE` validation rather than duplicated scans.

For generic `LOAD_ATTR`, the checker recognizes the existing
`IR_COMPLETE_ATTR_READER(pending, pending_self, receiver, method); attr =
pending;` sequence. The trusted `value_attribute_complete()` and
`value_reader_complete()` helpers in `Python/ceval.c` validate output homes,
publish and store the first temporary result before closing the receiver and
refreshing. On a folded local, the result remains C-owned; the old home
remains installed until retirement begins, but callbacks may mutate public
locals. Failure closes the C-owned result without installing it over callback
mutations; success stores into the current home. The method path publishes
both temporary outputs before close and refresh, but the metadata deliberately
describes only the first. Earlier
generic lookup can invoke Python; `result_phase` does not assert purity or
publication before that helper call. As with existing DSL intrinsics, the
checker verifies a narrow declaration and source shape; it does not prove
arbitrary C helper bodies or pointer provenance.

`Lib/test/test_generated_cases.py` exercises the emitted publication order,
folded-local failure argument, companion output and rejection of malformed,
conditional, consumed or aliased declarations and unsafe suffixes.
`Lib/test/test_ir_contract.py` checks the public metadata values. The final
generator suite passed 69 tests in 5.493 seconds
(`build/pyjit-link-wave1/interpreter-architecture-20261107/r4-generator-v6.log`).
The coordinator then regenerated cases and built release and debug runtimes;
both builds completed with 117 checked modules and no import failures. The
same eight focused test files passed in both builds: 1,100 tests, one skipped,
in 6.8 and 35.2 seconds respectively. These outcomes are recorded in
`r4-checks.json` and the `r4-release-{regen,build,tests}.log` and
`r4-debug-{build,tests}.log` files in the same evidence directory. The
generated `Python/generated_ir_cases.c.h` was unchanged; the generated
metadata header adds the phase field, and its six attribute rows have
`dst=-1` and the declared phase. The full builds printed existing warnings
from generated cases, `ceval_macros.h` and unrelated modules; there is no
diagnostic pointing to the changed R4 C code. These checks validate the
bounded source fact and existing behavior, not a performance improvement or
native JIT recovery.
