# Semantic IR trial results

Round67 source implementation is active under [design67](../../design/proposals/interpreter-parity67.md)
and [review67](review67.md), after independent adoption and shared-interface GO.
A is the complete native callout/result consumer; B is one caller-anchored
exact-method self. Both start selected57 and will be integrated independently.
Neither has a runtime qualification or performance result.

Round66's final design by Astra (`945afac1e2546555da9c41cda708952af254db73`)
was integrated at `fd42900d565`. Frozen runtime `64348dd5c182af27c058e60f1e3f849f00d7ef81`
qualifies without source changes and completes the unchanged hot8 screen, but
selection rejects it. Unit2 stops; selected57 remains unchanged. Full
qualification, screen and diagnostic evidence follows below.

Round65 is closed with no selected runtime. A passes source, runtime and native
qualification but loses its standalone hot8 screen: coroutine time falls 39.3%,
while logging loses 16.2% and generators 5.5% in both pairs. B passes source,
products and focused runtime checks, then fails its native cap, including after
one reviewed correction. B was not timed in round65. Selected57 remains
unchanged. The [design65](../../design/proposals/interpreter-parity65.md) and
[review65](review65.md) retain the contracts. Review66 reassesses architecture
and admission gates without inventing a performance loss for unmeasured B.
No cause of A's broad regressions is established. No larger campaign follows
the round65 decisions. Earlier attempts and sealed evidence remain.

Selected57 remains selected. The first loop-binding executable, standalone
builders and builder+Invoke composition all lose their performance screens.
Continuation encoding and isolated Invoke also lose. All five first-cycle
executable screens are closed; golden is unchanged. The round60 engine also
loses every screen. [Round61](../../design/proposals/interpreter-parity61.md)
tests producer/role contracts and complete creation/delegation transitions. These are incremental
results, not a completed parity cycle. All four round62 candidates and both
round63 fixed-role trials are rejected. The compact64 executor stops at its
native net-work gate; complete deferred await qualifies and loses its screen
under the [round64 design](../../design/proposals/interpreter-parity64.md) and
[review](review64.md). See the [workstream](README.md) and
[ordered design](../../design/proposals/register-semantics59.md).

## Round66 frozen continuation: qualified, screen rejected

Astra's final design `945afac1e2546555da9c41cda708952af254db73` is integrated
at `fd42900d565`; independent grouped review approved it. Frozen runtime
`64348dd5c182af27c058e60f1e3f849f00d7ef81` receives no source, generated-product
or build changes. Both 117/79 builds and fresh 48-method runs per role are
reused by exact identities. The independent
`continuation66-source-host/unit1-final-qualification-peer.md` confirms actual
natural return, terminal suspension, armed error and armed normal controls.

The stable 32-row hot8 screen completes with no errors. It rejects selection:
generator mean 1.0144569295 exceeds the 1.01 gate, and logging loses in both
pairs (1.0731164899, 1.0798281655). The geometric mean is 0.9368713684.

| Workload | Frozen B / selected57 |
| --- | ---: |
| comprehensions | 0.9747244492 |
| deltablue | 0.9791755373 |
| coroutines | 0.6260262279 |
| generators | 1.0144569295 |
| logging_silent | 1.0764670969 |
| raytrace | 0.9553110279 |
| spectral_norm | 0.9697604717 |
| nqueens | 0.9818861826 |

Root's decision and independent screen review are
`continuation66-source-host/root-screen-decision.json` and
`continuation66-source-host/unit1-screen-peer.md`; timing rows are in
`E59/parity66-continuation-eight-selected57`. No confirmation, all29, fresh
upstream comparison or full-system campaign followed. Do not multiply these
selected57 ratios by an older upstream result.

The finite logging FIFO PMU diagnostic completes at
`E59/parity66-logging-diagnostic`. It uses eight stable intervals: three paired
stat-counter samples and one cycles sample per role, with the same hot workload
and 60,000 warmup iterations. Candidate/selected57 instruction ratio is
0.992588295967 (-0.74%); cycles ratio is 1.085426304460 (+8.54%). Both roles
report 100% running, with 2,421 golden and 2,510 candidate samples and no lost
or throttled samples. Independent
`continuation66-source-host/logging-diagnostic-receipts-peer.md` gives GO.
These counters reject an equal-CPI, proportional-instruction-count explanation
for the full logging loss only. They establish no frontend, cache or other
mechanism. Offline first-IP physical-leaf attribution matches all 2,421/2,510
samples and weights each period once. Changed terminals account for 8.31%/8.45%
of self-periods (all RETURN_VALUE_EMPTY); proved-equal ordinary clearing accounts
for 13.42%/13.52%. Most remaining samples are ordinary attribute, Python-call,
branch and dictionary work, whose native equality is not assumed. These separate
profiles cannot allocate the 8.54% cycle gap to individual functions or establish
a cause. The script, hashes, disjoint categories and complete symbol table are
`continuation66-source-host/logging-leaf-attribution.{py,json,md}`. No further
profiling or source repair follows this diagnostic. The controller collection
fix changes no runtime.

Unit2 stops because the non-generator logging gate fails; its union proposal
remains unimplemented. The broader native-callout/result-consumer ABI is the
leading deferred option for the next grouped architecture review. Unit2 has no
runtime implementation authorization.

## Round65 SEND-yield continuation: runtime checks pass, native reject

Initial trial `980baf6bbf3aa1d8182c35f8c7166c662c4ad1f0` preserves semantic
SEND words and moves private suspension preparation into one dispatch-only
continuation, ID385. Ordinary YIELD no longer classifies its parent or prepares
private suspension. Cold logical-PC readers, writers, error completion and
copy/detach paths resolve or cancel the executable target. The header remains
112 B; the private record grows from 72 to 80 B. This is an unselected trial.

Source `8373e9d5fd61` integrates at `e7f61a81b69`; root-generated products are
`09124a5c05a`. Independent full-source review first rejects a missing pinned-
number checker consumer; the shared explicit target reservation repairs it,
retaining disabled-source and unknown-entry rejection. An unused emitted
selector is removed. Full regeneration passes. Independent product review finds
308 old handlers byte-equal, fourteen debug-assert-only changes and eight
intended SEND/YIELD release changes. All 76 ownership entries and existing
semantic format rows are byte-equal. Three affected source consumers pass.

Both `parity65-continuation3` builds pass 117/79. The initial focused runs pass
47 of 48 methods per role; the new mortal-input fixture retains position rows
while wrongly emptying its semantic recovery table. Explicitly supplying the
original matching table repairs public construction. The next isolated run
then correctly rejects the unversioned FunctionType leaf before private
nomination. The fixture now uses the existing MAKE_FUNCTION factory pattern;
its private-head, callback-count and real-SEND assertions remain intact.
`parity65-continuation3-input` passes the corrected method in both roles,
completing the 48-method set. Runtime code/products never changed during these
fixture corrections. Initial failures and source/peer evidence remain under
`boundary65-source-host` and the corresponding continuation1/2 logs.

The native-alias test passes in both original focused runs: a real C pointer
survives foreign observation of an unobserved armed private frame, followed by
actual yield, identity detach and completion. `root-b-capi-modules.json` pins
the shared modules. The static capture is **`parity65-continuation2/python`**,
SHA256 `99ffb00af89bcc4cc0701cd6971d7128eaeb2cc4a01aa7a1e4ea67f61bc46d38`.
The fixture-only refreshed binary has byte-identical .text and unchanged C API
modules, documented by `root-b-fixture-native-text-equivalence.json`.

Independent native review gives **NO-GO**. Complete EMPTY private return costs
158+K instructions versus await9's 153+K and A's 152+K; K is the same actual
private clear path. Warm reservation remains 44 instructions. The changed
consumer is generic completion's unarmed WAIT path, 93→99: a target comparison
and additional decode work, without new spills, calls or owner moves. Text
adds 1,600 B and rodata 96 B against await9, within their caps. Remaining native
gates stop unqualified at this failure. Preserve
`pyjit/parity65-continuation-initial` at this head.

The one bounded correction passes independent proposal and actual-source
review. Source `50adc6d3927` integrates as final
`64348dd5c182af27c058e60f1e3f849f00d7ef81`. Each existing retirement branch
selects the live semantic continuation after clear callbacks: AWAIT uses its
disarmed proof; GENERATOR retains full Take/cancel. The single shared completion
consumer takes that continuation explicitly. ReturnResumable's interface and
the terminal recipes stay unchanged. No mode, header or alternate engine is
added. Full regeneration changes no products; both continuation4 builds pass
117/79 and all 48 focused methods pass afresh in each role.

Final `parity65-continuation4-native/python` SHA256 is
`79bd05e06af44d730c343d8e34a41ae1538041e4f1e27655bd853e587c7d9fc4`.
The common completion returns to 93 instructions, but fourth-argument transport
adds five instructions in ReturnResumable against A. Complete uninstrumented
EMPTY costs 157+K versus await9's 153+K, reaching the four-instruction cap.
INSTRUMENTED EMPTY costs **171+K+M versus 166+K+M** in both await9 and A,
exceeding that same cap. M is the unchanged actual observer interior. The final
review gives **NO-GO**; no further source correction or performance run follows.
This is a native-contract rejection, not a measured throughput regression.

The checked ordinary SEND_GEN→YIELD_VALUE→exit cycle does shrink: 157 instructions
versus selected57's 160 and await9's 176. Generic SEND's direct-generator cycle
is 185 versus 187 and 201. All four YIELD forms have no private-parent checks
and no added local stack slots in the inspected listings. Text adds 1,664 B
and rodata 96 B against await9, within their caps. Complete cleanup/reentry
coverage, first-suspension and aggregate resolver budgets remain unqualified
after the mandatory stop. Smaller ordinary paths do not supply a B speedup.

`unit-b-native-cold-review.md`/`native-b65` preserve the initial failure;
`unit-b-corrected-native-cold-review.md`/`native-b65-corrected` preserve the final
counts and residual gates. G remains on `pyjit/parity65-continuation-integrated`
at the final head; the initial branch is retained separately. No GDB transaction
probes, broader runtime campaign or timing ran for B. A grouped review will
consider both the architecture and whether hot and observation-path gates
reflect the performance goal. Selected57 remains unchanged.

## Round65 return/accessor boundary: qualified mechanism, rejected executable

Final integration `a6695887e949c3019216120c544e171dfa7ff90e` changes existing
complete RETURN recipes to a THREAD branch and one owned-result resumable
helper. State-proved embedded accesses avoid selecting an active view. The
three retirement APIs consume the reached residence proof, retaining the
existing clear engines, profiling update and live parent after callbacks.
No new return opcode, classification pass or frame field is introduced.

Independent source/generated reviews pass. Both `parity65-residence3` builds
pass 117/79, all 46 focused methods per role including sixteen await lifecycle
methods and public generator construction from ordinary code, and 708 runtime/
reader controls per role (thirteen release and twelve debug skips). All twenty
bounded source methods and three affected generated-case consumers pass.
These are focused integration checks, not a fresh full-system qualification.

The first generated draft was rejected before compilation because a missing
nonescaping declaration restaged YIELD's taken owner. Source `04e0b8a6c2c`
repairs the declaration and adds an actual-emission regression. Final generation
changes only four RETURN bodies and getter names in four YIELD bodies; metadata
and other products are unchanged. The first compiled integration `ee0e7704659`
then fails the native gate: EMPTY retirement still rediscovered residence in a
generic helper, adding three instructions. Preserve its source branch
`pyjit/parity65-residence-initial`, captured ELF and rejected path evidence.

The independently peered four-file correction `8ed034598b3` narrows the existing
API contracts, adds a wrong-residence mutation negative and updates three
missed generator-source fixtures. Root integrates it as `a6695887e94`;
regeneration changes no products. The corrected cold native review passes:
complete EMPTY scratch/local/discard/nonfinal paths save five instructions,
GENERAL saves seven, and redirect/rooted reentry variants also shrink. Ordinary
THREAD stack does not grow; generic CALL normalizes equal and exact/general/
binder paths retain their bounds. Admitted generator entry has zero active-view
selections with 32 B local stack. Complete ordinary generator-return wrappers
cost 64+G versus await9's 79+G GENERAL/75+G EMPTY, where G is the identical actual
clear/completion interior. Text adds 1,664 B versus await9, within the 2 KiB cap.
Private-await clearing adds 32 B simultaneous helper stack versus await9; that
cost is retained in the ledger. These static facts establish no dynamic
frequency or cycle explanation.

`parity65-residence3-native/manifest.json` pins ELF SHA256
`314e32ec75c112095a16deb5ffeede5913b8991760cb312ef432c9c08d057a73`.
The independent final report and path evidence are under
`boundary65-source-host/unit-a-corrected-native-cold-review.md` and
`native-cold65-corrected`. Root's fresh `retirement65-native-controls/natural1`
receipts link an actual private child to its identity-free NORMAL_RETURN and
completed retirement on this exact binary. Independent ABI and receipt peers
pass. This is one transaction, not new suspension/OOM/frequency evidence.

The balanced, identity-stable 32-row hot8 screen is
`parity65-residence3-eight-selected57`; no failure or postflight error occurs.
Ratios are elapsed-time geometric means, below one is faster:

| Workload | Candidate / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.972740 | 0.951365–0.994596 |
| deltablue | 0.989654 | 0.984450–0.994885 |
| coroutines | 0.606811 | 0.604728–0.608901 |
| generators | 1.055271 | 1.051395–1.059162 |
| logging_silent | 1.161661 | 1.161023–1.162298 |
| raytrace | 0.980528 | 0.979084–0.981975 |
| spectral_norm | 0.991837 | 0.989236–0.994445 |
| nqueens | 0.999876 | 0.997984–1.001771 |

Root rejects the standalone executable: logging and generators lose both pairs.
No confirmation/all29/direct-upstream/PMU/full-system campaign follows. Preserve
`pyjit/parity65-residence-integrated` at the final head. No new upstream result
is inferred by multiplying historical ratios. The optional B continuation is a
separate reviewed attempt, not evidence that its proposed mechanism caused or
will repair these regressions. P and G developed that attempt from the qualified
A head; its final native rejection and unqualified residual gates are recorded
above.

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

### Isolated Invoke: reject

Candidate `5c60e9664faa87c7f6cf1909609554755d83d5ee` is preserved on
`pyjit/reg59-invoke-only`. It contains source packets ff3+6b978 and three
generated products, without any builder/recipient change. Generation, both
117/79 builds and **66 focused methods per role** pass. Actual source and
native review is GO; handler sizes remain 1,553/1,473 bytes and both native
stack frames are 48 bytes. The repeated clear and NO_SELF memset are absent.
The generic value provider, normalization and delayed recipient commit remain.

| Workload | Invoke / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.968664 | 0.949679–0.988028 |
| deltablue | 1.008872 | 1.008846–1.008898 |
| coroutines | 1.081475 | 1.076103–1.086874 |
| generators | 0.992578 | 0.991518–0.993638 |
| logging_silent | 1.125967 | 1.106542–1.145733 |
| raytrace | 0.992047 | 0.989399–0.994702 |
| spectral_norm | 1.141652 | 1.111479–1.172645 |
| nqueens | 1.006120 | 1.004160–1.008085 |

Reject the standalone executable: coroutine, logging and spectral_norm losses
repeat in both pairs despite comprehension improvement. This isolates Invoke
from the losing builder component; it does not establish a cause for unrelated
workload regressions or a cache/layout explanation. Evidence:
`invoke-only1-eight-selected57` with stable identities, `invoke-only1-native`
and `reg59-invoke-only1-{regen,build,focus}-commands.json`. No new full-system
or upstream campaign is claimed for this loser.

## Continuation recipient encoding: reject

Continuation source `7b295bf6caf74a03414f9995f0f6e4523223d49b` is integrated
with products and corrected fixtures at
`14bf4eb1784a8312fd2a6d89f2f63d191547bfe5`, preserved on
`pyjit/reg59-recipient`. Generation, both 117/79 builds and **112 focused methods
per role** pass. Three remote metadata/corruption/delegated-discard methods
also pass per role on the same production implementation at c1da.

The first release focus failed only the new local witness: ordinary lowering
yields to scratch then STORE_FAST. The corrected witness constructs a valid
direct-local YIELD, preserves RESUME, jumps over the later store and exercises
all four public image routes with strict LOCAL and sent-object identity checks.
Independent peer caught and corrected an encoder-name shadow before the final
gate. Both the failed fixture/setup logs and final passing gates remain in E59.

The [source/native post-review](recipient-post-review.md) is GO. It charges
generic call binding growing from three to twelve instructions, including a
caller code/nlocals query. Publication removes the old partition lookup but
adds action decoding: scratch takes three action branches and local four.
Checked SEND binds its already-proved scratch action without that query.
The four sampled handlers grow by 85 bytes in total; stack reservations are
unchanged. Native mechanism evidence is not selection evidence.

| Workload | Recipient / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.972637 | 0.948980–0.996885 |
| deltablue | 1.154853 | 1.149621–1.160109 |
| coroutines | 0.996981 | 0.996942–0.997020 |
| generators | 1.054556 | 1.049383–1.059755 |
| logging_silent | 1.115302 | 1.111971–1.118644 |
| raytrace | 1.004561 | 0.994257–1.014971 |
| spectral_norm | 1.101446 | 1.080222–1.123088 |
| nqueens | 1.019988 | 1.018720–1.021258 |

Reject this executable: DeltaBlue, generator, logging and spectral_norm losses
repeat across both pairs. Do not rescue it with another tag/layout search.
Evidence: `recipient3-eight-selected57` (stable identities), `recipient3-native`,
`recipient1-native-review`, `reg59-recipient3-{build,focus}-commands.json` and
`reg59-recipient1-remote-focus-commands.json`. No fresh full-system/upstream
campaign was run for this rejected increment.

## Remaining coverage and next grouped review

Edge-allocation trial 5 is demoted without a new real consumer beyond the
closed zero-COPY census. All eight root all-word diagnostics complete with
stable identities; `resident-allword-commands.json` and case-specific
`resident-selected57-*.json` retain the commands and every word. The
[coverage ledger](resident-coverage.md) weights pre-monitor adaptive labels,
not natural guard hits: monitoring deoptimizes many entries. Its finite motifs
cover at most 1.58% of comprehension and .88% of DeltaBlue semantic dispatches,
and none for coroutine/generator/logging. These are pilot packing ceilings,
not bounds for every possible region. Their same-line restriction is
conservative, not a load-bearing debug requirement.

The [resident/backend proposal](resident-value-proposal.md) and independent
[backend peer](backend-peer.md) support a broader same-GCC engine experiment.
Measured upstream uses computed goto; selected57 uses separate tail-called
handlers with path-specific state repacking. Astra's [next grouped review](review-next.md) and
[ordered design](../../design/proposals/interpreter-parity60.md) are complete.
Root admits a same-GCC resident-engine trial, then the concrete cross-word
inline-scalar consumer. Root composes the engine and identity/config packets at ec230e55641. Generation
and the official-container configure regeneration pass. Only the target table,
UNKNOWN block and generated configure change; semantic metadata is unchanged.
Both fresh configurations match all compared compiler/core/module flags and
byte-identical selected57 pyconfig.h. Both builds complete 117 modules/79 shared
extensions. Each role passes 465 methods across 17 focused modules. The balanced
hot-eight screen rejects the candidate in every workload and every pair:

| Workload | Engine / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 1.218648 | 1.175412–1.263475 |
| deltablue | 1.338757 | 1.333543–1.343992 |
| coroutines | 1.185821 | 1.172873–1.198913 |
| generators | 1.438227 | 1.425445–1.451124 |
| logging_silent | 1.599627 | 1.598617–1.600637 |
| raytrace | 1.300208 | 1.297438–1.302985 |
| spectral_norm | 1.291620 | 1.286112–1.297151 |
| nqueens | 1.302795 | 1.297197–1.308416 |

`E59/parity60-engine1-eight-selected57` records stable identities and successful
pre/post checks. No all29, full-system or direct upstream rerun is justified for
this broad loser. Preserve `pyjit/parity60-integrated` at ec230e55641 and its
builds; the next scalar trial starts from unchanged selected57.

The [native post-review](engine-post-review.md) resolves all 367 table rows
inside `run_handlers`. Actual machine code has stack-backed engine state,
a 4,472-byte stack reservation and one shared indirect dispatch after GCC
tail merging. Candidate text grows 55,424 bytes. SEND also gains an outlined
return-publication helper. These concrete costs defeat the intended simple
state-residency argument; they do not quantify the cause of each elapsed loss.
No flag, compiler or alignment search follows. Setup is retained in E59
`parity60-configure-commands.json` and `configure-parity60.py`.
The four shared tool consumers and bounded ELF target mapper have actual source
peer GO; six host mocked/synthetic methods pass. Unsupported ELF/relocation or
outside-engine targets fail; no stale tail-call object check is reported as an
engine pass.
The [scalar implementation proposal](scalar-proposal.md) also rejects the
allocated descriptor framework before coding: per-code records, reverse index,
generation and mutation hooks are disproportionate for two operations. A fixed
two-word rival avoids those fields but still needs a real common admission
proof and shared arithmetic/commit receipts. Root defers it behind the broader
producer-fact and register-role work; no scalar implementation or native saving
is claimed. No proposal or small pilot is a parity result; the cycle remains active.

## Round61 common-constant producer: not selected standalone

Source packet 57b2411b1f4 and runtime fixture 4a80ea6b1ff are integrated with
generated products at `8bb861d357eaa8349a0fd89c0ef11df5dd72818d`, retained as
`pyjit/parity61-producer-only`. Its checked getter reads the initialized
interpreter's canonical inline/immortal table without acquiring a count. The
shared typed result receipt selects existing canonical publication. There is
no new opcode, frame field or descriptor. Destination classification and
old-local retirement remain generic. Actual source peers approve the packet;
four focused host source methods pass.

Full generation passes. Only LOAD_COMMON_CONSTANT's handler and resolved owner
program change; the other 330 populated format rows retain their contracts.
Both builds complete 117 modules/79 shared extensions, with compiler/core/module
flags and byte-identical pyconfig.h matched to selected57. All 43 methods across
five focused runtime modules pass per role, including all 14 constants in
scratch/local/discard recipients, four image routes, finalizer reentry,
malformed indices, scalar ownership and subinterpreter initialization/clear.

Native capture `E59/parity61-constant1-native` has ELF SHA256
`28326a4e7c99114d090d7ea8d849a998fae3b276f3097d03546ec203eab063c8`.
The actual handler shrinks 620→356 bytes: new-result DUP/count acquisition and
numeric normalization disappear, with no getter call. Generic destination and
old-owner handling remain. The balanced two-pair screen has stable identities:

| Workload | Producer / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .969376 | .944770–.994623 |
| deltablue | 1.036249 | 1.026599–1.045990 |
| coroutines | .979717 | .972948–.986533 |
| generators | .984184 | .982033–.986340 |
| logging_silent | 1.139183 | 1.137531–1.140837 |
| raytrace | .989650 | .987821–.991483 |
| spectral_norm | 1.015950 | 1.015859–1.016041 |
| nqueens | .995691 | .986902–1.004559 |

The standalone producer is **not selected**: logging and DeltaBlue lose in
both pairs. The separate TEMP refinement completes the producer/destination
contract and will be screened independently as a composition. Creation and
ascent start from selected57, without this losing prerequisite. No all29,
full-system or direct upstream campaign is claimed for the producer-only packet.
Timing evidence: `E59/parity61-constant1-eight-selected57`. Build, generation
and focused manifests use the same `parity61-constant1` tag; setup is retained
in `prepare-parity61-builds.py` and `parity61-build-setup-commands.json`.

A short matched PMU diagnostic distinguishes removed work from elapsed cost:

| Workload | Instructions / selected57 | Cycles / selected57 |
| --- | ---: | ---: |
| deltablue | .991916 | 1.038745 |
| logging_silent | .986431 | 1.119502 |

These are two balanced pairs of **whole-process** user counters, including
imports, one warmup and seven unchanged timed kernel calls. They are not the
earlier kernel-only interval and must not be combined with its per-tray counts.
Both instruction pairs decrease while cycles increase. This rules out a net
whole-process instruction-count increase as the sole explanation; it does not
establish branch, cache, layout, scheduling or individual-handler attribution.
The diagnostic retains commands, raw counter CSVs, benchmark-source and ELF
identities in `E59/parity61-constant1-counter-diagnostic`, driven by
`E59/diagnose-constant1-counters.py`. No compiler/flag/alignment search follows.

## TEMP refinement: qualification in progress

Source packet `8db82c67bd4` adds one executing-only scratch-destination variant,
cold selection before the no-feedback return, public-image rejection and base/
shadow/instrumentation mappings. Actual source peers approve it and six focused
host methods pass. Root generation and both 117/79 builds pass at fe38bad0b50.
The first six-module release runtime check runs 153 methods and fails one new
scratch-owner admission case; the other five modules pass. Debug has not run.

The image LOAD_CONST A0 → LOAD_COMMON_CONSTANT A0 → RETURN A0 is rejected by
selected57 but admitted by the candidate. Root's constructor-only comparison
is retained in `E59/constant2-vacancy-baseline.json`; no malformed image was
executed. The source-derived normal graph must preserve the existing recipient
rule when replacing the old FRESH summary. The worker is reviewing that common
consumer before a correction and source peer. This failed revision was not
timed. Failed evidence remains
`E59/parity61-constant2-release-focus.log` and its command manifest.

### Corrected TEMP composition: rejected

Peer-reviewed cold replay repair `65c99464e0b` retains a proved scratch-owner
collision through descendants and joins and diagnoses it only at the selected
normal endpoint. It preserves the existing same-owner stamp proof and leaves
unreadable prefixes uncovered. Public constructor, replace and marshal routes
reject the malformed image. The correction adds no execution field or hot work.
Root integrates it at `53e9a57cfa25665c503010e7a49a557b096d4287`.

Both corrected builds complete 117 modules/79 shared extensions. All 161 methods
in seven focused modules pass per role, including test_ir_error_checkpoints'
actual repeated SAME_VALUE publication. Native ELF SHA256 is
`c0ea36348c034313fc4995bba1f885d3364fe6739748f567e75f765db0401f54`.
The real TEMP handler is 69 bytes, ordinary handler 356 bytes. The TEMP body
directly reads the canonical table, stores the result and dispatches; no owner,
numeric-normalization, destination or finalizer branches survive.

The stable-identity balanced screen still rejects the composition: logging and
spectral_norm regress in both pairs. Large opposing outliers elsewhere prevent
claiming their aggregate ratios as reliable gains. No confirmation, all29,
full-system or direct upstream campaign follows this loser. Raw evidence is
`E59/parity61-constant3-eight-selected57`, with matching build/focus manifests
and `parity61-constant3-native`. Creation and ascent retain clean selected57 bases.

| Workload | TEMP composition / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 1.060650 | .968009–1.162158 |
| deltablue | .850720 | .698020–1.036826 |
| coroutines | .875078 | .864887–.885390 |
| generators | .969242 | .955817–.982856 |
| logging_silent | 1.144407 | 1.135283–1.153604 |
| raytrace | .980502 | .975321–.985710 |
| spectral_norm | 1.058099 | 1.058028–1.058169 |
| nqueens | 1.112951 | .994279–1.245787 |

## Checked delegated ascent: not selected

Source `86886b6f846` and fixture correction `53df1c14deb` are integrated with
generated products at `410bea361909d37aa4ae998a5084648f117b7200`, retained on
`pyjit/parity61-ascent-integrated`. Root source post-review approves the bounded
loop. Ownership, error and phase metadata are byte-identical to selected57;
only the delegated handler product changes. The fast loop is GIL release
without DTrace; debug and instrumented handlers retain ordinary execution.

Both builds complete 117 modules/79 shared extensions. Focused coverage is
397 methods across eight suspension, observer and lifecycle modules per role.
The initial release run's only failures were two negative subcases in the new
fixture: omitted semantic recovery caused an earlier constructor rejection.
The fixture now supplies its original recovery table and all nine methods
pass on the targeted release retry; the seven other modules' production code
is unchanged. Debug passes all 397 in one run. This is focused qualification,
not a full-system result.

Separate GDB probes establish actual native composition with stable pre/post
source/ELF identities. Depth8 has 68 entries of eight real suspensions;
depth40 has 136 entries of sixteen and 68 of eight. Profiling has zero ordinary
entries and 544 instrumented entries. A custom iterator splits exact groups,
with 136 entries of eight. Workload identity, send, completion and exception
checks pass. Optional reject-site counters incorrectly count guard visits due
to GDB Python condition handling; they are explicitly invalid as rejection
evidence. The independent entry/fold/publication histograms are unaffected.
Raw logs and executed scripts remain in `E59/parity61-ascent-diagnostic`.

Native ELF SHA256 is
`1c1ec948c9bc5fc727c295877c2332442a1eb3b7085ffb90bd423acc555f162b`.
Ordinary YIELD_DELEGATED grows 556→1,076 bytes and its stack reservation grows
16→32 bytes; SEND_GEN and the instrumented twin remain unchanged. The admitted
continue edge removes intermediate publication, dispatch and result take/clear.
Its guard chain nevertheless stages operands through four stack stores and
six reloads, besides type, PC, word, identity and vacancy branches. This is a
concrete added cost, not exclusive attribution of every elapsed change.

The stable two-pair screen rejects this version: generators regress in both
pairs despite coroutine gains. Do not compose this unselected dependency with
creation or spend full diagnostics on it. No all29, direct-upstream or full
system rerun is claimed. Evidence uses `parity61-ascent{1,2}` build/focus tags,
`parity61-ascent2-native` and `parity61-ascent2-eight-selected57`.

| Workload | Ascent / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .979626 | .963690–.995826 |
| deltablue | .999177 | .994097–1.004284 |
| coroutines | .951782 | .950362–.953205 |
| generators | 1.078980 | 1.072055–1.085949 |
| logging_silent | 1.004998 | 1.002926–1.007074 |
| raytrace | 1.010922 | 1.010803–1.011041 |
| spectral_norm | .955606 | .955289–.955922 |
| nqueens | .990123 | .986422–.993837 |

## Direct final-frame creation: rejected

Frozen source `b10667203fee0e2ef31ad366e201b9eb678564b1` has root and independent
actual-diff approval. Root integrates the source and five generated products at
`f8ecd806cdbcfb34e76ad8fab533bf966b785330`, from clean selected57. The two new
learned-only CALL handlers reuse the existing-size feedback payload and shared
frame initializer/return writer. Ordinary generated bodies are unchanged.
Both native builds complete 117 modules/79 shared extensions; three focused
host source methods pass. Final fixture-only corrections bring the integrated
head to `4ac529577e2f16bcc4fae76e30c7bd61d04216cb`.

The first release runtime module runs ten methods and fails three assertions.
Two expect creator nomination after a prewarm function mutation; a read-only
probe shows these functions no longer have a usable specialization version.
The corrected nomination controls use pristine MAKE_FUNCTION targets. The
third expects immediate weakref death after a callback writes
through f_locals. That setter deliberately retains the old binding in
f_overwritten_fast_locals to protect borrowed local reads. The probe confirms
that edge and eventual release after closing the parent and releasing its
frame. Correct the fixture's lifetime claim; the source result-root handoff
is a separate ownership proof. A clean gc_verify checks reachability, not
exact owning counts.

Evidence is `E59/parity61-created1-release-focus.log` and
`E59/created-generator-diagnostic/probe.log`. No production repair was needed
for those assertions. The initial native capture stopped at a compiler-renamed
helper; `parity61-created1-native2` retains the successful capture using the
actual `.constprop.0` symbol. Neither failure is hidden or counted as a pass.

The allocation-callback fixture exposed a design error: the selected GIL
allocator/reftracer contract explicitly forbids running Python or changing
exceptions. GC allocation links the object and schedules collection through
the eval breaker; it does not collect. Thus no GC callback can change function
code, origin tracking or caller locals during this admitted allocation-only
interval. The fixture now collects and mutates **after** complete creation;
`gc_verify` runs after collection returns. Publication/redirect cleanup and
unknown-owner release remain real callback boundaries. The
[allocation62 proposal](creation-allocation62.md) removes staging based on this
reached-path proof, rather than weakening ownership at actual callbacks.

All ten new runtime methods now pass in release and debug. Eleven existing
call/completion/suspension/observer/GC modules pass 515 methods per role, giving
525 focused methods per role in total. Build/focus records use
`parity61-created4` and `parity61-created4-protocol`. No full-system run is
claimed. Final native capture is `E59/parity61-created4-native`, ELF SHA256
`e03950221a03b6b875ae05b5d6a6c6b9616f1a93f7605f4a2be135a842b833ec`.

The stable two-pair screen rejects this packet. Logging does not execute the
new constructor, so its regression cannot be assigned to argument staging.
There is no confirmation, all29 or direct upstream campaign for this loser.
Evidence: `E59/parity61-created4-eight-selected57`.

| Workload | Creation / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .960055 | .938534–.982070 |
| deltablue | 1.088650 | 1.078661–1.098731 |
| coroutines | .964122 | .955505–.972817 |
| generators | .999804 | .997300–1.002314 |
| logging_silent | 1.301688 | 1.300362–1.303015 |
| raytrace | .994576 | .989606–.999572 |
| spectral_norm | .961952 | .961006–.962899 |
| nqueens | 1.018826 | 1.013089–1.024596 |

A short matched diagnostic uses two balanced pairs of whole-process user PMU
counts: imports, warmup and seven unchanged timed kernel calls. Source, workload
and ELF identities remain stable. This is not the kernel-only upstream interval.

| Workload | Instructions / selected57 | Cycles / selected57 |
| --- | ---: | ---: |
| deltablue | 1.001867 | 1.086420 |
| logging_silent | 1.002486 | 1.241474 |

Near-identical instruction totals do not explain the cycle loss. Actual
GENERAL handler comparison finds two additional static instructions in each
inline initializer, with changed SIMD/scalar scheduling; the binder adds one
instruction at each of two initializer sites. No extra call, guard, stack slot
or larger stack frame appears. `initialize_locals` is unchanged. RETURN/SEND
changes are cold-tail sharing; ordinary exact-call and delegated-yield paths
retain their previous work. These observations do not establish a cause for
the broad regression or justify a padding/compiler search. Preserve raw
disassembly in `E59/created-generator-diagnostic/native-review.md` and PMU
commands/results in `E59/parity61-created4-counter-diagnostic`.

## Allocation-only creation62: rejected after broad comparison

Source `4a732f4d7c1af9e8a56bf2b04f9107a1ad14faf7` has root and independent
actual-diff GO. Root integrates at `15edce610d09ffcc37f7b7ed3465852101397b49`
on `pyjit/parity62-allocation-integrated`. Generation passes with no product
diff. Both builds complete 117/79; 526 focused methods across twelve modules
pass per role. This is focused qualification, not full-system qualification.

Inputs remain in their original owning CALL homes through allocation, then
move once into the complete heap frame. Allocation failure retains the ordinary
CALL prestate. The result-only EXTERNAL root survives real redirect cleanup
and transfers at the existing consuming writer. The shared initializer uses a
compile-time ownership mode to retain ordinary code acquisition at its original
header store; no runtime ownership branch is intended. The
[component proposal](creation-allocation62.md) owns the precise source contract.

Native capture `E59/parity62-allocation1` has ELF SHA256
`c51546d9df97d8397d8e511eae4d77f5ce5afd2f18498b0db217cef6ae879685`.
The helper shrinks 2,475→1,382 bytes; explicit stack adjustment including its
saved-register pushes shrinks 328→168 bytes. No owner buffer, input prefix,
second argument move or known-code/generator numeric classifier remains. One
postpublication refresh and final-home NULL initialization remain. NO_SELF
GENERAL returns to 1,378 bytes; supplied-self GENERAL stays 1,461 bytes.
These static facts do not establish an elapsed improvement.

The stable two-pair screen shows a repeatable coroutine gain with sentinel
losses. Root keeps the candidate **provisional**, advancing to confirmation
to assess the tradeoff rather than calling it a winner from fewer stores.
Golden remains unchanged. Evidence uses `parity62-allocation1` build/focus
manifests and `E59/parity62-allocation1-eight-selected57`.

| Workload | Allocation62 / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .983374 | .958832–1.008544 |
| deltablue | 1.009277 | 1.004707–1.013868 |
| coroutines | .957328 | .954150–.960517 |
| generators | 1.000100 | .991305–1.008973 |
| logging_silent | 1.021175 | 1.016053–1.026323 |
| raytrace | .991176 | .989330–.993025 |
| spectral_norm | 1.044129 | 1.043082–1.045177 |
| nqueens | 1.008648 | 1.002679–1.014652 |

Six-pair confirmation has stable source/native identity and no failure. The
coroutine improvement and DeltaBlue/logging/spectral losses persist in every
pair. Evidence: `E59/parity62-allocation1-confirm-selected57`.

| Workload | Confirmation / selected57 | Six-pair range |
| --- | ---: | ---: |
| comprehensions | .995230 | .965291–1.004610 |
| deltablue | 1.005235 | 1.001161–1.007419 |
| coroutines | .955934 | .952305–.964509 |
| generators | 1.005605 | .996386–1.015946 |
| logging_silent | 1.033865 | 1.011046–1.055721 |
| raytrace | .986471 | .977594–.990449 |
| spectral_norm | 1.049700 | 1.043736–1.053897 |
| nqueens | .999857 | .996094–1.003659 |

Root advances to the broad set to decide whether the actual coroutine gain
and cleaner creation contract justify these measured tradeoffs. The directory
`parity62-allocation1-all29-selected57` is **misnamed**: the coordinator omitted
`--cases`, so it contains only two logging pairs, not an all29 campaign. Its
commands/results remain unchanged and are excluded from broad qualification.
The explicit-roster retry uses a new output directory. No upstream comparison
or full-system qualification is yet claimed for allocation62.

### Broad decision: allocation62 is not selected

The explicit 29-case retry completes two balanced pairs (116 workload rows),
with stable source/native identity and no failure. Its observed geometric mean
is **1.004907/selected57**, with 12 wins out of 29. Coroutines still improves,
but logging, spectral_norm and deepcopy_memo lose in both pairs. Root rejects
the standalone packet: the measured broad result does not justify those losses.
No direct-upstream/PMU/full-system campaign follows. Preserve its branch and
native evidence; the coroutine gain is evidence for the design, not a selected
dependency. Timing: `E59/parity62-allocation2-all29-selected57`. Independent
cold native review: `E59/parity62-allocation1/native-review.md`.

| Workload | Allocation62 / selected57 | Two-pair range |
| --- | ---: | ---: |
| richards | 0.996216 | 0.995507–0.996924 |
| go | 0.993276 | 0.989002–0.997569 |
| nbody | 0.956744 | 0.924647–0.989955 |
| float | 0.996076 | 0.995562–0.996590 |
| spectral_norm | 1.053241 | 1.051913–1.054571 |
| raytrace | 0.982213 | 0.980913–0.983515 |
| deltablue | 1.007473 | 1.004705–1.010248 |
| chaos | 0.994621 | 0.990428–0.998832 |
| nqueens | 0.996371 | 0.994562–0.998183 |
| fannkuch | 1.021605 | 1.013282–1.029996 |
| generators | 1.004635 | 1.003703–1.005567 |
| comprehensions | 0.995968 | 0.994229–0.997711 |
| coroutines | 0.957226 | 0.953193–0.961276 |
| unpack_sequence | 1.004298 | 1.002000–1.006601 |
| deepcopy | 1.010824 | 1.010536–1.011113 |
| deepcopy_reduce | 1.001750 | 0.996778–1.006748 |
| deepcopy_memo | 1.079159 | 1.078662–1.079656 |
| json_dumps | 1.011768 | 1.010221–1.013318 |
| json_loads | 1.007427 | 0.997115–1.017845 |
| pickle | 1.002672 | 0.999430–1.005924 |
| unpickle | 1.015601 | 1.011764–1.019452 |
| regex_compile | 1.014558 | 1.012866–1.016253 |
| logging_format | 1.013480 | 1.005394–1.021631 |
| logging_silent | 1.020099 | 1.019242–1.020956 |
| logging_simple | 0.988310 | 0.963631–1.013620 |
| xml_etree_generate | 0.990827 | 0.990470–0.991184 |
| xml_etree_iterparse | 1.020280 | 1.001211–1.039711 |
| xml_etree_parse | 1.015717 | 1.006164–1.025361 |
| xml_etree_process | 0.997386 | 0.994905–0.999874 |

## Delegated resume62: rejected

Source `8697fe15235f5a91c1bc188ff0c94d6e03a5326a` starts from selected57.
Root integrates generated products at `c80deebb7a9`, then correction
`d43f88d0402d11321aaefb8e1ecce895751b6c29` at final
`17139aa0166e18158765b4bccc11408ecbfe1759`. The correction changes source
analysis/fixtures; generated runtime products remain byte-identical.

The first release integration covered 397 methods and found two failures.
The exact monitoring fixture expects a LINE event absent on both candidate and
selected57; matched read-only probes retain identical instruction/resume events
and stable source/ELF identities. The correction checks actual instruction
and resume events plus LINE wrapper restoration. Separately, the new optional
source contract rejected a valid altered-source analysis globally. Unsupported
contracts now omit the variant; existing mandatory delegated-cursor rejection
remains. Neither correction weakens ownership or semantic image admission.
Evidence: `parity62-resume1-release-focus.log`,
`resume62-exact-observation-*` and the correction host log under E59.

Both corrected builds complete 117/79; all 397 focused methods in eight
suspension/observer modules pass per role. Capture `E59/parity62-resume2-native`
has ELF SHA256
`ddb5b6dd027091a1863575182a5ca020900037669c943feaab13baf0da66cacb`.
Its independent `native-review.md` confirms the fixed continuation and ordinary
entry protocols. The counted SEND continuation is 29→28 executed instructions,
conditional branches 2→4 and indirect transfers 2→1. This is a manual static
successful-path count, not PMU attribution. Ordinary RESUME success adds no
spill/helper; exact CALL still reserves 0x40 bytes, SEND gains no stack frame.
Reviewed symbols grow 604 bytes and whole ELF text 2,496 bytes. Removing the
38-byte jump handler transfer does not remove 38 bytes of dynamic work.

The stable two-pair screen improves generators 4.3% but loses DeltaBlue,
logging and spectral_norm in both pairs. Root rejects the standalone packet;
no confirmation, all29, direct-upstream/PMU or full-system campaign follows.
Preserve source `pyjit/parity62-resume-integrated` and its evidence, without
making this losing native layout a dependency of prepared CALL. Timing:
`E59/parity62-resume2-eight-selected57`.

| Workload | Resume62 / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.980435 | 0.958321–1.003058 |
| deltablue | 1.065177 | 1.063197–1.067161 |
| coroutines | 0.995901 | 0.987009–1.004874 |
| generators | 0.956959 | 0.950052–0.963915 |
| logging_silent | 1.203048 | 1.190222–1.216012 |
| raytrace | 0.998169 | 0.997124–0.999215 |
| spectral_norm | 1.092709 | 1.089842–1.095584 |
| nqueens | 0.992462 | 0.986017–0.998950 |

## Prepared CALL62: rejected after focused qualification

Source `8d418b47f669dfa5c7c6db8434bdfbf31c8e22e8` has root and independent
actual-diff approval after the active-only `runs_code` correction. Four host
source methods pass. Root generation found a CWriter compound-brace error;
source correction `49105087f2bb25bb1c32622c0995cdb075af388a` fixes it and
adds actual metadata-header emission to the focused source control.

Generated integration `c5f03fe81d7` completes both 117/79 builds under
`parity62-prepared2`. The first release fixture invocation discovers eight
new methods plus eight unintentionally imported admission methods. Seven new
methods pass, exercising all four active heads, public image routes, misses,
result ownership and observation invalidation. One synthetic negative fails
before execution: its GET_ITER two-home output exceeds the declared frame.
Fixture correction `ae579492c5d` fixes geometry, removes accidental imported
TestCase discovery and tests a real cold exceptional-entry UNKNOWN negative
through all public image routes without entering that handler. The unrelated
GET_ITER work legitimately preserves the argument home's positive vacancy.
Release then passes all 227 methods across ten modules. Debug passes the
other nine modules but aborts in the synthetic iterator fixture: its second
cursor home was not retired. Exact selected57 debug reproduces that abort.
Correction `6e07c1a8953` uses the existing POP_ITER protocol for both outputs;
matched corrected probes pass all public routes on both debug interpreters.
Raw commands, identities and both original aborts remain under
`E59/prepared-vacancy-debug-probe`.

Root integrates the fixture corrections at
`a3cbad0637efe6498533d31abf074c0e544fa964`. Both refreshed builds complete
117/79 under `parity62-prepared4`. The corrected eight methods pass per role;
the other 219 remain qualified under `parity62-prepared3`, for 227 focused
methods per role. Only the corrected module was repeated. Capture
`E59/parity62-prepared4-native` has ELF SHA256
`017551836bf38f5a614d19d91d0d47294174989847c55c69060f5a212e44a619`.
The independent `native-review.md` confirms one conditional DUP increment and
original argument store per head, with no generic destination classification,
old-home read or intermediate table/indirect transfer. Next-word decode and
exact feedback lookup remain. Exact heads retain 64-byte stack reservations;
the two descriptor heads add 16 bytes each (48→64 and 64→80). The four new
heads total 5,210 bytes; ELF text grows 6,912 bytes. Nine ordinary LOAD/CALL/
SEND/RETURN bodies retain their sizes and address-normalized instruction
sequences. Differing rodata annotations resolve to identical string bytes.
Those facts establish the intended native transformation, not its throughput.

Unobserved common admission nominates seven distinct kernel sites in
comprehensions and 56 in DeltaBlue. After three ordinary workload calls,
comprehensions has three active sites (two exact and one descriptor); DeltaBlue
has 41 (28 supplied-self exact, two NO_SELF exact and eleven descriptor).
Kernel-filtered native breakpoints confirm at least one entry into each of
those handler families. These are static site counts and bounded first-hit
proofs, not dynamic frequencies. The fourth NO_SELF descriptor family passes
the focused fixture but has no kernel first hit in this probe. Exact identities,
commands, cold/postwarm images and limits are in `E59/prepared-coverage`.

The stable two-pair screen improves comprehensions and coroutines, but logging
loses 18.3% and DeltaBlue/generators also lose in both pairs. Root rejects the
standalone packet. Preserve `pyjit/parity62-prepared-integrated` and its source
branches; no confirmation, all29, upstream or full-system campaign follows.
The method consumer may reuse its source infrastructure independently, with
final-local nomination disabled for its isolated trial. Timing:
`E59/parity62-prepared4-eight-selected57`, 32 workload rows, stable identity,
no failure.

| Workload | Prepared62 / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.948255 | 0.931407–0.965408 |
| deltablue | 1.021921 | 1.018908–1.024943 |
| coroutines | 0.964114 | 0.956796–0.971488 |
| generators | 1.023161 | 1.015132–1.031253 |
| logging_silent | 1.182918 | 1.178172–1.187683 |
| raytrace | 0.988174 | 0.985945–0.990408 |
| spectral_norm | 1.012265 | 1.011034–1.013498 |
| nqueens | 1.019683 | 1.015668–1.023714 |

### Bounded cycle diagnostic

`E59/parity62-prepared4-branch-diagnostic` reruns the same balanced commands
with four hardware counters, with 100% event running time. Its interval is the
whole process, imports, warmup and seven timed kernel calls. It is distinct
from a qualified kernel-only PMU campaign. Source-controller and executable
hashes are stable; these ratios do not replace the elapsed screen above.

| Workload | Instructions | Cycles | Branches | Branch misses |
| --- | ---: | ---: | ---: | ---: |
| comprehensions | .988185 | .970045 | .993012 | .962041 |
| logging_silent | 1.000317 | 1.147451 | 1.000491 | .996456 |

Logging executes essentially the same number of instructions and branches
while spending more cycles. Aggregate branch-miss counts decrease slightly;
an increased count does not explain this loss. This does not identify the
remaining mechanism. A follow-on fetch diagnostic retains six Zen2 counters
at 83% multiplexed running time under
`E59/parity62-prepared4-fetch-diagnostic`. Its scaled counts do not establish
an exclusive instruction-cache or fetch-stall cause and are exploratory only.
No compiler, padding or alignment search follows. The next grouped architecture
review must account for recurrent broad cycle losses as well as known repeated
transport and dispatch work.

## Method-to-exact CALL62: rejected after focused qualification

Root integration `ab4cb5f49fb0210a80bcdb94b597ad6ed94db9ac` starts from
selected57 and source packet `a67bdfc0b1da80607d1915840affb4f4a911574b`.
Registry correction `2e7c277dde5` uses removed reservations for unused IDs
370–374; generated products are committed at `665fee2062d`. Fixture correction
`882309dd897` supplies fresh source maps for complete synthetic bodies and
checks cold non-nomination instead of an admission rejection absent on selected57.
Matched release/debug probes in `E59/method-fixture-diagnostic` establish the
baseline behavior. The malformed images are never executed; no production
admission rule was weakened to satisfy these fixtures.

Four host source methods pass. Both final native builds complete 117/79 and
all 227 focused methods pass per role: eight new methods under method3, the
other 219 release under method2 and debug under method3-debug-protocol. Root
saves the native artifact at `E59/parity62-method3-native`, ELF SHA256
`a39e077632782a8d7538fa7134305b241f63ef150691db58c53d01bbba556624`.

Independent `native-review.md` confirms the actual acquired method owner enters
`f_funcobj` directly. Success has no callable-home store/load/final clear or
intermediate dispatch. One write-only B=NULL and the owning receiver store
remain. A miss materializes the method once before ordinary interior CALL;
heap callers leave before acquisition. The active head is 1,151 bytes with
64 bytes of stack reservation; ELF text grows 3,264 bytes. Eight inspected
ordinary handlers retain address-normalized instructions, sizes and stacks;
corresponding rodata bytes match. Generic prefix role classification,
next-word/feedback decode and an early heap test remain. This establishes the
mechanism, not its throughput.

Unobserved coverage in `E59/method-coverage` records Delta 43 pending sites
becoming 42 active plus one pending. Ray has two active and eight pending sites
after setup, then ten active after workload warmup. Kernel-filtered native
first-hit probes confirm entry on both kernels. These are static site counts
and bounded first-hit evidence, not dynamic frequencies. No zero-admission
claim is made for unprobed comp or logging sites.

The balanced hot-eight screen has 32 rows, stable pre/post identity and no
failure. Root rejects the standalone executable: both Delta and logging pairs
lose. No confirmation, all29, upstream or full-system campaign follows.
Preserve branch `pyjit/parity62-method-integrated`, source and evidence;
selected57 is unchanged. Timing: `E59/parity62-method3-eight-selected57`.

| Workload | Method62 / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | .965035 | .942478–.988132 |
| deltablue | 1.032009 | 1.030685–1.033335 |
| coroutines | .998281 | .996426–1.000140 |
| generators | .998863 | .997176–1.000554 |
| logging_silent | 1.228535 | 1.226871–1.230202 |
| raytrace | .996699 | .990930–1.002502 |
| spectral_norm | 1.003543 | 1.002691–1.004395 |
| nqueens | 1.008903 | 1.008064–1.009742 |

Round62 closes with no selected packet. Native work removal is real in all
four candidates; their broad elapsed results do not earn selection. Round63
uses their ownership and writer lessons as source inputs, without inheriting
losing binaries. The repeated cycles/instructions divergence remains unexplained;
no exclusive cache, branch-target or compiler cause is asserted.

## Fixed list roles63, phase one: rejected

Source `90cd71b4874e6229940ef7593f3f32624c52486a` starts independently from
selected57. Root integrates source at `519205b2d57`, completes generation and
commits products at `c7fecde25a4`. Only FOR_ITER_LIST_LOCAL and LIST_APPEND_TEMP
bodies are added; pending FOR aliases the existing ordinary handler. Source
owner receipts and conditional FOR authority have independent actual-diff
approval. Four focused host source methods pass in both record modes.

The first release run covers 49 methods with one fixture failure: private-ID
rejection inserted a FOR window below the scratch partition, so geometry
validation rejected before the intended execution-only check. The other 48
methods pass. Fixture-only source correction `a0e1cea9da2` replaces actual
ordinary FOR/append words while preserving their operand geometry; strict
private-ID rejection remains. Root integrates it at final
`034f2b22f87bb4e9e54e689bb5ead8a252e60a97`. Both refreshed builds complete
117/79, and the corrected six methods pass per role. The other 43 pass release
under role1 and debug under role1-debug-protocol: 49 focused methods per role.
No production admission rule changes to accommodate the test.

Native capture `E59/parity63-role2-native` has ELF SHA256
`e8d3f5aa293aa030978b60822a77d63450e771adb1f5818dc14cca26edd0761a`.
Independent `native-review.md` confirms FOR shrinks 1,153→658 bytes with the
same 32-byte stack reservation and getter spill/reload traffic. Item
normalization, staging store and destination-role choices disappear; cursor
normalization, one B+2 clear, required item owner and local retirement remain.
The compiler already kept the ordinary item in a register, so no native reload
saving is credited. Append is 249→217 bytes; its reviewed scratch success
path is 38→35 caller instructions and 3→2 conditional branches, retaining the
same consuming provider/error protocol. Ten ordinary controls retain their
normalized instructions and sizes; differing SEND literal annotations resolve
to byte-identical strings. Whole text grows 2,496 bytes, rodata 608. This is
mechanism GO, not a throughput result.

Unobserved code nomination and kernel-filtered GDB first hits are retained in
`E59/role63-coverage`. Comp has eight pending FOR plus one already active after
setup, and four static append sites; after warmup there are eight active FOR
and four append sites. Both native bodies record a kernel first hit. Delta
13 pending→six pending and logging 16→15 pending have no active-body first hit
in the bounded runs. The logging scope includes its benchmark and imported
standard-library logging module. Static counts are not invocation counts, and
setup can already specialize a site. The first logging driver incorrectly
looked for logging_silent.py; its failed log is retained. The corrected driver
uses logging.py with the proper variant and separate output; no runtime
failure is inferred from that driver error.

The balanced hot-eight screen has 32 rows, stable source/native/controller
identity and no error. Comp improves 4.1%, coroutines 3.6% and spectral_norm
4.5%, but logging loses 8.3% in both pairs; Delta/nqueens also lose. Root rejects
the standalone executable. No confirmation, all29, direct upstream, PMU or
full-system campaign follows. Preserve `pyjit/parity63-role-integrated` and all
evidence; selected57 remains unchanged. Timing:
`E59/parity63-role2-eight-selected57`.

| Workload | Role63 phase one / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.958819 | 0.947011–0.970774 |
| deltablue | 1.009161 | 1.008785–1.009538 |
| coroutines | 0.963592 | 0.959279–0.967925 |
| generators | 1.004905 | 0.999091–1.010752 |
| logging_silent | 1.083074 | 1.077502–1.088675 |
| raytrace | 1.002200 | 0.994760–1.009694 |
| spectral_norm | 0.954570 | 0.950743–0.958413 |
| nqueens | 1.005140 | 1.001092–1.009205 |

The logging loss occurs without a new-body hit in its bounded coverage run and
with unchanged inspected ordinary native bodies. Those facts do not identify
an exclusive cause. No layout/flag sweep or further counter fishing follows.
Phase two is an independent attribute-role packet from selected57, excluding
these losing FOR/append bodies; deferred await remains a separate source lane.

## Round63 isolated attribute roles: qualified, rejected

Source `c1066265668d0256f78eaf04e3894f011485184e` starts directly from
selected57, excluding the rejected list/FOR and append implementation. Root
integrates at `1fe09389531`, commits generated products at `18445aa21d5`, then
integrates fixture-only `035aa32e16c` at final
`581c018a34a2c4d521cec789e1261a4b331c7dc8`. The
[unit](role-lowering63.md#phase-two-source-qualification) records actual source
receipts, independent approval, four passing focused host methods and coverage
limits. No active ownership graph claims an unsupported poststate.

Both `parity63-attributes2` builds complete 117/79. All 54 focused runtime
methods pass per role across attribute_roles, operand_modes,
native_owner_effects, admission, error_scope and transfer_admission. The first
run passes the existing 47 and fails three new fixture assertions: method
cases share warmed code, mutable payload instances do not qualify for CLASS,
and a custom metaclass requests the unsupported metaclass-check member. The
aborted finalizer fixture also reads an unset closure cell. The fixture-only
successor uses fresh method code, supported builtin-list payloads containing a
mortal object, and an ordinary type with abort-safe weakref.finalize. Exact
active-head checks and four construction routes remain. Corrected seven pass
both roles under attributes2; existing 47 pass release under attributes1 and
debug under attributes1-debug-controls. No production gate is relaxed.

Native capture `E59/parity63-attributes2-native` has ELF SHA256
`0b827d770ba806326b82565bb916e8e19f2b5af7d5ead7e4e81cc71c714349a0`.
Independent native mechanism review passes: two method owners/stores remain,
class takes its receiver and publishes once before retirement, and shape moves
the value before retiring the old field without a fictitious receiver owner.
Reviewed successful static paths fall 54→46, 56→49, 88→71 and 73→58 instructions
for NO_DICT, WITH_VALUES, class alias and shape store respectively. These paths
include dispatch, exclude callees, use mortal acquisitions and nonfinalizing
closes; they are not dynamic workload counts. Ten ordinary controls retain
their normalized instructions/sizes; SEND's differing annotation anchors
resolve to identical string bytes. Explicit class/shape stack stays 16 bytes,
with no new common-path spills. Whole text grows 3,328 bytes and rodata 1,376;
new bodies total 1,225 bytes, not the whole growth. Bounded actual probes in
`E59/attribute63-coverage` have stable identities and kernel-filtered native
first hits for all four active bodies. Comp finishes with six method sites and
one store, Delta with 28 class/64 method/19 store sites, and logging with 12
method sites. Logging also records a store first hit despite no active store
at its final snapshot. These are static site counts and first-hit lower bounds,
not invocation frequencies, steady-state store coverage or timing evidence.
Setup can already specialize sites. No throughput or selection claim is made
before the unchanged balanced screen.

That screen completes 32 rows with stable identities and no errors in
`E59/parity63-attributes2-eight-selected57`. Comp improves 3.5% and Delta 3.1%,
but logging loses 22.3%, spectral 6.3% and generators 1.8%; every listed loss
occurs in both pairs. Root rejects the standalone packet. Preserve
`pyjit/parity63-attributes-integrated`, source branches and sealed evidence;
selected57 remains unchanged. No confirmation, all29, direct upstream, PMU or
full-system campaign follows. Natural handler entry and removed native work do
not establish a net gain; an exclusive cause of broad cycle losses is still
unproved. No compiler/layout/padding search or repeated counter diagnostic
follows. Both fixed-role phases are now closed as losers. Deferred await
remains an independent incomplete source unit, and the next grouped review
must account for these results before another execution design is implemented.

| Workload | Isolated attribute63 / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.964506 | 0.942078–0.987467 |
| deltablue | 0.968592 | 0.966129–0.971061 |
| coroutines | 1.002732 | 1.000612–1.004856 |
| generators | 1.017802 | 1.017655–1.017950 |
| logging_silent | 1.222917 | 1.195991–1.250450 |
| raytrace | 1.000004 | 0.998176–1.001836 |
| spectral_norm | 1.063393 | 1.062747–1.064039 |
| nqueens | 1.010458 | 1.010310–1.010607 |

## Compact domain64: stopped at the native work gate

Source `6a889e40ba1ba3ca4e22ef922a3117f76493d9bb` starts independently from
selected57 and has root plus independent actual-source approval. Final
integration `9de7005b670b1acc0350c22f53f43dc91ccc9f14` contains generated
products and a fixture-only correction. Both refreshed builds pass 117/79;
eight new methods and 21 existing controls pass per release/debug role. The
first release run passes seven new methods and fails one because it divides
the semantic-word `tb_lasti` by eight. The correction compares raw indices
and reproves the changed synthetic OOM image with fresh empty source/exception
maps. Production remains unchanged. No new full-system qualification is claimed.

The [unit](compact-domain64.md#native-gate-stop), [source peer](compact-domain64-peer.md)
and independent [native review](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-compact2-native/native-review.md)
close this packet. Captured ELF SHA256 is
`d3246e84e58f812cc30241d36db6365708d5372af7a512942c62331e991906b7`.
Stack/text caps pass: 112 simultaneous bytes, including both return addresses,
and 1,665 hot bytes. The required complete-path instruction reduction fails:

| Explicit static path to the ordinary boundary | Selected57 | Compact64 |
| --- | ---: | ---: |
| FOR-local → NO_DICT method → ARG → CALL | 173 | 227 |
| APPEND → BACK → FOR-local → NO_DICT method → ARG → CALL | 234 | 280 |
| Singleton APPEND → unsupported word | 38 | 128 |

The five internal words fall 234→187, saving 47 instructions, but admission
and entry add 59 and exit adds 34. Inline-ID paths also grow, 169→219 and
230→272. Counts include executed envelopes, dispatches and provider calls;
provider internals and the ordinary CALL body are excluded equally. These
are static paths with explicit branch assumptions, not measured frequencies
or elapsed-time results. Thirteen ordinary handlers/providers normalize
equivalently; differing rodata annotations resolve to identical bytes. Whole
`.text` grows 4,800 bytes, without an exclusive performance-cause claim.

Root accepts native NO-GO and stops before timing or target coverage. Preserve
the packet and `E59/parity64-compact2-native/{native-review.md,static-path-counts-final.json,static-path-counts.py,native-static-facts.json}`,
`compact64-source-host` and `parity64-compact{1,2}` qualification logs/commands.
Partial `E59/compact64-coverage` scripts are unused and unqualified; they prove
no natural entry, completed segment or absence. There is no rescue through
expanded operations, mirrors, providers, flags or layout searches. Selected57
remains unchanged. The independently reviewed complete await source
`f98decf1109` proceeds separately to root generation/native qualification.


## Deferred await64: qualified mechanism, rejected executable

Final integration `0bbb42ef46ab9c55996e9b507a528a12705eedf0` implements the
complete stable-frame immediate-await unit, independently of every rejected
creation, role, prepared-call and compact packet. Both await9 builds complete
117 modules/79 extensions, and all sixteen new lifecycle/admission methods pass
per role. Fourteen host methods cover the final terminal correction. The twelve
runtime control modules run 1,066 methods per role with only the same six
inherited sys_settrace debugger-jump errors; C API frames, remote inspection and
async graph controls run another 140 per role (three skipped). These are focused
integration checks, not a fresh full-system qualification. The
[unit](deferred-await63.md) defines the ownership/reader contracts; the
[handoff](../../HANDOFF.md) owns the exact integration chain and build paths.

Root's earlier native diagnostics exposed real admission and lifecycle defects:
SEND's ordinary resume backedge must be separate from the linear CALL prefix;
LOAD_GLOBAL needed a reached normal-result fact after unknown-effect
invalidation; compatible SEND specialization must preserve an existing checked
certificate; and automatic terminal emission must stop invalidating the parent
once the departing-frame alias switches. The last correction fixes both lost
late completion observation and a debug suspension abort by removing sixteen
parent phase clears across four RETURN and two YIELD variants. It changes no
other generated product and relaxes no runtime assertion. Fixture corrections
create the executing copy explicitly, use valid source/exception maps, capture
tracebacks before unittest clears them, and make allocator failure one-shot.
All failed attempts and source/generated peers remain sealed in E59.

Independent cold source and actual native post-reviews find no must-fix defect.
The retained ELF in `E59/parity64-await9-native` hashes
`1c8f91ebf882e01250dd2c8a32a96039b76a4eff129d6b45208085412cbc68f8`.
The actual private path removes coroutine construction/copy/initial reentry,
but pays a 184-byte fixed arena reservation (112-byte header plus 72-byte record),
disjoint argument moves, two name snapshots, exception/record linking and
rooted retirement. Ordinary generic CALL bodies normalize exactly equal;
ordinary exact CALL broadens residence predicates without extra instructions.
RETURN/SEND/YIELD and generator accessors have real additional paths and text:
RETURN_VALUE 596→650 bytes, CompleteGeneratorReturn 1856→1962 (local stack 40→56),
SEND_GEN 776→808, SEND 2388→2594, YIELD_VALUE 525→732 and
YIELD_DELEGATED 556→756. Complete symbol sizes are not dynamic counts or an
exclusive explanation of measured losses.

Actual native controls, independently receipt-reviewed, establish:

- `natural9b.native.json`: unchanged coroutine kernel `run(4)` enters a private
  leaf and retires that same child with no identity object.
- `terminal9b.native.json`: first true suspension transfers two private frames
  in the same evaluator suffix; Python verifies the chain, resume and GC.
- `arena-oom9d.native.json`: one actual private VirtualAlloc is forced to NULL.
  CALL homes/refcounts/PC/arena/exception state remain unchanged through the
  child-NULL edge; the helper returns error, Python records the exact CALL word,
  runs finally, retries successfully and eventually releases its payload.

Those are matched transactions, not frequencies. Logs, commands, ABI manifests,
controller/probe hashes and the receipt peer are in
`E59/deferred-await-native-controls`. The initial GDB driver needed quoted clone
symbols and then an exact relocated helper-PC range because DWARF omits the
ELF clone suffix. Both driver failures and corrected copies remain retained;
no runtime workaround or fallback fault injection was used.

The balanced unchanged hot8 screen completes 32 rows with stable identity and
no errors in `E59/parity64-await9-eight-selected57`:

| Workload | Await9 / selected57 | Two-pair range |
| --- | ---: | ---: |
| comprehensions | 0.987018 | 0.964970–1.009570 |
| deltablue | 1.003049 | 1.000341–1.005765 |
| coroutines | 0.571378 | 0.570239–0.572518 |
| generators | 1.051852 | 1.049759–1.053949 |
| logging_silent | 1.276929 | 1.272406–1.281468 |
| raytrace | 0.983750 | 0.977447–0.990093 |
| spectral_norm | 0.924742 | 0.922693–0.926795 |
| nqueens | 1.024620 | 1.024589–1.024650 |

Coroutines elapsed time falls 42.9% in both pairs: the complete activation change
has a genuine measured benefit on its target. Logging loses 27.7%, generators
5.2% and nqueens 2.5%, also in both pairs. Root rejects this standalone executable
because those broad regressions are unacceptable. Comprehension pairs disagree;
DeltaBlue is close to flat. Preserve the complete source, native artifacts and
mechanism success, but do not select it or import it automatically into another
lane. Selected57 remains unchanged. No confirmation/all29/direct-upstream/PMU
or full-system campaign follows this rejected screen. Old upstream ratios must
not be multiplied into a claimed new upstream measurement.

Round64 is closed: compact failed its native net-work gate and await passed
mechanism qualification but lost the broad screen. Astra's next grouped review
must use these distinct outcomes to specify concrete IR/interpreter changes,
including how to preserve structural activation savings without ordinary-path
regressions. No causal claim about code layout, compiler behavior or one branch
is established by this screen. The completed [next review](review65.md),
source `374f8e3eadb` plus `ceb46447bb0`, orders two bounded units under the
[round65 design](../../design/proposals/interpreter-parity65.md). Unit A changes
existing complete returns and proved embedded access; B is an optional private
SEND-yield continuation with complete reader/writer closure. A gets an
independent native/hot8 decision before B, and may advance on its own. Source
work is now closed: A loses its standalone screen, and B fails the native cap
after one reviewed correction. The current results are recorded above; B has no
performance screen.
