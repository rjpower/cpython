# Option 3 implementation post-review

Final disposition, 2026-11-11: **approved for isolated measurement** at clean
`d1353a16f3dff1cc3a2f7a556b75b675d34498c0`. The helper classification and
generated-product findings below are closed by the follow-up evidence. Debug
qualification, matched timing and integration admission remain pending.

Reviewed source commit `8b2289c03aaf9aff282f1778bd11a9b9b3335bd4` against bf810,
its generated handlers, retained native disassembly, build log and check journal.

## Required correction

`call_transfer_exact_args` only moves raw homes and clears sources; it cannot
allocate, retire or call Python. The analyzer does not yet recognize that
contract. Regeneration consequently adds ESCAPES to CALL_PY_EXACT_ARGS in
`pycore_ir_metadata.h` and `pycore_opcode_metadata.h`. These products were dirty
during the reviewed release run; the journal correctly reports the dirty tree.
Classify this specific helper as nonescaping, regenerate all products and commit
the complete result. Check the resulting clean source/native identity and rerun
the relevant generator checks. Retaining false effects or omitting regenerated
products is not an acceptable resolution.

## Source and ownership

The final base is chosen before reservation. Heap caller addresses do not enter
thread-stack geometry: non-thread callers select the current thread top, and
caller-tail NULL checks remain inside the thread-only debug branch. Reservation
uses max(callee end, saved top), truncates initialized storage before the header,
then initializes once. Stack capacity admission remains before this infallible
sequence. Pop restores saved top and never restores an obsolete frontier.

Function transfer, code acquisition and frames_pushed occur once. The cold
argument move writes self only when present and copies only explicit arguments;
no-self zero-argument calls preserve slot-zero constants. It clears transferred
source homes, with callable cleared in common commit. `_PyFrame_PushOverlapping`
keeps the shared reservation and existing always-inline binder policy.
The outlined initialized-tail helper fills only the uncertain suffix and does
not release owners, allocate or call Python. Other reservation callers retain
their capacity/truncation rules.

## Executed and native evidence

`E/option3-release-build.log` checks 117 modules with zero import failures.
`E/option3-release-focus.log` and `option3-focus-checks.json` report 668 tests,
9 skips, all seven requested modules passing. New tests cover heap generator
arguments/self, escaped frames, deep narrow/wide reuse and finalizer reentry.
Existing ConstSlotsTest covers generator/coroutine zero-argument constant homes
and mortal replaced constants. This review did not run another build or suite.

`E/native/option3-exact.txt` shows one header/constant initialization region and
973 handler bytes versus 1,410 in selected50. The C-stack reservation remains
0x50 bytes. Both outlined calls are behind the intended exceptional storage-
growth or non-thread branches; the initialized thread-owned hit has no helper
call. Its spill burden improves modestly, not proportionally to symbol size.

A static selected path through the retained disassemblies (thread owner,
initialized storage below header, callee end no higher than saved top, mortal
code, no const homes, direct first-op dispatch) contains 150 instructions and
22 rbp-addressed operations in selected50 versus 149 and 18 here. The selected
baseline branches are 0x37a8b5, 0x37a8c9 and 0x37a92f; candidate branches are
0x37a92e and 0x37a952, with all other hit-path conditions falling through.
These are static path counts, not measured dynamic instructions. They support
reduced spills and code duplication but promise no application gain. Matched
DeltaBlue/richards/coroutines/logging timing still decides admission.

Independent source/native post-review; no performance approval granted.

## Finding closure

Reviewed follow-up commits `1fe5957d22d` and `d1353a16f3d`. The analyzer now
classifies the four audited direct helpers as nonescaping:
`_PyFrame_ReserveForCall`, `_PyFrame_Initialize`, `call_transfer_exact_args` and
the pure address conversion `_PyDataStack_Slots`. Generated metadata is committed
and the source tree is clean. No runtime C implementation or generated handler
body changed from the previously reviewed implementation.

`E/option3-final-release-build.log` checks all 117 modules with zero import
failures. `option3-effect-fixed-release-focus.log` passes the new exact-activation
property regression; its journal records clean d1353 source and matching native
preflight, exit zero. The regression checks escapes, error_without_pop and
sync_sp are all false. The earlier 668-test/9-skip release behavior run remains
applicable to the unchanged runtime implementation. The earlier failed
intermediate property run remains retained; it is superseded by this clean pass.

Both post-review findings are resolved. This closes source/native review for
isolated measurement, with debug and performance still unqualified.
