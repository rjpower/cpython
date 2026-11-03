# H3-1 stage2a landing review

2026-10-29. Independent read-only review of624c75f6455b2dee3642c9d7575cda549edff829
against integration6c724c29408. Verdict: LAND_STAGE2A. No tests, builds or
edits by the reviewer. Integration waits for ROOTS2b qualification.

All numeric IDs remain unchanged. Independent reconstruction of the old
shadow/deopt relation confirms the302-entry uint16 base map preserves its
results across288 format entries and is idempotent. Specialization kind
remains distinct from normalization. Generated properties separate metadata,
dispatch, pristine admission and required state:287 dispatch handlers divide
into153 pristine and134 runtime forms. Compiler-selected inline constants
remain admissible; runtime specializations, monitoring wrappers and static
execution shims are rejected during shared construction.

_PyIR_MakeWord/_PyIR_SetHandler check the original integer before narrowing
and preserve the destination on failure. All setter results are consumed.
Copied-header controls cover truncation to both CACHE and valid NOP.
_PyCode_New owns shared validation and replacement-reference cleanup;
constructor, replace, compiler and marshal paths reach it, as does active
frozen loading through marshal. Padding and inline replacement remain intact.
The additive CAPI fixture matches the authorized scalar-only handoff.

Recorded source/build/product/evidence hashes match. Tooling parent3dff
has no native/generated/test delta. Release/Og smoke1,924 per build,
affected974 per build and corrected strict-kill974 pass. The last covers
all ten selected modules, including existing specializer phase cases.
Raw counters confirm38 processes/808 collections/0fallback/0unpinned.
Earlier worker-setup failures remain preserved and superseded.

Clearance covers stage2a only. Required-state bits express installer
obligations; scalar rewriting alone does not prove state installation.
Monitoring storage/publication, narrow consumers, statistics, inspection,
transport/schema closure, renumbering and final H3 gates remain open.
No CFG/root-map validation or performance result is established.
