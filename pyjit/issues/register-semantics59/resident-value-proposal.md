# Resident values: opportunity and first execution contract

Proposal only. No runtime change, probe, build or measurement accompanies this
note. Selected57 is `0dcba75bb3df40c32137db453b001f7e8f101de0`; its source in
`BASE/worktrees/call55-integrated` and its existing builds remain immutable.
Trials 1–4 are independent. A losing result-placement trial does not prohibit
this mechanism, and unfinished M4/IRSC2/bitmap execution is not a prerequisite.

## Trial 5: no admitted allocation consumer

Demote trial 5 for this cycle. The retained closed census
`E56/multi-use-borrow-census/census-final.json` has 426 graphs, 3,092 fresh borrow
loads and zero eligible COPY webs: 1,798 roots were already removed, 1,068 had
nonborrow terminals, 204 crossed callbacks, 12 crossed line/EH boundaries and
10 had nonfresh COPY provenance. Its morphology did not include arbitrary
acyclic joins; zero is not a claim that every possible join transformation is
useless.

Selected `regcfg_passes.c:218–306` counts outputs and implicit reads, folds
single consuming borrowed uses and preserves callback/local-write barriers.
`regcfg.c:457–481` canonicalises by renaming earlier operands, not by emitting
an edge copy. The retained optimized comprehension trace has folded local
inputs, window demands and callback boundaries; it supplies no surviving
diamond whose edge records would demonstrably remove an owner or physical
move. Do not add edge-argument storage, multi-reader reservations or parallel
move machinery without such a consumer. Reopening this lane requires one
retained post-pass example, its terminal acquisitions and its dynamic count.

## Trial 6: one resident lowering, several typed consumers

The unit is a short semantic region with original operation PCs, value IDs,
owner/use modes and exits. It is not a catalogue of opcode-name pairs. Lower
the admitted operations through the same current-value/typed-producer receipts
used by ordinary generation, retaining C values and checked projections until
the first boundary that needs materialization. Ordinary handlers keep their
current direct path: no region-table or operand-cache load is added to them.

The first concrete shape is two checked local-anchored field reads followed
by an inline-int comparison. Retained warmed IR in
`E56/comprehensions57-inspection/candidate-dis.txt` contains
`creator_id`, `owner_id`, `COMPARE_OP_INT` consecutively. Corrected inherited
site counts at `196284139a1`, words 62–64 of `WidgetTray._add_widgets`, are
108,000 visits each over 6,000 trays: 18 candidate runs per tray. This is
inherited evidence, not a fresh selected57 eligibility measurement. The
selected source still has these typed producers; the probe below must confirm
current operands, adjacency and entry restrictions.

Admit only local-anchored, nonmethod, mono SHAPE reads in the initial template,
with scratch results used only by the comparison. Each actual type-version,
shape, offset and filled-field contract remains required. Current inline-int
guards establish that the field values have no mortal count and their use and
retirement cannot call Python. Unknown or owning receivers, method/self
outputs, poly selection and noninteger values use ordinary execution. This
extends the existing checked field-read contract; it must not grow a second
body recognizer or infer purity from a handler number.

On success the two intermediate results remain in C, rather than being
published and reloaded. Their inline-int facts reach the comparison without
rechecking the same values. The comparison commits its ordinary destination
and leaves every consumed scratch cell in the existing NULL poststate. Local
replacement can finalize, so that final result and all other retained owners
must already be materialized before displaced-owner retirement. This removes
two intermediate stores/reloads and two dispatches; it does not remove the
final vacancy writes, necessary guards or final result commit. Selected
SHAPE_LOCAL_TEMP already avoids the ordinary getter/receiver cost, and
COMPARE_OP_JUMP is already fused. Neither is credited as a new saving.

The reusable boundary is `RegionValue(value_id, use/owner, representation,
projection_anchor)` plus a sequence of existing typed operations and
`RegionExit(original_pc, committed_prefix, materializations)`. These are
compiler/generator records, not per-frame proof bits. Canonicality never
creates an owner; a native owned result is not canonical until an actual
normalizing publication or a checked canonical producer proves it.

## Execution and reader closure

Keep immutable semantic words and their PCs. Build a code-owned admitted
region descriptor from their operands and existing value uses. An executing
first word may select a region entry whose payload identifies that descriptor;
the original first word is read from `_PyCode_WORDS(code)`. Interior words keep
their ordinary entries. The first bounded template needs an entry index and
an immutable original-PC span, not a persistent type plane or a map per home
per node. Descriptor allocation/index products must be bounded and checked.
Public constructor/replace/marshal images cannot nominate an executing region
without the code-owned validated plan; use the existing learned-only admission
boundary for that executable capability.

This is an executing-image encoding proposal, not a requirement to preserve
eight-byte physical instructions. It fits an index in the executing word's
48-bit payload for the first consumer. If subsequent independent operands need
more space, use a wider descriptor with explicit operands and price its loads;
do not force a window, synthetic move or extra home to retain eight bytes.
No semantic opcode or marshal-format change is needed merely for this first
execution choice. Actual plan fields and entry publication need source peer
before implementation.

The complete exit contract is:

| Boundary | Required action |
| --- | --- |
| Initial guard failure | No semantic effect committed; enter the original first operation with unchanged owners |
| Failure after a completed prefix | Materialize each retained prefix result exactly once, set the next original PC, and continue there; never repeat an effect |
| Consumer overflow/type fallback | Exit before the allocating/callback-capable path; materialize the original consumer inputs and run that consumer |
| Final local/discard cleanup | Publish the selected result/continuation first, then reuse ordinary displaced-owner/error cleanup |
| Line/opcode tracing, monitoring or source redirect | Remove/avoid the region entry and execute the original sites; preserve all events and replacement state |
| Interior branch, EH or debugger entry | Enter that unchanged ordinary word; no resident value or predecessor fact is assumed |
| Error, poll, callback, suspension or external call | Materialize all owners and required protocol state, publish the actual original PC/phase, then use the existing boundary |

Initially no region crosses a line/EH change, branch target, recovery entry,
poll, window call, frame switch or suspension. No unenumerated C owner or
borrow crosses a callback or collection point. The GIL build is the first
scope; asynchronous FT collection/inspection requires its own safepoint map,
not a claim that absence permits a raw read. Cold line attribution is not the
authority for either effect freedom or value lifetime. Instrumentation must
restore the original first word before interior events can become active;
installation publishes an immutable ready plan before its entry word.

For this first callback-free guarded interval, all guards can precede its
commits. Failure can therefore restart its original first operation without
allocating a materialization owner. The common committed-prefix exit remains
necessary for the next consumer family; do not describe a pre-effect guard
as having already performed a semantic read or ownership acquisition.

## Coverage and budget, before implementation

One small template is an architecture test, not a parity proposal by itself.
Selected57 comprehension counters are about 278,517 instructions/tray versus
193,897 upstream, an excess of 84,620. Even 20 instructions saved at each of
18 runs gives 360, or 0.43% of that excess. The already documented hypothetical
20 × 270 native-call saving gives 5,400, only 6.38% of the excess. Neither is
a measured instruction saving.

The same resident-state and exit machinery must next cover these finite
families, conditional on the full-row probe rather than assumed frequency:

| Follow-on | Eligible success interval | Boundary kept ordinary |
| --- | --- | --- |
| Inline integer arithmetic → arithmetic/compare/return | Actual inline/tag/range facts; no allocation or count retirement on the admitted arm | Overflow/boxing, noninteger input, local finalizer and return retirement |
| Exact tuple/list indexed read → typed scalar consumer | Existing exact-container/index/bounds receipt, live anchor, atomic acquisition; list remains GIL-only | Arbitrary element, mutation, callbacks and unknown consumer |
| Checked SHAPE read → another scalar use | Same current receiver/projection and no receiver retirement callback | Descriptor/method fallback, changed shape/type and observer entry |
| Dictionary subscription → predicate | Only an existing complete no-callback lookup-and-consumer contract | Mixed-key equality, hashing or Python predicate callbacks; currently excluded |

Do not generate the Cartesian product of specialized opcode IDs. Generate a
small set of typed computational shapes from one lowering interface and admit
runtime sites only when their complete operands/effects match. Charge code
size, descriptor loads, failed guards and materialization exits. A general
resident operand cache is a separate rival if these admitted regions cannot
cover enough work; it is not silently added to every handler.

Before selecting a first packet, retain nonoverlapping eligible-run counts for
the eight hot/sentinel cases, categorized exit counts, static code bytes and
native instructions saved per successful run. Compute
`sum(nonoverlapping runs × measured saved instructions)` per workload against
the retained instruction deficits; ranges/uncertain guard success stay visible.
The current retained evidence does not close that aggregate budget. If the
first template works but the follow-on population cannot make a material dent,
stop adding pair handlers and compare a bounded resident-cache/backend rival.

## Root probe and bounded implementation packet

Root should rerun the retained corrected site counter on immutable selected57,
but extend its output to **all** words. Its current feedback-only rows cannot
establish region adjacency or nonoverlap. Retain both generic/adaptive full
`get_ir` rows, feedback, physical operands, line/EH/recovery bytes and monitored
word counts before/after counting. Cases are comprehensions, deltablue,
coroutines, generators, logging_silent, raytrace, spectral_norm and nqueens.
Monitoring counts baseline semantic entries; they are not region success rates.
Use a new evidence tag and existing source/build/affinity/identity preflight.

The task-owned `E59/resident-allword-census.py` now implements this adapter,
without modifying or importing the sealed counter. All eight serialized root
runs complete successfully, with exact before/after identities; the command
ledger is `E59/resident-allword-commands.json`. It
pins selected57 source/native/Lib, the retained ELF hash, CPU 2 and JIT-off;
checks clean source and before/after identity; records generic/adaptive words,
feedback, recovery/positions and visits for every word; and exclusively creates
each output. Kernel and adapter hashes are checked across collection. Warmups
and the five existing hot-case loop counts match the corrected counter;
raytrace/spectral_norm/nqueens use one diagnostic invocation each.

Retained first command (already executed; do not overwrite its output):

```sh
PYTHON_JIT=0 taskset -c 2 \
  "$BASE/call55-integrated-release/python" -S -B \
  "$E59/resident-allword-census.py" comprehensions \
  --output "$E59/resident-selected57-comprehensions.json" \
  > "$E59/resident-selected57-comprehensions.log" 2>&1
```

All other seven cases have distinct retained outputs/logs under the same
preflight. Monitoring deoptimizes many entries; counts weight pre-monitor
warmed labels rather than natural specialized successes. Also capture the existing
`PYTHONREGCFGDUMP=1` pre/post compiler dump for shortlisted functions so source
renames, already folded loads and implicit/EH uses are excluded. No selected
build, sealed script or sealed evidence is modified.

After that frequency gate, the first source packet is: compiler admission/use
validation (`regcfg_passes.c`, existing operand slices); region plan ownership
and entry installation (`pycore_ir.h`, `ir.c`); shared resident lowering and
typed producers (`ir_generator.py` and existing helper contracts); executable
entry/admission/defusion (`codeobject.c`, `instrumentation.c`, generated
products); focused region ownership and event fixtures. Frame layout stays
unchanged. Tests must cover guard misses at each site, operand aliases, live
output rejection, locals rebound by finalizers, all observer modes, interior
entry, public image forging, original error PC, no duplicated effects and
missing source/publication receipts. Root owns full generation/native gates.

## Backend rival: keep interpreter state within one GCC function

This is a separate, unimplemented rival to resident regions. The retained
selected57 ELF is `E55/comp57-dictionary-native/python`, source `0dcba75`,
SHA256 `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
The following anchors are virtual addresses in that ELF, read from disassembly;
they describe particular paths, not average executed costs.

| Handler | Entry/dispatch anchors | Actual state and frame work |
| --- | --- | --- |
| `LIST_APPEND` | `0x354090`; dispatch `0x3540f8–0x354119` | Four incoming state copies, three outgoing copies; push/frame-pointer setup/pop, no stack reservation |
| `LOAD_COMMON_CONSTANT` | `0x35d080`; dispatch `0x35d163–0x35d184` | Four incoming and three outgoing state copies; push/setup/sub/leave, 16-byte reservation |
| `BINARY_OP_ADD_INT` | `0x367370`; ordinary success dispatch `0x36744f–0x367472` | Four incoming and three outgoing state copies; four frame instructions, 32-byte reservation; operand-word decoding and overflow spills are separate costs |
| `COMPARE_OP_INT` | `0x366790`; dispatch `0x366986–0x3669a7` | Four incoming and three outgoing state copies; four frame instructions, 16-byte reservation |
| `LOAD_ATTR_SHAPE_LOCAL_TEMP` | `0x378a80`; dispatch `0x378b2e–0x378b48` | Only tstate is copied out and back; PC uses a temporary; three frame instructions, no stack reservation or helper call; total handler 240 bytes |
| `JUMP_BACKWARD_NO_INTERRUPT` | `0x34c390` | No stack frame and no persistent-state parameter repacking |
| `FOR_ITER_LIST` | `0x3957e0`; getter `0x3958b0–0x3958c9` | Four incoming state copies and four frame instructions, 32-byte reservation; three spills and three reloads around `PyList_GET_VALUE` at `0x3958bc` belong to the getter live range, not automatically to dispatch |

The five release parameters enter in RDI/RSI/RDX/RCX/R8. Several handlers move
frame, slots, tstate and PC into RBX/R12–R14 across ordinary helper calls, then
move them back to ABI parameter registers before the tail jump. GCC's
`no_callee_saved_registers` already avoids saving/restoring those registers for
the caller. Those pushes/pops must not be counted again. `enter_handlers`
(`ceval.c:2404–2419` in selected57) is a once-per-activation ABI shim, not a
per-dispatch call. Native call handlers have additional provider/cleanup live
ranges; their stack spills are not a uniform five-parameter tax.

The ordinary dispatch sequences above load one next word, extract its handler,
load a table target and jump. They also form the next PC and usually load the
table base. There is no established duplicate ordinary word load to remove.
Quickening's current-handler reread and callback/frame-change continuation
reloads serve different purposes. Computed goto still needs a word load,
handler extraction and indirect transfer.

A closed same-GCC experiment would put the existing generated bodies in one
normal-ABI `run_handlers` function, with function-local label addresses and
scoped `TARGET`/global-label blocks. Keep `_PyEval_EvalFrameLoop` as the outer
activation owner and debugger symbol. `DISPATCH_GOTO` becomes a computed goto;
prediction preserves the current word and `this_instr`; error-label jumps
still finish value bridges. `PROCESS_ERROR_AT` retains its actual value-temp
ownership and reloads returned frame/slots/continuation. Interpreter exit and
unhandled error return from the engine exactly once through outer activation
cleanup. Entry chooses START/THROW once; label addresses do not escape the
engine. Helper ABIs, semantic words, metadata, proof consumers and observer
protocols remain shared.

The source closure is `Python/ceval_macros.h:45–174` and `NEXTOPARG` near 249,
the entry/outer loop in `Python/ceval.c:2404–2640`, and target/global-label/table
generation in `Tools/cases_generator/ir_generator.py` near 7105, 7381 and
9001–9042. Configure already checks computed-goto support near
`configure.ac:7906`; build/preflight must nevertheless record the chosen
backend explicitly rather than reuse the tail-call compiler identity.
Generated bodies and tables change; semantic metadata should remain identical.
Audit GDB variable visibility, Py_STATS' sixth parameter, free-threaded atomic
loads, LLTrace, scoped locals and transition-root cleanup. A single larger
function can increase stack size, spills and cold-path register pressure;
GCC tail merging, CET targets and changed dispatch locality can reverse the
benefit. Do not rescue it with alignment searches or unreviewed compiler flags.

For scale only, the inherited corrected comprehension counter
`E56/site-counts-corrected-196284/comprehensions.json` records 13,674,014
kernel-word visits over 6,000 trays, about 2,279 visits per tray. A hypothetical
ten instructions removed at every recorded visit would save about 22,790
instructions against the retained 84,620-instruction deficit. This overstates
cheap handlers and excludes library visits; neither ten nor the resulting
saving is measured. The new selected57 all-word census must supply actual
frequencies. Budget each path's removable entry/exit work, subtract new spills
and dispatch work, then compare same-work PMU counts and throughput. This audit
does not establish parity or authorize a backend implementation.
