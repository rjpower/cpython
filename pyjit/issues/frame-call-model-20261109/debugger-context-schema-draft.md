# Cold context schema: smallest coherent next step

Proposal only. Actual homes and producer sites are in
`debugger-context-census.md`; identity is an independently admitted increment.
No general protected/suspended implementation is admitted by this document.

## Start with derived metadata, not a second serialized format

The concrete ordinary-try/caught-exception cases retain their semantic
PUSH_EXC_INFO/POP_EXCEPT operations and canonical EH table. First derive a
per-request cold context table from that final IR, together with the existing
owner analysis. This avoids another co_linetable/marshal/debug-offset migration
before demonstrating that compiler-only annotations are necessary. It adds no
code-object field, cache lifetime or dispatch work. Cache it only if actual
repeated debugger analysis warrants that cost.

Use explicit private records rather than a frontend stack projection:

```
DebugContextNode:
    kind                 INHERITED or CAUGHT
    parent               context node restored on exit
    producer_pc          actual PUSH_EXC_INFO site, or no producer for root
    previous_home        actual saved-previous owner, not an unwind floor

DebugSiteState:
    actual_pc
    phase                REAL_BEFORE / SOURCE_EDGE / YIELD_PRE /
                         SUSPENDED_AFTER_YIELD / RESUME_ENTERED
    context              unique node, UNREACHABLE or AMBIGUOUS
    previous_owner       current physical home/provenance of saved previous

DebugTransferPlan:
    current_phase, target_phase
    preserve_homes
    restore_actions      none / caught-link transfer / named-local restore
    detach_homes
```

The first builder supports INHERITED/CAUGHT only. Ordinary protected try changes
its EH destination by selecting a new PC, without adding an owned context node.
The actual census's PUSH5(prev3) and PUSH7(prev2) create distinct caught nodes.
Normal and exceptional finally copies get different states despite equal source
lines. The bare-reraise site's caught node must survive even with no explicit
value operands. Shared physical ReadsHome/DefinesHome/Outcome/NullableHome remain
independent from handled-exception context.

Do not treat a producer PC as a dynamic owner generation. For a retained caught
node require a pathwise no-pop/no-reentry/no-rebind proof from its actual entry
phase to the current/selected site, analogous to the bounded FOR proof. A merge
with different contexts is AMBIGUOUS until a checked set of alternatives can be
handled; do not silently choose the first predecessor.

## Builder and validation contract

Walk ordinary **and exceptional** edges over actual allocated homes. Before
using a field, validate the handler, relevant role/home bounds, successor bounds
and canonical EH structure using shared checks. The existing completion verifier
already requires sorted nonoverlapping marked EH entries; reuse its rule rather
than inventing a different table lookup. Allocation limits fail before mutation.

Transfers must reflect actual semantic operations:

- PUSH_EXC_INFO B consumes the delivered exception and writes previous owner B,
  new exception B+1; it establishes the corresponding caught node.
- POP_EXCEPT's exact input must be a proven saved-previous value for the active
  node. It restores the parent context and consumes that owner.
- COPY duplicates an ownership edge with the same value provenance; SWAP moves
  existing ownership edges. Neither is an arbitrary fresh exception definition.
- Named locals, explicit inputs, sparse window holes and physical output homes
  use the common contracts. An unknown overwrite of a required context owner
  ends its availability; a later equal value does not recreate it.
- An exception edge clears the actual unwind suffix, writes lasti/exception,
  and preserves active handled context until real PUSH/POP operations change it.
  No normal-only CFG proof can justify active exception ownership.

This metadata is not supplied by the caller. Its coherence comes from deriving
it from the final encoded program on every request. If code construction or
replace changes that program, a subsequent request derives fresh metadata.
No trusted eligibility bit or stale compiler map survives. The builder must
refuse an unsupported/ambiguous recipe rather than grant unsafe jumps. This is
not permission to reject normal execution of an otherwise supported code object.

A shared verifier may later run during construction and cache the result; it
must remain the same final-word proof. A future serialized compiler annotation
is admissible only if constructor validation proves its semantic completeness
and owner correspondence, not just record bounds. The census has not yet shown
that serialized annotations are required for the first caught cases.

## First supported transfers after identity

1. INHERITED to INHERITED, with independently available target operands: ordinary
   try-body, into/out-of ordinary try, and normal finally copy. Keep missing
   context refusal for bare raise/except targets.
2. Same active CAUGHT node, preserving its actual saved previous owner and
   handled exception link on every admitted path. Do not yet leave that context.
3. Caught exit only after target exception-link resolution is separately proved.

The third step is not a direct tstate->exc_info assignment. A callback-owned
except may store the target's old handled exception in a callback saved-previous
cell. Resolve the target link through actual callback/activation/generator
contexts, prove the cell lies outside every live child overlap, then transfer the
new context there. Preserve the callback's current sys.exception(), and ensure
its eventual POP_EXCEPT restores the new target context. The coordinator-run
caught-callback probe checks both sides. If no safe link can be resolved, keep
that exit unavailable; do not guess or mutate overlap.

Named exception-as cleanup is a distinct reviewed action, not automatically
implied by POP_EXCEPT. The census shows local error2 clearing at PCs13/14; match
unchanged debugger tests and ordinary scope behavior before adding its action.

## Extending the same schema

Comprehension adds a SAVED_LOCAL node: actual LOAD_FAST_AND_CLEAR producer,
current transported home and named destination. The census establishes saved
marker home3→local1 in the body, and transported home4→local1 in cleanup. Prove
all admitted exits/paths restore that exact owner; preserve cell objects as
owners. Do not restore a local merely because its slot is non-NULL.

With adds actual exit callable/self owners (census3/4), with explicit provenance
from LOAD_SPECIAL and its SWAPs. Entry requires those owners; a scope exit plan
must follow existing debugger cleanup policy. For a shape the final IR cannot
unambiguously identify, collect compiler region intent and peer its validation
before adding annotations. Do not infer from source line or callback names.

Yield phases are not interchangeable. YIELD_PRE redirects cancel the pending
yield before the semantic operation suspends, so the outgoing value's borrowed
or held owner must remain valid until that redirect decision. Already suspended
state and RESUME_ENTERED need separate recipient/exception-link handling; NoDst
is an intentional sent-value sink. Delegate ABI and PEP479 handlers stay intact.

## File ownership and gates

B owns the cold context builder/plan/application in frameobject.c and focused
code/trace tests. Shared cold operand/CFG/EH readers belong in internal helpers
only when both constructor and debugger consume them. A peers exception-link,
yield and callback ownership; runtime entry/exit changes remain A-owned. No
assembler, codegen, marshal or remote changes are needed for the first derived
INHERITED/CAUGHT tables. If a concrete later shape requires compiler annotations,
B owns regcfg/assemble and constructor consumers; coordinate codegen/shared
headers before mutation.

Keep existing try-entry/except-refusal/finally/identity tests unchanged. Add bare
raise entry refusal, matched caught identity and same-context moves, and the
callback-owned exception control before caught exits. Then use the actual
comprehension and yield probes to admit their separate producer/action rules.
The full release diagnostic remains required; this staged builder is not a
waiver for broader debugger behavior.

## Independent peer and identity checkpoint

A approved the bounded derived-table direction with three important constraints.
INHERITED is the caller's context and can be non-None; a bare raise in inherited
context can legitimately use that exception. Refuse entry into an actual caught
node from root rather than treating all bare raises as invalid targets. COPY and
SWAP preserve previous-value lineage with distinct owner edges, not new caught
generations. SOURCE_EDGE describes the post-from semantic phase and cannot be
replaced blindly with the context at event.to. Caught exits remain unadmitted
until saved-previous link resolution is explicit.

The independent real-LINE identity implementation is now source-frozen for
coordinator qualification. It additionally requires that the first actual
marklines candidate for the requested resolved line is the current real PC;
line equality alone cannot silently select a later duplicated-line instruction.
It requires the actual LINE wrapper and excludes source/error/suspended/pending
phases, preserves all homes and exception state, avoids None warnings, and commits
redirect generation. A actual source peer approved; native tests are pending.
