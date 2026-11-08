# H3-1 stage2b.2b storage design review

Coordinator and independent ROOTS read-only review, 2026-10-29 10:08 PDT.
Native baseline is frozen `dc02f76c0ccf`. Initial proposal:

- Design SHA256 `2738f4d380bbbf9d48b0fc9d39414c202f000ea10cf4d295490e750153fcffee`.
- Unapplied CAPI patch SHA256 `1bf0f9cd66e62b548a0887f93a70c11ef36203ed3de1380d9413be4655189982`.

Verdict: REVISE. No implementation or shared-hunk grant yet.

Fixed absolute int lines and separate uint16 originals are the recommended
single format. Eight authorized children exposed old-code failures on valid
compiled ASTs: backward line deltas corrupt or omit release LINE events and
abort debug; INT_MAX also aborts debug. Normal module line zero passes.
The new format removes the codec and its range assumptions. Its requested-byte
formulas are forecasts, not measurements of an unimplemented allocation.
Actual allocation counts, requests and preparation/failure peaks remain required.

The coordinator approves the private API/layout decision in PROCESS section0.2.
This does not approve an incomplete caller update or a fixture with the wrong
ABI. Three concrete blockers require a revised frozen proposal:

1. The design inventories three GetLine calls, but Python/legacy_tracing.c
   has two more in sys_trace_jump_func. That file is built and the function
   registered. Update all five callers, preserve to/from bounds before the
   direct decoder, and include backward-edge tracing controls and file ownership.
2. ROOTS found that monitoring_deinstrument and monitoring_state are registered
   METH_O with pointer argument/result signatures. This fork's METH_O dispatch
   uses PyValue; inline integer arguments and pointer results would be
   misinterpreted. Use its real value signature/cast convention or explicit
   VARARGS parsing. The roundtrip VARARGS hook has the correct interface.
3. The state hook passes mutable header-byte pointers to Py_BuildValue.
   ROOTS found that result allocation can reenter monitoring even under the
   owning GIL. Copy all requested bytes, version and presence flags into
   stack-local storage before the first Python result allocation; construct
   the result only from those copies.

Independent review continues on initialization, publication, local OOM rollback
and retry, global partial progress, enabled-tool membership and the exact
read-only snapshot. Documented spurious DISABLE callbacks must not become
new test failures. No additional native probe or rerun is requested here.

## Corrected proposal, 10:18 PDT

Independent ROOTS verdict: IMPLEMENT_2B2B. Coordinator agrees after reading
the corrected design and exact fixture patch:

- Design SHA256 `84e518cb872e0522247ffced7f131e943add7891c348746e9ca626c62b2f2116`.
- Patch SHA256 `71ec68cb7da7e217e213aa548a74f352176bf150f9ef233298b291b970899cae`.

All five callers now have explicit bounds; legacy forward/different-line
DISABLE and same-line backedge delivery remain unchanged. Every line slot is
initialized, with real-head absolute values and synthetic trailing-word
-1/token0 kept distinct. All three hooks use VARARGS. The state hook copies
masks, tool versions, code version and presence flags before result allocation.
The original rejected patch and probe evidence remain preserved.

Grant the exact additive CAPI include, three functions beside the safe helper,
and three VARARGS method entries on the H3 branch, coherently with its new
header/storage. Existing GC, compiler and scalar fixtures remain outside scope.
PROCESS section0.2 records the private API/layout decision.

Implementation must preserve installed-tool membership, local failure atomicity
and identical-mask retry, while recording global partial progress separately.
Require real old-fail/new-pass controls, fixed-width adjacency/line boundaries,
tracing and CALL restoration, actual allocation/peak accounting, generated
domain/phase checks and affected normal/debug/kill qualification. No runtime
landing or validation outcome is inferred from this design clearance.
