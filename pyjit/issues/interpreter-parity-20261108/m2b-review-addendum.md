# M2b review addendum: interpreter-only scope and corrected test

2026-11-08. This supplements the original source and pending post-review;
their snapshots and the first failed run remain historical evidence.

The coordinator accepted marking `_EXECUTE_ORDINARY_NOP` **tier1**. This is an
interpreter dispatch/PC mechanism, not a new semantic operation for optimizing
IR. `uop_id_generator.py` omits tier1 IDs; the register-IR generator still
emits the suffix and the same checked runtime intrinsic. Actual register-IR
JIT metadata and instruction words remain unchanged.

There is an explicit tradeoff: opcode_metadata_generator.is_viable_expansion
rejects an entire legacy opcode expansion when one of its parts is tier1.
Consequently the unused legacy `_PyOpcode_macro_expansion` table omits
POP_JUMP_IF_FALSE_BOOL, rather than merely omitting this suffix. A source
search found no current Python runtime consumer of that table. A future
frontend using it must lower the bool guard/branch while omitting the
interpreter-only continuation under a reviewed rule. Do not accidentally
reintroduce the intrinsic as an optimizing uop or misuse the specializing
annotation to make the table skip it. This is a future lowering dependency,
not an active JIT-off limitation or a change to the retained NOP word.

The initial new observer test failed because it compared frame.f_lineno with
dis.Instruction.starts_line, which is boolean here. The corrected source uses
nop.line_number and asserts the NOP starts a line and has a line number. This
repairs the test's intended observer location; it does not change runtime
behavior or weaken the expected result. The coordinator's
`E8/m2b-corrected-test-checks.json` and both referenced logs show that this
corrected case passes against the retained R5 control ELF and the current M2b
release ELF. This distinguishes the test API error from a runtime regression.

No build or test was run by this reviewer. Final source/regeneration and
focused release/debug round-two evidence remain necessary before the final
post-review verdict. There is still no throughput claim.
