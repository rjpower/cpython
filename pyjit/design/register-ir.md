# Register IR: current contract

This page defines the executable and ownership contract in selected runtime
`bf810ec1a33`. [Status](../STATUS.md) owns qualification and performance
admission; the [current report](../issues/deltablue-call-model-20261110/report50.md)
owns measurements. Historical results and rejected experiments are not
transferable to this build. The bounded observer and debugger rules below do
not authorize every source transfer.

The [archive](archive/register-ir-20261108.md) retains earlier designs.
[Call/frame lifetimes](call-and-frame-lifetimes.md) and the
[observer contract](handler-and-observer-contract.md) define ownership and
observation. Release points implement the selected RC runtime; they are not a
Python guarantee of immediate finalization. See
[language and lifetime policy](lifetimes-and-language-semantics.md).

## Producer path

```mermaid
flowchart TD
    source[Python source] --> ast[AST and code generation]
    ast --> stackcfg[Optimized stack CFG]
    stackcfg --> prep[Pre-register preparation in flowgraph.c<br/>pseudo-op lowering, stack depth, jump normalization]
    prep --> regcfg[Register CFG conversion]
    regcfg --> passes[Register passes<br/>rename, copy propagation,
store and truth simplification]
    passes --> compact[Temporary liveness and slot compaction]
    compact --> assemble[Word and metadata assembly]
    assemble --> code[Code object<br/>immutable words + feedback init]
    code --> first[Lazy image creation<br/>ir.c: make_exec]
    first --> live[Per-code executing copy<br/>words + feedback vector]
```

`Python/compile.c` sequences CFG optimization, `_PyCfg_OptimizedCfgToRegCfg`
and code-object assembly. The register CFG and passes are in `Python/regcfg.c`
and `Python/regcfg_passes.c`; word emission is in `Python/assemble.c`.
Line and exception tables use the same word-index coordinate system. There is
no runtime stack-bytecode lowering stage.

## Word format and operands

`Include/internal/pycore_ir.h` defines `_PyIRWord`, an eight-byte word:

```
 63      52 51      40 39      28 27      16 15       0
+----------+----------+----------+----------+----------+
|    D     |    C     |    B     |    A     |  handler |
+----------+----------+----------+----------+----------+
```

The low 16-bit handler selects the operation. A, B, C and D are 12-bit fields;
CD is the 24-bit value `C | (D << 12)`. There are no prefix words. Handler
metadata in `pycore_ir_metadata.h` and generated dispatch in
`Python/generated_ir_cases.c.h` define each handler's field roles and effects.

- Slots share `localsplus` numbering: locals, cells, hidden scalar constant
  homes, free variables, then compiler temporaries. A slot field addresses 0–4094; `0xFFF` is
  also `IR_NO_DST`, the encoding for an unused/discarded destination.
- Register-form operations name a destination and their inputs. Inputs are
  ordered bottom-first; stores name the target/object before the stored value.
- Window-form operations name a base slot and access consecutive inputs by
  offset. Call preparation and unpack operations use this form.
- An immediate may be a count or local index in one field, or a constant/name
  index in CD. A feedback index points into the executing copy's feedback
vector. Some operations store an operation argument in feedback metadata.
- Ordinary jumps carry an absolute word index in CD. `FOR_ITER`, `SEND`, and
  fused compare-and-jump handlers carry a signed word distance in D from the next
  instruction. `FOR_ITER` and `SEND` use forward distances; fused
  compare-and-jumps are forward-only. The assembler relaxes long loop exits and
  leaves out-of-range fused branches unfused.

The generator's format metadata describes reads, writes, destination,
feedback/immediate location, branching, and declared effects. These facts drive
verification, register allocation/compaction, instrumentation and handler
generation; they are not a native-JIT recovery map.

## Constants, temporaries and jumps

Only eligible inline scalar loop constants receive hidden homes in the frame's
`localsplus` array. Other constants use ordinary constant-loading forms. Hidden
constant homes and local/cell/freevar slots are fixed before temporary slot
compaction. The compiler assigns temporary registers from the remaining slot
range and rewrites them densely after liveness analysis.

All physical homes are initialized to a valid `PyValue` or NULL. Compaction
tracks occupied owners through their consumption, not only future reads; a
live owner may need cleanup despite having no later program read. The
verifier checks slot ranges, window extents, destination legality, and jump
boundaries. A jump target is a word index and must land on an instruction.
Exception ranges and source lines are likewise expressed in word offsets.

## Compact execution and source positions

`Python/ir.c:make_exec` allocates a shared `_PyCodeExec` for a code object:
mutable executing words and feedback storage. Specialization and monitoring
patch its handlers; pristine code words remain the template. A frame owns its
code, keeping both code and executing allocation alive. Feedback contains
borrowed predictions and counters, not owning GC edges. Entry zero is the
shared header-only entry. Free-threaded specialization remains disabled.

The serialized `co_fbinit` records include feedback sizes and offsets. Changes
to their geometry require a new bytecode magic, even when handler numbers and
the eight-byte word encoding stay unchanged. Current magic **64417** identifies
48-byte attribute entries with independent guarded-value storage. At the
4096-unit limit, optional sites without private capacity use shared entry zero;
the compiler must preserve required feedback and valid generic execution.

Compiler source-only operations occupy no executable word. Pristine streams
admit semantic operations and real control transfers; source-only line events live in the versioned
`co_linetable` container alongside ordinary word-position data. `f_lasti`,
disassembly offsets, jump targets and `co_positions()` describe compact real
words. `co_lines()` also retains source-only zero-width entries. There is no
second public instruction coordinate domain and no INSTRUCTION event for an
eliminated operation.

Eligible unique fresh scalar results use `IR_NO_DST`. Calls use the same plain
recipient, including across source lines and generator-related control flow;
there is no tagged postcall recipient or live-POP decoder. A producer may still
use a real internal result window before its final discard; that slot traffic
is distinct from dispatching a release operation. Shared-owner/exception/branch
cleanup words remain a separate unfinished inventory. The overall objective
of eliminating standalone release-only execution is not yet complete.

A single shape-reader ownership variant, `LOAD_ATTR_SHAPE_LOCAL_TEMP`, remains:
its local input and temporary destination are static. Generated canonical,
family, statistics and feedback mappings cover the physical handler domain.
Adaptive disassembly names the specialization; internal executing-IR inspection
can expose an ownership variant. This does not create a product of every mode.

Round 50 adds `IR_CANONICAL_OUTPUTS` for a checked terminal producer of
multiple owning canonical outputs. Its bounded proof accepts guarded-home
duplication and unchanged canonical-input transfer, and rejects unproved
transformations or escaping prefixes. Output provenance is independent of the
existing receiver-retirement proof; ownership transfers still occur normally.
The adjacent method/CALL experiment was rejected on performance; it is not
part of the selected candidate. See the
[round report](../issues/deltablue-call-model-20261110/report50.md).

## Guarded Python calls

CALL_PY_GENERAL and CALL_KW_PY can invoke `call_bind_known_python` through
`IR_INVOKE_GUARDED_PYTHON_CALL`. The generator requires the exact nonescaping
PEP 523, function-version and recursion-check prefix, the typed `_PyFbCall`
feedback entry, and the corresponding consuming window shape. The keyword
shape includes its trailing kwnames owner. Altered guards, extra callbacks or
changed consumption invalidate the producer proof. CALL_KW_BOUND_METHOD and
instrumented calls retain their ordinary transaction invocation.

The keyword plan caches one to three parameter destinations within the existing
feedback payload, independently of function arity. It supports positional
prefixes, permutation, positional defaults and explicitly supplied keyword-only
arguments. The hit validates current names against the guarded code and checks
current-default required-slot coverage before any owner moves. At most three
keyword owners move through fixed C-local storage; the positional prefix stays
in the overlapping frame window. Complete child homes and the existing return
recipient/continuation are published through the same frame ABI. The
[call/frame contract](call-and-frame-lifetimes.md#known-python-binding-and-generic-providers)
defines the detailed guards, exclusions and ownership order.

A helper miss occurs before effects with the original window intact. Generated
completion selects the existing transaction for that call kind: vector for a
positional call and keyword for CALL_KW_PY. No generic fallback starts after
owner transfer, and no preparation or callback is replayed. Both hit and
fallback use the same child-dispatch and error completion protocol. These
checked producer facts authorize a bounded interpreter binding optimization;
they do not define a native-JIT recovery map.

### Classmethod pairs

`LOAD_ATTR_CLASSMETHOD` is an executing specialization of ordinary `LOAD_ATTR`,
with A as the first output TEMP, B the class receiver, C attribute feedback and
D the original name/method operand. It admits method-form access only and writes
the existing two-home function/receiver window. Its canonical base is LOAD_ATTR;
the generic family retains its conditional one/two-output extent.

Guarded feedback holds the descriptor. The live wrapped function is acquired
before transferring the receiver; all misses precede either ownership change.
Subsequent argument evaluation and ordinary CALL remain distinct boundaries.
Handler111 intentionally reuses a retired number under format magic64418;
serialized compiler output remains generic. See the [interpreter contract](interpreter.md#classmethod-capture).

## Semantic completion and generated contracts

FOR_ITER defines its item recipient only on its body edge; exhaustion retires
its iterator/cursor and preserves the last item binding. SEND yields into its
receiver window but completes into its explicit result recipient. CLEANUP_THROW
has its own named real target and shares SEND completion ownership. YIELD_VALUE
names its incoming resume recipient separately from its outgoing value.
Semantic edge definitions override the frontend's stack-shape union.

Source-edge assembly follows the same controlling operation as word encoding.
A fused comparison uses its paired conditional jump's target and fallthrough,
while source records retain the emitted comparison's actual executable offset.

Long FOR_ITER/SEND branches use actual IR_JUMP adapter words. The observer
recognizes only their exact assembler shape for LINE/branch grouping; helper
words retain their own INSTRUCTION events. The static initialization shim also
contains only its real operations, with an explicit saved continuation.

`_PyIRFormat` describes operand roles, stack-shape extents, fold capability,
effects, feedback and monitoring. Per-handler `output_kind` separately describes
actual publication: register dst, window, first window result moved to dst, or
semantic outcome. A conditional trailing window output stays in its actual
window even when the first result moves. `_PyIR_STORE_DST` alone says a local
fold is allowed, not where every output is physically written.

Cold consumers share `_PyIR_Outcome`, `_PyIR_ReadsHome`, `_PyIR_DefinesHome` and
`_PyIR_NullableInputs` from `pycore_ir_outcomes.h`. They distinguish explicit
source fields from untouched window holes, preserve through owners and SWAP
provenance, and admit only audited nullable roles. A generated per-edge
`observer_outcome` declares which terminal observer sees committed output;
unmarked handlers receive no POST claim. `_PyIR_ObservedHomes` decodes the
physical result and retired input homes, keeping a new value or explicit NULL
separate from an old input's unavailable owner. The constructor checks a
declared source callback's POST exception recipe and both compute PRE and
observer POST for ordinary exception coverage. FOR_ITER's temporary A must be
its actual B+2 item home; a local A moves that item, and IR_NO_DST discards it.
These checks are not a general bytecode type verifier or native recovery map.

The positive terminal marker supplies reported source, physical continuation,
monitoring event and an explicit owner edge. Its literal edge and destination
shape drive the same metadata and runtime call. The event does not select the
owner edge: POP_ITER reports BRANCH_RIGHT while its own outcome falls through;
an unconditional JUMP takes its target. The actual executing word remains the
owner/EH site even when a callback reports another source word. Zero-output
transfers have valid committed facts without creating a result owner.

Scoped source callbacks retain the immediate real continuation separately from
the normalized landing in their source record. Only a completed provider grants
SOURCE_POST scope; unproved inline hooks still deliver source events under an
unsupported scope. The setter uses the same Plan for real PRE, certified
owner-neutral committed JUMP and bounded non-neutral committed outcomes. PRE
and neutral JUMP borrow exact site rows. A non-neutral view copies that site's
context, saved bindings and origins, applies the selected normal transfer, then
cuts old input provenance independently of replacement values. Token-creating
transfers are excluded from this copied-row path.

On source-observer failure, the observer passes the resolved semantic recovery
recipe into activation-local pending state; the unwinder consumes it for the
matching frame without decoding a source map. The frame display ordinal remains
separate. Compiler line predicates and the serialized container that combines
observer recovery with source positions remain future work.

The new non-neutral permission requires exact scope/site/edge/continuation and
redirect generation, inherited handled context, empty stopped and target
saved-binding state and a real target with no incoming scratch-owner requirement.
It cannot use a
pre-site survivor recipe or a source-only fallback. Existing PRE restoration
and neutral JUMP rules retain their own proofs. General SOURCE_POST and yield
transfers require separate stopped-state authority.

## Experimental leaf-call metadata (not selected)

The rejected leaf candidate added an optional `_PyCodeExec` certificate for a
complete conditional body. It is derived from immutable final words and is neither
serialized bytecode metadata nor a rooted runtime value. The bounded form has
at most 32 words, four complete value paths, two guarded attribute steps per
path, one inline comparison and two return arms. Paths start from arguments,
current constants (including common-constant roots), small integers or guarded
globals. Every executable word must be accounted for and feed the predicate or selected result; unsupported
effects, reads, joins, loops and handlers reject certification.

Runtime execution is attached to plain `CALL_PY_EXACT_ARGS` after its checked
prefix. A zero-shape terminal marker is accepted only by positive generator
validation of the exact guard sequence and untouched call window. The
no-certificate gate skips the helper. Admitted reads share the ordinary guarded
global, shape and class probes, and numeric computation shares the ordinary
inline comparison semantics. A MISS preserves all original homes and continues
through the existing initializer. A hit requires callback-free, GIL-held
execution and an identity-counted nonfinal-release proof before detaching
inputs; it returns the actual continuation without creating a frame. The
[experimental interpreter design](interpreter.md#experimental-conditional-leaf-call-design-not-selected),
[rejection report](../issues/deltablue-call-model-20261110/integration49.md) and
[next proposal](../issues/deltablue-call-model-20261110/C2-proposal.md) record
its restrictions and status. This mechanism is not selected or enabled by the
current runtime design; the next proposal has peer approval as a future direction and remains unimplemented.

## Explicit boundary

The baseline IR is an executable register instruction stream, not optimizing
SSA. It has no machine-register allocation, deoptimization state, OSR protocol,
or general native-JIT recovery map. The current method/tracing JIT recovery
contract remains future work. The [checkpoint report](../bench/results/interpreter-parity-20261108.md)
and [status](../STATUS.md) record qualification and open work.
