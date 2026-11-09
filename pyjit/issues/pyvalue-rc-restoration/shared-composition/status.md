# Shared interpreter/compiler/native composition

- Stage: independently accepted; source committed as `c21d0a374ee36c68e9049a3e3164801896bc77ba`
- Owner: coordinator; wiki_audit reviewed
- Scope: 41 current files and three empty superseded StackRef path deletions

Named component endpoints and intervening hunks cover compiler/constant
consumers, code objects, VM publication/retirement, selected native callers,
fixed/variable native engines, fixtures, registration and generated products.
Retained generation evidence matches current producers and all eight products.
Full Objects/abstract.c, object.c and typeobject.c are excluded for concrete
scalar object/call boundaries and the separate cache correctness hold.

[Review](../../../../build/pyvalue-source-integration-20261102-v1/shared-composition-review.md),
[allowlist](../../../../build/pyvalue-source-integration-20261102-v1/shared-composition-files.json),
[commit](../../../../build/pyvalue-source-integration-20261102-v1/shared-composition-commit.json).
This dependent checkpoint is not linked, runtime-qualified or measured.
