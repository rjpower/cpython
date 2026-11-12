# Activation creation, resume and completion audit

Static proposal, 2026-11-12. Selected source R is
`BASE/worktrees/round51-golden`, commit
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`; U is
`BASE/worktrees/upstream-3a440b6`, commit `3a440b65695`.
BASE is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Canonical checkout C supplies documentation, not measured C runtime sources.
Applicable guidance, the official [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
the three canonical call/interpreter/IR designs and round51/round52 reports,
next plans and rejected-source evidence were read. No runtime changes, builds,
execution, tests or measurements were performed. One existing release ELF was
read with objdump. This file owns proposals, not admission.

## Findings that constrain the thesis

The useful common abstraction is **choose the final owning activation storage,
then perform typed input/result transfers through the actual frame chain**.
The current interpreter already uses iterative handler transfers for exact
Python calls and generator resumes. Replacing them with an iterative loop does
not by itself remove nested C evaluator entry. The candidate must remove actual
storage or transfer work across creation, descent and completion; yield routing
alone already failed in round52.

| Phase | Selected R | Upstream U | Potential work removal |
| --- | --- | --- | --- |
| Exact call | `Python/bytecodes.c:4331–4397`: choose overlapping thread homes, reserve once, initialize once; heap callers move arguments to thread storage | Same broad frame-entry architecture, with stack homes | Preserve round51's one reservation/header. Avoid adding a universal transaction to ordinary CALL. |
| Generator/coroutine construction | `Python/bytecodes.c:5256–5284`: allocate object, move whole frame, pop temporary thread frame | `Python/bytecodes.c:5806–5832`: also allocate object, copy frame, pop temporary thread frame | Choosing final heap homes before initialization could remove the temporary frame lifecycle and move, in both designs. |
| Construction copy | `Include/internal/pycore_interpframe.h:239–279`: copy header, then copy and NULL every physical slot | Same file `:134–161`: copy through current stackpointer; popped source homes are left alone | Register storage does extra loads/stores for unused scratch and clears source homes. Its initialized frontier requires the cleared source; dropping stores alone is invalid. |
| Exact child resume | Emitter `Tools/cases_generator/ir_generator.py:3915–3959`, generated `Python/generated_ir_cases.c.h:16875–16950`: admission, detach input, save parent recipient/PC, link frame/exception chain, recursion entry, input publication, resume poll and dispatch | `Python/bytecodes.c:1725–1746`: state/type guards, stack push, exception links, return offset and inlined frame dispatch | Already enters the same evaluator directly. New descent routing must name redundant operations it actually eliminates. |
| Yield | `Python/bytecodes.c:1708–1747`: input recipient, exception/current-frame unlink, recursion exit, suspension state, canonical output transfer | `Python/bytecodes.c:1852–1892`: essentially the same frame/exception/recursion transitions, without register recipient metadata | Required state changes remain per actual activation. Repeated eligibility checks are not a replacement for them. |
| Return | `Python/bytecodes.c:1464–1491`, emitter `:3889–3913`: C-held result survives unlink/clear, ordinary return inline; generator completion calls adapter | `Python/bytecodes.c:1574–1590`: clear child, load return offset, deliver output | Share semantic transfer primitives without forcing ordinary return through generator recovery. |
| Generator completion | `Python/ceval.c:2987–3122`: immutable SEND/FOR decode, canonical typed completion, detach window, publish output/exit, observation and owner retirement | Returns to the upstream stack completion route through saved return offset | R already specializes canonical SEND/FOR kinds. More inlining or another decoder name alone is not a new semantic saving. |

R's generated SEND_GEN directly calls `DISPATCH()` after publishing the child.
The apparent `_PUSH_FRAME` at the DSL macro tail is not an additional executed
push after that terminal. Likewise, generators are allocated at creation, not
again on every resume. Their constants are filled during initialization, then
moved at creation; resume does not refill constants.

## Creation is a real whole-activation opportunity

R `_PyFrame_Initialize` (`Include/internal/pycore_interpframe.h:300–333`)
installs function and separate code owners, globals/builtins, locals, observation
state, IR bases, return state, owner and constant homes. `_PyFrame_Copy`
duplicates that metadata into the new generator frame and moves every home.
For N physical homes the copy loop performs N source loads, N destination stores
and N source-NULL stores, including NULL scratch. These are static operation
counts, not measured instruction counts. Thread reservation/frontier work and
thread push/pop accompany this temporary location.

A shared activation constructor could accept a storage plan: overlapping thread
homes for ordinary functions, owned heap homes for generative functions. Binding,
code/function ownership and initialization then target that final home once.
The creation prologue still executes normally. RETURN_GENERATOR would seal and
return the already constructed object rather than allocate and move it. Exact,
known-plan and generic binders must use the same final-home interface; a separate
coroutine-only benchmark shortcut would not establish this contract.

This is broader than an allocator substitution. Allocation is currently after
creation-prologue operations. `Python/codegen.c:5237–5254` deliberately places
GET_ITER/GET_AITER before RETURN_GENERATOR in generator expressions; closure
setup also precedes normal execution. Therefore a proposed early heap object
needs an explicit constructing state and valid GC/reentry rules, rather than
pretending an uninitialized object is suspended or executable. Frame
completeness currently derives partly from owner
(`Include/internal/pycore_interpframe.h:343–359`): changing owner to GENERATOR
early would incorrectly make the prologue complete without another rule.

The construction transaction must specify:

- Caller arguments remain owning until allocation/binding succeeds. A failure
  consumes/retains them exactly once according to the binder's existing contract.
- Heap homes are canonical or NULL before any GC-visible publication. The
  constructing frame is current with valid code/function anchors before a
  prologue callback can observe ancestors or run a finalizer.
- A hidden in-construction object cannot run the ordinary never-awaited warning
  or generator close protocol when setup fails. Define its owner and teardown
  before exposing it to callback-capable retirement.
- Pre-RETURN_GENERATOR effects, errors, tracing and recursion accounting retain
  their semantic order. The object allocation's earlier timing requires explicit
  review of GC callbacks and allocation-failure behavior.
- Coroutine origin tracking currently walks past the incomplete creation frame
  (`Objects/genobject.c:1201–1218`). Preserve the caller-origin result when the
  storage changes. Async-generator hooks/finalizers need the same care.
- Allocate and initialize from the pinned executing code, not a later reread of
  mutable `function.__code__`. Existing `_Py_MakeCoro`/`make_gen` use the function
  (`Objects/genobject.c:1146–1174`); the new interface must not assume that a
  callback-free exact admission guard remains fresh through a prologue.

This removes more work than specializing a full-frame copy, but is the larger
semantic option. Measure creation weight separately for generators and
completion-only recursive coroutines before selecting it. If the construction
state/observation contract is out of scope, retain current creation explicitly;
do not advertise a resume-only change as solving this cost.

## Descent and completion should share transactions, not preparation

There are three useful typed outcomes: yield to a parent's SEND/FOR recipient,
complete a parent's SEND/FOR window, and return to an ordinary call recipient.
All hold a canonical owner across retirement, publish the actual continuation
before callback-capable closes, and reload any callback-selected PC afterward.
They have different retained/retired homes. A common implementation can expose
those shared commit/retirement operations while keeping those differences
literal at each entry.

R already has the important partial specialization:
`_PyEval_CompleteGeneratorReturn` decodes the immutable A/B/signed-D fields and
instantiates `complete_step_impl` with canonical input and literal SEND/FOR kind.
No generic value publication remains on that path. The saved continuation
already handles assembler far-exit adapters. Do not propose this existing work
as new, or cache another per-frame completion descriptor on every resume merely
to avoid decoding it once at completion. Long-lived generators can resume many
times before completing.

Static inspection of selected `round51-golden-release/python` finds the adapter
at `0x377e80`; its prologue saves rbp, r15, r14, r13, r12 and rbx and reserves
0x28 bytes. `0x377eb5–0x377f17` obtains the site, computes the word index, reads
the immutable word and distinguishes SEND/FOR. Completion code is already
inlined into that adapter; there is no second mandatory call to
`complete_step_impl`. Calls into `_PyIR_ObserveStep` and deallocation are on
their actual branches. This proves the native boundary and metadata decode
exist, not their dynamic cost or removability.

A bounded alternative for review is to put a minimal unobserved canonical
completion entry next to the generated return terminal, sharing owner-commit
and retirement source with the existing cold observer/error tail. Admission
must happen after child cleanup, since cleanup may install hooks or redirect
the parent. This can remove the adapter call/prologue/status-return and repeated
state loads on admitted completions. It cannot remove immutable operands,
window detachment, receiver retirement or late PC reload. Avoid duplicating the
full body into every return variant; inspect code size and actual generated
machine paths before interpreting it as a win. Ordinary CALL completion keeps
its current inline recipient path.

For descent, use the existing exact admission and explicit input transaction.
R's initial non-None guard is additional versus U, but protects observable
generator behavior and cannot simply be removed. Input may be IR_NO_DST or a
local with a displaced owner. `_PyFrame_SetReturnValue` can therefore close a
last mortal owner. Keep the current order: detach the parent transfer cell,
publish child links/exception/recursion state, then dispose input. No borrowed
delegate or pending sent value may survive outside declared owners.

### Explicit continuation kind: feasible bounded variant

Add a plain `uint8_t return_kind` to the caller's existing continuation fields:
CALL, SEND, FOR, INTERPRETER_EXIT. Keep `return_slot` untagged and `return_pc`
a real executable pointer. The selected x86-64 header has owner at 0x6c,
bitfields at 0x6d and padding before owned_slots at 0x70; one byte can fit that
padding on this configuration. Verify all supported layouts rather than assume
the byte is universally free.

The existing call-recipient commit writes CALL; exact SEND/FOR entry writes its
literal kind with the already required return_slot/return_pc; the interpreter
entry initializes INTERPRETER_EXIT. Generic paths that really push a VM child
must use the same commit. Construction copies/initialization, frame takeover,
exceptional exits and observer redirection all require an explicit rule. A
suspended generator's incoming send recipient is separate from the continuation
it most recently used to call a child; resume input must not interpret stale
return_kind as an input protocol.

Return dispatch can then select its transaction from the receiving frame's
declared kind instead of the dying frame's owner plus immutable handler-family
lookup. SEND/FOR still read immutable A/B/D at completion for window,
destination and exit. Thus this bounded version removes the dying-owner test,
interpreter-owner discrimination and handler-family table lookup/dispatch from
the completed-return route, while adding one literal kind store at child entry.
It does not remove the full operand decode or any owner retirement. The native
check must establish whether kind can be packed with an existing header store;
source proximity is not sufficient. Ordinary returns should still reach their
present short recipient transfer after a cheap kind discrimination.

A larger completion record could also save both yield and completion
recipients, window and done PC. That would avoid the A/B/D decode, but populate
and retain more state on every call/resume and potentially enlarge every frame.
It is not a free consequence of adding a kind. Bound that separate option by
measured resumes-per-completion and actual generated stores/header size. A
kind-only variant is feasible and small enough to test composition with final
homes; it is unlikely to explain the whole upstream gap by itself.

An actual shared descent/suspension design can remove repeated routing dispatch
and intermediate owner publication only within a positively proved nonescaping
region. Every real parent remains executing/visible while a leaf callback runs.
Hooks, eval-breaker changes and escaping operations end that region and require
the full current state. Persistently caching a chain or assuming an earlier
observation check stays valid is outside this proposal. A new loop with all
current state operations and added eligibility guards has no work-removal case.

## Rejected shortcuts and admission evidence

Round51 option5 already changed normal generator return to local-only cleanup
under the empty-scratch proof; it was semantically reviewed but rejected after
a confirmed DeltaBlue regression. R still clears generator physical homes in
`Python/frame.c:198–238`. Reintroducing that flag/helper route is not a fresh
whole-activation thesis. Preserve escaped-frame takeover before any shortened
clear and generic close/throw/unwind full cleanup.

Round52 yield forwarding removed intermediate owner publication and SUSPEND
dispatch but retained resume descent, real frame transitions and completion.
Its coroutine Fibonacci workload completes awaits without yielding. Its entry
revision also moved wrapper metadata cold and still lost. Retain those negative
results; do not combine their ratios or treat sample percentages as removable
time budgets.

Required diagnostic evidence is creation/resume/yield/completion counts and
depths on the selected image; separate VM exact paths from native providers,
FOR from SEND, and completion-only coroutines from actual yielding. Count copied
physical homes versus non-NULL homes at creation. Collect diagnostics outside
quiet timing. Compare candidate instructions/cycles and unchanged workload
elapsed time directly with both selected golden51 and immutable upstream.

Meaningful correctness coverage includes genexpr iterator callbacks/errors;
cells/closures and code replacement during callbacks; origin tracking;
never-awaited warnings; async-generator hooks; reentry and parent frame/running
inspection; initially non-None send; discard/local/overlap recipients and mortal
input finalizers; return values through deep SEND/FOR chains; throw/close and
PEP479; frame escape and GC; recursion failure; pending redirects; hooks added
or removed during child cleanup; PRE/POST monitoring failures; and far completion
adapters. Constructor and emitter negatives must reject changed owner/recipient
or exceptional-resume contracts. Source proof and selected native inspection do
not replace executed release/debug tests and measured breadth.
