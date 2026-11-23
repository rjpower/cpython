# PyJIT goal

Build an optimized Python runtime with a baseline method JIT and a tracing JIT
on canonical eight-byte PyValues and a register IR. Rebuild and port
extensions. Preserve Python behavior, explicit ownership and useful
observation APIs.

Selected runtime: golden87, `684a98eb30e` (`pyjit/round87-golden`, tag
`pyjit-golden-20261123-contract87`), selected on 2026-11-23. It adds
[contract87](issues/register-semantics59/contract87.md)'s ten units to
golden86: the operation contract, a function form for every handler, and
stack bytecode in `co_code` and `.pyc`. It runs 0.876 of upstream's time on
the timed all-29 geomean (golden86 0.877). Comprehensions, nqueens, pickle,
deepcopy, unpack_sequence and logging_simple remain slower than upstream.

## Next steps

The [tiers87 plan](design/proposals/tiers87.md) sets three phases:

1. **Contract87, the interpreter's last wave** (done: golden87). The cases generator exports
   one operation contract: each handler as uops with operands, ownership,
   effects, guard facts, typed transfers and a feedback schema, and every
   handler with a function form. The other units improve the shared runtime
   that compiled code also calls: native calls, raise and catch, attribute
   misses and the frame's exec pointer. Stack bytecode recovery lands
   alongside. Interpreter tuning that a compiler removes (dispatch, decode,
   operand views, layout) stops.
2. **A baseline method JIT, Sparkplug style** (active:
   [jit88](issues/register-semantics59/jit88.md)). It compiles a warm code
   object's IR words to x86-64 with interpreter frames unchanged. Each word
   becomes a generated inline template or a call to its handler's function
   form; every word boundary is an interpreter state, so it needs no
   deoptimization.
3. **A tracing JIT.** It records hot loops in the interpreter, expands each
   handler into its uops, optimizes in SSA from the declared guard facts and
   effects, and exits through frame homes to baseline code or the
   interpreter. It shares the baseline's assembler and code cache. An
   optimizing method tier waits for evidence that traces leave a gap.

`Python/bytecodes.c` and its generator are the one statement of what an
operation does. The interpreter, the baseline's helpers and templates, and
the tracer's uop expansion are generated from it. Hand-written machine
lowering is limited to a closed set of value primitives, each tested against
its C definition.

## Standing decisions

- One canonical value format spans frames, containers, attributes and rebuilt
  extensions. Owning bridges support incremental ports.
- Reference counting with cyclic collection is selected. Frame storage, calls,
  results, unwind and observation share one ownership model.
- Rebuild source and binary interfaces as needed. Hard extension compatibility
  is not an admission gate; Python value/control behavior and owners are.
- Semantics that are incidental to upstream's implementation, such as
  finalization timing or when a tracing switch takes effect, may change where
  that simplifies the design. Language behavior and ownership may not.
- Tighter hot code comes before layout rules; no placement tuning.
- Compare fixed workloads and immutable controls using one GCC configuration
  and fixed affinity and hash. Interpreter comparisons run with the JIT
  disabled; JIT measurements report cold and warm time separately. Startup
  RSS, zero-box counts and exact CPython destruction or display are not
  throughput gates.
- Workers implement bounded units; the coordinator owns integration,
  qualification, measurement and selection. Correctness gates each step;
  performance is recorded at integration points and selection is the
  user's.

## Controls and records

Golden87 is the runtime baseline. Golden86, golden85, golden84, golden82,
golden73 and selected57 remain immutable comparison controls; their evidence is preserved
without making it a prerequisite. [Status](STATUS.md) records the selected
runtime's measurements and limits; the [active plan](issues/interpreter-performance56/plan.md)
and each wave's record hold the work. The [trial report](issues/register-semantics59/report.md)
keeps the history of earlier selections and rejected rounds.

The [roadmap](ROADMAP.md) describes stage outcomes; the
[architecture](design/runtime-architecture.md) and linked designs define the
runtime contracts. Follow [process](PROCESS.md) and
[validation](design/validation.md) for changes.
