# Common constants: retain the producer's facts through publication

Proposal only, based on selected57 `0dcba75bb3df`. Keep its NCSR handlers,
compiler, flags, frame/header layout and semantic words. Do not use the rejected
resident engine, scalar descriptors or earlier provider rewrites as a foundation.

## Source facts and actual remaining work

`Python/pylifecycle.c:913–935` fills all `NUM_COMMON_CONSTANTS` entries. Each
non-NULL object is made immortal before
`PyValue_DUP(PyValue_FromObjectBorrow(common_objs[i]))` is stored. The canonical
constructor yields either an inline value or a no-count immortal pointer.
The only other table assignment is teardown's NULL initialization in
`Python/pystate.c:713`; execution must not read a partially initialized or
cleared interpreter. The table is per interpreter, not a process-global pool.

This covers **every entry**, including minus one (canonical inline integer),
True/False (immortal bool pointers), None, empty objects, exception classes,
type objects and the captured all/any callables. It does not claim that every
entry is a pointer or that arbitrary `LOAD_CONST` values are immortal.
`PyValue_DupImmortal` is unsuitable unchanged: its final debug assertion treats
the value as an object pointer, which is invalid for inline minus one.

The ordinary producer at `Python/bytecodes.c:1862` still executes a DUP of the
table word. The selected native handler at `0x35d080` tests whether that word
owns a count; its publication then classifies A and checks for exact numeric
objects through `_PyValue_Publish`. The initialization facts already exclude
both a required increment and a needed normalization. Coroutine census weights
971,142 visits to this word, with the monitoring limitations described in
[resident-coverage.md](resident-coverage.md). Counts establish a consumer, not
a speedup estimate.

## One checked producer; one destination refinement

**Packet 1 is producer receipt transport in the ordinary handler.** Make the
table's initialization/read contract explicit with a small static-inline
internal getter, provisionally `_PyInterpreter_CommonConstant(interp, index)`,
returning a non-NULL canonical, no-count PyValue. It names that interpreter's
fixed table; it must not accept an arbitrary PyValue array or return a
persistent borrow descriptor. It reads one table word and gives the current
result a logical owner without taking a heap count. Release code is a bits
copy, not an out-of-line getter or guard. No new DSL framework is required.

Factor the initialization contract beside its current writer: immortalize the
object, use the canonical constructor, then store the resulting no-count value.
Debug assertions check non-NULL, canonical form and absence of an owned count;
they must handle inline values before any pointer projection. Do not infer a
constant's bits from its numeric index or substitute a special literal for -1.
Retain teardown behavior and the interpreter lifetime requirement.

Register its one audited contract in the shared typed helper/effect table and
use the existing complete direct-RHS assignment/current-value receipt. Its
**reached result receipt** establishes the current Local's canonical
representation and ownership using existing result/token facts. Canonical,
non-NULL, no-count and logical owner are separate properties; do not infer
one from another. The source walk knows that this getter neither writes arena
homes nor calls Python. An instruction name, arbitrary helper call or a second
hand-maintained name allowlist must not grant those facts. Renaming the
instruction while retaining the checked getter should work; replacing it with
an opaque read must lose the receipt. Reassignment, address escape, shadowing,
unknown calls and joins use existing invalidation rules. Do not add a general
no-count alias-propagation lattice or ordinary-native-function replacer.

Ordinary `LOAD_COMMON_CONSTANT` uses this producer too. Its existing final
writer can then select `IR_STORE_CANONICAL_DST`; local replacement and deferred
retirement of the **old local** remain unchanged. A discarded result needs no
new owner cleanup, but retain the ordinary sink protocol. Do not remove old
local finalizers merely because the new value has no count.

**Packet 2 is a separately reviewable destination refinement.** Do not delay
the ordinary producer receipt if its admission closure is larger. Add exactly
one output-mode refinement, provisionally
`LOAD_COMMON_CONSTANT_TEMP`, for an admitted scratch A. Reuse the existing
`ExecutionVariant.temp_fields`/IRStack final-store mechanism rather than a
second hand-written constant handler. Its current canonical-result receipt
selects the checked canonical temporary publication: no destination role
branch, numeric normalization or DUP/count test. Keep ordinary scratch vacancy
admission and post-publication checks; no fresh-write/NO_HOME relaxation belongs
to this packet. Local and `IR_NO_DST` recipients use the generic producer and
ordinary final writer. No per-type, per-index or local/discard variant family.

## Selection, images and observation

Prefer the refinement only in the executing copy, after common code-image
admission has established constant-index bounds and A's scratch geometry.
Immutable words continue to name ordinary `LOAD_COMMON_CONSTANT`; compiled,
replace, constructor and marshal paths therefore share the same authority.
The cold `make_exec` walk can select the TEMP refinement using the already
admitted word and `co_nlocalsplus`/extent. It must run before the no-feedback
early return. This is compile/admission-time mode selection, with no hot plan,
feedback lookup, generation test or dynamic destination branch.

The compiler/assembler already knows the result's assigned slot when choosing
handlers (`assemble.c:880,1350`), but compiler-only trust would leave constructed
images inconsistent. Keep that path's immutable handler ordinary; an assembler
nomination is unnecessary in the first packet if the common executing-copy
selection is used. Do not add redundant flags or value IDs to regcfg.

Mark the new refinement execution-only in the generated admission policy so a
public image cannot forge it. Its metadata/shadow/base mapping must retain the
ordinary input, result, source and observation identities. Existing bounds and
semantic-vacancy checks still run on the original word. The generator must
refuse the refinement if the producer's actual canonical-owner receipt is
missing; silently selecting the direct store would be unsound.

Instrumentation must identify the ordinary immutable site, not persist a
private TEMP handler as its original semantic opcode. LINE/INSTRUCTION wrappers
may execute or restore the generic form; it has the same checked producer and
correct local/discard/scratch behavior. De-instrumentation may remain generic
until executing-copy preparation is repeated. No requirement to preserve the
optimization through observation justifies new mutable side tables. Enumerate
the real base/shadow/deopt/name consumers before freezing; do not assume an
existing opcode comment automatically supplies the mapping.

## Bounded source closure and gates

Expected source consumers are the lifecycle writer and interpreter table
contract; bytecodes plus the shared typed-effect/result receipt; IRStack's
existing canonical store; the single execution variant and generated mapping;
`Python/ir.c`'s cold executing-copy selection; and image/instrumentation
validation. Generator products remain root-owned. Add no frame/exec field,
allocation, public API, general result descriptor or new backend policy.

Required controls:

- All constant indices, especially -1/bools/None; exact value/type/identity
  behavior across all four public image routes and repeated interpreter use.
- TEMP execution choice for legal scratch A; generic local/discard choice;
  forged private handler, invalid index, local/sink used as TEMP, live/unknown
  scratch recipient and malformed extent rejection.
- Current-result source receipts in both record modes; renamed instruction
  positive, missing/changed producer and overwrite/escape negatives. Ordinary
  normal/error maps gain only what their existing consumers can actually prove.
- Mortal old local whose finalizer mutates locals, runs GC or raises through
  monitoring; new constant is visible before old-owner retirement. Preserve
  source/continuation behavior on the callback route.
- LINE/INSTRUCTION/trace enable-disable, exception/recovery mappings and
  subinterpreter initialization/teardown. A test must exercise a code object
  with no feedback so the early-return path cannot hide selection failure.

Before timing, root should inspect the actual scratch handler: a table load,
direct canonical store and ordinary dispatch, with **no** DUP/refcount test,
normalizer call/type checks, `nlocals`/sink branch or new getter call. Charge
operand decode, PC publication, remaining stores, any new spills and code size;
keep the ordinary local old-owner schedule. Compare generated formats and all
other handlers before claiming narrow scope. Run the matched eight-case screen;
broader qualification follows only if the mechanism and sentinel results are
credible. A negative screen closes this implementation without alignment or
compiler-flag search.

This is a small dialect contract for a published no-count constant producer,
not a parity forecast or a reopening of the closed zero-COPY borrow experiment.
