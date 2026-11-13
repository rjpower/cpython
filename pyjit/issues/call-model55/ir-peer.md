# Common admission source peer review

2026-11-12: GC lane independent source review GO for bounded M1 hardening in
`BASE/worktrees/call55-ir`. Reviewed `Objects/codeobject.c`,
`Python/{assemble,marshal}.c`, and `Lib/test/test_ir_admission.py` against
selected54. No native execution or compile was performed by this reviewer.

Basic constructor and feedback structure is checked before word decoding.
The common `_PyCode_New` boundary validates and normalizes into an independently
held byte string, runs semantic/source/kwargs checks against those same words,
and allocates with a copied constructor. Caller fields and borrowed references
remain intact. Compiler, public construction/replacement and marshal now pass
the same checks. Integer frame bounds precede slot addition. Encoded local,
window, branch, pool/name and typed feedback checks use existing metadata.

The tests cover three public/serialized admission routes and preserve malformed
payload lengths without executing bad images. Inline-constant replacement
exercises normalization against actual replacement values. Native compile and
candidate test execution remain coordinator gates, including compiler-produced
valid images, frozen initialization, sentinel constructors and generated forms.

Two nonblocking improvements were sent to the owner: assert that loop-based
malformed tests actually encounter each intended handler, and remove the now
unused `_PyCode_Validate` wrapper/declaration if no external internal consumer
requires it. The new common boundary is `_PyCode_New`.

This increment is not a full initialized-value/unique-owner proof. It retains
NULL authority and current allocation. In particular, current PUSH_NULL elision
and UNKNOWN physical-transfer facts cannot admit stale scratch later. General
edge, phase, callback, overlap and exception proofs remain M2–M4 work.
