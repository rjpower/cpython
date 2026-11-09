# Operand-mode integration decision

2026-11-08. The bounded local-receiver/temporary-result attribute reader is
admitted after independent design/source review, native checks and compiled
inspection. This is focused qualification, not the final batch full-suite or
performance decision.

The actual candidate release and debug each passed **465 tests in six files**:
`test_ir_operand_modes`, `test_ir`, `test_ir_contract`, `test_generated_cases`,
`test_opcache`, and `test_dis`. Release/debug compile succeeded. Stats compile
and its two-file smoke passed 142 tests. Evidence is under
`build/pyjit-link-wave1/execution-boundaries-20261108/operand-v3-*`; compile
logs are `operand-v1-{release,debug}-build.log`. The source-only repairs changed
no C or generated handler bytes after those native compiles.

The failed checks remain retained. One early runner attempt used the golden
executable; it is not candidate evidence. The actual candidate exposed an
adaptive-disassembly mapping bug, synthetic generator family ambiguity and an
incorrect shape-history fixture. The fixes preserve canonical specialized
adaptive display, constrain alias generation to declared specialization
families, and establish shapes/type versions before the history test. Original
opcache assertions are unchanged. Golden's opcache control passed. Independent
peer review accepted the repairs; the final native rerun passed.

The [compiled review](operand-compiled-review.md) confirms removal of the two
ownership-class branches on the admitted path. Its representative static path
has 48 versus 70 instructions, with required type/shape guards and canonical
duplication retained. This is not a hardware count or speedup claim. The
224-byte variant is added beside the existing generic handler. Wider executable
text changes cannot be attributed to this local mechanism without further
evidence. Basic performance and spectral screens follow the source commit;
final admission belongs to the coherent combined batch.

The coordinator reviewed producer/generated/reader changes and executed logs.
The integration source matches the worker's reviewed repair set; no boundary
or call-result changes are included in this increment. Golden source/builds
and unrelated canonical edits remain unchanged.
