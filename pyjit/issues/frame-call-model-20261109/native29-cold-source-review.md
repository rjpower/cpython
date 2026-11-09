# Native29 cold source review

The frozen native29 source gives the observer one generated, edge-specific
commit fact. Ten instrumented terminal branch providers and direct SEND have
nonempty facts; other ordinary handlers retain no observer fact. The generated FOR
and OR_POP cases publish their result homes before invoking a monitoring
callback and close deferred C owners afterward, including the callback-error
path. The constructor uses the fact for source-handler POST inspection and
checks both compute PRE and committed POST states for covered ordinary
exception handlers. Its selected-home proof keeps a retired old owner's
obligation even when a new value or explicit NULL lands in the same home.

Saved-local restoration separately tracks an actual NULL binding through the
existing sparse control-flow graph. Only LOAD_FAST_CHECK and DELETE_FAST are
accepted as checked unbound-local accesses. Every physical IN or window alias
requires a bound value, even if that operand's C ABI accepts NULL. This matters
for STORE_ATTR/GLOBAL deletion and CALL's optional receiver. A guaranteed
semantic result can discharge the obligation on a committed edge; generic
STORE_FAST or a physical output declaration cannot. Source review found no
remaining blocker in this bounded model. It does not certify unmarked
terminal providers or a general bound-value producer.

The first release build completed 117 modules. Its focused run found three
failed test modules: eight positive instrumented-FOR constructor variants,
two synthetic completion-handler hole positives, and a metadata snapshot.
`_PyIR_Outcome` had used `_PyIR_BaseOpcode`, which leaves
INSTRUMENTED_FOR_ITER unchanged. The reviewed fix uses the generated
`_PyIR_HandlerBase` mapping only in that cold semantic decoder. The two hole
fixtures actually read B+2, the not-yet-produced FOR item now tracked by the
ordinary PRE proof. Their reviewed correction moves required reads outside
B..B+2 and keeps the retired-read negative variants. The generated metadata
row contains the new observer field; the literal test expectation was stale.

The first-run log is
`build/pyjit-link-wave1/frame-call-model-20261109/ownership29-small-release-focus.log`
in the python-boil-the-ocean checkout. The corrected native29-v2 source and
tests were frozen for the coordinator's rebuild and focused/corpus gates. The
first v2 retry passed the corrected metadata and hole tests, but its eight
instrumented-FOR positives still used an older `codeobject.o`: that object was
older than the changed outcomes header because `PYTHON_HEADERS` omitted it.
The Makefile dependency repair adds the four existing internal IR headers it
had omitted: exception, observer, outcomes and source. The v2 receipt is not
evidence for the corrected image. This review did not run a native job or
establish performance qualification.

The coordinator's dependency-correct release rebuild subsequently completed
117 modules. The targeted retry passed all three formerly failed methods,
including the instrumented-FOR variants. The compiler corpus accepted 2,156
valid files and reported the same 11 intentionally invalid SyntaxErrors, with
zero new constructor rejections. A focused ordinary-PRE versus source-POST
fresh-item witness and the 21 diagnostic-only tracing expectations have been
added as test changes. The release minimal set then passed 22 selected tests.
The debug six-module gate passed 363 tests with one skip; both builds checked
117 modules. Debug owner-exit probes created 660 objects across six cases and
found none alive after collection. FOR callback probes likewise found zero
survivors from three produced items on both error and non-error paths. A
separate eight-module observer integration gate passed 64 tests in each build.

## Generated-code scope and remaining broad screen

An independent hunk-to-handler audit of the current generated cases found
changes in exactly ten `INSTRUMENTED_*` terminal providers. All 262 ordinary
handlers have no changed generated body. The generated cases hash is
`6468665bd39a300a1f4e8d5748c352225bd7f991e2934e34e81685e41b4c5dba`,
matching `ownership29-generated-review.json`. The generated metadata carries
the same ten observer facts plus direct SEND's completion edge. This source
identity and the targeted native checks do not establish a wide-suite pass.

The current `ownership29-wide-release-focus.log` ran 999 tests with 14 skips
and still failed four modules. The prior `ownership28-wide-v2` screen had the
same broad behavioral clusters, but it predates this packet. Each remaining
case needs an encoded transfer witness before implementation is chosen.

| Rank | Observed cluster | Structural capability to check next |
| --- | --- | --- |
| 1 | Monitoring: 107 tests, one error; peepholer: 166 tests, three errors and one skip. Expected line jumps raise `jump requires an incoming temporary register owner`. | Verify the callback's actual PRE/POST phase, target input homes and available owner lineage. Admit a transfer only when its inputs have an owner or a proved producer. |
| 2 | `sys_settrace`: 480 tests, 38 errors and one skip. Jumps across comprehension, `with`, async and exception regions raise the same owner refusal. | Model destination needs and scope-exit effects from final IR and encoded EH/source edges, including per-local restoration and owner retirement, then compare exact accepted/refused cases. |
| 3 | `pdb`: 246 tests, two failures, two errors and 12 skips. One case stops on the wrong initial line; another reaches `never_executed` during stepping. | Check executable source landing and step/continue transfer against the same final IR locations, before changing display transcripts. |

Twenty-one other tracing failures in the old screen compared only an obsolete
stack-era error phrase. The approved test port kept their `ValueError` and
expected side effects; the current wide run has zero `sys_settrace` assertion
failures. It does not account for the 38 behavioral errors. The old
`test_stack_too_deep_is_refused` failure is absent from the current test source,
so that old result is not evidence of a current pass or failure.

These are bounded correctness gates, not broad parity or performance
qualification. The release wide screen remains failing. The executed receipts
are `ownership29-final-{minimal-release,debug-focus,debug-owner-probes}` and
`ownership29-observers-{release,debug}-focus.log` alongside the wide log under
`build/pyjit-link-wave1/frame-call-model-20261109/`.
