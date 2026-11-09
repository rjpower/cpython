# Canonical35 source checkpoint

Source is frozen for root's regeneration/native gate. Runtime changes are
limited to `Tools/cases_generator/ir_generator.py`,
`Lib/test/test_generated_cases.py` and `Lib/test/test_ir_canonical_home.py`.

The list-child proof matches the complete two guards, bounded duplicate and
two retirement uops by stack shape and body, with named caches/extra parts
refused. It returns the existing CanonicalResult. Its early temporary store
and list retirement stay unchanged; only the final shared store changes.

The plain yield proof matches exact TAKE plus the full unchanged suspension,
frame switch and output identity. The existing flag/function are renamed to
canonical_frame_transfer/canonical_frame_transfer_input throughout live
generator/tests. Instrumented yield retains ordinary publication. No parallel
fact, ownership phase, state/recursion change, metadata or ABI is added.

Raw source matching retains CPP delimiters. To bind it to the body actually
emitted, the helper reparses original token objects with the existing Parser
and requires complete BlockStmt equality with the current AST. Appending a
statement or moving the debug arm into else while retaining flattened tokens
both refuse the proof. This addresses root's stale-source review finding.

Standard UV host command:

```sh
UV_CACHE_DIR=/home/power/.cache/uv uv run --python python3 --no-project python -B \
  pyjit/issues/frame-call-model-20261109/run-canonical-transfer35-host-checks.py \
  /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108
```

Eight actual test methods pass, including prior numeric/local/return proofs,
new renamed positives and source/AST mutation negatives (final run 30.486s).
In-memory generation changes exactly BINARY_OP_SUBSCR_LIST_INT's final store
and plain YIELD_VALUE's final transfer. All 285 other handlers and the complete
metadata header are byte-identical. Generated SHA256 is
`e2633cf2b82aad2d73a1568bcaf17bb4131db626c5058f74fdd31411cba10582`.
AST parsing and diff checks pass. The initial host harness selected ten methods
against an eight-test assertion; its selector was corrected before test execution.

Three added native methods cover specialized list value/error routes, last-list
owner retirement with sibling GC/finalization and retained child identity, and
yielded numeric/mortal owners. Their execution is root-owned. Root's retained
`E11/canonical35-route-probe` confirms unchanged compiler geometry: local A=2
with nl=3, temporary A=2 with nl=2 and discard A=4095. The native fixture now
asserts these destination categories from the actual specialized rows, in
addition to handler presence and logical results. Its AST/diff checks pass;
this route receipt is not yet a native35 execution result.

B and H approved the actual source direction; H also accepted the raw/AST
binding refinement. No native or performance claim follows from host proof.

## Independent post-integration review

The committed runtime source is `df12a12b131384976466310b7a6138417b7a36eb`.
Release and debug builds each completed 117 modules; the focused gate passed
582 tests in 12 modules in each profile (the `E11/canonical35-release-*` and
`E11/canonical35-debug-*` build/focus logs). Regeneration changed only the two
final stores described
above. The list handler still publishes its early temporary before retiring
the prior owner; plain yield retains its owner transfer. The instrumented yield
handler and the other 285 generated bodies are unchanged.

Read-only comparison with the retained clean34 ELF found static handler sizes
of 1225 to 985 bytes for `BINARY_OP_SUBSCR_LIST_INT` and 654 to 453 bytes for
plain `YIELD_VALUE`; instrumented yield stayed 851 bytes. Calls to
`_PyValue_Canonicalize` in those handlers changed from 3 to 1, 1 to 0, and
1 to 1, respectively. These are compiled-code observations, not a measured
throughput gain. The actual route probe and native assertions cover local
destination A=2/nl=3, temporary A=2/nl=2, and discard A=4095; they do not
establish an A=B route. The root-owned golden timing screen is still pending.
