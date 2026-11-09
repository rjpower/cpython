# Pathwise NULL-only PRE requirements: implementation brief

Proposal for independent peer and root admission after the performance packet.
No runtime/header/test changes or native execution by B. This refines the
reviewed null-only-consumer-interface.md for the current488e85fa3a8 consumer.
It changes no protocol-region policy and adds no per-test setter branch.

## Actual first witnesses and scope

The three existing peepholer methods test_setting_lineno_no_undefined,
test_setting_lineno_one_undefined and test_setting_lineno_two_undefined stop
at real INSTRUMENTED_LINE pc17/17/18, then request line3. All have zero IRSM
records in ownership31-remaining-phases.json. Their compiler writes scratch6
with COPY, takes it into x through STORE_FAST, later uses6 as CALL's optional
self, and clears it on FOR exhaustion before the current stop. The global
ever-written exclusion loses this valid NULL fact.

First new permission requires all of:

- a freshly validated real-LINE PRE stopped view on the actual observed frame,
  with no source/error cursor, pending child/redirect or suspended phase;
- an ordinary nonsuspending function, excluding generator/coroutine/async-gen
  entry and external/child-overlap entry assumptions;
- inherited handled context and positively empty stopped/target save columns;
- a real target whose incoming demands contain no VALUE owner, only proved
  NULL_ONLY homes; no VALUE/NULL_ONLY conflict is allowed;
- original-path all-ingress NULL expectation at the read, actual executable
  reader capability across every alias, stopped-view must-NULL and actual cell
  NULL for each incoming NULL_ONLY home.

Named-local binding safety, generation and existing source-target restrictions
remain separate. This is not SOURCE_POST, committed POST, retained-root,
same-caught, restoration-action or yield admission. Existing PRE/neutral/strict
permissions and their old globally invariant NULL allowance must not broaden
or regress as an accidental consequence of the new matrices.

## Shared cold transfer contract (H)

`_PyIR_NormalPhysicalTransfer(word, arg, edge, home, nl, limit)` returns one of
PRESERVE, SET_NULL, UNKNOWN or TRANSPORT(source_home). Decode actual physical
roles/output mapping; no runtime dispatch walk or opcode-name eligibility list
is introduced. Apply transports from the old row simultaneously. A source
outside tracked scratch cannot contribute a must-NULL bit. Invalid geometry or
unproved affected homes loses knowledge conservatively.

Minimum exact facts come from emitted code: COPY maps B to B+arg; SWAP exchanges
B and B+arg-1; STORE_FAST clears its taken scratch A; FOR exhaustion clears
B/B+1. Other known outputs/consumed inputs are UNKNOWN unless their precise
clear is proved. A WINDOW_DST_FIRST moved output can establish NULL only at
the actual moved temporary, with final output alias precedence respected.
An opaque child activation loses its affected tail. The ordinary LOAD_GLOBAL
single-output shape must preserve untouched optional-self6 while writing5.

UNKNOWN is not a definition, owner or provenance repair. Existing
_PyIR_DefinesHome, semantic outcomes and ObservedHomes.old_cut remain their
separate contracts. Generic reads alone do not prove a NULL kill. H supplies
source/contract tests for output aliasing, untouched holes, simultaneous
transport and conservative unknowns before B wires the consumer.

## One forward fact and the existing backward requirements (B)

Add request-local arrays to RegisterResumeAnalysis and its existing clear
function. Use checked n×stride, byte products, sums and queue sizes; allocate
nonzero backing for zero logical extents. All fallible work precedes frame
mutation. An allocation failure propagates, without dropping obligations.

1. **Forward must-NULL.** `register_resume_null_states()` uses existing sparse
   forward/reverse edges, one bounded queue and reached/queued flags. Seed only
   ordinary fresh-frame scratch NULL, excluding constant homes. An unreached
   node has no fact; first ingress copies, later ingresses intersect. An entry
   backedge intersects the initial-entry state too. Facts then only lose bits,
   so the worklist converges. Compute normal edges from the old row using the
   shared transfer. Do not scan all source records per iteration.
2. **Error facts.** Ordinary exceptional edges start before successful defs;
   initially forgetting all scratch facts is conservative. Source callback
   errors may use a selected POST row only when publication is positively
   proved; otherwise lose knowledge rather than assuming an inline hook has
   finished. Payload homes are not NULL. The first three witnesses have no
   error/source records, so precise partial-error state is not required to
   make their gate pass. Existing EH geometry and owner requirements remain.
3. **Backward classification.** Refactor the existing liveness pass, not a
   second stack/protocol model. At each actual read, classify NULL_ONLY only
   if that node's original incoming forward fact is NULL and
   _PyIR_NullableHome on the recovered executable handler permits every alias.
   Otherwise classify VALUE. Preserve the old globally invariant omission
   where its existing proof applies.
4. **Backward transfer.** Carry VALUE and NULL_ONLY in separate matrices and
   OR them independently at joins. COPY/SWAP map outgoing requirements back to
   their old source homes simultaneously; they are not fresh-output kills.
   Other proven output definitions discharge incoming demand on normal
   outcomes, not ordinary PRE errors. Source error needs follow the same
   selected success outcome. Ordinary error prefix needs join after normal
   definition kills as today. Required aliases classify a read VALUE; across
   CFG joins an incoming home with both bits is a refusal, not permission to
   erase its NULL expectation.

Keep `analysis.required`/`safe` as the conservative ALL-demand projection used
by existing strict adapters: VALUE|new NULL_ONLY. Preserve the old global
invariant omission in that projection. Additional VALUE and NULL_ONLY matrices
may be retained for the new permission; aggregate allocations must be checked.
Do not relabel the existing safe array as VALUE-only and thereby let neutral
POST, FOR/caught slices or pending calls silently ignore new NULL obligations.
The NULL-aware empty-target predicate is a separate cold Plan check.

## Plan and reproof

Add `register_null_only_target()` within the shared Plan path. It accepts only
the scope above, checks VALUE is empty and NULL_ONLY conflict-free, and requires
every NULL_ONLY demand to be present in the current must-NULL row and physically
NULL. It creates no owner and adds no preserve bit for those homes. Keep the
existing saved-column/context checks even when target VALUE demands are empty.
Targets with other roots continue through their prior ALL-demand proofs; this
packet does not add a retained-root algorithm.

Warning preparation recaptures the stopped phase and rebuilds forward/backward
facts, actual handler capabilities and Plan. A newer setter generation wins.
Getters do not count as intent. The final application uses the existing
detach/install/PC-generation commit/source-error invalidation/close sequence.
There are no old frame writes after callbacks and no per-home hot checks.

## Files and first gates

B: Objects/frameobject.c (Analysis storage, forward/backward consumers and Plan)
and focused Lib/test/test_code.py. H: Include/internal/pycore_ir_outcomes.h
(cold transfer query) and its existing contract-test location. No constructor
or generator schema change is presumed; any additional client change needs
its own concrete proof. A peers callback/phase and reentry edges. Root owns
native builds, all tests, integration and isolated measurement.

Retain all three original peepholer source/result/warning/co_code assertions.
Add focused positives for a COPY→STORE_FAST-cleared optional home and a later
FOR-done re-established NULL. Add negatives for a real method receiver, a
required alias, a forward join containing a non-NULL path, a VALUE/NULL_ONLY
conflict, unsupported transport, and an EH/source path which loses the fact.
Include getter/nested-warning and actual-handler-change reproof controls using
existing observation helpers. These are new tests, not rewritten originals.

Root first runs the three original witnesses plus these bounded methods in
both profiles, then the established frame/observer/contract core. Existing
pending-child, FOR/caught, neutral/committed POST and saved-binding controls
must retain behavior. The two unresolved except-arm cases in the scope audit
remain probe-first; no lexical ban or refusal-test port is part of this packet.
