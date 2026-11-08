# Cold review of the interpreter parity batch

2026-11-08. Independent source and retained-evidence review of committed
runtime through the code-home test-interface follow-up `0267d428808`
(P3 contract follow-up `1c8774797f3`, P3
`10627c339fa`, P1/P2 `0539e0c1ff9`, M2b `73c20472a9f`, M1
`d93b83800a5` on qualified R5 `0953757f4e7`).
Canonical contracts and reviews are in this experiment directory. This is a
review of the actual six commits and their composition, not a full correctness
or performance admission. I did not run a native build, test suite or timing.

## Verdict and findings

**No remaining must-fix source finding in the six landed commits.** One
omitted P3 test-contract usage was found in merged smoke and fixed in the
fifth commit; a full-suite code-home test-interface issue was fixed in the
sixth. The changes are narrow, and the source, generated handlers and focused
evidence agree. Keep the performance decision open. The direct P1/P2 screen
is bimodal; M2b and M1 have no isolated speed attribution.

1. **P1/P2 ownership and failure path:** `_PyFrame_CodeHome` calls
   `PyValue_DUP` on the known pointer representation. A mortal code object
   gains exactly one owner; an immortal keeps its no-count tag. Both frame
   initializers use it. The extracted `_PyValue_PointerBorrow` is reached from
   the generic object-entry function only after its exact-number conversion
   path, so that path's canonicalization remains intact. The helper's pointer
   precondition is a real internal contract: adding a numeric caller without
   proving boxed canonicality would be wrong. `call_transaction_refresh`
   returns after `value_operation_refresh` succeeds; on failure it still
   checks whether the frame changed and retains the useful preparation error.
   This avoids the duplicate success check without dropping failure handling.
2. **M2b observer and PC path:** `POP_JUMP_IF_FALSE_BOOL` alone appends the
   checked terminal intrinsic. On a GIL build it loads the *live* target word
   and accepts only ordinary `NOP`; it sets `frame->instr_ptr` to that retained
   NOP word, increments `next_instr`, then enters ordinary `DISPATCH` at P+1.
   This matches the current ordinary NOP's PC and advance effects. Wrapped
   line/instruction targets, instrumented branch handlers, debug lltrace and
   free-threaded builds keep their existing dispatch. The generated handler
   consumes the exact boolean before the intrinsic and has no pending value
   boxes, result or displaced local. Actual entered-handler stats omit the
   absorbed NOP, consistently with their definition; the boundary still has
   its Python PC. The generator validates producer sequence, guard text,
   scalar shape, terminal position and nonescaping properties. The test for
   line relocation, opcode tracing, monitoring and observer error passed in
   the final focused runs.
3. **Metadata/layout debt, not a current failure:** Marking the intrinsic
   tier1 removes `POP_JUMP_IF_FALSE_BOOL` from the otherwise unused legacy
   `_PyOpcode_macro_expansion` table. It leaves the register-IR word format,
   live handler metadata and retained NOP word alone. Any future optimizing
   frontend using that table needs an explicit lowering rule for the original
   guard and branch; simply treating the interpreter-only intrinsic as an
   optimizing uop would be unsound. Record this dependency at that frontend's
   design gate. The added producer contract is intentionally specific, which
   is appropriate for one handler, but it does not prove arbitrary future
   changes to the trusted bool branch primitive.

The release/debug P1/P2 builds and six-file focused runs succeeded, with 540
tests on each build (11 and 9 skips). The M2b final release/debug builds
reported 117 modules and five-file runs succeeded with 805 tests and one skip
each. `p1p2-call_transaction_refresh.asm` shows the successful return after
refresh, and `m2b-r2-bool-handler.asm` shows the target-handler comparison,
NOP PC store, advance and direct P+1 dispatch. These assembly observations
prove removal of the named machine-code work on their accepted paths, not an
elapsed-time improvement. The final clean six-commit HEAD passed full merged
release and debug qualification, as detailed below.

## Independent route and measurement audit

The retained `call-routes/summary.json` reconciles every named counter at
0/1,000/2,000 loops. Empty-interval subtraction yields ten direct exact-thread
entries and ten successful overlap entries per loop. All ten overlap entries
create fresh kwargs and empty varargs; the measured loop adds no overlap
decline, consuming fallback, native call or instrumented invoke. These are
route frequencies, not time shares. The original stats scaling shows 161
entered handlers and twenty pushed frames per loop, with ten dictionary
subscriptions and ten NOP entries. The post-M2b stats
capture at clean `73c20472a9f` shows **151 entered handlers and zero NOP
entries per loop**, while twenty frames, ten dictionary subscriptions,
twenty boolean branches and nine POPs remain. Its 0/1,000/2,000 counts scale
exactly. This confirms the predicted ten-handler accounting change, not its
time cost. The separate native or no-kwargs paths still exist and remain
correctness obligations for later guarded entry.

The temporary M1 provider diagnostic on the four-commit runtime reports
0/10,000/20,000 attempts and the same hit counts at 0/1,000/2,000 workload
loops. Empty-interval subtraction therefore gives ten successful first-probe
hits out of ten attempts per loop, with exact linear scaling. The overlay
added only attempt/hit counters, and its recorded restoration returned the
source to byte-identical, clean state before the later test-only fix. These
counts establish M1's coverage on this workload; they are not time shares.

The frozen R5-vs-P1/P2 diagnostic compares distinct retained ELF files and
has 323 matching before/after file hashes with zero changes. Its four direct
`logging_silent` candidate/R5 ratios are 0.9105, 0.9356, 1.0421 and 1.0382:
two clusters with opposite signs, geomean 0.9798. That is insufficient to
attribute a benefit to P1/P2 or to accept a performance-only expansion. The
two-round P1/P2 basic candidate/upstream screen has `logging_silent` geomean
1.7286; it is a different time/control campaign from the pinned six-round R5
baseline of 1.657289 and cannot be subtracted to infer a regression. Both
remain far from the plan's parity threshold. The executed source/ELF identity,
fixed workload, JIT-off and affinity data are useful controls, but host timing
variation and limited rounds constrain inference.

The later two-round post-M2b basic screen has `logging_silent` geomean 1.7159
candidate/upstream. Its 1.7046 and 1.7272 paired ratios exceed parity and do
not isolate M2b against the exact P1/P2 binary; comparing basic-screen
geomeans across runs would conflate time and layout variation. Its control
rows also need the planned broader regression screen before a keep decision.

The post-M1 two-round basic screen reports `logging_silent` paired ratios
1.7024 and 1.4739, geomean 1.5841 candidate/upstream, with stable file
identity and no run or postflight failure. The two rounds differ substantially;
they do not isolate M1 against the M2b ELF, and cross-campaign comparison
cannot assign the apparent change to M1. The result remains above parity.

The two-round post-P3 basic screen reports `logging_silent` ratios 1.3703
and 1.3945, geomean 1.3824 candidate/upstream. Identity and postflight
checks passed. This is the best basic ratio in the sequence, yet it remains
well above parity. Its `deltablue` geomean is 1.9083, so the wider set is
not uniformly favorable. Campaign timing and layout differ from earlier
screens; the changed basic ratios do not isolate P3's causal effect.

## What the design actually buys

This batch has removed generic numeric entry work for code homes, one
successful refresh revalidation, and on accepted ordinary NOP targets one
dispatch/handler entry. M1 now bypasses generic dictionary lookup and
completion for exact first-probe identity hits; other shapes keep that path.
P3 removes initial generic route rediscovery on its guarded Python-call edge.
The batch has **not** removed frame ownership, the observable kwargs
allocation and release, the twenty call frames, or the explicit
local/temporary register roles. The V8 comparison yielded eight ideas; the
two local cleanups, one dispatch continuation, narrow dictionary leaf and
guarded call-entry route have landed. The register-IR word layout and call
window contract remain unchanged. No broad interpreter or JIT parity follows
from this review.

The remaining gap is too large to justify another tiny source cleanup from
these counts alone. The observed outer kwargs creation and twenty frame
entries are plausible cost centers, but the route and dispatch counters do
not rank their elapsed cost. Use the final clean-source profiles and balanced
campaigns to choose the next mechanism; require an exact semantic proof for
kwargs lifetime, frame ownership and callback order before changing either.

The eight-byte word and four 12-bit fields are layout decisions: they bound
operand range and make variable call arguments use contiguous windows. They
do not require the duplicate refresh or pre-escape provider rediscovery.
Owning homes, callback-visible locals, precise fault PCs and sequential
reference retirement are semantic obligations in this runtime; neither V8's
accumulator nor its tracing collector removes them. Generated terminal edges
can use proved leaf results while keeping the whole-handler effect metadata
conservative. That is the useful abstraction demonstrated by M1 and M2b. It
is narrower than a general effect-state engine, and the cost of its
extra guard and generated code must be checked in GCC and timed controls.

The current owning-home rule is a runtime design choice, not a claim that
every call argument in CPython must own a separate reference. Pinned upstream
can transfer a `LOAD_FAST_BORROW` argument into a child while its caller local
anchors the object; it protects overwritten locals and promotes unsafe borrows
when copying a frame to the heap. The candidate's surviving message load
duplicates an owner. That is a legitimate structural comparison for a later
contract review, but the aggregate refcount statistics do not isolate its
cost. See [the source-only borrow follow-up](borrow-frame-followup.md).

M1's first-probe hit and terminal completion have passed independent
post-review of their actual nine-file materialization. P3's guarded call-entry
proof has also passed source, generated C, fixed-GCC and focused post-review
of its materialized diff. Its entry proof stops at allocating kwargs; the
post-allocation guards and generated generic fallback remain. Both changes'
machine-code savings are established, but their time effects cannot be
added. Full merged qualification has passed; route remeasurement and the
plan's two fresh six-round campaigns still control any parity claim.

## M1 materialized post-review

The materialized nine-file M1 diff has a sound narrow provider/continuation
shape. The helper declines before touching a borrowed owner unless the
container is an exact dict, the key is an inline int, and the table is a
combined general-key table. `_PyLong_ValueHash` takes the same inline-int
arm as `_PyValue_Hash`, including signed and -1 handling. Generic
`do_lookup` examines the same first index, and `compare_generic` accepts
`PyValue_Is` there before any equality callback. A different first key
declines, so its generic lookup still runs a collision callback once. A hit
duplicates a canonical entry value while the dictionary remains owned;
result, key and dictionary aliases remain owned through subsequent retirement.

The materialized generated handler consumes B then C from borrowed snapshots,
publishes a temporary destination before closing input owners, checks key deallocation
then dictionary deallocation in order, and reloads word/slots after an
out-of-line retirement. Its local destination is installed only after input
retirement, and the existing displaced-local path handles the old local.
On retirement failure the local's held result is passed for cleanup; a
temporary result stays in its published home. The generic emitter extraction
still uses its existing value-record decision. The marker validator requires
the named guard/producer sequence, scalar B/C inputs, A destination and no
window, and rejects a boxing value record. Regeneration also removes the
dictionary handler from the unused legacy optimizing expansion table; a future
frontend must lower its ordinary dictionary operation explicitly. It passed
source and generated-output review. Fixed-GCC disassembly shows the provider
result branches to a success edge before the separate decline edge calls
`value_subscript`; the accepted path skips that generic helper. It still
pays a call to the out-of-line first-probe provider.

The initial mutating-collision fixture could mutate the previous `d` during
construction of the new dict literal. The final case arms mutation only after
construction. The focused cases cover identity hits, collision callbacks,
wide/bool keys, misses, frozen dictionaries, subclasses, last-owner dictionary
retirement and a callback-mutated folded local. The first focus run reported
one new-test failure because the fixture expected `Old` destruction before
`Sentinel`. Both the retained R5 control and M1 produced
`[("dict", True), "sentinel", "old"]`. The frame-locals proxy pins overwritten
fast locals in `f_overwritten_fast_locals` until frame destruction; the
callback's Sentinel is displaced first. Correcting the expected order to the
observed baseline preserves the key assertion that dictionary destruction
sees the old local before the folded store. The final corrected cases pass
on the retained R5 and M1 ELFs. M1 regeneration, release/debug builds and
both 453-case focused runs returned zero. The separate [M1 post-review](m1-post-review.md)
records the detailed post-review and remaining timing limit.

## P3 materialized post-review

The materialized P3 runtime diff has no must-fix finding. Its private guarded
runner initializes the same
transaction record, tries the existing overlap and consuming Python fallback,
then uses the existing native route on a positive decline. The prefix proof
is limited to `CALL_PY_GENERAL`: PEP 523 is off, the callable is a Python
function with the specialized version, recursion is available, and no
instrumentation/monitoring preparation has run. The shared version equality
guard is unchanged. Specialization assigns only versions at least two;
`PyFunction_SetVectorcall` clears the live version before changing provider.
The new debug-only assertion checks the cached valid-version premise on the
private edge. Once kwargs allocation may run Python, the overlap and
consuming paths retain their original revalidation and native fallback.

The generator selects the guarded runner only for this marker; its fallback
label still calls the generic runner. The analyzer distinguishes general
register-IR marker effects from the two call-invoke markers, avoiding an
incorrect call-specific classification of M1. The validator checks the named
prefix and its stack/window shape, rejects placement in bound/instrumented
handlers, and generator tests mutate these conditions. The runtime tests
exercise code/default mutation, a custom function vectorcall provider, bound
method and generator declines, arity errors and monitoring. The actual
generated handler calls the private runner only after the guards; its
fallback label and bound method handler still call the generic runner.
Release/debug builds checked 117 modules, and both focused runs passed 583
tests with eight skips. Fixed-GCC assembly calls the private runner directly
and shows its overlap, consuming fallback and native calls with no generic
`call_transaction_invoke` call on that edge. This proves route rediscovery was
removed there, not a speed gain. The separate [P3 post-review](p3-post-review.md)
records the full result and qualification limit.

The first merged stats-build smoke found seven `test_ir_contract` failures
with one cause: its source scanner treats only `IR_INVOKE_CALL` as terminal
and scans P3's new terminal marker through trailing legacy frame uops. This
is an affected test usage omitted from P3's initial focused list. Commit
`1c8774797f3` makes it recognize both call-invoke markers and checks the
new guarded uop's effect/frame properties. After the temporary diagnostic
overlay was retired, `test_ir_contract` passed all 24 cases on both release
and debug builds; the six targeted marker-property cases passed on both.
`test_ir` also passed in the initial smoke. The seven scanner failures did
not demonstrate a runtime defect. The [P3 post-review](p3-post-review.md)
has the contemporaneous addendum.

At the clean follow-up HEAD, release and debug identity rebuilds returned
zero with empty source diffs. The clean stats rebuild and two-file
`test_ir`/`test_ir_contract` smoke passed all 142 tests. This establishes
that the temporary diagnostic overlay was retired from the executed stats
binary: the rebuild forced the restored `pystats.h` dependency despite
restored source mtimes. The final HEAD was rebuilt and checked again as
described below.

The first full release run then found `test_capi` failing because automatic
zero-argument discovery calls the P1/P2 `test_frame_code_home` helper, which
requires one code-object argument. `_testinternalcapi` has no other
argument-taking `test_` method in this table. Renaming that helper's C symbol,
exported method name and the `test_pyvalue_api` caller to
`check_frame_code_home` is a focused test-interface correction; its ownership
assertions did not change. Commit `0267d428808` makes exactly those name
changes. Both builds and focused `test_capi`/`test_pyvalue_api` runs returned
zero; each focused run passed 1,354 tests with nine skips. The failed full
run remains retained evidence. At clean `0267d428808`, the qualified release
and debug identity rebuilds, stats rebuild/smoke and value-API lint all
returned zero with empty source diffs. The stats smoke passed 142 tests;
the lint checked 306 value-native units with zero failing findings.
Each qualified build reported 117 checked modules with none failed on import.
The full release and debug suites each ran 54,394 tests, with 2,531 and
2,469 skips respectively; both logs report 537 test files OK and SUCCESS.
The full receipts identify the same clean source HEAD. This closes the
merged correctness qualification for the reviewed batch, subject to the
usual scope of these suites. The original failure did not indicate a
code-home ownership defect.

Evidence inspected: actual diffs for all six commits; `pycore_value.h`,
`pycore_interpframe.h`, `ceval.c`, `ceval_macros.h`, `bytecodes.c`, generated
IR cases, generator and tests; P1/P2 and M2b post-reviews; build/focus
receipts and logs; assembly; `existing-stats/scaling-summary.json`,
`call-routes/summary.json`, `m2b-stats/summary.json`, baseline, basic and
direct diagnostic summaries, qualified full-suite/identity/stats/lint
receipts and terminal logs under
`build/pyjit-link-wave1/interpreter-parity-20261108/`. I also checked the
direct diagnostic hash manifests offline. No runtime or native evidence was
modified.
