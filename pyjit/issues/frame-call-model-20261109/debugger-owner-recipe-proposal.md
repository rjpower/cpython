# Debugger owner recipes: proposed next increment

2026-11-09. Proposal only. The current implementation admits empty incoming
temporary sets plus the reviewed ignored-result pending Python CALL case. It
still rejects useful loop, protected-region and suspended jumps. Existing
behavior tests remain admission requirements; these restrictions are not a
final public contract. No implementation expansion belongs to the current
observer-repair/initial-measurement batch.

## Required state

A debugger destination is a real compact PC and an execution phase, not a
frontend stack depth. Its recipe must specify required homes and their value
provenance, owner identity, structured-region state, exception state and any
pending result/continuation. The current cold `register_resume_sites` already
uses real field roles and window extents; replace its boolean result with a
verified recipe. Do not manufacture a value because a historical stack slot
would have held one, or authorize a home merely because it is non-NULL.

The first target is the two existing failures: a backward jump within one loop
body preserving that loop's iterator/index, and re-executing a folded assignment
without inventing its removed result. Obtain actual word/read/write inventories
for the folded assignment before changing transfer equations.

## Edge-specific dataflow

Generate definitions, reads, retained/through homes and moves from the same IR
contract as execution. Scalar DST can define multiple homes; windows have
argument-dependent extents. A discard defines no home. Branch outputs are
edge-specific: FOR_ITER item exists on the body edge, while completion retires
iterator/index; SEND yield and completion have different recipients. Do not
union conditional definitions and then treat them as guaranteed on both edges.
Error edges use the precise post-failure ownership state advertised by each
semantic handler, including partially published outputs if supported. If the
contract lacks that information, that edge is not an admitted reconstruction.

Value provenance and ownership are separate. COPY produces another owner even
when its value provenance equals its input. A move transfers the owner and
clears its prior home. A borrowed read preserves its anchor. A slot overwrite
kills its old owner and installs a distinct owner. Canonical constants have
permanent valid homes. Folded locals are read from their actual local, not a
missing temporary. Owner merges must describe valid incoming alternatives,
not silently equate unrelated values occupying the same physical slot.

Static producer-PC identity alone is insufficient across loops: one producer
can create a new dynamic value/owner each iteration. A recipe must prove the
current dynamic owner survives into the selected target phase. An initial
bounded proof can admit a connected region with no definition/kill of the
required owner between its relevant phases, or a compiler-proven loop-invariant
anchor whose lifetime covers both phases. Per-iteration values do not acquire
that invariant merely because a reaching-definition set has the same static
PC. Nested-loop re-entry and jumping across an iteration boundary need their
own proof; otherwise reject that particular jump.

## Applying a recipe

Verify all target-required owners and anchors before mutation. Preserve exactly
those homes plus locals/constants; detach surplus owners. Publish the target
PC and exception/structured state before closing detached owners, since closes
can invoke Python. Do not overwrite a successful callback redirect afterward.
No observer may see half-applied aliases or two homes advertising one owner.
The current empty-set path is the special case with no retained temporary homes.

For an active child, use the actual continuation phase. Preserve its overlapping
window untouched until child retirement. The existing pending redirect bit and
plain result recipient remain separate; success applies the recipe while the
result is C-owned, error cancels the pending redirect before and after cleanup
callbacks. Do not infer phase from PC distance or resurrect a discarded result.
External re-entry needs an explicit owner/activation handoff before admission.

## Protected and suspended regions

Derive region identities from compiler structure, not nearest exception-table
range or approximate depth. A recipe names the active handler and exception
owner, with/async-with cleanup obligations, iterator scopes and pending finally
or reraising state. Same numeric handler/slot is not proof of equivalent region
state. Initially admit jumps within an identical verified region with unchanged
required anchors; crossing a region needs explicit semantic enter/leave actions.
Do not synthesize a context manager entry or exception owner that never existed.

Suspended yield/send needs the generator's semantic phase, receiver/index,
sent-value recipient and exception-state link. Yielded and completed outcomes
must use their own recipes. The cleanup-word removal work must share these
contracts; debugger reconstruction must not bring END_SEND/POP temporaries back.

## Verification and ownership

B owns cold recipe generation/validation and frameobject application; A owns
handler outcome contracts and pending child/suspension completion. Root owns
integration and native tests. No ordinary handler gets a liveness walk or a new
per-dispatch cursor. Prefer compiler metadata and cold analysis; validate recipe
coherence after structural code.replace/marshal construction rather than trusting
an unchecked public flag.

Keep current test expectations. Add equal-slot/different-producer and
same-producer/different-iteration rejection, COPY versus move owners, nested
loops, folded assignment replay, context-manager cleanup, nested EH, child
success/error/finalizer re-entry, generator send/throw/close and frame escape.
Check owner reachability and exactly-once retirement without claiming CPython's
historical prompt-destruction coordinates are Python language guarantees.

## Two-case refinement from the build8 word inventory

Read-only native PRINT evidence is in E11
`b-debugger-two-case-inventory-v1.json` / `b-debugger-two-case-values-v1.json`.
The dump compiled the two minimal bodies; it did not execute them or run tests.
The runtime source remains unchanged by this refinement.

### Folded assignment: prove NULL, do not reconstruct a result

The straight-line example has x/y in homes 0/1 and no constant homes:

| PC | Actual operation | Relevant homes |
| --- | --- | --- |
| 1 | LOAD_SMALL_INT_INLINE | defines temporary 2 with 1 |
| 2 | LOAD_GLOBAL | defines callable 3; oparg 0 defines no NULL home |
| 3 | LOAD_CONST | defines argument 5 |
| 4 | CALL | reads window 3,4,5; publishes result 3 |
| 5 | BINARY_OP_ADD | consumes 2/3; writes local x0 |
| 6 | LOAD_SMALL_INT_INLINE | defines temporary 3 with 1 |
| 7 | BINARY_OP_ADD | reads x0/3; writes local y1 |
| 8 | RETURN_VALUE | reads y1 |

The existing debugger jumps from PC 6 back to PC 1. CALL's home 4 is its
self-or-NULL input. The compiler elided PUSH_NULL under the scratch NULL
invariant, so no producer defines an owner there. The current read bitmap
incorrectly demands an incoming owner at PC 1. The folded binary result is
already local x0; there is no removed result temporary to recreate.

Add a cold forward *definitely NULL* fact, with every scratch home NULL at
normal frame entry. A definition changes its output fact; consuming a temporary
clears it; unchanged/through inputs retain their facts. For CALL, distinguish
callable, nullable self-or-NULL and actual arguments rather than treating the
entire window as mandatory owners. A nullable role alone is insufficient:
target home 4 must be definitely NULL on every admitted incoming path, and its
current actual home must also be NULL before redirection. A method call whose
receiver is required must not be admitted after losing that receiver merely
because CALL's raw slot accepts NULL in another shape.

With those facts, PC 1 needs no incoming owner and the existing empty-owner
redirection can safely replay the assignment. This is a small correction to
actual register requirements, independent of the larger loop recipe. No dynamic
tracking or per-dispatch classification is needed.

### Loop body: retain a verified anchor pair in the current iteration

The loop has total/i in 0/1, immutable 5/6 constants in hidden local homes 2/3,
and iterator/cursor in scratch homes 4/5:

| PC | Actual operation | Outcome requirements |
| --- | --- | --- |
| 5 | GET_ITER | creates iterator/cursor pair 4/5 |
| 6 | FOR_ITER A=1,B=4 | body defines i1 and preserves valid pair 4/5 |
| 7 | BINARY_OP_INPLACE_ADD | updates total0 using constant home2 |
| 8 | BINARY_OP_INPLACE_ADD | updates total0 using constant home3 |
| 9 | JUMP_BACKWARD | returns to FOR_ITER 6 |
| 10/11 | current END_FOR/POP_ITER | completion retires loop homes |

The existing test redirects from PC 8 to PC 7. The target needs iterator/cursor
for the subsequent backedge, but no per-iteration item temporary: the item was
published directly into local i1. The current union-format liveness leaves
conditional definitions unmodeled, and the empty-owner gate rejects the pair.

The shortest bounded recipe is an explicit active-loop anchor, not a general
set of static producer IDs. GET_ITER establishes the pair; successful FOR_ITER
body ingress establishes its valid body phase. Required pair availability at
the actual source must hold on **every** incoming path. Analyze paths from the
target to their *first* arrival at the source; both belong to the same active
loop region and no admitted path may kill/rebind/recreate the pair or cross a
completion/re-entry boundary. In this example the successful path contains only
PC 7, which changes a local and cannot replace 4/5. Preserve 4/5, retain locals
and constants, and retire surplus scratch after committing PC 7.

This proof must include source availability plus pathwise survival, not mere
CFG connectivity. A static GET_ITER/FOR_ITER PC can execute again in another
iteration or nested loop. Equal defining PCs or non-NULL homes are not dynamic
owner identity. A target outside the currently active region, or a path crossing
a new GET_ITER/iterator completion, is rejected until an explicit transition
recipe exists. COPY creates a separate owning home; a value-equal copy is not
interchangeable with an anchor that was killed.

## Shared completion facts for the compiler and debugger

The frame worker's next ABI has FOR_ITER A=item recipient, B=iterator/cursor,
and a real exhausted target. Body preserves B, updates B+1 and defines A.
Exhaustion retires B/B+1 and defines no A, preserving the last loop variable.
A Python generator yield publishes to A; its return value is a C-owned discard.

SEND instead yields into B+2, preserving receiver/cursor, and completes into
explicit A (or discards at NO_DST), retiring the completed window and branching
to the real target. A child-return completion helper needs an explicit error
result because STOP_ITERATION/branch callbacks can raise after child retirement.
The held result then retires exactly once under SEND/FOR_ITER's semantic EH
state. Both compiler outcome dataflow and debugger recipes must consume these
facts; the old union fmt.writes is not a guaranteed definition on both edges.

## Mixed observation matrix for the next test batch

Use two tool IDs, one LINE and one INSTRUCTION, with exact code filtering.
Parameterize local/global scope, enable order (LINE first, INSTRUCTION first,
both), and producer kind (ordinary branch twin versus delegated generator with
noninterrupting ingress). This gives twelve main combinations; each runs the
same short phase sequence, rather than an unbounded Cartesian product:

1. Enable LINE alone: source lines occur, private ingress wrappers produce no
   INSTRUCTION callback, and instruction offsets remain real compact PCs.
2. Enable the second tool, disable one LINE source site and one INSTRUCTION
   site independently from their callbacks, then revisit the edge. A disabled
   source line must not reappear through its landing; disabled instruction
   callbacks must not remove ingress tracking needed by LINE.
3. Disable/re-enable each tool independently, then restart events. Verify
   unchanged tool masks survive, disabled sites restart at the same real code
   identity, and no displaced opcode becomes zero or another wrapper.
4. Suspend after a yield; change local/global observation while suspended;
   resume and complete/close. Check native iterator and Python child delegation,
   both normal completion and a callback error with a subsequent clean run.

Add one focused tool-ID free/reuse case and one source path whose lines leave
and return to the same landing line. Keep exact LINE order/counts, exact expected
real INSTRUCTION offsets and callback counts; do not accept arbitrary line
membership or omit unsupported paths. The repeated edge also checks that
DISABLE is local to its source event/site, while restart is explicit.

## Next bounded implementation after the semantic focused gate

Implement in `Objects/frameobject.c` with the existing cold
`pycore_ir_outcomes.h` edge contracts; no handler instrumentation or hot frame
field is needed. Keep the two existing behavior tests unchanged.

1. Factor cold required-input analysis from recipe application. Add a forward
   must-NULL bitset, intersecting incoming edges. Entry scratch is NULL;
   definitions become unknown, consumed scratch becomes NULL, and through
   owners remain. Apply semantic body/completion/resume definitions separately.
   A CALL self-or-NULL input is exempt from owner requirements only when its
   actual read site is definitely NULL on every incoming path. Retain this NULL
   requirement in the target recipe and check the current home is also NULL.
   This admits folded-assignment replay without inventing an old result home
   or treating arbitrary nullable method receivers as missing owners.
2. Add one bounded active FOR_ITER anchor-pair recipe. Its body edge establishes
   the iterator/cursor pair. A forward must-analysis proves availability at
   the current site on every path. Source and target must be in the same body
   region, and every path from target to its first arrival at source must
   preserve that pair: no defining GET_ITER, retirement, consuming move, COPY
   substitution or matching FOR_ITER re-entry. Unknown ingress and paths that
   cross a new iteration before first arrival reject. Static producer-PC
   equality or non-NULL cells alone do not establish dynamic generation.
   The straight-line two-update test needs only this pair; FOR_ITER's item is
   already a named local and its exhaustion edge does not define that local.
3. Apply a verified preserve set rather than clearing every scratch home.
   Allocate bounded cold storage before changing frame state, detach all
   surplus owners without callbacks, publish target/continuation, then close
   only the detached owners. A nested finalizer redirect cannot make the outer
   cleanup clear a newly required frame home. Preserve locals, constants and
   the proven pair; never overwrite callback-selected PC afterward. Leave
   pending-child overlap to the existing empty-owner continuation recipe.

Tests add known-NULL plain-call versus required method-self input, unavailable
or replaced iterator homes, same producer in another iteration, nested-loop
re-entry, COPY versus move ownership, and a retirement finalizer redirect.
The original folded assignment and loop-body jumps must pass with their exact
results. Protected/suspended region transitions remain separate recipes and
retain their tests; this bounded increment does not claim their completion.
Implementation waits for the coordinator's native window and source admission.
