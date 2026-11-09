# H1-1 bounded implementation readiness

Verdict: the frame-passing slice is ready for a bounded implementation
assignment after the coordinator grants source ownership and a quiescent
compiler/build handoff. No H2 map or frame-pointer-policy change is required
for that slice. This is source/design clearance, not permission to edit,
landing approval, or evidence of a speedup.

Reviewed H1-1.md from commit `617bea5be1b`, SHA256
`265bf1df1253e1be8c6023e66fb52bfd1060417fa40ad6c893e31739722f1031`.
Its paused status and source line numbers describe its earlier checkpoint;
primary PROCESS §0.4 now explicitly keeps H1 before checkpoint I and allows
frame passing independently of H2. The design was preserved unchanged.
Current dispatch source was read in the H3 worktree at HEAD
`878647973ba08be9f2c8e32e972504b507440b46` plus frozen H3 source manifest
`54b2832334af6148e6852799333361e1e089ffc6fc7642dbec92503ab84084a9`.
No compiler, native interpreter, build, test, probe or benchmark was run
for this review.

The smallest reusable change is one coherent tail-call interface:
`slots, tstate, frame, next_instr, ir_word[, lastopcode]`, with `frame`
being the mutable current frame. Every table entry, handler, generated
label and direct entry must use that interface together. A subset of
handlers cannot acquire an incompatible function-pointer signature.
Remove the tail-call entry reloads; preserve the existing local frame in
the computed-goto/switch evaluator. Do not add a runtime convention switch.
Keep the debug equality assertion, and assert the invariant at tail-call
handler/label entry before dereferencing frame. Direct transfers bypass
PRE_DISPATCH_GOTO, so its outgoing assertion alone is insufficient.

The current closure is finite:

| Owner/source | Required interface work |
|---|---|
| H1: Python/ceval_macros.h:92–138 | TAIL_CALL_PARAMS/ARGS, function-pointer typedef and TARGET share the frame parameter. Both explicit JUMP_TO_PREDICTED lists must pass frame while retaining **this_instr**, rather than next_instr. DISPATCH_GOTO/JUMP_TO_LABEL retain musttail and preserve_none. Remove the reload macro and its generated uses coherently. |
| H1: Python/ceval.c:1288–1301 | All four stats/non-stats start/error calls pass the actual frame. start_frame intentionally receives NULL slots/IP and reconstructs them; adding frame must not change that behavior. |
| H1 after H3 generator handoff: Tools/cases_generator/ir_generator.py:1781,1882,1939,1944 | Remove handler, label and UNKNOWN_OPCODE reload declarations. Keep function_proto's precise-preserve-none section attribute and all generated declarations/table types. UNKNOWN_OPCODE uses frame while reporting the error. |
| H1 generated products/tests | Regenerate Python/generated_ir_cases.c.h and Python/ir_targets.h; update the eight reload-bearing test_generated_cases expectations and add transition controls in the existing IR/monitoring/generator tests as appropriate. No opcode, word layout or metadata-domain change. |
| ROOTS/compiler owner | Supply frozen checker/compiled-walk controls and the exact tail-call build identity; review changed compiled register ownership. H1 does not change checker classes or fixtures to accommodate its output. |

H3 currently owns ir_generator.py, bytecodes.c and generated products.
Implementation must start from an integrated, frozen H3 result or an
explicit coordinated handoff. That is a source-ownership boundary, not an
H2 dependency. Canonical numeric guards and range-spill investigation stay
separate H1 assignments. Call-window placement additionally needs the
H1/H2 alias, liveness and initialized-span contracts.

The caller/callee audit supports passing the existing frame variable:

| Path | Invariant that the implementation must preserve |
|---|---|
| Initial entry / throw entry | ceval.c:1274 publishes frame before either start_frame or error. Throw monitoring may reenter Python; on return the evaluator still owns this frame. Pass that actual frame, not its predecessor or entry shim. |
| Python call / generator send | DISPATCH_INLINED at ceval_macros.h:250 updates frame and tstate together before start_frame. _PUSH_FRAME at bytecodes.c:4237 does the same before reloading slots/IP. All call shapes using these routes inherit the same interface. |
| Return / yield / RETURN_GENERATOR | bytecodes.c:1441,1697,5416 select the caller before further dispatch. Preserve early return-value anchoring before unlink/collection, generator state/exception-chain updates, frame copying/popping and the existing LOAD_IP/RELOAD_STACK order. The forwarded parameter is the caller, never the popped callee. |
| Handled / propagated exception | exception_unwind uses next_instr for CURRENT_OFFSET because RERAISE can change frame->instr_ptr. exit_unwind_notrace:5770 selects the caller, clears/pops the dying frame, reconstructs caller IP and transfers to error. Forward the new frame without replacing that IP protocol. |
| Terminal exit | INTERPRETER_EXIT:1405 and terminal unwind restore tstate to the outer C caller and return directly. They perform no later handler transfer; do not impose a post-restoration frame-equality assertion on these terminal paths. |
| Deoptimization | JUMP_TO_PREDICTED redispatches the same word using this_instr and existing ir_word. IR_DEOPT_BINARY_OP:610 directly calls DISPATCH_GOTO. Both need the current frame even though PRE_DISPATCH_GOTO is not universal. |
| Monitoring / tracing | INSTRUMENTED_LINE and INSTRUMENTED_INSTRUCTION:5508–5550 redispatch with the same frame/word; callbacks may collect, reenter or change f_lineno. Preserve their IP selection, feedback pause and RELOAD_STACK behavior. INSTRUMENTED_JUMP:397 preserves the requested destination even if the callback then raises. |

Ordinary generated handlers still publish frame->instr_ptr
(ir_generator.py:1803). IR_SKIP_RESUME at ceval_macros.h:630 explicitly
publishes the skipped RESUME; IR_JUMP's no_save_ip at bytecodes.c:5678
preserves the originating loop position. These are observer contracts,
including asynchronous inspection, not stores that may be postponed until
collection. Most handlers therefore already depend on frame even when
their semantic operation does not. The first slice needs no new semantic
dependency analyzer. Inspect emitted loads after passing frame, then assign
any remaining dependency-based access reduction separately.

Passing an interpreter-frame pointer does not anchor a PyObject or license
an unrooted PyValue temporary. Keep slot clearing, early stores, initialized
helper spans and precise marks unchanged. pycore_gc_walk.h:30–39 and
gc_walk.c:789–797,943–966 deliberately assign pending registers to
preserve_none handlers differently from ordinary precise functions.
The extra argument can change spills, CFI and register ownership even with
identical Python semantics. Preserve source checking and the final linked
image audit; zero fallback counts alone cannot validate attribution.

Existing retained evidence is useful but does not qualify changed H1 code.
ROOTS' `build/takeover-20261029/settled-walk-tailcall-v3.state.jsonl`
records Clang **21.1.8 (6ubuntu1)**, source `8ce9c57c662`, actual driver
and worker identities, optimization mode zero, off/stress-zero runtime.
The corresponding Makefile has `CC=clang-21`, `-O3`,
`--with-tail-call-interp`, retained frame pointers, and no configured
PGO/LTO option. The v3 walk log reports 29 selected checker/StackWalk
tests, no skips; its result summary records 296 collections, zero fallback
and one intentional unpinned-counter test. The separate stale log reports
six cases at 100/100 and its summary 705 collections, zero fallback/unpinned.
The retained smoke log reports 1,924 tests/50 skips and 10 missing optional
modules. These are bounded existing-configuration results, not full
tail-call 8.2 qualification or all-extension coverage.

The positive ownership test, test_gc.test_tail_call_handlers_precise,
walks a normal CALL and a RAISE error label, checks precise classification
and more than one pending register assigned to each handler. The documented
removed-preserves-nothing flag control demonstrates why that matters; a
fresh changed-convention qualification must retain an effective negative
attribution control and actual compiled CFI/register evidence. Scope that
claim to the qualified x86-64 ELF/glibc walk platform. ROOTS retains earlier
walk-harness failures; v3 is its authoritative replacement.

Meaningful first failing controls should be fixed before broad timing:

1. In a copied experiment, forward a saved callee frame at one actual
   return/yield/unwind transition while tstate selects the caller. The
   next-entry invariant must fail before its position store; independent
   caller-local/code-dependent results catch a missing assertion. Cover
   generator creation, send/throw/close, handled/unhandled exceptions,
   recursive-call failure and collecting callbacks.
2. Exercise monitoring instruction/line redispatch, a f_lineno jump that
   raises, specialized guard failure and predicted same-word entry. Check
   offsets, traceback/locals and result, not merely callback presence.
3. Compile the coherent stats and non-stats interfaces with the supported
   tail-call compiler; verify initial/error, generated label and UNKNOWN
   signatures, preserve_none declarations and actual tail branches. A
   missing frame in either explicit predicted list must be rejected.
4. Run unchanged checker positives/negatives, linked inline audit and
   pending-register ownership controls on the frozen candidate, including
   normal/error handlers and opaque collecting callees. Keep actual-state
   kill/stress witnesses and protected root tests. Use current shared
   launcher semantics rather than a helper that bypasses worker.main.

Before adoption, collect matched release wall time on two frozen source
states in an approved existing tail-call build with identical compiler,
optimization, frame-pointer, PGO/LTO, stats and instrumentation settings.
Inspect representative leaf arithmetic plus calls, yields and error paths
to establish whether the entry load disappeared and whether argument
pressure added moves/spills/code size. Record exact binary/config/source
and compiler identity, stable benchmark inputs, warm-up, repeated timings,
uncertainty and persistent regressions. Include the existing micro/kernels
and frame-heavy calls/generators/coroutines/exceptions, with broader gates
scaled to the final diff. GCC non-tail-call timings cannot establish this
convention's benefit; instruction counts alone cannot select it.

The configure help/default discrepancy and GCC DONT_SLP_VECTORIZE's forced
frame-pointer term are real separate policy work. A retained-frame-pointer
baseline can measure frame passing today once its handoff is approved;
removing that term, adopting O1, or qualifying a new no-frame-pointer matrix
must have its own comparison. ASan retains its prescribed frame pointers.
Historical note-14 6d instruction counts are not current H1 measurements.
An inconclusive or slower result keeps the current convention.

Reviewed source SHA256s: ceval_macros.h
`9e42436849758192d17093baa3491b5c0d51f4216cb074efd6b16521236757fc`;
ceval.c `112150c7a927ac15aa5e882a3bb96b44984e016f918f446b59b0fdbacfb572ce`;
bytecodes.c `0b45b41af826d87ad7bff32b0446a3016f7815d30317cfdb213d8b3eade1f561`;
ir_generator.py `b28932bdcba9e52b30c79ad4716e438129f66b253882a58fb9f44dfe733138a6`.
ROOTS v3 result-summary SHA256:
`956d0024a8fcca3f9892ae3ffa58a1bb8bb032877af80766b70f373ff33bcd37`;
walk-state SHA256:
`3ccc513f320d0575ea9f51dd8358f1fc3d5c06d746494d00b339de4acae342f0`.
The summary binds tailcall/python
`ca05db82136413c9ca3a5164dbb9e0c56d93c6f044590b60894bc902babed00a`,
Makefile `075fccfbfb80dba0677a4f4c6b0d30fc2b655cedb0ffa30d43221c7aad30aa6b`
and pyconfig.h `5cdd2951307367fce05478bf20dd29efad092227c0e92e3ff9c1208f97574a3f`.
