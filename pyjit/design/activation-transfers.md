# Activation transfers

This document describes the selected call, construction and resume boundaries;
[status](../STATUS.md) records the selected identity and qualification. The
[frame ABI](call-and-frame-lifetimes.md), [register IR](register-ir.md) and
[interpreter](interpreter.md) define storage, encoding and observation in detail.
Round53's [report](../issues/activation53/report.md) and
[review](../issues/activation53/astra-review.md) record rejected alternatives;
none changes this selected contract. The public C-constructor repair remains
separate from ordinary creation. The [active performance plan](../issues/interpreter-performance56/plan.md)
distinguishes implemented increments from remaining successor work.

The [combined performance proposal](proposals/interpreter-performance56.md)
and [critical audit](../issues/interpreter-performance56/review.md) describe
the architecture and its review. The selected56 subset is described here;
remaining storage-authority and borrowing proposals are identified in status.

## Ordinary calls and returns

An activation owns its function, independently pinned executing code, named
locals and live temporary homes. Thread frames use an initialized arena frontier.
An exact call from a thread frame overlaps its argument homes with the child's
binding prefix. It reserves once and initializes one header; it does not copy
those arguments or acquire another owner for each. Heap callers need a move into
thread storage. Hidden constants are initialized once at construction, never at
resume. Source positions do not authorize execution or transfer.

```mermaid
flowchart TD
    call[CALL with explicit argument homes] --> admit{Exact Python call admitted?}
    admit -->|No| generic[Existing provider and binder]
    admit -->|Yes| storage[Reserve overlapping thread homes]
    storage --> init[Initialize one child header and constant homes]
    init --> enter[Save recipient and continuation; link child]
    enter --> dispatch[Dispatch child in the same evaluator]
    dispatch --> result[Hold canonical result owner]
    result --> retire[Unlink child; retire required homes; release reservation]
    retire --> publish[Publish to actual caller recipient]
    publish --> close[Retire displaced owner]
    close --> resume[Reload callback-selected continuation and dispatch]
    generic --> outcome{Provider outcome}
    outcome -->|Initialized child frame| dispatch
    outcome -->|Immediate owned result| publish
```

The result remains owned across callback-capable child cleanup. Returning a
temporary moves its owner; returning a named local currently duplicates it before
frame retirement, since an exposed frame can retain that local. Ordinary return
publishes inline. It does not enter the generator-completion adapter or normalize
an already canonical result through the generic value bridge.
Certified `RETURN_VALUE_EMPTY` clears thread bindings only. General
`RETURN_VALUE` uses the outlined full-extent cleanup, including retained scratch;
both protect the result, executing code and reservation through callbacks and
frame-object takeover.

The caller's instruction cursor records the executed call while its separate
return PC records future control. A finalizer may expose the caller, install
hooks or select another continuation. Publish consistent ownership/control state
before callback-capable closes, then reload the winning PC. Never overwrite a
callback-selected redirect with a saved cursor. Observation reconstructs source
events from this semantic state.

## Generative construction

The selected path constructs a temporary thread frame using the existing binder
and initializer. Any creation prefix executes normally. RETURN_GENERATOR allocates
the generator/coroutine object and selects one of two ownership moves. A code
plan with `ALL_CREATED_EMPTY` certifies absent scratch at every creation site:
the fast path moves the header and bindings, then initializes destination
scratch to NULL without reading source scratch. Otherwise, the protected copy
view supplies the readable homes, including any retained scratch; the copy
moves those homes and NULL-fills destination gaps. Both clear moved source
homes and release the thread reservation. The object retains its own
executing code and function owners. Initial resume uses the actual successor and
the explicit incoming recipient.

```mermaid
flowchart TD
    provider[IR call or native vector provider] --> bind[Bind inputs into thread frame]
    bind --> prefix[Execute creation prefix if present]
    prefix --> constructor[RETURN_GENERATOR]
    constructor --> allocate[Allocate generator or coroutine]
    allocate --> proof{All creation scratch proved absent?}
    proof -->|Yes| bindings[Move header and bindings; initialize destination scratch]
    proof -->|No| copy[Move readable copy view; initialize destination gaps]
    bindings --> seal[Seal CREATED heap frame; release thread reservation]
    copy --> seal
    seal --> return[Return canonical generator or coroutine owner]
```

Cell/closure setup and generator-expression iterator acquisition can precede
construction. Code flags alone cannot authorize skipping them. Async-generator
hooks, origin tracking, allocation errors, observers and alternate evaluators
retain their established behavior. Allocation occurs once at creation, not on
each resume.

Destination scratch initialization remains real work even on the empty-created
path. Logical `NO_HOME` proves absence, not physical NULL or readable bytes;
it does not permit removing the current NULL-control initialization. Constants
move with the bindings. Moved-source NULL stores preserve reusable arena storage.

Public `PyGen_New`, `PyCoro_New` and `PyAsyncGen_New` construction from an
existing frame bypasses ordinary RETURN_GENERATOR. This cold ingress explicitly
installs the initial discard recipient, acquires the frame-object association
owner, and completes subtype state before publishing that association and
GC-tracking the object. Fallible coroutine-origin construction precedes the
ownership move. Failure frees the untracked allocation and consumes the stolen
input frame reference. The subtype wrappers return the common constructor
directly. These repairs do not redirect normal Python creation through this
public helper or change the ordinary call/resume ABI.

## Resume, suspension and completion

Exact SEND/FOR admission already dispatches the child within the same evaluator.
It publishes the child input, frame/exception links and recursion state, then
polls and dispatches. Initial non-None send and running/finished state have their
own semantic checks. Real parent frames remain visible while their children run.

The actual terminal `IR_ENTER_GENERATOR` and its source-checked IO/guard recipe
select the yielded-result recipient. FOR uses A, including folded locals and
discard; SEND keeps B+2. This recipe does not supply normal, exceptional or
suspended-home authority.

GET_AWAITABLE can transfer an exact coroutine's input owner directly when its
callback-free state predicate accepts it. Folded locals still duplicate; other
states/custom awaitables keep the general helper and error path. The two arms
meet only a first-publication owning-result fact, with no token-identity or
home-readability claim.

Yield reverses those links, marks suspension and transfers a canonical owner to
the parent's declared recipient. Completion first retires the child while
holding its result. For a normal parent, the generator-return adapter decodes
the pinned immutable SEND/FOR word and performs the corresponding canonical
transaction. An interpreter-owned entry parent instead receives the result
directly through its recipient. Source lines and columns are not execution
authority.

```mermaid
flowchart TD
    parent[Executing parent SEND or FOR] --> link[Detach input; link child and exception state]
    link --> child[Poll and dispatch real child frame]
    child --> outcome{Child outcome}
    outcome -->|Yield| suspend[Unlink; mark suspended; publish yielded owner]
    outcome -->|Return| cleanup[Hold result; unlink and clear child]
    cleanup --> entry{Interpreter entry parent?}
    entry -->|Yes| direct[Publish to entry recipient]
    entry -->|No| complete[Decode immutable SEND or FOR completion]
    complete --> commit[Detach window; publish result and completion PC]
    commit --> retire[Retire held owners; reload winning PC]
    suspend --> parent
    retire --> parent
```

SEND completion may overlap an input home. FOR exhaustion retains the last item
binding. Observed PRE/POST callbacks, failures and supported observer redirects
retain the existing semantic transaction. Ordinary and interpreter-entry
recipient paths handle their pending redirects. Literal native completion
terminals call `_PyEval_CompleteForIter`, `_PyEval_CompleteSend` or
`_PyEval_CompleteSendThrow` without a runtime kind argument; FOR also omits the
destination argument. They share the existing completion body and retain
general publication. VM generator return separately decodes the immutable
parent word and selects its canonical-input instantiation. No persistent chain
cache, completion descriptor or extra continuation-kind store is installed on
every resume.

## Evidence and limits

The selected header occupies 14 eight-byte words on the measured configuration,
versus upstream's 10. Extra execution-image and recipient state, canonical-value
classification, destination initialization and general full-extent cleanup
remain costs. Overlap and the initialized frontier save work upstream still
performs. Source store
counts and flat samples alone cannot establish their net elapsed-time cost.

Round53 tested direct final-home construction across IR/native calls, a shared
completion terminal, and a narrower native-only constructor. All removed the
intended native work and passed focused semantic checks, but all failed measured
breadth. Native-only construction improved generators while repeatedly losing
on DeltaBlue and Richards. These implementations remain experimental branches,
not hidden selected behavior. See the [benchmark tables](../issues/activation53/benchmark-tables.md)
and [next plan](../issues/activation53/next-steps.md).
