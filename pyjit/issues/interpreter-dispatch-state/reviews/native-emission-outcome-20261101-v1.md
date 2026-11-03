# Slot-base native work review

**ACCEPT the bound native observations for a separately reviewed elapsed
comparison.** The nominated outgoing slot-cache pairs disappear. The sampled
frame/IP register-residency hope fails: those values instead incur repeated
stack traffic. These are compatible facts, not a net-work or speed verdict.
No source correction, compiler-flag search, timing or landing is authorized by
this review.

The source is `3ddc437679ebbb9f8629c1670655c58776075dfc`; the separate
[native behavior review](native-behavior-outcome-20261101-v1.md) accepts its
actual first-feedback scopes. The frozen emission receipt is SHA256
`0fd13a76e2eabcc708f887b1d3d390db7d88378bedc1386c29bc1f3e6dcf0c22` at
`build/worktrees/dispatch-slot-base/build/dispatch-slot-base-feedback/emission.result.json`.
Independent host receipt
`build/interpreter-dispatch-state-review/native-emission-outcome-20261101-v1.json`
is `db59d26907b56984094674e71764bb47334bd6ce322ad9c8c7677f1012316836`.

The author analysis, read as evidence rather than independent acceptance, is
`native-analysis.md` SHA256
`f18fd62f3559aa91e59a70d89b713324111c5a723c67a1a2865409d891a097a0`;
its capsule is `a1b9d6c2cfbc212f21da47ee8ae9df28a333eaabc6eabfd4e1eb600dd7f2fda3`.
The independent replay rehashed all 100 bound author artifacts and all 18
original captures, reconstructed 45 exact saved windows and 18 segment lists
from the complete disassemblies, and checked 46 named candidate control-flow
edges plus value-provenance anchors. Segment lists can include conditional
alternatives and one copy of a loop; they are not dynamic traces or frequencies.

## Outgoing transport and preserved work

| Transition | Accepted baseline pair | Candidate successful continuation |
| --- | --- | --- |
| YIELD_VALUE | `457acb` loads caller base; `457ad2` stores slot cache | `44fe3b–44fea3` stores caller frame and next IP, without that base/cache pair |
| RETURN_VALUE | `452bf3/452bf8` after clearing; generator-resumer variant `4631c6/4631cb` | Ordinary `465453→451bb9→451be4`; generator-resumer `466c6e→466c73→451bc8/451bb9`; no resumed-base cache pair |
| RETURN_GENERATOR | `45c95c/45c96f` before dispatch | `451dbf–451e16` preserves frame/IP publication and dispatch, without outgoing base/cache transport |

YIELD's base-frame bookkeeping detour `4653c3` rejoins the same tail. RETURN's
ordinary no-injection edge goes through FrameClearAndPop at `46545a`; its
generator-resumer edge publishes the result at `466c44`, clears at `466c6e`
and chooses the computed/fallback continuation. Optional injection `473571`
rejoins before clearing. The corresponding baseline generator-return branch
does reload/store the base cache after clearing. The difference is not inferred
from the ordinary-return entry alone.

RETURN_GENERATOR's bookkeeping `46541c` and chunk-reclamation `4653f7` branches
rejoin its reviewed tail. Float/long conversion branches `46542a`, `466f86`,
`46e019` return to result publication; `466e2e` rejoins final dispatch. Allocation
failure `470513` publishes next IP and enters common error `44f3a0`; it is not
a successful return transition. Numeric conversion code exists statically;
this review does not claim generator objects execute those numeric branches.

Caller-base loads used to publish actual values remain: for example YIELD's
`44fdd5` and RETURN_GENERATOR's `451d47`. Removing those would change semantics.
The old slot-cache stack address and the new IP stack address are both
`-0x628(%rbp)`: they hold different values. The candidate stores derive from
frame IP plus 8, rather than frame base plus 0x10. A textual search for that stack offset
alone cannot prove the work removed or identify an unchanged slot cache.

## Receiving loads and observed costs

The sampled baseline entries retain frame in r14 and IP in rbx and load slots
from their C-stack cache. Candidate entries instead load frame from `-0x630`
and IP from `-0x628`, then derive slots through frame offset 0x10. The receiver
therefore gains a frame-to-base dependency, and repeated frame/IP memory
operations accompany it. This source cut does not preserve baseline residency.

LOAD_FAST_BORROW is a complete straight-line example: baseline `45ddca–45de04`
contains 14 static instructions; candidate `455836–455892` contains 19. Candidate
`45583a` loads frame, `455841/455848/455857` reload IP, `455853` derives base,
and `455887` stores next IP. Its value copy remains. That is an actual emitted
path cost, not a dynamic-frequency or whole-program instruction estimate.

STORE_FAST preserves both clear/no-clear joins but adds repeated frame/IP
loads and the next-IP store. Exact tuple/list unpack retain the useful copy
loops and zero-count branches; their guard/length misses reach generic unpack
through `460cb0/450630`, which derives current base at `450660`. The existing
unpack helper, boxed-input precise window, specialization redispatch and error
cleanup are retained. FOR_ITER_RANGE's small-int, exhausted and boxing-success
paths dispatch; boxing failure publishes IP then enters error. Type misses
reach generic iteration at `453dd0`, with actual success/exhaustion/error and
specialization edges followed in the saved full text. No incoming slot-cache
dependency is smuggled into these fallbacks. This remains bounded inspection
of the named paths, not a whole-evaluator CFG or root-correctness proof.

## Matched footprint and limits

The accepted image is
`289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`;
candidate is `d3d9843ee1a6d441f22d753d96d1d3224c9fa0799f14503f4d71f3fd792ef035`.
Their Makefiles match after only source/build-path normalization, pyconfig
bytes match, and actual compiler/tool/product bindings pass the behavior
audit. Read-only ELF checks give one unique evaluator range per image:
baseline `[0x44f270,0x471ef7)` is 142,471 bytes; candidate
`[0x44f270,0x475b2c)` is 157,884 bytes, up 15,413 bytes (10.81834%). No separately
named evaluator partition is present. Identical dynsym/symtab entries count
once; distinct ranges would require separate accounting.

An initial reviewer assertion incorrectly expected one raw readelf row rather
than one unique range. Both images have duplicate identical symbol-table rows;
readelf also prints these sizes in hex. The preserved host-only failure record
is `build/interpreter-dispatch-state-review/native-emission-original-reviewer-failure-20261101-v1.json`.
Only reviewer deduplication/parsing changed; all reviewed source/products and
native receipts stayed frozen.

The text increase and extra sampled traffic are diagnostics, not a proxy veto
or evidence of cache causation. Executed frequencies, net workload work,
elapsed importance and GCC allocation-pass cause remain unmeasured. The
predeclared elapsed campaign can answer whether this correct source cut is
useful; no new source or flag change should intervene before that decision.
