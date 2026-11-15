# Grouped architecture review63

Status: final source/evidence review, 2026-11-14, including method62. The
[proposal](../../design/proposals/interpreter-parity63.md) ranks five options
and sets their implementation gates. This review changes only that proposal and
this ledger. Selected57 stays unchanged. No build, target execution, benchmark,
runtime source, selected contract or sealed evidence was changed.

The [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/)
and canonical project guidance were read before proposing changes. The scope is
holistic interpreter/IR architecture, not approval of a current patch.

## Identities and evidence boundaries

- `I`: selected source `R/call55-integrated`, HEAD
  `0dcba75bb3df40c32137db453b001f7e8f101de0`; selected ELF SHA256
  `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
- `U`: upstream source `R/upstream-3a440b6`, HEAD
  `3a440b65695615e9fd9912342b79cdd54fced0d2`.
- `R`, `BASE` and `E59` have the canonical meanings in
  [HANDOFF](../../HANDOFF.md). This checkout's runtime is stale and was not used.
- Current prepared integration is `a3cbad0637efe6498533d31abf074c0e544fa964`.
  Its native and screen evidence is retained independently of this review.
- Method62 source is frozen at `a67bdfc0b1da80607d1915840affb4f4a911574b`.
  Native integration and the hot-eight screen are complete. It retains only method heads
  376–378; unused final-local heads are removed. The acquired owner now comes
  from the actual reached DUP/TAKE receipt. Four host methods and eight source
  AST modules pass, as reported by root. Both native roles now pass all 227
  focused methods. Bounded unobserved coverage is Delta 43 pending→42 active
  plus one pending, and ray eight pending plus two active→ten active;
  kernel-filtered first-hit probes confirm both native paths. The executable
  is rejected by the completed hot-eight screen below.

Host-only reading of source and retained JSON supplied the additional census
and scale calculations below, using `uv run --no-project python -B`. Monitored
word visits are not naturally executing specialization hits. Static source
removal is not measured dynamic work; whole-process counters are not the
matched kernel-only upstream intervals.

## What round62 actually establishes

| Packet | Mechanism and qualification | Measured decision supplied by coordinator |
| --- | --- | --- |
| Allocation-only creation | Both 117/79 builds and 526 focused methods per role; helper 2,475→1,382 bytes, explicit stack 328→168; ordinary GENERAL scheduling/count restored | Coroutines .955934 confirmation; all29 mean 1.004907, 12 wins; logging 1.020099, spectral 1.053241, deepcopy_memo 1.079159; reject |
| Delegated resume | Both 117/79 and 397 focused methods per role; full SEND continuation 29→28 instructions, indirect branches 2→1, total branches 2→4; reviewed text +2,496 bytes | Generator .956959, logging 1.203048, Delta 1.065177, spectral 1.092709; reject |
| Prepared final local→CALL | Both 117/79 and 227 focused methods per role; positive per-home admission; actual comp 7 cold/3 active and Delta 56/41; first-hit probes establish real kernel entry | Comp .948255, Delta 1.021921, coro .964114, gen 1.023161, logging 1.182918, ray .988174, spectral 1.012265, nq 1.019683; reject |
| Method→exact CALL | Actual acquired method owner enters shared initializer; receiver owner and one callable-home clear remain; both roles pass 227 focused methods and native first-hit probes confirm Delta/ray entry | Comp .965035, Delta 1.032009, coro .998281, gen .998863, logging 1.228535, ray .996699, spectral 1.003543, nq 1.008903; reject |

Prepared native review: all four heads retain one DUP/store, remove generic
destination handling and intermediate dispatch, but retain next-word/feedback
decode. Descriptor heads add 16 stack bytes; exact heads retain 64. Four heads
total 5,210 bytes; ELF text grows 6,912. Nine ordinary handlers have unchanged
address-normalized instruction sequences; corresponding rodata bytes match.
Sources: `E59/parity62-prepared4-native/native-review.md`,
`E59/prepared-coverage/*.json`, and
`E59/parity62-prepared4-eight-selected57`.

Method native review: active head 1,151 bytes, 64-byte frame and 30 static
rbp-memory instructions, the latter equal to the ordinary exact CALL body
count but not a dynamic path count. The acquired callable enters `f_funcobj`
directly; no callable B store/load/final clear or intermediate dispatch remains.
One B=NULL write and the self store remain, as do generic local classification,
next-word/feedback loads and an early heap test. ELF text grows 3,264 bytes.
Eight ordinary handlers retain their normalized instructions, sizes and stacks;
pending 376/377 table entries alias the actual old functions, and unused
370–374 entries are UNKNOWN. Native peer disposition is GO for the mechanism,
not for selection. Source: `E59/parity62-method3-native/native-review.md`.

The final method screen has 32 rows, stable identities and no errors.
Comp's pair is .942478/.988132; Delta 1.030685/1.033335; logging
1.226871/1.230202. Coro/gen/ray are mixed; spectral/nq lose both pairs.
Source: `E59/parity62-method3-eight-selected57`. Root rejects standalone and
does not spend all29, upstream, full diagnostics or repeat timings on this
loser. All four round62 mechanisms have valid source/native evidence, but
none supplies a selected executable. No exclusive cause of the repeated
unrelated cycle losses follows from that result.

The earlier common producer/TEMP native path became 69 bytes and still lost.
Round60's same-GCC engine has one merged indirect jump, a 4,472-byte native
frame, common state accessed through stack slots and about 55 KiB more ELF
text. All eight cases lost 18–60%. These constrain a new execution design;
they do not rule out every bounded interpreter domain or prove one micro-
architectural cause. See [engine review](engine-post-review.md).

## Cycle divergence remains unexplained

Root's `E59/parity62-prepared4-branch-diagnostic` uses two balanced pairs with
stable source/native identities, whole-process imports, warmup and seven kernel
calls. Four events run without multiplexing:

| Case | Cycles | Instructions | Branches | Branch misses |
| --- | ---: | ---: | ---: | ---: |
| logging_silent | 1.147451 | 1.000317 | 1.000491 | .996456 |
| comprehensions | .970045 | .988185 | .993012 | .962041 |

Logging cycle ratios are 1.144259 and 1.150652; branch-miss counts are lower
in both pairs. Thus an aggregate increase in retired branch misses cannot
explain this particular loss. These counters do not measure miss latency,
front-end delivery, dependency stalls or the complete spatial execution path.

Root's follow-up `parity62-prepared4-fetch-diagnostic` multiplexes six events
at about 83% running. Approximate logging ratios are cycles 1.1514,
instructions .9993, IC fills from L2 1.0213, system fills .9621,
`ic_fetch_stall.any` 1.0211 and `dq_empty` .8052. Scaled exploratory counts
do not establish a 15% front-end penalty or precise attribution. No further
counter fishing, layout or compiler search follows.

**Measured:** cycles can worsen substantially with near-flat instructions and
unchanged inspected ordinary bodies. **Static inference:** a source change can
alter the complete executable and execution context outside those normalized
bodies. **Hypotheses:** front-end residency/delivery, target interactions and
other pipeline/resource effects may matter. None is selected as the cause.
Explicitly reducing duplicate hot code and live state is a clean testable
remedy; it earns no performance credit before the unchanged broad screen.

## Selected source findings

| Source in I | Actual contract and architectural implication |
| --- | --- |
| `Include/internal/pycore_regcfg.h:15–48` | Fresh temporaries are single-definition within fallthrough regions; joins use canonical stack-position temporaries without phi bindings. Arbitrary cross-edge value identity and native caching are not already implemented. |
| `Python/regcfg_passes.c:154–310` | Copy propagation folds a local load into a borrowable input, stopping at writes, Python-running effects and source/recovery restrictions. More broad COPY elimination is not a new lane; CALL windows and physical transport remain. |
| `Python/ceval_macros.h:64–74,519–626` | Five main values cross the NCSR ABI: frame, slots, thread, PC, word. Input acquisition and generic publication still classify local/scratch/discard and canonical representation unless an existing variant removes them. |
| `Python/bytecodes.c:4353–4418` | Exact thread calls already overlap argument storage, consume callable into f_funcobj and enter child inline. Heap callers move arguments. A borrowed-cell proposal cannot claim these copies again. |
| `Include/internal/pycore_interpframe.h:478–514` | Ordinary initialization establishes code/function owners, frame links, execution and recipient state and constants. A slimmer activation must account for all later readers; changing a header write is not a whole-call saving. |
| `Include/internal/pycore_interpframe.h:700–755` and `Python/pystate.c:3130–3207` | Frame headers and value homes grow from opposite sides of an arena chunk. Pop asserts NULL over the departing value suffix and restores a fixed header extent. Await-only raw records must be placed on the header side and their full extent included in pop/chunk restoration. |
| `Python/frame.c:420–499` | Clearing closes actual owned cells; fast clearing stops before a callback-capable release. Borrowed parameters require real clear/exception changes, not only a different argument store. |
| `Python/gc.c:483–522` | Ordinary lifecycle scanning still uses initialized NULL control; special error/created views are limited. Borrowed bits and uninitialized absent homes cannot be introduced without changing readers. |
| `Tools/cases_generator/ir_generator.py:5621–5690` | Generator entry publishes recipient/resume input, links current/previous and exception state, accounts recursion, and performs the existing RESUME guard. A fast chain must preserve these semantics even if it removes physical visits. |
| `Python/bytecodes.c:3840–3879` | List iteration has actual type/bounds/read/acquire work. Canonical result transport and cursor proof must come from reached producer/protocol facts. Mere output type assertions are insufficient. |
| `Include/internal/pycore_value_call.h:16–27,108–165` | Call windows own strong snapshots; value providers borrow and acquire what they retain. The dispatcher chooses value/object/type slots. A trusted native boundary can remove rediscovery but not callback ownership. |
| `Python/bytecodes.c:5270–5295` | RETURN_GENERATOR allocates an object, copies the entered frame, pops the thread frame and returns the object. An immediate await later reenters the heap frame. Removing this whole cycle differs from the rejected direct-allocation helper. |
| `Objects/frameobject.c:3915–3927,4541` | `frame.f_generator` and `PyFrame_GetGenerator` derive the object from frame ownership. A private await activation requires promotion here; it cannot use the ordinary embedded-generator cast. |
| `U/Python/bytecodes.c:5290–5315` | Upstream's admitted descriptor FAST path uses a narrow checked helper. This is a concrete boundary comparison, not permission to repeat the rejected round57 provider bundle. |

Frame exposure and mutation also reach `Python/frame.c:_PyFrame_MakeAndSetFrameObject`
and `Objects/frameobject.c` locals-proxy writers. A persistent borrow design must
close those paths, preexisting frame objects, debug APIs, GC, copying and error
roots. Keeping one replacement snapshot owner makes its supposed refcount
saving zero.

The selected frame's `visited` bit has no C read sites in the whole source
tree. Its five writes set zero in frame copy/init/trampoline, interpreter entry
and the thread sentinel. This is a concrete possible bit for the narrowly
scoped borrowed-self mode, subject to target-tree peer review. Adding a bit
beside the existing debug `lltrace:5` can grow the header; using phase_state's
spare bit is unsafe because many ordinary paths reset that field wholesale.
The locals proxy's `f_overwritten_fast_locals` retention is real, but does not
close every external writer or replace the proposed promotion protocol.

## Coverage and scale readback

Host sums of `before[].words[].instruction_visits` in
`E59/resident-selected57-{case}.json` reproduce the existing
[coverage ledger](resident-coverage.md). They assign semantic visits to
pre-monitor adaptive labels, not unobserved hits:

| Case | Total semantic visits | Source concentrations relevant to architecture |
| --- | ---: | --- |
| comp, 6,000 trays | 13,674,014 | `_add_widgets` 9,696,000; `_any_knobby` 1,716,000; generator expression 1,218,000; predicate 936,000 |
| Delta, 30 loops | 14,679,757 | `BinaryConstraint.output` 2,469,060; `EqualityConstraint.execute` 2,448,000; `BinaryConstraint.input` 2,107,260; `Plan.execute` 1,480,200 |
| generators, 2 loops | 11,813,690 | `Tree.__iter__` 11,413,666; SEND/delegated-yield/backedge form the repeated depth cycle |
| coroutines, 4 loops | 9,711,441 | `fibonacci` 9,711,372; 971,141 exact no-self calls and 971,136 SEND/awaitable operations each |
| logging, 60,000 loops | 8,520,033 | `Logger.debug` 3,600,000; `Logger.isEnabledFor` 3,000,000; real GENERAL/exact call paths remain sentinels |

The actual comprehension inner mapping loop is `_add_widgets` words 43–48:
list step into local `dwid`, method selection from `id_to_widget`, argument
snapshot, descriptor call, direct consuming append, backedge. Its six words
are fewer than upstream's eight, yet native work is higher. This is the unit
that a transport/execution proposal must cost. The adjacent local→CALL saving
has already been measured and rejected as an executable. A proposal covering
only that edge or only its attribute neighbor cannot claim the whole loop.

The proposed five source-role consumers have disjoint structural weights:

| Role signature | Comp | Delta |
| --- | ---: | ---: |
| List step to local | 1,686,000 | 0 |
| Managed-values/no-dict method from local to scratch pair | 912,000 | 1,451,610 |
| Append consumes scratch | 864,000 | 0 |
| Nonmethod class read with A=B scratch | 0 | 942,090 |
| Shape store with local receiver and scratch value | 6,000 | 91,530 |

These filters use visible local geometry from the retained record; common
admission must distinguish real scratch from hidden constant homes and prove
actual pre/poststate. They cover 25.4%/16.9% of monitored words, not native
instructions. The proposal's illustrative 30/8/8/12 instructions per comp
list/method/append/store gives about 10,810 per tray, 3.9% of selected count.
This is a stated hypothetical scale calculation, not a native count, guaranteed
ceiling or expected gain. It replaces an unjustified hard 10% admission target.
Useful smaller broad winners remain eligible for selection.

The new coroutine option has a much larger complete operation to remove.
Retained `fibonacci` words 8–11 and 17–20 are CALL→GET_AWAITABLE→None→SEND;
each executes 485,568 monitored visits. Both corresponding delegated yield
words have zero visits. Deferred immediate-await activation can avoid heap
object/frame creation, copying, creation return and reentry until actual
suspension or observation. The 971,136 structural cycles are not natural
admission. First scope retains owning args and a real exception record in the
thread arena; it never writes a fake coroutine PyValue into a canonical home.
Storage residence, activation kind, promotion and deferred arena reclamation
are explicit peer obligations, including ordinary nested calls and raw C
frame aliases. Recursive immediate awaits must not themselves force promotion.
The proposal now names the minimum source reader/transition closure. GC visits
actual private roots without allocating during traversal. Cross-thread frame
queries while a target is inside a native call still require stable aliases or
a complete relocation/refresh protocol; GIL-only scope does not solve that
problem. Mid-activation relocation remains the main unresolved layout risk.

The narrower borrowed-self option has 1,280,190 structural Delta local-method
zero-explicit-argument exact pairs, 1,026,471 in ray, and none in comp/logging.
The common callee read-only proof is still needed. Even a hypothetical twenty
instructions per Delta pair is only about 853,460 per workload loop before
new costs, about 6.1% of the first-pair instruction deficit below. It is an
incremental call-lifetime option, not a coroutine or whole-parity explanation.

Per-loop first-pair PMU readback from
`BASE/pyjit-link-wave1/comprehensions57/dictionary-perf-stat/`, dividing each
`*-perf.json` event count by its matching `*-child.json:hot_loops`:

| Kernel | Selected instructions | Upstream instructions | Difference |
| --- | ---: | ---: | ---: |
| comprehensions | 278,530.60 | 193,879.56 | 84,651.04 |
| DeltaBlue | 53,014,240.31 | 38,941,736.64 | 14,072,503.67 |
| coroutines | 385,373,460.06 | 294,262,051.86 | 91,111,408.20 |

These are a transparent single-pair scale check. The published three-pair
ratios remain the comparison result. No exact savings follow from combining
these PMU intervals with the separate monitored census. In particular, removing
one mortal owner pair per Python call cannot be assigned the full Delta gap;
coroutine inline integers do not pay that mortal owner pair in the first place.

## Review disposition and unfinished proof

Prefer broad static transport lowering before adding another execution engine.
It can consume positive per-home facts without inheriting unfinished bitmap
or exceptional-transfer work. Require whole source paths, actual native
classification removal and bounded duplicate text. A general use/def/effect
model earns implementation only through those consumers.

A deferred exact-coroutine await activation is the second option and the
independent source-design lane beside ordinary transport lowering. It targets
the actual zero-outward-yield recursive await paths and requires a complete
promotion/error/observer protocol before runtime work. It is not another
direct-creation helper and need not enlarge every ordinary frame header.

A compact effect-bounded engine remains the third option because it can
remove repeated physical boundaries beyond pairs. It must have a concrete
whole-domain budget and a small same-GCC native artifact with resident state;
the proposal sets rejection caps. It is not an instruction to implement an
arbitrary region framework or revive the monolithic engine.

The fourth option is a retained delegation spine.
Its advantage must survive full effective-running/exception/recursion/observer
semantics. That reader closure is not yet implemented or proved; this review
does not authorize a shortcut that only updates SEND and YIELD. Persistent
argument borrowing is likewise a complete experimental frame ABI, ranked fifth
and narrowed to read-only self, because promotion and nested owning calls can
consume its entire saving. The low-budget trusted provider rewrite is not a
sixth lane. Exact provider facts may later extend rank 3's effect cut, but
METH_VALUE alone does not prove no Python callbacks and no mixed-key table
assumption is admitted.

No five options have an established combined budget for parity. Method62 is
another valid bounded ownership consumer with a losing broad screen, not an
explanation for 30.38% of comp native instructions. The coordinator keeps
selected57 and selects future increments only after their actual broad results.
