# Nonneutral POST: native32 source and result review

Role: B authored the POST implementation. This document is the author's source
and result checkpoint, not an independent post-review of that implementation.
H and A provide separate source reviews; root requested H's independent final
post-review before integration.

Author disposition: accept the bounded engineering increment after the narrow
allocation warning fix below and its coordinator gate. This is not full
behavior or performance qualification.

Reviewed the current three-file runtime diff against
3cde040a1afdc433db274ff3771dabff08d7b777: Objects/frameobject.c,
Lib/test/test_code.py and Python/legacy_tracing.c. The coordinator retains the
builds, commands, native identities and failures in E11. B performed no native
execution, generation or integration.

## Source contract

One common committed-scope check validates the positive generated declaration,
decoded physical geometry, selected normal edge and exact physical successor.
The existing neutral-JUMP classification adds its previous zero-input/output
constraints. Actual frame/event/generation, instr_ptr/return_pc and absence of
child/pending/source cursors establish the stopped association.

The new nonneutral view copies the exact site's save/origin rows into one
Analysis-owned block, with checked size arithmetic and a nonzero allocation for
zero logical extents. It applies the selected normal transfer once. Token
creators are rejected before this transfer, so shared parent tables are not
mutated. Shared old-origin cuts apply after transfer even when the same home
contains a replacement VALUE or NULL.

Permission is limited to inherited handled context, positively empty stopped
and target save columns, and a safe real target needing no incoming scratch
owner. It does not invoke a pre-site FOR/caught survivor slice or source-only
OTHER fallback for a newly consumed POST phase. PRE, neutral-JUMP and their
existing strict fallback retain their separate proofs. The separately bound
OTHER row used by those old source-target paths is conservative PRE state,
never a transferred POST row.

Warning preparation rebuilds scope, analysis, bound rows and Plan. Previous
Analysis pointers are not reused. A newer generation can authorize only the
separately proved actual real-LINE landing, with instr_ptr equal to return_pc;
the old edge facts are discarded. Existing detach/install/PC-generation commit/
source-error invalidation/close ordering remains unchanged.

The legacy tracing change wraps its three vectorcall argument reads in explicit
PyValue_Borrow before the existing predicates/accessors. It changes no event
classification or retained owner. The API text checker now accepts that route.
There are no producer, generated-handler, format or hot-dispatch changes.

## Executed evidence

Read ownership32-minimal-v2/{release,debug}.log: seven selected methods pass in
each profile. These include the original monitoring callback, five focused
new methods and the API text checker. The new methods cover cold/warm actual
conditional redirects, required iterator-root and active saved-binding
refusals, warning getter/nested reproof, and close-time newer redirect at a real
LINE landing.

The initial minimal receipt is retained: four negative-fixture subcases had
zero attempts because those two actual conditional producers emit BRANCH_RIGHT
on their true taken paths. ownership32-negative-probe/result.json independently
records src7→9 and src12→14 at INSTRUMENTED_POP_JUMP_IF_TRUE. The corrected
helper parameterizes the event and only those two negatives use RIGHT; all
refusal/state/result/count assertions remain, with explicit taken-successor
checks. The other three cases remain LEFT. No production change followed this
fixture failure.

Read ownership32-observer-core-{release,debug}-focus.log and the matching
focus-checks receipt: all fourteen modules pass in each profile, 455 tests with
one skip. This includes whole test_code and test_monitoring plus the existing
frame/observer/completion/contract/source/error/entry modules. Both builds
complete the coordinator's 117-module gate. The existing test_ir unbound-local
warning remains visible in both logs; it is not suppressed or reclassified.

Host AST and diff-whitespace checks pass. A and H separately reviewed the
actual source and corrected fixtures. The broader full31 diagnostic and the
remaining NULL-only, retained-context, caught-exit and yield capabilities remain
separate evidence/workstreams; these focused passes do not close them.

H's independent post-review identified new signedness warnings from
`Py_MAX(count, 1)` in the copied-row allocation. The admitted correction uses
one size_t `alloc_count = count ? count : 1` for the overflow check and
allocation; logical copy extents are unchanged. Both rebuilds and the cheap
seven-method retry are coordinator-owned and pending at this checkpoint.
