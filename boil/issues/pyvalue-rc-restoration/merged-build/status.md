# Merged restored-runtime diagnostic builds

- Stage: latest make failed before linking; no restored execution or timing
- Owner: coordinator
- Purpose: expose actual baseline compiler/linker dependencies on frozen merged source

Latest attempt: [v3 record](../../../../build/pyvalue-merged-build-20261102-v3/results.json),
source HEAD `7b923d2b935` plus the held typeobject provider/core1 and applied
single-function type_new repair. `make -k -j4 python` exited 2 in 33.7 seconds.
There are 152 objects present, including earlier focused builds; this is not a
count of newly compiled objects. There is no interpreter executable. All 7,337
source paths remained unchanged.

The log contains 540 repeated/cascading error diagnostics. Their count is neither
a count of independent defects nor a performance/progress metric. Concrete
remaining consumers include core tuple/list/dict reads, generated AST, format
parsers, type/MRO/slots, lifecycle/import and bundled modules. Select finite
provider/caller ports from these actual contexts rather than repeating inventories.

[Identity](../../../../build/pyvalue-merged-build-20261102-v3/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v3/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v3/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v3/source-after.json).

Later graph and object-call ports are outside that frozen build. Their focused
checks are linked from the board; they do not turn v3 into a passing merged build.
Repeat merged validation at the next coherent integration point, then execute
startup/focused runtime cases once linking succeeds. Timing requires a working,
qualified register interpreter with unified values and JIT explicitly disabled.

Earlier preserved attempt [v2](../../../../build/pyvalue-merged-build-20261102-v2/results.json)
bound fifteen checkpoints at `6780ba211ac` plus three then-held object files;
make exited 2, 145 objects were present, all 7,332 source paths stayed unchanged,
and no executable linked. Its 612 repeated/cascading diagnostics remain raw evidence.
The [first diagnostic build](../first-build/status.md) is preserved separately.
