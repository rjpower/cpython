# Grouped interpreter review62: contracts and evidence

Frozen review date: 2026-11-14. Supports
[interpreter-parity62](../../design/proposals/interpreter-parity62.md).
Documentation only. This review read actual runtime source, retained native
disassembly and retained JSON; a host Python script summarized existing JSON.
No native target, build, generation or benchmark ran for this review. The
coordinator's final creation qualification, screen and bounded diagnostic are
incorporated below. No round61 executable is selected.

## Identities

Canonical documentation: `/home/power/code/cpython/.worktrees/python-jit`.
Its runtime source is stale and was not used as source authority.

Runtime root R:
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees`.

| Name | Actual source |
| --- | --- |
| I | R/call55-integrated, selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0` |
| A | R/parity61-ascent, HEAD `53df1c14debe67a55f092e70ac8dc3c5a1f9458d`, source packet `86886b6f846`; integrated native `410bea361909d37aa4ae998a5084648f117b7200` |
| W | R/call55-ir, creation source `b10667203fee0e2ef31ad366e201b9eb678564b1` |
| U | R/upstream-3a440b6, `3a440b65695615e9fd9912342b79cdd54fced0d2` |

Source anchors below use these trees, not canonical checkout C. E59 is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59`.
Selected ELF is sibling `call-model55/comp57-dictionary-native/python`, SHA256
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.

The CPython [AI policy](https://devguide.python.org/getting-started/ai-tools/)
was read. The coordinator retains responsibility for integration and review.

## Closed cycle and remaining uncertainty

| Packet | Evidence and disposition |
| --- | --- |
| Round59 final binding/builders/Invoke/recipient | Five executable screens lost. Isolated Invoke `5c60e9664faa87c7f6cf1909609554755d83d5ee` removed duplicate clearing and reduced FAST's native frame, yet lost coroutine/logging/spectral. Recipient `14bf4eb1784a8312fd2a6d89f2f63d191547bfe5` moved classification but added binding/action decode. No selected dependency |
| Round60 engine | All eight lost 18.6–60.0%; actual 4,472-byte frame, stack-backed state, one GCC-merged dispatch. No compiler/flag/alignment rescue |
| Round61 producer | 620→356-byte handler and removed new-result DUP/normalization; logging 1.139183, Delta 1.036249. Reject |
| Round61 producer+TEMP | Corrected `53e9a57cfa25665c503010e7a49a557b096d4287`, both 117/79 and 161 focused methods/role pass; actual 69-byte handler. Logging 1.144407, spectral 1.058099 lose both pairs. Reject |
| Round61 ascent | Integrated `410bea361909`, both 117/79, focused 397 methods/role with explicit fixture retry; real composition proved. Generators 1.078980 loses both pairs. Reject |
| Round61 creation | Integrated `4ac529577e2f16bcc4fae76e30c7bd61d04216cb`; both 117/79 builds and 525 focused methods per role pass. Logging 1.301688 and Delta 1.088650 lose both screen pairs. Reject |

The
[trial report](report.md) owns full qualification and all ratios. Producer PMU
diagnostics are whole-process intervals, unlike earlier kernel-only upstream
counters; do not combine them into an attribution. Fewer instructions with
more cycles is possible and observed. No exclusive cache/layout explanation
was established.

Ascent stable screen: comp .979626, Delta .999177, coroutines .951782,
generators 1.078980, logging 1.004998, raytrace 1.010922, spectral .955606,
nqueens .990123. Coroutine improvement is not proof that its recorded workload
used YIELD_DELEGATED; prior monitored coroutine traffic recorded zero of it.

Creation's final ELF SHA256 is
`e03950221a03b6b875ae05b5d6a6c6b9616f1a93f7605f4a2be135a842b833ec`, retained in
`E59/parity61-created4-native`. Runtime production source is unchanged from
packet `b10667203fee0e2ef31ad366e201b9eb678564b1`; subsequent fixes were fixtures.
The final focused gate is 10 new plus 515 existing methods per role. Initial
fixture assumptions about nomination after function mutation, immediate proxy
overwrite release and Python GC callbacks during allocation were corrected;
they are not production repairs. FrameLocalsProxy retains overwritten mortal
locals, so exact immediate death was an invalid owner oracle.

`E59/parity61-created4-eight-selected57` has stable source/native identities:

| Case | Creation / selected57 | Two-pair range where supplied |
| --- | ---: | --- |
| comprehensions | .960055 | .938534–.982070 |
| DeltaBlue | 1.088650 | 1.078661–1.098731 |
| coroutines | .964122 | .955505–.972817 |
| generators | .999804 | .997300–1.002314 |
| logging | 1.301688 | 1.300362–1.303015 |
| raytrace | .994576 | — |
| spectral_norm | .961952 | — |
| nqueens | 1.018826 | — |

Reject the complete executable; no all29, full-system or upstream campaign
follows. Logging does not create coroutines, so the direct helper cannot by
itself explain that loss. Root's bounded matched diagnostic,
`E59/parity61-created4-counter-diagnostic`, uses two balanced pairs of
whole-process user counters: imports, warmup and seven timed calls.

| Case | Instructions / selected57 | Cycles / selected57 |
| --- | ---: | ---: |
| DeltaBlue | 1.0018668 | 1.0864199 |
| logging | 1.0024863 | 1.2414737 |

These near-flat instruction counts and larger cycle losses are not the older
kernel-only upstream intervals and provide no kernel-only attribution.
The cause of the cycle loss remains unresolved. Do not infer a cache/layout
mechanism or reopen padding/compiler searches.

## Source-bound findings

### Prepared call has a surviving consumer

I/Python/regcfg_passes.c:154–310 already folds single-use local loads into
borrowable operand fields. It rejects crossing callbacks/local writes and
preserves source/recovery restrictions. CALL's window format has no ordinary
borrow field for those argument preparations. This explains why the following
adjacencies remain; a new broad copy pass is not the answer.

Host analysis summed `min(load.instruction_visits, call.instruction_visits)`
over adjacent LOAD_FAST_BORROW followed by a warmed CALL label in each retained
`E59/resident-selected57-{case}.json` `before[].words` list:

| Case | Exact Python | Descriptor FAST | Other relevant calls | Total all adjacent CALL labels |
| --- | ---: | ---: | --- | ---: |
| comprehensions | 258,000 | 648,000 | allocation/init 6,000; builtin class 1 | 912,001 |
| DeltaBlue | 183,360 plus no-self 60 | 64,350 | len 52,110; allocation/init 9,030; builtin class 31 | 308,941 |
| logging | no-self 1 | 0 | GENERAL 600,000; builtin class 1 | 600,002 |

These are structural monitored counts, **not admission, natural hit or cost
counts**. Monitoring can deopt the warmed labels. No alias, last-use, recovery,
observation or final-argument proof was applied in this summary. In particular,
comprehension's 108,000 `_any_knobby` exact pairs fill self, not the first
argument-only consumer. The proposed implementation must count actual accepted
and executed sites separately.

Concrete retained images:

- `WidgetTray._add_widgets` word 45: LOAD_FAST_BORROW A19←local 4; word 46:
  CALL_METHOD_DESCRIPTOR_FAST A17/B17/C1/D58, 648,000 visits each.
- The same function word 9: LOAD_FAST_BORROW A14←local 2; word 10:
  CALL_PY_EXACT_ARGS A12/B12/C1/D15, 144,000 each.
- Delta `Strength.weakest_of` word 3: LOAD_FAST_BORROW A6←local 2; word 4:
  CALL_PY_EXACT_ARGS A3/B3/C2/D8, 24,120 each.
- Delta `Variable.add_constraint` word 3: LOAD_FAST_BORROW A4←local 1; word 4:
  CALL_METHOD_DESCRIPTOR_FAST A4095/B2/C1/D14, 21,180 each.
- Logging `bench_silent` has ten final-local→CALL_PY_GENERAL pairs, each 60,000.

I/Python/bytecodes.c:245 produces an owning snapshot even for LOAD_FAST_BORROW.
I/Include/internal/pycore_value_call.h:16–27 requires owned call-window words;
the provider borrows them and acquires anything retained. A prepared-call span
therefore removes dispatch/general publication, not the necessary owner.
I/Python/bytecodes.c:4353–4418 already overlaps exact thread-call arguments and
transfers heap-caller inputs into the arena. Preserve those branches.

I/Python/bytecodes.c:5010–5037 still routes descriptor FAST through the value
dispatcher. That is real work, but the rejected provider rewrite is not this
packet. Its guards, provider, recursion/effect and cleanup remain the existing
CALL consumer. I/Tools/cases_generator/ir_generator.py:168–245 supplies current
ExecutionVariant and CallRoleFamily mechanisms; extending generated CALL input
preparation is preferable to cloning runtime CALL logic manually.

The prepared head must enter the ordinary CALL post-LOAD state on every guard
miss. It must never redispatch the LOAD after acquiring an owner. Instrumenting
the interior site requires invalidating the head; the ordinary CALL remains a
valid independent entry. These are explicit peer obligations, not existing
proved APIs. The existing copy pass's line restriction is descriptive, not a
requirement for the new executing span. Source lines/columns never establish
ownership or eligibility. Preserve each site's failure/recovery identity and
restore both event sites when observation is enabled; admit differing positions
while observation is disabled. Any initial recovery restriction must identify
an unsupported semantic exit, not use equal source positions as its proxy.

Retained selected57 ELF inspection closes the distinction between logical
traffic and existing physical fusion. `_TAIL_CALL_LOAD_FAST_BORROW` is 356
bytes at `0x35c3d0`: the scratch path stores at `0x35c4c7`, loads the next word
at `0x35c4cc`, loads its table target at `0x35c4e6` and jumps at `0x35c4ed`.
`_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` is 249 bytes at `0x37aaa0`: method/self
stores are `0x37ab46`/`0x37ab51`; the next-word load, table load and indirect
jump are `0x37ab56`, `0x37ab62`, `0x37ab69`. There is no call recognition or
composed CALL body on that successful native path. Actual source is
I/Python/generated_ir_cases.c.h:19054 and :20285; its DISPATCH macro is the
ordinary table tail call. Source searches found existing compare/jump fusion,
not an alternate method/CALL span in this selected tree. Historical fusion in
other revisions cannot be credited to this exact control. This proves the
remaining static native mechanism, not a new dynamic hit count.

The larger source consumer is method selection plus invocation. A second host
summary starts at each LOAD_ATTR_METHOD label, permits only intervening
LOAD_FAST/BORROW, LOAD_CONST, LOAD_SMALL_INT_INLINE or LOAD_COMMON_CONSTANT, and
stops at the next CALL. Weight is the minimum semantic visit count over that
span; it applies no admission/effect/alias proof. Results:

| Case | Warmed method/call families | Structural visits |
| --- | --- | ---: |
| comprehensions | WITH_VALUES→exact | 150,000 |
| comprehensions | NO_DICT→descriptor FAST; FAST_WITH_KEYWORDS | 648,000; 6,000 |
| DeltaBlue | WITH_VALUES→exact; descriptor FAST | 1,390,590; 100,980 |
| logging | WITH_VALUES→GENERAL; LAZY_DICT→descriptor FAST | 600,000; 3 |

These overlap the previous table. No sum across tables is meaningful.
Delta `Plan.execute` words 4–5 are METHOD_WITH_VALUES A4/local1 followed by
CALL_PY_EXACT_ARGS discard/B4/C0, 364,800 visits each. The two adjacent
method/exact-call sites in `EqualityConstraint.execute` have 306,000 each.
Comp `_add_widgets` words 8–10 and 44–46 add method selection immediately before
the previously listed final-local preparations. Logging's ten GENERAL pairs
likewise have an immediately preceding METHOD_WITH_VALUES.

I/Python/bytecodes.c:4079–4116 supplies the actual method read: type/version and
managed-values guards, `_PyValueGuarded_Load`, callable DUP, receiver TAKE,
and `IR_CANONICAL_OUTPUTS`. Reusing that source receipt can remove a method
transport/dispatch boundary. The callable owner still becomes f_funcobj on an
exact call; the receiver still supplies an owning argument. Existing overlap
means an argument-home store may already be the final callee store. No new
refcount saving is assumed. Method publication already uses canonical TEMP
stores, so generic normalization/destination checks are not a charge on that
path. The concrete further transport target is its transient callable home:
carry the acquired owner to the shared initializer, with ordinary prefix
publication only on a later CALL miss. Type-version and function-version guards cover
different mutations and must not be conflated in a combined cache.

### Delegated resume has a simpler first outcome consumer

Retained `Tree.__iter__` contains these complete semantic cycles:

| SEND | YIELD_DELEGATED | RESUME | NO_INTERRUPT backedge | Backedge visits |
| ---: | ---: | ---: | ---: | ---: |
| 7 | 8 | 9, argument 2 | 10→7 | 1,507,832 |
| 20 | 21 | 22, argument 2 | 23→20 | 1,430,060 |

I/Python/ceval_macros.h:845–877 already skips ordinary RESUME when the current
eval breaker equals the code instrumentation version, with TLBC/Emscripten
conditions. I/Tools/cases_generator/ir_generator.py:5621–5690 uses it after
generator entry, recursion handling and resume-input publication.
I/Python/bytecodes.c:3641 deliberately omits interrupt checks on this backedge.
An admitted resume-to-SEND execution target can remove one remaining handler
transfer while retaining the original suspended PC and all real child entry.

This is not permission to save SEND as the suspended frame's semantic instr_ptr.
Original RESUME/jump locations and events remain needed when observation turns
on while suspended. The existing equality check is a promising shared entry
gate, not proof that every instrumentation writer already knows the refinement.
Those writer/restoration paths must be ported with the consumer.

I/Objects/codeobject.c:1885 checks basic delegated operands; its normal replay
around 3771 rejects unreachable/unproved delegated entries. I/Include/internal/
pycore_ir.h:464–488 has an immutable semantic plan and executing words already.
Use this common source of admitted facts. A generic per-word plan lookup would
be a new cost, not an existing free continuation proof.

### Ascent proved transport removal and priced its dynamic proof

A/Python/ceval.h:56–95 `delegated_ascent_next` checks parent ownership/type,
redirect, saved SEND_GEN/return PC, successor handler, three operand relations,
receiver identity and vacancy. A/Tools/cases_generator/ir_generator.py:3585–3658
adds the bounded loop and all actual suspension commits. The ordinary selected
frame layout is I/Include/internal/pycore_interpframe_structs.h:30–83: it has a
return PC and recipient, not a preexisting active delegation capability.

`E59/parity61-ascent-diagnostic/native-comparison.txt` records native 556→1,076
bytes, stack 16→32, four operand stack stores/six reloads. It also identifies the
real removal: the canonical outgoing owner stays in r12 across admitted hops;
intermediate publication, load and clear disappear. Neither source branch
count nor that removal establishes a lower total two-hop cost.

GDB actual admission remains valid: depth 8 {8:68}; depth 40 {8:68,16:136};
profiling ordinary 0/instrumented 544; custom iterator {8:136}. Optional reject
site counts were guard visits and are invalid. The new outcome proposal must
remove immutable-geometry reconstruction while retaining mutable guards or
showing exactly which activation invariant replaces them. Child execution can
enable observation, so a SEND-time off switch alone cannot authorize ascent.

The rejected recipient action's [native review](recipient-post-review.md)
quantifies why a universal action field is the wrong first increment: generic
binding three→twelve instructions, scratch publication three action branches,
local four. A targeted outcome must save a semantic transition and price its
whole bind/consume/reset cycle. A field is allowed; a field presented as free
is not.

### Creation, scalar and cache boundaries

W/Python/specialize.c:2280–2350 reuses function-version/CALL binding payload for
direct creation and caches the instrumentation epoch after cold admission.
The source packet separately handles mutable context, owned code/function/args,
and defensive late code/origin checks. Its final qualified executable is
rejected. The direct-creation source alone did not predict that screen.

The coordinator's first runtime fixture observed no GC callback during
allocation, prompting a source audit. The actual selected GIL chain is:

| Source | Reached work |
| --- | --- |
| I/Python/gc.c:2075–2128 | gc_alloc and GC_NewVar allocate, link and initialize |
| I/Python/gc.c:2034–2062 | Link increments the count and schedules GC through the eval breaker; it does not collect |
| I/Include/internal/pycore_object_alloc.h:41–59 | Choose allocation heap, call PyObject_Malloc, restore heap selector |
| I/Objects/obmalloc.c:1819–1832 | Invoke configured allocator within _PyReleaseCheck |
| I/Include/internal/pycore_object.h:111–117,155 | Explicit allocator/reftracer contract: no Python, no raising, preserve current exception; wrapper checks in debug |
| I/Objects/object.c:2850–2906 | Initialize reference state and invoke reference tracer under that same contract |
| W/Objects/genobject.c:1178–1265 | make_gen allocates, initializes cleared/header state, acquires name/qualname, tracks; make_coro computes origin only for nonzero depth |

This corrects review61's blanket allocation-callback premise. At admitted
origin depth zero, the ordinary successful allocator path cannot invoke a
Python GC callback to mutate locals/function code or enable origin tracking.
The source contract also excludes that behavior from allocator/reftracer
hooks; it is not a hypothetical hook escape hatch. Tracemalloc's native wrapper
at I/Python/tracemalloc.c:606–665 records allocations; it is not a Python callback.
Failure, explicit polling/GC and later unknown-owner retirement remain separate
effects, with their own owner states.

W/Python/ceval.c:2024–2105 currently moves CALL inputs into an EXTERNAL owner
prefix before allocation and from there into final heap homes. That is a real
two-move success path; its precaution is not required by an allocation-time
Python callback in the admitted subset. A next allocation-only creation
consumer can preserve the existing owning inputs through allocation and move
them once on successful final initialization. It must use the ordinary failed
CALL prestate when allocation fails, and the shared consuming result writer
before a real destination-local/discard release. The existing production packet
is not changed by this review; its screen rejects that executable independently
of the proposed allocation-only replacement.

Retained `E59/created-generator-diagnostic/native-review.md` identifies source
and native `f8ecd806cdbcfb34e76ad8fab533bf966b785330`, ELF SHA256
`04ebfaf3758751e74081c200fbf1fa69275db0e824aa67cfd6424d91f3625d10`.
The new NO_SELF/supplied-self handlers are 380/412 bytes and call a 2,475-byte
helper. Its 280-byte reservation plus five saved-register pushes and rbp total
328 bytes of explicit stack adjustment. EXTERNAL roots link at
`0x36fea8–0x36fefb`; argument moves appear at `0x36ff20–0x36ff3a` and
`0x3700c0–0x3700dc`; two refresh calls remain. Known code/created objects still
pass numeric-normalization tests at `0x36ff70–0x36ff83` and
`0x370016–0x370029`. Direct known-object owner receipts are a concrete consumer
inside creation, rather than a reason to revive the losing common constant.

The shared return writer changes ordinary RETURN_VALUE from 596 to 628 bytes
and SEND_GEN from 776 to 791; the native review attributes these differences
to cold-tail sharing, with no added ordinary success operations or spills.
YIELD_DELEGATED, RETURN_VALUE_EMPTY and both ordinary exact-call handlers remain
the same size. These facts prevent charging creation's new staging costs to
every ordinary call or claiming the cold changes explain its screen.

The final native review also inspects ordinary GENERAL and binder paths.
Owned-code initializer factoring changes their SIMD/scalar scheduling:
GENERAL has two more static instructions per handler and the binder one more
at each of two initializer sites. There is no added call, guard, spill or
frame growth; initialize_locals is unchanged. These concrete small differences
do not establish the cause of logging's 30% elapsed loss. The updated review's
SHA256 is `5154d063ce1e2879e736ece19b89a91b81c42b8c7330128dd2fb974d18e656c4`.
Future native closure must include shared ordinary consumers, rather than
inspect only new creation symbols or attribute unrelated work to them.

I/Include/internal/pycore_feedback.h:157–178 uses a 16-byte CALL payload after its
header, shared by positional and keyword plans. Do not add a second feedback
store or duplicate version system for prepared CALL. ATTR's existing guarded
shape/result mechanism and BINARY's existing adaptive guard remain authoritative.
No cached result can become a permanent fact after a callback.

I/Tools/cases_generator/ir_generator.py:2451 ScalarStorageGraph supplies reached
physical storage receipts; it is not already a cross-word liveness certificate.
I/Python/bytecodes.c:3349–3352 already has typed compare-and-jump macros, and
SHAPE_LOCAL_TEMP is already a local-borrow/canonical-TEMP variant. Repeating
those proposals would add no consumer. The fixed arithmetic pair remains a
bounded later experiment; the prior finite motifs' comp 1.58%/Delta .88% semantic
dispatch ceilings do not justify a broad scalar framework or a parity claim.

## Review disposition

Proceed to source proposals for allocation-only creation, prepared CALL and
delegated resume. Require
actual generated prefix/effect/exit contracts and complete instrumentation
writer closure before implementation. Defer activation-outcome ascent until
its concrete representation and total-cycle native budget are peered. Retain
the scalar pair as a distinct later consumer, not a prerequisite. Creation is
closed with native evidence and rejected; selected57 remains unchanged.

Matched selected57/upstream PMU still gives comp instructions 1.436416/cycles
1.333179, Delta 1.361424/1.118423 and coroutines 1.307831/1.267788. Generator elapsed
1.372523 is not fresh matched generator PMU. Comp count parity requires removing
30.38% of candidate instructions; no complete category attribution exists.
The proposal does not claim these bounded consumers sum to parity. Root must
select actual winners, retain broad gains and repeat the grouped design cycle
if the hot set remains behind upstream.
