# Semantic iterator and delegation completion

Proposal only, 2026-11-09. No runtime edits, native jobs or builds accompany
this packet. Read against current integrated source while root qualifies the
first frame-binding increment. B reviewed the destination split and requires
explicit edge definitions/kills and fallible cold completion. Implementation
waits for B's complete phase review, root admission and the first measurement.

## Scope

Make FOR_ITER and SEND complete their own operations. Remove executable
END_FOR, POP_ITER and END_SEND, including their instrumented forms. Remove the
initial generator-resume POP and unused ordinary yield-resume POP through an
explicit input recipient. Compact the static init shim physically. Keep one
frontend/compiler and one owning-frame ABI. No replacement cleanup opcode,
ordinary dispatch metadata interpreter, ghost instruction event or historical
finalizer PC requirement is introduced.

Shared exception-stack and branch cleanup POPs are a subsequent inventory.
This increment does not claim the whole no-release-word objective is complete.
The existing compiled-call ledger is a separate performance baseline; this
proposal does not infer speedup from sample percentages or fewer words.

## Words and frame phases

| Operation | A | B | C | D |
| --- | --- | --- | --- | --- |
| FOR_ITER and variants | yielded item recipient, local/temp/IR_NO_DST | iterator window base | feedback | real exhausted successor |
| SEND and variants | completed result recipient, local/temp/IR_NO_DST | receiver window base | feedback | real completed successor |
| YIELD_VALUE and instrumented form | incoming resume-value recipient, local/temp/IR_NO_DST | outgoing yielded-value home | existing suspension flag | unused |
| RETURN_GENERATOR | initial incoming recipient, always IR_NO_DST | existing construction window if needed | unused | unused |

D remains a forward branch encoding, not an absolute pointer. Its decoded
successor is `site + 1 + signed_distance`; it never means a cleanup word or
"skip one more". Existing relaxation can use real IR_JUMP instructions to
represent distant semantic branches. Once both exhaustion routes agree,
relaxed FOR_ITER needs a body jump and an exit jump, not distinct generator-
cleanup and iterator-cleanup jumps. SEND uses the same body/exit arrangement.
These are actual control transfers, not aliases that perform release work.

The frame fields have explicit meanings in their existing phases:

- A parent awaiting an ordinary CALL stores its result destination in
  return_slot and its real continuation in return_pc.
- A parent awaiting a FOR_ITER generator stores A in return_slot for a yielded
  item. A generator's final return does not publish there. It must preserve
  the last named loop-variable value.
- A parent awaiting a SEND child stores B+2 in return_slot for a yielded item
  and the real fallthrough continuation in return_pc. Completion instead
  publishes to SEND's A and takes D.
- A suspended generator stores the incoming resume-value recipient from its
  YIELD_VALUE A in return_slot. FRAME_CREATED uses IR_NO_DST. The yield's B
  still identifies the owner transferred out; it is not an implicit input
  destination.

No new per-frame step tag or tagged result slot is necessary. The existing
child owner/state distinguishes a generator return from an ordinary return;
the cold generator-completion path decodes the parent's actual pending
FOR_ITER or SEND word. Executing words have code-pinned lifetime. Parent PC
and pending continuation cannot be changed independently of their admitted
owner recipe while a child is active.

## Positive edge ownership contract

Here a window slot contains an owner; a cursor is a tagged scalar or NULL and
has no object count. A is not a claim that every outcome writes a value.

| Semantic outcome | Preserved owners | Definitions | Killed homes |
| --- | --- | --- | --- |
| FOR_ITER item | B iterator; updated B+1 cursor | A item, unless discarded | input-only C-held step owners |
| FOR_ITER exhausted | unrelated locals, including previous loop A | none | B and B+1; returned C owner is disposed |
| FOR_ITER error | pre-step iterator window until unwind | none | only detached C owners; unwinder handles published inputs |
| SEND yielded | B receiver and B+1 cursor | B+2 yielded item | previous sent argument at B+2 |
| SEND completed | unrelated locals | A completed value unless discarded | B/B+1/B+2 window owners after detachment |
| SEND error | receiver window until unwind | none | detached sent argument/result owners exactly once |
| YIELD suspension | other generator locals and delegate window | parent's yield recipient | outgoing B owner |
| YIELD resumption | other suspended owners | A incoming value unless discarded | no extra synthetic POP owner |

Compiler analysis, constructor validation, debugger admission and source-error
recipes must use these outcomes. A union `fmt.writes` cannot claim FOR_ITER A
is defined on exhaustion, or SEND A on yield. The compiler records through,
definition and kill sets for the actual edge, and verifies them after folding,
window allocation and relaxation. Completion destinations cannot alias an
unrelated live owner. SEND completion may reuse a detached receiver home;
FOR_ITER A may be a named local and remains unchanged on exhaustion.

The format needs an explicit completion/resume destination phase, rather than
treating SEND A or YIELD A as an ordinary same-instruction output. B owns the
exact compact representation of these fixed semantic edge facts. They are
compiler/verification/observer metadata; the ordinary handlers implement their
known outcomes directly and do not interpret a runtime kill list.

## One completion primitive, direct normal paths

A shared inline primitive, provisionally
`ir_complete_step(tstate, frame, site, base, destination, exit, value, kind)`,
consumes a **canonical owned** completion value and implements FOR_ITER or SEND
completion. Its kind is a compile-time literal at native handler sites. A small
fallible result contains status and the selected executable continuation:

```c
typedef enum {
    IR_STEP_ERROR_PRE = -2,
    IR_STEP_ERROR_POST = -1,
    IR_STEP_EXIT = 0,
    IR_STEP_REDIRECT = 1,
} _PyIRStepStatus;

typedef struct {
    _PyInstr *next;              /* valid only for EXIT/REDIRECT */
    _PyIRStepStatus status;
} _PyIRStepResult;
```

On either error the helper has installed the matching source/EH phase before
returning. The caller tests failure, rather than interpreting a cleanup recipe.
EXIT/REDIRECT both tail-dispatch the returned real target. The distinction
keeps observed continuation changes explicit in review and tests.
There is no transaction record or addressable handler-local ownership array.
The slow observation/retirement helper is called only when callbacks or real
last-owner releases require it. Fast list/tuple/range item paths retain their
existing inline tests and item production; they do not call a generic
completion helper on every successful iteration.

DSL completion markers for iteration and delegation emit this common
primitive and checked ownership transition. Every family member uses the same
contract rather than duplicating retirement code. Native provider results
normalize at their publication boundary; exact tuple/list home copies and
unchanged slot-origin generator returns keep their proven canonical bits.
No blanket canonical-result assertion is inferred from a handler name.

Refactor `_PyForIter_VirtualIteratorNext`'s provider/result boundary carefully:
it currently drops a returned value and generates exception monitoring inside
the retrieval helper. The new step result must distinguish NEXT, DONE and
ERROR and hold DONE's value until common completion. Preserve the intentional
RAISE/STOP_ITERATION behavior for ordinary iterators versus Python generators;
do not duplicate an event in both provider and completion. Native SEND,
SEND_VIRTUAL and SEND_ASYNC_GEN similarly pass their actual outcome, not a
jump followed by cleanup elsewhere.

### Return and yield transfer

Make RETURN_VALUE a terminal owner transfer: take the source owner, retire the
child, complete the parent's pending operation, and tail-dispatch. Its DSL
must not describe a fictitious result pushed onto another frame's evaluation
stack. The ordinary CALL case keeps its inline `_PyFrame_SetReturnValue`
publication/discard and explicit continuation, with the already reviewed cold
debugger redirect handling. It does not enter an all-return generic helper.

For a generator/coroutine return to a VM parent, call a cold, fallible
`complete_generator_return(tstate, parent, canonical_owned_value)` after child
retirement. It decodes actual FOR_ITER/SEND fields and invokes the same
completion primitive. It returns status plus selected continuation, rather
than the current pointer-only `_PyEval_GeneratorReturnTarget`. It consumes the
value on success and failure. For an interpreter-entry parent, publish to its
ordinary explicit exit recipient; the external send caller receives the
existing NEXT/RETURN distinction.

YIELD_VALUE continues to publish its owned outgoing value to the parent's
saved yield recipient. It saves its own independent resume recipient from A
before suspension. It does not perform potentially fallible completion or
observation after committing suspension: the existing rule that YIELD_VALUE
cannot raise remains. Instrumented PY_YIELD observation happens before the
irreversible suspension using the existing checked ownership transfer.

Canonical RETURN provenance stays fail closed. Update the full checked token/
ownership contract for the new terminal marker, and retain mutation cases for
extra input writes, transformed results, changed held owners and pointer escape.
The marker consumes a proven canonical source exactly once across child
cleanup; raw/native returns retain publication unless independently proven.

## Observation, errors and reentrancy

Completion has three explicit phases:

1. **Produced, before commit.** The result has one C owner. Receiver/iterator
   homes still publish their input owners. STOP_ITERATION observation, when
   applicable, runs at the real FOR_ITER/SEND site with that valid pre-commit
   state. A callback exception consumes the C result and unwinds using the
   corresponding semantic input/EH recipe; it does not use a deleted word's
   exception depth.
2. **Commit.** Detach the window owners before any closes. For SEND, publish
   the completed result (or hold it for discard) only after clearing any
   overlapping receiver home. FOR_ITER never publishes its DONE value to A.
   Publish a coherent post-state before branch/source callbacks. Each object
   has either a window/home owner or a detached C owner, never two counts
   represented as one owner and never an unanchored borrow.
3. **Retire and continue.** Close detached owners, preserve any callback-
   selected continuation, then execute the actual successor. A callback error
   after commit uses the post-edge owner/EH recipe. A finalizer enabling
   observation starts the next supported event; it cannot recreate END/POP
   events or resurrect a disposed value.

The observer path may use explicit phase/outcome metadata, as B's source
engine already does. The unobserved path reads no source records. Callback
redirect admission must validate the actual pre/post owner set, not projected
frontend stack depths. Active child overlap is never cleared by an ancestor
setter. The first implementation must either support a reviewed pending
iteration redirect recipe or reject that active-child state explicitly; it
must not redirect PC while leaving a stale return recipient/completion site.
Ordinary loop-body redirects that preserve iterator/index owners are a separate
B recipe and must agree with these edge contracts.

Monitoring must move to semantic locations. FOR_ITER emits BRANCH_LEFT for an
item and BRANCH_RIGHT for exhaustion; its Python-generator completion can also
emit STOP_ITERATION. SEND emits STOP_ITERATION on applicable completion, and
its async-for completion supplies BRANCH_LEFT. END_ASYNC_FOR's successful
StopAsyncIteration case continues to supply BRANCH_RIGHT, referring to the
associated SEND site rather than END_SEND. INSTRUCTION events concern only
real operations.

Reuse the existing per-event tool-mask machinery. The current per-site tools/
left_tools split cannot conflate FOR_ITER's third event: if a third sparse
observer-only mask is needed, integrate it through the common tool accessor,
add/remove/restart/disable/sanity-check paths. Do not add independent duplicate
tool state or ordinary per-handler event-mask tables. Tests must prove a tool
can DISABLE STOP_ITERATION, LEFT or RIGHT independently at the same site.

Dynamic observation enabled inside the child body or its frame cleanup is
checked by cold completion against current instrumentation after retirement.
Observation enabled during completion retirement affects subsequent supported
events. Source-edge LINE handoff must neither omit nor duplicate a landing
LINE, including relaxed jumps and callbacks that themselves enable tools.

## Resume inputs without POP artifacts

Initial generator/coroutine construction records IR_NO_DST for its first
incoming value. The runtime must still reject non-None first send before
publishing or discarding the input. Audit all providers: external generator
send already validates FRAME_CREATED, while `gen_try_set_executing` alone does
not prove this input constraint for inlined SEND. A non-None created-state
fast entry must decline to the validating provider or perform the explicit
check before consuming input.

For an ordinary nondelegating yield whose incoming value is unused, the
compiler sets YIELD_VALUE A=IR_NO_DST and removes its successor POP. The send
provider consumes/disposes that input at the resume boundary. For a used
incoming value, A is its allocated home, including an admitted folded local.
No runtime path examines the next word to infer discard.

The two input ownership routes are different. `gen_send_value_ex` duplicates
its borrowed argument: the native caller still anchors that argument while
the duplicate is discarded. VM SEND transfers an owner from window[B+2]. It
must first detach that source home; calling `_PyFrame_SetReturnValue` with a
taken C variable while the window still advertises the same count is invalid.

For a possibly last mortal VM input whose destination is IR_NO_DST, use a
shared prepared-resume commit before closing it. Complete the parent yield
recipient/continuation, child's previous link and exception-state link, switch
the current frame and account for recursion entry. Only then close the single
C-held input, with the child at a valid real resume PC and no duplicate window
owner. Continue through the already-entered child path, never entering
recursion twice. Entry failure closes the detached input exactly once and
unwinds the prepared child with the original error preserved. This is a
generator activation primitive shared by generic and specialized entry, not a
second call runner or a fake result home.

The no-callback cases (None, tagged/immortal or proven nonlast owners) can
retire after source detachment on the existing fast activation path. Only the
callback-capable discarded VM input needs the cold prepared-resume phase; no
new pending-input field or per-dispatch input interpreter is proposed. The
ordinary native duplicate case must not inherit unnecessary VM transfer work.
If an already established child-entry primitive can provide this commit,
reuse it; the implementation must show the actual recursion, frame and
exception-state ordering before admission.

This chooses input retirement as part of semantic resumption. A last-owner
finalizer may run before the child's subsequent PY_RESUME observation rather
than at a removed POP. That ordering is an explicit implementation policy,
not a Python requirement for prompt destruction. Supported observations must
still see valid committed frames and ownership. Tests must exercise
`send(Resource())` through both VM transfer and native duplicate routes,
reentrant send/close, frame exposure, tracing enabled inside the finalizer and
exceptional entry. No count is retained in an unbounded queue or ghost slot.

Delegating suspension (oparg=1, FRAME_SUSPENDED_YIELD_FROM) retains its real
window resume home B+2: the incoming value feeds SEND and anchors the receiver
relationship. `_PyGen_YieldFromRef` currently derives the receiver at
return_slot-2; the verifier must prove that relation only in this state. The
remote asyncio reader must explicitly reject IR_NO_DST and out-of-range homes
before subtracting two. Otherwise a sentinel passes its current `>= 2` guard.
Frame copy/escape, gi_yieldfrom/cr_await, throw/close and remote inspection must
honor this phase distinction; no new tagged encoding is introduced.

## Static init program

Both `_PyIR_InitCleanup` and `_Py_InitCleanup` become exactly two real words:
EXIT_INIT_CHECK(slot1), RETURN_VALUE(slot0). Change the static code/exec array
types from three words to two, Py_SIZE/nwords/firsttraceable and no-location
payload accordingly. `_CREATE_INIT_FRAME` saves shim.return_pc=word0 and
checks real word0/word1. The existing native pointer continuation removes the
reason for a dummy predecessor. Do not leave an unused zero/NOP word behind
in either canonical or executable storage. Keep static no-monitoring and
free-threaded no-specialization restrictions.

## Exact file and consumer boundaries

A owns implementation in:

- `Python/bytecodes.c`: generic, list, tuple, range, virtual, generator and
  instrumented FOR_ITER; generic/gen/virtual/async-gen SEND; RETURN/YIELD/
  RETURN_GENERATOR, init entry and END removal.
- `Tools/cases_generator/ir_generator.py` and focused generator tests:
  terminal return owner transfer, completion/resume operand phase, checked
  markers, generated failure cleanup and provenance mutation gates.
- `Python/ceval.c`, `Python/ceval_macros.h`, coordinated `Python/ceval.h`:
  raw step-provider outcome, inline common completion, cold generator return,
  canonical transfer and observer/error entry points.
- `Include/internal/pycore_interpframe*.h`, `Python/frame.c`, `Python/pystate.c`:
  phase assertions, initialization/copy and any reviewed recipient helpers.
  Prefer no extra hot frame fields.
- `Python/specialize.c`, `Python/ir.c`, `Include/internal/pycore_code.h`,
  `Include/internal/pycore_ir.h`: remove END adjacency assumptions, compact
  static init storage, decode/display semantic operand phases.

B owns CFG/dataflow/assembly, source/observer/instrumentation migration,
constructor validation, disassembly consumers and debugger admission. Shared
interfaces above require a source freeze and coordinator regeneration. B's
compiler must remove cleanup ops only after transferring their real output
and owner facts to the producer; it cannot merely retarget past them.

Additional mandatory consumers: `Objects/genobject.c` incoming send,
`_PyGen_YieldFromRef`, throw/close/await and frame state; remote asyncio raw
return_slot lookup and debug offsets; frameobject f_lineno/f_locals/escape;
frame-copy/GC traversal; exception unwinder/source callback recipes;
`Python/instrumentation.c` EVENT_FOR_OPCODE, tool masks, twins, branch mapping,
instruction/line exclusions and `jump_dest + 1`; public `_opcode` contract,
`dis`, code/marshal validation; jump relaxation and `_PyCode` word-depth/
result queries. A/B assign these explicit out-of-scope files before admission.
Generated metadata, opcode numbers/cache magic and static fixtures follow the
new stream under coordinator integration, never optimistic old snapshots.

## Check ladder

1. Host generator/number checks, positive edge contracts and negative mutated
   return/step contracts. Verify every specialization uses shared completion,
   no sentinel address/index, and no hidden ordinary metadata walk.
2. Focused native ordinary list/tuple/range/custom/virtual iterator empty,
   one-item, mutation and failure paths; folded loop local remains the final
   yielded value after exhaustion; FOR_ITER_GEN/SEND_GEN selection proven.
3. Generator/coroutine NEXT versus RETURN and native send outcomes; used and
   unused send values, rejected non-None initial send, initial construction,
   nested delegation, async-generator asend, async-for termination, throw,
   close, PEP 479, PEP 523 fallback and free-threaded guards.
4. STOP/branch/LINE/INSTRUCTION/profile events; independent DISABLE;
   observation enabled in child body, child cleanup and result/iterator
   retirement; each callback raises in pre/post phases; valid debugger loop-
   body and folded-assignment redirects, rejected missing-owner states.
5. Lifetime/GC/frame escape/remote await chain tests with mortal iterators,
   returned values and sent values. Assert safety and bounded owners, without
   turning one CPython destruction coordinate into a language requirement.
6. Near/far relaxed loops and sends, constructor/marshal/positions/dis/static
   programs. Census no END_FOR/POP_ITER/END_SEND and no initial/unused-yield
   POP in this scope. Keep unrelated shared exception/branch POP census open.
7. Root's release/debug/stats merged suites, compiled-path review and matched
   unchanged workload screens. This source increment is admitted only after
   the current correctness gate and first frame-binding measurement.
