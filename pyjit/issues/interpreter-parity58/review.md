# Interpreter parity58: coordinator peer review

Date: 2026-11-14. Astra authored the [design](../../design/proposals/interpreter-parity58.md)
and [research ledger](research.md); the coordinator reviewed both independently
and requested the corrections below. This is a design review, not a new cold
implementation review or runtime qualification. No implementation or performance
result is claimed.

Disposition: **approve bounded prototypes under the stated gates**. The design
provides a consistent ownership/effect contract across compiler, generator,
physical execution and activation completion. It does not yet demonstrate that
the admitted opportunities cover the entire upstream gap. Selected57 remains
unchanged; the as-built architecture is not replaced by this proposal.

## Findings resolved in the design

| Concern | Resolution and consequence |
| --- | --- |
| Repeating rejected local fixes cannot justify parity | Distinguishes whole typed intervals from getter inlining and cross-word value flow. Rejected round57 changes remain rejected. The 84.6k instruction budget and coverage gates make insufficient scale explicit. |
| Existing optimizations were at risk of being proposed anew | Identifies overlap, frontier, inline return, direct generator entry, canonical yield, current borrow/copy passes and good SHAPE_LOCAL_TEMP as implemented. No release charge for the debug-only home scan or a normal-ABI save around each handler. |
| Borrowing could become a disguised zero-opportunity pass | Retains the negative 21-file/426-CFG census. Names a distinct acyclic snapshot-merge candidate and requires actual post-pass frequency and positive net materialization savings. Demote it if absent. |
| Upstream borrowing changes the ownership comparison | Documents upstream StackRef borrowing and borrowed arguments versus current owning homes. Keeps a borrowed-activation rival separate, with its own frame-reader, escape and lifetime admission. |
| A virtual value could lose its GC owner or evaluated snapshot | Existing owners anchor aliases. Physical homes remain owning or NULL. Effects, mutation, observation and suspension force demanded materialization. Initial regions have no unpublished owners at effect exits; no new root protocol is assumed. |
| Region metadata and invalidation were underspecified | Adds immutable plan rows, executing rows and reverse site index; entry-last install and disable-before-interior-patch. Covers data guards versus handler changes, instrumentation/tool reuse, constructed/marshaled images, EH/resume entries and debugger defusion. Effects are never replayed. |
| Restrictive short regions might have too little coverage | Compares resident-value caching, separately admitted borrowed arguments and larger-block/alternate dispatch. A low upper bound on eligible traffic invalidates catalog-as-parity claims. |
| Foreign-runtime analogy could substitute for evidence | Pins V8/LuaJIT primary source references and limits their relevance to equivalence/materialization and native ABI planning. Python ownership, effects and observation remain explicit. |
| Instruction and elapsed objectives were conflated | Uses the candidate denominator: 30.38% instruction or 24.99% cycle removal for comprehensions. Counts are separate from time; flat samples cannot supply an exclusive savings budget. |

## Remaining admission requirements

The first change must identify actual native work that disappears. A small
successful handler is evidence for its mechanism, not proof of parity. No full
static storage, bitmap or IRSC2 experiment becomes a global prerequisite; new
reader or ownership authority needs its own coherent qualification.

Call/resume work must count entry and exit together. Recipient specialization
must honor current parent bindings and callback redirects, and retain independent
code pins and required exception/running state. Header size is not a success
metric; the earlier 80-byte trial remains negative evidence.

Focused tests accompany bounded changes. Coherent protocol/composition gates
receive full diagnostics and balanced direct benchmark comparisons. Preserve
known debugger limits and the existing PrettyPrintTests quarantine without
claiming a green full suite. Publish residual coverage and per-case losses.

The coordinator reviewed the final document diff, local links and Markdown
fences. No new builds, tests or measurements were needed for these documents.
