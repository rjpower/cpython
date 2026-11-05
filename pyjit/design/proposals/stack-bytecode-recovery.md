# Stack bytecode in co_code, register IR as a hidden cache

Status: accepted by the user 2026-11-05. Scheduled after the interpreter
checkpoint (full PyValue integration plus the register-IR interpreter). Until
then the B and I steps continue on the current form.

## Decision

`co_code`, `co_linetable`, `co_exceptiontable`, marshal and `.pyc` go back to
upstream's stack bytecode. The register IR becomes a hidden, derived form on
the code object, lowered from `co_code` and cached. The interpreter executes
only the IR.

This reverses the v2 choice in [register-ir.md §1.1](../register-ir.md)
("the compiler emits the register IR directly ... no lowering at the first
call, and no map back to bytecode offsets"), made in step 2a
(`6f7efa1d0ab`, 2026-10-25).

## Why

- Ports and tools read `co_code` and `.pyc`: `dis`, coverage tools, bytecode
  rewriters, `code.replace(co_code=...)` users such as cloudpickle, and
  freezers. With the IR in `co_code`, each one needs porting.
- IR changes stop being format changes: no magic bump per IR step, and the
  handler-number pin no longer has to follow the magic.
- Upstream's `dis`, `test_dis`, `test_compile`, `test_peepholer` and
  `test_code` expectations come back, which shrinks the fork's test diff.
- A code object built from bytecode (`code.replace`, `types.CodeType`) runs
  through the same lowering as compiled code, so hand-made bytecode gets the
  same checks as upstream and no IR-specific ones (see the crafted-code rule
  in `build/pyjit-link-wave1/B-COMMON.md`).

## What carries over unchanged

The IR words and handler set, the B steps' instruction changes, the executing
copy and feedback vector, specialisation on the handler column, the value
API, and the regcfg passes (`Python/regcfg.c`, `Python/regcfg_passes.c`).

Phase 1 (PR #1, `92cc151177c`) also kept bytecode beside the IR, but
instrumentation rewrote the bytecode and three subsystems (pcmap,
absorption, `IR_REMAP`) mirrored it into the IR. The recovery does not bring
back absorption or `IR_REMAP`: instrumentation acts on the IR's handler column
only, and the pcmap serves only to report offsets.

## Steps

1. **Lower from `co_code`.** `_PyIR_LowerCode(co)` decodes `co_code` and
   `co_exceptiontable` into the stack CFG and runs the existing
   `_PyCfg_OptimizedCfgToRegCfg` pipeline. It records the source bytecode
   offset of each word. The compiler and `.pyc` loading use this single path,
   so both produce the same IR. The IR's words, IR-pc line and exception
   tables, feedback init table, constant slots and pcmap move into a hidden
   `co_ir` structure. Measure lowering at the first call against lowering at
   code creation, using startup time (`python -c pass`, richards `-l 1`), then
   pick one.
2. **Offsets.** `f_lasti`, `tb_lasti`, `frame.f_lineno` (getter and the jump
   setter, which needs the inverse map), generator and coroutine `f_lasti`, and
   `sys.monitoring` offsets all report bytecode offsets through the pcmap.
   `co_lines` and `co_positions` read upstream's tables again.
3. **Monitoring.** The handler column is instrumented as today. Open
   question: `INSTRUCTION` events and `f_trace_opcodes` fire per bytecode
   instruction, but the IR folds instructions such as `LOAD_FAST`, `COPY`,
   `SWAP` and `POP_TOP` away. A candidate answer: while such events are on for
   a code object, lower it again without folding.
4. **Format.** Marshal returns to upstream's layout. `PYC_MAGIC_NUMBER`
   changes only for fork format changes outside the IR. The
   `PINNED_MAGIC_AND_TABLE` test goes.
5. **Tools and tests.** Revert the fork's `dis` and test-expectation changes
   to upstream. IR tests use an internal view, for example
   `_testinternalcapi.get_ir(code)`, or a private `dis` option.

Costs to record: the lowering time (step 1) and the memory of the two forms
(bytes per code object across the stdlib).
