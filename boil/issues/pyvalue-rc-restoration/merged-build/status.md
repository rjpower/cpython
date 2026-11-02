# Merged restored-runtime diagnostic builds

- Stage: latest make failed before linking; no restored execution or timing
- Owner: coordinator
- Purpose: expose actual baseline compiler/linker dependencies on frozen merged source

Latest attempt: [v10 record](../../../../build/pyvalue-merged-build-20261102-v10/results.json),
at checkpoint37 `ef48e73fb24` plus the held typeobject composition `6b42e1b2`.
Make exited 2 in 35.19 seconds after the public-layout change, with 164 objects
present,361 repeated/cascading errors and no executable. All7,344 inputs stayed
unchanged. This is a compiler probe, not candidate execution.

[Identity](../../../../build/pyvalue-merged-build-20261102-v10/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v10/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v10/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v10/source-after.json),
[summary](../../../../build/pyvalue-merged-build-20261102-v10/summary.json).

Previous attempt: [v9 record](../../../../build/pyvalue-merged-build-20261102-v9/results.json),
at checkpoint35 `aa46fc4fd01` plus the held reviewed typeobject state.
Make exited 2 in 2.89 seconds, with 164 objects present, 376 repeated/cascading
errors and no executable. All 7,343 source inputs stayed unchanged.
There are 320 distinct diagnostic sites in 59 source/header paths.

[v9 identity](../../../../build/pyvalue-merged-build-20261102-v9/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v9/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v9/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v9/source-after.json),
[summary](../../../../build/pyvalue-merged-build-20261102-v9/summary.json).
Counts include cascades and shared-header expansions, not independent edits.

Previous attempt: [v8 record](../../../../build/pyvalue-merged-build-20261102-v8/results.json),
at checkpoint32 `87ec5704d6c` plus the held reviewed typeobject state.
Make exited 2 in 2.99 seconds, with 164 objects present, 392 repeated/cascading
errors and no executable. All 7,342 source inputs stayed unchanged.

[v8 identity](../../../../build/pyvalue-merged-build-20261102-v8/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v8/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v8/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v8/source-after.json).
Counts include cascades and shared-header expansions, not independent edits.

Previous attempt: [v7 record](../../../../build/pyvalue-merged-build-20261102-v7/results.json),
at e0dfcd4d7ff plus then-unaccepted frozen TypeVar v2 and held reviewed typeobject.
Make exited 2 in 2.93 seconds, with 163 objects present, 403 repeated/cascading
errors and no executable. All 7,342 source inputs stayed unchanged. TypeVar v2
subsequently passed independent review and was committed as bef49ec8fc9.

[v7 identity](../../../../build/pyvalue-merged-build-20261102-v7/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v7/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v7/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v7/source-after.json),
[mechanical summary](../../../../build/pyvalue-merged-build-20261102-v7/summary.json).
There are 337 distinct diagnostic locations in 63 source/header paths. The
largest file is held typeobject.c (38 observations), followed by itertools (29).
Counts include cascades and shared-header expansions, not independent edits.

Previous attempt: [v6 record](../../../../build/pyvalue-merged-build-20261102-v6/results.json),
an early probe at 4bf934bb4ec plus then-unaccepted frozen unified Clinic48-v2
and held reviewed typeobject. Make exited 2 in 4.97 seconds, with 161 objects
present, 430 repeated/cascading errors and no executable. All 7,341 source paths
stayed unchanged. Clinic v2 subsequently passed independent code/results review
and was committed as 180b4b6d9af; the build itself is not runtime admission.

[v6 identity](../../../../build/pyvalue-merged-build-20261102-v6/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v6/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v6/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v6/source-after.json).

Previous attempt: [v5 record](../../../../build/pyvalue-merged-build-20261102-v5/results.json),
an early compiler probe at b1dce616174 plus the unaccepted frozen Clinic39-v1
and held reviewed typeobject endpoint. Make exited 2 in 4.00 seconds, with 160
objects present, 434 repeated/cascading errors and no executable. All 7,341
source paths stayed unchanged. The Clinic batch still needs finite code-review
corrections and production controls; this build neither accepts it nor executes
the candidate. Its new unused-nargs warnings are part of the correction packet.

[v5 identity](../../../../build/pyvalue-merged-build-20261102-v5/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v5/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v5/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v5/source-after.json).

Previous attempt: [v4 record](../../../../build/pyvalue-merged-build-20261102-v4/results.json),
source HEAD e645682dab5 plus held provider/core/type_new/graph and the corrected
MRO15 endpoint d0721997. Make exited 2 in 3.34 seconds, with 154 objects present,
519 repeated/cascading error diagnostics and no executable. All 7,339 source
paths stayed unchanged. Object counts include prior focused builds; error
counts are neither independent defects nor a progress/performance metric.

[v4 identity](../../../../build/pyvalue-merged-build-20261102-v4/identity.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v4/make.log),
[source before](../../../../build/pyvalue-merged-build-20261102-v4/source-before.json),
[source after](../../../../build/pyvalue-merged-build-20261102-v4/source-after.json).

Previous attempt: [v3 record](../../../../build/pyvalue-merged-build-20261102-v3/results.json),
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

The later graph, object-call, AST sequence and MRO ports are included in v4;
their focused checks do not turn either merged build into a pass.
Repeat merged validation at the next coherent integration point, then execute
startup/focused runtime cases once linking succeeds. Timing requires a working,
qualified register interpreter with unified values and JIT explicitly disabled.

Earlier preserved attempt [v2](../../../../build/pyvalue-merged-build-20261102-v2/results.json)
bound fifteen checkpoints at `6780ba211ac` plus three then-held object files;
make exited 2, 145 objects were present, all 7,332 source paths stayed unchanged,
and no executable linked. Its 612 repeated/cascading diagnostics remain raw evidence.
The [first diagnostic build](../first-build/status.md) is preserved separately.
