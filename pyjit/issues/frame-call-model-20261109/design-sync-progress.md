# Current design synchronization

Updated only the five assigned canonical architecture pages:
register-ir, interpreter, call-and-frame-lifetimes,
handler-and-observer-contract and runtime-architecture. They describe the active
frame/call source and distinguish it from qualified golden and the earlier
engineering measurement. They do not claim full-suite admission or parity.

The current model now uses compact executable/source-edge coordinates, plain
result recipients and real continuations, known binding plans, bounded pending
kwargs, semantic completion/resume, initialized chunk frontiers, precise
generated physical output mapping and shared cold ownership contracts. Observer
sections cover activation-local source errors and independent LINE handoff.
The callback-epoch capture refinement is still a source18 integration repair;
the pages describe its intended invariant, not a completed native gate.

Retained detail includes canonical representations, RC/GC and ownership versus
future-read liveness, feedback/execution copies, provider fallback, defaults,
frame escape, callback phases, R4/R5 publication proofs and future JIT recovery
boundaries. Prior NOP/bool/tagged-discard protocols are linked as history rather
than advertised as current ABI. Shared release-word and debugger/remote gaps
remain explicit.

Local link targets, code-fence balance and whitespace checks pass. Mermaid
flows were reviewed against the source phase order; mmdc is unavailable, so no
rendered-diagram verification is claimed. No runtime edits or native jobs were
performed for this documentation batch. Awaiting independent/coordinator review.
