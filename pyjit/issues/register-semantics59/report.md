# Semantic IR trial results

Selected57 remains selected. The first executable trial is rejected; the
builder and native-Invoke trials remain in progress. These are incremental
results, not a completed parity cycle. See the [workstream](README.md) and
[ordered design](../../design/proposals/register-semantics59.md).

## Final loop-output binding: reject

Candidate `f61edc21a4efc3e3a09ee99bbca9d031ee9565dc`, preserved on
`pyjit/reg59-results`, changes only FOR_ITER_LIST's generated final BODY
publication. The generator acquires the same result, publishes the same cursor,
vacates B+2 and writes local A directly. Temporary/discard paths retain staging.
The [post-review](results1-post-review.md) found no source blocker.

Full generation passes. Release and debug each build 117 modules/79 shared
extensions and pass 37 methods across the new source fixture, semantic
completion, window-result transfer, transfer admission and native-owner effects.
No fresh full-system qualification was run for this rejected increment.

The native handler grows from 1,153 to 1,225 bytes. Source publication removal
does not establish reduced executed work: the local arm also changes recipient
classification and normalization lowering. The eight-case screen below rejects
this executable without an alignment sweep or repeated searches for a better
result. It does not establish the cause of every regression.

| Workload | Candidate / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .992510 | .980711–1.004451 |
| deltablue | 1.010990 | 1.006335–1.015667 |
| coroutines | 1.087467 | 1.084682–1.090260 |
| generators | 1.002899 | 1.000303–1.005501 |
| logging_silent | .993374 | .989928–.996832 |
| raytrace | 1.000804 | .999718–1.001891 |
| spectral_norm | 1.315221 | 1.314132–1.316311 |
| nqueens | 1.009831 | 1.007904–1.011762 |

Ratios are elapsed-time geometric means; below one is faster. Both coroutine
and spectral_norm pairs lose substantially, while comprehension pairs disagree.
This is a rejection screen, not a confidence interval or an all29 result.
No direct upstream campaign or PMU claim is made for this losing executable.
The selected57 upstream gaps in [status](../../STATUS.md) remain the reference.

Evidence is under `BASE/pyjit-link-wave1/register-semantics59`:
`results1-eight-selected57/{summary,status,identity-before,identity-after,commands}.json`,
`results1-native`, `results1-selected57-for-list.txt`,
`reg59-results1-retry-build-commands.json` and
`reg59-results1-focus-commands.json`. Timing identity is stable, both pre/post
checks pass, and compiler/core flags match selected57. The first build failed
because pkg-config returned unrelocated dependency paths. The durable
config.status module flags were matched to selected57, regenerated through
Makefile.pre and checked before rebuilding; `configure-module-relocation.json`
records all 25 changed module variables per role. No control was rebuilt.

## Next independent candidates

Builder source packet `1b2298676389a00aa43d3875e18962c678e3d516` is preserved
on `pyjit/reg59-edge-results`, before generated products or native qualification.
Its candidate will retain the result-binding mechanism needed by BUILD_LIST
and BUILD_TUPLE, without enabling the rejected FOR route. The existing native
Invoke worker is implementing the separately reviewed input-retirement contract.
Neither is a measured winner yet.

Full builder generation changes only BUILD_LIST/TUPLE handlers and their format
rows. Initial native bootstrap at `64257f49a55` rejects valid compiler images:
the new independent recipient required both physical NULL and logical vacancy,
while selected call cleanup can leave a logically absent home without a NULL
certificate. One reader also discarded an existing positive SET_NULL receipt;
`2665e634f42` repairs that loss without granting readability or initialization.
The next bootstrap exposes other legitimately vacant homes without physical
NULL. Both failed build logs and diagnostic traces remain in E59.

The reviewed correction is to distinguish fresh-result publication from legacy
in-place window publication. A fresh scratch destination can overwrite its old
bits after positive NO_HOME/no-owner admission; it must not read or interpret
them first. Existing normalization, post-publication checks, local replacement,
discard and input detachment remain. This needs a separate checked publication
macro and fixture updates, not invented physical NULL facts or additional
clearing stores. Candidate `d4584f8e45abeafaf0e919111e7fb5b86d7ab7a9`
implements that correction, passes full generation, both 117/79 builds and
207 methods per role across ten focused modules. The exact small `two_empty`
witness drops from two scratch cells to one in both roles; single-empty and
nonempty controls retain their expected extents. Its eight-case performance
screen is running. No selection or upstream result is claimed yet.
