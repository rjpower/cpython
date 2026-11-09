# First universal-value diagnostic build

- Outcome: configure passed; compile/link attempt failed (make exit 2, no timeout)
- Owner: coordinator; source frozen throughout the bounded build
- Source: HEAD c21d0a374ee plus frozen object/type, codegen and sys.flags changes

The out-of-tree debug configuration matches interim CC=gcc, CFLAGS=-O1 -g,
mimalloc/computed-goto/shapes options and selects `boilvalrc1-316d-x86_64-linux-gnu`.
`make -k -j4 python` completed before its 240-second bound, producing 141 objects,
including symtable, codegen and sysmodule. It produced no interpreter. All 7,328
source manifest paths were unchanged. Build products stay in the reusable
project build directory; raw logs and hashes stay with this experiment.

There are 616 diagnostics across 96 source/header paths, including repeated and
cascading errors, not 616 independent bugs. Repeated old tuple/list APIs and
Clinic object-vector assumptions dominate. Dictionary legacy bodies, type/object
boundaries, marshal/errors/AST and bundled consumers also need ports. The two
new high-fanout design assignments are dictionary closure and scoped Clinic
argument lifetimes. No fake APIs or boxed alternate container was introduced.

[Design/configuration](../../../../build/pyvalue-first-build-20261102-v1/design.md),
[results](../../../../build/pyvalue-first-build-20261102-v1/results.json),
[grouped diagnostics](../../../../build/pyvalue-first-build-20261102-v1/compile-diagnostics.json),
[raw make log](../../../../build/pyvalue-first-build-20261102-v1/make.log).

This attempt is development feedback. No candidate was linked or executed;
runtime/ABI admission, JIT-disabled verification and benchmarks remain pending.
