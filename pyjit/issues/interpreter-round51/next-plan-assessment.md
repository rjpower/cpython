# Next-plan source assessment

Assessing frozen **0e5e61f4c6b**, `BASE/worktrees/round51-golden` (R below).
E denotes `BASE/pyjit-link-wave1/interpreter-round51`.
This proposes no implementation and expands no current-round scope. Reject B
as an already-implemented mechanism. Keep A as a low-priority, evidence-gated
question. Fresh selected-image samples now rank resume/yield ahead of final
completion cleanup and support the bounded hypothesis below. No native
execution, benchmark, build, perf collection or generator run was performed.

## 1. Generator transitions: measured priority and one bounded hypothesis

The selected-image generator recording in
`E/final51-perf-record/record-r1-candidate-generators-cycles-record-symbols.txt`
and its `-flat.txt` callchains has about 2,495 user-cycle samples and zero lost
samples. SEND_GEN accounts for 27.41% (684 samples), YIELD_VALUE 13.92% (348),
and JUMP_BACKWARD_NO_INTERRUPT 5.11% (128). RETURN_GENERATOR is 2.39%,
CompleteGeneratorReturn 1.44%, and FrameClearLocals 1.36%. Separate whole-workload
PMU intervals report 1.414550 instructions and 1.443516 user cycles per upstream
loop ([tables](benchmark-tables.md)). These are different measurements: neither
the whole-workload excess nor sampled shares identifies removable operations.

The recording ranks normal resume/yield control ahead of creation, final
completion and final clearing for this workload. The flat report places the
hot SEND_GEN path under the generator iteration entry chain; it also attributes
some samples to its inlined recipient publication. These are locations, not
event-frequency counts or proof that an individual check dominates. In
particular, SEND_GEN plus YIELD_VALUE is not a 41% optimization budget.
The older exact-CALL 13.39%, clear 5.68%
and RETURN 4.40% samples are **selected50 DeltaBlue** samples
([report50](../deltablue-call-model-20261110/report50.md)), not a profile of the
current recursive-generator kernel. They do not rank current generator paths
or give removable fractions. Rejected option5 and the rejected 2+3+4 addition
must not be revived to fill a planning quota.

Actual remaining transitions are identifiable. `R/Python/bytecodes.c:1629`
checks exact generator/coroutine type, initial-send legality and executing state;
`R/Tools/cases_generator/ir_generator.py:3915` emits resume-input detachment,
caller recipient/continuation, current-frame and exception links, recursion
entry, incoming-value publication and dispatch. `bytecodes.c:1708` performs the
reverse yield handoff. `R/Python/ceval.c:3095` reconstructs SEND/FOR completion
from the real immutable caller word and instantiates the existing shared
completion with canonical input. That specialization already avoids generic
result publication; it is not missing work to propose again.

**One hypothesis: a proved delegated-resume continuation can avoid an otherwise
empty backward redispatch to SEND.** The general yield-from/await lowering at
`R/Python/codegen.c:553` emits SEND, delegated YIELD, RESUME and a noninterrupting
backward jump to SEND. On ordinary resumption, `enter_generator` installs the
incoming owner, reloads the suspended IP, skips eligible RESUME, and dispatches;
the generated JUMP_BACKWARD_NO_INTERRUPT only selects its real target and
dispatches again. A compiler-certified continuation could select that same real
SEND landing directly when the complete intervening chain is proved empty.
This targets a control detour, not all SEND/YIELD work, and is not a new fused
opcode family or a second generator evaluator.

The first proposal should cover only this exact delegated continuation, with
ordinary creation, nondelegating yield, close/throw and unproved or observed
continuations retaining their existing handling. Prove the same receiver window,
incoming recipient and real SEND target; retain the suspended yield's diagnostic
PC and every required source/INSTRUCTION/RESUME event. A monitoring change or
debugger redirect cannot reuse stale shortcut authority. `_PyGen_YieldFromRef`
(`R/Objects/genobject.c:265`) reads the receiver through return_slot-2, so the
real recipient/window relationship must remain intact for gi_yieldfrom, close
and throw. Do not replace it with an unowned C-only value.

Necessary work stays: provider/PEP523 and initial-send guards, executing-state
admission, exception links, recursion accounting, caller continuation, coherent
current-frame publication, input detachment, result ownership, DTrace and debug
entry. `_PyFrame_SetReturnValue` already publishes canonical bits directly;
there is no normalization there to remove. Its pending-redirect handling and
displaced/unused-owner retirement also cannot be skipped by analogy. Keep those
operations while testing the control hypothesis.

Before implementation, inspect the existing native resume/landing sequence and
earn an encoding/state plan that preserves the single owning-frame model. Any new
continuation decoding, guard, saved field or per-suspension store may cost more
than the eliminated dispatch; do not hide that cost in a general runtime flag.
Require source/AST proof negatives for changed control, recipient, callbacks,
instrumentation and redirects, then focused release/debug semantics and matched
generators/coroutines with DeltaBlue/logging guards. The present samples justify
this bounded next question, not its design, a speedup, or a removable share.
No new evaluator, owner ledger or source-map execution dependency.
The prior slot-base/cache-transport experiment is also
[closed and rejected](../interpreter-dispatch-state/status.md); this assessment
grants no retry, opcode-template migration or broad state-transport rewrite.

## 2. A: constant homes have a real cost, but a bank is not yet earned

**What exists.** `R/Python/regcfg_passes.c:805–1110` folds selected scalar or
singleton loads in CFG loop blocks into hidden `CO_FAST_CONST` locals. It
requires a local-capable borrowed reader; window operands and loads outside
those loop blocks retain their loads. Distinct constants consume frame slots;
`shift_slots` increases co_nlocalsplus and shifts free variables/temporaries.
`R/Include/internal/pycore_interpframe.h:282–331` fills those homes on each
actual frame initialization. `R/Python/frame.c:181–235` scans and clears them
again with the locals. Generator creation moves all homes into the heap frame
through `_PyFrame_Copy`; resume does **not** refill them. Most nominated values
are count-free inline scalars/singletons, so the usual cost is loads, stores,
classification and slot footprint, not mortal refcount traffic. Replaced mortal
constants do acquire and later release a separate owner.

**A new array alone is redundant.** `R/Include/cpython/tupleobject.h:12` stores
`co_consts` tuple items as canonical PyValues already. `_PyFrame_Initialize`
acquires the executing code owner separately from the mutable function; that
code pins its immutable co_consts tuple. Thus the useful question is whether
consumers can read the existing code-owned source without per-activation homes,
not whether to invent canonical constant storage. A dense second bank would
need its own explicit ownership/construction/GC contract and must justify its
extra storage or indexing layer.

**The tradeoff is at every read.** Current `IR_SLOT`/`IR_VIEW` use the frame's
cached slot base; `IR_IS_LOCAL` and `IR_CONSUME_INPUT` distinguish borrowed locals
from consumed temporaries (`R/Python/ceval_macros.h:499–546`). Removing a hidden
home saves entry fill, retirement scanning and some frame/copy capacity. It
replaces a direct local read with code/constant-base addressing and an operand
domain choice. Long loops amortize the existing one-time fill particularly well.
Do not count initialized-frontier zeroing as a full per-entry cost: reservation
already reuses initialized NULL storage.

The same-word encoding has four 12-bit fields. Ordinary slots already need
0–4094, with 4095 reserved for a discarded destination; LOAD_CONST can use a
24-bit CD index. A bank bit in every input would reduce slot/index range or
add common-path classification. A declared constant operand form could reuse
one existing input field without widening the word, but needs a defined index
limit, ordinary-load behavior outside that limit, compatible generic/specialized
misses and all format consumers. A compact bank mapping adds another representation
and possibly another lookup. There is no unallocated general-purpose tag bit.

Code pinning makes a C-local borrow plausible even for mortal replacements,
but a constant operand is not a frame owner to clear or steal. TAKE/escape must
acquire an owner; callbacks, native providers and exceptional transport must
retain the code anchor. Update constructor verification, code.replace/mortal
constants, marshal/magic as applicable, GC/frame copies, closure indices,
disassembly, specialization/instrumentation, debugger reconstruction and
semantic exception recipes together. Hidden constants are filtered from public
locals, yet their physical geometry currently participates in owner/recovery
proofs. Removing their homes cannot just bypass those consumers.

**Existing negative evidence matters.** The preserved `constant-operands`
worktree at `a84965eaa90` tested thirteen same-word 0–255 RHS handler forms,
not this bank: C became a literal while A/B/D retained destination/LHS/feedback.
It removed producer/frame/RHS work but was
[rejected on elapsed performance](../interpreter-constant-operands/status.md)
(coroutines +4.47%/+11.64%; other registered cases also slower). It was not
rejected by a text-size veto. Do not restart that handler expansion or its
campaign. A code-owned bank is distinct, but distinction alone is not evidence
that new operand forms will pay off.

Before any A proposal, inventory **weighted** co_nconstslots, activations,
constant reads per activation and frame/copy footprint in the unchanged hot
kernels. In particular, spectral eval_A has no own loop; its literal arithmetic
cannot simply be counted as hidden homes because its callers loop. Recursive
fibonacci's arithmetic precedes its await cycles. Await/yield-from do introduce
CFG backedges (`R/Python/codegen.c:553`), so inspect emitted nomination rather
than assuming every source function without a loop has zero constant slots.
If actual fill/retirement is material, compare a complete read-side design with
the saved work; otherwise reject A before implementation. Current evidence
supports neither a return-on-investment claim nor broad constant-opcode work.

## 3. B: reject the missing-indirection premise

`R/Include/internal/pycore_interpframe_structs.h:51` declares ir_feedback as
`char *`; `_PyFrame_SetExec` assigns `exec->entries` directly. `_PyCodeExec`
likewise holds `char *entries` (`R/Include/internal/pycore_ir.h:365`).
`IR_FB` calls `_PyIR_FbEntry`, which returns
`entries + ((size_t)field << 3)` (`pycore_ir.h:539–544`). There is **no indexed
entry-pointer lookup**. Feedback already lives in one directly addressed pool.

The field is a 12-bit offset in eight-byte units, not an ordinal into a pointer
table. Units 0–4095 cover a vector of at most 32KB; entries start at unit 2
because shared generic entry zero is a 16-byte header. Overflow sites share that
non-specializing header; operations needing stored arguments receive entries
first or lower to the existing intrinsic path. Typed entry geometry and these
fallback rules are part of the contract.

Encoding literal byte offsets instead would need up to 15 bits for the same
pool range, or shrink capacity to roughly 4KB in the existing 12 bits. It
removes no memory indirection and risks more generic sites or word-format work.
An executing-copy-only rewrite also cannot casually change operand fields:
specialization, transparent instrumentation and immutable code-word readers
share their geometry. Preserve word-size, executable offsets and observation
independence; do not create source-only execution metadata to solve a lookup
that does not exist. Reject B without a build or benchmark.

The canonical register-IR document's generic term “feedback index” is a scaled
offset, not evidence of a pointer table; current runtime headers explicitly say
“with no table.” Any older pointer-vector description is stale. The current
round remains frozen: these are follow-up questions and rejections, not new
candidates or permission to reopen preserved experiments.
