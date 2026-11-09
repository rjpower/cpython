# Owner retirement at semantic operations

Proposal only, after source20 (`3e5cbdb11236`). No runtime change or performance
claim. The coordinator's corpus contains 112,869 retained POP_TOP and 340
POP_ITER words in 21,363 code objects; these are static counts, not logging
execution counts. Normal loop/send completion, ignored fresh producer results,
and unused resume inputs already have separate lowering proofs.

## Decision

Start by investigating consumption at an owner's last actual semantic read.
Terminal RETURN_VALUE retirement needs a separate explicit owner contract; the
original assumption that ordinary frame exit already closes scratch was rejected. Do not introduce an owned bank or change
all branch encodings before inspecting the surviving physical words.

The bounded compile-only `owner-retirement-inventory.py` is ready for coordinator
execution after timing. Its three-word neighborhoods distinguish release inputs
from their producers and successors, including the large real pattern matcher in
`Lib/_pyrepl/utils.py`. Concrete fusion counts remain pending that receipt.

## Actual owner model

Named locals and scratch homes are distinct. Canonical temporaries in
`pycore_regcfg.h` are shared CFG stack positions, not Python locals: conversion
canonicalizes them at region boundaries and exception entries. Both canonical
and fresh temporaries become physical scratch homes. A physical temporary input
is normally consumed; a named local input is borrowed. Copies and pass-through
windows can keep an independent owner alive across an operation.

The current fresh-result sink pass deliberately rejects canonical/shared values,
checks explicit and implicit reads, and requires a single output and a
DISCARDS-capable producer (`regcfg_passes.c`). Thus the corpus count is not a
count of missed ordinary discarded calls. Enlarging this pass to shared
producers without edge liveness would be wrong.

A semantic operation should specify the owners it consumes, the owners it
preserves, and its conditional definitions. The shared `_PyIR_ReadsHome`,
`_PyIR_DefinesHome`, nullable roles and outcome helpers are the existing cold
contract. They must remain authoritative for constructor and debugger proofs;
`fmt.reads/writes` window unions cannot stand in for ownership.

## Terminal retirement: rejected first implementation assumption

Independent B and coordinator source review found a blocking error in the first
version of this proposal. Ordinary thread-frame retirement does **not** close
all scratch homes. RETURN_VALUE asserts temporaries are clear; frame.c again
asserts that invariant and closes only nlocalsplus. Generator-owned frame clear
has a different full-frame obligation. Removing adjacent scratch releases and
relying on existing ordinary frame clearing would leak their owners in release
builds and violate the debug/reuse invariant.

Consequently, terminal release-chain elimination is not admitted. It requires
an explicit semantic return owner set/range, or a changed full-scratch retirement
policy whose scan and reentry costs are measured. Either choice must preserve
the C-owned return input across cleanup and install valid source/EH owner maps.
There is no justified zero-cost/no-operand claim. An extra cleanup-only return
alias would not meet the architectural requirement.

The exact physical inventory is now available from the coordinator; selecting
an implementation is deferred until that witness and the encoding/cost choices
have independent review. The current invariant remains: scratch owners must be
NULL before ordinary thread-frame pop and arena reuse.

## Next increment: last-read consumption

Use edge liveness to remove a COPY plus a later release when the semantic reader
can consume the original scratch owner directly. The proof includes successor
and handler reads, not merely reads in the current block. On a conditional
operation, consuming an input before choosing the edge is valid only when both
outcomes no longer need that owner. A one-edge death requires an explicit
outcome contract; unconditional consumption would break fallback and matching.

Useful source candidates are the optimized-builtin guard's original callable,
pattern subject prefixes, and decisive any/all branches. Codegen's successful
builtin guard drops a callable also used by its fallback; pattern fail_pop chains
carry different subject prefixes. Neither is a unique-producer sink. For each
candidate, first show actual allocated inputs and alias owners; then select
consumption only where that exact edge kills the last owner. Preserve containing
iterator windows on continue and outer anchors on nested exits.

This uses existing semantic operations and normal input retirement where
possible. It needs no runtime liveness lookup. If an operation's existing inputs
do not name the dying owner, operand encoding is a separate design decision;
adding an otherwise unused input is not automatically justified.

## Completing the model

| Category | Required semantic home transition |
| --- | --- |
| Loop break / decisive any/all exit | Real transfer retires exactly the loop windows being left; continue and outer loops preserve theirs. |
| Match failure / guard success | Actual decision edge consumes dead subject or callable owners; alternate edge retains required aliases. |
| Return / finally override | Return owns its held result; override replaces the pending result only after the finally body's real state changes. |
| Exception matching / except-star | Consuming match and exception-state operations retire dead payloads without losing active exception state or restorable locals. |
| Comprehension failure | Partial result retires while saved enclosing locals remain available to the existing restore-and-reraise path. |
| Context exit | Exit call keeps its callable/arguments valid through callbacks; ignored result already uses NO_DST; loop/exception anchors follow the actual continuation. |

For genuine edge-only owners not expressible by existing operands, compare two
encodings with measured dynamic prevalence: a bounded owner-range operand in a
real transfer, or a nonexecutable operand extension owned by that transfer.
A descriptor must name exact ranges and successor ownership, be constructor
validated, and have no independent PC, INSTRUCTION event or dispatch. A transfer
executes its bounded retirement primitive directly; it must not interpret a
per-dispatch cleanup program. Branch target, feedback and condition already
occupy the present 64-bit format, so claiming this is a free field would be
incorrect. Changing every branch to an indirect target descriptor would tax
ordinary branches and is not admitted by this proposal. Likewise, a second
handler named only to perform cleanup would reproduce the rejected dispatch.

The representation choice needs the physical inventory and a dynamic count of
candidate transfers. Account explicitly for its decode/scan cost on ordinary RETURN_VALUE; current
thread-frame clearing relies on scratch already being NULL.

## Reentry, errors and memory

Before closing a detached owner, publish the semantic operation's committed
state and real continuation. C-owned result values remain outside the retired
window until publication/disposal completes. Last-owner finalizers may reenter;
homes must already be NULL, frame links and arena reservations must remain valid,
and successful callback redirects must not be overwritten with cached targets.
No exact historical destructor PC is required.

PRE callback errors use the input state. POST errors use the completed owner
state. Existing completion construction proved that an EH handler can tolerate
NULL implicit unwind anchors only when it does not semantically read them;
restored locals, PEP479 payloads and required call operands cannot be waived.
New edge retirement must rerun that same encoded proof, including reachable
source-only callback errors. Source metadata preserves required LINE events;
removed release words do not retain INSTRUCTION events.

Frame-owned retention beyond terminal retirement is not authorized. Leaving
owners in normal scratch would violate existing producer NULL assumptions and
argument-window overlap. A separate retained range would also need GC traversal,
copy/escape, overwrite, unwind and cycle proofs. A bounded number of homes does
not bound retained bytes or cycles in a long-running loop. These obligations
make general retention a larger alternative, not a shortcut to deleting words.

## Review and gates

1. Review the coordinator's bounded physical inventory, then select actual
   consuming-reader shapes and separately cost any terminal-retirement change.
2. B reviews allocated liveness, implicit reads, source/EH maps and constructor
   outcomes before any source change.
3. Add positive and negative compiler checks: aliased return input, named local,
   implicit handler read, fallback edge, nested loop and preserved saved-local
   cases must prevent invalid elimination.
4. Runtime gates cover return through finally/with, any/all fast and fallback,
   nested break/continue, pattern alternatives/guards, except-star and failed
   comprehensions. Include tracing toggles, source callback exceptions, far
   targets, generator escape and eventual exactly-once disposal with reentry.
5. Recount retained words by category, then measure dynamic dispatches and broad
   workloads. Static corpus reductions alone do not establish a speedup or the
   final no-standalone-release checkpoint.
