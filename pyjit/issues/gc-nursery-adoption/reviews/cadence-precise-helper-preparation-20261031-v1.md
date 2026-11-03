# Independent precise-correction feedback preparation

Disposition: **ACCEPT the exact held successor for coordinator activation.**
No blocker found. This review executes no build or target and gives no linked
audit, collection, performance, qualification or landing credit.

Reviewed at 2026-10-31 23:40:04 UTC. The master is isolated
`build/worktrees/nursery-adoption/build/nursery-cadence-first-feedback-precise`.

| Artifact | SHA256 |
| --- | --- |
| plan.json | `755aaa840793d90e56a7e47832fa38c138d1fac5460767c0eab1e34a9ae6ab99` |
| launch-held.json | `5f281b264476c0ab0564b5e5de205f0169804588164183bed7d7cae4fc083618` |
| ready.json | `7f106095e7563f143d808e6e4654e1fad6767f9c97c824610177ffdc9348884b` |
| successor-helper-diff.patch | `bb5bfe5b3eb147557f5b8caa85738f5164c6646c14049643dc10c4dc14f0b21e` |
| successor-bridge.json | `686915f1c61d85b8e30318808ae344efa3c50960a8185d3a7e36378cca4d2cc3` |

The [accepted v4 protocol review](cadence-helper-preparation-20261031-v1.md),
SHA `f966587b969051ee06434a98a847b18c70150794e60b1530fea7afdace3fa571`,
continues to cover the unchanged worker/result/full-contract, child observer,
child contract, module validator, gate, supervisor and direct-negative bodies.
I independently compared actual old and new helper bytes, not only the bridge
description. Each scope changes only build_once's head/build path,
run_feedback's image path and feedback_inputs' head/exact deadline guard.
The master launch changes only that exact deadline guard. Executable modes
match. All eight literal launch/verify rows are identical after replacing
their owned source, evidence and build paths.

Candidate source is clean `5cc1e5310e2cb5f11feade09195b5a3695188161`, bound
to the [accepted two-annotation review](cadence-precise-correction-source-20261031-v1.md),
SHA `6da24f1dd4a1bf61cf975dd789c7761ef31e6b161dc04fe0f13cfaa9ed583be1`.
Each negative is a clean direct child of that candidate, changing only
Python/gc_live.c:

| Scope | Commit | Preserved fault |
| --- | --- | --- |
| major | `4bbafc10d7986f027a720f336a48758de6815b92` | Force automatic major selection |
| postpone | `b9ab9d12ba532714567976ffede0a4b66aad9441` | Successful minors add RETRY_BYTES to M |
| attempt | `10fca1832f4996eecda2450ce77316a095bc1d4e` | Consume H before young marking |

I reconstructed each negative's entire source from its previous accepted
negative plus the exact two precise annotations and compared it with the new
commit. The nominated child files are byte-identical. Positive test_gc source
is unchanged. Thus the planned thirteen parent methods/twenty-two children and
three direct expected-failure auxiliaries retain their original semantics;
the auxiliaries earn zero ordinary method credit. Attempt retains mode [1,0];
major and postpone remain OFF [0,0]. Existing isolation, root-counter, skip,
fault-consumption and exact AssertionError requirements still apply.

All four scopes' 6,944 bound inputs, helper hashes/modes, symlinks and recorded
tool binaries/resolutions independently match. Four owned
`build/debug-cadence-precise` directories are empty, with no configured or
linked products. Prior product sets of 588, 588 and the failed cadence build's
587 entries independently rehash with original path and symlink identity.
The failed ELF and original failure remain at their old paths; they are not
reused or patched. The additional precise annotations still require a fresh
complete linked audit; this preparation does not infer its success.

The original deadline remains **2026-11-01 00:14:59.201344 UTC**. Both activation
validators enforce its literal value. I exercised the actual master loaded()
against a separate host fixture, then verified rejection of a one-second
extension that would still fit the old 45-minute ceiling. The fixture is
disabled; the actual master activation is absent. Existing 420-second build,
180-second focus, 90-second negative and 30-second verifier caps, downward
clamping with the 210-second reserve, first-failure stop, terminal receipts,
raw stream preservation and idle cleanup remain unchanged by byte/hash bridge.
Verification and aggregation must complete before the unchanged deadline.

Independent host receipt:
`build/nursery-adoption-review/cadence-precise-helper-preparation-20261031-v1.json`,
SHA `3dd3f7e8c6462ca7d7a78bf42d5e84f33e58e3afbe24a3532bdf3e4410fae1cb`.
The adjacent script preserves the replay. Its first two host attempts exposed
reviewer-only path-normalization/fixture omissions; short logs and disabled
fixtures are retained. No frozen helper/source changed to resolve them.
