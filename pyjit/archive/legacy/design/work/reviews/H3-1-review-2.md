# H3-1 design review, round 2

Reviewer: `/root/compiler_review`, gpt-6.1-sol, xhigh reasoning.
Date: 2026-10-29. Read-only re-review; no tests, builds or edits.
Coordinator preserved the review below.

**IMPLEMENT. No remaining design blocker in the revised H3-1 draft.**

The revision closes all three round-1 findings:

- Target-schema provenance: section 5 specifies immutable target-exported
  schema bytes, a distinct cookie and fixed bootstrap prefix, size/protocol
  validation before the full copy, and actual-target equality before
  reading handlers. The writer receives the verified identity.
  Same-version target mismatch and file-schema mismatch have separate
  controls.
- Construction and narrowing: section 2 separates metadata, dispatch
  bodies, pristine admissibility and runtime-state requirements. Shared
  `_PyCode_New()` validation covers the marshal/freeze bypass while
  preserving CACHE padding and inline-constant replacement. Section 7
  places checks at production packing/rewriting boundaries and requires C
  controls for both 65536 and 65536 + NOP, with unchanged-word witnesses.
- Threshold compatibility: the exact inventory implements the coordinator's
  removal ruling, updates affected consumers and preserves unrelated
  name/argument APIs. It distinguishes existing Python exports from
  C-only macros.

The four stages are coherent: establish the domain under existing IDs;
close fixed-numbering consumers; close target transport and codec
together; then renumber, remove thresholds, bump magic and regenerate
frozen code. Final-head gates remain separate from intermediate evidence.

Implementation review must verify declared state requirements at installer
paths, constructor replacement-reference/OOM handling, actual atomic
publication, and complete remote discovery/reader/writer migration. These
are implementation gates already required by the revised design.

The scope remains bounded: no new operation, H2 CFG/root reconstruction,
dispatch tuning or weakening of the compiler phase oracle.

Read-only re-review; no tests, builds or edits. The untracked draft and
preserved round-1 review remained untouched.
