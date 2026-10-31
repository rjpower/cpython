# Lowering already optimizes locally; stage literal operands next

2026-10-31. Source research only. We already perform useful compiler
optimizations, including constant folding, control-flow cleanup, local load
and store folding, limited dead-result deletion, and frame-slot compaction.
We do not presently run a general SSA optimization pipeline. The strongest
small next step is the already assigned constant operand prototype. A literal
subscript operand is the clearest conditional successor. General SSA is not a
prerequisite for either change.

## Exact scope and evidence

The inspected PRIMARY checkout is `32f14069fd2a0b34b67f167621d2de4671369a49`.
`git diff --quiet d98291018c9665ae3db6952da0b0a40a4e04992b 32f14069fd2a0b34b67f167621d2de4671369a49 -- Python Include Tools Lib/test`
returned zero: these runtime/compiler sources are unchanged from integration.
This report reads source and existing issue evidence; it creates no native
execution, build, profile, benchmark or speed estimate. Later source candidates
must bind their own identities. LuaJIT references are the official `v2.1`
branch retrieved on 2026-10-31, not an asserted immutable commit.

## What our pipeline actually does

[`compile.c:1662`](../../../Python/compile.c#L1662) sends stack instructions
through `_PyCfg_OptimizeCodeUnit`, then `_PyCfg_OptimizedCfgToRegCfg`, before
assembly. These stages must be inspected together: an optimization absent from
the register pass list may already have happened in the stack CFG.

| Stage | Present behavior and limits | Source |
| --- | --- | --- |
| AST preprocessing | Invoked before symbol-table construction. Its `fold_binop` performs a string-format rewrite, not ordinary numeric arithmetic folding in this snapshot. | [`compile.c:146`](../../../Python/compile.c#L146), [`ast_preprocess.c:371`](../../../Python/ast_preprocess.c#L371) |
| Constant evaluation | Adjacent constant producers and unary/binary operations fold; arithmetic has size/complexity limits. Failed constant arithmetic normally stays for runtime evaluation; KeyboardInterrupt propagates. This does not propagate constants through arbitrary local assignments or joins. | [`flowgraph.c:1900`](../../../Python/flowgraph.c#L1900), [`flowgraph.c:1961`](../../../Python/flowgraph.c#L1961) |
| Stack CFG cleanup | Constant conditions, unreachable blocks, jump threading, redundant jumps/NOPs, constant collections and selected stack permutations are simplified. Uninitialized-local checks and cold-block layout are also computed. | [`flowgraph.c:2300`](../../../Python/flowgraph.c#L2300), [`flowgraph.c:2436`](../../../Python/flowgraph.c#L2436), [`flowgraph.c:2720`](../../../Python/flowgraph.c#L2720), [`flowgraph.c:3890`](../../../Python/flowgraph.c#L3890) |
| Truth and comparison staging | `COMPARE_OP` followed by `TO_BOOL` gets its bool-result bit; identity/membership results avoid redundant `TO_BOOL`. Conditional jumps consume arbitrary truth values, removing preceding conversion and inverting for `not`. `x is None` plus a branch already becomes a dedicated None branch. | [`flowgraph.c:2346`](../../../Python/flowgraph.c#L2346), [`flowgraph.c:2574`](../../../Python/flowgraph.c#L2574), [`flowgraph.c:2679`](../../../Python/flowgraph.c#L2679) |
| Register representation | Fresh temporaries have one definition within a region. Locals/cells/free variables remain mutable. Joins, jumps and exception handlers use canonical stack-position registers **without phi nodes**. | [`pycore_regcfg.h:14`](../../../Include/internal/pycore_regcfg.h#L14) |
| Register rewrites | Fixed order, once each: swap renaming, copy propagation, store elimination, pop-top elimination, truth folding. Swap renaming removes selected stack permutations. Copy propagation folds a safe borrowed local load into a consumer field. Store elimination writes a producer result directly to a local; it is not general dead-store elimination. | [`regcfg.c:1405`](../../../Python/regcfg.c#L1405), [`regcfg_passes.c:65`](../../../Python/regcfg_passes.c#L65), [`regcfg_passes.c:153`](../../../Python/regcfg_passes.c#L153), [`regcfg_passes.c:310`](../../../Python/regcfg_passes.c#L310) |
| Limited dead-result deletion | A single-use, nonfailing, nonescaping DSL-pure producer whose inputs are locals/constants can disappear with `POP_TOP`. Other pops retain root-clearing work, and a NOP can preserve a line event. This is not general transitive DCE. | [`regcfg_passes.c:416`](../../../Python/regcfg_passes.c#L416) |
| Short-circuit value context | Selected copy/test/fallthrough-pop sequences become `JUMP_IF_*_OR_POP`, retaining the original value on the taken edge and clearing the other edge. | [`regcfg_passes.c:523`](../../../Python/regcfg_passes.c#L523), [`bytecodes.c:3406`](../../../Python/bytecodes.c#L3406) |
| Liveness and slot assignment | Backward fixed-point dataflow includes jump edges, implicit unwind uses and exception-handler live values. Compaction enforces canonical unwind slots, contiguous call windows, root lifetimes and folded-load reservations. This is substantial interblock analysis, but not value propagation. | [`regcfg.c:609`](../../../Python/regcfg.c#L609), [`regcfg.c:658`](../../../Python/regcfg.c#L658), [`regcfg.c:771`](../../../Python/regcfg.c#L771) |
| Representation selection | Assembly already selects inline compact-integer and non-NaN float constant loads. A small-integer load produces an inline value, but still occupies its own word before arithmetic. | [`assemble.c:628`](../../../Python/assemble.c#L628), [`bytecodes.c:265`](../../../Python/bytecodes.c#L265) |

The complete lowering driver and its representation provide the relevant
absence evidence: no dominator tree or SSA phi construction, sparse conditional
constant propagation (SCCP), global value numbering/common-subexpression
elimination (GVN/CSE), general dead-store/result elimination, or loop-invariant
code motion (LICM) is invoked by this pipeline. The stack optimizer does have
specific constant/control-flow simplifications; calling all constant
propagation absent would be wrong. Likewise, liveness reaches a fixed point,
whereas the five register rewrite passes do not iterate to a fixed point.

The missing machinery alone is not evidence of a useful next experiment.
For example, commoning dynamic `a + b` can suppress Python calls and exceptions;
moving a load can cross `frame.f_locals` mutation; deleting a dead temporary
can leave an owned root uncleared. A proposal needs a concrete legal consumer,
not just the name of a compiler pass.

## What LuaJIT contributes at each stage

The official overview distinguishes its assembly interpreter from its SSA
trace compiler. They offer different comparisons for this work.
([LuaJIT overview](https://luajit.org/luajit.html))

The baseline parser folds numeric literal arithmetic, chooses variable/constant
arithmetic variants, selects literal-byte or constant-string table access, and
emits comparison tests directly in branch context. These are decisions about
the source operand and expression use, not speculative facts propagated from
an earlier mutable frame slot.
([`lj_parse.c`, `foldarith`, `bcemit_arith`, `expr_discharge`, `bcemit_comp`](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_parse.c#L750))

The bytecode definitions explicitly distinguish `ADDVN`/`ADDNV`/`ADDVV`,
`TGETB`/`TGETS`/`TGETV`, and comparison/test operations.
([`lj_bc.h`](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_bc.h#L65))
The x64 interpreter uses those operand sources, retains table/type/bounds and
metamethod fallback checks, and implements comparison plus a following JMP
without a separate ordinary JMP dispatch. The latter uses the next bytecode's
target: it is not a self-contained one-word comparison-and-branch encoding.
([`vm_x64.dasc`, comparison handlers, arithmetic forms and TGETB](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/vm_x64.dasc#L2949))

Trace optimization is separate. The FOLD/CSE pipeline is under `LJ_HASJIT`
and repeatedly simplifies emitted IR.
([`lj_opt_fold.c`](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_opt_fold.c#L14))
Trace DCE marks snapshot roots and side effects before removing unused results.
([`lj_opt_dce.c`](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_opt_dce.c#L10))
Its loop optimizer uses trace-specific unrolling, substitution, redundancy
elimination and phi construction with guard constraints; it is not evidence
that the baseline bytecode compiler already performs general LICM.
([`lj_opt_loop.c`](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_opt_loop.c#L19),
[`lj_ir.h`](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_ir.h#L1))

The transferable lesson is to expose a compiler-known operand source to the
interpreter and keep fallback semantics shared. Our existing borrowed-local,
producer/store and short-circuit folds already apply related staging. This
comparison does not establish that adopting LuaJIT's trace optimizer or frame
conventions would improve this interpreter.

## Ranked, bounded hypotheses

### 1. Finish the assigned small-integer arithmetic operand prototype

**Claim:** a source literal RHS can eliminate a load word, temporary traffic
and a redundant RHS admission while preserving the existing arithmetic bodies.
This is the closest concrete analogue to arithmetic constant-operand staging.
It is an existing experiment, not a second implementation assignment.

The [current design](../interpreter-constant-operands/design.md) admits existing
`LOAD_SMALL_INT` 0–255 producers, adjacent to a single-use arithmetic consumer
in the same region, line and exception context. It covers add/subtract/multiply
and in-place operations, their integer/mixed-float specializations and sequence
fallback. Fields remain A=destination, B=dynamic left slot, C=unsigned immediate,
D=feedback; no field budget or feedback entry is added.

The same word materializes its exact RHS, so entry does not trust the previous
word's output. Dynamic left guards, overflow, reflected/in-place operations,
allocation and errors remain. The generator must distinguish logical inputs
from physical slot reads, derive unwind/root handling, preserve the form on
deoptimization and monitoring restoration, and update debugger stack analysis,
disassembly and serialization consistently. A scalar immediate needs no GC
root; other inputs and published results retain the existing ownership rules.

**Finite decision:** honor the assigned 90-minute host prototype, its 30/60-minute
checkpoints and existing independent review. Reject if it needs a duplicate
effect/fallback model, trusts mutable slots, or cannot cover every dispatch and
observer entry with one authority. The [existing results](../interpreter-constant-operands/results.md)
show actual remaining native work; they do not show a speedup. Native feedback
and its gates require a separate coordinator decision after source review.

### 2. Conditional successor: a small literal integer read subscript

**Claim:** after operand-source authority exists, `container[0]` through
`container[255]` can remove the separate small-integer load and index admission
without assuming a container type. Keep scope to read-only `NB_SUBSCR`, not
stores, slices, negative-index encodings or range analysis.

The proposed word uses A=result, B=container slot, C=immediate index, D=feedback.
It reuses the current generic subscription semantics and all reachable
specializations: list, tuple, ASCII/unicode string, dictionary and Python
`__getitem__`. The latter can transfer into a Python frame and must receive
the materialized index in its argument slot. Literal integer input makes the
list-slice specialization unreachable; that is a source-kind proof, not a
container proof. The existing bodies are in
[`bytecodes.c:1055`](../../../Python/bytecodes.c#L1055) through
[`bytecodes.c:1243`](../../../Python/bytecodes.c#L1243). The selection logic in
[`specialize.c:2501`](../../../Python/specialize.c#L2501) admits these six
specialized bodies; the extension descriptor table at
[`specialize.c:2321`](../../../Python/specialize.c#L2321) has no `NB_SUBSCR` entry.

Use the same adjacent/single-use/same-line/region/exception restrictions as the
first prototype, and reject implicit unwind uses. At every entry reconstruct
the literal from the word. Retain dynamic container guards, existing container
access and ownership checks, bounds checks, missing-key behavior, `__getitem__`
callbacks, exceptions and generic fallback. Do not hoist an element or a length
across mutation. Check
that physical index-slot removal neither loses container roots nor changes
callee argument publication; the immediate itself needs no root.

**Finite decision:** first allow at most 30 minutes of host-only source and
saved-artifact inspection to inventory reachable handlers and identify an
actual eligible site in a registered workload. Reject this follow-up at that
checkpoint if the authority cannot express it without another mechanism, or
only a new synthetic example supplies evidence of usefulness. Source work and
native measurements need a new explicit budget after that review. Before any
timing, require causal word/dispatch/index-admission elimination plus correctness
for every reachable specialization, warmup/deopt, observer entry, frame transfer,
IndexError/KeyError and custom failing/mutating `__getitem__` paths. A proposed
native gate is two rotating-order five-sample rounds against the accepted
arithmetic parent: at least 1% fewer instructions and lower elapsed median in
both rounds on the preselected affected workload, with agreed unrelated
controls. A diagnostic loop alone cannot satisfy the workload gate. No gain
has been measured, and this proposed gate authorizes no native execution.

## Why identity/compare-to-branch is not ranked as an easy next increment

There is real residual work: even a bool-result `COMPARE_OP` (argument bit 16),
or `IS_OP`, can feed a generic/adaptive truth branch, whose bool specialization
still checks the input. Deleting that guard based on the previous producer is
unsafe: [`frameobject.c:1912`](../../../Objects/frameobject.c#L1912) considers
compatible stacks at every word on a target line, and its stack kinds do not
carry a bool refinement. Same-line adjacency does not prove that producer ran
on every legal debugger entry. A callback can also mutate a local folded into
a consumer. This remains true when identity itself cannot invoke Python code.

A self-contained identity/branch would recompute `PyValue_Is` from its two
actual inputs, avoiding a stale bool fact. But A/B input slots plus CD's
24-bit target consume all four fields of the current word; no feedback field
remains. Encoding identity polarity and branch sense into handler variants
does not recover the feedback field. Current truth branches record branch
history, and even the existing `JUMP_IF_*_OR_POP` explicitly omits feedback
because its fields fill the word
([`pycore_ir.h:18`](../../../Include/internal/pycore_ir.h#L18),
[`bytecodes.c:3299`](../../../Python/bytecodes.c#L3299),
[`bytecodes.c:3406`](../../../Python/bytecodes.c#L3406)).

Retaining feedback would require a separately reviewed representation, such
as a target in the feedback entry plus a defined entry-zero fallback. Borrowing
LuaJIT's following-JMP target would instead introduce a multiword entry and
event contract. Neither is an encoding-neutral peephole. The common narrow
case `x is None` is already covered by dedicated one-input branches, so it
cannot justify new infrastructure. Hold general identity fusion until saved
opportunity evidence pays for this explicit encoding/observer cost; reject
unchecked bool-consumer forms in the current scope. General rich comparison
also adds callback/error semantics and should not be smuggled into that cut.

No rejected frame-copy, call-setup or shape-admission experiment is reopened.
No SSA framework, pass-order iteration or generalized pure-expression DCE is
recommended without a concrete missed consumer and an independent lifetime
and observer proof.
