# Frame homes: liveness and initialization

Status: source review and bounded proposal; no runtime change or speed claim.
Source baseline: runtime `6a0e4c12c17`. This packet owns documentation only.

## Finding

`_PyRegCfg_Liveness` computes future reads of temporary virtual registers for
compaction and verification. It does **not** describe which physical frame
slots own values. `regcfg.c` explicitly keeps a temporary's slot until an op
takes it: a value below a loop with no exit can be dead to future reads and
still own a reference. The verifier permits that case. Compaction also reuses
one physical slot for different virtual registers, so a live-register set is
not a direct physical-slot validity map. The computed liveness is discarded
after compilation.

Every initialized frame slot currently has a valid `PyValue`, usually NULL.
`_PyFrame_Initialize` NULLs the part of the frame not occupied by incoming
arguments and then fills hidden constant slots. `_PyGC_VisitFrameStack` and
frame copying visit or move all physical slots. An exposed frame, callback,
trace event, exception, or suspended generator can make the frame observable
between instructions. The current NULL invariant also lets a handler read an
empty temporary and lets an unwinder clear from the highest slot downward,
with the established finalizer order. Omitting entry initialization of a slot
merely because no later IR word reads it would leave an invalid GC root and
an invalid value for copying or cleanup. Hidden constants must remain rooted
from entry, including mutable `co_consts` replacements with mortal values.

Normal thread-frame return already has a narrow path:
`_PyFrame_ClearExceptCode` asserts that temporaries are NULL and clears only
`co_nlocalsplus`. Generator-owned or exposed frames use the full clear.
Exception unwinding clears temporaries separately. There is no general
normal-return full-temporary clear to remove. The source-level profile still
shows initialization and frame-clearing cost, but it does not show that
skipping initialization is safe or faster.

## Bounded first increment

Measure the existing work before changing the frame representation. Add
`Py_STATS`-only counters, with no per-instruction fast-path writes, for:

* initialized NULL words and filled constant slots at frame entry, separated
  into exact/overlapping/generic frame pushes;
* slots scanned and non-NULL owners closed by temporary unwind, ordinary
  thread-frame clear, and generator/exposed-frame clear;
* code/frame size buckets for the hot workload, so a large cold frame cannot
  hide small hot frames.

Use the existing stats output and a private dump directory. Validate counter
totals with a small function, an overlapping call, a raising call, and a
generator that suspends and closes. The coordinator can then compare the
counter cost with the matched interpreter profile. If almost all scanned
slots are NULL, evaluate a bounded initialization implementation change
while preserving the same all-slots-valid invariant. GCC may already turn the
current loop into a bulk zero fill; inspect generated code before replacing
it. Do not add a per-slot validity bitmap or per-word dispatch updates from
this evidence alone.

If a later profile justifies avoiding writes to never-owned homes, the
contract needs a separate **ownership-validity** representation, including
entry arguments, hidden constants, slot reuse, overlapping call windows,
exception edges, generator moves, tracing/monitoring, and GC traversal.
Future-read liveness alone cannot supply it. The JIT's word-boundary frame
state also needs this ownership fact; recomputing `_PyRegCfg_Liveness` from
the IR is not enough. Such a representation is a distinct reviewed design,
not part of this increment.

## Source ownership and review gate

The first increment would touch only frame entry/clear counters in
`Include/internal/pycore_interpframe.h`, `Python/frame.c`, and the stats
declarations/output. It would leave `Python/ceval.c`, generated handlers,
`regcfg.c`, code-object layout, and the frame/GC contracts unchanged. The
coordinator should assign exact files after the stats interface is agreed.
An independent review must verify that the counters are outside hot dispatch,
preserve clear order and callback behavior, and count the existing branches
accurately. No runtime implementation is authorized by this proposal.

Source points: `Python/regcfg.c` (`_PyRegCfg_Liveness`, compaction note,
`_PyRegCfg_Verify`); `Include/internal/pycore_interpframe.h`
(`_PyFrame_Initialize`, `_PyFrame_Copy`, `_PyFrame_ClearTemporaries`);
`Python/frame.c` (`_PyFrame_ClearExceptCode`); `Python/gc.c`
(`_PyGC_VisitFrameStack`); `pyjit/design/register-ir.md` §§1.4–1.5 and O2.
