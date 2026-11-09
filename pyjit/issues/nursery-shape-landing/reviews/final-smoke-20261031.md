# Final post-merge smoke outcome review, 2026-10-31

**ACCEPT landing closure** for merge
`d98291018c9665ae3db6952da0b0a40a4e04992b`, using products compiled from
qualified union `e9e10d5b503eaf219797a5db44766eeab6fad4dc`.
Required qualification, exact prose-only merge bridge, release/debug smoke and
current cleanup evidence now pass independent review. No new performance or
broader nursery-support claim follows.

## Concrete evidence checked

Completion `build/nursery-shape-integration/smoke-batch-completion.json` has
SHA-256 `10b94754ddc481da16b4c68c705331196481010216da73d5ec4db8df58e36750`.
Independent host audit
`build/nursery-shape-integration/final-smoke-independent-audit.json` has
SHA-256 `e8d229da5b8bde98b5a5674ed983bf85e36dc36b87c51b4d6ac1db8dbdf3eb54`.
It completed at **20:07:12.992869 UTC on 2026-10-31**.

The audit rehashed all 45 smoke-completion artifacts, all 19 ready-plan inputs
and all 334 references in the accepted qualification capsule. No mismatches
were found. Current frozen source/configuration/product/tool guards passed.
The actual independent qualification ACCEPT record, prose and audit hashes
still match; its 19:55:44 acceptance precedes the 20:14:30 qualification deadline.

The unchanged full-contract validator was independently replayed on both actual
smoke state/root records, with the original protected selections and bindings:

| Executed OFF/N0 row | Modules | Tests | Exact skips | Collections | Unwind fallbacks | Report-only unpinned |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Release | 9 PASSED | 1,937 | 80 | 784 | 0 | 0 |
| O1/Py_DEBUG | 9 PASSED | 1,937 | 50 | 823 | 0 | 1 |

Both replays exactly equal their stored summaries. Each has ten paired actual
driver/worker witnesses, no inner skip events and seed zero in every worker.
Release has 27 physical/main-root process records and debug 31; this includes
test children rather than inflating the ten-worker coverage claim. Executable,
extension and build configuration origins, actual mode/stress, source/config
hashes, terminal roster, protected cases, exact skip identities/reasons and
auxiliary separation pass. The debug unpinned visit remains report-only.

## Bridge, execution and cleanup

Actual primary HEAD remained the reviewed merge throughout execution and this
audit. Its actual merge/tree proof was replayed: 34 exact no-renames prose paths
and 6,903 otherwise identical tracked mode/type/blob entries against the union.
The [actual activation review](actual-activation-20261031.md) retains the
inspected prose patch, preserved held metadata and exact build/source distinction.
The ready helper/plan/launch/activation identities remain bound and unchanged.

The smoke batch began **20:01:38.626516 UTC**, with its declared fifteen-minute
deadline **20:16:38.626516 UTC**. It completed **20:04:58.714119 UTC**.
Literal launch commands, positive downward timeout clamps, successful terminal
return codes and gate-log hashes match the ready manifest. Post-gate idle
receipts, verification commands, return codes and log hashes all pass before
the deadline. Cleanup receipts bind their exact successful native results.
Disposable pycache directories are gone; only previously admitted regular
build-cache additions remain under the existing product guard.

Fresh independent build-lock observations at 20:07:12 UTC found no live release
or debug target executable. The runner's current idle receipts retain the new
boot ID and no after-deadline qualification credit. There is no smoke batch
failure envelope and no native retry or rebuild.

## Preserved reviewer preparation error and limits

The first host-only auditor treated the recorded `flags` field as a dictionary;
the frozen witness schema stores `str(sys.flags)`. It stopped with `TypeError`
before writing an acceptance receipt. The original body remains in
`final-smoke-independent-check-v1.py` and its failure in
`final-smoke-independent-original-failure.json`. The successor changes only
that host assertion to check the existing `optimize=0` string, then completed
the same hash, validator, origin and idle checks. No helper, selection,
qualification artifact or native execution changed to address that mistake.

The successful host auditor is `final-smoke-independent-check.py`, SHA-256
`bdf6a9f07e74b50fc43bbe9a1b69dd30cc99aab634ea2f9e631790b3e7ceca4b`.
This reviewer ran no target interpreter, native suite, rebuild, benchmark,
runtime source edit or commit. The original expired qualification attempt,
restart carry and component-only negative retain the scope recorded in
[final qualification acceptance](final-qualification-20261031.md).

The source being accepted here retains the nursery's original opt-in and
verifier/stress limits. The user's next architecture-adoption decision requires
its own implementation and qualification; this smoke does not supply them.
No outstanding landing gate remains for this exact integrated source.
