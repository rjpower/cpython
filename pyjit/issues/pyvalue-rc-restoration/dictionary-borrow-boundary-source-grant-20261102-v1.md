# Dictionary boundary providers and owned iteration bridge

Adopt design51e56173/receipt74ad5177 and independent design review1790b048
in build/pyvalue-dict-borrow-boundary-design-20261102-v1 and
build/pyvalue-rc-native-consumer-map-20261101-v1. C implements; D reviews.

Conservative start2026-11-02T03:32:40Z, checkpoint03:52:40Z,
ceiling04:32:40Z. Record actual first edit. Concrete checkpoint must contain
actual checked borrowed boundaries and owned NextEntryRef adapter, not headers
alone. Previously accepted storage/readers remain frozen outside exact seams.

Scope: the six retained failing dictionary bodies and their explicit aliases;
additive owned entry adapters/declarations; _PyDict_HasOnlyStringKeys;
focused existing dictionary fixture/tests. Keep all symbols/signatures.
Object-only borrows require proven pure metadata access or final current-entry
revalidation after callback-bearing cleanup. No temporary borrowed scalar boxes,
box cache, TLS owner engine, second core representation, or removal script.

Inline results in retained borrowed APIs get the design's explicit unsupported
TypeError policy. Old Boolean Next returns0/error without cursor advance or
partial outputs, never-1. It is not general rebuilt-source compatibility.
The borrowed SetDefault result branch rejects before hashing/mutation.
All affected core and bundled-module consumers must migrate before candidate
qualification; this grant does not claim their compatibility or port them.

Owned NextEntryRef returns-1/0/1 with independent owned snapshots and NULL
failure outputs. Reserve cursor before boxing, publish no partial pair, preserve
nested progress and primary errors. Complete final dictionary-owner cleanup
before output transfer; never overwrite later cursor progress afterward.
No callback-capable raw table pointer survives allocation or owner retirement.

Review ownership/status/lifetime first, then focused AST checks and actual
Dict/changed-fixture syntax with real headers, at most30seconds each. Preserve
residual diagnostics. No object/link/candidate/runtime/timing. Signal-diagnostic
raw dict/set cursors, compiler/runtime/bundled consumers and parser registration
remain separate exact source cuts.
