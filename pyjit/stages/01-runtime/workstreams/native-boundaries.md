# Native parsers and extension boundaries

Parent: [runtime integration](../README.md). Owner role: design and review agents,
worker agents for mechanical packets, coordinator coherent composition and gates.

State: P1 to P3 landed, P4 in progress, post-review running.

[native-boundary.md](../../../design/native-boundary.md) (accepted with changes,
[review 1](../../../design/reviews/native-boundary-review-1.md)) replaces the
explicit owner-scope approach with an implicit per-thread borrow stack. The four
frozen FORMAT templates are withdrawn. Checkpoint56 and its export-lifetime
fixture are history.

| Packet | State | Commits |
| --- | --- | --- |
| P1 borrow stack and getargs; P2 entry points and scopes | landed | `acbdf45627d` |
| P3 Clinic on the borrow stack; `_PyArg_OwnerScope` deleted | landed, with review-1 follow-ups and borrow scope tests | `56ff66a596b`, `643c69c7244`, `6d39221996d` |
| P4 argument vectors on the borrow stack | in progress | none yet |

`56ff66a596b` alone does not build; the two regeneration commits complete it.
P4 removes the per-call malloc in `native_scope.c`, a known cause of suite
failures. A post-review of P1 to P3 is running.

Hard upstream compatibility is not a gate. The opaque API should make rebuilt
source ports practical and incremental; it must not preserve unsafe borrowed
results or add a permanent parser fallback. See [extension stage](../../05-extensions/README.md).
Producer changes belong in `Tools/clinic/libclinic/`. The
[native parser record](../../../issues/pyvalue-rc-restoration/native-parsers/status.md)
retains historical component gates and designs.
