# Context source26 handoff

B source and tests are frozen. A actual-source peer approved the bounded context
implementation and sparse-graph refinement. Root owns merged native validation.

Changed B files are `Include/internal/pycore_ir_exception.h`,
`Objects/codeobject.c`, `Objects/frameobject.c`, and `Lib/test/test_code.py` in the
runtime worktree. A's private debugger-entry event case remains intact.

First targeted methods are:

- `test_settrace_inherited_protected_redirects`
- `test_settrace_same_caught_redirect_preserves_exception_owners`
- `test_settrace_cannot_manufacture_or_exit_caught_context`

The original NULL, FOR and real-LINE identity cases remain unchanged. The new
engine adds no ordinary dispatch work or serialized context schema. No target
handled-exception link is changed. Protected FOR survival, protected source-only
destinations, caught exits, comprehension restoration and suspension are still
open recipes; this is a bounded increment, not full debugger qualification.
