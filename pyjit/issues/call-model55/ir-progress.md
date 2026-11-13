# IR lane progress

2026-11-12: Read canonical coordination, model55 proposal/resolution, applicable
AGENTS and the official CPython AI policy. Audited selected54 source in the
isolated `BASE/worktrees/call55-ir`; canonical runtime source was not used.

M0 found a split admission boundary: `_PyCode_Validate` is common, but public
`check_words` alone checks operands/normalizes changed inline constants, and
`_PyCode_New` trusts callers. Existing regcfg, outcome and encoded worklist
proofs are the extension points. See [proposal](ir-proposal.md) for the bounded
first executable M1 change, follow-on contracts and tests.

Root peer review approved the bounded admission increment. The isolated source
now makes `_PyCode_New` the common structural-check, word-normalization and
semantic-proof boundary, using a copied constructor. Assembly, marshal and
public construction no longer perform separate admission. Existing zero-code
padding remains accepted. Window/output extents, local/register domains,
branch targets, name/constant indices and typed feedback entries are checked;
frame geometry is bounded before integer addition. Inline constants normalize
on every route. This is admission hardening, not full definite assignment or
an ownership-map proof.

Authored `Lib/test/test_ir_admission.py` covers constructor/replace and patched
marshal records without executing malformed code, plus valid round trips and
constant normalization. Host AST parsing and `git diff --check` pass. No builds,
candidate runtime runs or native tests were performed by this lane. Diff is
ready for independent GC-lane review and root compilation/execution. Root owns
generation, validation, integration and commits. NULL homes remain authoritative.
