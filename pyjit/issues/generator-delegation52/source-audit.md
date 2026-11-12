# Independent source and feasibility audit

Draft for architectural review, 2026-11-12. Read-only audit of selected runtime
`0e5e61f4c6b` at
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/round51-golden`
(R). Canonical documentation is in the python-jit checkout (C); C's runtime
sources were not treated as selected source. Repository guidance and the
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/) were
read. No builds, native execution, tests or runtime changes were performed.

## Judgment

Explicit DELEGATE and SUSPEND operations are feasible without a second evaluator
or per-resume frame allocation. They need a real change to transfer semantics:
DELEGATE retains the delegate owner while the parent activation is suspended,
routes an incoming owner to that delegate, forwards a child yield into parent
suspension, and completes once into its result recipient/done continuation.
Changing SEND's name or choosing the existing backward jump's target alone does
not implement that contract.

A first implementation should retain existing generator heap frames and actual
parent links throughout child execution, reuse completion/publication helpers,
and introduce a shared suspension transfer used by plain yield and delegation.
The representation should keep a real suspended site distinct from DELEGATE's
send/completion site. This is needed for recovery and observation, not merely
for display. The design must settle the encoding and exceptional-resume contract
before source edits. Approval remains subject to architectural review and the
coordinator's performance and correctness gates.

## Selected-source facts

| Concern | Current implementation in R | Consequence |
| --- | --- | --- |
| Frontend delegation | `Python/codegen.c:553`, `codegen_add_yield_from`: SEND; protected YIELD_VALUE(1); RESUME; JUMP_NO_INTERRUPT; CLEANUP_THROW; END_SEND | SEND errors and injected suspension exceptions have different recovery contexts. |
| Register conversion | `Python/regcfg.c:496`, `convert_instr`: jump/window outputs canonicalize the stack union; exception prefixes canonicalize separately | New suspend/completion edges need semantic reads/definitions and liveness, including owners with no future reads. |
| Physical completion lowering | `Python/assemble.c:1144`, `lower_completions`: after allocation, validates SEND/END_SEND and CLEANUP_THROW windows; marks frontend cleanup wordless; `encode` at 1486 takes the final recipient from END_SEND | Keep frontend allocation effects until equivalent new edges exist. Do not remove the owner model along with the dispatched loop. |
| SEND encoding | `Include/internal/pycore_ir_metadata.h:847`: A=result recipient, B=three-home window, C=feedback scaled offset, D=signed 12-bit completion distance | There is no fifth field for another independent PC. Far exits already need three-word relaxation adapters. |
| Plain yield encoding | Same header:860: A=incoming recipient, B=outgoing one-home window, C=yield kind, D=unused | An arbitrary 24-bit resume PC does not fit while C also carries kind. Split handler kind or another explicit design is required. |
| Exact child entry | `Python/bytecodes.c:1574–1652`; `Tools/cases_generator/ir_generator.py:3915`, `enter_generator` | Retain exact type, PEP523, created/non-None and executing guards, input detachment, recursion, current frame, exception linkage and publication order. |
| Yield transfer | `Python/bytecodes.c:1708`, `_YIELD_VALUE` | Stores incoming recipient, unlinks exception/current frame, leaves recursion, sets suspended state and returns the owner to the caller. Shared helper must preserve these transactions. |
| Completion | `Python/ceval.c:2987`, `complete_step_impl`; 3095, `_PyEval_CompleteGeneratorReturn` | Completion already publishes canonical generator output directly. It decodes the caller's immutable SEND/FOR word and uses shared completion observers. Add DELEGATE explicitly. |
| Resume publication | `Include/internal/pycore_interpframe.h:188`, `_PyFrame_SetReturnValue` | Completes pending redirect first; handles unused input and displaced owner. A new direct transfer cannot blindly write its cached slot/PC. |
| Yield-from discovery | `Objects/genobject.c:265`, `_PyGen_YieldFromRef` | Borrowed delegate is `base[return_slot - 2]`. Keep the real three-home geometry initially, or replace all consumers with a declared generator suspension interface. |
| Close | `Objects/genobject.c:515`, `gen_close` | Uses suspended-yield-from state to close child; recognizes suspended RESUME and depth1 flags for its no-unwind fast finish. Port this check to explicit suspension state; do not leave accidental decoding of a different word. |
| Throw | `Objects/genobject.c:655`, `gen_throw_current_exception`; 678, `_gen_throw` | Child throw occurs while parent is executing and linked into current-frame chain; StopIteration is reinjected through parent suspension's CLEANUP_THROW path. |
| Admission verification | `Objects/codeobject.c:1190`, `validate_completion_words` | Explicit checks include delegated YIELD A==B, completion window extents and owner-safe exception handlers. Update these alongside the instruction producer. |
| Observer/recovery facts | `Include/internal/pycore_ir_outcomes.h:261`, `_PyIR_Outcome`; codeobject and frameobject analyses | SEND fallthrough currently defines B+2 as yielded owner; completion retires three homes. A direct suspension must not claim this ordinary fallthrough output. |

## Bounded implementation contract

1. Define DELEGATE as one persistent semantic operation with child-yield,
   child-completion and error outcomes. It owns the receiver/cursor window until
   completion. A child yield need not round-trip through B+2 and a dispatched
   parent yield; it can pass the canonical owner to the same suspension routine
   used by explicit SUSPEND. Parent input must still be an owning value before
   callbacks can run. No C-only borrowed delegate may outlive its frame owner.
2. Keep a real SUSPEND site carrying incoming recipient and explicit resume
   continuation, even if the hot child-yield route invokes its transfer directly.
   For delegation this site remains the exceptional-resume site under the
   compiler's synthetic CLEANUP_THROW handler. Preserve a distinct actual send
   site for normal send failures. An undifferentiated DELEGATE PC cannot select
   both recipes from today's exception table.
3. A practical encoding can retain DELEGATE's existing A/B/C/D layout and use a
   declared adjacent SUSPEND site for its suspension edge. A SUSPEND with A=input,
   B=outgoing and CD=absolute resume PC fits only when yield kind is represented
   by handler identity or another explicitly validated encoding. This is a
   feasible option, not an approved interface. If adjacency is used, constructor
   verification must prove it from immutable semantic words, independent of
   mutable instrumentation wrappers; no source-map execution lookup is needed.
4. Decide how resume polling and events reach that explicit continuation.
   `IR_SKIP_RESUME` (`Python/ceval_macros.h:793`) checks eval-breaker versus
   instrumentation version, TLBC and Emscripten signal state. Instrumented RESUME
   always runs. Removing the backward jump does not authorize removing this
   boundary. A resume target may name a real RESUME with a declared continuation,
   or a common resume routine may implement the exact boundary; document which.
5. Put child admission/entry, suspension and completion behind common transfer
   interfaces. Exact types specialize admission; custom iterators, virtual
   iterators, async asend and PEP523 retain their protocol providers but finish
   through the same semantic yield/completion outcomes. A miss occurs before
   effects; no fallback may replay an already-sent owner or callback.
6. Cascaded delegation must walk actual frame transitions, not skip parents
   wholesale. While a leaf callback runs, all entered parents remain executing,
   linked and visible through current-frame/back chains. On a child yield, each
   parent publishes its own suspension state, exception link, recursion exit,
   source/PY_YIELD observation and profile/DTrace behavior in order. Resume does
   the reverse without allocating new frames.

The scope need not include a new frame storage model, lazy stack materialization,
a general native-JIT recovery system or constant operand redesign. These are
unrelated to representing delegation correctly.

## Recovery and observation risks

- `assemble_exception_table` (`Python/assemble.c:162`) assigns a handler from
  each operation's physical word range. Today's delegated YIELD is protected;
  SEND is outside that synthetic region. Keep injected throw/close recovery
  distinct from provider failure. Port both the bytecode and register CFG
  ownership recipes; merely keeping a line number cannot recover owners.
- `_PyIR_Outcome` and `_PyIR_CompletionWindow` feed constructor owner proofs,
  debugger reconstruction and handler-read safety. DELEGATE needs audited
  suspended/resumed/completed outcomes. New input is defined on resume, not an
  ordinary same-activation output. Scratch owner occupancy survives suspension.
- `_PyIR_ObserveStep` (`Python/instrumentation.c:1640`) already centralizes
  STOP_ITERATION before completion, branch observation after owner publication,
  redirect checks and source transitions. Reuse it for DELEGATE completion.
  Preserve the existing finalizer order in `complete_step_impl`: detach inputs,
  publish result/continuation, observe, then retire owners with errors preserved.
- Instrumentation has explicit SEND exceptions throughout its event maps and
  wrapper management. SEND has no ordinary instrumented twin; semantic helper
  observes completion. Add DELEGATE deliberately instead of relying on family
  renaming. Relaxation grouping (`instrumentation.c:99`) recognizes exact
  adapter shapes and each real helper retains INSTRUCTION events.
- `Objects/frameobject.c:1440` rejects committed stopped-state proofs for
  frame-pushing/suspending operations; the setter also rejects suspended yield
  redirection without a dedicated owner recipe (`:3485`). Do not accidentally
  broaden this permission by clearing effect flags. Preserve redirect generation
  checks and pending-result cancellation when a child returns.
- The canonical yield publication proof
  (`Tools/cases_generator/ir_generator.py:4706`,
  `canonical_frame_transfer_input`) matches the actual uop body, not its name.
  A shared helper rewrite requires a corresponding narrow proof; a silent proof
  miss can restore generic publication and materially affect the hot path.
- `_YIELD_VALUE`'s debug assertions admit only parent SEND/FOR, and completion
  decodes immutable caller fields. Update every consumer together; accepting a
  new opcode only in release dispatch will miss debug-only failures.

## Required evidence before promotion

The coordinator should cover multi-level send/next return values, reentry and
leaf-callback inspection of every parent's running/frame state; exact generators
and coroutines; custom iterators with send/throw/close and absent methods;
GeneratorExit and StopIteration failures; exceptions caught inside/outside the
synthetic delegation region; recipient discard/overlap and destructor callbacks;
finally/with/handled-exception state; gi_yieldfrom/cr_await; pending child redirect;
monitoring installed/removed while suspended; source, INSTRUCTION, PY_RESUME,
PY_YIELD and completion callback errors. Add malformed encoding/CFG negative
coverage, long completion distances and changed recipient/window/control cases.

The selected profile's SEND 27.41%, YIELD 13.92% and backward jump 5.11% are
sample locations, not removable fractions. Existing guards, ownership and state
publication remain required. Candidate release/debug semantics and paired
unchanged-workload timing must decide promotion; a cleaner semantic IR alone
establishes no speedup.

## Concrete interface check with design worker

The design worker proposes three real words: DELEGATE retaining A=result,
B=window, C=feedback and D=done distance; SUSPEND retaining A=incoming recipient,
B=outgoing and C=plain/delegated kind; RESUME_DELEGATE with A=resume flags and
CD=absolute delegate continuation. The saved suspension PC names the real
RESUME_DELEGATE. This fits the existing word size: explicit continuation lives
in its own semantic resume operation rather than a fifth DELEGATE operand.

This is a feasible bounded encoding provided the constructor proves the triple,
window, incoming recipient and resume target. It preserves injected exception
selection because `Python/ceval.c:2298` starts throw handling with saved PC and
`error`/`exception_unwind` use next_instr-1, selecting SUSPEND immediately before
RESUME_DELEGATE. Normal DELEGATE provider errors keep the separate send recipe.

One concrete encoding trap remains to resolve: current long SEND relaxation
(`Python/assemble.c:1609`) inserts two real jumps between SEND and its body.
The entry helper saves the immediately following adapter as return_pc. A
DELEGATE implementation cannot assume return_pc or site+1 always names SUSPEND.
It must use a declared validated adapter shape, change DELEGATE's long-exit
layout coherently, or bound its direct path to proved nonrelaxed triples. All
forms must retain correct generic execution and actual INSTRUCTION events.

The direct canonical held-value route must check parent observation/redirect
state before bypassing result publication and SUSPEND dispatch. The ordinary
route can publish to B+2 and execute real SUSPEND, retaining a single semantic
contract. It must not advertise a yielded B+2 owner on an edge where the value
was transferred directly and the home stays NULL. New outcome declarations need
to distinguish that held-input suspension contract from ordinary SEND
fallthrough; simply mapping the opcode to SEND inherits the wrong facts.

A bounded alternative to adding a third general CFG edge is a checked internal
fusion: cold analysis retains the logical DELEGATE-yield-to-SUSPEND edge, while
the hot transfer proves the two operations have identical final owner state and
no observable intermediate stop. The monitored/recovery route materializes the
B+2 owner and executes SUSPEND. This may reuse the two-edge analysis, but needs
an explicit producer/transfer proof; the physical outcome claim is valid only
at actual stopped states. This clarification was sent to the design worker.
