# Full stats census at af82c03c4e5

This census folds 6,513 dumps and records **14,211,307 debt boxes** across
core, core-owned adapters and unresolved owners. It is failing admission
evidence. Coverage and source differ from earlier censuses; these totals do
not establish a controlled performance improvement or percentage reduction.

There are zero invalid owners, zero owner-table drops and no unreadable dumps.
Two deliberate `test_sys` resets remain. Process accounting retains 545
unfinished markers and 423 uncounted launches; one marker is empty and therefore
fails the strict argv decoder. The exact-shape rules remain empty. No unknown
shape is admitted merely because its test tag is allowlisted.

The functional run fails in `test_pyvalue_operator_readers`: the new operator
instance-vectorcall types supplied a type-call thunk without its required
type-level value-call row. The reviewed correction uses `PyVectorcall_Call`,
whose existing registration in `Objects/value_pairs.c` supplies
`_PyValue_InstanceVectorcall` as the paired value-call row. It must pass the
existing direct `__call__` regressions and merged checks before admission.

| Core-owned unit | Debt boxes |
| --- | ---: |
| `Objects/call.c` | 2,284,775 |
| `Modules/_ctypes/_ctypes.c` | 1,942,348 |
| `Objects/dictobject.c` | 1,411,496 |
| `Python/bltinmodule.c` | 1,369,896 |
| `Objects/typeobject.c` | 1,285,935 |
| `Modules/_functoolsmodule.c` | 697,917 |
| `Objects/unicodeobject.c` | 689,067 |
| `Modules/_sre/sre.c` | 577,453 |
| `Modules/timemodule.c` | 386,393 |
| `Modules/mathintegermodule.c` | 370,203 |
| `Objects/descrobject.c` | 323,828 |
| `Modules/selectmodule.c` | 293,562 |
| `Objects/longobject.c` | 250,692 |
| `Python/native_scope.c` | 230,129 |
| `Modules/itertoolsmodule.c` | 229,432 |

Kind subtotals within debt are core 4,429,383; thunk 3,900,840; slot adapter
3,089,001; call adapter 2,792,083. They sum to the debt total above.

Evidence is under the external runtime build root at
`pyjit-link-wave1/checkpoint-closure/suite-af82c03c4e5-census/`. That directory
retains the clean source identity, exact stats executable and ELF notes,
functional log, strict gate output and resolved summary. The retained
executable prevents later relinks from invalidating source attribution.

The remaining queue is constructor/call continuations, reviewed formatting and
I/O packets, ctypes and LRU ownership (implementation awaits explicit approval),
finite API/provider and copied-slot closure, and complete process accounting.
The checkpoint and its qualified performance comparison remain open.
