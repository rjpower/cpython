# Final union qualification review

**ACCEPT for integration:** frozen source and native union
`e9e10d5b503eaf219797a5db44766eeab6fad4dc`. Independent source review and the
required carried/refreshed qualification scopes pass. This is not a claim that
the primary merge or post-merge smoke has already happened.

The sealed completion is primary
`build/restart-recovery/qualification-completion.json`, SHA256
`ca931e655365f1995a37ef3b429d333bb8d4dd73d6ed82935b69c1b7832994da`.
Independent host audit `build/restart-recovery/final-review-audit.json`
(`c39a737a`) finished at **19:54:45.707157 UTC on 2026-10-31**, before the
remaining batch's **20:14:30.135818 UTC** deadline. It rehashed all 334 sealed
artifact references and replayed all six unchanged broad validators against
actual worker/root records. Source/config/tool/product and borrowed-header
loaders passed; the union worktree was clean. No native suite or rebuild was
run by the reviewer.

## Required outcomes

| Executed row | Tests | Skips | Collections | Unwind fallback / report-only unpinned |
| --- | ---: | ---: | ---: | ---: |
| Release full OFF/N0, carried | 51,261 | 3,795 | 58,701 | 0 / 1 |
| O1/Py_DEBUG full OFF/N0 | 51,261 | 3,744 | 60,750 | 0 / 4 |
| O1/Py_DEBUG full KILL/N0 | 51,261 | 3,749 | 60,708 | 0 / 1 |
| Release stress11 OFF/N1000 | 3,121 | 153 | 35,917 | 0 / 41 |
| Debug stress11 OFF/N1000 | 3,121 | 118 | 38,381 | 0 / 38 |
| ASan16 KILL/N0 | 4,043 | 146 | 13,128 | 0 / 4 |
| Closure `test_cext`, CPU enabled | 16 | 7 | 557 | 0 / 0 |

Each full roster returned all 564 modules: 462 PASSED, 94 SKIPPED and eight
RESOURCE_DENIED, with 565 paired driver/worker witnesses. Both stress rows
returned eleven PASSED modules; ASan returned sixteen. Replay preserved actual
mode/stress, executable/extension origins, seed, protected selections, terminal
cohorts and auxiliary separation. No ENV_CHANGED aggregate was accepted.
The five unsupported nursery skip identities/reasons match exactly in KILL and
stress profiles. Report-only unpinned visits remain diagnostic counts.

The release remote-exec exclusion remains exact. ASan retains its two exact
approved exclusions and fake-stack/leak-check limits; its multiprocessing
umbrella modules do not establish multiprocessing leaf coverage. Ordinary N0
workers omit the stress variable. Cleanup result hashes, current idle receipts,
literal launches and deadline envelopes are retained in the sealed index.

Closure passed `regen-cases clinic` with frozen generated/source bytes unchanged,
traversal audit and all nineteen specified unbuilt-module syntax receipts. The
seven `test_cext` skips are the exact six unsupported limited/stable-ABI cases
and one MSVC-only case. Missing SSL/pip diagnostics remain in the raw log.
The ten missing build modules are retained, with zero import failures among
116 module checks. Borrowed headers provide syntax coverage only; `_tkinter`,
`_dbm`, `_gdbm` and `readline` remain explicitly unchecked for lack of headers.

## Carried source/backend evidence

The [source review](source-union-20261031.md) inspected the exact thirteen-file
composition against `71ee990b802`, including nursery publication/root/lifecycle/
age/OOM behavior and generated shape admission semantics. Four fresh union
builds retain their GCC O3, GCC O1/Py_DEBUG, ASan Og/Py_DEBUG and Clang21 tail
identities and effective policies. Early five nursery OFF and 34 shape/IR KILL
controls passed unskipped. The three stale six-by-100 rows, ASan/tail OFF29 and
tail KILL ownership control retain zero fallback and verified cleanup.

Nursery first/follow-up and shape component capsules remain component evidence.
The safe full-instead-of-minor negative is bound to its unchanged GC/test inputs,
fails at the intended old-owner assertion and completes `finally`. Its `-I`
hash seed was uncontrolled. It is not new union execution and does not grant
nursery verifier/stress support. The supported boundary remains one main
interpreter, Linux/GIL/mimalloc; public subinterpreter creation rejects.

## Preserved failures and acceptance limits

The original attempt expired incomplete at **17:36:20.262873 UTC**. Completed
checks stand through the explicit restart bridge; remaining work ran once in
the separate 19:14:30–20:14:30 batch and completed at 19:52:27 UTC. The carried
release full's missing historical cleanup was repaired only by a current
19:14:30 UTC idle/verification observation on the new boot, not backdated.

Original host preparation failure
`build/nursery-shape-broad/host-preflight-original-failure.json` in the union
(`ad3ff453bbe2db3c68a3001e9e65e910b22e076e987ae2e634591f01a33b20dd`)
remains preserved: an assertion chose `uv run` rather than the helper's `run`
token; no native workload launched. It was corrected as host preparation,
without changing frozen source, selections or native helpers. Wrapper review
findings and their corrections remain in the earlier unique review records.

Acceptance keeps the nursery opt-in/default-disabled, retaining its temporary
control for the next bounded coverage stage, and shape work a shared family
simplification. It establishes no speedup, dominant nursery coverage,
reduced all-old scanning, new allocator or broader support policy. Integration
must still prove an exact enumerated project-prose-only tree bridge to the
actual merge, then complete one fresh release and debug smoke on the sealed
products, with current cleanup and independent evidence review.
