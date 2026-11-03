# Canonical PyValue foundation checkpoint

- Stage: source committed on the restoration branch; dependent, non-runnable
- Owner: coordinator; fresh independent checkpoint reviewer: wiki_audit
- Commit: `359ea4732de4681c6f93396ca0c8531c3a59d77b`, parent `95be1d28a12`
- Goal: checkpoint the reviewed canonical value/frame foundation without
  overwriting later container, compiler and native work.

The commit applies the frozen foundation-owned patch to the index only:
78 changed paths representing 82 owned manifest entries, excluding provider/
shared context-only files and the separate special-method lookup fragment.
Three Windows files pass through their declared Git CRLF clean filters; their
working bytes remain unchanged. All 152 working source paths are preserved.

The earlier component acceptance binds 39 host generator checks, seven
generators and six syntax-only TUs. This checkpoint reuses that scoped evidence;
it has no linked candidate or runtime pass. Storage/operation/native dependencies
still need subsequent commits before coherent build and integration.

Evidence: [commit receipt](../../../../build/pyvalue-source-integration-20261102-v1/foundation-commit.json),
[independent review](../../../../build/pyvalue-source-integration-20261102-v1/checkpoint-review.md),
[Git clean-filter bridge](../../../../build/pyvalue-source-integration-20261102-v1/foundation-clean-filter-bridge.json),
[original component review](../../../../build/pyvalue-rc-native-consumer-map-20261101-v1/foundation-freeze-review.md).

Next: [coordinator integration](../integration/status.md) commits the reviewed
storage/providers and shared composition, then validates the merged runtime.
