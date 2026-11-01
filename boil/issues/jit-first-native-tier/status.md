# First native tier

2026-11-01: host/source-only design prepared against selected RC source
`d7df12b1d81722927e71d9734c70eeafdccfee73`.

The [proposed first tier](design.md) is a guarded compact-integer entry prefix
with local SSA versions, hardware register allocation and transactional return
to existing interpreter semantics. `spectral_norm.eval_A` is the source-nominated
consumer; actual word admission, native behavior and elapsed gains are unproved.

No source implementation, compiler, target, profile or timing is authorized by
this design. RC qualification and independent design review precede a later
bounded source decision. Tracing-GC work remains stopped.

The frozen design has been sent directly to
[Claude Fable5.1 at medium effort](peer-review-request-20261101.md), live
session44860, with read-only local source tools and a fifteen-minute ceiling.
It returned terminal0 by07:34 UTC. The [verbatim review](claude-fable-5.1-review-20261101.md)
recommends amendment. The [coordinator disposition](coordinator-disposition-20261101.md)
accepts explicit guards and a pure scalar interval, defers lowering expansion
until an actual word witness, and orders C-leaf contract tests before encoding.
Actual emitted-word admission remains pending; original design bytes stay frozen.
