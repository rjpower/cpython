# M2 semantic transfer and shadow-map increment

Status: proposed after M1b source review; no schema or generated changes yet.
This is a dependency plan for executable increments, not an ownership proof.
Selected54 and M1b keep initialized/NULL homes as execution authority.

The first collector consumer should be the agreed unlinked-frame retirement
continuation plus its independent result owner. The GC lane confirms that its
NULL/full-range remaining-owner cursor can later consume an exact code-owned
release list without changing the nested-root or fixed-last-code contract.
Do not delay that real collector bridge while trying to describe every active
instruction phase. Conversely, an incomplete active-map audit cannot admit M4.

## First IR increment: generated semantic edges used by admission

Extend the existing generator `HandlerContract` and `external_io` analysis,
`_PyIR_Outcome` decoder and encoded constructor worklist. Do not infer owners
from `_PyRegCfg_Liveness`: that is future-read dataflow, and dead values may
still require a release. Keep `_PyIR_NormalPhysicalTransfer` separate; UNKNOWN
does not establish a logical definition or an owner.

The first concrete output is an explicit normal-edge transfer table consumed
by common encoded-image admission. Its source is the same transfer analysis
that emits handler stores, not a second manually maintained opcode list.
Each input role states binding borrow versus scratch ownership, preservation,
duplication, move or release. Each output states fresh value, canonical NULL,
transported input or duplicated input, physical destination and nullable state.
Edges state fallthrough/taken/activation completion, clobber window and whether
callbacks/exception exits require a staged contract. Existing generated
`output_kind`, `borrow_mask`, `_PyIR_HandlerBase`, `observer_outcome`,
`reads/writes/through` and effects remain sources, not parallel facts.

Start with scalar load/store/retirement and COPY/SWAP transport, then the
existing explicit FOR/SEND/completion outcomes and calls. Constructor worklist
tests must detect a use after move, duplicate source requiring two counts,
misplaced output and join missing one predecessor definition. Use the same
encoded shapes through compiler, constructor/replace and marshal. A table row
without a proved transfer is explicitly uncovered during shadow development;
it grants no semantic-map authority and does not relax existing admission.
That coverage marker is removed as a representation choice before M4, where
every admitted executable family and fixed sentinel must have a complete proof.

The encoded worklist should reuse `owner_handler_safe` scheduling and
`_PyIR_ReadsHome`/`_PyIR_NullableHome`/binding-read rules. Separate lattices track
logical availability, allowed nullable values and owner obligations. Joins
intersect definite availability, while remaining releases are merged as owner
obligations; do not combine them into one live bitset. Window pass-through
holes are not semantic reads. Named locals remain independently initialized
and observable, hidden constant homes remain readonly, and borrowed mortal
scratch is not introduced.

There is a concrete encoding dependency: PUSH_NULL is STACK_ONLY, lacks
`_PyIR_VALID`, and `_PyRegPass_ElideNull` removes every instance. Therefore
disabling its elision alone does not produce executable definitions. During
M2 the separate physical must-NULL proof can explain nullable implicit inputs
under the retained NULL invariant. Before M4, introduce a reviewed explicit
canonical-NULL producer or a generated family form that supplies canonical
NULL without reading an inactive home. That change updates generator,
assembly, construction/marshal, disassembly and format/version tests together.
It must not import hidden constants or optional pool operands into M2.

Files: `Tools/cases_generator/{ir_generator,ir_metadata_generator}.py`,
`Include/internal/pycore_ir{,_outcomes}.h`, generated metadata (root),
`Objects/codeobject.c`, focused generator and IR-admission tests. Keep regcfg
allocation unchanged initially; extend its existing verifier to compare the
virtual transfer proof with the emitted physical edge contract after allocation.

## Retirement bridge and first actual map consumer

In parallel, frames/GC implement the reviewed reached slow continuation,
nested root chain, initialized prefix, header partition and independent result.
It starts with the current NULL range and remains valid when no semantic map
is available. This is M2/M3 development authority, never a permanent M4 fallback.
The code pin is fixed-last and the physical reservation survives every nested
callback. Heap edges moved to external records disappear from the heap's
subtraction authority before any reentrant close.

The first map allocation lands with a retirement consumer and its shadow
comparison. A code-owned immutable plan interns sorted half-open uint16 ranges;
canonical SiteId is uint32 word index, PhaseId is a uint8 family literal.
PRE=0, POST=1, ERROR=2, RETIRING=3, SUSPENDED=4, staged roles start at 5.
No per-inline-operation map lookup, per-call image count, or large hot automatic
record is added. At a reached retirement phase, the continuation decodes the
plan once, compares its authorized owners against the still-NULL control and
uses the same cursor order for nested-root enumeration. Shadow comparisons
check logical values and unique owning edges, including NULL/inline/immortal
values; they do not equate nonNULL with mortal ownership.

Ordinary return, error unwind and heap clear require distinct certified
retirement entry states. A RETURN opcode alone does not prove normal return:
monitoring or a helper may fail at that site. The frames owner must publish the
explicit phase/reason after result transfer and before unlink/first callback.
Until that handshake and the corresponding descriptor are certified, the
continuation uses the M2 NULL control and records missing coverage; it must not
guess a map from frontend stack depth or a function's current `__code__`.

## Complete phase generation before M4

Use `IREmitter`/`IRStack`'s existing consume, `OWNER`/`TAKE`, publication and
escape/error checks to capture transfer events at the same points as emitted
stores. The generator already knows pending C owners, taken outputs, borrowed
views, committed result publication and escaping statements. Each escaping
statement must have a PRE/STAGED/POST owner partition or be rejected. Generated
family phase IDs describe finite shapes; branches merge compatible states or
emit finite alternative roles. Reject/split a family exceeding 256 phases.
The metadata decoder and generator must be tested against each other.

Dynamic unpack uses one validated window/source/capacity recipe with an
initialized-prefix cursor, not one phase per item. CALL commit excludes the
transferred window from the caller and proves survivors below the entire
child clobber extent. LOAD_ATTR method output always defines callable and
self-or-NULL. FOR/SEND distinguish body/yield, exhaustion and error. Observer
callbacks use their checked committed facts, and exceptional effects remain
explicit. Specialization/restore/instrumentation may change low16 only within
the same certified family contract.

Root freezes shared descriptor/frame/transition headers with both lanes before
the first runtime map allocation. Root owns generation, release/debug tests,
native publication/cursor checks and integration. Required tests include nested
GC/arena growth, last-owner callbacks, mutable locals/result destinations,
partial unpack, generator creation-prefix/resume/throw/close and escaped frames.
No authority switch or removal of scanner/NULL stores follows from M2 alone.

## Bounded first patch for peer review

GC peer accepts the following deliberately limited first executable patch.
It is a normal-edge hardening worklist, not the complete definite-assignment
gate described above. It allocates temporary admission facts and consumes the
generated transfer facts immediately; it adds no persistent unused descriptor.

Add one uint8 transfer-kind column to generated `_PyIRFormat`/`HandlerContract`.
Kinds cover fresh output, input retirement, binding duplication, binding move,
binding store/clear, COPY duplicate, SWAP transport and scope exit; zero is
explicitly unproved. The generator checks each admitted row against its source
IO and body-owner operations. The concrete starting families are LOAD_CONST
and inline twins, LOAD_COMMON_CONSTANT/LOAD_SMALL_INT, LOAD_FAST/LOAD_FAST_BORROW/
LOAD_FAST_CHECK, LOAD_FAST_AND_CLEAR, STORE_FAST/DELETE_FAST, POP_TOP, COPY/SWAP
and exits. Borrow in LOAD_FAST_BORROW's historical name does not create borrowed
mortal scratch: its materialized result duplicates the binding owner.

Expose a cold semantic-home transfer query beside existing Outcomes APIs. It
returns preserve, unavailable-after-release, fresh value, canonical NULL,
duplicate-of-home, move-of-home or unknown. Source and destination are physical
home indices with separately identified local bindings. It reuses existing
field/read/output/outcome decoding; `_PyIR_NormalPhysicalTransfer` retains its
different physical-NULL meaning. No inline handler calls the new decoder.

The temporary worklist tracks availability and authorized-owner provenance
separately. At an unsupported row it loses proof for affected homes; when the
footprint is itself unproved it loses all scratch proof. Unknown neither
initializes a home nor authorizes an owner. Definite negative facts survive
only across certified untouched homes. Under this bounded hardening policy,
a join with an unknown predecessor remains unknown; it must not reject an
image by treating an unsupported handler as if it had preserved an old release.
The complete later admission gate instead requires all predecessor transfers
and rejects a missing definition on any executable edge.

First tests cover known load/consume/read rejection; direct entry unavailable
versus permitted nullable implicit input; repeated COPY producing independent
owners and SWAP moving provenance; local move yielding initialized NULL when
unbound; all-covered branch/loop joins; unknown touched/untouched barriers;
and a branch with an unknown predecessor. Test through constructor, replacement
and patched marshal without executing bad code, with explicit reached-family
assertions. Compiler/generator tests perturb source owner operations and require
generation to reject a falsely claimed transfer kind. Root runs generated
consistency plus existing IR, constructor and completion packages.

The exact choice between emitting a canonical NULL producer and supplying
NULL in an admitted ordinary-call family is deferred to the producer/site
census. Prefer evaluating the existing ordinary-call forms before paying a new
NULL store at every call. Both approaches must establish the same explicit
logical NULL fact before stale scratch is permitted.
