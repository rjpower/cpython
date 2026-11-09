# Worktree closure: execution record and initial inventory

Completed authorized housekeeping,2026-10-30. The initial read-only inventory
below was against primary `ad69f335a262fb42a2e371a902cca24c0a6e566f`;
the user then authorized archival and retirement. **32 worktrees were removed
with ordinary `git worktree remove`;8 registered roots remain.** No force
removal, branch deletion, runtime merge, compilation, target test or benchmark
was performed by this lane. Topic branches remain intact.

The initial inventory had **39 worktrees**:6 retained for work/decisions
(including the repository anchor), 22 landed historical copies,
8 negative fixtures, 1 deferred unique topic and 2 dirty historical
trees. One new selective-death worktree was created by its owner and preserved:
39+1−32=8. H1, H2, deferred Queue2b and all eight negative-fixture checkouts
are now archived and retired, without merging their code.

## Terminal cleanup checkpoint

All32 exact tips have `archive/boil/2026-10-30/<worktree-name>` tags pushed to
GitHub and verified by `ls-remote`. Source has remote protection; raw evidence
has **LOCAL durability** in retained primary, as before cleanup. No offsite
raw-data backup or new artifact service is claimed or required for this batch.

The local archive preserves11,809 file/symlink entries,918.53MiB of regular-file
bytes: raw receipts, logs, helper sources, manifests, configuration, disassembly
and recipes. Retained regular bytes were SHA-verified and symlink text checked.
Manifests explicitly list excluded closed ELF/static-library/object products
and caches totaling22.13GiB of logical file sizes. This is not a claim of
physical filesystem space reclaimed. Closed experimental binaries were not
retained without an active consumer.

The [locator](../../../build/worktree-archive/2026-10-30/locator.json),
SHA374da3b3cca5c1d47a8c394e9108c752f31dadfb89c2632a235ca71a8b423668,
maps each retired root to its archive/ref and per-file manifest. Existing
finalGC12f1 (276 refs), quiet29e5b0 (393 refs) and postmergef56e (20 refs)
remain byte-identical; every referenced artifact was rehashed after cleanup.

| Registered root retained | Reason |
| --- | --- |
| `/home/power/code/cpython` | Repository/common-Git anchor |
| `python-boil-the-ocean` | Primary development and local evidence archive; unrelated user reviews preserved |
| `boil-buffer-xid-main-only` | Qualified serving4da, original three product cohorts, Lib/config/tools and frozen guards |
| `boil-checkpoint-i-upstream` | Stock and matched eval-nogcse comparator products |
| `boil-gc-selective-death` | New active implementation lane; untouched by housekeeping |
| `boil-7a` | Three nonignored untracked evidence scripts await owner disposition |
| `boil-gc-simple` | Unknown ambient in-source build and idle shell ownership remain unresolved |
| `/tmp/boilreview2` | Uncommitted review rewrite retained |

The old `boil-gc-simple-negative4` path is **not a worktree**. During an
explicit operator guard pause, its worktree was removed and exactly55 original
build-only dependency files (3,070,413 bytes) restored at their original paths.
There is no `.git`, tracked source checkout or native runtime there. The
[dependency locator](../../../build/worktree-archive/2026-10-30/batch-05-negative4/dependency-locator.json)
labels this evidence-only directory and binds the archived copies. Unchanged
serving `current_inputs_v2.loaded()` passed, including original config/tool/
three-cohort product checks, before the operator pause was released. Only host
tool identity queries ran; no compilation or target invocation was needed.

## Executed batches

| Batch | Worktrees retired | Archived entries | Regular-file MiB | Manifest |
| --- | ---: | ---: | ---: | --- |
| batch-01 | 6 | 1,559 | 78.12 | [manifest](../../../build/worktree-archive/2026-10-30/batch-01/manifest.json) |
| batch-02 | 8 | 642 | 30.33 | [manifest](../../../build/worktree-archive/2026-10-30/batch-02/manifest.json) |
| batch-03 | 8 | 1,178 | 65.76 | [manifest](../../../build/worktree-archive/2026-10-30/batch-03/manifest.json) |
| batch-04 | 9 | 8,322 | 737.82 | [manifest](../../../build/worktree-archive/2026-10-30/batch-04/manifest.json) |
| batch-05-negative4 | 1 | 108 | 6.49 | [manifest](../../../build/worktree-archive/2026-10-30/batch-05-negative4/manifest.json) |

Each manifest binds the exact old root, tip/tag, retained file hashes and
explicit closed-product/cache exclusions. Batch05 also records the guard-pause
conversion and exact55-file evidence-only restoration. Local topic branches
were checked after retirement and still point to their original tips.

## Interpretation and current dependencies

`ancestor` means the exact tip is reachable from primary. Three non-ancestor
positive tips are patch-equivalent by `git cherry` (minus result): finalizer
orderinga285, stack-hit reader7c703 and Unicode writer2f37. They need no
second implementation merge. H1, H2 and Queue2b have genuine unmatched patches;
negative tips deliberately contain regressions and must never be merged just
to empty the worktree list. Except primary itself, no complete tip tree equals
current primary; that is expected for historical bases, not evidence of missing
landings.

The initial table records local remote-tracking refs at the **exact tip**, not a fresh
remote-server verification. A branch without an upstream setting can still
have a matching `origin/boil/...` ref. No experiment archive tag/ref existed
at that snapshot. The completed batches above created
and remotely verified32 source archive tags; local topic branches were kept.
Ancestor tips remain reachable through pushed primary; unique local tips need
an explicit preserved ref/bundle before branch retirement. All regular tracked
and nonignored untracked changes were enumerated; ignored build/cache contents
were deliberately excluded from dirty counts and were not sized recursively.

The active `current_inputs_v2` guard reads serving source/config/tools and
the original three product maps; its55 negative4 input paths required the
evidence-only conversion above. Closure header roots are external SDK/vendor
paths, including the existing `ports-imaging-native` libmpdec source, not the
retired experiment checkouts. These were left untouched. The operator confirmed
that the new selective lane consumes primary/new-topic/serving/upstream paths,
with no active H1/H2/H3 or old compiler-copyprop/base-tree consumer. Historical
selection-origin metadata is archived; it is not an active source import.

Current direct receipt dependencies were read from existing indices:

- [Final GC index12f1](../../../../boil-buffer-xid-main-only/build/gc-final-traversal-closure/final-completion.json)
  has276 absolute artifact refs:221 in the qualified serving XID tree and55
  in the evidence-only `boil-gc-simple-negative4` directory. These exact files
  remain readable after its checkout retirement; negative code is not landed.
- [Postmerge indexf56e](../../../build/gc-main-only-integration/final-receipts.json)
  has20 absolute refs:19 in primary and one final index in the serving tree.
- [Matched elapsed closuree5b0](../../../build/checkpoint-i-qualified-refresh/quiet29-closure.json)
  retains393 primary artifacts plus explicit upstream/serving runtime paths;
  [the measurement record](pre-I-post-GC-timing.md) links the raw vectors.
  Executable/source/config/product-map dependencies extend beyond a direct
  receipt path count: both upstream build images and the serving ordinary
  products/Lib must stay in place while reused. A path count is not a complete
  recursive archival manifest.

No compiler/regrtest/gate argv was observed in the initial process snapshot. Accessible
`/proc` cwd/exe reads showed primary coordinator/tool processes and one idle
`/bin/zsh`, PID2046633, in `boil-gc-simple`, with no children at inspection.
499 process entries were unreadable or vanished; this is not a universal
no-user/no-process proof. Recheck owner activity and relevant build locks
immediately before any later removal. Available filesystem space was203GiB;
no claim is made about per-tree reclaimable sizes or archive capacity.

## Initial per-tree inventory

Names below expand under `/home/power/code/cpython/.worktrees/`, except the
explicit repository root and `/tmp/...` paths. Dirty counts exclude ignored
build products. Branches are local; the remote column is the observed local
remote-tracking snapshot. These are pre-cleanup facts and proposed dispositions;
the terminal checkpoint and executed batches record what actually happened.

### RETAIN ACTIVE

| Worktree / tip | Branch | Relation to primary | Remote at tip | Nonignored state | Proposed disposition / dependency |
| --- | --- | --- | --- | --- | --- |
| `/home/power/code/cpython` / `3a440b656956` | `main` | ancestor;0 unique | `origin/main` | clean | Repository/common-Git anchor; keep. This is not an experiment to merge. |
| `boil-buffer-xid-main-only` / `4da292b12942` | `boil/gc-final-validation` | ancestor;0 unique | `origin/boil/gc-final-validation` | clean | Current qualified serving images/Lib plus final276 refs (221 here); keep paths stable. |
| `boil-checkpoint-i-upstream` / `321925c92c9e` | `detached` | ancestor;0 unique | `none at tip` | clean | Current stock and eval-nogcse comparator images; timing/origin input, keep. |
| `boil-h1-frame` / `94ab98e8fd3d` | `boil/h1-frame` | not ancestor;3 unique | `none at tip` | clean | Archive for current GCC milestone; tail-only frame ABI has no measured benefit. Separate strrchr tool fix can be evaluated independently. |
| `boil-h2-root-effects` / `b3e6847348d2` | `boil/h2-root-effects` | not ancestor;2 unique | `none at tip` | clean | Archive tested decoder/parser patch and evidence; no current consumer or active maps. |
| `python-boil-the-ocean` / `ad69f335a262` | `weaver/python-boil-the-ocean` | self | `origin/weaver/python-boil-the-ocean` | 1 tracked / 4 untracked | Primary development/docs; preserve known review drafts. |

### LANDED HISTORICAL

| Worktree / tip | Branch | Relation to primary | Remote at tip | Nonignored state | Proposed disposition / dependency |
| --- | --- | --- | --- | --- | --- |
| `boil-10a` / `837ea97fd75d` | `boil/10a` | ancestor;0 unique | `origin/boil/10a` | clean | Historical ROOTS/raw-mutex/Clang baseline/stress failures and controls; retain bound evidence before closure. |
| `boil-10a-base` / `e40dd593bb65` | `detached` | ancestor;0 unique | `none at tip` | clean | Detached landed baseline; tip reachable from primary; archive any ignored historical inputs first. |
| `boil-buffer-ndarray` / `baf8e0ea4f20` | `boil/buffer-ndarray` | ancestor;0 unique | `origin/boil/buffer-ndarray` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-buffer-readiness` / `87b6d9c3ccbe` | `boil/buffer-readiness` | ancestor;0 unique | `none at tip` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-buffer-sealed-consumers` / `6275905c8c06` | `boil/buffer-sealed-consumers` | ancestor;0 unique | `origin/boil/buffer-sealed-consumers` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-compiler-copyprop` / `81417aff0971` | `boil/h3` | ancestor;0 unique | `origin/boil/h3` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-gc-finalizer-order` / `a285a658b36e` | `boil/gc-finalizer-order` | patch-equivalent;1 unique | `none at tip` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-gc-simple` / `3c6b8daa5065` | `boil/gc-simple` | ancestor;0 unique | `origin/boil/gc-simple` | clean | Preserve unknown ambient in-source configure/build (686 ignored files,446MB) and idle shell PID2046633; quarantine, not immediate cleanup. |
| `boil-gc-stack-hits` / `7c703fda1b7d` | `boil/gc-stack-hits` | patch-equivalent;1 unique | `none at tip` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-gc3a-bytes` / `08953c89274b` | `boil/gc3a-bytes` | ancestor;0 unique | `none at tip` | clean | Historical writer/ASan capsules and controls; archive before closure. |
| `boil-gc3a-unicode` / `2f37be1d0592` | `boil/gc3a-unicode` | patch-equivalent;1 unique | `none at tip` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-h3-stats` / `5d108561e7af` | `boil/h3-stats` | ancestor;0 unique | `none at tip` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-h3-union` / `e9a1bcc65cf5` | `boil/h3-union` | ancestor;0 unique | `none at tip` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-main-only` / `dbb5b9859b26` | `boil/main-only` | ancestor;0 unique | `origin/boil/main-only` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-pre-i-union` / `5a45f9bb6a73` | `boil/pre-i-union` | ancestor;0 unique | `none at tip` | clean | Historical qualified5a45/e9 Clang baselines and raw gates; decide H1 baseline retention before closure. |
| `boil-queue-single-drainer` / `44a5d023e88e` | `boil/queue-single-drainer` | ancestor;0 unique | `origin/boil/queue-single-drainer` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-queue2a-snapshot` / `07af55a7ef3d` | `boil/queue2a-snapshot` | ancestor;0 unique | `origin/boil/queue2a-snapshot` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-queue2a-sweep` / `7f4459bf2590` | `boil/queue2a-sweep` | ancestor;0 unique | `origin/boil/queue2a-sweep` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `boil-queue2a-workstack` / `9e223040a207` | `boil/queue2a-workstack` | ancestor;0 unique | `none at tip` | clean | Historical safety168/native9e test bridge plus component/broad evidence; archive before closure. |
| `boil-unpickler-buffer-owner` / `48b021223210` | `boil/unpickler-buffer-owner` | ancestor;0 unique | `origin/boil/unpickler-buffer-owner` | clean | Archive historical raw/control evidence and any referenced products, then close checkout. |
| `/tmp/boilgate` / `f2a5b3ca33d1` | `detached` | ancestor;0 unique | `none at tip` | clean | Detached landed f2 gate tree; historical full/ASan/failure evidence first. |
| `/tmp/boilreview` / `b59ab985ae73` | `detached` | ancestor;0 unique | `none at tip` | clean | Detached landed review tree; preserve ignored review evidence first. |

### REJECTED NEGATIVE

| Worktree / tip | Branch | Relation to primary | Remote at tip | Nonignored state | Proposed disposition / dependency |
| --- | --- | --- | --- | --- | --- |
| `boil-buffer-readiness-negative` / `8d41ad568c2f` | `boil/buffer-readiness-negative` | not ancestor;1 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |
| `boil-gc-finalizer-order-negative` / `025d549368a0` | `boil/gc-finalizer-order-negative` | not ancestor;2 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |
| `boil-gc-simple-negative4` / `35c92d91bec2` | `boil/gc-simple-negative4` | not ancestor;1 unique | `none at tip` | clean | Four intentional omissions; final276 directly binds55 refs here. Archive evidence before closure, never merge. |
| `boil-gc3a-bytes-negative` / `d16f5e9258e7` | `boil/gc3a-bytes-negative` | not ancestor;1 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |
| `boil-queue2a-snapshot-negative` / `8d9f72dd4f27` | `boil/queue2a-snapshot-negative` | not ancestor;3 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |
| `boil-queue2a-sweep-negative` / `5ae1269bd148` | `boil/queue2a-sweep-negative` | not ancestor;1 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |
| `boil-queue2a-workstack-negative` / `3f2a2c683e52` | `boil/queue2a-workstack-negative` | not ancestor;1 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |
| `boil-queue2b-owner-negative` / `fcb8cde5c4a5` | `boil/queue2b-owner-negative` | not ancestor;4 unique | `none at tip` | clean | Intentional omission control; archive source/raw named failure, never merge into primary. |

### DEFERRED UNIQUE

| Worktree / tip | Branch | Relation to primary | Remote at tip | Nonignored state | Proposed disposition / dependency |
| --- | --- | --- | --- | --- | --- |
| `boil-queue2b-owner` / `63b8328769c8` | `boil/queue2b-owner` | not ancestor;3 unique | `origin/boil/queue2b-owner` | clean | Foreign-owner/adoption work deferred by main-only policy; retain exact topic offline, do not integrate merely to close tree. |

### UNKNOWN DIRTY

| Worktree / tip | Branch | Relation to primary | Remote at tip | Nonignored state | Proposed disposition / dependency |
| --- | --- | --- | --- | --- | --- |
| `boil-7a` / `052d8342e562` | `boil/7a` | ancestor;0 unique | `none at tip` | 0 tracked / 3 untracked | Three untracked evidence scripts; owner/archive decision first despite landed tip. |
| `/tmp/boilreview2` / `a0338fd76348` | `detached` | ancestor;0 unique | `origin/boil/7a-rung1` | 1 tracked / 0 untracked | Uncommitted 7a landing-review rewrite (40+/34-); preserve/attribute first. |

## Approved dispositions and remaining holds

1. Keep primary, upstream and the qualified serving tree through the next
   measurement/code decision. Preserve both upstream stock/eval-nogcse images
   and the serving ordinary/ASan identities. Do not rebuild them to simplify
   labels or copy their runnable payloads to arbitrary paths.
2. H1/H2 active lanes are **closed after verified archival**. H1 was archived
   for this GCC milestone: its unmeasured tail-only convention does not affect
   the current interpreter. Reopen for one bounded comparison only when Clang
   tail dispatch is an active target. Its
   independent strrchr tool classification can be evaluated separately without
   importing the frame ABI. H2 is inert and has no immediate consumer or
   independent correctness fix: its tested patch, design and validation were
   archived and its checkout removed. Reuse it for a scheduled usable liveness slice;
   do not merge unused code merely to close a branch. Keeping a branch/ref does
   not require keeping a checkout.
3. Queue2b is deferred, not a queued merge: main-only support removes its
   immediate foreign/adoption prerequisite. The topic and its negative
   companion are archived and retired; reopen only with a declared change to
   support scope. Do not impose a GC integration campaign just to close it.
4. Within the same closure pass, have the owners identify the three untracked
   old7a scripts and `/tmp/boilreview2` review changes. Preserve a reviewed
   patch/file copy or explicitly retain them. Primary's unrelated48-line owner
   review and four draft reviews are user work and remain untouched. Release
   the gc-simple shell and preserve its unknown ambient build before considering
   that directory. If ownership remains unknown, hold only these exceptional
   trees rather than every landed duplicate.

## Evidence preservation and later closure

Use existing commits and artifact indices rather than a new registry/service.
For each approved batch, record tip/ref, disposition, retained evidence paths
and the existing index hashes in this plan or one simple archive manifest.
All retired tips have pushed archival tags
`archive/boil/2026-10-30/<worktree-name>`; the original branches were not deleted.
Negative fixtures remain identifiable as controls, not landed implementations.

Git history does **not** preserve ignored receipts, stdout/stderr, copied helper
sources, native binaries, disassembly or native/config/tool manifests. Copy
required evidence into a project-owned archive outside the checkout selected
for removal and verify against the existing hashes. The approved archive is
local under retained primary `build/worktree-archive/2026-10-30/`; it is not a
remote raw-data backup. Use selective retention, not an unmeasured duplicate
of every build object/cache. Preserve exact binaries and config/source inputs where a current
receipt or future comparison needs them. Keep original failed receipts and
expected negative outcomes, not only successful summaries.

Absolute historical paths identify where a run occurred. If evidence is moved,
retain immutable originals and a small old-path-to-archive locator with the
same byte hashes; do not rewrite a failed receipt or relabel a relocated image
as runnable/qualified. The current221 serving refs remain in place; the55
negative4 dependencies are preserved at both original evidence-only paths and
archive locations. The qualified serving tree and upstream products stay
runnable at their established paths. Other historical references resolve via
the archive locator and source tags, not a claim of relocated runnable images.
Unknown ambient output must be quarantined with its attribution limits.

After owner release, preservation verification and explicit cleanup scope are
approved, ordinary `git worktree remove` should refuse dirty cases; branch/ref
retirement is a separate decision. The existing
[`worktree.sh drop`](../../tools/worktree.sh) uses `--force` plus `branch -D`,
so it was not used. No forced cleanup, `git clean`, branch deletion, source
merge, build or measurement occurred in this housekeeping execution. Landed
work needs no merge; rejected controls must not be landed; genuinely unique
work needs a decision rather than indefinite checkout retention.
