# GH-127321: immediate-entry position and continuation

Proposal only. Scope is N `Lib/test/test_pdb.py`; no runtime or other test
changes. Root is confirming the current38 Pdb census before implementation.

The retained36 debugger receipt reports the same two transcript failures, one
per backend. Expected doctest line4 `a = 1`; actual line3 is the existing
`[1, 2] and pdb_instance.set_trace()` expression. The actual call-line position
is consistent with `Doc/library/pdb.rst`'s since3.13 immediate-entry contract.
That does not by itself prove the stopped PC has a valid position or that
continuing executes the binding correctly.

## Exact test change

Keep the original doctest function body and short-circuit expression unchanged.
Change its expected transcript to line3 and the exact call expression, without
accepting both old and new lines. Clarify that the regression requires a valid
position at immediate debugger entry, not a later instruction event.

Add one companion method to the existing PdbTestCase, with explicit subtests
for `settrace` and `monitoring`, using Pdb's existing backend constructor
argument and in-memory input/output. A small RecordingPdb subclass overrides
interaction only to snapshot `(frame.f_code, frame.f_lasti, frame.f_lineno,
whether a is bound)`, then delegates to the real superclass interaction.
Feed exactly `continue` through the real command loop. This counts actual
interactions rather than tracing instructions or relying on transcript prompt
counts alone.

The companion keeps `[1, 2] and pdb_instance.set_trace()` as its actual entry
expression. Its next statement binds `a = 1`, then records and returns that
value. Assert:

- Exactly one interaction occurs, for the intended function.
- Its recorded line is the call expression's one exact source line; `a` is
  absent at entry, so no next-line execution is mistaken for immediate entry.
- Exactly one `dis.get_instructions` row has offset equal to the recorded
  f_lasti; its public positions have a positive line equal to that exact
  call-expression line. No assumption about byte versus word offsets or a
  fixed opcode name is needed.
- Continue returns1 and records `[1]`, with no second interaction or duplicated
  binding observation. Retain the complete real command-loop output and check
  that no error was printed.

Use a finally block to stop this debugger and restore the prior trace; clear
its last-instance cache only if it still names this test's debugger. The
recorded snapshots retain no live frame. Do not change the global backend or
install synthetic monitoring events.

## Gates and limits

H independently peers this proposal before source edits. Host syntax and diff
checks must pass; an AST comparison must prove the original doctest's defined
function and short-circuit expression remain unchanged apart from expected
output prose. Root then runs the original doctest under both existing backend
suites plus the companion method. The ordinary Pdb basic-command jump case
retains its positive transcript and execution expectations.

This qualifies one public position and continuation behavior. It introduces no
instruction-event count, prompt-finalizer timing or general debugger-transfer
guarantee. Native results remain pending until the coordinator's gate.

## Source checkpoint

Root's current38 census confirmed only the two GH-127321 transcript failures
in Pdb; its ordinary basic-command case passed. H accepted the proposal, then
H and B accepted the actual N test-only diff. The edit preserves the original
doctest function AST and the complete basic-command test AST. The companion
records snapshots rather than retaining frames and restores the previous Pdb
cache entry only while its own instance still owns that cache slot. Host syntax,
preservation assertions and diff checks pass. Source is frozen for root's native
gate; no worker candidate execution occurred.
