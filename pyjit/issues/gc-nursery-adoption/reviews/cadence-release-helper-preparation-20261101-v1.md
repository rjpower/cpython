# Cadence release helper preparation review

Decision: **ACCEPT for the six held release correctness rows only**. Reviewed on 2026-11-01 at 00:17 UTC. Root must activate the exact reviewed manifest after the active IR batch has terminated and its cleanup is confirmed. This review grants no execution, performance, qualification, or landing credit.

The frozen master is `build/worktrees/nursery-adoption/build/nursery-cadence-release-readiness-v1`:

| Artifact | SHA-256 |
| --- | --- |
| `plan.json` | `00bce1f174b4e4b66746f57cb67f9af5eaf8de9fdc040bdd0a99335d4ad79e9c` |
| `launch-held.json` | `e444fe3de10f6458506b9d2ab9a1baaa6e53d71d49a5119e18e9f7da1e92983e` |
| `ready.json` | `839f98d11961b6651f9169795bcbdb6a83daa285d128ecc550bd745a68fc74f5` |
| `launch.py` | `9194cfc75b29e57e6f7792bfca80c6b8ad7ea0ace58b2c94c6ffa34d9fd672d8` |

The six literal rows build adopted `ef0af95904b87d888eee4a98b6a745835ff95085`, run its focus and stale controls, then build cadence `47ccf35e21a94fde9351d1c2e4571bf207257a4c` and run its focus and stale controls. Both `build/release-cost` paths are fresh. Configure remains `CC=gcc`; post-build guards require O3, NDEBUG, the accepted frame-pointer/zero-initialization/no-GCSE settings, and no debug, stats, free-threaded, tail-call, JIT, or tier-two build. Compiler-selected tool paths, including assembler, linker, cc1 and collect2, must match the frozen readiness identities. No third build is planned.

The adopted selection retains all 20 original method IDs. Two exact `support.Py_DEBUG` guards justify the declared `KILL requires a debug build` skips; these receive no execution credit. Its five supported verifier-mode children and other selected bodies produce 31 expected normal children. Cadence retains all 13 IDs and 21 release children; its one debug-only KILL subcase accounts for the difference from the accepted 22-child debug result. I rehashed all 22 assembled cadence child files and independently matched the release roster. Thus there are 33 selected methods, two declared skips, and 52 expected normal child processes, with zero expected KILL auxiliaries. These are expectations awaiting native evidence.

The adopted finite-major child was reassembled from the actual source prelude, wrapper and method, and matched the frozen child byte for byte. Its sole changed method and all other method ASTs match the explicit bridge from the previously observed retirement selection. Child validators retain exact case/code rosters, actual observer and extension origins, mode/stress checks, return codes, positive root collections, and zero fallback/unpinned requirements. The inherited natural `-I` settrace child remains explicitly isolated and hash-randomized; it earns no deterministic-seed claim.

The canonical stale probe file and its `CHILD` are unchanged. Each image runs the same six cases, 100 internal iterations each, OFF/N0. The scoped spool correction addresses the review finding that the old capture-output wrapper could lose partial child output on an outer timeout: the new wrapper launches the identical canonical child directly into exclusive stdout/stderr files opened before waiting. Both real wrappers, with only the subprocess mocked, preserved partial streams and bound their hashes in failed receipts after `TimeoutExpired`. The combined log is assembled after return for the unchanged canonical contract. Prior drafts remain preserved.

The historical e9 image is reused through the accepted bridge: clean `e9e10d5b503eaf219797a5db44766eeab6fad4dc`, 6,916 exact source entries, 594 exact products, native SHA-256 `289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`. I replayed source/product/configuration/tool/library checks, rehashed the retained native/extension ELF inputs, and rehashed all 334 artifacts in the qualification completion capsule. Existing qualification scope and limitations remain inherited; this does not claim new historical execution.

The master and both scope loaders bind helper bytes and executable modes, clean source heads, source inputs, review identities, product indices and bridges. The five protected product sets include the failed cadence build and previously accepted precise image. The unchanged accepted row supervisor retains two-slot admission, per-image locks, deadline-clamped execution with 210 seconds reserved, terminal receipts before cleanup, current idle observations even on failure, bounded verification, and first-failure stop. The new batch has at most 45 minutes and cannot extend beyond 04:18:11 UTC. The old first-feedback deadline and failed attempts remain historical evidence. Final success includes a post-aggregation deadline check.

Independent host replay rejected wrong child code, mode, stress, observer identity and return code; stale unpinned roots and incomplete fractions; a 46-minute activation; an altered manifest; and missing mandatory aggregation evidence. Optional auxiliary absence remains explicit. Fixture activations were confined to the review directory and disabled after checks; the real master activation was absent.

Evidence is under `build/nursery-adoption-review/cadence-release-helper-preparation-20261101-v1/`: `audit.py`, `receipt.json` (SHA-256 `e954e64bfb64e7b729fd793aa2f3bbeaaa80b5f0d006704f613c7e9549e3c22a`), and the separate transitive/roster `supplement.py` and `supplement.json`. No compiler, target interpreter, workload, counter probe, or kernel warmup ran during this review. All 12 kernel-origin witnesses and later measured samples still belong inside the separately activated cost campaign.
