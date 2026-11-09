# Full stats census at 7baf96ed017

The counted integration run ran 54,309 tests and folded 6,456 dumps with no unreadable dumps, owner-table drops or stray compilation units. Regrtest exits 2: the inspect suite has three errors because the newly value-native property getter/setter/deleter methods lack their upstream text signatures. A control-verified signature patch is queued. This is a failing qualification run.

Strict debt is **25,119,440 boxes** across core, core-owned adapters and unresolved owners. Kind totals are core 13,062,688, thunk 5,407,389, slot adapter 4,200,827 and call adapter 3,428,543; kind totals also include legitimate non-core rows and are not themselves the debt sum. Coverage and source differ from prior censuses, so this report does not claim a controlled percentage improvement.

Accounting retains 586 unfinished markers, 423 uncounted launches and the two expected test_sys resets. The environment-probe change removes the previously diagnosed repeated uncaptured probe mechanism. Exact remaining mechanisms and counts still need admission validation; tag-only allowlists are insufficient. One invalid owner remains: test_xxlimited, legacy attribute slot adapter at 0+0x21db80, owner `-`. Source diagnosis finds the adapter records NULL tp_setattro when it actually calls tp_setattr; the paired getter has the same latent problem. A narrow selected-slot attribution repair is queued, without changing counts or allowances.

| Core-owned unit | Debt boxes |
| --- | ---: |
| `Modules/_collectionsmodule.c` | 3,771,760 |
| `Python/bltinmodule.c` | 2,937,307 |
| `Modules/_ctypes/_ctypes.c` | 2,846,736 |
| `Objects/call.c` | 2,503,527 |
| `Objects/dictobject.c` | 2,092,702 |
| `Objects/typeobject.c` | 1,283,103 |
| `Modules/mathmodule.c` | 1,018,123 |
| `Objects/longobject.c` | 961,866 |
| `Modules/mathintegermodule.c` | 935,369 |
| `Modules/_operator.c` | 750,863 |
| `Modules/_functoolsmodule.c` | 736,207 |
| `Objects/unicodeobject.c` | 700,993 |
| `Modules/_sre/sre.c` | 577,415 |
| `Modules/selectmodule.c` | 455,125 |
| `Modules/_remote_debugging/binary_io_writer.c` | 435,233 |
| `Modules/timemodule.c` | 384,100 |
| `Objects/descrobject.c` | 323,940 |
| `Modules/itertoolsmodule.c` | 236,654 |
| `Python/native_scope.c` | 229,769 |
| `Objects/frameobject.c` | 185,145 |
| `Python/getargs.c` | 163,424 |
| `Objects/abstract.c` | 141,109 |
| `Objects/sliceobject.c` | 133,825 |
| `Modules/posixmodule.c` | 128,283 |
| `Modules/_pickle.c` | 116,076 |

Next reviewed packets target tuplegetter (3,758,102), id audit tuple arguments (1,568,829), value import/ModuleSpec continuations, deferred annotations, dictionary/memoryview iterator consumers, property signatures and accurate legacy-slot owner attribution. Long subtype/base-copy and saved paired iterator consumers follow. Ctypes needs explicit approval after automatic review blocked its raw foreign-pointer ownership transaction; its incomplete WIP is not integrated.

Evidence is retained under the external runtime build root at `pyjit-link-wave1/checkpoint-closure/suite-7baf96ed017-census/`, including exact source identity, full suite log, strict summary, and inspect reproducer. The checkpoint remains open and current performance has not been measured.
