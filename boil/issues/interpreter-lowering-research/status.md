# Interpreter lowering and LuaJIT staging

- State: closed
- Owner: coroutine_design_review; coordinator selects follow-up work
- Updated: 2026-10-31, 20:28 UTC
- Budget: 45 minutes of source research from approximately 20:19 UTC; decision by 21:04 UTC
- Depends on: [constant operand prototype](../interpreter-constant-operands/status.md), [shared interface review](../interpreter-shared-costs/reviews/holistic.md)
- Source inspected: documentation checkpoint `32f14069fd2a0b34b67f167621d2de4671369a49`; runtime/compiler source identical to integrated `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Outcome: reported
- Decision: research accepted by coordinator; continue the existing constant operand prototype first; no new runtime experiment authorized here

## Hypothesis

Compiler-selected operand forms can remove interpreter dispatch and slot work
without first introducing whole-function SSA. Determine what the present
pipeline already does and which additional consumer has a concrete, affordable
entry, ownership, observer and encoding proof.

## Experiment

Read the actual compiler, stack CFG, register CFG, assembler, opcode DSL and
frame-entry implementation. Compare official LuaJIT parser, bytecode,
interpreter and trace optimizer sources, keeping their stages distinct.
No target interpreter, build, new profile, timing or runtime-source edit is
part of this investigation.

## Decision rule

Report only source-supported opportunities. Rank small consumers above a new
optimization framework. Reject proposals already covered upstream, proposals
that trust a mutable slot at arbitrary debugger entry, and proposals whose
encoding or observer protocol exceeds the stated scope. A source opportunity
does not establish a speedup.

## Now / next

The [results](results.md) identify existing local optimization and interblock
liveness, distinguish them from full SSA optimization, and rank two concrete
operand-form hypotheses. Small integer arithmetic is already assigned to its
own bounded prototype. Literal integer read subscription is a conditional
follow-up after that shared representation is reviewed. General identity-test
and branch fusion is held because its feedback/target encoding and event
contract are materially more expensive. Nursery source reviews take priority
over further research in this issue.
