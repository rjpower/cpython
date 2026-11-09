# Semantic completion: independent source review

2026-11-09. Read-only review of A's runtime/generator changes against
`6ef45e391aa` in the runtime integration checkout. B's compiler/observer outcome
integration was still in progress. No runtime edits, native builds or native
tests by this reviewer. This is source admission for the bounded chunk, not
merged runtime or performance admission.

## Assessment

No concrete ownership or completion defect was found in A's reviewed chunk.
Proceed to regeneration and the focused merged native ladder once B freezes.
The distinction between PRE inputs and POST committed state is explicit and
the generator terminates these paths without publishing the other outcome's
cached outputs. FOR_ITER exhaustion does not store into the last loop local;
SEND completion has its own destination, distinct from the yield recipient.

`_PyEval_CompleteStep` retains the completion value in C through PRE. An error
or redirect returns before reusing cached window inputs. The redirect path
rereads the published PC after disposing the held result. A nonredirecting PRE
continues with the receiver anchored in its window. Commit detaches receiver,
cursor, sent value and CLEANUP_THROW's exception before publishing a possibly
overlapping SEND destination. POST observes committed ownership while retaining
the semantic operation's PC for exception attribution. Successful completion
publishes the continuation before detached closes, then rereads the PC so a
later successful callback redirect is not overwritten.

PRE/POST status values must continue to describe these distinct owner states
in B's compiler/observer recipes. Error dispatch uses the returned next pointer
and the normal error label; it must not reconstruct retired completion inputs
or apply body-edge definitions to exhaustion. B's source-only exception cursor
remains the separate source exception recipe.

Prepared VM resume takes the sent owner from the caller cell, clears that
cell, sets previous/current frame and generator exception links, and performs
the recursion entry before disposing or publishing the input in the resumed
frame. Thus last-owner disposal cannot observe both the caller cell and the
child claiming the owner. The recursion failure branch closes the held input
and takes the normal unwind path once; the successful branch does not enter
start_frame and decrement recursion a second time. SEND guards decline a
non-None first send to the generic error path before marking a created
generator executing.

RETURN is now terminal. Its taken canonical input survives child retirement;
the generator return follows the same completion helper as native exhaustion,
while ordinary CALL publication remains inline. The complete checked token
contract still recognizes the two canonical return chains and falls back to
publication when changed. The init cleanup shim has two real words, with the
explicit continuation at the first real check. Yield/creation resume recipients
and remote awaited-coroutine lookup understand IR_NO_DST.

## Validation required

The six new runtime cases are a useful first layer. Add or retain focused
cases for these ownership boundaries before broader qualification:

- STOP_ITERATION PRE callback error and redirect for generator/coroutine
  completion; verify the intact input window and one retirement of the held
  result. Include callback enable/disable of tools.
- BRANCH/source LINE POST callback error and redirect, including a completion
  result stored into a local that replaces a mortal owner. Verify traceback
  attribution and no reconstruction of detached inputs.
- Last-owner ignored resume input with a finalizer inspecting the current
  frame, caller window, generator executing state and nested exception state;
  repeat on recursion failure and through SEND specialization/fallback.
- Native list/tuple/range/value-slot iterator completion, Python generator
  return, native SEND and async-generator SEND, CLEANUP_THROW, interpreter entry,
  and specialized/deoptimized variants under tracing and monitoring. Check
  exactly-once input/result disposal and the last loop local.
- Large-distance relaxed loop/send targets and pending-child debugger redirects
  through success and error. B's positive ingress/outcome maps must target the
  first real continuation after cleanup removal.

Host full-source analysis and IR generation completed in memory (769,858
characters). Both checked canonical return chains were recognized; inspected
SEND_GEN and FOR_ITER_GEN each contain one recursion entry, and CLEANUP_THROW
contains one semantic completion call. No native execution follows from this
check. Remaining standalone/shared POP and protected/suspended debugger work
were already disclosed and are not newly discovered defects in this chunk.

The remote awaited-coroutine sentinel exclusion does not establish its upper
slot bound. Root/B identified that missing guard during this review. The local
`_PyGen_YieldFromRef` now asserts the upper bound, but the remote reader needs
a target-described value-home count before calculating an address. See the
versioned descriptor recommendation in `frame-arena-review.md`; matching local
layout guesses are insufficient for foreign targets with the same Py_Version.

## Compile-repair and remote-span addendum

Reviewed the actual post-failure source repair read-only. The failed C build is
retained as a compile failure; the first generator output was not buildable.
`complete_step` now names its status/continuation object `ir_step_result`, so
`_PyValue_Publish(&completed)` still refers to the outer DSL `PyValue` rather
than a newly shadowing result struct. Return completion uses the same distinct
name; prepared resume uses `ir_resumed_frame` and `ir_resume_input`. The added
real-source regression checks the affected producer families and the held-value
argument. This fixes the observed collision; it does not claim a general
fresh-name allocator for arbitrary future DSL identifiers.

Variable declarations now stop at a proven always-exiting uop or existing
terminal call marker, matching the emitter's unreachable-tail stop. Conditional
completion/resume markers inside branches do not turn their enclosing uop into
an unconditional exit: analyzer depth tracking keeps those fallthrough paths.
Thus specialized SEND_GEN/FOR_ITER_GEN no longer declare the unreachable old
_PUSH_FRAME input, while generic SEND retains its live fallback declarations.
No additional ownership transfer or recursion entry was added by the rename.
No blocker found in this narrow repair; root's regenerated native build remains
the next evidence.

The remote awaited-receiver check now covers the complete `PyValue` read:
it first bounds `slots_addr + sizeof(PyValue)`, then compares the bounded
receiver delta with the remaining address range before addition/read. The
actual slot-count bound already limits multiplication. This resolves the exact
span omission reported in review. Prefix-before-body validation and exported
target chunk/code fields remain coherent. Source-only remote cursor support is
a separate open consumer task. No runtime edits, builds or native tests were
performed for this addendum.


### Bootstrap ownership-proof repair, awaiting native7

The failed one-floor assumption is superseded. Diagnostic6 proves that the
observed genexpr handler calls STOPITERATION_ERROR with the exception and then
RERAISE1; its iterator/cursor prefix is implicit owner inventory, not semantic
reads. Objects/codeobject.c:948 now checks final encoded handler paths for
actual reads before definitions. Both source-edge handlers and ordinary
exception recipes call that checker. Exceptional edges invalidate homes
cleared by unwind before defining exception/lasti. Zero-mask paths remain
reachable because later unwind can invalidate a newly defined home. Initial
publication into a retired SEND home does not restore its old receiver.
Explicit POP reads receive no exemption.

CLEANUP_THROW now has A/B/CD result/window/absolute continuation, so successful
completion bypasses the old shared-jump layout rather than executing a word
with hidden mixed ownership phases. Python/assemble.c matches the explicit
frontend target, and instrumentation observes its TAKEN source edge without
inventing BRANCH monitoring. Temporary full CFG dumps are gone.

Host AST and whitespace checks pass. Native7 regeneration passed under root;
build and focused execution remain pending. No new performance attribution or
qualification follows from these source changes.


### Effectful branch taxonomy and constructor proof closure

Native8 retained an apparent END_SEND ingress because CLEANUP_THROW had been
mistakenly added to the assembler-only opcode set. The correction uses a
separate IS_NO_FALLTHROUGH_OPCODE predicate in basic-block fallthrough,
frontend stack propagation, register CFG flow and source-edge emission.
IS_UNCONDITIONAL_JUMP_OPCODE continues to describe pure transfers used by
threading/removal; effectful THROW is excluded from those optimizations. A
new two-delegation regression requires two surviving semantic THROW words,
valid explicit targets and both completed results.

A's independent review identified three constructor-proof assumptions now
made explicit. Completion-containing programs validate named-local/CELL
fields below co_nlocalsplus; a LOAD_FAST cannot disguise a scratch-home read.
Their exception table is fully validated before proof: positive, sorted,
nonoverlapping ranges, bounded handler homes/targets and entry markers only
at entry starts. This aligns cold linear lookup with runtime binary lookup.
Finally, reachable source-only callback error edges join the same worklist
with their post-operation ownership facts and their own exception/lasti
installation; they cannot bypass the ordinary handler proof. Conservative
from/edge matching can reject an extra unreachable source route, but cannot
admit a missing owner. No new source schema or hot state was added.

Negative tests cover malformed table markers/order/overlap, scratch disguised
as a named local, and a noncompletion source callback routing from an accepted
PEP 479 wrapper into a forged retired-owner consumer. Private fixture table
entries now include the canonical start marker so failures reach their intended
ownership checks. Native qualification of this repair remains pending.


### Native11 nullable-self refinement

Native11 identified a concrete conservative rejection in
FileFinder._find_children: handler entry71 reached CALL pc14 with SEND base7,
count3, and only B+1 unavailable. That home is CALL's optional NULL self cell.
The cold proof now tracks U (original owner unavailable) and N (that unavailable
home may still be non-NULL), four bits each. Entry starts allU/allN and applies
the actual handler unwind: cleared homes become U/definitelyNULL; exception and
lasti writes define fresh homes. A preserved receiver prefix stays U/N even
though POST physically retired it; PRE may still require the old owner. A
published result does not reconstruct that owner.

Joins OR both facts, restricting N to U. Only exact CALL/CALL_KW B+1 may read a
U home with N clear. No other operand or call family receives this exemption.
Fresh definitions end the old-owner obligation; only fixed semantic retirement
outcomes and real unwind establish NULL. Generic reads never imply retirement.
The worklist remains cold and uses uint16_t facts, with no runtime field.

Tests preserve a retired receiver below the exception floor and forge its use
as CALL-self, with both discarded and overlapping published SEND recipients;
both must fail with may_nonnull diagnostics. A positive yield-from handler
returns through a normal factory call. Host AST/diff checks pass; next merged
native result remains pending.
