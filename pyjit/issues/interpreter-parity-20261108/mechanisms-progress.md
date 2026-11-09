# Structural mechanism proposal progress

2026-11-08. Design-only assignment against runtime `0953757f4e7`.
Budget: 60 minutes; first progress packet within 30 minutes. Canonical project
instructions, the CPython AI policy, GOAL/PROCESS and the four experiment
design/review pages have been read. No runtime edits, builds, native workloads
or commits are authorized by this assignment or have been performed.

First source packet:

- The warmed `isEnabledFor` path is shape read, boolean branch, shape read,
  dictionary subscription, return. The method loader and shape reader already
  avoid refresh on their proven leaf paths. A broad claim that every such
  access performs refresh would be false.
- The dictionary handler still passes its successful result through generic
  `value_reader_complete`, then reloads the word and slots. Its provider calls
  `PyDict_GetValue`; an inline-int query alone does not exclude equality
  callbacks from colliding stored keys.
- An index cache cannot bypass those callbacks merely because keys/version
  and the target entry still match. A first-probe identity hit, or an audited
  no-callback probe-prefix certificate, is required.
- Call-return fusion can retain the existing POP word as an explicit observer
  boundary and skip its ordinary dispatch only after child-frame clearing and
  current instrumentation checks. Frame clearing itself can enable tracing.
- A later direct recount reconciles the coordinator's nine dynamic POPs:
  the tenth static POP is before the loop, after `truncate_stream`. The tenth
  hot call already has IR_NO_DST and no POP. Fusion coverage is nine, not ten.

Completed the comparative proposal with concrete phase, return-boundary and
dictionary-feedback designs, owner ledgers, generic fallbacks, migration costs
and cheap rejection criteria. Source references and the nine-POP correction
were checked against the retained runtime/capture; pinned primary V8 sources
were inspected. Only the two mechanism documentation files are owned by this
assignment. No runtime tests are claimed.

Independent peer correction, round 1: Terra identified an aliasing hazard in
the generic multi-owner retirement wording. The revised M1 checks MayDealloc
immediately before each ordered close, after earlier closes; on the first
possible callback it routes this and all remaining owners to escaping cleanup.
Precomputing all checks is explicitly forbidden, and the aliased-final-owner
case is part of the proposed correctness coverage. This is a design correction,
not an implemented or executed runtime result.

Coordinator follow-up: added M1b, a distinct temporary-producer phase/class
diagnostic, and M2b, explicit NOP line-boundary execution on selected branch
edges. These cover the measured ten message producers, ten None producers and
ten NOPs without presuming that PC stores can simply disappear or adding a
next-NOP branch to every dispatch. POP and NOP prototype benefits remain
separate until measured.
