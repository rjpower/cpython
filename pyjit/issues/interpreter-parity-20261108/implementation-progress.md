# Implementation progress

2026-11-08: Read repository guidance and CPython AI policy, the canonical project
guides and active experiment plan/reviews. Inspected runtime base
`0953757f4e7`, refresh/error flow, canonical value construction and both
code-pointer frame initializers. Wrote `implementation-proposal.md` for P1 and
P2. No runtime files changed, builds, native tests or workloads run. Awaiting
design review before P1; coordinator owns builds and integration.

P1 approved by coordinator after peer source review. Added only the early
success return in W `Python/ceval.c`; the failure branch is unchanged. Reviewed
`git diff` and `git diff --check` passed. No build, native tests, workload or
commit. Suggested coordinator checks: `test_call`, `test_monitoring`,
`test_sys_settrace`, `test_frame` and `test_capi.test_mem` in release and
debug; memory-hook tests are general allocation-failure coverage, not proof
that refresh's invalid-frame branch was injected. Awaiting P1 validation and
P2 authorization.

Coordinator hold: broaden architecture alternatives and cost coverage before
selecting priorities. Preserve the uncommitted P1 diff in W; no integration,
build, test, commit or further runtime edits, including P2 and P3.

User authorized implementation; coordinator approved corrected P2. W now has
P1 plus the P2 restricted `_PyValue_PointerBorrow` shared with generic borrow,
and typed `_PyFrame_CodeHome` used by both normal and trampoline frame
initializers. Added a direct internal C test of a mortal code's acquire/release
and the static immortal `_Py_InitCleanup` tag, plus Python active-code
replacement, suspended-generator, class-init and context/exception exercises.
Reviewed diff; `git diff --check` and host AST parsing pass. No runtime build,
native test, workload or commit. Ready for independent actual-diff review and
coordinator build; the trampoline specialization should be confirmed by the
coordinator, not inferred merely from warming the class call.
