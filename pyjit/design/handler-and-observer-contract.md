# Handler and observer contract

The active [interpreter](interpreter.md), [IR](register-ir.md) and
[frame ABI](call-and-frame-lifetimes.md) share one owning-home model. This page
describes the 2026-11-09 integration source, including observer repairs still
under qualification. [Status](../STATUS.md) separates it from the immutable
qualified golden runtime. Release phases implement RC; they do not impose an
immediate-finalization guarantee on Python programs.

## Generated execution contract

`Python/bytecodes.c` defines semantic operations. The cases generators emit
handlers, operand layouts and effects; `Python/ceval.c` enters their musttail
dispatch domain. Pristine code is compiler-produced register IR. Execution
patches handler identities for specialization and observation without another
stack-bytecode compiler. GC, debugger, feedback, statistics, disassembly and
remote readers must cover the actual handler domain.

| Contract | Meaning |
| --- | --- |
| Field roles and extents | Encoded operands and frontend stack shape |
| Through prefix | Values remain in the same physical homes, not fresh definitions |
| output_kind | Actual register/window/first-result publication, or semantic outcome |
| result_phase | Checked first-result publication before input retirement |
| Effects | Conservative raises, escape, frame switch, suspension, polling and other capabilities |
| Events | Monitoring capabilities of the semantic handler or its wrapper |

Fold capability does not identify every output home. An ignored A on a window
producer cannot create an owner; a first window result moved to A does not move
its variable tail. Likewise, full frontend window reads can include untouched
holes; explicit source fields may point outside that window. Cold consumers
share ReadsHome/DefinesHome/NullableInputs and per-edge Outcome rather than
reconstruct these facts from one extent or guessed opcode range.
NullableHome checks every actual aliased read: a nullable deletion value cannot
authorize a required receiver to be NULL in the same physical cell.

Suspension metadata recognizes actual generator-state writes, not a comparison
against a child's FRAME_CREATED state. SEND can push/resume a Python child
without suspending its own generator. Static provider or ownership refinements
inherit only facts their actual path establishes.

## Publication, release and callbacks

A handler starts from valid canonical homes. Before an opaque callback it
publishes the actual frame, PC, executable, exception state and committed owner
phase. Detached inputs are NULL; an unpublished result remains an explicit C
owner. Decline paths preserve the generic path's inputs. Post-commit errors
retire the actual phase rather than replay a call or restore fictitious owners.

The specialized reader retirement markers publish a temporary result before
receiver retirement. A folded local instead keeps the result owned in C until
successful retirement/refresh, then replaces the current local and retires its
actual displaced owner. A failure closes the C owner and preserves observer
mutations. The checked first-output phase is narrower than whole-handler purity
or a general error root map. See [reader publication](call-and-frame-lifetimes.md#first-output-publication-phase).

Semantic FOR_ITER/SEND/THROW completion distinguishes PRE receiver observation
from POST committed-window observation. Destination definitions differ by edge.
One helper owns the held result and detached inputs through callbacks, publishes
the selected real continuation before closes, and honors later redirects.
Ordinary CALL return remains inline. Implicit StopIteration completion and real
error reporting must each produce their appropriate single monitoring event.

## Executable position and source context

`f_lasti`, branch targets, disassembly and INSTRUCTION events use compact real
word offsets. Eliminated source-only spans are immutable edge records in the
normalized co_linetable envelope. LINE callbacks temporarily expose their
source span through the observed frame object. Source-only records add LINE
context within the single executable-offset domain. Shared cleanup words still
present in this increment are an unfinished inventory.

RESUME owns entry synchronization and PY_START/PY_RESUME ordering. Source
records may describe its outgoing body edge; a source-only landing at RESUME
is invalid. Compiler-generated initial input disposal carries no source origin.
The constructor checks this barrier before admitting code.

```mermaid
flowchart TD
    edge[Observed semantic transition] --> records[Match immutable source edge records]
    records --> display[Publish source cursor and line for callback]
    display --> callback[LINE callback]
    callback -->|success| landing[Cold expected landing and line handoff]
    callback -->|redirect| redirect[Invalidate handoff and commit actual target]
    callback -->|error| recipe[Activation-local source exception recipe]
    landing --> consume[Detach at matching PC and monitoring epoch]
    consume --> line[Continue real landing LINE handling]
    recipe --> unwind[Existing exception and owner unwind]
```

An evaluator activation owns its pending source-error frame/record. Nested C
entry, including finalizers and instrumentation callbacks, links a fresh
activation and restores the parent on exit. The error recipe therefore survives
a nested error during outer result retirement. Frame-object allocation failure
can still record it. The optional frame-object source cursor supplies display
and traceback state; it is not the only copy of handler selection.

A separate cold frame-object record carries expected real LINE landing,
previous source line and monitoring epoch. Public f_lineno remains display/cache
state. Actual LINE mask, tool-identity or restart changes invalidate the old
handoff; unrelated INSTRUCTION changes preserve it. Consumption is
one-shot and occurs before callbacks. A stale configuration gets a fresh
landing event without retroactively replaying completed source events. Exact
assembler relaxation adapters normalize LINE landings while helper words retain
real INSTRUCTION callbacks. Exception selection, debugger redirection and frame
clear invalidate the record before callback-capable retirement.

Tool reuse is checked against the code's last successfully instrumented global
version, even when SetLocalEvents has refreshed its local version field. Reused
LINE bits pass through both removal and addition so old DISABLE decisions do
not transfer to a new identity. Callback replacement alone retains the existing
no-automatic-restart policy: later selected events use the new callback; an
explicit restart re-enables disabled events.

Debugger intent has its own cold frame-object generation. A successful setter
commit advances it even when the selected physical word is unchanged. Source,
jump and semantic-completion callback transactions compare both PC and generation
before returning a cached continuation or restoring source context. If a callback
commits a redirect and then raises, attribution uses the committed real PC;
otherwise the original source exception recipe remains valid. Frame-object
creation and public line-cache reads do not advance the generation. Ordinary
dispatch does not check it.

An identity request checks the actual phase as well as the resolved source line.
It requires the current real LINE wrapper to describe that line, the observed
frame itself and no source/error, suspended or pending-child phase.
It preserves every owner and exception link, including protected regions,
and keeps the current PC while committing redirect generation. An earlier word
sharing the line does not make identity restart a scope or expression.
Other protected transfers still need
their own context and owner proof.

## Boundaries and open work

Specialization guards receiver behavior, representation, type/shape version,
provider and descriptor precedence as needed. Opaque hooks and misses retain the
generic route. Names alone do not prove allocation-free or non-reentrant work;
canonical transfer requires an actual producer chain. Future method/tracing
JIT recovery maps remain separate from these baseline contracts.

Debugger owner recipes remain bounded: NULL requirements and actual iterator
generation must be proved before admitting a target. Remote layout is versioned
and target-described; remote/GDB active source-cursor display is a separate
consumer port still pending. The current integration must complete its native
observer gates, broad repair and parity campaign before promotion.
