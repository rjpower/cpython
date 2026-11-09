# Independent design synchronization review

Reviewed 2026-11-09 UTC against integrated runtime `5bfd1e7e457`.
Scope: `design/register-ir.md`, `design/interpreter.md`, and
`design/call-and-frame-lifetimes.md`. No source changes or native jobs by this
reviewer. No documentation blocker remains.

The documents distinguish register-owner retirement from a runtime operand
stack, and Python language requirements from the selected RC release policy.
They describe all six physical aliases while keeping canonical opcode, feedback,
adaptive-disassembly and handler-domain identities separate. Boolean variants
require generated SAVES_IP successors, decline in debug/free-threaded builds,
and invalidate before instrumentation patches. The unrefined false-BOOL checked
NOP continuation is still described. Retained canonical anchor storage is not
presented as physical compaction.

The exact and guarded-general routes share the existing child runner and return
completion protocol. Numeric destinations survive preparation and native
fallback; only committed Python children receive tagged recipients. The result
stays C-owned through child retirement. Completion clears the tag before its
live POP/input guard, publishes on observer fallback, and directly releases at
CALL PC otherwise. The code-object pin and generator/coroutine exclusions are
documented. Successful f_lineno writes cancel pending tags; this does not promise
general cross-parent frame jumps.

One wording issue was repaired during review: active observation was too broad.
The final text correctly says a live POP line/opcode action causes fallback;
profile/start/return observation alone need not wrap POP. Observation enabled
inside the result finalizer after the completion guard begins at the next real
instruction, without a retroactive POP. The raw f_lasti coordinate policy is
explicit and does not claim language-mandated destructor timing. Mermaid flow
was checked against source, including tag clearing and observer-error unwind;
no rendered-diagram validation is claimed.

The census helper uses existing canonical execution-count keys, subtracts the
zero-loop capture, and checks 2N linearity and matching kernel/runtime identity.
Its physical get_ir inventory is selection evidence, not per-site hit evidence.
The coordinator's census-v2 records 142 entries per outer iteration, zero POP
entries and 20 frames/inlined calls; all reported deltas are linear. The golden
151-entry comparison supports nine eliminated general-call POP entries, not a
new NOP dispatch reduction or elapsed-time claim. Static retained-word counts
include cold code and must not be confused with executed entries.

Qualification at review: repaired combined focus-v2 release/debug runs each
passed 40 tests; CDB lint covered 306 units with zero failing findings. Full
suites and final performance admission remain coordinator-owned and pending.
