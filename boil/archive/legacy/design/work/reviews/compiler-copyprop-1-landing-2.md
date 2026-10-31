# Compiler-copyprop rung 1: landing review, round 2

**Verdict: LAND. No blocking findings.**

Reviewed head `b59ab985ae7` against integration `71c1e8d1fa9`, on a
detached checkout. This record preserves the review findings and verdict
supplied through the coordinator; its wording is normalized rather than
a verbatim transcript. The original fix-first review remains in
`compiler-copyprop-1-landing.md`.

## Disposition of the prior blockers

1. **The specialization phase is now distinguished from generic lookup.**
   The C fixture directly invokes a specializer without executing the
   generic operation. The comparison event records its phase and word.
   Removing only the dict guard fails two invariant assertions; bypassing
   only the MRO guard fails six. The semantic `__getattribute__` case
   omitted from the earlier test is present. These controls establish the
   no-Python lookup invariant on the exercised paths, independently of
   receiver-result assertions.

2. **Control and final-build provenance are reconstructible.** The exact
   guard variants reconstruct to their recorded source hashes. Applying
   `baseline.patch` reconstructs the two production source files from
   `41c716c5947`. All 20 final source hashes and both interpreter and
   extension hashes match `final-provenance.json`. Production compiler,
   handler and metadata source matches `1bf1f941c88`, apart from the
   declared comment; the final evidence commit changes no tested C or
   test modules. The invalid historical control is withdrawn rather than
   reused as evidence.

3. **The measurement corrections and scope are explicit.** All 94
   allocation/box counter comparisons and the corrected instruction
   deltas recalculate from the retained artifacts. The before-equivalent
   comparison is labelled as a controlled source experiment, not a
   pristine baseline checkout. The coordinator's checkpoint I ownership
   of remaining uncertainty and memory metric scope is accepted.

## Verification scope

The historical full-suite gates retain their original revision. Their
survival through the declared test/accessor/comment fixups is explained
honestly. The final targeted checks on both builds, actual-mode asserted
debug-kill run, content-preserving regeneration and unbuilt checks
support the permitted fixup scope. The coordinator's merged integration
ladder is still due; this verdict does not substitute for it.

The reviewer inspected source and artifacts read-only and ran no tests or
builds. Reconstructing hashes and recalculating retained measurements are
artifact checks, not new runtime measurements.

## Limits and hand-offs

- Forced specialization bypasses adaptive thresholds and redispatch.
  The phase oracle covers eight colliding-key paths. Constructors,
  STORE_SUBSCR, statistics-only lookups and other native callees remain
  outside that proof and within the checker hand-off.
- The heap-operand regression is semantic coverage, not a proof of the
  missing reader root. The collecting reader oracle remains 10a work.
- Allocation-time buffer release remains 10a work; its exception is
  explicitly recorded rather than claimed fixed here.
- Checkpoint I retains the unexplained performance/counter/RSS changes
  and memory allocation/pause scope under the coordinator's disposition.

---
Reviewer: `/root/compiler_review`, Codex gpt-6.1-sol, xhigh, read-only
source/artifact review of a detached checkout; approximately
2026-10-29 06:17 PDT.
