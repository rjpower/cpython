# Terminal observer source checkpoint

Source-only first staged migration; no native build, regeneration or performance
claim. Runtime baseline is committed 4a1f70f; root owns integration.

Implemented the positive IR_OBSERVE_COMMITTED_BRANCH marker for FOR body, OR_POP
and consuming/fused conditional observers. One shared final-publication emitter
keeps ordinary text unchanged. Outputs/window/local are committed before callback;
NoDst or displaced-local owners remain in C. IR_DEFER_CLOSE captures an input
owner in a hygienic local, NULLs the original spelling, and closes it only after
the terminal continuation is valid. There are no post-callback old stores.

The structural checker rejects following semantic work, nonfinal composed-uop
placement, undefined outputs/live inputs, untransferred C owners, use/close/rebind
after transfer, unknown owner transport, ambiguous reachable ownership joins,
duplicate transfer and injected-local collisions. Its C-owner ledger covers the
terminal uop; preceding composed uops retain the existing Storage publication and
scope contract. It is not a general C-language ownership prover. A helper's trusted
nonconsuming boundary is explicit and shadowed bindings/caches are refused.

The analyzer marker advertises escapes/error/sync/PC but deliberately is not a
logical return or always-exits instruction. The C emitter terminates each selected
path after publication and dispatch; its normal CFG/output metadata remains.
Defined NULL and undefined output are separate host-test cases.

Activation-owned observer scopes pair actual frame/event and original intent
snapshot per callback, restore before arbitrary callback-result close, and mask
unknown/private-entry hooks. The cold getter walks only active activations and
copies the first matching record; UNSUPPORTED masks outer authority. Exact-frame
source errors are invalidated only at actual redirect commits, including deferred
CompleteRedirect after child retirement. PRE and POST CompleteStep errors save the
callback exception around held-owner retirement and keep the latest committed PC.

Host validation command:

```
UV_CACHE_DIR=/home/power/.cache/uv uv run --python python3 --no-project python -B \
  pyjit/issues/frame-call-model-20261109/run-terminal-host-checks.py \
  /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108
```

The runner executes the three actual new TestIRCases methods and their mutations
using AST-isolated class/helper definitions, then generates the complete IR and
metadata in memory. It does not write generated runtime products. Before this
harness, importing the full local Lib overlay into the host stdlib failed in
asyncio; an initial wrong-class selector ran zero tests and was rejected as no
test evidence. Later isolated methods initially exposed expected-diagnostic and
release-classification mistakes; those were corrected and rerun. The retained
passing receipt is terminal-host-checks.log; it asserts three tests ran.

Final source review added dotted-owner transport and early-exit-after-defer
negatives. The close-only `_PyValueTemps_Close` helper is explicitly nonboxing:
it drains existing exact numeric boxes and cannot add any. The tests check that
terminal publication does not allocate a boxing record solely for this cleanup,
while an unknown value-record helper remains conservatively boxing.

The full in-memory comparison changes ten instrumented handlers and no ordinary
handler. All 262 ordinary handler blocks share SHA256
953e15a9f8b555ab3973972012e8c1472259767e57c865d54635b1de4b8bf4bf before/after.
Actual before/after FOR and OR_POP output is terminal-output-examples.md.

Runtime test source covers eventual weakref collectability after FOR body and
OR_POP retained/drop callback errors, published folded locals, read-only finalizer
inspection of the new binding, and ordinary LINE landing without relying on a
source-only span. No unsupported POST setter positive is forged: the first native
packet preserves the existing refusal scope and PRE restoration authorization.

Remaining direct/composed provider census is in terminal-observer-commit-proposal.
END_ASYNC_FOR/END_SEND/END_FOR and true yield/suspended transitions are not silently
certified by this marker. Full provider completion and actual POST/yield debugger
application remain follow-up work after this source's independent review/native
gate. B owns restoration/frameobject hooks; the third worker owns generated-format
consumer and encoded owner/error proof. No final qualification is claimed here.

## Coordinator-generated product peer

Read the actual regenerated products before native29. Comparing handler blocks
to HEAD confirms 262 ordinary blocks exactly unchanged and ten instrumented
blocks changed. Actual FOR and OR_POP bodies have no boxing bank. FOR publishes
the cursor/item and selected destination before observation. OR_POP publishes
VALUE or defined NULL and captures its drop owner. Typed error handling saves the
callback exception across deferred retirement and uses the latest committed PC;
neither success nor error performs an old result store afterward. This is source
post-review of coordinator-generated files, not a worker regeneration or native
result claim.

Native29 release built 117 modules. Coordinator focus reports the five new
terminal methods, all 95 test_code cases and 24 contract cases passing. The
generator module's only failure was an expected-row port mismatch: its synthetic
LOAD/IR_PASS snapshots omitted observer_outcome. Root reviewed the two-line
fixture correction to exact NONE/NONE (these bodies have no certified terminal
provider). All previous exact flags/roles/effects/twin fields remain. The
corrected metadata method and three terminal generator methods pass together in
the host isolated harness; native selector retry remains coordinator-owned.
