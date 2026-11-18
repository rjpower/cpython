# Handlers75: smaller handlers and fewer variants

Golden73 `44f066f5fd5` spends about twice upstream's instructions on simple
operations ([execution74](execution74-diagnosis.md)), and code placement alone
moves single workloads by up to 13%. This workstream shrinks what each handler
executes and how many handlers exist. The aim is that layout matters less and
fixed per-handler costs fall. Round 1 removes fixed costs without changing the
IR. Round 2 replaces per-site handler variants with flag bits in the word.

Source: branch `pyjit/handlers75` in `BASE/worktrees/call55-ir`; builds
`BASE/handlers75-{release,debug}`. Evidence is under `E59` with prefixes
`handlers75-` (build and test manifests) and `execution75-` (measurements).

## Round 1: cold slow paths and leaf destruction

Commits `ade5e57240c`, `25793370a73` (generated cases) and `91c9baa9600`.

- **Cold slow paths.** `_Py_COLD_FUNCTION` (GCC `cold`) marks error
  processing, deferred cleanup that can run Python, and frame retirement through
  rooted state. GCC then moves its callers' paths into `.cold` sections.
- **Leaf destruction.** Releasing the last owner of an exact int, float,
  complex, str or bytes object cannot run Python. A result that displaces such
  a value from a local now frees it at once rather than deferring it to the
  end of the handler. `_PyValue_CloseMayReenter` replaces
  `_PyValue_MayDealloc`. The handler keeps only the last-owner test inline.
  The type test and destruction live in `ir_retire_last_owner()`.
- **No frame pointers in handlers.** `ceval.c` is compiled with
  `-fomit-frame-pointer -momit-leaf-frame-pointer` (`CFLAGS_HANDLERS`). Each
  tail-called handler paid a push, a move and a `leave` on every dispatch.
  `perf -g` frame-pointer unwinding now stops at the interpreter; DWARF
  unwinding still works.
- **`PyList_GET_VALUE` inline.** `FOR_ITER_LIST` called it out of line,
  with three spills.

Handler code (`_TAIL_CALL_*` symbols, release):

| | Golden73 | Round 1 |
| --- | ---: | ---: |
| Hot bytes | 299,868 | 282,691 (−5.7%) |
| Cold bytes | 0 | 44,341 in 266 fragments |
| `LOAD_FAST_BORROW` | 356 | 270 |
| `FOR_ITER_LIST` | 1,153 | 984 |
| `BINARY_OP_ADD_FLOAT` | 1,144 | 1,058 |
| `BINARY_OP_SUBSCR_DICT` | 1,868 | 2,114 |

`BINARY_OP_SUBSCR_DICT` grows because its ordered input retirement calls
`_PyValue_CloseMayReenter` on the key and the dict. That call now tests the
exact type of each one inline.

Qualification (`handlers75-r1b-{release,debug}-focus`): 80 release / 79 debug
modules, covering every `test_ir*` module and the generator, frame,
monitoring, GC, exception and dis suites. Four release and three debug
modules fail. All fail identically on golden73's build, so they predate this
round:

- `test_ir_classmethod_cache` pins magic 64422; the tree is at 64427.
- `test_ir_operand_modes.test_metadata_domain` compares two contract rows that
  differ in a handler id.
- `test_ir_error_effects` reports five subtest errors in
  `test_send_source_changes_revoke_the_whole_family`.
- `test_generated_cases` (release only):
  `test_abrupt_prefix_requires_complete_suspension_source` and
  `test_handlers_take_no_local_address`.

### Round 1 performance

Two-pair all-29 campaigns on CPU 15 under the core-local gate:

| Campaign | Geomean | Wins | Rejected invocations |
| --- | ---: | ---: | ---: |
| Round 1 / golden73 (`execution75-handlers-r1-all29-golden73b`) | **0.9973** | 13/29 | 28 |
| Round 1 / upstream (`execution75-handlers-r1-all29-upstream`) | **1.1236** | 7/29 | 25 |

Against golden73 the largest moves are spectral_norm 0.878, logging_silent
0.913, unpack_sequence 0.945, nbody 0.963 and go 0.972. The regressions are
coroutines 1.070, deepcopy_memo 1.052, deltablue 1.040, unpickle 1.033 and
richards 1.024. Every pair range lies within ±0.03 of its geomean.
`execution75-handlers-r1-all29-golden73` stopped at its last invocation, when
the sibling gate rejected five attempts (see [measurement](measurement.md)).
It is retained but not used.

PMU counts per hot loop, mean of two stat runs per role
(`execution75-handlers-r1-pmu`):

| Workload | Event | Upstream | Golden73 | Round 1 | R1/G73 |
| --- | --- | ---: | ---: | ---: | ---: |
| logging_silent | cycles | 4,753 | 3,784 | 3,407 | 0.900 |
| | instructions | 13,944 | 13,502 | 13,439 | 0.995 |
| | op-cache µops | 8,035 | 6,574 | 10,993 | 1.672 |
| deltablue | cycles | 15.69M | 17.33M | 17.87M | 1.031 |
| | instructions | 38.92M | 53.04M | 52.37M | 0.987 |
| | op-cache µops | 22.25M | 27.24M | 21.72M | 0.797 |
| coroutines | cycles | 108.7M | 125.5M | 134.7M | 1.073 |
| | instructions | 294.9M | 378.6M | 381.6M | 1.008 |
| | op-cache µops | 125.1M | 117.0M | 54.2M | 0.463 |
| deepcopy_memo | cycles | 124.1K | 153.2K | 161.5K | 1.055 |
| | instructions | 346.9K | 459.4K | 453.8K | 0.988 |
| | op-cache µops | 105.2K | 123.1K | 68.6K | 0.557 |

Round 1 retires 0.5–1.3% fewer instructions, except coroutines (+0.8%).
Branch mispredictions are unchanged. Each regression coincides with a
collapse in op-cache delivery, and logging's gain coincides with its
delivery returning. Placement still decides single workloads in both directions.
Removing fixed work does not stop that; a smaller, denser hot set should.
Round 2 targets that.

## Round 2: word flags instead of handler variants

Today a per-site fact becomes another handler. Examples: whether a call has a
self argument (35 `*_NO_SELF*` handlers, 27 KB), whether a destination is a
local (`*_LOCAL_TEMP`), or which consumer receives a generator's completion.
Separately, each handler decides whether its destination is a local with two
dependent loads from the code object.

Round 2 splits the 16-bit handler field into a 9-bit handler id and 7 flag
bits. Dispatch masks the id, so the table keeps 512 entries.

- Slot-class flags, one per field, say whether the field names a local. They
  depend only on the fields and `nlocalsplus`, so `make_exec` derives them for
  the executing copy. The destination and input tests then read the word in
  the handler's register.
- Call-role flags (`NO_SELF`, `NO_KW`) are compiler proofs, so the encoder
  writes them into canonical words. The 35 NO_SELF handlers are deleted, and
  call handlers derive their skipped homes from the word.

Executable words keep physical slots; logical values belong to the compiler
before slot allocation. Follow-ups are a single generic `BINARY_OP` (26
identical 1,027-byte bodies today) and one generator-completion handler
whose consumer is a word code.
