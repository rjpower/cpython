# Shared completion specialization: bounded source shape

This is a next experiment proposal, not source admission. Decoder33 remains an
isolated experiment whose measurements decide whether to retain it. The direct
heap-creation and saved-continuation alternatives remain separate in
`generator-completion-and-creation-alternatives.md`.

## Smallest useful change

Keep the exported `_PyEval_CompleteStep` signature and all generated native
completion calls. Move its current body, in order, into one `static inline
Py_ALWAYS_INLINE` implementation in `Python/ceval.c`. Give that implementation
two extra compile-time inputs: semantic kind and whether the owned result is
already a canonical home value. Kind is already an argument; only the canonical
fact is new. No new frame fields, code metadata, cache entries or public APIs.

Illustrative boundaries, with the existing arguments abbreviated:

```c
static inline Py_ALWAYS_INLINE _PyIRStepResult
complete_step_impl(/* existing arguments */, enum _PyIRStepKind kind,
                   bool canonical_input);

_PyIRStepResult
_PyEval_CompleteStep(/* existing arguments */, enum _PyIRStepKind kind)
{
    return complete_step_impl(/* same arguments */, kind, false);
}
```

The VM-parent portion of `_PyEval_CompleteGeneratorReturn` selects SEND/FOR from
the positively declared immutable word, then calls this same implementation
with literal `_PY_IR_SEND, true` or `_PY_IR_FOR_ITER, true`. Its interpreter-parent
branch stays unchanged. Native and THROW results continue through the generic
entry with `canonical_input=false`; they cannot inherit a canonical fact from a
handler name. The shared implementation begins with either ordinary Publish or
a debug home assertion. It retains one source copy of the commit and retirement
logic. There are two specialized machine paths, not two manually maintained
ownership transactions.

The existing generator `return_to_caller` emitter is the positive boundary for
canonical input: the exact checked return chain has a canonical home input; its
fallback explicitly publishes before calling CompleteGeneratorReturn. Inspect
every call site again before adding the fact. No body-name-only bypass and no
new source proof are needed. An unsupported return-chain mutation must retain
the fallback publication.

## What becomes constant, and what must remain dynamic

Inlining into the two literal-kind calls gives the compiler ordinary C constant
conditions. FOR removes SEND destination installation and third/fourth window
loads, and its sent/exception owners are constant NULL. SEND removes the FOR and
THROW alternatives, and its exception owner is constant NULL. Both canonical
paths omit Publish's numeric object/type/canonicalization checks. This predicts
which source conditions are eligible for elimination, not a measured saving or
a guarantee about final machine layout.

Keep destination classification dynamic: SEND may target a local, a temporary,
an aliased window home, or NoDst. Keep receiver/cursor/sent closes and their
deallocation paths. FOR may return an owned mortal value that must be retired.
No result is boxed merely to simplify the interface.

The observation adapter remains the existing out-of-line `_PyIR_ObserveStep`.
Do not add per-tool checks to the normal path or treat a non-NULL monitoring
pointer as proof that an event is enabled. The existing guard and receiver test
select PRE observation, and the existing POST adapter handles branch/source
events. Native allocations, failures and owner retirement stay on their current
paths. A new deferred-owner struct or shared six-owner close helper would add
stack/argument work and obscure constant NULL elimination; neither is part of
this experiment.

## Ownership and reentry ledger

| Phase | Required invariant |
|---|---|
| Input | Result has one C owner; receiver/window still frame-owned. Canonical assertion observes bits without taking a count. |
| PRE observation | Preserve input window and held result. Receiver borrows stay anchored. On redirect, never reuse the borrowed window; close held result and reload the latest frame PC. |
| PRE error | Preserve callback exception across result close. Use published PC/error recipe after close; do not cache the old continuation. |
| Callback-free commit | Capture receiver, cursor, sent and THROW exception before clearing their homes. Capture an aliased destination only after detaching the entire window. Install the result once, clear its C owner, then publish return_pc. |
| POST observation | Observe actual semantic site and committed owner state. Keep existing scoped phase, source error and generation rules. No fresh PRE authority is inferred here. |
| Successful retirement | Publish observed continuation before closes. Retire detached owners in the existing order; reload actual frame PC after all possible finalizers. No slot or PC writes follow that reload. |
| Failed retirement | Hold the raised exception across all closes; restore it, then return the existing typed error result using the latest published PC adapter. |

The destination can equal receiver, cursor or sent. Detaching first prevents
double retirement and ensures the new result never resurrects an old receiver
origin. Local displacement remains a separate C owner until after publication.
No change to ordinary CALL return, recursion accounting, CREATED/send checks,
generator exception-state unlinking, child retirement, PEP523 or GC traversal is
proposed. The dying child is already retired before this helper is called.

## Code-size and verification gate

The generic implementation is currently 1858 bytes in both release32 and
release33. Literal-kind inlining can duplicate significant observed/error code;
the hypothesis must be checked rather than assuming that removed calls win.
Record symbol sizes and total affected text for generic Step, generator-return
wrapper and any compiler-created clones. Inspect SEND/FOR unobserved paths for
remaining generic kind and Publish tests, stack spills, and extra calls. Inspect
observed paths to ensure the same PRE/POST adapters and error ownership survive.
If growth or spills outweigh simplification, retain decoder-only and stop.

Required focused checks: existing SEND/FOR/THROW payload and callback-error tests;
local/temp/NoDst and window-alias constructor/execution cases; instrumented FOR
completion added in decoder33; far physical continuation tests; mortal returned
value and displaced-local finalizer reentry; setter-then-raise exception
preservation. Keep canonical-return mutation negatives and verify a changed
return chain still emits publication before the helper. No generated provider
text should change in this C-only experiment.

Independent source peers precede implementation. Root owns builds, native
selectors, disassembly receipt and the same paired generator/coroutine elapsed
and PMU protocol. The accepted32 profiles locate completion work but do not
assign its sampled share as an available gain budget.

## Source34 checkpoint

Root admitted the isolated implementation after decoder33's corrected gate
passed 575 tests in each profile. Source34 changes only `Python/ceval.c`:
`complete_step_impl` owns the existing body, the exported generic entry passes
false, and the VM generator return branch passes literal SEND/FOR and true.
The interpreter-parent path remains unchanged. No generated text, metadata,
header, ABI or test fixture changes accompany it.

A host UV check compared the entire owner body from the window capture through
its final return against HEAD and found exact text equality. It also verified
one body, three intended call shapes, and the two generated callers' canonical
owned-temp assertions. `git diff --check` passes. These are static source checks;
root owns the release/debug compile, existing focused native gates and compiled
code-size/spill inspection. B independently approved the actual source diff.
