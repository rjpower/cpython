# Golden runtime: independent interpreter and IR review

Reviewed 2026-11-08. Runtime tag `pyjit-golden-20261108-runtime`, commit
`0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae`. In source references below,
`R` means `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`.
Line numbers refer to that frozen tree, not the documentation checkout's older
runtime sources. This is a source review from a V8/interpreter engineering
perspective; it claims no personal V8 employment or access to private evidence.

I read the runtime format, compiler passes, generator, calls and frame code
before reading earlier recommendation documents. I then read the new IR and
interpreter specifications and the retained performance report. There were no
runtime edits, builds, timings or new qualification runs. The applicable
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/)
was read before making recommendations. Implementation requires the project's
design and independent review process; this review approves no implementation.

## Assessment

The register interpreter is real. It directly executes explicit operands,
folds eligible local loads and stores, compacts temporaries, has typed feedback,
specialized handlers, a frame stack, and overlapping Python argument windows.
It should not be described as an upstream stack interpreter with different
names. Nor is it an optimizing SSA IR: joins use canonical stack-position
homes, windows constrain allocation, and source observation constrains motion.
`R/Include/internal/pycore_regcfg.h:14` documents those choices precisely.

The architecture has sound ownership discipline but makes physical execution
carry several logical contracts: instruction boundaries, source events,
ownership retirement, and reconstruction of a stack model for debugger jumps.
That coupling leaves meaningful work even after a handler dispatch disappears.
The highest-value next work is to separate those contracts enough to eliminate
unneeded execution, while keeping their proofs explicit. Another broad call
transaction rewrite or a new word encoding is not yet justified.

Python requires observable locals, correct exception attribution, reentrant
finalizers, monitoring and generator behavior. It does **not** require every
source boundary to occupy an executable word, every child argument to own an
independent count, or every consumed home to be cleared immediately when no
observer can read it. Those are current implementation choices. Replacing any
one requires replacing its consumers' assumptions, not declaring it redundant.

The measurement evidence is consistent with this diagnosis but does not prove
an attribution. Logging retires 1.4349 times upstream's instructions and uses
1.3839 times its cycles per unchanged workload loop. The latest batch reduced
entered handlers from 161 to 151 while retaining 20 frames; ten NOP words remain
and nine result POPs remain. Samples locate work in overlap preparation,
cleanup, calls, returns and lookups; their self shares cannot be added as a
removable-time budget. These are existing results from the
[checkpoint report](../../bench/results/interpreter-parity-20261108.md), not
measurements made for this review. The accepted spectral regression remains a
required screening workload.

## What the V8 and LuaJIT comparisons actually support

Pinned V8 reference: tag **13.0.245**, commit
`4be854bd71ea878a25b236a27afcecffa2e29360`. This deliberately uses a fixed source
revision, not an assertion about today's V8. Ignition's register optimizer
tracks equivalence and materialization and flushes live register state; its
baseline bytecodes are not the optimizing compiler's phi graph.
See [register optimizer, especially Flush](https://github.com/v8/v8/blob/4be854bd71ea878a25b236a27afcecffa2e29360/src/interpreter/bytecode-register-optimizer.cc#L322)
and the separate [bytecode graph builder's loop phis](https://github.com/v8/v8/blob/4be854bd71ea878a25b236a27afcecffa2e29360/src/compiler/bytecode-graph-builder.cc#L788).
This supports keeping a cheap executable baseline and a separate optimizing
representation; it does not support importing global SSA passes into dispatch.

V8 records source positions in a side table, but its bytecode writer still
limits elision when source information conflicts and invalidates elision state
at exception-region boundaries. Metadata therefore helps; it does not erase
the semantic problem. See [source position emission and bounded elision](https://github.com/v8/v8/blob/4be854bd71ea878a25b236a27afcecffa2e29360/src/interpreter/bytecode-array-writer.cc#L167).

Ignition has general register-list calls and small known-arity call handlers;
both use call machinery and collect feedback. That supports specializing
frequent call shapes, not a claim that JavaScript calls have no checks or
frame work. See [JSCall and JSCallN](https://github.com/v8/v8/blob/4be854bd71ea878a25b236a27afcecffa2e29360/src/interpreter/interpreter-generator.cc#L1310).

Pinned LuaJIT reference: v2.1 commit
`c5a2fa4df71eb5ee9c71bc7b32bb5630d18568d7`. Its trace snapshots map changed slots
and carry restoration distinctions; those maps belong to trace recovery, not
to a general license to leave arbitrary interpreter homes stale. See
[snapshot_slots](https://github.com/LuaJIT/LuaJIT/blob/c5a2fa4df71eb5ee9c71bc7b32bb5630d18568d7/src/lj_snap.c#L56).
Neither comparison transfers tracing-GC root rules or delayed collection to
Python's reference-count and finalizer semantics.

## Ranked recommendations

Ranking weighs concrete covered work, implementation cost, semantic risk and
how cheaply an experiment can reject the idea. It is not a speedup forecast.
The entries are distinct alternatives; several may ultimately share metadata.

### 1. Finish dead call-result lowering across the return boundary

**Implemented:** `IR_NO_DST`, call discard flags and return-side discard are
already present. **Missing:** the compiler retains a final `POP_TOP` when it
alone preserves the call's line after control returns. The reason is explicit
in `R/Python/regcfg_passes.c:620–655`: tracing can start inside the callee, so
the next word must still expose the caller's call line. This is an observation
constraint, not a use of the returned value by Python code.

**Change:** represent the post-call observation/retirement point separately
enough to let the returning path deliver directly to a discard recipient.
Preserve the ordering of line callbacks and result destruction. Do not simply
delete POP or close the result early. The recipient slot is on the **caller**:
`R/Python/bytecodes.c:5549` and
`R/Include/internal/pycore_interpframe.h:169`.
The existing Python return holds its result in a C owner, restores the caller
as current, retires the child, and only then publishes or discards that result
(`R/Python/generated_ir_cases.c.h:16015–16060`). Child-cleanup finalizers run
before the destination changes; the new lowering must preserve this ordering.

**Covered work:** nine retained result POP handler entries in the measured
logging loop, plus eligible other callers. The result's required release
remains. No numeric elapsed saving is established. **Cost/risk:** medium;
debugger activation and finalizer order dominate the proof.

**Smallest experiment:** lower one eligible call/POP form with an explicit
post-return boundary description. First compare instruction/event sequences
and owner retirements, then inspect generated code and per-workload handler
counts. Reject if maintaining the boundary merely recreates equivalent work.
**Gate:** tracing activated inside the callee; line/opcode monitoring toggled
there; callbacks that mutate locals or raise; a returned last-reference object
whose finalizer inspects the caller; discard to native and Python callees;
exceptions and frame escape. Broader performance screening follows only after
these semantics agree.

### 2. Make logical source boundaries independent of executable NOP words

**Implemented:** the boolean-false specialization can execute one ordinary
live NOP inline. It still tests that word and publishes its PC:
`R/Python/ceval_macros.h:741–760`,
`R/Python/bytecodes.c:3488,3533`, and the checked generator rule at
`R/Tools/cases_generator/ir_generator.py:3669`. Wrapped targets, lltrace and
free-threaded execution decline that shortcut. Calling this “NOP removal” is
inaccurate. **Missing:** a logical-boundary map independent of execution words.

**Change:** keep a logical position/event identity and map it to a resume
position and required state. A boundary may be on an edge, including a taken
edge, rather than on the following instruction. Multiple boundaries at one
physical PC need ordering; a source line number alone cannot represent them.
The ordinary path should not decode a NOP just to discover an empty action.
Instrumentation can use mapped boundary handlers or another reviewed slow
path, without requiring a dormant executable word at every boundary.

This cannot be accomplished by editing the line table alone. Today:

- `R/Include/internal/pycore_ir.h:65` makes lasti, line and exception offsets
  word indices; jump encoding and assembler relaxation use that coordinate.
- `R/Python/instrumentation.c:58–135` patches handler words and indexes
  monitoring data by word, including destinations and relaxed jumps.
- `R/Objects/frameobject.c:1648` reconstructs stack kinds from executed IR
  formats for `f_lineno`; folded inputs and elided NULLs already need special
  reconstruction rules.
- `R/Python/regcfg_passes.c:70,169,534,1195` preserves line and stack-shape
  boundaries during several different transformations.

**Covered work:** residual NOP word checks/PC writes, remaining ordinary NOP
dispatch elsewhere, and constraints that retain otherwise removable code.
The ten logging NOPs already avoid their own dispatch; do not claim ten more
dispatches can be saved there. **Cost/risk:** large for a general solution,
medium for a bounded first boundary kind; debugger jumps and dynamic monitoring
are the main risks.

**Smallest experiment:** create a compile-time census of NOP reasons and their
incoming edges, then prototype only the existing boolean continuation using
an edge boundary record. Compare physical word count and generated hot-path
instructions. **Gate:** exact event order, lasti/traceback attribution, exception
range endpoints, forward/backward/taken/fallthrough edges, tracing enabled
during a callee or callback, f_lineno legal/illegal jumps, generators and
monitoring changes while suspended. Preserve a single selected representation
if adopted; a temporary experiment is not a permanent dual architecture.

### 3. Encode frequent operand ownership modes once, instead of rediscovering them in each handler

**Implemented:** local-versus-temporary class is fixed by field index; the
compiler knows it. **Missing:** many generated handlers still test that class
at runtime through `IR_IS_LOCAL`, `IR_CONSUME_INPUT`, `IR_STORE_DST` and related
macros (`R/Python/ceval_macros.h:498–641`). This is distinct from borrowing
arguments across calls: even under today's owning-home policy, the same
instruction's operand mode does not vary between executions.

**Change:** assess a small number of generated handler variants for the most
frequent input/destination modes. Select them after slot allocation, and make
their deopt/instrumentation mappings explicit. Avoid the Cartesian product of
all types, modes and families. A side-table load or branch can cost as much as
the current comparison; packing new bits is not automatically better.

**Covered work:** repeated slot-class comparisons, conditional consumption,
and local/temp output paths in admitted handlers. Dynamic value-tag tests that
remain semantically necessary are outside this saving. **Cost/risk:** medium;
handler text growth and mapping mistakes are the principal costs.

**Smallest experiment:** census operand modes for one hot scalar reader,
generate only its dominant mode, and inspect compiled instructions/text size
before timing. **Gate:** local/temporary aliasing, deopt before consumption,
instrumented twins, finalizer mutation of folded destinations, error cleanup,
and canonical result publication. Require a broad screen including spectral;
fewer branches with larger text is not necessarily a win.

### 4. Minimize frame entry and retirement on the actual exact and general routes

**Implemented:** exact positional calls already bypass the transaction helper:
`R/Python/generated_ir_cases.c.h:7186` executes `CALL_PY_EXACT_ARGS` with its own
guards and `_PyFrame_PushOverlapping`. The guarded general path is separate at
`R/Python/ceval.c:3376`; its overlap preparation is at `3055`. Initialization
still performs the general frame-header protocol
(`R/Include/internal/pycore_interpframe.h:282`), and ordinary cleanup is already
locals-only (`R/Python/frame.c:185`). **Missing:** an attributable hot entry/
retirement cost model and a proof of which remaining header/guard work can
be moved to code preparation or observation slow paths.

**Change:** evaluate a validated call-entry descriptor containing immutable
code-layout facts, and a minimal synchronous entry/return sequence using it.
Keep dynamic function/provider/recursion/stack guards. Consider lazy derivation
of observer-only header data only after a consumer census proves it safe.
Do not remove code ownership merely because the function also owns its current
code: `__code__` can change while an older activation is running.

**Covered work:** recurring header setup, code/layout reloads and retirement,
on whichever route compiled-code inspection actually identifies. This does
not save binder copies in logging: the retained diagnostics found none.
General-path post-kwargs revalidation is necessary after an allocation that
can run Python, not redundant by inspection. GCC may already combine repeated
loads in exact-call source; source duplication is not machine-code evidence.
**Cost/risk:** medium to large; frame introspection and entry failures are hard.

A bounded read-only inspection of the retained spectral candidate ELF
(`spectral-perf-identities/candidate-python`, GNU build ID
`d70c9143464a2fb1efeb488c5609c502cc4e5caf`) confirms this is worth inspecting:
`objdump -d --disassemble=_TAIL_CALL_CALL_PY_EXACT_ARGS` shows a 48-byte C stack
allocation and several stack stores/reloads in its entry sequence. This does
not prove that any particular spill is avoidable or explain spectral's loss.
It supports an emitted-code experiment, not source-only claims about the
tail-call convention. Keep GCC fixed; compare hot/cold factoring and live
state across the existing calling convention before proposing another backend.

**Smallest experiment:** use the retained exact/general route counts and
disassembly to count stores, loads and owner operations separately for each
entry/return shape. Prototype a descriptor for one no-default exact shape
only if it removes demonstrated work. **Gate:** mutated `__code__`, defaults
and vectorcall; PEP 523; recursion/stack exhaustion; frame exposure and GC
during preparation; exceptions before commit; generator-owned callers;
monitoring activated during preparation. An exact-entry improvement must be
measured on the inner exact route, not credited from the P3 general route.

### 5. Test borrowed synchronous child arguments as an independent ownership design

**Implemented:** argument-window overlap transfers owners into child homes;
it already avoids an argument-array copy. Strong local loads used as call
arguments can still create counts, and the child retires its owning locals.
See `R/Include/internal/pycore_interpframe.h:526` and
`R/Python/bytecodes.c:4318`. **Missing:** mortal borrowed frame homes, anchor
identity, overwrite/escape promotion, GC edge accounting and consumer support.

**Change:** assess a representation where an eligible child argument names an
explicit surviving owner. Preserve canonical heap PyValues; borrowing is a
frame ownership property, not an alternate heap value format. Universal
eight-byte values do not themselves require a fresh child count.

**Covered work:** only mortal argument increment/decrement pairs that this
proof admits. Inline/immortal arguments, already transferred temporary owners,
and required result/kwargs destruction provide no corresponding saving.
Logging's 90 increments/100 decrements per loop are an aggregate, not the
borrowable-argument budget. **Cost/risk:** large; a hidden owner dependency can
break GC subtraction, cycles, frame escape and native transfer.

**Smallest experiment:** count argument provenance and surviving owner identity
on exact and general routes without changing ownership. Derive a ledger of
candidate removable pairs. If small, stop; if substantial, build one explicit
anchor/promotion design. **Gate:** deleting/rebinding the parent local;
callee argument overwrite; recursion; frame escape; generators; native calls
that steal ownership; callback mutation; GC cycles and incoming-edge counting;
ordinary guard decline and error paths. No fallback may observe an unanchored
borrow or consume it as an owner. This is worth investigating but is not the
only credible next direction.

### 6. Extend generated contracts into checked guard, heap-effect and phase facts

**Implemented:** metadata derives reads/writes, callback/raise/poll effects,
borrowable inputs and a narrow first-output publication phase. Shape checks
for the new bool, dict and guarded call paths are deliberately strict
(`R/Tools/cases_generator/ir_generator.py:3669–3801,4304–4770`).
**Missing:** general heap read/write domains, guard-established facts and
multi-output/error phase contracts; `R/Include/internal/pycore_ir.h:254`
explicitly says so. Copy propagation consequently respects a family's union
of possible Python callbacks (`R/Python/regcfg_passes.c:155–306`).

**Change:** add semantic declarations only for optimizations that need them:
guard before effects; possible callback/heap write; output ownership and
publication; decline preserves inputs; error cleanup. Derive consumer tables
from those declarations. Retain conservative default effects for unknown
helpers. Strict token-shape validation is a useful safety mechanism today;
relaxing it without a replacement proof would be a regression.

**Covered work:** duplicated synchronization/publication and overly broad
barriers in certified paths, plus safer reuse by future optimizing frontends.
A generic IR op cannot assume its warmed specialization will always run;
moving a local read across it needs a guard and correct fallback state.
**Cost/risk:** medium to large; false purity is a correctness bug.

**Smallest experiment:** describe one existing two-output method reader and
its success/decline/error states, then show one redundant operation the
generated contract can remove. **Gate:** negative generator tests for missing
effects and invalid phase promises; callbacks between outputs; destination
aliasing; receiver/displaced-owner finalizers; guard failures after an effect;
monitoring and exceptions. Do not turn a single success-path result fact into
a GC root or whole-handler purity claim.

### 7. Replace the universal NULL-home invariant only with an ownership/validity map

**Implemented:** consumption clears temporary cells and outputs publish valid
homes; overlap initialization already skips the known-NULL shared tail.
**Missing:** persistent validity/occupied-owner state independent of physical
bits. GC scans every physical slot (`R/Python/gc.c:483`); frame copying moves
every slot (`R/Include/internal/pycore_interpframe.h:224`); exception cleanup
releases temporaries in reverse slot order (`:198`). Compaction's future-read
liveness is not sufficient for any of those consumers.

**Change:** first try bounded coalescing of clear-and-overwrite within a proven
non-reentrant sequence. If that cannot cover enough work, assess an explicit
map that distinguishes valid slots, owned references and future reads at each
observation/cleanup point. Preserve release ordering and account for C-owned
values. Never expose stale bits as another owning edge or drop a no-future-read
owner that still requires retirement.

**Covered work:** selected NULL writes, initialization and scans; ordinary
normal cleanup already avoids scanning transfer temporaries. Logging's zero
constant homes mean constant-home initialization cannot explain its gap.
**Cost/risk:** small for a verified local pair, very large for general maps.

**Smallest experiment:** count adjacent consume-clear/overwrite pairs with no
callback, allocation, release, poll or observer between them; prototype one
coalesced form while keeping valid homes at all exits. **Gate:** allocating
and last-reference release paths, unwinding, GC, frame copy/escape, infinite
loops with owners lacking future reads, generators, monitoring and lltrace.
General maps require a complete consumer census before changing initialization.

### 8. Use polymorphic feedback to improve execution only where alternating sites justify it

**Implemented:** feedback records up to four observations, then MEGA; it also
helps select a non-shape attribute fallback. **Missing:** a general bounded
polymorphic execution chain; ordinary handlers still execute the selected
current specialization. See `R/Include/internal/pycore_feedback.h:49–58`,
`R/Python/specialize.c:354–559,876`. Recording a POLY state is not itself a
polymorphic inline cache.

**Change:** experiment with a two-entry guarded execution path for one family
where repeated deopts and respecializations are demonstrated. Keep raw cached
pointers borrowed and guarded by the owning type/version protocol; do not
equate a recorded type version with a proven guard.

**Covered work:** repeated guard misses and specialization churn in admitted
alternating sites. There is no evidence this helps the warmed monomorphic
logging target. **Cost/risk:** medium; larger handlers and stale-cache safety.

**Smallest experiment:** a two-shape attribute workload plus a real broader
workload with recorded misses; compare misses, retired instructions and text
growth with the monomorphic policy. **Gate:** descriptor precedence, deletion,
type/version invalidation, class mutation, callbacks, MEGA transition and
instrumentation. Branch-history writes are a separate policy: the runtime
currently records them under specialization, even with the JIT disabled
(`R/Python/ceval_macros.h:366`). Any sampling change must identify its consumers
and freshness contract; removing history because the JIT is off is not this
PIC experiment.

### 9. Keep optimizing SSA and backend work separate, but share semantic lowering and recovery contracts

**Implemented:** executable register IR plus generated C/musttail dispatch.
**Missing:** native method/trace SSA execution, hardware allocation, OSR and
general recovery. The current [JIT design](../../design/jit-e2e-system.md)
explicitly labels its Action/recovery interface as unimplemented. The presence
of a native publication/unwind utility does not establish a working optimizing
tier.

**Change:** use one semantic lowering contract for methods and traces, with
explicit effects and reconstructible state, while keeping baseline words cheap.
Start recovery with a bounded before-effect exit; then add after-effect/error
states without replaying callbacks or releases. Do not make the baseline word
format carry every future SSA fact, or treat opcode-to-uop expansion as a
complete frontend. Backend register allocation is distinct from frame-slot
compaction.

**Covered work:** future repeated guards, boxed/general operations, frame
traffic and loops that a native optimizer can eliminate. It saves **nothing**
in the current JIT-disabled measurements. **Cost/risk:** very large; recovery
and owner lifetime, not merely instruction selection, are the admission cost.

**Smallest experiment:** lower one pure arithmetic/control region to the
shared typed graph, force a before-effect exit at every guard, and compare the
reconstructed interpreter state and owners. **Gate:** exact fault/resume PC,
inline-frame and locals reconstruction, ownership and GC accounting, exception
state, tooling/unwind metadata, and no repeated effects. Later call/return and
trace side exits extend this gate; they cannot inherit it by assertion.

## Current specification review

The new short specifications are substantially more useful than a dated
multi-thousand-line design ledger. The following findings were made against
their drafts, using the frozen source. **All nine are resolved** in the current
`pyjit/design/register-ir.md` and `interpreter.md`, re-read after the final
return-order correction at 23:24 UTC. These are documentation findings, not
runtime bugs:

1. **Dispatch:** the interpreter draft describes a selected computed-goto or
   tail-call configuration. Frozen `R/Python/ceval_macros.h:44,108–115` has
   function-per-handler musttail dispatch. Describe that one backend.
2. **Return destination:** the return recipient's `return_slot` is on the
   caller, not the dying child. See `R/Python/bytecodes.c:5549` and
   `R/Include/internal/pycore_interpframe.h:169`.
3. **Constants:** hidden homes cover eligible inline scalar constants used by
   borrowable loop readers, not every constant. Constants outside loops and
   window inputs keep their loads (`R/Python/regcfg_passes.c:794`). Hidden
   constants sit between cells and free variables, not after all freevars.
4. **Producer ordering:** the optimized flowgraph already removes pseudo-ops
   and normalizes jumps before register conversion (`R/Python/regcfg.c:6`).
5. **Coordinates:** include fused compare/contains/is branches' signed D
   relative encoding as well as FOR_ITER/SEND. CD's numeric value is
   `C | (D << 12)`; the draft's “C:D” wording can suggest the opposite order.
6. **Sharing:** clarify that the executing copy is per code object, shared by
   frames and also threads in the free-threaded build; “private” means separate
   from immutable words. Free-threaded specialization is disabled
   (`R/Include/internal/pycore_ir.h:328`, `R/Python/ir.c:452`).
7. **Narrow continuation:** say the NOP shortcut is in the checked
   `POP_JUMP_IF_FALSE_BOOL` continuation. It is not a general scan of every
   dispatch target, as a broad flowchart arrow could imply.
8. **Calls:** distinguish exact direct entry from general guarded overlap and
   consuming fallback. The exact path does not first execute the general
   resource-preparation transaction. Preserve the as-built route distinction.
9. **Return order:** hold the owned result in C while making the caller current
   and retiring the child, then publish/discard to the recipient. The initial
   prose and diagram reversed retirement and publication. Cleanup can run
   Python, so this distinction changes what a finalizer observes. See
   `R/Python/generated_ir_cases.c.h:16015–16060` and
   `R/Python/ceval_macros.h:763`.

The coordinator applied the corrections while preparing the documents. The
register-IR diagram still uses “private executing copy” as shorthand, but its
execution-copy section now explicitly defines per-code sharing, so the
ownership meaning is clear.

The final return prose and call diagram now explicitly hold the owned result
across child cleanup and publish/discard afterward. I also read the
[coordinator's next plan](next-steps.md): its nine dispositions accurately
reflect this review, keep borrowing independent, preserve the two separate
call routes, and add child-cleanup finalizer ordering to the acceptance gate.
There are no remaining source-accuracy blockers in those pages from this
bounded review. The coordinator owns rendered-diagram inspection and final
publication; this review checked their text and control-flow meaning.

Review verification was source/retained-artifact inspection and whitespace
checks. Runtime HEAD and its clean working-tree status were checked again;
no runtime file was changed. The proposed semantic gates above are future
admission requirements, not tests executed by this review.

## Suggested next packet

Start with the first three entries as separate designs: return/discard
boundaries, logical source anchors, and one measured operand-mode variant.
In parallel with design work, derive the exact/general call cost ledger and
the borrowable-argument ledger from existing evidence or bounded diagnostics.
Choose implementation only after each experiment's removable-work budget and
semantic proof are concrete. This preserves borrowing as a serious option
without allowing a previously selected plan to decide the review's conclusion.

Do not bundle a new word format, global root maps, borrowed homes and JIT
recovery into one interpreter optimization. Their proofs overlap, but their
costs and failure modes need separate decisions. Keep the frozen baseline,
unchanged logging workload and broad spectral screen for subsequent comparisons.
