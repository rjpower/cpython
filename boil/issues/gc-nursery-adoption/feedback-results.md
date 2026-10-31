# Native feedback evidence

## First attempt: stopped before configuration

The coordinator accepted the [sealed helpers](reviews/feedback-helpers-20261031-v1.md)
and activated candidate `b34931a881b33b47a2b2f4b1b2a644c50fed963b` at
2026-10-31 21:07:43 UTC. Batch start was 21:07:46.664152, with the absolute
deadline **22:07:46.664152 UTC**. Root-owned exec session 94799 terminated with
status 1. The original evidence remains unchanged under the isolated
checkout's `build/nursery-adoption-feedback/`.

The build row failed after 0.704 seconds at the initial toolchain comparison,
before configure, compilation or a target interpreter. Readiness recorded
`/usr/bin/as` and `/usr/bin/ld`; the accepted live tool probe selected GCC's
`/usr/bin/x86_64-linux-gnu-as` and `/usr/bin/x86_64-linux-gnu-ld`. Their
resolved paths and hashes agree, but the full origin dictionaries correctly
rejected the different raw spellings. Readiness and build used different
identity procedures. This is a harness failure and earns no build/test credit.

The batch stopped at 21:07:47.646257 UTC. Post-gate cleanup acquired the build
lock and verified an empty target PID set. `build/debug/Makefile` and `python`
are absent; source remains clean at b34931a881b. The initial comparison was
outside the build receipt's `try/finally`, so this failure has no build-result
receipt. The gate traceback, launch result, batch failure and cleanup remain.

| Receipt | SHA-256 |
| --- | --- |
| `batch-start.json` | `986484b900b59b239f90929018473dca8c6364a97e7126341a98f36093922440` |
| `batch-failure.json` | `e71bf5726a24a8bc4ef8f19a589088e8f272e378fb84d32cd311b459fda86c6d` |
| `debug-build.launch-result.json` | `56b05f14552f5cde5656363262a32dbb9b62377e2c1cddd253948727b09a9c99` |
| `debug-build.post-gate-cleanup.json` | `1a95e9246e7b941b9f42e3b348219e03ff7936c74f312a8cb1db2c1e802825e8` |
| `debug-build.gate.log` | `f41d6cce29a2b3645e47833b582e3e87e95be7bcd22d5c6b1da801424d03fc77` |

At 21:08 UTC the coordinator authorized a fifteen-minute helper correction,
with no runtime edits or retry: use the same compiler-selected tool-origin
procedure for readiness and build, keep exact origin equality, and serialize
the initial comparison through the existing failure receipt. Prepare a new
held evidence directory and retain the original absolute deadline. Independent
correction review and fresh root activation precede any remaining-only retry.

## Reviewed remaining-only continuation

The [independent v2 correction review](reviews/feedback-helper-correction-20261031-v2.md)
accepted the exact helper change within its ten-minute cap, including an actual
host-only failure-receipt fixture. The v1 source, test selection, observer,
skip and cleanup contracts remain unchanged. All original receipts remain.

Root activated v2 at 21:16:41.661016 UTC and launched its sole invocation in
exec session **53437**. Batch start is 21:16:43.755905. Both activation and
supervisor enforce the original **22:07:46.664152 UTC** deadline. Evidence is
under the isolated checkout's `build/nursery-adoption-feedback-v2/`.

- Plan SHA-256: `9d994fc26185422694c8ee56124570a138616088c36125034fc53bb2801c9f46`.
- Manifest: `446e8b93f06d6d8968102ae6aa21c37fe3f3a28a7e41abd0178f89105f017924`.
- Activation: `f88feed52cd7195c0de1c37498dfd57efc5c57583354feaf53ae0c0d7ba87252`.
- Independent correction receipt: `dc9a98c6f81f80e5e7a2288efaa883b70a10c962ddc3f17e07e6e72f27423bef`.

At the 21:17 UTC checkpoint, the exact toolchain check had passed and configure
was running. No build, test, qualification or performance pass is recorded yet.
Owners monitor this invocation; no alternate worker or retry is authorized.

### Build passed; focused selection failed

The debug build returned zero after 95.871 seconds at 21:18:19.898 UTC;
post-cleanup verification returned zero at 21:18:21.934. Its module check
reported 116 modules, zero import failures and ten missing optional modules.
Build-result SHA-256 is
`760be694487803efbfd3c851f1e821a73ea2fa701354525141e92c4f325aedf2`;
post-cleanup receipt is
`45b86d2c0ba97561c796542361f4fa112fc5061b93cb127e0e5f31f72a8e2943`.
This is an unqualified build result for b34931a881b, not behavior acceptance.

The first OFF selection ran twenty methods in 5.995 seconds: eighteen passed
and two failed. The target returned 2; the enclosing row returned 1 and the
batch stopped at 21:18:40.749627 UTC. Later KILL/extras, retain/stress, remote
statistics and whole-GC rows did not launch. Session 53437 is terminal; cleanup
verified no live target PID under the build lock.

- `test_free_retires_last_diagnostic_record` failed its initial sole-record
  equality before testing post-free retirement.
- `test_private_tuple_resize_retires_diagnostic_record` passed the same-size
  and failed-realloc provenance checks and the post-move reclamation check,
  then failed an assertion that no record named the old address.

These are actual failures. Their snapshots were not printed. The fourth resize
callback allocates new cells/tuples, so legitimate address reuse with a new
record origin is a hypothesis to distinguish from stale original provenance.
Neither interpretation is accepted without the actual records. The dedicated
address-reuse case passed, but does not erase these failures or establish the
failed assertions' cause. Runtime and fixtures remain unchanged pending a
reviewed bounded observation/correction.

| v2 receipt | SHA-256 |
| --- | --- |
| `batch-failure.json` | `c325af1409fe2eed4c1ae940a390cc1f2080fb9c5724e1fde03569a4da1bd79f` |
| `focus-off.launch-result.json` | `fefd30b5660a0e944312e92a072162c342c02b7cc0c66452eb7818d48014c8dd` |
| `focus-off.post-gate-cleanup.json` | `b8378572fe3ca7f11dcaa152f6af862a50e2ed6808cb40e19310a91d814856cb` |
| `runs/focus-off.result.json` | `3788d594c50beb0cf052b6ae714ded73b652fd1e48a04e512483d6301bb48f7b` |

## Exact-body observations and bounded test correction

The [reviewed observation](reviews/failure-observation-20261031-v1.md) ran once
in session 81764 at 21:31:29 UTC. Its gate finished at 21:31:38 and terminal
verification at 21:31:40, with idle cleanup.
Both original bodies/assertions and observer prefixes were preserved. Each
child returned the required assertion failure; this earns diagnostic evidence
only. The independent [outcome review](reviews/observation-outcome-20261031-v1.md)
binds the raw snapshots, roots and all outer launch/gate/verification receipts.
Result SHA-256 is `ade9e44e45f664174641ef8b98b0d4b20deee7215999a3d54d9aac80659ecada`;
the independent audit is `8e4917acc2646c4aeb3677508c5f8a29585be5e74b57cdd4acb947c6e77a2f5c`.
The children recorded 37 collections, zero fallbacks and zero unpinned reports.

Both initial target records were present. Free removed its target while
preserving an unrelated record at the same attempt sequence; the nonempty hint
correctly stayed one. Resize preserved its original record through in-place
and failed realloc, removed it after moving, and retained two unrelated records.
This observed resize passed the old-address assertion and failed the later
whole-ledger-empty assertion. Added-prefix allocation layout can perturb the
failure: the first run's old-address failure remains preserved and is not
conclusively explained by this observation.

The review header's manually entered 21:32 minute is inaccurate; its written
and hashed approval preceded the actual 21:31:26 activation. Activation/start
receipts own the event times; the original review bytes remain preserved.

The [independent correction design](reviews/retirement-oracle-design-20261031.md)
accepts per-target original-provenance retirement, preserved unrelated records,
and a separate deterministic one-entry metadata check. Root granted source/
host work with a hard 21:49 UTC freeze. The scratch fixture calls the unchanged
live retirement function on private metadata, checks absent/no-op, exact
last-entry hint/count transition and harmless empty repeat, and never installs
state in the live collector or exercises interpreter creation. It supplies
metadata evidence rather than a B-reachability witness. The actual free,
resize and rooted-reuse controls remain. New C fixture code requires exact
source review and fresh debug products; no source correction was accepted at
this observation checkpoint.

## Reviewed correction on fresh products

Source `856222c62458bf3648e51d0bb0d278b1fbf54bd7` froze at 21:42:14 UTC,
before the 21:49 limit. The [independent exact-source review](reviews/retirement-correction-source-20261031-v1.md)
accepts its five-file correction for bounded feedback. The live retirement
hook is byte-identical; actual B/free/resize/reuse controls remain. The isolated
scratch fixture supplies the last-entry metadata transition without installing
state in the collector. Twenty method IDs are unchanged.

Root [accepted the fresh-build helpers](reviews/retirement-feedback-helpers-20261031-v1.md)
after host replay and preservation checks, then activated the sole invocation
at **21:47:26.018736 UTC**, exec session **90952**. Evidence is under the isolated
checkout's `build/nursery-adoption-feedback-retirement/`; products use the new
`build/debug-retirement/`, preserving the prior 588-product debug image.
The original **22:07:46.664152 UTC** deadline is unchanged. Configuration/build
was running at 21:48 UTC; no corrected target-test result is recorded yet.

- Plan SHA-256: `d5933054028035800bfbe0ad4cb87a77f8fc13d21fb454fa2d23260daa9c5568`.
- Manifest: `bdebe4659fedb4ad977788b1ce7b826368644564f87ffca2c0f3410281d5e6cf`.
- Independent source receipt: `1f1341be15100d4902459bb9ee7af49ae3a8630028d1c055f4cd879d0fc36684`.
- Coordinator helper receipt: `13a0febe6e179f48ad7ced40fd7ccdcf298ff268c0f65564eaba745fe2110d84`.

At 21:52 UTC, the fresh build passed, followed by the full focused selection:
twenty OFF methods/37 children, nine KILL/both-extras methods/12 children, and
one retain/both-extras stress method/two children. All thirty method executions
passed without skips; the 51 actual children include one separate expected
KILL auxiliary. Dynamic stress remains backed by the frozen assertions rather
than per-call observer telemetry. All three cohorts reported zero fallbacks.
Both remote-statistics cases actually executed and passed without an availability
skip. Whole-GC OFF then returned 204 tests with its 21 expected skips. Its
terminal contract and the subsequent KILL row still need final disposition.

### Terminal first-feedback decision

All seven rows completed at **21:53:34.769030 UTC**, before the original
deadline. Root polled session 90952 to terminal exit zero at 21:54:44 UTC.
The independent [outcome review](reviews/retirement-feedback-outcome-20261031-v1.md)
accepts this exact batch as first native feedback. Completion SHA-256 is
`3bb58d5a6c5dff679806ede5ec1bbe0f9a6ac38f65a2a2ddc6f1dd38d7cc90c1`;
independent receipt is
`f1faf960ffd8fd7838d965d4f1b37bf2e1664b9e8e600524fd541cd125dc1a69`.
All 91 bound artifacts, 6,944 source inputs, sealed products, toolchain and
seven gate/verification/idle chains match.

Whole-GC OFF and KILL each passed 204 tests with the exact 21 expected skips.
All normal nursery children have positive main-interpreter collection evidence
and zero fallback; expected KILL auxiliaries earn no normal-end credit.
Every row has zero fallback. Each whole-suite worker records one unpinned
event, consistent with its passing deliberate `test_unpinned_counter`; no
per-collection attribution is claimed. Focused and remote-stat rows have zero
unpinned events. The owner's extra blanket-zero audit assertion was too broad,
was preserved and corrected in its host receipt; it was not a native failure.

This accepts first feedback only. Temporary force-major and minor-debt-reset
causal controls remain before qualification. Automatic halfway cadence and its
cost decision remain separate; there is no landing or performance acceptance.
