# A50 component source review, first pass

Reviewed the working A50 source delta against runtime48 `5559f60093e`,
`A50-implementation.md` and the grouped Astra contract. Runtime changes are in
`BASE/worktrees/execution-boundaries-20261108`; this review edits documentation
only. The reviewed tree is not yet a committed or qualified candidate.

The cache and ownership changes are sound in the inspected source. All three
method variants read the separate guarded canonical word only after their
existing type/layout/absence guards. They acquire the descriptor owner before
TAKE transfers a temporary receiver or duplicates a surviving local receiver.
No receiver retirement or callback intervenes. A canonical method descriptor
is not assumed to be a Python function. Nomination, the historical-row payload
discriminator and raw/guarded storage agree; generic reset clears the entire
attribute payload, and `fb_put_row` copies the tagged payload without conversion.
No feedback-size change is introduced.

The positive `IR_CANONICAL_OUTPUTS` declaration is independent of result
retirement, checks both output roots and rejects a changed/unknown output
transformation. The output facts reach the first destination and the second
temporary, including repeated publication. New canonical stores retain debug
home checks and the ordinary ownership transfers. This removes representation
work without dropping reference owners.

**Must fix before source approval:** the first validator version's prefix
check restricts statements containing literal `=`, but not all assignment
operators. A borrowed pointer alias such as `tp` can mutate memory with
`tp->tp_version_tag += 1` without mentioning the protected `owner` input or
using ++/--. Reject all assignment operators except the sole initializer in
an audited guard-local declaration; reject nested initializer assignments too.
Add false-declaration tests for borrowed pointer mutations and verify changed
source AST, prefix reassignment, helper shadowing and preprocessing rejection.
This finding was sent to the implementation worker before this report.

Generator tests already cover direct/renamed pair outputs, the three real
handlers, invalid source roots, repeated assignment, output/address escape,
helper shadowing and preprocessing. Runtime tests and final host results are
still pending at this first pass. The runtime coverage must exercise actual
warmed variants, temporary/local owners, mutation/shadowing/materialized dicts,
historical rows and reentry; passing generic fallback alone is insufficient.

No native build was run by this reviewer. Coordinator release compilation,
generated/native inspection showing removed numeric recanonicalization, focused
release/debug execution and balanced timing remain required before admission.
C's appended consumer also needs an explicit checked ownership-flow contract;
the intentionally terminal A declaration does not yet certify that suffix.

## First-pass follow-up

The worker fixed the prefix assignment finding by admitting only a single
EQUALS initializer and rejecting other assignment tokens. Added negative cases
cover pointer compound/direct writes, input mutation/address escape and nested
initializer assignment. An independent in-memory synthetic generator run of
the compound-pointer-write fixture now raises `SyntaxError` at the canonical
prefix check. The finding is resolved in the inspected working tree.

The new runtime tests explicitly require each warmed method handler and cover
replacement/deletion, instance shadowing, captured function lifetime during
argument reentry, temporary receiver lifetime and polymorphic method snapshots.
Their source is sound; execution evidence is still pending. Subject to the
remaining coordinator generation/release/debug/native/test gates, the reviewed
A implementation has no remaining source blocker from this first pass.

## Indirect-call finding

A second independent synthetic fixture exposed another prefix escape:
`PyObject *p = (*callback)(owner);` is accepted and emitted. The validator checks
named calls with IDENTIFIER followed by LPAREN, missing indirect call syntax.
This violates the nonescaping-prefix contract. Source approval remains pending
rejection of indirect calls (including parenthesized function names) or a
narrower positive initializer grammar, with negative cases. The worker and
coordinator were notified immediately. This finding supersedes the preceding
no-blocker statement; native qualification is still pending independently.

The indirect-call finding is now resolved: the worker rejects a call opening
after a closing parenthesis or subscript and added all three negative cases.
Independent full generator runs now reject `(*callback)(owner)`,
`(callback)(owner)` and `callback[0](owner)` with the canonical indirect-call
error. The reviewed frozen source has no remaining identified source blocker.
Final approval still requires the coordinator's generated/native and executed
validation evidence.

## Committed/generated/native review

Reviewed commit `e014965013bb11b5db400bb23e50a9a22fcae340`, its generated
handlers and coordinator `native50a` before/after release disassembly. Release
and debug both compile successfully with 117 checked modules. Generated methods
acquire the descriptor before `IR_CONSUME_INPUT`/`IR_TAKE`, then publish both
canonical owners. Native output retains the counted descriptor increment and
the local-receiver increment or temporary-source NULL transfer; no owner is lost.

All three method variants remove the former PyLong/PyFloat type comparisons and
`_PyValue_Canonicalize` calls. Method handler sizes fall from 741/720/710 bytes
to 249/244/249 for with-values/no-dict/lazy-dict. No new out-of-line call appears.
Ordinary exact CALL remains 1,410 bytes and unchanged in scope. These are native
mechanism results, not a whole-workload speedup claim.

Coordinator release focus completes successfully: 1,405 tests, 10 skips across
13 test files. Worker reports the combined 110 host checks pass after the final
hardening; the three indirect-call rejection cases were independently checked
as recorded above. Debug focused execution remains pending at this update.

Final debug focus also completes successfully: 1,405 tests, eight skips across
the same 13 files. The coherent release/debug focused logs and source/native
evidence resolve this component's review gates. **A50 at `e014965013b` is
approved for coordinator performance admission.** This does not claim full-suite
qualification, measured speedup or deployment; those remain coordinator gates.
