# ROOTS 2b + bytes3a composition review

## Preflight: SOURCE_COMPOSITION_CLEAR, actual candidate still pending

2026-10-29. Independent read-only source review; this reviewer authored
neither runtime patch. No merge, source mutation, compiler, target test,
probe or launcher was run. The coordinator owns integration/execution and
its accepted scoped 2b exception; this is not a LAND verdict.

Pinned inputs are primary1e82f399ddfd45034fdc80ba09297cba51cc1acd,
ROOTSc67954f336a90402bf320d2cd02fa30e4ca7d223 and
bytes08953c89274bf5541e49ebaf41147912734cbf15. The latter descends
ce2ac8991eebb1bbb10698ff2d07a79bd4cba452; its runtime patch was built at
f5e818b0454764be06968b463b600820553ccbe3. Their primary/ROOTS merge base
is64ab2528bf891c93e2a8b41875adf89ffdf81036. Root and bytes endpoints
were clean when read. Primary had an unrelated new untracked attribution
work document, excluded from this review and left untouched.

Read the coordinator's independent gc3a-bytes-writer-1-checkpoint review
and the independent scoped-exception section of10a-2b-landing-readiness.
They preserve distinct parent evidence: actual writer root-class/omission
oracles; ROOTS's causal controls and six successful profiles; original
stress100k FAIL; threshold eligibility; inherited HTTP/finalizer-progress
debt and final GC/performance limits. This preflight does not repeat those
source reviews or convert their histories into merged-head passes.

A read-only Git blob-map composition confirms:

- Primary changes after052 have no changes under Include/Objects/Python/
  Modules/Lib/Tools or native configure/Makefile inputs. Its shared launcher
  change must survive: gate_run uses literal /tmp/boilgate-slot.N, independently
  of task TMPDIR, with the existing slot/build-lock/sweep lifetime.
- ROOTSc679 adds only the exact two-line eligibility decorator in
  test_interpreters.ListAllTests.test_destroyed_by_gc plus docs relative to
  ce2ac. The test body and native source are unchanged. Preserve its declared
  stress skip without relabeling the failed full or claiming reclamation.
- Bytes089 adds only its documentation relative to f5. Its four native/test
  paths apply directly to ce2ac: private bytes header, bytesobject.c, additive
  CAPI helper/table entry and inherited bytes/bytearray writer test. ROOTS's
  c679 successor does not touch those paths.
- The only primary/ROOTS path changed on both sides is
  boil/design/11-precise-roots.md. It is documentation. Preserve both primary's
  hazard-driven future qualification/retirement wording and roadmap link,
  and ROOTS's outlined-callee rule, attribution/retained-exclusion material.
  Other coordinator-owned status/exception records must keep current primary
  wording. No native or runtime-test hunk requires competing resolutions.
- After applying the disjoint source changes conceptually, the only non-boil
  difference from f5 is Lib/test/test_interpreters/test_api.py. Native source,
  headers, generated files and tracked native build inputs are byte-identical.
  This is a prospective content argument, not an actual merged tree identity.

No new composition-specific source blocker was found. Finish the required
ROOTS tail/extras boundary, resolve the one shared doc domain, freeze the
actual candidate, then close this review against its exact Git delta.
Primary release and existing Og integration builds/tests remain fresh;
bytes's O1 receipts do not adopt O1 globally. One combined ladder suffices
under the coordinator's exception; unexpected failures stop the candidate.

An owned writer ASan reuse decision is technically coherent only after the
actual union confirms full native/build-input equivalence and config/compiler/
product bytes with f5. Retain f5 native/DWARF identity and actual binary paths;
separately bind the merged runtime Lib/Tools consumed by the fresh run.
Compiled srcdir normally selects the writer tree's old runtime files, so the
actual origin/selection must be explicit in the existing invocation. Keep
matching ASan extensions/options, exact existing allocator filter and fresh
once-only ASan16 plus new writer coverage. Do not weaken old source guards,
call the products union-built or apply this reuse to a later H3/native union.
No ASan reuse or execution is authorized by this review.
