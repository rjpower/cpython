# Canonical compiler metadata dictionaries

- Stage: source reviewed; commit/integration and candidate execution pending
- Owner: coordinator; previous implementation/review assignments are closed
- Hypothesis: compiler flags and indices can use canonical values directly
  without borrowed scalar boxes or changed scope/index behavior.

Exactly six functions in compile.c, assemble.c and flowgraph.c plus two includes
use owned reads/iteration, inline integer conversion, explicit cleanup and
post-allocation bounds. All other source matches the preserved baseline.

[Author receipt](../../../../build/pyvalue-compiler-dict-consumers-source-20261102-v1/checkpoint-v1/receipt.json)
`61a1d727`; [independent review](../../../../build/pyvalue-rc-native-consumer-map-20261101-v1/compiler-dict-consumer-component-review-receipt.json)
`39a69223`. Three actual full TUs pass syntax. Four host-reference controls pass;
the four candidate methods remain unexecuted. Symtable/codegen/marshal are not
closed by this increment.

Next: commit the reviewed delta with dependencies, then run its focused cases
on the coordinator's coherent candidate. See [integration](../integration/status.md).
