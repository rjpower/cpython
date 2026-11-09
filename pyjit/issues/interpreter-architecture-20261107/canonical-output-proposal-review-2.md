# Canonical shape result: proposal review 2

2026-11-07. **Decision: accept the amended R5 design for a bounded
implementation after R4 integration.** This supersedes the disposition in
`canonical-output-proposal-review.md`; it does not claim that R4 or R5 is
implemented, qualified or faster. I reread the amended proposal against
runtime `225480a4250` and the existing home, shape-writer and generator
paths. No runtime code, build, test or workload was changed or run.

The internal store boundary now has one explicit rule: a caller of
`_PyObject_StoreInstanceValue` passes NULL or a borrowed canonical home
value. The proposed `assert(_PyValue_IsHome(value))` at function entry is
fatal in a debug build, before any duplication, callback or store; the
configurable home-violation reporter is not its enforcement mechanism. The
runtime rebuild/port policy covers extensions and native/JIT adapters, and
the proposal requires object-pointer callers to use
`PyValue_FromObjectBorrow`. Thus skipping publication is conditional on a
real supported-caller contract, not an assumption that
`PyValue_IsLanguageValue` implies canonical bits. A caller that still needs
the broader input contract must use a normalizing boundary instead; direct
shape-result publication cannot be admitted in that case.

The in-tree writer chain supports the declared premise. The generic object
and value-slot adapters canonicalize object inputs; the internal store
duplicates its input into the shaped inline slot; specialized shape stores
transfer canonical frame-home values; initialization writes NULL and
copy/move preserves bits. Materialization clears the shape, so the shape
load cannot use an invalidated inline layout. `_LOAD_ATTR_SHAPE` reads the
slot under the preceding shape guard, and `PyValue_XDUP` preserves the
borrowed bits and gives the result its required owner. This is a manual
source proof for this site and the current supported writers. The generator
need not pretend to infer C pointer provenance from the cast in the read.
The proposed debug assertion on `found` checks the asserted premise at the
actual producer as a backstop.

R4's shared result-publication checker and
`IR_RETIRE_PUBLISHED_RESULT` are a hard prerequisite; they are not in the
current runtime. Once they land, the one trusted
`IR_CANONICAL_HOME_DUP(attr, found)` declaration can attach the canonical
source fact to R4's existing result record. The generator should validate
the declared output identity, unconditional placement, matching retirement
marker and no later mutation, address-taking, escape or ambiguous join. It
does not need a new C-provenance parser, opcode list, global value type or
second metadata table. Direct assignment applies only to the first
temporary-destination flush. The folded-local final store and every
unmarked result retain `_PyValue_Publish`; owner retirement, error cleanup,
refresh and companion output keep their current order.

Implementation review must check the actual supported native adapters and
the final R4-generated form before enabling the direct store. It must also
check both debug assertions, temporary/local and error paths, and the GCC
machine code, then use matched workloads to decide whether the extra DSL
marker merits keeping. These are admission checks for the accepted bounded
experiment, not an elapsed-time claim from this design review.
