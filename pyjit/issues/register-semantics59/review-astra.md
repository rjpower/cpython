# Register-semantics59 architectural review ledger

Status: architectural source review complete, 2026-11-14. No runtime edits, builds,
generation, tests or performance executions were performed for this review.
The [proposal](../../design/proposals/register-semantics59.md) contains the
implementation contracts; this file records their evidence and limits.

## Authority and sources

Canonical documentation is `/home/power/code/cpython/.worktrees/python-jit`.
Actual source is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`
at selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`. Pinned upstream is
the sibling `upstream-3a440b6`, commit
`3a440b65695615e9fd9912342b79cdd54fced0d2`. Canonical-checkout runtime sources
are stale and were not used as implementation evidence. Applicable AGENTS,
the CPython AI policy, README, GOAL, STATUS, HANDOFF and PROCESS govern this work.

The [round58 ledger](../interpreter-parity58/research.md) retains generated-C,
native-symbol and upstream comparisons. Its measurements and rejected57
experiments remain evidence; they are not new round59 results.

## First result-binding finding

`Include/internal/pycore_ir_outcomes.h:_PyIR_Outcome` already gives FOR_ITER's
BODY edge destination A and its exhaustion edge the iterator/cursor retirement.
`Tools/cases_generator/ir_generator.py:handler_output_kind` selects OUTCOME when
the producer contains `IR_COMPLETE_STEP`; selected FOR_ITER_LIST does.
`Python/bytecodes.c:_ITER_NEXT_LIST` produces an owning `next` using
`PyValue_XDUP`, then advances the tagged cursor. `IRStack.store` nevertheless
routes every ordinary window output through `IR_PUBLISH_TEMP(window[offset],
var)`. `emit_final_outputs` subsequently moves the window result to A. Thus the
first defect is lowering of an already explicit semantic result, not missing
semantic outcomes.

Do not redirect `SlotLayout.address` globally. The generator may flush before
final publication, and early observer/error states still require their declared
homes. An actual final-publication receipt must distinguish these cases.
Likewise `_PyIR_PhysicalTransfer` does not grant unconditional NULL from
logical absence. Independent peer review found compiler `compact_op` leaves
folded jump output B+2 unoccupied, but `Objects/codeobject.c`'s geometry check
only bounds the span; the unknown FOR semantic leaf does not prove incoming
physical NULL. The first proposal therefore retains explicit B+2=NULL at
commit (`VACATE_SOURCE`). `KEEP_NULL` is deferred until positive admission.

## What is actually coupled

All source paths below refer to selected I, not the canonical checkout's C
files. Line numbers are anchors in selected57; symbols carry the meaning.

| Source fact | Architectural implication |
| --- | --- |
| `pycore_regcfg.h:15–100` defines virtual names by stack position at joins and describes an op through original opcode, stack-ordered inputs, `npop`, implicit exception uses and stack outputs | The register graph is not yet an independent semantic operation graph |
| `regcfg.c:457` canonicalises by renaming previous ops, not by emitting copies | Canonicalisation has no automatic per-edge instruction saving; show a constrained allocation/ownership consumer |
| `regcfg.c:518–524` canonicalises exception prefixes and jumps; `:542–573` gives nonjump outputs fresh virtual registers | Calls already have distinct compiler output IDs; do not claim otherwise |
| `regcfg.c:962–1025` frees taken inputs, requires consecutive window inputs and assigns window outputs at base+j | Input packing and output placement are conflated; this is the first real allocation freedom in trial 2 |
| `regcfg.c:1080` omits an occupied output for folded locals/jump-only outputs, but still computes the output's slot/frame extent | A fictitious result home can survive as geometry even when it owns nothing |
| `ir_generator.py:1236–1268` finds a window result by new output or assignment to an input C variable | Explicit native result tokens can replace inference, independently of the compiler's existing fresh result |
| `ir_generator.py:3002` routes window stores uniformly; `:6485` flushes then moves the final result | Existing edge semantics are lost in physical publication |
| `Objects/codeobject.c:1940–1963` bounds full read/write spans and enforces ordinary window scratch result at base+offset | A changed allocator requires changed constructor admission, not just a compiler flag |
| `pycore_interpframe.h:283` explains that slot order is stack order and prefix/range clearing relies on it | Arbitrary allocation requires a materialization or explicit retained-home map at effects; it cannot remove the invariant without a reader replacement |

The first allocation packet uses BUILD_LIST/TUPLE because the selected
`bytecodes.c:2294–2311` already expresses steal-on-success, `ERROR_NO_POP` on
failure and `INPUTS_DEAD` only afterward. The input span remains valid through
allocation. After successful stealing, an independent result can go directly
to its admitted destination. Extending the same geometry to FOR requires all
family members and deoptimization paths, including generator entry, to agree.

## Calls and continuations: useful changes versus already selected behavior

`bytecodes.c:5010`'s `_CALL_METHOD_DESCRIPTOR_FAST` produces a C-local result,
assigns it to `callable` and closes the old callable. Its macro then performs
argument/self cleanup and a periodic check. These phases form one dispatched
handler. Retained native `_TAIL_CALL_CALL_METHOD_DESCRIPTOR_FAST` at `0x390a50`
contains vectorcall dispatch, argument close/NULL work and a later clear loop.
Trial 3 proposes one explicit result-owner and retirement contract. It must
prove the exact duplicate physical work in generated release C before changing
it; the DSL's appearance alone is not that proof.

The generic path is already further along: `ceval.c:1693–1867`
`call_transaction_native` detaches the window, publishes a scratch recipient
directly, holds `local_result` until cleanup/periodic success, and leaves it in
the error state on failure. Trial 3 does not claim that generic path still
needs a window-to-result roundtrip. The chosen provider stays the same so it
does not reintroduce the rejected57 checked-native-entry experiment.

Exact Python arguments already overlap in `call_transaction_overlap_python`
and guarded exact binding; warm initialization uses the existing frontier.
`return_slot` and `return_pc` already supply a recipient and executable
continuation. `pycore_interpframe.h:235–275` classifies local/scratch/discard
and handles pending redirects on completion. Trial 4 proposes carrying that
classification as an admitted action, not adding continuations. Its cost must
include masked descriptor reads, remote-debugging readers and every writer.

Source census of `return_slot` readers/writers includes `ceval.c`, generator
yield/send/return paths, `pycore_interpframe.h`, `Python/ir.c`, `frame.c`,
`Objects/frameobject.c`, `genobject.c`, `pystate.c`, remote-debugging asyncio
and offsets, and testinternalcapi. This is why a single fast return helper is
not a complete implementation. Retained RETURN_VALUE_EMPTY, SEND_GEN and
YIELD_VALUE native paths still classify recipients, but existing direct
generator completion and checked recipients must not be counted twice.

The debug home scan in call-transaction entry is absent from the release
binary. Selected frames have a 112-byte GIL header; the 80-byte header trial
lost. Handler ABI already uses no-callee-saved/preserve-none conventions, with
a normal-ABI initial shim for the documented GCC15.2 entry problem. Getter
spills exist; ordinary per-handler SysV save/restore overhead is not the cause
shown by the retained FOR_ITER_LIST disassembly.

## Borrowing and the CFG opportunity

Selected `regcfg_passes.c:155–186` already folds a local read into a borrowable
consumer field. It requires a single consuming reader and rejects changes of
line, writes and callback-capable intervening operations. An unfolded ordinary
home owns the result. Pinned upstream `pycore_stackref.h:546,638,672,699`
marks borrows, avoids refcount updates in borrow DUP/CLOSE and materializes a
strong reference for heap safety. Upstream's compiler/escape rules are part of
that protocol; copying the bit representation alone would be invalid.

Round58's 21-file/426-CFG closed COPY-web census found zero eligible sites.
Trial 5 is a distinct, named morphology: acyclic edge-selected snapshots with
an actual surviving allocation/ownership cost and a consuming or multi-use
join. Its existence in source syntax is insufficient. If captured optimized
IR has no such opportunity, the lane is demoted. A borrowed-argument frame ABI
is a separate, broader ownership experiment requiring observer, frame scanning,
escape, suspension and retirement admission. It is not smuggled into C-local
residency or the first allocation packet.

## Common paths and budget

Round58's trace A records the selected comprehension inner dict.get loop as six
IR words versus upstream's eight bytecodes. There are 108 dict.get, 162 append,
54 dictionary subscriptions, 42 predicate calls and 18 generator creations per
tray. Dispatch count alone therefore cannot explain a promised advantage.
The selected native list iteration still performs getter-related spills,
generic cursor/item publication and the local-output roundtrip. SHAPE_LOCAL_TEMP
already avoids the ordinary helper/receiver cost and is the negative control
against a universal new abstraction tax.

Retained selected57 instructions/cycles versus pinned upstream are:
comprehensions 1.436416/1.333179, DeltaBlue 1.361424/1.118423, coroutines
1.307831/1.267788. These mean 30.38%/24.99%, 26.55%/10.59% and
23.54%/21.12% of candidate work must disappear for equal counts/cycles.
Comprehension excess is about 84,620 retired instructions per tray. A
hypothetical 20 instructions removed from each of 270 native calls saves 5,400,
only 6.38% of that excess. This arithmetic is a scale test, not a forecast.

The rejected57 four-lane composition removed about 4.8% of comprehension
instructions versus56 while losing logging 13.2%, coroutines 4.6% and remaining
flat across29. Uniform alignment and local append also lost. The failed
frontend capture supplies no exclusive cache/layout explanation. Selected57's
all29 .995586/56 and 1.127137/upstream are observed two-pair aggregates with
opposing XML outliers, not a precise global parity gap. No rejected or flat
sample is a saving available for the new composition.

## Review disposition

The [independent first-lane peer](peer.md) admits final-local publication with
VACATE_SOURCE for implementation, subject to actual diff/native review. The
broader architecture is a proposal with six concrete trials and falsifiers;
none is claimed integrated or measured here. Root is implementing the first
lane while preparing the compiler allocation packet.

The important correction from58 is to allow the operation model and physical
format to change when a consumer proves the need, while respecting the current
EH/observer/storage contracts until their replacements are admitted. The
important correction within59 is that semantic outputs/continuations and
compiler call result IDs already exist. The work is to preserve them through
native ownership lowering and to remove genuine stack-geometry constraints.

The small trials alone cannot plausibly supply the entire parity budget.
Cross-operation residency, less restrictive allocation and/or a broader
execution backend must provide measured coverage beyond them. If they do not,
change the execution choice and continue the review/implementation cycle;
do not call the architectural document completion of the user's task.

Validation of this review consists of selected-source inspection, retained
generated-C/native evidence, arithmetic and independent peer reconciliation.
No candidate test/performance result is claimed. The source/native ledger in58
remains the detailed trace for dict.get/iteration/append, exact call/return,
attribute reads and resume/yield. External V8/LuaJIT examples remain historical
implementation comparisons at the commits recorded there; no new external
performance or Python-applicability claim is introduced by59.
