# Independent frame/call and compact-IR design review

2026-11-09. Read-only source review at `4b3d739d867`, existing profiles/counters,
the frame-call proposal and revised IR-position proposal. No native jobs or
runtime edits. **Approve the revised design for implementation, with the
interface obligations below; this is not code or performance admission.**

## Correction adopted

The first IR proposal unnecessarily preserved removed instruction identities.
Reject that design. Public instruction offsets remain compact executable PCs;
removed POP/NOP words get no INSTRUCTION event, recovery slot or second public
offset domain. Preserve executed source LINE events using sparse edge metadata,
the existing f_lineno override and observer-only resume state. This follows the
[monitoring instruction contract](https://docs.python.org/3/library/sys.monitoring.html#sys.monitoring.events.INSTRUCTION)
and separates it from [PEP 626 source tracing](https://peps.python.org/pep-0626/).
The latter and [co_lines](https://docs.python.org/3/reference/datamodel.html#codeobject.co_lines)
permit zero-width eliminated ranges; static ranges alone cannot encode which
CFG edge executed an empty source statement.

## Frame proposal assessment

The direct stable binder is the right structural direction. Existing overlap
already transfers argument owners in place, so there is no supposed argument
copy budget to recover. The actual expensive general path constructs a
transaction, allocates kwargs and repeats validation/refresh before committing
(ceval.c:3070–3144). One immutable numeric binding plan plus function/provider
guards can remove repeated shape discovery and generic-runner scaffolding.
Keep the generic transaction as a miss path before ownership changes, rather
than replaying partial preparation or creating another parallel frame ABI.

Feedback currently contains borrowed references rather than GC-owned roots.
Therefore initial plans should contain scalar offsets/counts/flags and guards,
not new owning code/default pointers without a storage/traversal change.
funcobject.c:276 documents version invalidation and its setters clear versions;
the in-place kwdefaults dictionary case still needs its own version/current
read if supported. Initial exclusion of kwonly plans is a reasonable bound.
The executing frame must retain a separate code owner across mutable
function.__code__; the function owner alone is insufficient.

Virtual empty kwargs can remove work that is never semantically demanded,
including allocation/release and associated generic preparation. It is not a
shared empty dict or a noncanonical value sentinel. Pending+NULL is valid only
if every possible read materializes first and frame exposure cannot report an
unbound local. The first implementation must cover folded local operands,
window construction, return/compare/truth and LOAD_FAST_AND_CLEAR, not just
LOAD_FAST. Rebinding/deletion cancels pending; nonlocal/cell capture and
suspended callees initially decline. Add no classification test to unrelated
local reads. The compiler inserts a semantic materialization barrier at the
relevant binding's read paths and verifies it dominates those reads.

The allocation proof is stronger than the old generic-path comment suggests:
dictobject.c:959 uses a freelist or GC allocation; gc.c:2018 links/schedules GC
rather than running it synchronously, and frame.c:33 already relies on a
noncallback frame-allocation contract. Under that existing contract a pending
kwargs dict can be installed before a poll/callback. Keep OOM unwind valid and
preserve the raised exception while frame exposure materializes. Do not turn
this into a speculative custom-allocator redesign. Publish a frame object only
after its supported locals have been materialized.

Plain result slot/discard plus real executable continuation is sufficient.
Remove IR_POSTCALL_DISCARD and live-POP classification, including source-line
restrictions in regcfg_passes.c:632. Native and Python completion use the same
recipient categories; SEND has an explicit semantic continuation kind.
Result ownership survives child retirement, but no discarded result is retained
solely for an obsolete POP/debugger state. Source observation never implies
preserving a historical decref location as a Python guarantee.

Canonical RETURN_VALUE transfer is another sound shared simplification, if
proven by the generator. _PyValue_Publish (pycore_value.h:122) checks object
type and exact long/float canonicalization. _MAKE_HEAP_SAFE/IR_TAKE of a
canonical home, followed by _RETURN_VALUE, preserves the owned bits through
retirement; re-canonicalizing that result is redundant. IRStack.store currently
emits IR_STORE_RETURN before considering canonical_result (ir_generator.py:2110).
Propagate an explicit provenance fact there. Keep publication for native/raw
or SEND producers until separately proven. Instrumented/yield return chains
need the same audit, not a handler-name exception list.

## Architecture comparison and limits

[V8's bytecode declarations](https://raw.githubusercontent.com/v8/v8/main/src/interpreter/bytecodes.h)
encode explicit register lists/counts for calls and an accumulator result;
[LuaJIT's bytecodes](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_bc.h)
encode register-window calls/results, with its
[x64 interpreter](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/vm_x64.dasc)
keeping dedicated base/PC state. The useful lesson is a coherent semantic call
ABI and direct continuation, not preservation of frontend stack artifacts.
Neither implementation proves that Python RC owner transfers, exceptions or
frame observation can be omitted: their GC/runtime contracts differ. Import
the structural discipline, not unreviewed lifetime assumptions or their word
format.

The proposal correctly defers borrowed parent locals: exposed ancestors can
mutate them while children execute, and overlap already removes copying.
Any future borrowing needs durable anchors and promotion, not weaker comments.
An owner-map cleanup increment is still a major opportunity: establish actual
occupied homes and avoid redundant scans/stores, while keeping GC and escaped
frames coherent. Method lookup remains independent substantial work. The
measured 5,617-instruction logging gap cannot honestly be promised away by
empty-boundary removal or by adding sample percentages.

## Implementation interface and sequence

1. Agree plain continuation/recipient fields and kwargs helper signature first.
   IR worker owns first-read barriers and source maps; frame worker owns the
   pending binding flag/local and noncallback materialize helper. Compiler
   metadata conservatively applies to all calls of eligible code, even when a
   generic binder has already produced a dict; the barrier then becomes a no-op.
2. IR worker owns regcfg/passes/assemble, codeobject/marshal/dis, source-event
   instrumentation and frameobject debugger/line consumers. Frame worker owns
   ceval, frame.c, interpreter-frame headers, feedback plans and specialization.
   IR worker sends frame-exposure insertion requests to the frame owner and
   kwargs calls in frameobject are applied by IR owner. No concurrent edits.
3. Route bytecodes/generator changes through frame worker sequentially: remove
   boundary/discard variants and hooks; add semantic result sinks, completion
   edge cleanup/targets and canonical-result transfer. IR worker supplies exact
   producer/observer contracts before these edits.
4. Integrate executable compaction/source LINE handling with plain recipients;
   then direct binding/virtual kwargs under the same representation. Neither
   component is qualified in isolation from its changed consumers. Verify the
   actual executable stream has no empty/release-only instruction artifacts
   and unobserved dispatch has no replacement metadata interpreter.

No remaining architectural blocker prevents this split from starting. Concrete
schemas and verifier failures are implementation gates, not permission to
invent fallback representations. Root owns native validation and measurements;
the previous qualified checkpoint and golden remain evidence/control. Continue
through measured logging parity and broad-regression repair, rather than
stopping after this design or a small local instruction reduction.

## First source handoff: provisional integration, not admission

The IR worker has implemented the normalized `IRSM` linetable envelope,
zero-word frontend NOPs, semantic result sinks for unique fresh producers,
source-edge LINE instrumentation and explicit callback exception recipes.
Public offsets still count real words. Observer-only frame cursors and the
exceptional thread-state cursor add no ordinary dispatch metadata work.
Construction validates source bounds, including the `lasti` slot before the
exception home. Virtual kwargs construction proves immutable local use and
barrier domination, including handler targets belonging only to source events.
Structural `code.replace` clears compiler-derived kwargs eligibility; replacing
words while retaining a nonempty source map requires a matching new map.

The debugger implementation deliberately does not authorize jumps using the
old frontend stack projection. Its cold fixed-point analysis reads actual IR
slot roles, window extents, successors and constant homes. Both source and
target must need no incoming temporary owners; scratch owners are then cleared
without fabricating stack entries. Source-only callbacks use their real landing
state. This is a provisional safety gate: protected/exception and suspended
resumptions need explicit recipes and currently raise `ValueError`. Existing
`test_sys_settrace` expectations remain unchanged. Native failures must identify
missing incoming-register, exception-state or suspended-continuation recipes;
blanket rejection is not final debugger admission. Cross-frame debugger changes
while a child owns an overlapping call window also remain an explicit test.

The initial sink pass does **not** finish the requested artifact removal:
canonical/shared POP_TOP values crossing CFG edges remain; FOR_ITER/SEND and
END_FOR/POP_ITER/END_SEND completion still need a semantic edge owner transfer.
Do not rename those cleanup instructions or drop them under a NULL-slot
assumption. The next lowering must attach retirement to the actual semantic
producer/edge, account for both normal and exceptional successors, and make the
loop/send destination the first real successor. Relevant hooks are
`discard_region` in `Python/regcfg_passes.c`, handler/word selection in
`Python/assemble.c`, loop/send bytecodes and generator retirement metadata.
Canonical constructors reject NOP words now; the POP_TOP ban is deferred until
the compiler no longer emits the supported remaining ownership cases.

Boundary tests now ask for real instruction offsets, zero-width source lines,
late instruction enabling, source callback exceptions, suspension and actual
owner-preserving line jumps. Discard tests retain finalizer reachability,
late observer enabling, callback failures and cross-frame line changes, while
removing dead-POP event and deleted-specialization assertions. Host syntax and
diff checks pass; root owns the first generated/native integration run. No
runtime or performance qualification is claimed for this handoff.

### First bootstrap failure and bounded correction

The first native merged release build reached the new freezer, then failed
compiling importlib kwargs reads with `SystemError: stack underflow` (lines 543
and 399). No runtime tests passed that build. The inserted materializer was
IR-only, absent from frontend stack-effect tables; their `-1` result triggered
`mark_blocks` rejection. Its self-shadow also violated the existing 8-bit
instrumentation event opcode contract. The bounded correction makes
MATERIALIZE_KWARGS a real semantic opcode (pinned 127), with generated zero
stack effects and its own public instruction identity. No fake NOP event view
or special stack-effect bridge remains.

The same bounded batch validates every canonical handler before invoking new
format consumers, validates location payload boundaries and coverage, and
normalizes raw missing location suffixes into no-location entries. Normalized
maps must cover exactly the actual word count; empty raw tables therefore
produce one no-location position per executable word. Focused tests cover
malformed headers/lengths/record fields, empty locations, stale retained maps,
zero-width range ordering and marshal roundtrips. These changes are frozen for
the coordinator's second regeneration/build. They have host syntax/diff checks,
not native admission. The provisional debugger and retained ownership-artifact
limitations above remain open.
