# Option 5 constructor revision: independent source review

GO for the bounded revision, with the selection-bound correction below. This
reviews `option5-constructor-proposal.md` against the existing clears source;
it does not approve the current measured candidate or claim a timing gain.
No source, build, or runtime was changed for this review.

The complete geometry is A==B, C!=B, nlocalsplus<=B, and B and C both
below the actual frame-slot count and below IR_MAX_SLOTS. Thus neither input
is the discard sentinel. C may be a local; B must be a temporary. Use an
arithmetic type that does not overflow when computing the constructor's
localsplus-name count plus stacksize. A equals B, so it needs no independent
upper-bound test. The actual executing code supplies the local boundary;
mutable function code must not participate.

The proposal's instruction to retain the *existing* specialization predicate
needs correction. `Python/specialize.c:specialize` currently proves only
A==B, C!=B, B>=co_nlocalsplus and B!=IR_NO_DST. It does not prove either
input's actual frame extent or reject C==IR_NO_DST. Add the missing bounds
at this once-per-selection decision. This matters because marshal validates
common constructor data but does not run public `check_words`, even when the
serialized word starts as an ordinary handler and is refined later. There
is no need to widen general constructor admission to resolve this contract.

## Construction and execution ingress

- `Objects/codeobject.c:code_new_with_fbinit` is shared by public code
  construction and `code.replace`; it calls `_PyCode_Validate`, then
  `check_words`, then `_PyCode_New`. The narrow refinement check belongs in
  common `_PyCode_Validate`, preferably in its existing word loop. Checking
  only `check_words` misses marshal.
- `Python/assemble.c` and `Python/marshal.c` each call `_PyCode_Validate`
  before `_PyCode_New`. A constructor-supplied refined handler must prove
  both B/C extents itself; public operand validation is insufficient proof.
- `_PyCode_New` explicitly requires already-valid input. The other direct
  caller is `PyCode_NewEmpty`, whose fixed three `assert0` words contain no
  refinement. Keep that trusted internal contract; do not add a second
  broad admission pass.
- `Python/ir.c:make_exec` copies immutable `co_words` and records the same
  code's `co_nlocalsplus` in the execution prefix. It does not manufacture
  new operands. `_PyCode_GetExec` is the ordinary allocation/install route.
- The executing entry trampoline contains only INTERPRETER_EXIT. The
  static initialization shim's code and executing copy contain only
  EXIT_INIT_CHECK and RETURN_VALUE. Neither bypass can introduce this
  refinement.
- This tree's active frozen-module build creates marshaled frozen headers
  (`Makefile.pre.in`, Programs/_freeze_module). Import uses
  `PyMarshal_ReadObjectFromString`, reaching the common validation above.
  `Tools/build/deepfreeze.py` is a retained generator for the older
  `co_code_adaptive` layout, with no active generation/link rule here;
  it must not be described as a working unchecked refined-word producer.
  Future static executing refinements would need an explicit equivalent
  contract, rather than inheriting a debug assertion as admission.

## Handler installation

`_PyIR_SetHandler` and specialization's FT compare/exchange replace only the
low 16 handler bits. Operands remain tied to the validated word. The new
refinement is selected in `specialize` after the corrected geometry proof.
`fb_record` and `fb_put_row` canonicalize it to ordinary ADD_INT, so a
polymorphic row cannot install the refinement independently.

Instrumentation routes through `_PyCode_HandlerFor`; a canonical restore
can return the original refined handler from immutable `co_words`. This is
safe only because construction validates such original words. It does not
need a second per-dispatch check. Deoptimization returns to an ordinary
handler; respecialization must repeat the checked selection decision.

Replace only this variant's four-condition release fallback with debug
assertions of the complete contract. Preserve the analyzed integer body,
inline guards, fallible boxing/error ordering, and the existing source proof
that the omitted NULL store is immediately superseded without an escape.
Leave the other LOAD_ATTR execution variant's fallback untouched.

## Required focused gates

Negative public-construction/replacement fixtures must reach the new
validator with matching source metadata, including A!=B, B==C, B local,
either input at the discard sentinel, and each input outside the actual
frame. A changed source-map bytes object avoids `code.replace`'s earlier
identity-based changed-code rejection; preserve a valid matching map rather
than bypassing validation. Include common validation through marshal, since
that is the route public `check_words` does not cover.

A valid direct refinement must execute, survive marshal round-trip and
instrumentation enable/disable/restoration, and preserve ordinary inline,
boxing/overflow and error behavior. A narrow selection test must distinguish
a valid temporary overwrite from rejected geometry; constructor rejection
alone does not exercise specialization's added bounds. Root owns execution.
Root must regenerate and independently inspect the actual native handler:
release geometry comparisons/fallback removed, debug assertions retained,
and successful reached-site census repeated. The old four-check candidate's
screen and identity remain separate evidence.
