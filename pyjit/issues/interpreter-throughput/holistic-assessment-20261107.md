# Interpreter and IR assessment at runtime `7780e32e0e2`

2026-11-07. Independent, read-only source review of the clean runtime and the
final throughput report. No new build or measurement. This assesses the
interpreter and compiler together, including code cleanup. It does not claim
to reproduce any particular engineer's opinion.

## Judgment

The design has substantial correctness work behind it: a compiler-produced
register CFG, a generated handler/operand table, explicit owned frame homes,
contiguous call windows, verifier checks, generated tail-call handlers, and
reviewed publication before reentrant release. The exact and common positional
call transfer, known-NULL initialization, and five attribute publication
deduplications are already integrated. Ordinary thread frame retirement already
clears only locals (`Python/frame.c:_PyFrame_ClearExceptCode`). Recommending
those as missing work would be wrong.

The code is still a safe, relatively heavy baseline interpreter rather than a
lean execution engine. The 29-kernel candidate/upstream elapsed factor is
1.263136 and is noisy; it does not prove parity. In three selected hot regions,
instructions per workload loop remain 2.00x, 1.77x, and 1.61x upstream, with no
matched dynamic IR-word/bytecode denominator. Profiles put notable samples in
call preparation, frame setup, attribute helpers, and cleanup. Those observations
justify focused source experiments, not a claim that any single helper causes
the whole gap. [Final evidence](../../bench/results/interpreter-throughput-20261106.md).

The earlier [holistic design review](holistic-design-review.md) and cold code
reviews examined ownership, frames, and publication carefully. They did not
complete a whole-system code simplification audit. This review finds one small
unambiguous cleanup and several architectural seams that deserve measured,
bounded work.

## Ranked actions

1. **Account for the remaining call work before changing it.**
   `Python/ceval.c:call_transaction_push_python` first tries the integrated
   overlap path, then copies a caller window into a consuming provider; its
   owner duplication is required by that provider's reentrant failure
   contract. `call_transaction_invoke` also handles method expansion,
   monitoring, native adapters, and decline. The final logging profile gives
   `call_transaction_push_python` 16.18% physical-symbol self share, while the
   annotated common route includes frame preparation, guards, tail clearing,
   and empty-varargs handling. Count executed call shapes and accepted/declined
   adapters in the same hot region, then test one guarded common-route or
   slow-provider outlining change. Preserve prepare/guard/commit, event-once
   behavior, and ownership; the prologue alone is not a proven bottleneck.

2. **Make one useful phase/output contract shared by the generator and IR
   consumers.** `Include/internal/pycore_ir.h:_PyIRFormat` and
   `Tools/cases_generator/ir_generator.py:handler_effects` provide operand
   roles, extents, and conservative word-level effects. The header explicitly
   says heap effects, guards, and result facts are absent. Yet
   `IRStack.published_temporary_result` and `retire_published` know a special
   five-name attribute postcondition, while `Python/regcfg_passes.c` uses
   coarser effect tests. This is a real abstraction gap, not evidence that the
   five-way optimization is wrong. Start with one attribute family: describe
   consume, publication, possible release/callback, refreshed output, and
   success/error states once, and make the generator plus one analyzer or
   verifier consume it. Do not invent a second hand-maintained opcode table or
   treat future-read liveness as ownership. The broad contract listed in
   `pyjit/design/register-ir.md` is partly aspirational at this HEAD.

3. **Reduce executed native work at a measured handler/helper boundary.**
   The generated `BINARY_OP_ADD` in `Python/generated_ir_cases.c.h` shows
   operand views, possible specialization, consume/take, a value operation,
   publication/refresh, destination store, and displaced-owner retirement.
   Those steps protect observable Python semantics, but the emitted handler
   makes the per-word cost concrete. `Python/ceval.c:value_reader_complete`,
   `retire_published_values`, and `finish_displaced_local` demonstrate why
   helper calls and state refresh recur. Pick one hot shape or attribute path,
   inspect GCC output and same-workload counts, and remove only a redundant
   phase proven by its producer and consumer facts. The final profile points
   to shape attribute load (10.73% self in deltablue) and method attribute load
   (6.32% in logging) as candidates; these are sample shares, not isolated
   savings. A broad dispatch rewrite is not yet supported by that evidence.

4. **Delete misleading call bookkeeping in a separate small patch.**
   `_PyCallTransaction.effects` in `Python/ceval.c` is initialized and set by
   preparation, monitoring, and generic provider paths, but has no reader at
   this HEAD. `CALL_EFFECTS` still has generated handler uses and must be
   assessed separately. Removing the record field and its writes would reduce
   false protocol surface without implying a speedup. Review the generated
   output and focused call paths after the edit. Broader style cleanup of the
   call transaction or generator should wait for a proven simplification.

5. **Before resuming method/tracing JIT work, settle recovery as an interface.**
   The interpreter's physical homes, compiler liveness, and generated effects
   are useful inputs, but they are not typed SSA or deoptimization state.
   `pyjit/design/jit-e2e-system.md` specifies `Action`, `FrameState`, effects,
   and snapshots; current runtime source has no corresponding shared JIT
   implementation. Define how a guard or callback recovers owned homes,
   exception state, PC, and already-completed side effects. A narrow baseline
   builder can test that interface before optimizing methods and traces share
   it. This is a design dependency, not a prerequisite to the call/attribute
   throughput packets.

## LuaJIT-inspired critique, with the comparison kept straight

LuaJIT keeps its [baseline bytecode](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_bc.h)
and [typed optimization IR](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_ir.h)
distinct, and its [snapshot code](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_snap.c)
exists to reconstruct interpreter state after exits. The fair comparison is
this project's 64-bit baseline words against LuaJIT's baseline bytecode, then
this project's planned typed SSA against LuaJIT's optimization IR. A 64-bit
baseline word can be a sound engineering choice, but it does not itself buy
fewer executed instructions or recovery-ready state. The useful critical
questions are: how much native work does one common operation actually emit;
which helper effects are proved rather than assumed; which observer-visible
state must be materialized at each exit; and what work can disappear without
changing finalization or monitoring order? Current evidence answers parts of
the first two and leaves the JIT recovery question open. This is an engineering
lens, not an attributed statement from Mike Pall.
