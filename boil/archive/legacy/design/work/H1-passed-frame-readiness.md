# H1 passed-frame experiment: source readiness after H3 composition

Source-only follow-up to H1-1.md, 2026-10-29. No experiment patch, configure,
compiler invocation, target test or benchmark was run. Inspected stats source
19f4b2631d185b806d9b7c6a943289b11036d6cf, original H3 81417aff097,
ROOTS successor c67954f336a and its existing ce2ac tail build configuration.
The actual integration union remains the required implementation baseline.
This does not authorize H1 or change the qualified compiler/build policy.

## Smallest coherent source change

Pass the existing mutable frame alongside slots/tstate/next_instr/ir_word.
Use the same parameter order everywhere, for example slots,tstate,frame,
next_instr,ir_word[,lastopcode]. No runtime convention switch is needed.

- Python/ceval_macros.h: add frame to both TAIL_CALL_PARAMS/ARGS forms and the
  explicit stats/nonstats JUMP_TO_PREDICTED argument lists. Function-pointer
  typedef and TARGET signatures already consume that macro. Remove
  TAIL_CALL_LOAD_FRAME and all its generated uses once frame is a parameter;
  leaving per-entry reloads would hide stale forwarding and erase the change.
- Python/ceval.c: pass the actual current frame at the two explicit initial
  _TAIL_CALL_error/start_frame calls, in both stats forms. Keep entry-frame
  linkage, throw setup, lastopcode=0 and position publication unchanged.
- Tools/cases_generator/ir_generator.py: remove the handler-local
  frame=tstate->current_frame declaration in generate_handler, label reload in
  generate_ir_labels and UNKNOWN_OPCODE reload in generate_ir_targets. All
  use the passed parameter. Keep raw counter names, semantic opcode assignment,
  saved instr_ptr, operand/feedback layout and predicted counter placement.
- Regenerate Python/generated_ir_cases.c.h and Python/ir_targets.h from those
  handwritten changes; metadata/IDs should be byte-unchanged. Add focused
  generator interface assertions and actual transition workloads in the
  existing generated-cases/IR tests, preserving ROOTS checker classes.

That is three handwritten native/generator files, two generated products and
focused tests. No bytecodes.c transition rewrite is needed for the positive
change: DISPATCH_INLINED already assigns both frame and tstate->current_frame;
return, yield and exception unwind do the same before redispatch. Initial
start_frame also has a valid frame even though slots/next_instr start NULL.
The caller's current frame must be passed after every transition, including
labels and UNKNOWN_OPCODE. Do not reintroduce a tstate reload as a repair.

The existing debug PRE_DISPATCH_GOTO equality check remains. A first-entry
identity check under the existing debug assertion policy should compare the
passed frame with tstate->current_frame before any handler/label use; it must
not become a new release hot-path operation or a permanent runtime option.
Canonical guards/value ingress, FP policy, root clearing, lastopcode lifetime,
H2 maps and semantic counters are outside this experiment.

## Matched build reuse and what one build can establish

The inspected ROOTS build/tailcall configuration is CC=clang-21 with
--with-tail-call-interp, O3/NDEBUG, retained frame pointers and zero-init;
Py_DEBUG and Py_STATS are undefined, _Py_TAIL_CALL_INTERP=1. Its config hashes
are Makefile075fccfbfb80dba0677a4f4c6b0d30fc2b655cedb0ffa30d43221c7aad30aa6b
and pyconfig5cdd2951307367fce05478bf20dd29efad092227c0e92e3ff9c1208f97574a3f.
The recorded driver is /usr/bin/clang-21 resolving to the llvm-21 binary.
These are configuration/record reads, not a new version/compiler probe.

Those ce2ac products are not an H3-union control. Reuse their configuration
and tool identity, not their runtime qualification as if built from H3.
If integration supplies an actual-union Clang21 tail nonstats release baseline
with the same policy, preserve it and reuse it as control; then one owned
experimental build establishes the first positive semantic/compiled-code
checkpoint. Do not touch the original ROOTS/H3 builds. If no such exact union
baseline exists, a fresh control build is also necessary: a GCC non-tail build
cannot be the tailcall experiment's matched comparator.

Keep compiler/ISA/O3/FP/zero-init/PGO/LTO/stats policy identical between the
actual-union control and passed-frame variant. Do not combine frame-pointer
removal or Clang-vs-GCC tuning with passing frame. The current GCC Py_STATS
build proves counter storage only; it does not exercise preserve_none,
musttail, new signature register allocation or tailcall root ownership.
Nonstats Clang evidence also does not qualify the extra stats lastopcode
argument; source/generator checks cover that signature until a real matching
stats-tail build is separately granted.

## First falsifiable evidence, before a performance decision

Run a short semantic/identity pilot on the actual tailcall convention:
inlined call/return, nested C-to-Python eval activation, generator yield/resume,
coroutine resume, exception unwind/throw, monitoring redispatch and predicted
deopt. Results must depend on the current caller's locals/code and collecting
monitoring callbacks. Preserve existing slot anchors and initialized spans.

Use a task-owned copied omission at a real transition/forwarding site. Its
first-entry identity or semantic oracle must fail specifically for the stale
frame; compilation failure, unrelated crash and timeout are not credit. The
positive must have that same oracle. A debug-policy build or explicitly
reviewed copied assertion-enabled native control is needed for an identity
oracle compiled out by release NDEBUG. Such copied negative/control variants
need bounded recompilation; they are not magically established by one release
build or by the earlier primitive stats fixture. Do not change the timed
cohort's assertion policy to obtain a favorable comparison.

Inspect actual Clang-produced arithmetic and call/return paths to show the
per-entry tstate frame reload disappeared and quantify any new spills/extra
moves. Retain full binaries and literal disassembly/source context. ROOTS must
supply current C-checker and actual compiled-stack ownership controls for
precise preserve_none handlers; passing frame creates no new object-root
permission. Check CFI/root-relevant DWARF and live attribution at collection,
not just zero fallback. No broad repeat matrix follows from this readiness.

Only after semantic/root proof and an explicit quiet reservation compare
matched release elapsed/instructions on the declared workloads, retaining
regressions. The current 0.828243 diagnostic and slow coroutine/generator rows
motivate investigation; they do not prove frame reloads caused those costs.
