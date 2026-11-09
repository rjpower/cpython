# Safe transition refusals: test-only packet

Proposal before edits. Scope is N `Lib/test/test_sys_settrace.py` only. The
eleven TRANSITIONS in `probe-trace-jump-capabilities36.py` keep their original
function bodies, requested offsets and callback events. Eleven ordinary cases
and the two unresolved except-arm positives keep their existing expectations.
No runtime policy, per-method setter rule, skip or broad ValueError suppression
is proposed.

## Reusable callback contract

Add an optional refusal callback to JumpTracer and optional refusal regex to
the existing jump decorators/runners. Only the eleven explicit decorators opt
in. The original request predicate/ancestor lookup stays shared. On a refusal
request, mark the attempt done before invoking the checker so the continuing
original program cannot retry the crossing. Assert the runner actually saw
that one attempt; missing source events must fail.

The checker snapshots actual frame PC and line, code identity, every visible
local's identity, output contents and the inherited handled exception. It then
handles a distinct callback-owned LookupError and attempts the setter under
assertRaisesRegex for the exact expected ValueError. Check the callback-owned
exception remains installed after the setter's exception handler exits, then
check the inherited exception after leaving the LookupError handler. Finally
assert identical PC/line, local keys/value identities and output contents. An
unexpected success or another exception fails inside the callback. Continue
the original program and require its complete normal output, including all
normal protocol exits. No unrelated GC/finalizer exclusion is claimed.

Ten cases use the anchored incoming-owner error already observed in36. The
yield case uses the anchored suspended/protected-recipe error, not a generic
line-event error. Warnings remain errors for these cases. No diagnostic
substring loosening is needed.

## The eleven decorator changes

| Cases | Full normal output after the refused request |
| --- | --- |
| sync/async inner with inside for | [1, 3, 4, -3, 5, 6] |
| sync/async inner with inside outer with | [1, 2, 3, 4, -3, 5, -2, 6] |
| sync/async between sequential with bodies | [1, 2, 3, -2, 4, 5, -4] |
| except to finally | [4, 5, 7, 8] |
| bare/qualified caught exit | [1, 3, 6, 7] |
| complex loops/finally | [1, 3, 5, 8, 8, 10, 5, 8, 8, 10, 11, 12] |
| yield return callback | [2, 5] |

These are explicit supported refusal tests, not old successful jumps silently
accepting either outcome. The six manager programs' logs prove each lexical
body/exit completes normally. They do not expose implicit manager identity, so
add a dedicated manager control with explicit named managers, distinct IDs and
enter/exit counters. The asynchronous variant must await an observable
checkpoint in its enter/exit methods and prove each method finishes once.

Add a caught-context control with the target's explicit exception local and
the callback's distinct exception. After refusal the target records the same
handled exception, and its finally runs once. Add a yield control holding an
explicit generator and mortal payload: refusal leaves gi_frame PC/line and
payload identity intact, next yields that payload, send resumes with its actual
value, and finally executes once. The shared checker supplies immediate
unchanged-state assertions; these controls supply protocol-owner evidence.

## Validation and limits

First independently peer this plan. After admission, host AST comparison must
prove all eleven original bodies are byte-for-byte equivalent as AST and the
ordinary/unresolved decorator/body ASTs are unchanged. Run host syntax and
diff checks; root alone runs the eleven methods plus dedicated controls and
the unchanged ordinary/unresolved census. A full pass is not inferred from
source checks. Native errors beyond these explicit crossings remain failures.

The permission is proof-based: a transfer requiring unimplemented protocol or
handled-context actions may refuse without corrupting the stopped state. It
does not establish that every lexical with/except/finally crossing is forbidden,
nor authorize restrictions on compatible same-protocol statement jumps.

## Source checkpoint

H accepted the design and actual N source for the coordinator's native gate.
Only `Lib/test/test_sys_settrace.py` changed. Host AST comparison confirms the
eleven original bodies are unchanged and all thirteen ordinary/unresolved
methods, including decorators, are unchanged. Syntax and diff checks pass.
The first AST comparison command used the runtime checkout as its working
directory with a canonical-document relative path; it failed before assertions.
The corrected absolute-path command passed all preservation checks. No worker
candidate execution occurred. Native results are pending.
