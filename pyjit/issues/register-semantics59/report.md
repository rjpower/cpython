# Semantic IR trial results

Selected57 remains selected. The first loop-binding executable, standalone
builders and builder+Invoke composition all lose their performance screens.
Invoke is being isolated from builders; continuation-action native gates follow. These are incremental
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

## Independent builders: qualified, not selected

Builder source packet `1b2298676389a00aa43d3875e18962c678e3d516` is preserved
on `pyjit/reg59-edge-results`, before generated products or native qualification.
Its candidate retains the result-binding mechanism needed by BUILD_LIST and
BUILD_TUPLE, without enabling the rejected FOR route.

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
nonempty controls retain their expected extents. The native list/tuple handlers
grow from 649/633 bytes to 762 bytes each. The completed eight-case screen has
stable identities and the following elapsed ratios:

| Workload | Builder / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .982348 | .959443–1.005799 |
| deltablue | 1.033512 | 1.030720–1.036311 |
| coroutines | .982970 | .975770–.990224 |
| generators | 1.005336 | 1.001136–1.009553 |
| logging_silent | 1.088743 | 1.079459–1.098106 |
| raytrace | .987281 | .986628–.987934 |
| spectral_norm | .969978 | .953958–.986268 |
| nqueens | 1.012138 | 1.007991–1.016303 |

The standalone builder executable is **not selected**: logging and DeltaBlue
lose in both pairs. The screen supports neither an upstream claim nor a general
attribution of the losses to one source operation. Root is testing the reviewed
composition with native Invoke before deciding which component survives.
This does not promote a losing executable to golden. Evidence:
`builders2-eight-selected57`, `builders2-native`, `builders2-small-*` and
`reg59-builders2-{regen,build,focus}-commands.json` in E59. There is no fresh
full-system or upstream campaign for this standalone candidate.

## Native Invoke and remaining trials

The frozen Invoke source packet is `ff3d5b23171007c8c195a12ce8b7475d3bca8eee`
on `pyjit/reg59-native-invoke`. Root has composed it with builders in G on
`pyjit/reg59-builder-invoke`. Full generation caught a missing callback-capable
release classification; registering the actual retirement marker repairs it.
IR metadata remains byte-identical to the builder candidate. Explicit tier1
annotation states that this marker currently has an interpreter lowerer only;
no tier2/JIT support is implied. Both peers approve B0 owner replacement without
an added take-to-NULL store. Final composition
`582e61c2b28445cadbf219df23f708d6e5988e40` passes generation, both 117/79 builds
and 66 focused methods per role across eight modules. The first native NO_SELF
fixture accidentally selected CLASS; the corrected fixture uses an actual
unbound `dict.get` provider and retains the required NO_SELF assertion. The
sole-result-owner fixture also removes unrelated surviving owners.

The [actual post-review](invoke-post-review.md) is GO. Native inspection finds
one descending take/close traversal instead of an additional NULL-only loop;
the NO_SELF handler no longer calls `memset`. B0 replacement adds no NULL store
or helper hop. FAST shrinks 1,697→1,553 bytes and its native stack frame
64→48 bytes; NO_SELF grows 1,446→1,473 bytes with its 48-byte frame unchanged.
The generic provider, normalization, periodic check and final recipient commit
remain. The composition's stable-identity screen is:

| Workload | Builder+Invoke / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 1.011116 | .984617–1.038328 |
| deltablue | 1.058183 | 1.055160–1.061216 |
| coroutines | 1.031120 | 1.029217–1.033027 |
| generators | 1.003613 | .995717–1.011572 |
| logging_silent | 1.128347 | 1.126770–1.129927 |
| raytrace | 1.012511 | 1.007048–1.018004 |
| spectral_norm | 1.111650 | 1.104577–1.118767 |
| nqueens | 1.022513 | 1.016132–1.028935 |

Reject this composition. Reduced handler work is real but does not establish
a benchmark win. Root will test Invoke independently so the rejected builder
component does not determine Invoke's disposition. Evidence is E59
`combined3-eight-selected57`, `combined3-native`,
`reg59-combined3-{build,focus}-commands.json` and the retained selected57
FAST/NO_SELF disassembly. No full-system/upstream campaign is claimed here.

Continuation source `7b295bf6caf74a03414f9995f0f6e4523223d49b` is integrated
in G with generated products at `c1da955989e`. Generation and both 117/79
builds pass. The first release focus runs 112 methods: only the new fixture
fails, in four local-route subtests. Its simple `incoming = yield ...` form
actually has scratch YIELD A followed by a local store; it does not exercise
the claimed local recipient. The worker is preparing a genuine local witness,
retaining the exact local assertion. The failed logs remain in E59
`reg59-recipient1-release-focus.log` and
`reg59-recipient1-diagnose-release-focus.log`; debug focus has not yet run.
This is not a native qualification pass or performance result. Edge-allocation trial 5 is demoted without a concrete
new consumer beyond the closed zero-COPY census. Trial 6's
[resident-value proposal](resident-value-proposal.md) requires current all-word
coverage and a nonoverlapping native budget before implementation. Its census
adapter has actual source-peer GO. All eight serialized root diagnostics pass
with exact before/after identities; `resident-allword-commands.json` retains the
commands and the case-specific `resident-selected57-*.json` files retain every
word. Frequencies weight pre-monitor warmed labels, not natural guard-hit
counts, because monitoring deoptimizes many entries. No proposal or small pilot
is a parity result; the broader cycle remains active.
