# Final attribute publication: artifact review

2026-11-06. **ACCEPT** the unexecuted `final-publication.patch` artifact for
normal reviewed integration. No remaining must-fix source finding. Root keeps
B independently validated/measured before considering C. This source review
ran no generator, tests, compiler, runtime or benchmark.

The two-line behavioral change reuses `published_temporary_result` inside the
existing five-load branch that selects `failure_result`. It adds no runtime
flag, opcode whitelist, representation or general publication cache. Final
flush therefore retains the folded-local write and displaced-owner retirement
but omits the redundant temporary write. First publication, error cleanup,
callback refresh and companion NULL writes are unchanged.

The proof and exact GCC baseline instructions are in
[final-publication-proposal.md](final-publication-proposal.md). It depends on
the current five flows leaving their published result/destination unchanged
after retirement; the postcondition is not permission to reassign that result
later without changing the generator contract. Retired temporary owners remain
rooted in the frame. A local result remains owned in C until its final store.

The updated generator fixture covers all five names and includes the companion
NULL uop. Assertions check exactly one initial store before retirement, local
failure-result cleanup, the unchanged companion slot, a local-only final store
and displaced-local handling. Existing escape rejection remains. This fixture
checks generated structure; the actual runtime's reentrant receiver/folded-local
semantic cases, regenerated-output/metadata comparison, release/debug builds
and before/after performance remain coordinator validation work. No speedup is
claimed by this acceptance.
