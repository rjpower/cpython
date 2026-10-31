# Minimal young collection: current evidence

2026-10-31. First bounded debug feedback passed. No performance measurement has
run. Nothing in this issue is landed or fully qualified.

- Base: `b3f2bcc605c0174909f2b6df8a92b51995fc070a`.
- Branch/worktree: `boil/gc-nursery-minimum`,
  `/home/power/code/cpython/.worktrees/boil-gc-nursery-minimum`.
- Existing allocator ownership inspection selected header age plus all-old
  conservative roots, avoiding new heap transfer/delete/abandonment semantics.
- 201GiB available before creating the one managed checkout. Shared repository
  objects/caches reused; serving0b15, baseline4da and upstream remain untouched.
- First decision budget began15:15UTC; checkpoint15:45, decision16:15.

The [design](design.md) bounds the consumer and remaining source work; the
[three-stage plan](plan.md) separates later coverage/reclaim/barrier decisions.
Source V1 froze at `build/gc-nursery-minimum-source-v1/` in the isolated tree;
host precise-root checking (five C files), traversal audit and five child-script
syntax checks passed. Independent review found two bounded correctness gaps:
the experimental object listing omitted GC/tracked filtering, and artificial
old retention rearmed lifecycle markers. Both are corrected in the successor;
the original snapshot remains preserved. Review also replaced a constant tuple
in the mutation control with a genuinely new young allocation and made the
allocator support skip explicit. Independent V3 source review is clear for
bounded native feedback (`source.diff` SHA256 `25e9c773d4e6cc1a457354e20290033fd824d3ab8098aeaab4e915b4b4a052a2`,
checkpoint `b4d2aafc9bec2fbf2eeaf708d2e5503ecade41af269158f3b35d528a743aac76`).
The exact ten source/test paths were committed as `3a473612157`. One fresh GCC
O1/Py_DEBUG configure/make and post-cleanup verification passed: 116 modules,
zero import failures (ten missing modules retained). The five focused controls
then passed with no skips, actual verifier off/stress zero, 108 collections,
zero fallbacks and zero report-only detached-snapshot visits. The auxiliary
mode witness and post-command/post-cleanup guards passed. All original source
review snapshots remain preserved.

At the 15:45 UTC checkpoint, the real minor consumer works for the bounded
cell/tuple cohort. Independent review verified the 24-reference first-feedback
capsule and replayed the unchanged validator with the same result. The capsule
is `build/gc-nursery-minimum-validation/first-feedback-completion.json` in the
isolated tree (SHA256 `5d960e70b6d5d107011912891695591cde2c5c57a03bcdec5cae4bc5bb0a2d22`).
The same frozen source and O1 image then passed whole `test_gc`, `test_tuple`
and `test_types` once in each requested mode. Off/N0 returned 362 tests/23 skips,
9,844 collections; kill/N0 returned 362 tests/28 skips, 9,799 collections. Both
had zero fallbacks, one report-only detached-snapshot visit, three passing
modules and verified cleanup. The five additional kill skips are exactly the
unsupported nursery methods: kill checks default-major behavior, not nursery
verification support.

One safe negative disabled the experimental mode immediately before the same
old-boundary `collect(0)`. The resulting major failed at the nominated
`minor reclaimed an old owner` assertion, before the subsequent field read.
Its `finally` completed and source/product guards passed. The standalone
negative used `-I`, so its hash seed was not controlled. The frozen follow-up
capsule is `build/gc-nursery-minimum-validation-v2/followup-completion.json`
(SHA256 `59c531941370d8f74b64578d22b8e7d991a4403f513435e713d9b13d6b49f009`,
29 references). Independent review rehashed all references, replayed both
unchanged validators and verified the negative and current source/product
guards: finite follow-up clear.

The first bounded hypothesis works. Retain the experimental consumer for the
next coverage stage and a coherent landing qualification; keep the default
disabled and decide the knob's longer-term fate explicitly. These controls do
not waive the GC qualification contract or establish LAND, speed or dominant
allocation coverage. No broad/full/performance campaign has run here, and no
historical full or timing result counts as this prototype's evidence.
