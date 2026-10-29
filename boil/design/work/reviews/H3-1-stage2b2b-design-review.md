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
