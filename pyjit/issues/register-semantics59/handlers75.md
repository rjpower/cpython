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

Before this round, each per-site fact became another handler. Whether a call
has a self argument took 35 `*_NO_SELF*` handlers (27 KB). Whether a
destination is a local took `*_LOCAL_TEMP` variants. Which consumer receives a
generator's completion took one variant per consumer. Separately, each handler
decided whether a field names a local with two dependent loads from the code
object.

Branch `pyjit/handlers75-r2` in `BASE/worktrees/handlers75-r2`, from round 1;
builds `BASE/handlers75r2-{release,debug}`. Magic 64428.

- `eb68d15190e` splits the 16-bit handler field into a 9-bit handler id
  (`IR_HANDLER_MASK`) and seven flag bits. Dispatch indexes a 512-entry table
  with the id. Specialization, instrumentation, the free-threaded CAS and
  generator boundary placement rewrite only the id. dis and the tests mask
  with constants that `_opcode` exports.
- `769514b07c7` adds slot-class flags `IR_F_LOCAL_A..D`: field X names a slot
  below `co_nlocalsplus`. They depend only on the fields and `nlocalsplus`, so
  `make_exec` derives them for the executing copy. Code objects and .pyc files
  never carry them. The result and input macros take the word and field letter
  and test the flag in the handler's register. A debug assertion checks each
  test against the old comparison.
- `f78c3902c1b` adds call-role flags `IR_F_NO_SELF` and `IR_F_NO_KW`. They are
  compiler proofs, so the assembler writes them into canonical words.
  `_PyIRFormat.word_flags` lists the flags each handler admits, and validators
  reject any other flag. Call handlers derive their virtual inputs from the
  word (`_PyIR_CallInputHoles`). The 35 NO_SELF ids, their role tables and
  their generator families are removed. dis and `get_ir` name a flagged word
  `CALL+NO_SELF`. Bit 15 is reserved.

| Release `_TAIL_CALL_*` | Golden73 | Round 1 | Round 2 |
| --- | ---: | ---: | ---: |
| Hot bytes | 299,868 | 282,691 | 264,211 |
| Handlers | 368 | 368 | 333 |
| Cold bytes | 0 | 44,341 | 41,196 |

Step 1 adds 2.9 KB (`and $0x1ff` replaces `movzwl` at each dispatch) and step
2 adds 3.8 KB. Each flag test is a 6-byte `test $imm,%reg`, where the old code
reused one loaded `nlocalsplus` in 2-byte compares, and GCC's inlining shifted:
`STORE_NAME` now inlines `_PyEval_StoreName` (252 → 1,108 bytes). The
fast paths lose two dependent loads per test; field decoding also improved,
with one `shr` and `and $0x7ff8` producing the scaled slot offset. Step 3
removes 25 KB.

Qualification (`handlers75-r2-{release,debug}-focus`): 83 release / 82 debug
modules, adding `test_import`, `test_compile` and `test_peepholer`. Only the
pre-existing `test_ir_error_effects` (five subtests) and, in release, the two
`test_generated_cases` methods fail. Round 2 fixes the two stale tests.

### Round 2 performance

| Campaign | Geomean | Wins | Rejected invocations | Round 1 |
| --- | ---: | ---: | ---: | ---: |
| Round 2 / golden73 (`execution75-handlers-r2-all29-golden73`) | **1.0088** | 10/29 | 7 | 0.9973 |
| Round 2 / upstream (`execution75-handlers-r2-all29-upstream`) | **1.1383** | 7/29 | 22 | 1.1236 |

Single workloads swing again in both directions. Against golden73,
spectral_norm goes from 0.878 to 1.043, logging_silent from 0.913 to 1.048 and
generators from 0.987 to 1.030. In the other direction, unpickle goes from 1.033
to 0.986, deepcopy_memo from 1.052 to 1.015 and comprehensions from 1.008 to
0.979. Coroutines (1.078) and DeltaBlue (1.048) remain the largest
regressions.

PMU counts per hot loop (`execution75-handlers-r2-pmu`, four roles in one
campaign):

| Workload | Event | Golden73 | Round 1 | Round 2 | R2/R1 |
| --- | --- | ---: | ---: | ---: | ---: |
| logging_silent | cycles | 3,753 | 3,417 | 3,926 | 1.149 |
| | instructions | 13,502 | 13,439 | 13,760 | 1.024 |
| | op-cache µops | 6,627 | 10,887 | 6,810 | 0.626 |
| deltablue | cycles | 17.36M | 17.74M | 18.01M | 1.016 |
| | instructions | 53.04M | 52.37M | 52.76M | 1.008 |
| | op-cache µops | 28.65M | 21.51M | 18.76M | 0.872 |
| coroutines | cycles | 125.4M | 134.6M | 134.5M | 0.999 |
| | instructions | 378.6M | 381.6M | 383.2M | 1.004 |
| | op-cache µops | 117.1M | 55.7M | 74.2M | 1.331 |
| deepcopy_memo | cycles | 153.2K | 161.3K | 155.6K | 0.965 |
| | instructions | 459.5K | 453.9K | 460.5K | 1.015 |
| | op-cache µops | 123.0K | 71.7K | 102.3K | 1.426 |

Round 2 retires 0.4–2.4% more instructions than round 1. Flag tests replace
loads one for one, so the increase is elsewhere. Flat instruction samples
cannot place it: at the kernel's 11,000/s cap the counts track time, and
Zen 2's skid moves dispatch samples into the next handler. AMD IBS op sampling
would attribute it precisely. Cycles again follow op-cache delivery.
Removing 25 KB of handler variants did not stabilize placement, which the
footprint below explains.

## Where the hot code is

Total handler bytes are not what the op cache sees.
`E59/execution75-footprint` counts the distinct 64-byte lines that hold 90%
of retired instructions, from long sampled runs on CPU 2:

| Workload | Golden73 | Round 1 | Round 2 | Upstream |
| --- | ---: | ---: | ---: | ---: |
| coroutines | 119 | 123 | 131 | 88 |
| deltablue | 435 | 413 | 404 | 254 |

Zen 2's op cache holds 4K µops, on the order of 500 lines. DeltaBlue's hot
set sits near that capacity; upstream's sits well below it. That fits the
placement sensitivity measured since execution74.

In round 2's DeltaBlue set, 162 lines are handlers, against 98 for upstream's
entire `_PyEval_EvalFrameDefault`. The other 244 are runtime helpers, against
139 for upstream. 166 of those helper lines are in functions with no hot
upstream counterpart:

- **Call transactions and physical transfer:** `call_transaction_native`,
  `_run_vector`, `_overlap_python` and `_PyIR_NormalPhysicalTransfer`, about
  43 lines.
- **Value-form wrappers of object operations:** `_PyValue_GetMethod`,
  `_PyValue_RichCompare`, `PyDict_GetValue`, `type_call_value`,
  `method_value_vectorcall_descr`. Each is hot next to the object form it
  shadows.
- **Frame clearing:** `_PyFrame_TryClearExceptCode` alone takes 5.6% of
  samples.

Handler variants are therefore the smaller part of the excess. Collapsing the
remaining duplicate handlers saves about 25 KB of generic `BINARY_OP` bodies
(identical except for a constant) and about 5 KB of generator completion and
retirement variants (`GEN_*COMPLETE_STORE`/`_STORE_FAST`, `_DISCARD`/`_POP_TOP`,
three identical `GEN_RETIRE_EFFECT_*`). The larger lever is the call/transfer path and the
value/object duplication in the runtime.
