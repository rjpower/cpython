# Register interpreter: current contract

This page describes the register interpreter and call paths integrated through
runtime 48, `5559f60093e`.
The [checkpoint report](../issues/deltablue-call-model-20261110/checkpoint-report.md)
records diagnostics and remaining observer/tooling capability limits. The guarded-attribute and keyword increments below are
implemented in source `b8e7e1bee66`. Canonical direct class reads and owned
classmethod pairs extend them at `5559f60093e`; see the [integration report](../issues/deltablue-call-model-20261110/integration48.md).
[Status](../STATUS.md) records qualification and performance separately. The
conditional leaf-call work is an experimental, rejected candidate and is not
part of the selected runtime design.
See [status](../STATUS.md), the
[round plan](../issues/deltablue-call-model-20261110/plan.md), and the immutable
[baseline report](../bench/results/interpreter-parity-20261108.md).

The [register encoding](register-ir.md), [frame ABI](call-and-frame-lifetimes.md)
and [observer contract](handler-and-observer-contract.md) describe the same
runtime. Its owning homes and release protocol implement RC; Python does not
require immediate finalization. [Lifetime policy](lifetimes-and-language-semantics.md)
separates language behavior from that implementation choice.
The [debugger transfer model](debugger-frame-transfers.md) defines explicit
entry, cold context/owner proofs and bounded saved-local restoration.

## Entry and dispatch

A code object holds pristine eight-byte IR words and feedback initialization.
`Python/ir.c:_PyCode_GetExec` lazily creates its shared mutable executing copy
and feedback vector. Running frames own the code and retain pointers into that
allocation. Specialization and instrumentation patch handler fields.

`Python/bytecodes.c` and the cases generators produce one `_TAIL_CALL_*` handler
domain, dispatch metadata and observer contracts. `Python/ceval.c` enters the
first handler; handlers transfer with GCC musttail dispatch. Execution consumes
register homes directly, without a runtime stack-bytecode translation stage.
IR registers are physical memory homes, not machine registers or optimizing SSA.

```mermaid
flowchart TD
    entry[Evaluator activation] --> word[Real executing IR word]
    word --> handler[Generic or specialized semantic operation]
    word --> wrapper[Instrumented wrapper]
    wrapper --> pre[PRE callback where required]
    pre --> handler
    handler --> ordinary[Publish result or branch]
    ordinary -->|declared terminal| post[Committed callback]
    ordinary -->|otherwise| word
    post --> word
    handler --> child_entry[Commit a child and its result recipient]
    child_entry --> word
    handler --> completion[Complete loop, delegation or return]
    completion --> word
    handler --> error[Select actual exception recipe and unwind]
    error --> word
```

Source-only boundaries have no dispatched word. Shared cleanup instructions
that remain in the current compiler are an unfinished separate workstream;
there is no claim that every release-only word has been removed.

## Frame storage and ownership

Locals, cells, free variables, hidden scalar constant homes and temporaries
share one canonical PyValue array. A home contains NULL, an inline value, an
immortal, or an owning object edge. Mortal borrowed views are C-local and require
an owner; ordinary frame homes are not borrowed references.

Pure local loads duplicate the canonical home's owner and reuse its value bits.
The generator proves the exact local-read/duplicate source before selecting the
shared canonical final store. Local, temporary and discarded destinations retain
the same ownership and displaced-owner handling; raw providers still publish
their results through the ordinary boundary.

Plain yield transfers an unchanged canonical input through the existing
canonical return sink. Specialized list indexing uses the canonical final store
only after the complete guard/bounds/duplicate/retirement chain is proved. Its
early temporary publication before retiring the list remains ordinary. Proofs
bind the original token structure to the AST actually emitted; altered bodies
fall back to ordinary publication. Instrumented yield keeps its full boundary.

Temporary future-read liveness is not owner occupancy. Cleanup must retain or
retire an initialized owner even if no later Python operation reads it. GC,
frame copying, unwind and debugger inspection share that invariant. Normal
thread retirement clears local homes after proving temporary homes NULL;
exceptional and escaped/generator paths retain their appropriate full handling.

Thread chunks have opposing header/value stacks and one initialized frontier.
Reservation establishes valid value cells before constructing a header, filling
only the uncertain suffix. A header truncates that frontier before overwriting
value-capable memory; pop never restores a saved frontier. All retirement and
reentry paths must leave reusable value cells NULL. Separate heap frames are
initialized once, while full ownership copies initialize their destination by
moving every home. See the [frame ABI](call-and-frame-lifetimes.md).

## Guarded attribute values

The [approved lookup increment](../issues/object-call-20261109/lookup-proposal.md)
extends the existing non-descriptor instance-load handlers to canonical inline
class values. Feedback stores `_PyValueGuarded` metadata, not a persistent
`PyValueBorrow` or an owning home. A hit checks the receiver type version and
its admitted layout, creates a transient borrow, then acquires an owned canonical
result. It publishes that result through the existing result-retirement
transaction before receiver retirement can run Python. Shape, instance-slot and
module reads use that same publication contract with their own storage guards.

The class-value path requires absence from shared instance keys and valid inline
values, or a type with no instance dictionary. Shared-key insertion invalidates
the owning type before publishing a new name; dictionary replacement invalidates
inline values. Type/MRO changes invalidate the class-value proof. Descriptor and
custom-hook classification remains unchanged. Polymorphic snapshots copy inert
metadata according to the recorded handler; they never turn stale payloads into
borrows or owners.

Direct class-object reads use the same guarded canonical payload for admitted
objects and inline scalars. The existing class/MRO version and, when required,
metaclass version protect it. Descriptor precedence and custom-provider
classification remain in specialization. The ordinary reader acquires a result
before receiver retirement through the same checked publication contract.

Guarded words occupy independent fields beside native-only pointer/shape
unions. Attribute feedback is 48 bytes; snapshot payloads are 16 bytes and the
four-row polymorphic record is 120. This satisfies the existing word/union API
rule without a checker exemption. Allocation, reset and accounting use actual
type sizes. Larger records reduce optional private feedback capacity; overflow
must retain generic execution. This footprint is a measured performance
tradeoff, not a change in value encoding or cache ownership.

## Classmethod capture

An eligible method-form class lookup supplies the ordinary owned
`(function, accessed_class)` pair. Its cache stores the exact builtin classmethod
descriptor, not its mutable wrapped function. A hit checks the builtin metaclass
and class version, reads the current exact Python function, acquires it, and
transfers the actual accessed class. No receiver retirement occurs in this split.
Inherited lookups bind the subclass. Attribute-form retrieval still produces a
bound method; unsupported receivers/descriptors retain generic lookup.

```mermaid
flowchart LR
    proof[Validate class and descriptor] --> capture[Acquire live function and accessed class]
    capture --> arguments[Evaluate explicit arguments]
    arguments --> admission[Ordinary current-function binding and entry guards]
    admission --> activation[Overlapping owning activation]
    activation --> completion[Ordinary result and continuation]
```

`classmethod.__init__` can replace its callable without invalidating the class
version. Capture therefore happens at lookup, before later arguments can mutate
the descriptor. CALL uses that captured pair and checks the function's current
code/default/provider facts; it never resolves the descriptor again. See the
[proposal and tests](../issues/deltablue-call-model-20261110/A-implementation-proposal.md).

### Experimental canonical method capture

Candidate A50 `e014965013b` extends guarded canonical capture to the three
ordinary method layouts. After the existing lookup guards, it acquires the
descriptor owner and transfers a temporary receiver or duplicates a local one.
The generator proves each output is canonical and publishes it without numeric
recanonicalization. The guarded cache word remains borrowed; canonicality does
not prove that the descriptor is a Python function. This candidate is not yet
selected; see the [round report](../issues/deltablue-call-model-20261110/report50.md).

Implementation of the reviewed adjacent method/CALL refinement has started;
it is not yet complete or measured. It retains both instruction positions for
errors and observation, and must prove control flow, exception handling and
observer eligibility directly. A post-capture miss publishes the captured owners
and resumes the ordinary CALL without repeating lookup.

## Python calls and ordinary return

Exact calls guard their standard Python provider and transfer argument owners
into a child. A versioned known-call plan lets guarded CALL_PY_GENERAL bind
common positional calls directly through `call_bind_known_python`; no generic
transaction record is built on that hit. It fills defaults and empty variadic
bindings under the same owning-frame ABI. Fixed 1–3-keyword plans use that
same overlap primitive and move only their bounded suffix through C-local
owners. A pre-effect miss uses the existing transaction/provider path.
Extended, instrumented and unsupported shapes retain complete provider handling.

Compiler-proven empty kwargs may remain a pending local until a materialization
barrier. Folded reads, cell/alias hazards and frame publication are covered by
the compiler/constructor proof and cold observation helper; unrelated local
loads have no pending-value class test. Excluded code eagerly creates its dict.

The caller saves `return_pc` as a real executable continuation and `return_slot`
as a plain local/temporary destination or IR_NO_DST. Return keeps its canonical
owned value in C across child unlinking and retirement. Ordinary CALL recipient
completion stays inline: publish into the destination, retiring its actual
replaced owner, or close the discarded C owner. The saved recipient and
continuation fully describe normal return.

```mermaid
flowchart TD
    guard[Known provider and binding guards] --> decision{Plan admitted?}
    decision -->|yes| commit[Reserve initialized storage and transfer owners]
    decision -->|miss before effects| generic[Generic transaction and binder]
    generic --> commit
    commit --> child[Complete child is current]
    child --> result[Return owns result in C]
    result --> retire[Restore caller and retire child]
    retire --> recipient{Caller recipient}
    recipient -->|destination| store[Install canonical owner]
    recipient -->|IR_NO_DST| discard[Close C owner]
    store --> resume[Resume saved real return_pc]
    discard --> resume
```

Generic transactions still protect caller owners across preparation, callbacks,
monitoring and fallible binding. They pin the actual executing code and refresh
after effects; a completed callback is not replayed on a guard miss. Once
ownership commit begins, failure retires the published child instead of falling
back to a second call. Code remains a separate owner because function.__code__
can change while a child executes.

## Experimental conditional leaf-call design (not selected)

The rejected candidate stored an optional bounded leaf certificate on `_PyCodeExec`,
built from immutable final IR. It describes a complete body as up to four value
paths rooted in arguments, constants, small integers or guarded globals, with up
to two guarded attribute steps per path, an optional inline scalar comparison,
and a selected return. Certification is limited to 32 words and rejects
unaccounted words, dead raising reads, unsupported control flow, effects,
handlers, cells/free variables and unsupported argument shapes. The record
contains indices and scalar metadata, no Python-object owner.

Admission also requires all arguments supplied, split-self arity to match, and
the thread-owned caller and current captured function/code to pass their exact
version and provider checks. Only plain `CALL_PY_EXACT_ARGS` has the terminal
marker. Positive generator validation binds it to the exact PEP 523, version,
arity, stack-space and
recursion checks, before initialization and any effect. Bound-method and generic
calls do not inherit it. A cheap absent-certificate check avoids the out-of-line
helper. The helper rechecks function, code, providers, caller and observation
state, then probes through the same guarded global, shape and class readers used
by ordinary handlers. It computes only the selected branch and reads only that
result path.

Every decline is a MISS with original call homes unchanged. After acquiring the
canonical result, an identity-based release multiset covers the consumed window
and either the old LOCAL or discarded result. An overlapping temporary's old
value is counted only in the window; a nonoverlapping temporary destination
must already be NULL. Each count-owning object must
have `Py_REFCNT > releases` after acquisition. This GIL-held proof excludes
finalization across the complete callback-free body and retirement interval;
the helper adds no temporary pins to force admission. It detaches and NULLs the
window, performs only preflighted nonfinal closes, then publishes with the
ordinary `_PyFrame_SetReturnValue` path. DONE returns the real next word and
bypasses child setup; no fallback or error edge follows commit.

Admission declines with trace/profile/PEP 523, monitoring, pending evaluation
work, nonstandard providers, redirects or unsupported runtime configurations.
Free-threaded, debug, DTrace and Emscripten builds currently decline; debug
qualification therefore exercises fallback, not leaf hits. Remote optimized
stacks may omit the elided callee. Runtime `24d97fa8c3b` integrated this
experiment and was rejected as a performance candidate; gate-off ablation
`af60e5eb43e` was also rejected. Neither is the selected or default
implementation. See the [rejection report](../issues/deltablue-call-model-20261110/integration49.md)
and [next proposal](../issues/deltablue-call-model-20261110/C2-proposal.md),
which has peer approval as a future direction; implementation is unproved. These mechanics are retained as experimental design
context, not a runtime qualification or performance claim.

```mermaid
flowchart TD
    call[Checked plain exact call] --> cert{Certificate present?}
    cert -->|no| frame[Ordinary frame initialization]
    cert -->|yes| probe[Recheck admission and probe full body]
    probe -->|MISS, homes untouched| frame
    probe -->|result acquired| owners{Aggregate releases nonfinal?}
    owners -->|no, close duplicate| frame
    owners -->|yes| commit[Detach window and close preflighted owners]
    commit --> publish[Ordinary return setter; dispatch real next]
```

## Iteration, delegation and resume

FOR_ITER's body edge preserves its iterator/cursor and defines only the item
recipient. Exhaustion kills the pair and does not overwrite the last loop local.
SEND's yielded result goes to B+2; completed delegation kills its window and
publishes A, or discards at IR_NO_DST. CLEANUP_THROW completes into that same
recipient and its named real successor. Native and Python-generator routes use
the same owning completion primitive; ordinary CALL return does not pay a
universal generator transaction.

A VM child's completed return reads the actual site's immutable SEND/FOR word.
The constructor proves its destination, window and signed exit geometry;
specialization and instrumentation preserve those operand fields. The saved
physical continuation plus the signed distance selects completion, including
an assembler-provided far-jump adapter. No general metadata decoder is needed
on this positively declared return path.

One C completion body serves all providers. Proved VM returns instantiate it
with a literal SEND/FOR kind and canonical input, letting the compiler remove
redundant publication and inapplicable owner checks. Native and THROW providers
use the same body with general publication. The existing observer adapter,
window detachment, destination installation and post-retirement PC reload are
shared; the specialization introduces no frame state or per-resume work.

Completion separates fallible PRE observation while original owners exist from
POST observation after the semantic outcome is committed. It detaches windows,
publishes destinations and continuations before closing owners, and preserves
supported callback redirects. STOP_ITERATION, branch directions and source LINE
are independent observer capabilities. Real error StopIteration reporting and
implicit completion reporting must not emit duplicate exception events.

Declared terminal providers commit output homes and the saved continuation
before observing. The actual PC remains their semantic owner site during the
callback, then the selected PC is published before deferred retirement. Errors
preserve the callback exception; close-time redirects retain the latest PC.
The backward JUMP poll remains before this barrier. POP_ITER holds its detached
iterator through observation and retires it afterward. Its reported branch
source and BRANCH_RIGHT event are distinct from its own fallthrough edge.

YIELD_VALUE separates its outgoing owner from the next resume input recipient.
For an unused input, IR_NO_DST closes the incoming owner only after the parent
transfer cell is detached and the resumed child's current-frame, exception and
recursion links are committed. Delegated suspension retains the actual receiver
lookup relationship. Initial generator construction uses the same explicit
recipient model. Hidden interpreter/initialization entry starts at its real word-zero
continuation.

## Errors and observation

`f_lasti`, disassembly and exception offsets remain compact executable indices.
Eliminated source LINE spans are edge metadata in the code's normalized source
container. Their callbacks publish an observer-only source cursor and line;
they do not invent an INSTRUCTION event or a virtual public offset.

The constructor's completion proof follows normal/error/source-callback edges,
tracks unavailable original homes and definite NULL, and rejects required reads
of retired owners. Generated terminal observer facts distinguish the committed
physical output from a retired old owner at the same home. A source-only
callback error uses the declared POST state; an ordinary exception recipe
covered by that word must work for both compute PRE and observer POST. Actual
exception/lasti definitions and fresh physical output locations matter;
frontend writes and pass-through shapes alone are insufficient.
Implicit RERAISE anchors may be semantically unread, while saved locals that a
handler restores remain required. This preserves the selected PEP479 and
inlined-comprehension error semantics without a second unwind engine.

Each C evaluator activation owns its pending source-error frame/record. Nested
Python entry during retirement has a separate activation, so its error cannot
replace the outer source handler. The frame-object cursor serves display; it
is not the sole error storage and need not exist on allocation failure.

Observer scopes pair actual frame, site, physical continuation, event, owner
edge and redirect generation. Completed source providers grant SOURCE_POST;
legacy inline hooks with unfinished work deliver the same events under an
unsupported scope. Source records name normalized real landings, while the
scope retains an immediate relaxation continuation when present.

The setter shares one owner/restoration Plan across real PRE and certified
committed views. Neutral JUMP borrows its exact site state; a legacy LINE
callback emitted on a backward JUMP carries that JUMP's committed scope.
Non-neutral outcomes use a request-owned copy of the exact site rows with the
selected normal transfer applied. Retiring an old input cuts its provenance
even when a replacement value occupies that home. This path excludes transfers
that create context or saved-binding tokens.

Non-neutral cancellation requires an exact scope, physical successor and
generation, inherited handled context, empty stopped and target saved-binding
state and no incoming target scratch owners. It cannot reuse PRE survivor proofs
or source-only fallback. Warning callbacks require a fresh view and Plan; a newer redirect
discards old edge authority. Existing PRE restoration and strict neutral
protocol recipes retain their checks. General SOURCE_POST, yield and
suspended-frame transfers require separate capabilities.

A cold frame-object LINE handoff records expected real landing, previous source
line and monitoring epoch. It is separate from public f_lineno. Landing detaches
it before callbacks and accepts only matching continuation/configuration;
reconfiguration produces a fresh landing event without replaying old edges.
Exact relaxation adapters normalize LINE landings while their helper words keep
real INSTRUCTION events. Setter, exception and frame-clear paths invalidate the
handoff before callbacks. Broader event parity remains under qualification.

## Limits

The active interpreter uses the selected universal PyValue/RC representation
and rebuilt-source ABI. Native method/tracing JIT tiers, machine register
allocation, OSR and general native recovery maps are separate future work.
Source-level ownership review and a short parity screen do not replace full
functional qualification, broad regression repair or the matched parity gate.
