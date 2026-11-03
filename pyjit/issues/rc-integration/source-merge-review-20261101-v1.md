# Actual local RC source integration review

**Accept the source merge and working-byte recovery for fresh main-tree
validation only.** This is not acceptance of those unrun builds or JIT work.

Reviewed merge `fb5dbc0627a1d5d56c621ce7c1fdad7de0cd04df` has parents
`4106e2068b4cd718168de057fc8ce1c93acf2fec` and qualified RC candidate
`c4acfdde9d794345cd1f7001bcc7198d7d810f2d`. Its complete6113-entry runtime
projection, including `boil/tools`, `boil/bench`, deletions, four symlinks and
executable bits, equals the candidate's committed tree. All runtime working
bytes now equal the qualified worktree. Existing primary prose and worklog
remain preserved.

The initial merge verifier correctly stopped at a working-byte difference.
Independent census found exactly eight LF/CRLF differences, all with existing
`text,eol=crlf` attributes and identical normalized Git blobs. Root copied only
those qualified byte sequences, preserving each main file's full mode. No
source blob, Git attribute/configuration, or commit was changed by this repair.
The subsequent refresh commands retain their actual failure/nonzero records;
acceptance rests on independently observed final state, not a claim that those
commands passed. Every index entry is stage0 and equals the complete HEAD tree;
the cached diff is empty. Working status contains exactly the protected one
modified and four untracked user files, whose bytes, full modes, index entries
and status match the pre-merge records. Historical default/debug/release product
inventories remain exact.

Root proof `build/rc-main-integration-20261101-v1/source-merge-proof.json`
SHA256 `4945ead24dfc610e41d5f1b98835337ffc1081e5b191b3c8adf8f9afa6f07f57`
and all its referenced artifacts were independently checked. The independent
post-merge receipt is
`build/rc-main-postmerge-independent-audit-20261101-v1/receipt.json`.
The earlier independent audit's incorrect directional line-ending assertion
and corrected full census remain preserved; neither audit changed runtime or
protected files. No compiler, target, remote push or fresh validation ran as
part of this review. Proceed only to the already reviewed fresh main-tree
release/debug build and42-method-per-image validation scope.
