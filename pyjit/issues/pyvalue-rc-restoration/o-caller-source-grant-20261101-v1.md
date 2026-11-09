# Specialized O-call source successor

The coordinator authorizes the exact reviewed successor proposed in
`build/pyvalue-opaque-extension-api-design-20261101-v1/o-caller-successor.md`,
SHA256 `3a0f3f94f485061e7c435e9fe6e05f975711023c7288ec6763e9b40aaab47b9b`.
value_api_consumer_map independently accepts that source scope. restart_audit
implements and the same independent reviewer checks actual generated code.

This is a new 30-minute implementation slice for the user-requested native API
bridge. The original foundation froze early at receipt
`7cb1d9e6bbf50f685419babf84463abf9a7b7a42727cfc845ea1d33ca20d56dc`;
its source budget is closed, not extended. The foundation review is
`build/pyvalue-rc-native-consumer-map-20261101-v1/foundation-freeze-review.md`,
SHA256 `2a49f58db20fba5640f66fbbb9fd8338f318b760a78a1b94e60db9ffee555e51`.

The successor uses the conservative observed start **23:29:00 UTC**, checkpoint
**23:44:00**, ceiling **23:59:00**, all on 2026-11-01. The author's earlier
23:31 estimate was corrected against the clock before any successor edit.

## Exact implementation boundary

- `_CALL_BUILTIN_O` in Python/bytecodes.c; the generic route stays outside this cut.
- Narrow pure publication and forced post-cleanup reload helpers in Python/ceval.c.
- Only the needed SYNC_SP output/cache seam in generators_common.py/ir_generator.py,
  and their actually changed generated products. Preserve IDs and formats.
- Objects/methodobject.c: native-scope include only; C owns the new gateway.
- Build/test wiring: Makefile.pre.in, the exact _testinternalcapi source-list
  addition in Modules/Setup.stdlib.in, the declaration in
  Modules/_testinternalcapi/parts.h and initializer call in _testinternalcapi.c.
- Only the actual O-call VM fixture in test_pyvalue_native_scope.py, coordinated
  with its owner; any focused producer assertion belongs in existing generator tests.

Routine declarations and dependencies needed for this named fixture registration
are included; record additions for review without seeking another approval per
line. Other shared functions and the native-scope engine remain with their owners.

Publish the canonical CALL temporary/window, retain original callable/argument
owners in their existing semantic positions, and invalidate cached generator
outputs before callback-capable cleanup. A temporary-argument finalizer and bound
receiver finalizer must see old local x; the later displaced-x finalizer sees the
new result. Do not publish the folded local early or defer arbitrary original
owners to a new handler-end cleanup point.

After End, force state/epoch/frame/word revalidation even when the scalar temporary
record is empty. Resume with fresh decoded state. A cleanup error after a completed
call uses output-window liveness and never replays the body. Abort publishes the
unchanged input/error state and unwinds each owner once. A block-local scope cannot
escape its C lifetime.

## Evidence and remaining work

First review one actual generated O handler/helper for publication, stale-store
prevention, exact release order and error destinations. Then run affected host
tests and required regeneration, and at most four actual changed syntax-only
translation units at 30 seconds each. No object output, linking or candidate
execution is authorized here. Runtime tests still require one coherent candidate.

The native-scope component is independently source-accepted at receipt
`59fd5663af00c6a36f684ec418a5e3f2d26671887f37ac61287959fc2efe719e`.
That is engine/gateway component acceptance; it does not establish this caller
integration, extension conformance or universal restoration.

Canonical common constants, remaining core providers, slots, native families,
Clinic, bundled consumers and private import/cache identity remain separate
required cuts. This successor makes no whole-runtime or performance claim.
