# Bounded debugger recipe implementation

Proposal/pseudocode while source `3e5cbdb1123` is frozen for measurement. No C or
test changes and no worker native jobs. Implement the folded-assignment case
first, then the active FOR_ITER anchor case. Existing behavior tests stay
unchanged. Protected/suspended transitions are a separate increment, not a
permanent rejection policy.

## Files and shared contracts

B owns `Objects/frameobject.c` cold analysis, target selection and application,
plus focused tests in `test_ir.py`, `test_code.py` and a small new debugger test
module if needed. The existing `pycore_ir_outcomes.h` supplies ReadsHome,
DefinesHome, NullableInputs and per-edge Outcome. Add a shared nullable-home
translation helper only to avoid copying the constructor's role/window mapping;
A peers that helper. A owns any real handler contract correction or active-child
completion change. Neither worker changes an ordinary handler or adds a hot
frame field for these recipes. Root owns native validation and measurements.

Replace the boolean-only analysis result with owned cold storage:

```
ResumeAnalysis {
    nwords, nlocalsplus, ntemps, stride;
    required_before[nwords][stride];
    null_required_before[nwords][stride];
    invariant_null[stride];
}
ResumeRecipe {
    target_pc;
    preserve[stride];
    require_null[stride];
    kind = EMPTY | FOR_BODY;
    anchor_site, anchor_base;  // FOR_BODY only
}
```

Keep allocation-size checks and failure cleanup explicit. Analysis lives only
for one setter call. Do not add cached code flags trusted through code.replace.
Factor the existing successor calculation once: real compact fallthrough/taken
PCs, JUMP_ALWAYS/scope exits, and separate edge Outcomes. Relaxation helpers are
real nodes; do not project them back into removed frontend stack instructions.

## Increment 1: a strong, cheap must-NULL fact

Start with an invariant sufficient for the actual folded-store case: a scratch
home starts NULL and no complete instruction can create an owner there on any
normal path. This is deliberately more conservative than a universal forward
input-consumption analysis. It is a must-NULL fact at admitted instruction
boundary phases, not during an opaque call's internal binder transaction or
while a child owns an overlapping window.

Compute a may-write mask across all real words. Shared DefinesHome supplies
ordinary actual outputs; both Outcomes conservatively mark their affected ranges
and recipients as possibly written. SWAP/permutation endpoints are writes even
though they are not fresh definitions. Unknown transports or overlapping-frame
effects exclude the affected home from the invariant. Named-local operand fields
must remain within nlocalsplus before using their exclusion from scratch; do not
let forged LOCAL_OUT fields bypass the proof. Constants/locals are not invented
scratch owners. A NULL-producing instruction may conservatively count as a
writer in this first increment.

```
may_write = union(actual possible output/transport homes of every instruction)
invariant_null = entry_scratch - may_write

for each backward liveness edge:
    needs = successor.required
    nulls = successor.null_required
    remove this edge's actual definitions/retirements from outgoing needs
    // Ordinary definitions use DefinesHome, not all fmt.writes or through.
    add every actual ReadsHome operand:
        if invariant_null[home] && actual_reader_accepts_NULL(home):
            nulls.add(home)
        else:
            needs.add(home)
```

The nullable role alone never proves NULL. The target recipe carries the NULL
requirement, checks the current home is actually NULL, and validates the actual
reader handler's capability (unwrap transparent observation wrappers). A generic
STORE_ATTR deletion capability does not grant its specialized handlers NULL.
Use the shared exact role translation; never infer NULL tolerance from unused
spelling, a CLOSE/XCLOSE, or a whole-window exemption.

The folded example's CALL self home4 has no possible boundary writer. Its
callable/argument homes3/5 do, so they receive no exemption. At the y-line source,
the target x-line needs no incoming temporary owner and requires home4 NULL.
No removed arithmetic result is reconstructed: x is already a local. Ordinary
CALL internal method expansion can transiently use its self home, but CALL/return
callbacks are not admitted setter phases; successful completion retires its
window. An active child phase does not use this boundary invariant.

This first proof may reject a home that becomes NULL after a real definition.
That is an explicit extension point, not authorization to guess generic kills.
Add a forward lattice only when a real positive requires it and its consuming
contract is available. Do not block this case on a universal ownership rewrite.

## Increment 2: one current FOR body anchor

Preserve only one iterator/cursor pair. The target and current required-owner
sets must fit that pair, with no unrelated per-iteration temporary. The iterator
is a real owner; its cursor may be NULL or a tagged index. Check the actual
receiver is non-NULL and cursor has the existing iterator ABI shape. NULL cursor
is preserved as protocol state; it is not an absent iterator owner.

For a candidate FOR_ITER at p, define the body phase by its successful body edge.
Cut analysis at its next execution, exhaustion, any definition/retirement/move
of either anchor home, and unknown physical clobbers. Outside the establishing
FOR_ITER, an actual read of the pair is conservatively a barrier too; COPY cannot
substitute a value-equal owner and SWAP cannot silently transport it. Through
holes untouched by ReadsHome/DefinesHome remain preserved. Reject ambiguous
inline-child overlap instead of assuming fmt.reads bounds every child write.

Require source availability on every incoming path: the body edge dominates
both source and target, and no alternate ingress bypasses the phase or replaces
its anchors. For the first backward body jump, the target must dominate the
current site within this cut body graph. Every path from target to its first
arrival at current must preserve the pair; reaching the candidate header,
completion or a new initialization before arrival invalidates that path/recipe.
There must be a real target-to-current path. Nodes whose incoming state is
unknown are not admitted merely because they are connected.

```
body_entry = actual fallthrough successor(p)  // may be a real relaxation helper
region = body reachability, cut at p and every anchor clobber/exit
require body_edge_dominates(current) && body_edge_dominates(target)
require target_dominates_current_inside(region)
require all target-to-first-current paths preserve both homes
require required[current] subset pair && required[target] subset pair
preserve = pair
require_null = target's independently proven NULL requirements
```

This proves the current dynamic pair survived from the same successful body
phase. Equal producer PCs, equal values and non-NULL cells are not the proof.
In particular a path crossing FOR_ITER again or a nested-loop reinitialization
cannot establish old per-iteration ownership. The existing two-add test has a
straight path from PC7 to PC8, changing only total; item i is already a local.
Its former END_FOR/POP_ITER words are gone and must not return in a recipe.
Forward body jumps, multiple active pairs and richer regions can follow after
this exact backward case is verified.

## Applying either recipe

Keep target selection separate from mutation. For the initial increment retain
the current pending-child behavior without broadening it: no anchor preservation
or scratch inspection while a child owns the overlap. Its existing deferred
empty-owner completion remains a separate phase and helper.

Unbound-local warnings can invoke Python. Emit any existing warning before the
final analysis/validation, snapshot the frame/PC/continuation/source cursor, and
honor a nested successful redirect if that callback changes the phase. Re-read
locals and execution handlers afterward; bind None only to still-NULL locals
under the existing debugger policy. There must be no callback between final
recipe validation and detachment/commit.

```
recipe = verify_target(current actual phase, requested source line)
validate current preserved owners and required-NULL homes
allocate detached_values[n_surplus]  // before mutating anything
for each surplus scratch home:
    detached_values.append(home.value)
    home.value = NULL                // no callbacks
clear pending LINE/source cursors
publish target instr_ptr and return_pc
for each detached value:
    XCLOSE(value)                    // can reenter and redirect again
free detached_values
return success                     // no further frame stores or cached PC reload
```

Detach **all** surplus homes before the first close. The old sequential
clear-and-close loop can let a nested redirect install new needed owners, then
clear them on its next iteration. C-held values are independent owners; freeing
the cold array after closes cannot touch frame cells. Never overwrite a callback's
successful target or inspect scratch after reentry. Locals/constants and the
verified pair remain in place. Allocate only the required cold storage, with
checked sizes and clean failure before mutation.

## Minimal verification

First keep `test_ir.StoreFoldTest.test_set_lineno_after_folded_store` unchanged.
Add a plain-call NULL self positive and an otherwise similar method-receiver
negative; a scratch home with a real writer must not acquire the invariant.
Test the actual-handler NULL capability rather than only a pristine generic row.

Then keep `test_code.ConstSlotsTest.test_settrace_lineno_jump` unchanged. Add one
exact-result body replay test for native-indexed and native-iterator sources,
a replaced-anchor path, a header/re-entry path and a COPY/transport path. These
negative cases verify proof refusal without changing existing debugger behavior
assertions. Add one surplus-owner finalizer that attempts a nested permitted
redirect and verifies the outer setter never clears its new homes afterward.
No wall-clock unit assertions or per-dispatch tracking.

Root runs the two originals first, then focused frame/trace/monitoring cases and
the new failure/retirement tests. Protected, suspended and multi-anchor cases
retain their original tests for the next recipe increment; this plan does not
claim full f_lineno support or final performance admission.

## First NULL increment: implementation review pending

The first implementation uses the whole-code no-boundary-writer invariant,
current NULL snapshot and actual reader handler. It replaces the old duplicate
input/output projection with the shared ReadsHome/DefinesHome contracts. SWAP
endpoints and unknown frame-pushing tail overlap exclude affected homes. Active
children retain the raw owner analysis and never inspect overlap for NULL facts.
The constructor and debugger now share NullableHome's exact role translation.

The setter detaches all surplus scratch owners before committing and closing;
it repeats the cold proof after a warning callback unless the callback visibly
redirected the frame. Three focused cases cover warmed plain CALL, required
method receiver refusal and warning-hook nested redirection. The original folded
assignment test is unchanged. Host AST and diff checks passed; native admission
belongs to the coordinator and is pending.

A pending lifecycle question is detecting a nested setter that deliberately
selects a target identical to the already-published source snapshot. Comparing
PC/cursor/continuation fields cannot prove absence of that call. A cold frame
redirect generation or scoped cancellation chain would make nested precedence
explicit without an ordinary dispatch cost; coordinator admission is pending.
The FOR anchor recipe remains a separate next increment.

The lifecycle question above is resolved in the reviewed source increment by a
cold `PyFrameObject.f_lineno_generation`. New frame-object allocation initializes
it once; frame escape preserves it. Successful actual and pending-child commits
advance it before closing callbacks; failures do not. Overflow rejects before
warning hooks or owner mutation. The warning revalidation compares this commit
generation, not the public line cache. New controls distinguish a same-PC nested
setter from a getter-only warning hook.

The shared NULL capability also needs an all-uses condition: one physical home
can be both a nullable deletion value and a required STORE_ATTR receiver. The
new window-only read predicate lets NullableHome veto any required explicit or
window read, while preserving the sparse untouched-hole contracts. A constructor
negative covers the scalar receiver/value alias; literal deletion admission is
unchanged. This closes a pre-existing hole in the duplicated constructor role
translation as well as guarding the new debugger consumer.

First NULL increment source is frozen for coordinator native21. A's actual
source peer approved the all-alias NULL helper, shared constructor use, boundary
invariant and redirect-generation lifecycle. Host AST parsing and diff checks
passed. No worker native jobs or regeneration ran. The coordinator will first
run the unchanged folded assignment case and new targeted cases; the active
FOR-anchor repair remains open and is not claimed by this increment.

## First FOR implementation: source review pending

The implementation retains the incoming-owner bitmap from the first cold
analysis and shares one physical-successor helper between it and the anchor
proof. For each FOR candidate, the union of source and target needs must fit
its two physical homes. The successful body edge establishes VALID; entry,
exhaustion and any later pair read/definition/retirement/unknown overlap establish
INVALID. The monotone two-bit reachability join requires only VALID at both sites.
This is availability from the current body phase, not static producer equality.

A separate cut-graph pass requires target dominance from body ingress. A second
pass explores target to first current and rejects cuts, scope exits or backwards
edges before arrival. This first implementation deliberately supports an acyclic
backward body replay; it does not infer preservation through unknown transports,
re-entered headers or nested owner lifetimes. Physical relaxation helpers remain
real CFG nodes. Actual receiver/cursor checks match the existing iterator ABI,
and the FOR item recipient cannot alias the pair.

Only that pair is excluded from detach-all retirement. Pending children still
use the old empty-owner recipe, and a warning hook triggers a full fresh recipe
check. The application transaction remains detach all surplus owners, commit PC
and redirect generation, close C-held owners, then return without frame writes.
No per-dispatch or dynamic owner-generation state was added.

The original ConstSlots loop test is unchanged. New focused cases require exact
result38 when replaying an addition with range/index and ordinary iterator/NULL
cursor, and reject entering before creation or reusing an exhausted loop's
anchor after another loop initializes its pair. Host AST and diff checks pass;
A's actual source review and coordinator native qualification are pending.

The FOR component is now source-frozen. A's actual source peer found no blocker
in the bounded dynamic phase, required-set, slice-preservation or application
proof. The coordinator's original loop test and three new focused methods are
the first native gate; no worker native execution or regeneration was used.
Protected, suspended and multiple-anchor recipes remain open.

The next proposed small extension is an exact real-LINE identity recipe:
current actual observed real PC equals the selected real target, with no child,
suspension, error phase or active source-only cursor. It preserves all scratch
and the caught-exception state, rather than reconstructing any owner. Same
physical predecessor or event.to equality during a source-only callback is not
this phase proof. Such a successful no-flow-change commit still counts redirect
intent, but should not manufacture None bindings in unrelated locals. This is
proposal only and is not part of the frozen FOR increment.
