# Option 2: derive frame feedback from its executing image

Proposed for independent review; no source edits or execution yet. Branch
`pyjit/frame54-feedback`, source `BASE/worktrees/frame54-feedback`, start
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. Root owns generation/builds,
release/debug execution, native inspection, performance and commits. This
proposal follows lane F in [Astra's final review](astra-review.md).

Remove `_PyInterpreterFrame.ir_feedback` and derive entries from the execution
header immediately preceding retained `ir_words`. Keep function and executable
owners, frame publication, continuation, locals and validity contracts intact.
Measured selected layout is112 bytes before homes; this removal predicts104.
No new universal guard, owner reference, allocation or opcode family is added.

The execution image is already one allocation laid out `_PyCodeExec | words |
feedback` (`pycore_ir.h:351–390`, `Python/ir.c:425`). `IR_NLOCALSPLUS` already
uses prefix arithmetic (`ceval_macros.h:505–506`). `_PyCode_ClearExec`
(`ir.c:505–520`) frees the allocation only at code deallocation; a frame's
independent executing-code owner pins it. Specialization/instrumentation changes
words in that image, not its lifetime. Static init cleanup has the same prefix.
The evaluator `entry_trampoline` also has an `_PyCodeExec` prefix with NULL
entries (`ceval.c:1077–1088`). The thread base sentinel has no executable words
and must never invoke feedback or normal instruction-offset accessors. These
are existing distinct frame contracts, not new hot eligibility tests.

Use an inline accessor for the executing-image prefix or feedback base, then
make `IR_FB` derive entries from that prefix. `_PyFrame_Exec` currently derives
from owning code and asserts base equality; do not route hot feedback through
that longer code-owner decode/load chain. Retain meaningful equality assertions
by checking derived entries against the code image at existing assertion sites.
No accessor should synthesize feedback for the base sentinel.

Affected handwritten usages, exhaustively searched by field name in selected:

- `Include/internal/pycore_interpframe_structs.h`: remove declaration.
- `Include/internal/pycore_interpframe.h`: `_PyFrame_SetExec`, copy and new
  accessor; existing initialization calls no longer write feedback.
- `Include/internal/pycore_ir.h`: update cached-bases description.
- `Python/ceval_macros.h`: feedback-pointer ownership assertion near323,
  IR_CHECK_BLOCK and IR_FB near749–756.
- `Python/ceval.c`: observer/call assertions near1357,1603,2045,2089;
  evaluator entry initialization near2281; generic binding refresh near3442.
- Generated IR/dispatch products must follow the handwritten definitions;
  root generates, then verifies no `ir_feedback` field access remains.

Layout consumers use offsets/macros and need regeneration/build verification,
not fabricated hardcoded replacements: code `co_framesize`, deepfreeze
(`Tools/build/deepfreeze.py:296` emits FRAME_SPECIALS_SIZE), frame/gen type sizes,
thread arena header math, `_PyEntryFrame` stack static assert, public debug
offsets and remote/GDB observation. `ir_words` remains at its existing offset,
but return/metadata/home offsets move. Do not edit canonical runtime C files.

The removed work is one stored header word per normal frame initialization,
one source/destination word per frame move, one refresh store, and the initial
load of entries that existed only to fill that cache. The cost is a dependent
entries load on every IR_FB access: `frame -> ir_words -> prefix.entries`,
versus `frame -> ir_feedback`. Actual machine code may share loads. Header
shrink is not proof that enough dynamic work disappears.

Root's native gate must confirm header104, no field stores, no newly introduced
code-owner chain/helper call, exact CALL construction changes, RETURN_GENERATOR
copy changes, and feedback dependency chains in BINARY_OP/LOAD_GLOBAL/LOAD_ATTR/
CALL and branch handlers. Run the six unchanged workload screen against golden51;
confirm promising breadth with six pairs. Reject if broader added feedback loads
or instruction/cache effects overwhelm construction savings.

Minimal semantic gate: release/debug `test.test_opcache.TestFeedback` (real
methods `test_kinds`, `test_poly_and_mega`, `test_operand_versions`) plus
`test.test_opcache.TestCallCache`, `test.test_monitoring.MontoringDisableAndRestartTest`
(actual spelling), `test.test_frame.TestIncompleteFrameAreInvisible`, and
`test.test_generators.GeneratorDeallocTest`. These cover hot feedback mutation,
constructor trampoline, instrumentation restart, entry sentinels and moved
heap frames. Root may add existing focused generator/coroutine and value gates
required by the project. No new test mirroring prefix arithmetic is proposed;
existing semantic suites exercise real behavior. Any discovered regression
gets a focused behavior test, then the relevant gate is rerun.
