# Register-home validity and ownership inventory

Source audit for round 54. Runtime references below are to
`build/worktrees/round51-golden`, selected commit
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`, not the older runtime in the
canonical documentation checkout. This audit changes documentation only; it
does not run a compiler, interpreter, test, or benchmark.

## Main finding

`PyValue_NULL` records whether a physical home contains a valid value and owns
that value. It is not merely a teardown marker. Consumers include generated
instructions, locals exposure, debugger reconstruction, GC, exception recipes,
frame copying, and finalizers that reenter after a partial retirement. A bitmap
consulted only during teardown would leave those consumers reading stale
values. Inline values and tagged iterator indices are valid occupied homes even
when they own no counted object reference.

The selected runtime already avoids clearing the whole arena on each ordinary
call. The stack chunk's initialized frontier fills only an unknown tail;
overlapping argument homes transfer ownership without copying. Ordinary thread
frame return proves scratch homes empty and clears the named-local prefix.
Generator-owned and independently owned frames have different cleanup rules.
The three rejected round-53 candidates and the previously rejected round-51
generator local-only cleanup are not new experiments here.

## Reader and writer contracts

| Area | Source anchors | Required state |
| --- | --- | --- |
| Arena initialization | `Python/pystate.c:_PyStackChunk_ExtendInitialized`; `Include/internal/pycore_interpframe.h:_PyStackChunk_EnsureFrameStorage` | Only the previously unknown suffix is filled with NULL. Header overwrite can truncate the frontier first. The frontier proves initialized storage, not current ownership. A callback must not restore an obsolete frontier. |
| Overlapping exact calls | `pycore_interpframe.h:_PyFrame_ReserveForCall`, `_PyFrame_PushOverlapping`; `Python/bytecodes.c:_INIT_CALL_PY_EXACT_ARGS` | Arguments become the child's homes in the same physical cells; the parent ceases owning them. The remaining shared suffix is empty. Pop restores the saved arena top and requires the departing suffix empty. |
| Input consumption | `Python/ceval_macros.h:IR_CONSUME_INPUT`, `IR_TAKE`; `Tools/cases_generator/ir_generator.py:IRStack.consume` | A local input remains anchored and borrowed until duplicated or explicitly moved. A scratch input transfers its owner to C and immediately becomes NULL. No borrowed C bits may survive an escaping operation without a home. |
| Result replacement | `ceval_macros.h` result publication; `pycore_interpframe.h:_PyFrame_PublishReturnSlot` | Install the new owner before closing the displaced local owner. Scratch destinations must already be empty. Displaced-owner destruction can call Python and inspect the committed state. |
| Generated vacating | `ir_generator.py:IRStack.vacate`, `flush` | Vacate runs before output saving. Window inputs use their consume rules; surrendered output positions become NULL. A same-home overwrite proof must cover this actual ordering, aliasing, and every intervening escape. |
| Return and unwind | `bytecodes.c:RETURN_VALUE`, `exception_unwind`; `Python/frame.c:clear_slots`, `_PyFrame_ClearExceptCode` | The result owner is held across unlink and cleanup. Retirement writes NULL before each close. Exception recipes preserve anchors, clear the required suffix, and then install handler inputs. Thread scratch emptiness is already proved; ordinary return does not unconditionally clear all physical homes. |
| Completion protocols | `Python/ceval.c:complete_step_impl`; `pycore_ir_outcomes.h`; `Objects/codeobject.c` completion validation | PRE observation sees the old anchored window. Commit detaches owners, clears homes, and publishes result/continuation before POST observation and retirement. The result can alias an input. Static future-use liveness alone cannot describe these phases. |
| GC | `Python/gc.c:_PyGC_VisitFrameStack`; `gc_free_threading.c` frame-stack visitation; `gc_trace.c:_PyGC_TraceVisitFrames` | Visit all physical homes, skipping NULL and nonobject values through canonical PyValue operations. The conservative thread-arena scanner is an additional reader. A new validity representation must define both precise visitation and conservative stale-address behavior. |
| Exposure and takeover | `Objects/frameobject.c` locals proxy, jump planning/commit, frame traversal/clear; `Python/frame.c:take_ownership` | Unbound locals are NULL. Jump planning depends on actual occupancy. Escaped frame takeover precedes shortened thread cleanup. Debugger mutation commits continuation state before closing retired owners. |
| Generator suspension and copying | `bytecodes.c:YIELD_VALUE`, `RETURN_GENERATOR`; `pycore_interpframe.h:_PyFrame_Copy`; `Objects/genobject.c` traversal/clear | Suspension publishes the saved frame, exception state, and recipient before closing displaced owners. Copy moves every physical home and NULLs its source. Suspended frames remain GC-visible. A bitmap must move with ownership, including transition from overlapping storage to independent storage. |
| Redirects and callbacks | `Python/frame.c:_PyFrame_CompleteRedirect`; `pycore_interpframe.h:_PyFrame_SetReturnValue`; `frameobject.c` pending-child jump commit | Pending redirect wins before destination classification. CompleteRedirect commits the winning PC and invalidates old error authority before clearing scratch owners. No destination proof may bypass this ordering, including same-PC redirects. |

`Python/regcfg.c` tracks physical owner occupancy through consumption. That is
stronger than future-read liveness: an object with no later read can still own a
reference whose destruction order and callbacks are observable. Existing
canonical designs distinguish initialized storage, owning occupancy, and
future-read liveness (`call-and-frame-lifetimes.md`, `register-ir.md`).

## Representation choices

**Dynamic bitmap.** A valid-home bitmap requires a set on each newly occupied
home and a clear on each transfer or retirement, before any callback. Reads
must test it or establish validity by proof; GC, copying, unwind, locals,
debugger mutation, and suspended frames must use it. Overlapping frames cannot
have divergent bits for the same physical owner. A per-frame bitmap therefore
needs explicit transfer or shared-storage geometry. The bitmap may remove
NULL stores, but adds bit addressing and read-modify-write traffic to the same
hot paths. A bitmap of counted references alone cannot establish whether inline
or index values are readable. There is no source evidence yet that this wins.

**Static occupancy maps.** They remove per-transfer bitmap writes, but must
describe the exact stopped phase: before/after commit, callback escape, error
recipe, pending child, and debugger mutation. A map keyed only by instruction
PC and future reads is insufficient. Named locals also have dynamic boundness.
A narrow producer proof can avoid a general map and its lookup cost.

**Dead tags in PyValue.** Canonical PyValue bits already encode object count
ownership, immortality, inline scalars, and indices. No universally free bit was
found. A dead-tag write still writes the home and requires every canonical
reader to understand it; it does not by itself remove consume traffic.

## Bounded experiments

### 1. Prove an empty return destination

Use an upper `return_slot` bit to distinguish a producer-proved empty scratch
destination from a replaceable local. Keep discard explicit. Resolve pending
redirect first, then classify the winning destination. On the proved-empty
path remove the old-home load and its XClose; retain the incoming owner check,
publication, continuation, and callback ordering. This does **not** remove the
consumer's earlier NULL store. Added work is a proof-bit test at publication
and the producer's encoded classification; measure whether that is cheaper than
the existing load and close classification. Do not introduce a dynamic local
comparison on every transfer without measuring it.

`return_slot` is 16 bits while normal IR homes and the no-destination sentinel
use 12 bits. This is representational room, not permission to treat existing
numeric readers as compatible. Mask before sentinel comparison, bounds checks,
or arithmetic in:

- `_PyFrame_PublishReturnSlot`, `_PyFrame_SetReturnValue`, and redirect assertions;
- `Objects/genobject.c:_PyGen_YieldFromRef`, especially `return_slot - 2`;
- `Objects/frameobject.c` pending-call validation;
- `Modules/_remote_debugging/asyncio.c`, including its raw uint16 reader and
  receiver delta calculation.

Audit all producers: Initialize, copy, interpreter shim setup, `_SAVE_RETURN_SLOT`,
YIELD, RETURN_GENERATOR, exact SEND/FOR entry, native callers, and debugger
redirects. Copy may preserve a valid proof; a newly selected destination must
recompute or clear it. The remote offset's unchanged field size does not prove
unchanged numeric semantics. Constructor or public frame mutations must not be
able to forge an empty-home proof. Retain debug assertions against the real
home on the specialized path.

### 2. Elide a NULL store immediately overwritten in one handler

First inspect the selected native artifact: establish an executed NULL store
that native dead-store elimination has not already removed. Then require an
exact physical-alias proof that the home is overwritten before any callback,
GC poll, error/deopt exit, dispatch, or frame switch. All input loads must
precede the overwrite; each displaced owner must remain held in C or retire
exactly once. Final home state and destructor ordering must match the generic
handler. Add a checked eligibility fallback when the proof is dynamic.

Removed work is the surviving NULL store; added work may include an alias
check/branch and generated code size. Reject the candidate before implementation
if the store is already removed, or if the check exceeds the removed work.
The generator's `vacate` followed by `save_variables` is a place to search, not
proof that such a store survives native compilation. Ordinary return, child
handoff, suspension, and completion-owner retirement cross observable phases
and are not blanket candidates.

### 3. Defer a static creation-map pilot

A later bounded comparator could preserve generator allocation order and
header copying, zero the destination, and move only producer-proved occupied
homes. Against the current full physical move (load, destination store, source
NULL per home), it adds destination zeroing and map/index reads but avoids moves
for proven holes. It requires a creation-point occupancy proof, not general
future-use liveness, and must include dynamic argument/local boundness. This is
not early heap construction or a revival of the rejected round-53 candidates.
It is lower priority than the two narrower experiments and needs a weighted
source/native inventory before implementation.

## Validation boundary

This inventory is source evidence, not a performance result. Root owns native
inspection, generation, execution, and paired measurements. Tests for any
accepted recipient change must cover local replacement finalizers, proved-empty
scratch, discard, native and exact generator entry, escaped and suspended frames,
pending redirects including same-PC intent, generator yield-from inspection,
and remote recipient decoding. Store-elision tests must exercise aliasing and
every rejected escape boundary; tests alone cannot establish the absence of an
escape in generated code.
