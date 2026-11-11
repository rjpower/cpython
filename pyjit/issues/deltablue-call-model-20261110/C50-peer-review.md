# C50 component proposal review

Reviewed `C50-implementation-proposal.md` against runtime48 `5559f60093e`,
the grouped Astra review and the ordinary method/CALL generator and runtime.
This is proposal review, not actual-code or performance approval. No runtime
edits or builds were made.

The two live words, direct current-neighbor check, capture-once fallback,
ordinary activation owners and shared entry implementation are the right
bounded experiment. Before implementation, resolve these two findings:

1. **Exact source-coordinate equality admits no ordinary DeltaBlue pairs.**
   A read-only host compile/disassembly of the unchanged kernel found 43
   adjacent zero-argument pairs: all 43 share line ranges; none share complete
   coordinates. LOAD_ATTR ends before the CALL's closing parentheses. Preserve
   both original positions and use a justified conservative line-range rule;
   setting the caller PC to CALL before admission preserves its separate error
   columns. Recheck the count with the selected runtime. Do not merge locations.
2. **A's terminal producer cannot accept C's consuming suffix yet.** A validates
   a terminal multi-output producer and retains facts for publication. C adds
   another uop consuming those owners and has no external pair output. State
   the concrete positive contract for that consumer before changing code:
   prove the canonical producer, stack positions and terminal consume/transfer,
   reject intervening transformations, and give every owner exactly one home
   on fallback or frame commit. A handler-name exemption or suppression of final
   stores is insufficient. Add negative generator cases for an altered suffix,
   reused owner, transformed pair and changed argument geometry.

Use `_PyCode_InstrTarget` for the nomination-time incoming-edge scan; it already
decodes the signed/absolute forms through IR metadata. Bound the scan by the
immutable code length, reject unsupported entry kinds, and avoid repeating it
on hot hits. CALL specialization must also nominate its preceding method,
because the normal specialization order otherwise misses the pair.

The actual implementation must still establish current instrumentation epoch
and raw CALL handler before capture. `_PyIR_RealHandler` unwraps instrumented
handlers and cannot establish plain execution. GIL-held code must have no
callback, decref, allocation or GIL release from this check to commit/fallback.
Decline unsupported DTrace/LLTrace and free-threaded cases. Keep function/code
version, arity, PEP523, recursion and stack admission current. A failed admission
must publish the captured owners and execute the original CALL exactly once.

Sharing `PushExact` and frame-entry/continuation source is reasonable, subject
to release assembly proving that ordinary exact CALL gained no out-of-line
boundary. Document which physical window cells are NULL or owned at each helper
entry, including TEMP receiver/output aliasing. Self must occupy the ordinary
overlapping argument home before frame publication; function transfers directly
to `f_funcobj` with independent code ownership.

Structural installation/hit evidence is required beside behavior checks. Source
and observer tests must prove the refinement ran before replacement/enablement;
otherwise passing fallback behavior cannot qualify it. Independent CALL entry,
live neighbor replacement, stack/recursion fallback, descriptor/code mutation,
temporary capture and ordinary return reentry remain admission gates. Coordinator
release compilation, native review and balanced timing are pending.

## Revised proposal review

The revised proposal removes source-position equality entirely. That is the
correct contract: source maps provide attribution and observation; equality
does not establish semantic safety. The earlier suggested same-line restriction
is withdrawn. Preserve both PCs/columns and prove control-flow, exception and
observer conditions directly. The initial no-exception-table restriction is
an explicit conservative scope choice. Nomination now uses
`_PyCode_InstrTarget` and acknowledges existing execution variants' canonical
stats accounting.

The proposed checked terminal-consumer descriptor resolves the A composition
problem in principle. A's source producer remains terminal and independently
validated. The execution variant can consume its complete proven owner vector
at the declaration boundary, with a checked fallback or shared frame commit,
and report unreachable so ordinary publication cannot follow. Reject changed
source/format, owner order, suffixes and reachable fallthrough. Include actual
refined handler effects and base/payload mappings in implementation review.

One ordering detail needs explicit implementation treatment: the declaration
boundary follows DUP and TAKE. Eligibility checks whose failure returns to
ordinary lookup must run before capture; after capture every failure must
publish the captured pair and execute CALL. Use validated pre-capture hooks at
the right guard boundary or retain the latter fallback protocol. Never emit an
ordinary-lookup fallback from the terminal consumer after moving the receiver.
With that ordering explicit, the revised direction is approved for the bounded
experiment; actual-code/native/executed evidence is still required.

The final revision explicitly places entry preflight before the lookup chain,
target-kind validation after lookup guards but before the first capture
statement, and terminal consumption after DUP/TAKE. It forbids post-capture
ordinary-lookup fallback and adds conservative execution effects. This resolves
the ordering issue. **Proposal approved for the bounded implementation.**
Refinement-entry counters include misses and are not activation-hit counts;
the revised proposal now states that limit correctly. Native/component and
merged execution review remain separate admission steps.

Consumer role binding must use the checked origins as well as canonical bits:
guarded payload duplicate for function, unchanged receiver TAKE for self.
Expose or reuse A's validated source kinds/matcher; do not infer these roles
from variable spelling or merely from a two-owner output vector. This agreed
implementation detail preserves the approved ownership contract.
