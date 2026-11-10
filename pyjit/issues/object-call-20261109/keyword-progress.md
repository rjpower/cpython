# Keyword lane progress

2026-11-09: Proposal ready for independent review in
[keyword-proposal.md](keyword-proposal.md), against runtime `4d666e1b482`.
Read applicable AGENTS, CPython AI policy and canonical project entrypoints,
next-five plan and micro addendum. Inspected actual specialization, guarded
binder, generic provider, overlapping frame helper, generator proof and tests.
Runtime worktree was clean on inspection. No runtime source edits, native
execution, builds, tests or commits.

Scope: at most three cached keyword destinations within existing feedback
storage, arbitrary positional prefix, permutation and positional defaults;
kwonly parameters only when explicitly supplied. Reuses the existing consuming
binder and child/return publication. Review should focus on all-guards-first
ownership, clearing kwnames before overlap, defaults-hole proof, version/name
guards and the generator's kind-specific fallback. Coordinator requested the
bound apply to keyword count rather than benchmark arity; proposal reflects it.

Implementation approved after the independent
[proposal review](proposal-peer-review.md) and golden publication at
`cb906f9bc52`. Call regions were explicitly assigned independently of the
parallel lookup lane. Root also assigned `CallStats.keyword_plan_hits` and its
dump/aggregation. No generated products, builds, native tests or commits belong
to this lane.

Implementation progress (2026-11-10): bounded mapping, unchanged-size feedback
union, explicit self/kwnames specializer interface, shared consuming binder,
keyword generator contract/fallback and stats counter are implemented in the
runtime worktree. All approved default-hole and explicit-kwonly scope remains.
Nine focused host generator tests pass; the complete host generator module
passes all 96 tests. Runtime tests parse and now cover permutations, positional/self prefixes,
defaults and mutations, complete observed child locals, retirement reentry,
ownership release, errors and generic fallbacks. Native execution remains root's.

Self-review added a current-default-shape guard: MAKE_FUNCTION-created functions
can share a code/version while carrying distinct default tuples. Required-slot
coverage is rechecked before detach using at most three cached destinations;
there is no generic name binding on the hit. A factory-constant regression
exercises missing-default fallback after such a callable change.

Host generator invocation must initialize the host standard library before
appending source `Lib` to `sys.path`. `PYTHONPATH=Lib` failed at startup because
this source codec module uses a newer CodecInfo argument than the UV host;
that failed invocation executed no tests. The ordinary host invocation without
the appended path likewise could not import `test`. Corrected invocation passes.

The final source diff was reviewed and `git diff --check` passes. The feedback
header asserts the unchanged call entry size. Root was notified that producer
edits are stable; generated products remain untouched by this lane.

Host command, from the runtime source:

```
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B -c 'import sys, unittest; sys.path.append("Lib"); unittest.main(module=None, argv=["unittest", "test.test_generated_cases"])'
```

Next: root regenerates,
compiles real release/debug builds and runs focused native tests. Independent
actual-diff review and measurements precede integration. Root owns all of these.

## Positional default-shape repair

Astra's source review found the same shared-code/version issue in the preexisting
positional binder: it reads current defaults with a cached `default_start`.
The [repair proposal](positional-default-guard-proposal.md) was independently
approved and root assigned its positional region. The binder now checks current
default count, required positional coverage and cached-offset agreement before
allocation or owner detach when defaults are needed. A miss uses the existing
transaction; no ABI, feedback layout or generated product changes are needed.

Added a same-code factory regression that first asserts warm CALL_PY_GENERAL,
then changes to equal-size, larger, shorter, empty and overlong defaults tuples.
It checks current values/right alignment or ordinary missing-argument errors.
AST parse and diff checks pass. No native run or commit was performed by this
lane; root's earlier native checks predate this repair, so its new native
validation remains pending. The design page records that limit.
