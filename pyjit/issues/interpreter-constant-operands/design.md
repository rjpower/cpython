# Constant operands through the existing operation model

Proposal against `d98291018c9665ae3db6952da0b0a40a4e04992b`, 2026-10-31.
No runtime implementation or native experiment belongs to this document.
The [results](results.md) distinguish actual instructions from proposed savings.

## Selected consumer

Select a bounded **small-integer RHS operand form** for ordinary and in-place
add, subtract and multiply. Start only with existing `LOAD_SMALL_INT` producers
that already encode exact nonnegative integers independently of `co_consts`.
`flowgraph.c:1444` admits exact integer constants 0–255; bools and subclasses
are excluded there. Do not propagate facts from locals, calls, shapes or
loop-carried values, and do not start with arbitrary float or constant-pool
operands.

The new form would keep the current 64-bit word and feedback size:

| Field | Current slot/slot form | Proposed slot/small-integer form |
| --- | --- | --- |
| A | destination slot | destination slot |
| B | left input slot | left input slot |
| C | right input slot | unsigned 12-bit integer operand |
| D | feedback index | feedback index |

The compiler initially emits the small form only for 0–255. Its encoded unsigned
12-bit value is always representable as exact inline int32. Materialize
`PyValue_FromInt32((int32_t)IR_C(word))` inside the generated handler, then run
the existing generic/specialized DSL bodies. The C compiler can eliminate the
RHS compact-int guard and representation branch by construction. The dynamic
left guard, overflow result allocation and error path remain. No unchecked
frame-slot fact crosses an observation point.

This is an input-source distinction, not a new type lattice. Ordinary values,
typed arithmetic and generic fallback remain in the existing DSL. Do not hand
write ADD_ONE, duplicate numeric algorithms, create a second effect table,
encode facts in feedback padding, or maintain a parallel fallback switch.

## Minimum coherent family

Do not stop at one increment intrinsic. Generate only the admitted combinations
for ADD, SUBTRACT and MULTIPLY, including their six ordinary/in-place generic
operations, their three existing integer specializations and three mixed-float
specializations. Multiplication must also retain the existing EXTEND route for
string/bytes/tuple repetition. Those thirteen concrete bodies are the initial
expected set, subject to generator inventory review; the operation bodies must
remain shared. Unrelated division, subscription, shifts and left-constant forms
remain unchanged.

The generic small form reconstructs the exact same RHS `PyValue` and calls the
same value-number operation with the original NB_ argument. Thus float-left,
bool-left, large integers, subclasses, reflected operations, exceptions and
in-place behavior retain their existing path. Preserve float and sequence
specializations rather than accepting an unmeasured regression for those
callers. A guard miss must preserve the small form; dispatching to ordinary
`BINARY_OP_ADD` would interpret the immediate as a frame-slot index.

## Shared authority changes required

The current generator deliberately gives every family member one format.
`HandlerDomain.operations` also assumes each operation number appears once.
The proposal therefore requires a reviewed extension of that single authority,
not an assembler peephole that bypasses it:

1. Describe operand sources alongside the semantic operation in the existing
   generator/domain. Derive physical reads, logical materialization, format,
   handler membership and effects from that description and the original DSL.
   Distinguish an immediate operand role from the operation's existing oparg;
   `_PyIRFormat.imm` currently names only one oparg.
2. Use the same generated relation to select a generic handler from
   `(NB_operation, operand_form)` on deoptimization, replacing the current
   arithmetic `_PyIR_BINARY_OP_FIRST + arg` shortcut. Specialization installation
   projects its chosen existing handler into the pristine word's form. This
   work happens at installation, not as another hot-path type test.
3. Derive `_PyCode_InstrOparg`, original-handler restoration, instrumentation,
   disassembly, static-domain auditing and feedback-entry-zero behavior from
   those relations. Keep semantic base opcode BINARY_OP and its NB_ argument.
   The transparent LINE/INSTRUCTION wrappers must restore a compatible form.
4. Model the materialized operand in register CFG conversion, verification and
   compaction before assembly. Do not reserve or clear a frame slot by treating
   C as a slot. Keep one source of truth for semantic inputs versus physical
   inputs; extend the existing folded-input mechanism used by frame stack
   analysis rather than adding a debugger-specific type map.
5. Regenerate executable cases, formats, handler/domain tables, dispatch and
   Python metadata together. Update all affected readers and the pyc magic
   under `pycore_magic_number.h`: serialized word meanings/handlers change.

Likely owned source spans `regcfg.c`, `regcfg_passes.c`, `pycore_regcfg.h`,
`assemble.c`, `ir_generator.py`, its metadata/domain consumers, `pycore_ir.h`,
`ir.c`, `ceval_macros.h`, specialization installation, `Lib/dis.py` and generated
outputs. Frame stack analysis may need the shared materialized-input reader;
it must not guess from the opcode name. Confirm this inventory in the prototype
before authorizing a native build. No GC runtime or frame-transfer ABI edit is
part of this cut.

## Dataflow, aliases and observations

Fold only a single-use fresh temporary produced by `LOAD_SMALL_INT`, popped as
the RHS of one eligible arithmetic instruction, with no implicit exception use.
Initially require adjacent live ops in the same reachable region, same source
line, no intervening line/jump/handler entry, and no window or callback. Reject
canonical registers, shared uses and mismatched exception regions. Make the
new operand form explicit in the CFG before physical slot reuse. Store folding
may still direct the arithmetic result into a local, including the left local;
all inputs must be read before that write, and generic calls must publish/root
their inputs under the existing generated protocol.

The immediate is a value in the current word, so legal debugger entry cannot
replace it with unrelated stack contents. Recompute actual physical/semantic
stack metadata for the new word: `Objects/frameobject.c:1489` already accounts
for local loads materialized inside a consumer. Preserve its compatible-stack
and clearing behavior for an additional materialized constant. Do not leave a
phantom RHS slot or let the unwinder read an uninitialized one.

Removing the producer changes the compiled word sequence. Monitoring must fire
once per actual emitted word, as current copy propagation does; it need not
invent an INSTRUCTION event for a removed word. Preserve line events, trace
rebinding of the dynamic local, arithmetic source spans/exception location and
legal `f_lineno` transfers. An overflow may allocate, and generic arithmetic
may call Python: derive roots and RUNS_CODE from the unchanged DSL family,
not from the fact that the RHS itself is pure and inline.

## Alternatives and rejection

- **Unchecked frame-slot promise:** reject as the initial design. Same-line
  adjacency does not establish every possible entry. The current `f_lineno`
  setter scans every target-line word for compatible stacks, not just lexical
  region entries. An instruction-event callback can also reenter observation
  machinery. Removing a guard needs that proof, not just a preceding constant.
- **Generic const-pool operand:** defer. A constant index needs its own field
  role/width and representation-aware disassembly/code replacement semantics;
  floats do not fit as literal 64-bit payloads in the available fields. The
  existing small-int instruction supplies a simpler first consumer.
- **Feedback fact bits or lookahead fusion:** reject for this increment. They
  create persistent facts or another entry/event/fallback protocol while the
  word itself can carry the value. Entry-zero, instrumentation and counter
  state are not spare storage by implication.
- **Signed-12 expansion:** defer. Positive small producers already answer the
  useful increment/decrement/multiply question. Broader constants require a
  separate demonstrated consumer, not automatic variant multiplication.

## Proposed bounded follow-up

Recommend a **90-minute host-only representation prototype**, beginning only
after independent design review. One owner changes the shared generator/CFG
authority and meaningful host tests; no native run is implicit. Checkpoint at
30 minutes for the exact admitted domain/format and at 60 for the generated
generic miss and observer/stack proof. At 90, accept the source candidate for
bounded native feedback or reject it; do not silently extend the budget.

Require generated consistency across every admitted member, shared-entry-zero
and instrumentation restoration, boundary encodings, single/multiple/implicit
uses, canonical registers, aliases and exceptions. Demonstrate that ordinary
slot handlers/metadata remain byte-identical except intentional domain numbering.
Reject if handwritten arithmetic/fallback/effect copies, unchecked producer
facts or a growing dataflow lattice are needed, or if frame/reader changes
cannot use the shared materialized-input contract coherently.

Only after source acceptance select one separately bounded native feedback
batch: fresh O3 build, behavior covering int32 overflow/OOM, bool/subclass/
large-int/reflected/in-place/float/sequence paths and monitoring/debugger cases,
then inspect actual emitted code. Reject if the RHS admission, producer dispatch
and transient slot work are not removed, or if code growth/regressions outweigh
the cut. Any timing decision then needs a named recurrence consumer and matched
instruction/elapsed vectors; old invariant-add timings are not its control.
Runtime landing would still receive one coherent applicable qualification batch.
