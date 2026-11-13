# GC verifier qualification triage

The release full-suite failure reproduces intermittently on the unchanged
selected51 control. Root's twelve alternating targeted executions produced
four failures and two passes for **each** executable. This establishes
preexisting diagnostic instability; it does not establish the precise source
of the conservative root or excuse any separate ownership failure.

No runtime source change, quarantine, skip or weakened assertion is proposed.
This worker inspected source and root's retained logs only, and ran no native
interpreter, test, build, generator or inferior.

## Matching paths and observed results

BASE is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
E is `BASE/pyjit-link-wave1/frame-state54`.

| Role | Source HEAD | Executable |
| --- | --- | --- |
| selected51 | `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` | `BASE/round51-golden-release/python` |
| cold correction | `61015a9452d9069646fb5fede9ff963896833b89` | `BASE/frame54-cold-construction-release/python` |

The command journal is `E/gc-verifier-repeat-commands.json`; each executable
was invoked from its corresponding build directory with
`-B -m test -j1 --match '*test_hole_list_is_kept' test_gc_verify`.
Tracebacks resolve to the corresponding source worktree. These are root-run
matching-control diagnostics, not this worker's execution or a timing sample.

| Alternating pair | selected51 | cold correction |
| --- | --- | --- |
| 1 | fail | fail |
| 2 | fail | pass |
| 3 | pass | fail |
| 4 | pass | fail |
| 5 | fail | fail |
| 6 | fail | pass |

Failures exit 2, passes exit 0. All failures have the same helper assertion,
`the object is not reported as a hole: []`. Per-run logs are
`E/gc-verifier-{golden51,cold}-repeat{1..6}.log`.

The original full-suite evidence is
`E/cold-construction-release-full-release-full.log:629` onward. Its stderr
reports one intentional string hole, but the traceback is at
`test_gc_verify.py:206 -> verify_pymem_array:185`. The helper exhausted five
attempts without observing its target as a hole. It failed **before**
`test_hole_list_is_kept` compared `gc_verify_holes()` with the returned list.
Therefore the failure is not evidence that saved holes disagreed with the
same verification's return value. The module's separately successful
`test_pymem_array_is_a_hole` can emit that one-hole report; the full-module
stderr alone does not identify which verification emitted it.

## Source explanation and its limit

`Lib/test/test_gc_verify.py`, `Modules/_testinternalcapi.c`,
`Python/gc_trace.c`, interpreter-frame headers and `Python/frame.c` are
unchanged against selected51. The cold candidate's only three changed files
remain its public-constructor helper/wrappers and their direct C API tests.
This verifier test does not call the newly added construction helper.

The negative-hole fixture deliberately hides a strong reference from the
verifier. `store_in_hidden_array` stores a fresh string in array[2] of a
four-pointer PyMem allocation, keeping the array address only XOR-masked in a
static integer. `clear_dead_stack` overwrites 16 KiB below its caller before
verification. The helper decodes the array only afterward, returns its owner
to Python and frees the array. The test keeps returned objects across five
attempts to prevent object-address reuse.

The verifier intentionally recognizes conservative roots. It captures current
registers and scans the live C stack; an interior pointer into an object marks
that object. A stack/register pointer into a PyMem block also queues that block
for scanning, which can mark the string stored in it. An incidental root thus
turns this deliberately hidden object into a reached object, yielding `[]`
without implying a missing owner or broken reference count.

The fixture's own comment acknowledges stale object addresses in live C
frames. A further plausible limitation is stale **array** addresses: varying
the object address does not necessarily vary the freed/reused four-pointer
PyMem block. A stale raw pointer to that block can expose a new object on a
later attempt. Clearing dead stack below the caller does not clear arbitrary
live caller spills or saved registers. These are source-supported mechanisms,
not identification of the exact pointer responsible for any retained failure.
No claim of a demonstrated compiler spill or particular allocator reuse is
made. Likewise, the equal small-sample failure counts do not establish equal
failure probabilities across binaries.

## Bounded optional follow-up

The paired targeted control runs already answer the immediate regression
question. If exact root attribution is needed separately, root can repeat only
this negative-hole test with `PYTHON_GCVERIFY_ROOTS=precise`, retaining both
default and diagnostic-root-set logs for both identities. This removes
conservative root classes; a stable result would support conservative-root
sensitivity but would still not identify a stack word. Do not apply that
environment to qualification generally: it changes what the verifier checks,
and other tests deliberately require conservative C roots.

Only if further investigation is warranted, authorize a separate diagnostic
probe of the existing root scanner recording whether the target string was
marked directly or through a PyMem block. Keep it outside this cold fix and
outside timing. No such probe or production fix is part of this handoff.
