# Project status

## Live state

**Selected: golden87, 2026-11-23.** `684a98eb30e` (`pyjit/round87-golden`,
tag `pyjit-golden-20261123-contract87`, pushed) is
[contract87](issues/register-semantics59/contract87.md)'s integration, the
first phase of the [tiers87](design/proposals/tiers87.md) plan: golden86
plus ten units:
- the operation record (CR);
- VM routines and a function form for every handler (TR);
- the leaf subset (LS);
- stack bytecode in `co_code` and `.pyc`, with the register IR lowered at
  first call (SB, with ST's lowering cost fixes);
- one native call adapter (NV), raise and catch (EH), one exec pointer per
  frame (FR), attribute misses (AT) and XP's iteration change.

It runs **1.001 of golden86's time and 0.876 of upstream's** on the timed
all-29 geomean, measured before ST, and executes 0.9935 of golden86's
instructions. The allocator product is 42.57M. richards runs 0.789 of
golden86's time; logging_silent, coroutines, float and five more are 2–33%
slower at equal instructions, from op-cache placement and, for float, one
shared native-call site. Startup is 79.0M instructions for `-I -S -c pass`
(golden86 37.5M): the cost of first-call lowering, which the user accepts.
The full suites show known failures only.

**Active: rir90, 2026-11-24**
([plan](design/proposals/rir90.md)). The user decided that semantics move
from the stack uops to a register instruction set designed as V8 would
design one:
- stack bytecode becomes transport, lowered at first call;
- register instruction families carry all behavior, defined in a typed
  language that generates the interpreter, function forms, templates,
  stubs and the tracer's SSA.

Astra's [design](design/proposals/rir90-astra-design.md) and
[addendum](design/proposals/rir90-astra-addendum.md) back the plan. By
the end condition agreed with the user, the register IR and the
interpreter hold no reference to the stack at step D, and tests check it.

`pyjit/rir90` at `674dc7aff6b` (pushed; OB merged, below) holds:
- the foundation: F0, the language; F1, its checker; F2, the generated
  products; F3, the stack-op translator; P0, per-arm tables;
- the pilot families: locals, constants and `Move` (P1), `Add` (P2),
  `GetAttr` and `GetMethod` (P3), `Call`, `InitResult` and `Return` (P4),
  and truth, branches and jumps (P5);
- the freeze: PT, TT, EC, SSA, the frozen language (LF), CA, HN, IL and TM
  ([plan](design/proposals/rir90.md));
- W1: global loads (GR), namespaces and imports (NS), items and slices (IT),
  functions and intrinsics (FN), displays and unpacking (DP), comparisons,
  containment and test-and-branch (CP), cells (CE), attribute writes, super
  and special lookups (AW), numeric operators (BN), text, length and
  patterns (TP);
- W2: keyword and expanded calls (KC), iteration (FI), exceptions (EX),
  resume and polls (RS), send, yield, awaitables and async iteration (SY),
  throw completion, async-for end and context exit (TC);
- LO, the last inherited handlers:
  - IR_JUMP becomes `Jump.Trailing`, an arm declared to belong to the
    instruction before it;
  - a word that is never emitted, such as NOP, takes no handler;
  - line and instruction events are `displacing` arms with a `dispatch`
    exit, which the checker refuses in any other arm;
- the known failures fixed once: settrace jumps refused by design and raw
  value bits gone (KF), tests that deletions cannot break, with STACK_ONLY
  derived (TS);
- PS:
  - stack opcode numbers are pinned to upstream's table, so ports no longer
    bump the pyc magic number (64451);
  - register handler ids are dense and independent of stack numbers, with
    generated maps between the two spaces and range asserts;
  - an assert build with every id shifted (`PYJIT_IR_ID_BASE`) runs the smoke
    set at integration points;
- D's steps so far:
  - the stack-op schema is a Python table (`stack_ops.py`) that the opcode
    generators read, and the `.rop` externs are the one declaration of each
    C row;
  - `bytecodes.c`, the stack-semantic generator and their tests are deleted;
    the arms are dense from id 1 (294 handlers);
  - specialization statistics are kept by register family and arm;
  - instrumentation was stated by handler (replaced by OB, below): a word's monitoring state was its
    handler, its observed twin, or a line or instruction wrapper over either,
    and the opcode-indexed event tables are deleted;
- WF1-WF3: the `.rop` language declares each family's homes, vacant parts
  and per-edge states (`vacant(...)`, `Out<G>`), and the generator emits
  them as a homes view of the word format.  The verifier, the `f_lineno`
  planner and the outcome readers read that view; the stack-shaped outcome
  tables and the verifier's dead owner-program paths are deleted.  This
  fixes three classes of wrong answers in the old planner: a completed
  value reported NULL, an IterClose cursor never named, and a keep-mode
  BranchTruth jump reported as retired;
- D's commit A: the planner and the verifier find a word's fall-through
  edge in its declared normal edges, not in stack scope-exit and
  jump-always tests;
- WF4: the register format keeps no stack field; the translator reads its
  own stack format, generated from `stack_ops.py` into a header only the
  translator includes, and derives an arm's stack row from its word format;
- D's step 6, first part: `test_ir_no_stack` scans every runtime source
  under `Python/`, `Objects/` and `Include/internal/`, less the compiler,
  translator and opcode schema, and fails on a stack op name, a
  handler-to-opcode map or a stack-shaped field; its exceptions name a file,
  its functions and a reason, and fail when stale.  The handler-to-opcode
  maps and the `shadow` field are deleted, and the `.rop` sources have no
  `ops(...)` clause;
- PAR: the tests that expected stack specialization names (`test_embed`,
  `test_type_cache`, `test_pyvalue`) ask which register arm a word runs;
- D's correction of a root error: item 6 had moved the `.rop` `ops(...)`
  clauses into `stack_ops.py` as a static stack-op-to-family table, which
  the user had ruled out (no fixed lowering per stack op).  The table is
  deleted; the stack-op schema keeps encoding facts only, every stack op
  the compiler emits has a translator rule, an op without one is refused,
  and a test checks that a translator rule emits every family;
- D's item 7: the test disassembler (`test.support.ir_dis`) yields one
  record per word, with the arm, its operands by declared name, kind and
  role, its edges and its origin offset, decoded from declared exports;
  its 26 test modules check arms, and `test_ir_no_stack` scans it;
- D's item 8: the checker refuses an arm that switches frames without
  declaring that it runs Python (ten definitions had lost that fact in
  their ports), and tests read the IR's declared enums by name.

D is closed: the register IR and the interpreter hold no reference to the
stack, which is transport only.

Interpreter-performance wave 1 is merged:
- DBG: the Py_DEBUG and Py_STATS builds run again.  A home store is checked
  by the declared type of the part it writes, so a loop cursor's tagged
  index no longer aborts the debug build at bootstrap, and a nested
  activation starts with a defined handler for the statistics;
- TB: the generator emits tighter arm bodies.  A guard's facts hold for
  the rest of its block, so a value a guard proves inline or immortal is
  not closed; the poll tests its flag inline; the integer and float arms
  decode each operand once; call anchors are scalar locals;
- INL: `ceval.c` holds the interpreter, the function forms and stubs
  compile in their own unit, and the hot helpers are declared always
  inline, with no global compiler flag.

From wave 2, SP's first part is merged: `Call.PyGeneral` and `ForIter.Gen`
are the ports of CALL_PY_GENERAL and FOR_ITER_GEN (a `for` over a generator
enters its frame in the same activation), and SP's coverage list names the
arm that covers each of golden87's specializations.  Twenty have no arm, a
breach of the port rule; SP's item 4 ports them, except floats' power, which
AR ports.  The id-shifted guard build had stopped importing
`test.support.ir_dis`, because the handler-name export assumed that every
id below the shift had a name; an id with no handler now has the name None.

PL is merged.  Each span arm's word length and element positions are
constants of its declared layout, so no hot path decodes span geometry.
Unpack writes its items straight into its homes, in target order: the
heap owner buffer, its reversal and its second pass are deleted.  A tuple
unpack cannot fail; a list is checked again under its lock before any
write; the generic and starred forms stage their items in a tuple, so a
wrong length or a raising iterator leaves every home unchanged.  The arm
publishes through a C park of 16 homes and holds a longer source by a
count; frame temporaries reserved at lowering would replace it, which
needs a language change (on the freeze list).  Measured on CPU 1 (cycles
of golden87's): unpack_sequence 2.91 to 1.83, nbody 1.91 to 1.53,
spectral_norm 1.87 to 1.28.

MV is merged: no parallel move is solved at run time.  A retirement is its
own family (`Retire.Close`), which closes its sources and moves nothing;
the translator orders a move's pairs at lowering so each destination is
vacant when its pair runs, and each cycle becomes a `Rotate.Cycle` word;
the unwinder's ingress recipes are sequenced the same way.  The run-time
solver (`_PyFrame_ParallelMove`) is deleted.  Generators run 2% fewer
instructions and about 7% fewer cycles; IP's estimate for this cause was
five times too high, and generators' excess (1.62 of golden87's
instructions) is still inside `ir_evaluate`.

AR is merged: a mode operand selects its arm.  The `.rop` language states
an arm over the members of an enum operand (`over(...)`), and the checker
expands it into one arm per member, so every product sees ordinary arms.
BranchTruth has an arm per sense, Binary.Float one per operator (power
among them, admitted for a positive finite base and a normal result),
Binary.Int one per hot operator and a rest arm, and Compare and TestBranch
one per relation.  The generator emits a C enum for every declared `.rop`
enum, and the hand-written duplicates are deleted.  There are 366
handler ids; the id-shifted guard build passes at base 96.  Measured on
CPU 0 (cycles of golden87's): nbody 1.90 to 1.76, spectral_norm 1.83 to
1.67; the float and branch micro-loops run 7-12% fewer instructions.
nbody still takes 1.58M branch misses per run against golden87's 1.08M,
from some other cause.

A refleak run (`-R 3:3`, 16 modules) finds no reference leak, and the same
memory-block growth before and after TB (a backlog item).

No word runs an inherited handler. The last merge check passed the smoke set
on release and assert, and the verifier reports no error over 108,927 code
objects. The full suites at `0a81f42d9bf` (release and assert, JIT off)
failed only the three stale specialization tests PAR then fixed, and the
id-shifted guard build passes at `d8cb2109192`. Ports keep the old
handlers' object locks and turn every old specialization into an arm.

Interpreter first (user, 2026-11-25): port every op, reach test parity, then
interpreter performance parity with golden87 (within 5% on the timed
geomean, JIT off); only then the JIT. Units run the tests their branch
touches; root runs the smoke set at each merge and full suites at the wave
join.

Interpreter timing at `d8cb2109192` (measure.py, both JIT off, all-29, two
rounds, CPU 15): rir90 runs **1.559 of golden87's time**.  C-heavy work is
near parity (pickle, unpickle and xml_etree_parse 1.04, json 1.08-1.10);
pure-Python kernels are far off (nbody 3.18, unpack_sequence 3.01,
spectral_norm 2.01, generators 1.88, chaos 1.86, nqueens 1.85).  nbody
executes 2.05 times golden87's instructions at an IPC of 2.06 against 3.08;
`ir_evaluate` takes 60% of its time and out-of-line routines most of the
rest.

IP's [diagnosis](issues/register-semantics59/rir90-interp-diagnosis.md)
(1.563 of golden87's cycles, 1.407 of its instructions): rir90 executes as
many words as golden87 or fewer, with the same dispatch, so the excess is
in the handler bodies.  The causes, by expected gain:
1. GCC outlines `static inline` helpers because `ceval.c` holds the loop,
   the function forms and the stubs (about 35k generated lines); raising
   the inlining budget for that unit alone gives 1.396.  Blanket inlining
   is worse (1.467).
2. Arm bodies are heavier even when inlined (7-22 cycles per hot arm):
   closes of operands a guard proved inline, address-taken aggregates,
   values kept live for cold exits, mode operands read at run time;
   10-15%, up to 19%.
3. Unpack goes through a heap buffer with span geometry decoded at run
   time: about 5%.
4. Missing arms (CALL_PY_GENERAL, FOR_ITER_GEN, BINARY_OP_POWER) and a
   parallel move solved at run time: 1.5-2.5%.

First-call lowering costs nothing in steady state.

Interpreter timing after wave 1 (`c2071606f67`, measure.py, both JIT off,
all-29, two rounds, CPU 15): **1.340 of golden87's time**, from 1.559.
nbody 3.18 to 1.85, chaos 1.86 to 1.37, float 1.79 to 1.34; unpack_sequence
(2.96) and generators (1.82) wait for wave 2.

SP's item 4 is merged except its last group.  Fourteen native calls
(`isinstance`, `len`, `list.append`, the builtin and method-descriptor
forms and the type, str and tuple constructors), the general bound method,
the generator-function form of `Call.PyGeneral`, and the property,
overridden `__getattribute__` and Python `__getitem__` frames each have an
arm, selected by adaptation where golden87's specializer chose them.  Over
the all-29 kernels, `Call.Generic` runs 1.3M words where it ran 13.2M.
`FOR_ITER_DICT_ITEMS` is held: golden87's form skips the unpack word that
follows it, and the port wrote that word's homes without declaring them
and added a verifier exception for its computed jump.  Its register form
is a fused for-loop step with both destinations declared at lowering (on
the freeze list); it is the one golden87 specialization without an arm.
There are 401 handler ids; the id-shifted guard build passes at base 96.

Interpreter timing after wave 2 (`25e88d21f6e`, measure.py, both JIT off,
all-29, two rounds, CPU 15): **1.191 of golden87's time**, from 1.340.
unpack_sequence 2.96 to 1.85, nbody 1.85 to 1.37, generators 1.82 to 1.39,
spectral_norm 1.79 to 1.41, comprehensions 1.47 to 1.16, nqueens 1.49 to
1.12.  Eight benchmarks are within 10% of golden87; json_dumps (1.03 to
1.13) and logging_simple (1.15 to 1.20) are slower than after wave 1.
The join's full suites (`25e88d21f6e`, release and assert, JIT off and
always) found two faults.  `test_value_slots` asserted that a plain
attribute read goes through the type's getattro slot, which SP's
`GetAttr.Generic` no longer does (behaviour unchanged; the test reads
through `getattr()`, `6b74f9f0cd6`).  With the JIT on, four modules
segfaulted: SP left `GetItem.ListSlice` compiled as a stub whose miss
calls a generic stub that its split family no longer has (JX, below).

**OB, merged at `58712a90555`, 2026-11-26** (user decision): observation is
a mode, not IR.  Every arm of an event family had an observed twin with
its own id, and line and instruction hooks displaced words in the
executing copy: upstream's in-place instrumentation restated per arm.  Now
an observed code object's frames run in a second compilation of the
evaluator (`Python/ceval_observing.c`), which sends each event family to
its observer and reads line, instruction and event bits per word from the
code's observe table.  The words, the handler ids (401 to 302) and every
reader carry no monitoring; `DISABLE` clears a bit and rewrites no word,
and specialization survives a tool's coming and going.  Frames switch at
RESUME (the observed mark fails the existing version test), at the poll
after a change, and by the stop-the-world walk that redirects each
running frame's resume point.  The normal path's instructions are within
0.3% of before; coverage-style monitoring runs 1.2 times the old cost and
`settrace` line tracing 0.7 times.  A change made by a finalizer or a
native call in the middle of straight-line code reaches the running frame
at its next switch point, not at the next instruction (`674dc7aff6b`
states it in `test_pyvalue_release`).

**TI, the tail-calling interpreter, measured 2026-11-26** (user approval;
gcc 15 or clang required): each generated arm is its own function and
dispatch is a guaranteed tail call through the table, so each arm gets its
own register allocation in place of one computed-goto function with a
6,920-byte frame that spilled hot values.  On the all-29 timing (CPU 15,
JIT off) the prototype (`954c8715770`) runs **1.114 of golden87's time**
against the computed-goto evaluator's 1.191; spectral_norm 1.42 to 0.98,
unpack_sequence 1.85 to 1.54, raytrace 1.28 to 1.08; coroutines lost
(1.02 to 1.08).  Clang was no better.  The landing deletes the
computed-goto evaluator and makes the observing mode a second table of
tail functions.

Running:
- TI's landing on rir90 with OB.
- JX's fix is being merged: SP's `edb30efc8e3` left `GetItem.ListSlice`
  planned for the JIT with no generic stub to miss to; every arm that can
  miss is now unplanned when its family's generic is split, and the
  generator refuses a planned miss with no generic stub.

**Closed: regions89, 2026-11-24**
([record](issues/register-semantics59/regions89.md)): declared operation
contracts (V8-style declarations, verified, replacing inference) and shared
stubs with typed completion for the baseline JIT, after Astra's review of
jit88's handlers. Merged into `pyjit/regions89` at `f91d4f80af3`: DV's signed
division templates; G0a and G0a2, the declared-contract notation and a
verifier that fails closed; TA, inline templates tightened from declared
facts; G0a3 and G0a4, the vocabulary the seed needs; G1a, whole-word
completion stubs with 23-byte sites; and G0b's seed, 195 of 351 uops
declared with unchanged generated output. Timed at `a5502da133f` against
golden87 with its JIT off, `on` runs 1.022 of the warm time (jit88: 1.060);
nbody 0.748, spectral_norm 0.792, unpack_sequence 1.380, richards 1.157.
Merged since, at `e0cefcf2df6` (pushed): G0c's eight review batches, G0a5,
G0c9, the four G0d units and G3a's declarations. All 373 uops are
declared, including those of IR-only handlers, and each generated
difference is listed. The inference is deleted, and the verifier fails
closed. G0d1 fixed a settrace regression: 31 `f_lineno` jumps under
`always`, from a wrong frame-push row. The full suites at `d344a3dcd5a`
(release and assert, `always`) and the smoke set at `e0cefcf2df6` show
known failures only.

Closed at `e571a8717e6` (pushed) with G3a's compiled frame transfers:
- execution pins;
- `RESUME_CHECK` as a template;
- calls and returns between compiled frames through the switch routine in
  one native activation (fib(18) under `always`: 16,687 driver turns
  before, 0 after).

The full suites show known failures, plus an intermittent `test_cprofile`
failure. That failure is an upstream `_lsprof` bug: an enabled profiler
finalized by the cyclic GC clears the running thread's `sys.setprofile`
function. Rir90's PF unit fixes it.

Timing is approximate since 2026-11-24, by the user's choice. Against
golden87 with its JIT off, the cold / warm geomean is:
- `always`: 1.070 / 1.048;
- `on` at warm-up 32: 1.051 / 1.025.

G1a's gated figures were 1.038 / 1.030 and 1.024 / 1.022. The `always` run
ran with busy neighbouring cores, so the difference needs a gated run before
any reading (`E59/regions89/bench/g3a-summary.txt`).

G1c, G2, G4 and G6 were held and are replaced by rir90's families. G0d1
fixed two crashes from plain Python, each in golden86 and golden87:
- a specialised in-place str add (`y = s + t`) aborted when `y` held an
  inline number (`cbf27b04cb2`);
- `yield from` a subiterator whose `throw()` raises a `StopIteration`
  made without `__init__` read its NULL value (`64daed03258`; upstream has
  the same bug).

**Jit88, qualified 2026-11-23** at `ea4d023c2b4`
([record](issues/register-semantics59/jit88.md)): the baseline method JIT
with B1 (every word a call of its function form), B2 (inline templates for
74 handlers from the leaf IR), unwind info for units and `ir_evaluate` as
the cold tier. Timed against golden87 with its JIT off, it runs 1.060 of
the warm time and 1.065 of the cold time under `on` (B1: 1.224), at 1.042
of the instructions; nbody 0.96, richards 1.19, unpack_sequence 1.42. The
full suites with `PYTHON_JIT=always` (release and the assert build) and
with the JIT off show known failures only.

**Golden86, 2026-11-22**, now a frozen control: `3cac420ddbf` (`pyjit/round86-golden`,
tag `pyjit-golden-20261122-tight86`, pushed) is
[tight86](issues/register-semantics59/tight86.md)'s IP1: golden85 plus eight
units (the handler skeleton, generator life, Python call and return under one
frame slot contract, native calls, the C-to-Python entry, compare and
membership, the json and pickle cases, and instance attributes). It runs
**0.970 of golden85's time and 0.877 of upstream's** on the timed all-29
geomean and executes 0.969 of golden85's instructions. `ir_evaluate` is
88.3 KB and the allocator product 43.77M. The full suites show known failures
only. Still slower than upstream: comprehensions 1.107, deepcopy_reduce 1.051,
deepcopy 1.049, nqueens 1.041, json_dumps 1.017, pickle 1.017 and
logging_simple 1.015. nbody's 1.016 of golden85 is branch prediction of
unchanged code at new addresses; the user chose no layout rule. Golden85
(`2a9ea8c4f69`, 0.900 of upstream's time) is a frozen control.

The checkpoints between golden73 and gap82, each measured against golden73:

| Checkpoint | Adds | vs golden73 | vs upstream |
| --- | --- | ---: | ---: |
| Round 2 (`f78c3902c1b`) | [handlers75](issues/register-semantics59/handlers75.md): cold slow paths, word flags instead of handler variants | 1.0088 | 1.1383 |
| [76](issues/register-semantics59/checkpoint76.md) (`013fb42c922`) | Round 3 call path ([ledger](issues/register-semantics59/execution76-call-ledger.md)), value rules first pass, frame-chain fix | 1.0067 | 1.1292 |
| [77](issues/register-semantics59/checkpoint77.md) (`3c8abb64776`) | Value rules' remaining rows; borrowing native scope | 0.9915 | 1.1167 |
| [78](issues/register-semantics59/checkpoint78.md) (`99a04733128`) | Value fields paired with object slots; cold out-of-line adapters | 0.9856 | 1.1123 |

What these rounds established:

- **Placement dominates single workloads.** Op-cache placement moves single
  workloads by ±10–15% at a constant instruction count. Round 3 alone runs
  DeltaBlue in 14% fewer cycles than round 2. Reserving `rbp` for the frame
  chain adds 0.5% instructions yet halves op-cache delivery
  ([checkpoint76](issues/register-semantics59/checkpoint76.md#deltablue-placement-not-work)).
  Until hot-code layout is controlled, the geomean is the only reliable
  signal, and per-change attribution needs cycle and op-cache counts.
- **The hot footprint** is about 1.6 times upstream's. 60% of it is runtime
  helpers rather than handlers.
- **Exact calls** execute 1.27–1.36 times upstream's instructions in fewer
  cycles. The remaining call losses are the slow and generic paths. 3,568
  generic calls per DeltaBlue loop are `list.append`/`list.pop` on a list
  subclass; upstream misses them too.
- **The value rules** ([record](issues/register-semantics59/values77.md))
  apply to every value-form function:
  - callers own and callees borrow;
  - public entries validate and internal entries assert;
  - a value's pointer space is tested once.

  An adversarial review found no use-after-free in about 5,400 cases.
- **The instruction gap**
  ([execution79](issues/register-semantics59/execution79-instruction-gap.md)):
  checkpoint78 retires 1.09–1.30 times upstream's instructions. Calls are
  the largest category in every kernel. Two causes cut across all handlers:
  GCC's handler calling convention moves the interpreter state between
  registers in every handler that contains a call (7–9% of upstream's
  instructions in each kernel), and the register operand and result protocol
  makes a specialized handler cost what upstream's whole stack sequence
  costs. Instruction work comes before hot-code layout; instruction counts
  attribute each step independently of placement.
- **Operand-encoded calls**
  ([proposal](design/proposals/operand-calls80.md), 2026-11-19): calls name
  their inputs in trailing operand words instead of a copied window, and the
  child frame starts at the arena top. Estimated, after the handler fix:
  `f(x, y)` from 430 to about 336 instructions (upstream 343); kernels 0.35–0.8%
  fewer instructions, coroutines 5.4%, deepcopy_memo 5%; constant slots
  another 0.3–1.8%. The user chose the child at the arena top, kept the
  snapshot rule and declined borrowed arguments.
- **Handler shape.** Call-free handler bodies under GCC's tail-call convention
  (handlers80, parked) saved only 0.4–2.4% of kernel instructions and grew hot
  handler text by 15%. Our handlers are fat because the generator inlines
  per-execution protocols (operand-kind decoding, publication, re-entrant
  release, native bridge records) that upstream avoids by plain ordering and
  helpers. The [handlers81 proposal](design/proposals/handlers81.md) writes
  every handler in upstream's order (guards, effect, detach, store, release,
  dispatch), deletes those protocols, and compiles the handlers into one
  computed-goto function. A compile probe of 336 labels kept a 40-byte frame,
  no canary and one dispatch jump per site. Estimated −8 to −17% of
  upstream's instructions per kernel before calls80. **Landed**
  (approved 2026-11-19, no performance gate): H0–H4 on
  `pyjit/handlers81`, and calls80 as H5 (`d6d1ed5ac93`). Kernels run
  7.9–10.4% fewer instructions than checkpoint78; against upstream,
  richards runs 0.4% fewer, raytrace 4.5% more, and deltablue, coroutines
  and deepcopy_memo 16–19% more. Operand calls and constant slots cost
  0.5–6.4% more instructions than step 1: after H4 an argument load costs
  about what its operand fill costs. GCC's register-allocator limit bounds
  the evaluator (41.9M of 50M). Timed all-29 geomeans: 0.958 of
  checkpoint78's time, 0.944 of golden73's and 1.065 of upstream's
  (checkpoint78: 1.112). logging_silent is 31% slower than checkpoint78,
  from H4's front-end stalls ([record](issues/register-semantics59/handlers81.md)). The
  [plan](issues/interpreter-performance56/plan.md#previous-rollout-handlers81-then-calls80-landed)
  lists the steps.
- **Gap82** ([record](issues/register-semantics59/gap82.md), approved
  2026-11-20): H5's remaining gap to upstream sits mostly outside the
  evaluator. Twelve library-heavy cases (xml_etree, json, pickle, deepcopy,
  logging_format and logging_simple) hold more log-excess than the whole
  geomean gap; generators and iteration hold about half as much.
  **Integrated** as `pyjit/gap82` at `5194e957c5d`: two analyses and ten
  implementation units. H6 (layout) was parked. Selected as golden82 on
  2026-11-20.
  - Full suites fail exactly H5's counts.
  - Timed all-29 geomeans: 0.992 of upstream's time (H5: 1.065), 0.928 of
    H5's, 0.884 of golden73's and 0.899 of checkpoint78's.
  - Ten cases beat upstream. Generators (1.34), deepcopy (1.20–1.22),
    logging_format, logging_simple and three xml cases (1.10–1.14) remain
    furthest above it.
  - Instructions are 0.912× H5's, and `-c pass` startup runs 63.6M
    instructions against H5's 530M.
  - Code placement now limits small hot sets: logging_silent is 1.15× H5
    and 1.48× checkpoint78.
- **A complete value-slot table**, kept in step with every object-slot write,
  was implemented and reverted: it falls out of step in four ways, two of
  them crashes, and saved no instructions. A value field is now an optional
  native pair of the object slot, which stays the definition.

Execution-boundary73 and its bounded RETURN isolation correction are implemented,
protocol-qualified and measured. On 2026-11-16 the user selected `44f066f5fd5`
as golden73, explicitly accepting the logging regression. Sol 6.1 workers
implemented and cross-reviewed; root
integrated, generated, qualified and measured both candidates.

The complete generator/coroutine path handles yield, resume, return, real HOME
results, direct consumers, observers and callback retirement. A guarded shared
close avoids effect packaging for callback-free completed receivers. The final
correction moves generator routing out of shared RETURN bodies into one private
tail entry. Public IR and frame layout stay unchanged. The native RETURN/EMPTY
bodies shrink from boundary10's 914/1114 bytes to 509/683, below selected57's
596/780; the private route and its cost are counted separately.

Both final builds pass 117 modules/79 shared extensions. The 24-module protocol
passes 803 release and 792 debug methods, eight skips each; debug excludes 11
host parser checks. Six additional retirement methods, eight owner controls,
injected rooted recursion failure and eight native cost controls pass. Arithmetic
and exact-call controls match selected57's counts; unique generator completion
is 722 versus 769, coroutine 700 versus 721, and two-await HOME 2055 versus 2099.
Retained completion costs 11 more instructions and yield ten more. These are
reached-path costs, not throughput predictions or full-system qualification.

The final two-pair hot8 screen has 32 successful rows and stable identities.
Candidate/selected57 GM is **0.985010**. Coroutines improve 8.70%, generators 6.73%,
comprehensions 3.49%, raytrace 2.61%, spectral_norm 2.18% and DeltaBlue 0.87%;
nqueens regresses 0.40%. **Logging regresses 13.73%**, failing protection in both
pairs. Independent review recorded NO-GO under the original protection rules;
the user subsequently accepted this specific tradeoff. The sealed verdict stays
unchanged. Winner confirmation, all29, full suite and a fresh upstream comparison
have not run for this checkpoint; broader qualification remains follow-up work.

The earlier boundary10 candidate `dee619e80ab` is separately preserved, including
its ELF copies: GM 0.962227, but logging +10.44% and raytrace +1.97%. Its natural
snapshots retain all 659/597 warmed logging/raytrace specializations. Its bounded
PMU diagnosis finds logging cycles +10.47% with instructions +0.15%. Those
counters belong to boundary10, not the final return-isolated executable.
Outlining RETURN did not solve logging; neither smaller handlers nor the two
builds establish a hardware cause. Do not combine their best benchmark results.

The complete fast paths remain preserved. The
[trial report](issues/register-semantics59/report.md) owns both results,
qualification and causal limits; the [plan](issues/interpreter-performance56/plan.md)
records the remaining common call/return cycle-cost problem. The older
[diagnosis](issues/register-semantics59/execution73-diagnosis.md) concerns frozen72
and does not reopen its selection decision.

The current stage is [interpreter performance](issues/interpreter-performance56/plan.md).
The active runtime is golden85 at
`2a9ea8c4f69`, branch `pyjit/round85-golden`, tag
`pyjit-golden-20261122-hot85`, pushed to origin. Golden84
`4be2eb5d31f3e06e39d158740b46666a3b8c2810` (`pyjit/round84-golden`, tag
`pyjit-golden-20261121-parity84`), golden82
`5194e957c5d573ef2b6112508a7b0990b7de656d` (`pyjit/round82-golden`), golden73
`44f066f5fd55947edf709bbcf9bf17c607307e4b` (branch `pyjit/round73-golden`, tag
`pyjit-golden-20261116-generator-boundaries`) and selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0` and their refs remain immutable controls.

Execution72 is complete and unselected. Sol 6.1 workers implemented and
cross-reviewed the compiler/validation and runtime/generator changes; root
integrated, generated, built, tested and measured them. The separate cold-layout
unit passed 1,143 related methods per role but lost its screen. It demonstrated
no frame-packing gain and was excluded from the ABI candidate.

The carried-owner ABI is preserved at `c6d54fd3b5d` on
`pyjit/execution72-abi-integrated`. Both final builds pass 117 modules/79 shared
extensions. Across the affected integrations, 768 methods in 19 related modules
pass per role (12 skips), plus eight discarded-result completion methods. Final
coroutine qualification passes all 100 methods per role. Six complete native
paths, six owner-placement probes and an injected native-entry failure pass.
Cold review finds no further structural blocker, while retaining actual branch
coverage and full-suite limits.

The unchanged two-pair hot8 screen rejects selection: **1.067106 GM** versus
selected57. Comprehensions improves 2.7% and spectral_norm 1.2%; DeltaBlue loses
6.8%, coroutines 1.8%, generators 1.6%, logging 21.1%, raytrace 29.0% and nqueens
1.4%. All 32 rows succeed with stable identities. No confirmation, all29, full
suite, fresh upstream measurement or repair follows for this loser.

The ABI retains the 112B frame header, existing reserved homes and initialization.
It adds an 8B executable-header pointer and 2B/word map. Complete native arithmetic
and exact-call paths improve slightly; generic calls and generator completion
add work. Thus carrying a value is demonstrated, but the intended common-case
simplification is not. The [trial report](issues/register-semantics59/report.md)
owns exact results, costs and follow-up constraints; the
[design](design/proposals/execution-state72.md) and
[source contract](issues/register-semantics59/execution72-abi.md) retain the
attempted architecture. Earlier rejected units remain closed. The authorized73
review must price a complete execution boundary and preserve specialization
coverage before broader implementation.

The register interpreter and canonical eight-byte PyValues are implemented.
Reference counting with cyclic collection is selected; extensions are rebuilt
and ported. Native method and tracing JIT tiers remain unfinished. The selected
runtime uses one owning frame stack, explicit recipients and continuations,
overlapping argument homes, and an initialized frontier. Source positions do
not prove ownership or initialization.

This checkout is the canonical documentation authority. Its C source is stale;
do not build it. Use [handoff](HANDOFF.md) for runtime paths and evidence, and
[process](PROCESS.md) for contribution steps.

## Selected57 performance and qualification

The selected change unifies dictionary lookup and result acquisition below
public validation and typed `dict.get`. Arbitrary callbacks retain dictionary
and query pins; results are acquired before input cleanup. Cached exact-string
shortcuts require a Unicode-only table under the GIL because mixed-table
equality can run Python. Both `GetValue` and `ContainsValue` use that rule.

The observed all-29 means are **0.995586/golden56** (16 wins) and
**1.127137/upstream** (six wins). These are two-pair aggregates, not statistically
established speedups; upstream XML iterparse pairs have large opposing outliers.
There is no fresh upstream performance claim for the current checkpoint.

In separate six-pair confirmation versus golden56, comprehensions **0.997484**, DeltaBlue
**1.000239** and logging **0.992049** are near flat with mixed pairs. Richards
**0.991167**, spectral_norm **0.931732** and nqueens **0.987729** improve in
every pair. Root accepts coroutines **1.019635** and raytrace **1.020117** losses
in every pair. Generators **1.024797** remains debt, with a wide
**0.992243–1.119177** range. Matched upstream counters show 36.14% extra native
instructions in DeltaBlue, 43.64% in comprehensions and 30.78% in coroutines;
these counts do not identify a cause or predict throughput.

Both final selected57 builds complete 117 modules and 79 shared extensions;
all 203 methods across three dictionary modules pass per role. The 28 focused
reader methods overlap that coverage. Selected56 supplies the inherited coherent
full-system checkpoint; no fresh full 55k run is claimed for this successor.
Qualification is GIL-only. Six debugger-jump errors remain in the exact
golden54 baseline; GDB PrettyPrintTests remain quarantined. No free-threaded or
arbitrary third-party extension qualification is claimed. The
[selected57 report](issues/comprehensions57/report.md) and
[benchmark tables](issues/comprehensions57/benchmark-tables.md) retain details.

## Round67 closure

Both independent candidates qualified their mechanisms and lost their balanced
screens. A's stable 32-row screen rejects selection: hot8 geometric mean
**1.026137293**, logging **1.210653142**, DeltaBlue **1.022084868** and
spectral_norm **1.053212755**. B's anchor4 builds pass 117/79 and eight focused
methods per role; natural and promotion receipts and two fallback controls
pass. Static native review and an independent comparison pass, but those
structural results do not establish performance cause.

B's stable-identity 32-row screen rejects selection. Its geometric mean is
**1.114220219841174**, and every design67 screen predicate is false:

| Workload | Anchor4 / selected57 |
| --- | ---: |
| comprehensions | 1.0208926125877458 |
| deltablue | 1.141349549410481 |
| coroutines | 1.0279987865402465 |
| generators | 1.0325652873804605 |
| logging_silent | 1.4303733131817118 |
| raytrace | 1.0613165681229204 |
| spectral_norm | 1.149823408700587 |
| nqueens | 1.1003578304681976 |

Root closes round67 without confirmation, all29, PMU, full suite or repair.
Independent screen-peer readback confirms rejection. A and B are rejected;
selected57 remains unchanged. The full account, including earlier failures and
native evidence, is in the [trial report](issues/register-semantics59/report.md).

## Direction and evidence

The [goal](GOAL.md) and [active plan](issues/interpreter-performance56/plan.md)
define the objective and current increment. The [runtime architecture](design/runtime-architecture.md),
[register IR](design/register-ir.md), [interpreter](design/interpreter.md),
[calls and frames](design/call-and-frame-lifetimes.md), and
[activation transfers](design/activation-transfers.md) describe selected
contracts. The [roadmap](ROADMAP.md) records stage outcomes.

The [selected56 report](issues/interpreter-performance56/report.md),
[model55 ledger](issues/call-model55/implementation.md), and linked trial
reports retain historical experiments and unfinished independent gates. They
are evidence, not an automatic dependency list for round68.
Bitmap execution readers, IRSC2 exceptional transfers and full static scratch
authority (M4) remain unfinished independent gates, outside selected57.
