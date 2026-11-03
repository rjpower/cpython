# Owning caller transaction

- Stage: independently reviewed and committed as `6780ba211ac9a242dee156e18572be2c246cfa03`
- Author: state_audit; independent review and integration: coordinator
- Scope: ten evaluator/bytecode/producer/product/observer/test paths

One numeric call record precedes preparation. Final input homes publish before
obsolete method/iterable retirement. NativeScope publishes to the actual result
destination before final input/result-box cleanup. Effects prohibit replay.
Python binding consumes independent copies using one pinned code snapshot;
actual child handoff precedes retiring parent duplicates. Error rollback restores
the parent before clearing the unentered child.

Analyzer properties expose real transaction callouts/errors/frame changes.
The producer removes stale trailing flush/moves and preserves all 293 IR formats.
Four generator commands reproduce output twice. Fifty host generator tests and
three ordinary reference controls pass. Full evaluator/fixture TUs pass and
actual ceval.o compiles. Eighteen warnings remain: fifteen generated variables
in changed handlers and three preexisting/unrelated warnings. Earlier failures
and their fixes remain in the record.

Eight candidate methods are unexecuted. Exact-argument preparation, the
constructor-init shim, broader binder cleanup, further parser families and
allocator/rollback fault witnesses remain separate qualification work.
The merged diagnostic build still has no executable; this is source acceptance.

[Design amendment](../python-transfer.md),
[independent code/results review](../../../../../build/pyvalue-native-call-transaction-20261102-v1/caller-independent-review.md),
[scope/results](../../../../../build/pyvalue-native-call-transaction-20261102-v1/receipt.md),
[actual native checks](../../../../../build/pyvalue-native-call-transaction-20261102-v1/caller-native-v2.json),
[actual object](../../../../../build/pyvalue-native-call-transaction-20261102-v1/caller-object-v2.json),
[commit](../../../../../build/pyvalue-native-call-transaction-20261102-v1/caller-commit.json).
