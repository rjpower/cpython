# Operand mode progress

2026-11-08: Design admitted after independent boundary-worker review. Runtime
implementation is ready for post-review in the separate operand-modes worktree.
One execution-only LOAD_ATTR_SHAPE_LOCAL_TEMP handler (319) is selected once
at specialization, inherits canonical metadata, and normalizes old/new feedback
rows. Local receiver consumption and conditional destination publication are
static in the generated handler; GCC savings remain unmeasured.

Completed cheap checks: normal handler/target/metadata/opcode generators,
number-table check, Python compile checks, generated format/contract/domain
inheritance checks, static-mode source assertions and invalid-mode rejection.
`git diff --check` passes. Added discoverable runtime/generator cases in
`test_ir_operand_modes`, adjusted two existing selection expectations in
`test_ir` and `test_opcache`. No native runtime pass, build or benchmark is
claimed. Root owns release/debug compile and actual focused suites; independent
actual-diff review requested from boundary worker. No commits.

Independent source post-review: `operand-mode-code-review-boundary.md` reports
no blocking source finding. New runtime tests explicitly require Py_SHAPES and
GIL specialization. Added warmed monitoring callback-error and re-specialization
coverage. Source is frozen pending root's native findings.

Cheap commands completed in the operand runtime tree, all using `uv run
--no-project python` with `UV_CACHE_DIR=/home/power/.cache/uv`:

- `Tools/cases_generator/ir_numbers.py --check-numbers Python/bytecodes.c`
- `Tools/cases_generator/ir_generator.py -o Python/generated_ir_cases.c.h -t Python/ir_targets.h Python/bytecodes.c`
- `Tools/cases_generator/ir_metadata_generator.py`
- `Tools/cases_generator/py_metadata_generator.py`
- `Tools/cases_generator/opcode_id_generator.py`
- `Tools/cases_generator/opcode_metadata_generator.py`
- `-m py_compile Lib/test/test_ir_operand_modes.py Tools/cases_generator/ir_generator.py Tools/cases_generator/ir_metadata_generator.py`
- `git diff --check`

An additional host driver checked same-object formats, equal contracts, handler
319/domain flags/canonical shadow/base, generated absence of dynamic class and
consumption macros, and rejection of invalid local/temp fields. Its assertions
are retained as discoverable `OperandModeGeneratorTests`; host pass is not a
native test run. Tool output is the execution record; no separate log artifact.
Native request: `./python -m test -v test_ir_operand_modes test_ir
test_ir_contract test_generated_cases test_opcache`, JIT disabled and environment
sanitized by the coordinator in the separate release/debug build directories.

Native focused gate found failures; root retained `operand-v2-release-focus.log`
and verbose opcache output. Two fixes prepared under review: execution-only
aliases require an actual declared specialization family so synthetic generator
fixtures reusing LOAD_ATTR_SHAPE do not inherit incompatible operand modes;
shape-history setup establishes both shared-key shapes before warmup because
inserting a new shared key invalidates the type version (`dictobject.c:1551`).
The latter preserves the strong canonical-feedback and eventual mode-fallback
assertions, testing the intended same-version policy. No uint8 handler truncation
was found in the saved/new feedback paths. Host reproduction of the previously
failing canonical-home fixture now passes; real variant remains active. Root
owns native reruns and golden comparison for two generic opcache selection
failures. No candidate timing claim is made while those remain unresolved.

Root's golden opcache control passed all 91 tests, and a matched native probe
showed the candidate really selects the physical variant while adaptive dis
incorrectly displayed generic LOAD_ATTR. The consumer bug was `dis.py` reducing
all nonfused IR-only handlers to their generic base. Repair uses the canonical
specialized shadow for execution variants while retaining fused and structural
IR_JUMP behavior. Existing opcache expectations are restored unchanged; the new
test now requires canonical specialized adaptive display alongside physical
get_ir selection. Peer reviewed the three repairs with no source blocker.
C and generated handler products are unchanged; root can rerun native focus
without a native rebuild. Source frozen again pending results.
