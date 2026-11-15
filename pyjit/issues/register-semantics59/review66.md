# Grouped review66: a native cap is not a throughput result

**GO for complete qualification and one unchanged screen of frozen B under a
new prospective contract. CHANGE the second unit to explicit coroutine
identity/origin storage, conditional on B's result. STOP round65 corrections
and preserve its NO-GO.** [Proposal66](../../design/proposals/interpreter-parity66.md)
contains five ranked options, both unit contracts, flows and numerical stop
rules. This review creates no implementation, native qualification or selection.

## Authority and method

Documentation authority is `/home/power/code/cpython/.worktrees/python-jit`.
README, GOAL, STATUS, HANDOFF, PROCESS, performance56 plan, report, design65,
review65, deferred-await63 and validation/measurement policies were read.
The review branch starts at canonical closure
`2125567df336d8fdd9f621d17f3a4ce815c0bdab`. That checkout and this docs worktree
are not runtime source authority. Applicable AGENTS.md and the official
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/)
were read before proposing changes.

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`E59=BASE/pyjit-link-wave1/register-semantics59`.

| Role | Actual runtime source under BASE/worktrees | Immutable review head |
| --- | --- | --- |
| I, selected57 | `call55-integrated` | `0dcba75bb3df40c32137db453b001f7e8f101de0` |
| U, upstream | `upstream-3a440b6` | `3a440b65695615e9fd9912342b79cdd54fced0d2` |
| G, final rejected65 B | `call55-gc` | `64348dd5c182af27c058e60f1e3f849f00d7ef81` |

Final B ELF is `E59/parity65-continuation4-native/python`, independently read
with SHA256 `79bd05e06af44d730c343d8e34a41ae1538041e4f1e27655bd853e587c7d9fc4`.
Reading a hash or retained listing does not execute that binary. Initial B
`980baf6bbf3aa1d8182c35f8c7166c662c4ad1f0`, its continuation2 capture and
fixture text-equivalence receipt remain sealed. A is
`a6695887e949c3019216120c544e171dfa7ff90e`; await9 is
`0bbb42ef46ab9c55996e9b507a528a12705eedf0`.

No runtime changes, agents, generation, build, test, benchmark, profiler,
control mutation or push occurred in this review. Actual I/U/G source and
retained source peers, generated comparison, native listings/path JSON,
layout receipt and A's raw summary were read. No new external V8/LuaJIT
comparison is needed to decide these units; earlier analogy supplies no cost
evidence. Only the proposal66 and this review file are owned.

## Established results and residual obligations

A passed source/native/ABI qualification, both117/79 builds, 46 focused and
708 ordinary/reader methods per role. Its stable32-row screen is
[parity65-residence3-eight-selected57/summary.json][a-screen]. These are elapsed
candidate/selected57 geometric means, with two independent process-pair ratios;
they are not confidence intervals.

| Workload | A / selected57 | Pair range |
| --- | ---: | ---: |
| comprehensions | .972740 | .951365–.994596 |
| DeltaBlue | .989654 | .984450–.994885 |
| coroutines | .606811 | .604728–.608901 |
| generators | 1.055271 | 1.051395–1.059162 |
| logging | 1.161661 | 1.161023–1.162298 |
| raytrace | .980528 | .979084–.981975 |
| spectral_norm | .991837 | .989235–.994445 |
| nqueens | .999876 | .997984–1.001771 |

Reject A standalone. The generator/logging cause is unknown. Await9's .571378
coroutine ratio and retained natural private-commit/identity-free retirement,
terminal suffix movement and forced-arena-OOM receipts establish a substantial
mechanism worth preserving, not selection or a fresh upstream comparison.

B passes complete source/product review and fresh continuation4 both117/79
builds plus48 focused methods/role. Real C native-alias foreign observation and
reached callback/redirect/private-admission coverage pass in root's record.
The source peer's initial registry blocker was repaired in the shared target
reservation, not waived. The later completion correction passes its independent
source peer; full regeneration changes no products. [Registry peer][registry]
[Product peer][products] [Correction peer][correction]

The [corrected native review][native], `native-b65-corrected/falsifier.json`,
ordinary-paths and retained listings distinguish the following static facts:

| Matched complete path or size | Selected57 | Await9 | A | Final B |
| --- | ---: | ---: | ---: | ---: |
| SEND_GEN → ordinary YIELD → exit instructions | 160 | 176 | — | 157 |
| Generic SEND → ordinary YIELD → exit | 187 | 201 | — | 185 |
| Private EMPTY synchronous return | — | 153+K | 152+K | 157+K |
| Instrumented private EMPTY return | — | 166+K+M | 166+K+M | 171+K+M |
| Private GENERAL return | — | 157+K | 152+K | 157+K |
| Instrumented private GENERAL return | — | 170+K+M | 166+K+M | 171+K+M |
| Private header plus record, bytes | — | 112+72 | 112+72 | 112+80 |

K is the matched clear_await_frame interior; M is the matched observer interior.
Neither is zero or omitted from the claim of a complete path: they are equal
opaque terms in these comparisons. Counts include actual entry/call/return/
dispatch branches, not a sum of symbol sizes. Warm reservation remains44.
The corrected common completion is99→93; ReturnResumable22→27 carries live
semantic_next into that consumer. Entry ABI stays unchanged. Four YIELD forms
lose await-parent checks without added inspected local stack slots. Generic
CALL/CALL_NO_SELF normalize equal to selected57; exact/general/binder bodies
match A/await. These equalities do not establish equal whole-program timing.

Text+1664, rodata+96 against await9 pass65's caps; the private target is196 B.
First-suspension's old64-instruction allowance, aggregate resolver1 KiB and
remaining cold/cleanup/control transactions are **unqualified after hardstop**.
No B screen or profile exists. The +5 versus +4 native failure is neither an
implementation failure nor a measured throughput loss.

## Architecture findings from actual source

| Actual source anchor | Finding and implication |
| --- | --- |
| [I generated CALL_METHOD_DESCRIPTOR_FAST][i-call] | Transaction and descriptor checks precede general value-vectorcall; B0 roots the result through callable/argument/self retirement and periodic processing. The repeated window sweep survives selected57 but Invoke59 already tested removing it and lost. No new cleanup retry. |
| [I exact-call initializer][i-exact] | THREAD arguments already overlap the callee; callable becomes the function owner. Heap callers move disjoint arguments. One must not claim those stores twice. |
| [I general fallback binder][i-bind] | Parent retains strong snapshots while copied arguments are consumed into a staged child, then retired. A direct borrowing input interface could remove intermediate storage, but not callback stability or both simultaneously live owner populations. Direct binding plans already exist below this fallback. |
| [U checked descriptor call][u-call] and [U RETURN/SEND][u-return] | Upstream has checked provider entry but still StackRef conversion/cleanup; return offset and embedded frame/exception access have simpler ordinary conventions. Neither observation grants a Python no-callback guarantee. |
| [G live completion][g-return] | AWAIT reads disarmed live return_pc after clearing; GENERATOR takes/cancels logical control; the single consumer uses explicit semantic_next and signed-D. The +5 compiled transport has a real consumer. Removing it by retaining stale state would violate the contract. |
| [G logical-PC APIs][g-pc] and [generated private target][g-target] | One target is outside semantic words. Real SEND indices survive; observers/redirects/copy must resolve or cancel. The source design removes a private classification from ordinary YIELD rather than creating per-code return aliases. |
| [G identity allocation][g-identity] and [admission][g-admit] | Only private exact coroutine identity creation installs active_frame; private creation denies origin tracking. Public constructors and async-generator finalizers still need their own initialized object-arm paths. |
| [G traversal][g-traverse] and [detach][g-detach] | Active views do not own the active frame's edges twice; terminal detach transfers actual owners to the embedded frame and clears the borrowed link. Union conversion must retain that ownership graph and stable active addresses. |
| [G remote asyncio reader][g-remote] | Reads the universal active pointer from exported debug offsets. Unit2 must change this real layout reader and format/validation together. In contrast, semantic-PC readers never need B's execution target as a source PC. |

The [layout receipt][layout] shows selected request240 versus A248 B for the
same generator framesize19, nlocals1, stack4. It establishes an added pointer
and request bytes, not allocator class, memory-bandwidth or elapsed causality.
The union proposal removes that pointer semantically because origin and active
view cannot coexist for an admitted private identity. An explicit discriminator,
getter and GC selection remain real work; they must be priced, not hidden in
the words “cold” or “union.” A callback-free publication transition is required.
Ordinary generators gain initialization of the formerly unused object arm plus
the tag; there is no universal net NULL-store saving. Root accepts cr_origin's
deliberate member-to-getset descriptor change, preserving value/read-only
semantics. Remote format2→3 and bounded rejection of transient active-tag/NULL
reads are actual reader obligations, not claims of atomic remote observation.

Comp's six-word loop still performs more native work than upstream's eight.
Retained source/monitored coverage identifies method selection and native CALL,
not a blanket fallback-binder opportunity. Selected matched PMU puts the comp
count gap at30.38% of its own instructions; Delta/coro count ratios are1.361424/
1.307831. Generator's old1.372523 elapsed ratio has no fresh PMU counterpart.
Neither one self owner pair nor eight object bytes can be assigned that gap.

The final ranking therefore puts a whole native-callout/result-consumer ABI
above the unmeasured generic binder. Keep B0 as the one result root through
callbacks; one ordinary-C activation can span provider, retirement, poll and
a proved consuming edge, returning to NCSR once. Comp's actual A=B=17 already
avoids the alleged B0-to-destination move. The new hypothesis concerns repeated
VM-state preservation and complete consumer boundaries, not that nonexistent
move or another Invoke/provider cleanup. Ordinary append error/observer PCs,
input ownership and no provider replay remain mandatory. Added entry/spill/
refresh costs are unproved and may defeat it. It is the preferred next broad
unit, not a third66 lane; the coverage table excludes unproved Delta benefit.

## Why the gates change, and what does not change

The old cap correctly stopped65, but a five-instruction transport instead of
four does not predict the application's result. Passing native caps did not
prevent A's logging loss. Instrumentation frequency is unmeasured; do not call
the failing path rare as a fact. The independent [gate memo][memo] agrees with
this distinction, but is an input, not authorization or a qualification receipt.

Proposal66 replaces individual count cliffs with complete hard behavior/owner/
ABI closure, representative whole-boundary cost accounting, bounded complexity,
and a predeclared elapsed/memory decision. Remaining suspension/resolver work
must still be reviewed; it is not silently waived because its numerical cap
becomes predictive. No exhaustive instruction census is demanded before first
timing. No new ordinary heap operation, universal resolver or owner engine can
be hidden behind the more practical policy.
The final table explicitly reports rather than hard-stops on old return+4,
suspension+64/frame, resolver1 KiB, text+2 KiB, rodata+128 B, spill and strictly
positive cycle-count caps. Frozen layout, one target, bounded work, ownership,
ABI and actual transaction closure remain required. First-suspension OOM may
retain valid identities/name transfers and an already resolved real SEND;
it requires recoverability and no partial suffix movement, not entry's original
bitwise state.

Prepared62's logging instructions1.000317/cycles1.147451 and unchanged ordinary
listings do not prove a frontend/cache/layout cause. Branch misses were lower;
separate83%-multiplexed fetch counts cannot settle the question. The recurring
unexplained loss justifies the proposal's one bounded matched diagnostic even
on a losing screen. It does not justify timing rescue or a counter/flag/padding
search. perf_event_paranoid2 may prevent useful capture; an unavailable or noisy
diagnostic ends inconclusive without changing controls.
Near-equal instructions and extra cycles exclude only an equal-CPI proportional
count explanation of the whole gap; expensive changed work remains possible.
The exception is exactly three logging instruction/cycle pairs and one fixed
cycles-only sample per role, capped at 60 seconds each, with declared coverage/
loss thresholds. It cannot rescue a failed screen or expand into counter fishing.

## Ordered disposition and limits

1. **New round66 unit1:** finish missing semantic/native transactions on unchanged
   final B and conduct one screen if qualified. Preserve65's NO-GO and ELF/source.
   The information value is high: a substantial real coroutine mechanism, an
   ordinary-path isolation change, and no measured B tradeoff yet.
2. **Conditional new source unit2:** typed origin/active-view union with explicit
   discriminator, complete constructor/GC/detach/remote closure and one independent
   screen. Its value is a bounded removal of universal object storage and a
   testable ordinary-generator representation hypothesis. It is not a likely
   explanation of logging, comp or Delta. A repeated logging loss stops it this
   round; no third frame-word repair follows.
3. **Defer the larger native-callout, anchored-parameter and retained-spine options** with
   explicit producer/consumer falsifiers in the proposal. They offer more scope,
   but also callback/owner or effective-state costs not justified by the current
   budget. Generic fallback binding is demoted for unknown reached coverage.
   The new whole-callout hypothesis is distinct from rejected cleanup/provider/
   fusion packets; those executables remain rejected.

Root owns independent grouped peer, source-unit peer, qualification and selection.
If unit2 builds on selected B, it must improve that checkpoint as well as preserve
the original broad gains; if B loses narrowly, unit2 is a distinct reviewed
hypothesis against selected57, not automatic integration of B. Full/all29/direct
upstream/PMU and supported diagnostic campaigns belong at promising coherent
checkpoints. No parity claim is made here. Document-only review checks cover
the two-file diff, links, anchors, fences and factual consistency; no tests run.

The independent grouped draft peer and source-only auxiliary proposal were read.
Their OOM, gate inventory, initialization, diagnostic and disposition findings
are incorporated. Unit2 requires B screen PASS or every non-generator requirement
plus both generator ratios≤1.03; no other losing-screen exception applies.
B can advance without unit2. Selection retains protected hot8 bounds, all29
mean≤1.00 and other all29 per-workload means≤1.02 against selected57. Final
independent adoption review remains root's responsibility.

[a-screen]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity65-residence3-eight-selected57/summary.json
[native]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/boundary65-source-host/unit-b-corrected-native-cold-review.md
[registry]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/boundary65-source-host/unit-b-registry-correction-peer.md
[products]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/boundary65-source-host/unit-b-generated-peer.md
[correction]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/boundary65-source-host/unit-b-completion-correction-source-peer.md
[memo]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/boundary65-source-host/performance-gate66-independent.md
[layout]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/layout65-generator-request.json
[i-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:8479
[i-exact]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c:4353
[i-bind]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval.c:3999
[u-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/ceval.c:871
[u-return]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c:1574
[g-return]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:3746
[g-pc]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Include/internal/pycore_interpframe.h:54
[g-target]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/generated_ir_cases.c.h:25780
[g-identity]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Objects/genobject.c:1222
[g-admit]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:1898
[g-traverse]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Objects/genobject.c:84
[g-detach]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/frame.c:259
[g-remote]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Modules/_remote_debugging/asyncio.c:315
