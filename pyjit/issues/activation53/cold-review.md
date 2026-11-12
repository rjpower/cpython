# Activation53 cold review

Reject all three activation53 candidates and retain golden51. The native-only
candidate has a clean ownership boundary and demonstrates a generator gain,
but six-pair confirmation also establishes material DeltaBlue and Richards
losses in every pair. No further rescue variant is recommended.

This review read the actual native-only source diff, component proposals and
post-reviews, rejected result reports, selected call/resume audits, phase census
and coordinator logs. It ran no runtime, debugger, build or performance job.
BASE and E follow [HANDOFF](../../HANDOFF.md) and the packet's evidence paths.

## Decisions supported by this round

[B](completion-result.md) removes the intended native completion boundary yet
loses 3.1% on coroutines and 1.1% on Richards in all six confirmation pairs.
The absence of new ordinary-return guards does not invalidate those losses.
Do not restore it for source sharing, smaller RETURN handlers or a presumed
call/prologue saving. The shared terminal adds selected native text overall.

[Full A](final-homes-result.md) removes temporary construction and has actual
native/IR hits. Its six-pair generator improvement is 7.0%, while coroutines lose
2.8%, logging 24.4% and comprehensions 1.5%, all six pairs losing in each case.
Those results reject the aggregate implementation. They do not identify which
subchange caused each effect. Its separate instruction ratios, 0.985784 for
coroutines and 1.008263 for logging, neither predict elapsed time nor establish
instruction-cache, branch, dependency or other hardware causes. No causal claim
follows by subtracting the native-only result from full A's result.

The native-only candidate is clean source
`779a1568a41652736dbd4c93b968d3381a9693b5`, directly based on selected golden51.
Its runtime diff changes only Objects/call.c, Objects/genobject.c and the internal
generator header, plus a seven-test module. The tri-state constructor owns
storage and input movement; its caller retains its own API completion. It does
not take over an IR caller's recipient, instruction cursor or redirect protocol.
The independent [source/native review](native-construction-post-review.md)
establishes bypass of temporary frame push and creation-only evaluator entry,
with address-independent unchanged ordinary exact CALL and RETURN bodies.
This is a coherent boundary, rather than a reduced wrapper around full A's
universal IR transaction.

The actual source preserves explicit selected-code ownership, valid CLEARED
allocation, NULL physical homes, one shared frame initialization and move/NULL
of inputs before CREATED. It admits only the proved first physical word and
exact supported binding; no generic binder or prologue interpreter is added.
MISS has no effects, ERROR leaves input owners intact for one cleanup, SUCCESS
transfers them once. No callback-capable operation separates first move from
seal. Code and function remain independent owners. Conservative hook/origin,
resource and concurrency exclusions are honest coverage limits.

The extra native provider work is material even though IR bodies are unchanged:
provider text grows 563 to 1085 bytes, C scratch 0x78 to 0xa8, and owning-array
pointer reloads appear in its argument loop. The helper retains allocation,
NULL initialization, code pinning, argument moves and ordinary C-call overhead.
Unchanged IR instruction sequences do not prove unchanged placement or dynamic
workload behavior; neither native-call overhead nor layout is established as
the cause of a particular regression without weighted path evidence.

## Qualification and performance decision

Coordinator release logs report 569 focused tests, plus the seven constructor
tests run separately; debug reports 576 passing tests including that module.
The corresponding focused logs were read. Both roles built 117 modules.
The GDB records show 100000 Tree generator constructors in one measured kernel
loop after warmup and 128 generator plus 128 coroutine constructors in the
separate native API probe. These are direct admitted-route diagnostics, not
timing evidence; the small probe is not the coroutine Fibonacci workload.
These results qualify the bounded mechanism for measurement, not a full suite
or exhaustive allocation/observer/recursion proof.

The two balanced pairs versus golden51 have stable identities and no recorded
campaign/postflight failure:

| Case | Two-pair screen | Six-pair confirmation | Confirmation losses |
| --- | ---: | ---: | ---: |
| Generators | 0.948050 | 0.940807 | 0/6 |
| Coroutines | 1.007538 | 1.004339 | 5/6 |
| DeltaBlue | 1.025437 | 1.024337 | 6/6 |
| Logging silent | 0.916938 | 0.973093 | 1/6 |
| Comprehensions | 0.997135 | 0.995756 | 0/6 |
| Richards | 1.030639 | 1.027155 | 6/6 |

Raw evidence is E/`native-construction-{screen,confirm}-hot-golden`.
The independent confirmation summary/status was read: stable identities, no
campaign/postflight failure, a 5.9% generator gain, and confirmed DeltaBlue 2.4%
and Richards 2.7% regressions. Logging's confirmation range is wide,
0.866070–1.013261. The screen and confirmation are separate campaigns, not
ratios to multiply. The repeatable breadth losses reject the lane despite its
generator benefit; no native-only all29 or upstream claim is established.
The coordinator's planned fresh golden51/upstream comparison concerns the
unchanged selected runtime and cannot qualify this rejected candidate.
Native-only coverage cannot be presented as solving recursive coroutine construction:
that workload's 242784 full-A hits were exact IR calls, outside this candidate.
Do not silently replace the original coroutine goal with the generator result.

## What remains relative to upstream

The prior direct selected/upstream ratios, about 1.446 generators, 1.268
coroutines and 1.160 DeltaBlue, are historical context, not current candidate
measurements. Upstream's collapsed evaluator also prevents comparing its
per-handler symbols directly with this interpreter's separate handlers.

- Ordinary exact calls already overlap arguments, reserve once, initialize once
  and return canonical values inline. Their remaining costs include the richer
  14-word header versus upstream's 10, actual value classification, code-owner
  handling, recipient selection and callback-safe retirement. Upstream pays
  argument copying and local initialization avoided here. No evidence assigns
  the whole DeltaBlue gap to calls; its other operations still matter.
- Generators perform about 1.569 million resume/yield pairs for 100000 Tree
  creations/completions. The remaining dominant-frequency boundary is actual
  descent/suspension: exception links, current frame, recursion, input/result
  homes, guards, poll and dispatch. Heap allocation occurs at creation, not at
  each resume. Round52's failed forwarding and round53's failed B rule out
  treating another shared terminal or chain cache as demonstrated progress.
- Recursive coroutine Fibonacci starts/completes without Python-visible
  suspension events. It still constructs, enters and retires real heap
  activations through SEND. Both A and B removed real work and failed its time
  gate. Native-only constructor hits in an API probe cannot explain or remove
  that workload's exact-IR construction, GET_AWAITABLE, SEND, body and retirement
  costs. Hidden constants are initialized at creation, not each resume.
- Register IR homes remain memory, not hardware registers. Repeated instruction
  decoding, dispatch, canonical-value classification and home traffic span
  workloads. The absent method/tracing compiler is an architectural gap; the
  register encoding alone does not realize those compiler savings.

## Ranked next options, for a new decision

These are alternatives to investigate, not an implementation queue or permission
to continue round53 variants. Each starts from the selected runtime. Require a
weighted native cost case before choosing one; if none meets it, stop rather
than add a new universal guard or persistent continuation descriptor.

1. **Contract eager activation state around existing entry authority.** Audit
   every initialized header field and separate independently owned semantic
   state from executable-derived or observer-only state. A concrete candidate
   must eliminate named stores/loads or shrink physical frame size across
   ordinary and generative construction, updating all consumers together;
   merely factoring `_PyFrame_Initialize` has already been done. Use existing
   provider/code authority and normal cold observation entry, with no new
   per-call eligibility branch. Keep independent function/code owners and real
   current-frame/exception/continuation state eager. First measure weighted
   header writes, later reads and cold-observer reconstruction frequency on
   DeltaBlue, Richards, recursive coroutines and generators. Reject any field
   removal whose derived reloads add more weighted work, or whose observer
   reconstruction needs a new common-path check. Native output must show the
   exact removed operations and compensating loads. Falsifiable gate: repeatable
   ordinary-call/coroutine improvement with acceptable generator, logging and
   all29 breadth; source store counts alone are insufficient.

2. **Carry existing resume proofs through a complete SEND/YIELD transition.**
   First count actual FOR versus SEND hits, CREATED versus SUSPENDED entries,
   local/discard/overlap inputs and callback-capable retirements. Inspect the
   corresponding native paths for a specific repeated load, classification or
   intermediate dispatch already made unnecessary by existing admission or
   producer facts. Emit the proven operation directly under that authority;
   add no universal guard, chain cache, resume descriptor or extra per-resume
   state. Every real frame/exception/recursion transition remains valid before
   input retirement or a poll, and a callback ends any carried fact it can
   invalidate. If the census/native audit finds only required work or compiler
   CSE has already removed the duplication, there is no candidate. Falsifiable
   gate: less dynamic work on the 1.569 million transition pairs and confirmed
   unchanged-kernel generator benefit, with no material coroutine, ordinary
   call, logging or all29 regression. Another renamed wrapper or revived
   round52 forwarding path fails the work-removal gate.

3. **Build a bounded shared method-compiler region on the current owning ABI.**
   Target actual hot arithmetic/attribute/call regions selected by existing
   feedback. Keep transient values in hardware registers and fold repeated
   decoding/classification inside a guarded compiled region; place its guards
   at compiled entry or actual invalidation points, not on every interpreted
   call. Initially preserve real owning frames and heap generators: allocation
   elision would need a separate escape/materialization contract and is not an
   implied part of this option. At callbacks, polls, exceptions and exits,
   recover explicit owners, exact semantic continuation and observer state.
   Share that recovery/backend contract with the later tracing compiler rather
   than introducing a second value or frame representation. Before broad
   implementation, demonstrate one end-to-end region with actual entry counts,
   fewer dispatch/home operations, checked invalidation and exception recovery.
   Falsifiable gate: net steady-state improvement including guards and exits on
   ordinary-call and coroutine workloads, generator suspension correctness,
   bounded compile/warmup cost, and all29/direct-upstream evidence. A compiler
   skeleton or a zero-box counter is not a performance result. This ranks third
   as the largest implementation commitment, not because another local terminal
   is known to close the gap first.

The selected runtime should absorb only measured improvements. Neither successful
ownership refactoring nor fewer instructions is enough to reopen a rejected
lane, and no proposed next option establishes an upstream speedup in advance.
