# General return and the empty-return certificate

Integrated at `ff14b98be4b`: GC runtime `a35d826f04e`, IR forms/selection
`ae26fb90aa0`, generated products and fixture ports. Both native builds
complete117/79; six focused modules pass 67 cases per role. Broader IR/debugger
qualification remains open in the [report](report.md). NULL control remains
authoritative; this is not a stale-scratch authority switch.

## Actual gap

`semantic_leaf_step` currently rejects any residual `SEM_MAY_OWNER` after
TRANSFER_EXIT consumes the result. Remaining frame-owned scratch is not
itself a language error. It must pass to frame retirement. However, simply
removing the rejection is unsafe in the current implementation:

- `_RETURN_VALUE` asserts all scratch NULL before unlinking.
- `_PyFrame_TryClearExceptCode` limits THREAD retirement to L and asserts
  scratch already empty; heap/generator retirement selects N.
- `_PyFrame_InitClearRoots` likewise selects L on THREAD association takeover.

The existing terminal-observer fixture exposes this gap when its synthetic
RETURN retains an exception owner. Do not add POPs merely to force the
fixture into the narrower runtime model. The separate synthetic SWAP failure
was a real invalid read of a cleared endpoint and has its own fixture repair.

## Canonical forms

`RETURN_VALUE` is the general language operation. Add an IR-only
`RETURN_VALUE_EMPTY` form and its instrumented twin, preserving the same A
result geometry. The compiler selects EMPTY only with positive absence
evidence; every public constructor, replacement and marshal route validates
that certificate. General RETURN still validates the result read/take and
rejects overwriting or duplicating ownership incorrectly. Only EMPTY requires
no remaining scratch owners. MAY facts alone never authorize cleanup reads.

The existing empty handler becomes EMPTY with unchanged retirement arguments
and no added runtime query or branch. Remove its blind scratch-NULL debug
assertion: logical absence does not authorize reading stale scratch words.
The retirement debug checker validates only its actual readable L extent.
General RETURN
uses a distinct outlined result-aware retirement entry. Both complete through
the same normal caller/SEND/FOR result protocol. Instrumentation activation,
deactivation, adaptive metadata, disassembly and deopt preserve the selected
form; mapping both to a single legacy handler would lose the contract.

Empty-owner selection is distinct from `semantic_exact_return`, whose current
NULL-shadow certificate requires physical NULL and no owner in every scratch
home. The latter must remain strict. An absence certificate cannot imply
physical NULL or readable inactive words. The proposed compiler selector
uses the common encoded ownership analysis rather than a second instruction
ledger or a trusted source-position hint. Assembly emits general RETURN.
Universal constructor validation selects EMPTY during the existing stable
post-transfer inspection, requiring every scratch home NO_HOME and no owner
obligation. Explicit EMPTY requires the same independent proof. Collect
rewrites into a lazy private word copy, never the borrowed image while proof
runs; the return normalizer makes at most one additional image copy and uses
O(1) extra proof state. Earlier `check_words` normalization can independently
copy words, for example when normalizing inline constants. Return the normalized
words through existing validation/construction. Exec-plan building validates
existing immutable words without rewriting them. All public routes share this
path; there is no compiler-trust bypass or second dataflow analysis.

## Actual retirement entry

Implemented private entry: `_PyEval_FrameClearAndPopGeneralWithResult(tstate,
frame, result)`, reached only by the general handler. The detached result is
passed and returned by value. It selects the current leaf frame's initialized
extent N before cleanup, then uses the existing M2 reservation and fixed-last
code-pin rules. No full-N read is permitted merely because a suspended or
waiting frame has code; this entry is the actual general return producer.

Share the Try/Finish algorithm in `frame.c` through a private implementation
whose explicit initial extent is supplied by its real caller. Keep the
existing empty THREAD entry selecting L, with no extra public hot argument.
Add `_PyFrame_TryClearExceptCodeGeneral(frame)` selecting THREAD N;
generator paths retain their
existing actual full-frame semantics. The outlined helper links the existing
40-byte root record before the first close that can reenter. The association
case must receive N explicitly: calling the current initializer and accepting
its L reconstruction would lose scratch during frame takeover.

Frozen cold initializer: `_PyFrame_InitClearRoots(frame, stage,
initial_extent, result, roots)`. Its three existing producers are reached
slow helpers or heap deallocation, so this adds no ordinary return argument.
The shared ceval slow helper takes extent as its sixth argument; the normal
EMPTY fast call signature is unchanged. Preserve cursor-before-close, header
authority removal before release, passed named-binding mutation semantics,
result rooting through final code close, and pop-before-reservation release.
Use existing shared Finish cleanup rather than a second handwritten drain.

For a later stale-scratch switch this entry must receive the selected readable
remaining shape, just as the error-to-M2 handoff must. Current NULL-control
full N is an explicit development capability, not a future fallback. General
RETURN with a nonempty or uncovered shape does not acquire an empty-return
certificate merely because cleanup succeeds.

The current storage proof is concrete: frame headers grow down from the arena
limit, below which active value storage ends. `_PyFrame_ReserveForCall` keeps
those extents disjoint; `_PyStackChunk_EnsureFrameStorage` truncates the
initialized frontier before header writes and NULL-fills newly exposed values
that cross old header storage. Retirement leaves the departing overlapping
value suffix NULL. This supports current initialized full-N control; abstract
NO_HOME alone does not prove a home readable or physically NULL.

## Source boundaries and validation

IR owns canonical IDs/metadata/family, all bytecodes and instrumentation
mapping, common admission/proof distinction and constructor normalization.
GC owns `frame.c`, the general ceval runtime entry, their prototypes, the heap
deallocator's cold initializer call and `test_ir_general_return.py`;
frames peers the extent/takeover/code lifetime changes. Assembly/compiler
selection requires no independent assembly pass. Root
owns generated products, integration and native execution.

Tests must admit unique residual owners through all construction/marshal
routes using general RETURN, reject forged EMPTY with the same residual,
retain unavailable-result and overwrite negatives, and keep nonempty returns
outside exact-empty plans. Actual execution must show a last-owner scratch
finalizer with GC/precise root inspection, a still-live detached result and
code pin, association takeover, and final root-chain exhaustion. Keep both
diamond predecessor orders; joins cannot erase a residual owner obligation.
The existing terminal-observer fixture should then be valid without added
cleanup words.

Cost: two IR-only finite handlers and instrumentation mapping, a reached
outlined full-extent scan/record on the general form, and cold compiler/admission
analysis. No new hot frame field, tail-call argument or per-return decoder.
Native inspection must verify EMPTY still has the old body/prologue; measure
how often compiler proof selects general before claiming ordinary return cost
is preserved. Full readable/error-phase coverage remains a separate M4 gate.

The [shared transport plan](semantic-phase-plan.md) owns map closure and
separation of semantic recovery from optional positions.
