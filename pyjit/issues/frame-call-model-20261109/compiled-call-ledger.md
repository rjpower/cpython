# Compiled call ledger

Read-only inspection on 2026-11-09. No native Python, build, test or performance
job was run by this worker. Source changes remain with the coordinator.

The stable candidate inspected at completion is
`build/execution-boundaries-release/python`, build ID
`fe1fa15416240abd51052c83534cffd4e6af94a2` (root's build6).
The frozen comparison is `build/pyvalue-rc-repr-release/python`, build ID
`61fbf1bb863cc2c72050228cc7704979a7779589`. Paths are relative to the
python-boil-the-ocean worktree. During inspection root replaced candidate
build5 (`ed24233b44b846c51e1388b12e2cb982d7200caa`) with build6. Root reports
that only compiler/source metadata changed; the frame/call C and generator
inputs did not. Counts below were recomputed against build6 and use symbol-
relative positions. They do not identify a sealed performance candidate.

## What the machine code establishes

`nm` finds no `call_bind_known_python` symbol. `_TAIL_CALL_CALL_PY_GENERAL`
contains the binder and overlapping frame initialization inline. On the direct
virtual-kwargs hit it calls neither a generic transaction helper nor
`PyDict_New`. Its miss arm calls `call_transaction_run_vector`. This is an
actual removal of the wrapper from hits, rather than an inline annotation
without effect.

The old general handler calls
`call_transaction_run_guarded_python_vector.constprop.0`, which constructs a
stack transaction, saves/checks a stack canary, calls
`call_transaction_overlap_python`, then unpacks its status/child result. The
old overlap route calls `PyDict_New` for empty kwargs, calls
`call_transaction_refresh`, revalidates callable/code/provider/stack facts,
and pins/unpins code around allocation. Those operations are absent from the
new virtual hit. Required owning frame initialization, code ownership,
argument ownership and the eventual frame retirement remain.

The exact handler still implements its existing owning overlap. It now saves
a real continuation pointer and initializes the enlarged header. It has not
acquired the general transaction wrapper. The frame header is 112 bytes in
these generated x86-64 paths versus 104 bytes in the frozen comparison.

The new canonical RETURN_VALUE has no long/float type comparisons or call to
`_PyValue_Canonicalize`. It holds its result across `_PyEval_FrameClearAndPop`,
loads the saved continuation and checks the pending-redirect bit before the
ordinary result recipient. That new ordinary bit check must be charged against
the removed publication work. The cold redirect helper is not entered on
ordinary returns. The old return derives its continuation from the call PC and
performs generic publication/type checks.

Frame clear still scans named/local homes, including current constant homes.
It already excludes ordinary thread-frame temporary homes, relying on their
NULL invariant. It is therefore wrong to credit this increment with removing
an existing full temporary-bank retirement scan. Frame-object escape,
generator ownership, real object decrefs and deallocators retain their own
paths.

`_TAIL_CALL_LOAD_ATTR_METHOD_NO_DICT` is 0x2d0 bytes in both ELFs and has the
same substantive checks/copy/publication sequence. Its cached method object
is still checked for number conversion, duplicated, then checked again during
publication; its receiver is also generically published. The unchanged size
is not itself a dynamic cost measurement. This representative method handler
is not asserted to be the logging workload's selected method variant.

## Conditional route counts

Counts are x86-64 instructions encountered along explicitly selected control-
flow routes, obtained from `objdump -d --no-show-raw-insn`. Calls count as one
instruction; callee bodies are excluded unless separately listed. They are not
cycles, PMU observations, whole-function instruction counts or elapsed-time
predictions. Entering `start_frame`, the callee body, frame-clear internals,
allocator/deallocator bodies and instrumentation are outside these rows.

| Selected route | Frozen | Candidate | Interpretation |
| --- | ---: | ---: | --- |
| General Python entry, below | 432 | 198 | 234 fewer instructions outside the removed dict allocator |
| Exact Python entry, below | 154 | 158 | Four more instructions on this matched route |
| Temporary None return to ignored recipient, ordinary caller | 56 | 49 | Seven fewer; common frame-clear callee excluded |
| Method-no-dict hit with mortal cached descriptor and mortal local receiver | 108 | 108 | No change on this representative route |

The general row assumes: specialization/version/recursion guards hit; thread-
owned caller; bound receiver present; all positional parameters supplied; no
missing defaults; no constant-home initialization; sufficient frame-stack
capacity and no new tail requiring zero fill; mortal code owner; empty varargs;
compiler-qualified virtual kwargs; no PEP 523 hook or observation. These are
valid route assumptions, not a claim that every logging frame satisfies all
of them. Actual constant counts, stack growth, code ownership tags and entered
routes must come from the coordinator's workload evidence.

The frozen total is 58 instructions in the handler, 40 in its guarded wrapper,
305 in the selected overlap route, and 29 in the successful refresh. The
`PyDict_New` instruction is included in the overlap's 305; its allocator body
is excluded. The candidate total includes its whole selected hit through the
jump to `start_frame`, with no helper calls. Thus the allocator is not counted
twice and deleting the wrapper alone is not credited with all 234.

For reproduction, conditional branches selected as taken are these symbol-
relative offsets; other conditional branches fall through, and unconditional
jumps are followed. Stop at return or a tail jump outside the symbol:

- Candidate general: `+f7,+198,+272,+300,+363`.
- Frozen general handler: `+cb`; guarded wrapper: none; overlap:
  `+87,+12d,+2f2,+41a,+470,+4b5,+5d2,+6e8`; refresh: none.
- Candidate exact: `+186,+20c,+28b`; frozen exact: `+182,+201,+274`.
- Candidate return: `+88,+1c6`; frozen return: `+8b,+184`.
- Method-no-dict local receiver: `+d9` in either ELF.

The None return is a canonical temporary, with no generator completion or
pending redirect. A local input may require a duplicate. An inline scalar
return avoids much of the old object-publication path already, so its net
change need not equal the None row. A mortal returned/displaced object can
enter refcount/deallocator paths absent from that row.

On the candidate ordinary clear loop, each NULL local costs seven instructions
(load, decrement cursor, clear, test/branch, loop compare/branch). An immortal
or tagged non-NULL home costs nine; a nonlast mortal costs eleven. Last-owner
release adds spills, a deallocator call and its body. Frozen has the same
per-home loop structure. Lazy kwargs changes the kwargs home from a real dict
to NULL when unused: it removes the dict's release/deallocation, but retains
that NULL home's seven-instruction scan. Do not claim the whole clear loop as
saved.

Ten general entries satisfying the illustrative row would account for 2,340
fewer instructions outside allocator bodies. This is conditional arithmetic,
not an attribution of the workload's approximately 5,617-instruction historical
gap. Exact entry adds work and other handlers/call routes can change. Only
matched counters can close that ledger.

## Next structural candidates if a gap remains

1. **Carry checked callable/code facts into the binder.** The general handler
   first checks pointer-space in memory, loads the word and checks its upper
   bits again, then reloads/redecodes the function inside the binder and
   overlap initializer. Pass the checked actual value/function/code through
   the existing one-frame ABI. Preserve version/provider/ownership guards;
   do not assert that a memory guard proves a separately loaded variable.
   The removable budget is the specific repeated load/classification/decode
   sequence, not the whole entry path.

2. **Publish canonical cached method values once.** Review cache producers and
   invalidation so known method descriptors/functions and canonical receiver
   home copies can retain their provenance through duplication and result
   publication. The representative method path still runs redundant exact-
   number checks. This requires producer/type proof; it does not license
   removing publication from arbitrary native results. Measure the actual
   selected logging method handlers first.

3. **Treat constant homes as borrowed from a pinned code anchor.** Current
   constant slots are initialized by canonical duplicates and retired as
   owning locals. The executing frame owns its code, whose constants can
   anchor borrowed canonical bits. A reviewed borrowed bank could remove
   duplicate/refcount classification and its later per-home clear/GC edge,
   while retaining required bit copies. Constant ranges/table already exist,
   but free-variable placement, frame copy/escape, constructor verification,
   marshal/deepfreeze and GC edge counting must agree. No fake PyValue tags
   or per-read bank test. Count actual logging constant homes before claiming
   any budget; not every constant-copy loop instruction disappears.

4. **Reduce genuine frame/call owner work.** Inspect actual mortal argument
   duplicates, code/function owner pairs and fixed frame-header stores after
   measurement. Current overlap already transfers argument owners, so a
   borrowed-argument redesign gets no credit for copies already absent.
   Any anchor must survive ancestor mutation, callbacks and escaping frames.
   Start with demonstrated redundant pairs rather than a general collector
   or per-dispatch owner-map interpreter.

5. **Finish semantic loop/send completion and consuming branches.** This is
   independently required to remove executable release-only words, even if
   it is not the largest logging cost. Existing truth folding still restricts
   COPY/branch/POP fusion to one source line. Source-edge maps may permit
   broader fusion after proving removed spans/EH states survive. FOR_ITER and
   SEND need the reviewed yielded/completed owner outcomes and real targets.
   Retained-owner regions remain unadmitted pending an irreducible inventory.

No direction is locked by this static ledger. The first coherent matched
measurement should choose the next performance increment and retain the
separate completion/language-correctness gates.

## Actual warmed logging layout

The coordinator's build7 native print-only probe loads the unchanged kernel,
selects `logging_silent` and runs its normal 60,000 outer iterations before
reading the actual executing IR. Evidence is
`E11/merge7-logging-layout-probe-v2{,-stdout,-stderr}.*`. This is a layout
inventory, not a timing or hardware-counter result. The executable identifies
dirty `4b3d739d867`, with the JIT disabled; it is not a qualified source revision.

- `bench_silent` has ten warmed `CALL_PY_GENERAL` sites, each reached through
  `LOAD_ATTR_METHOD_WITH_VALUES`.
- `Logger.debug` calls `isEnabledFor` through the same method-lookup variant
  and `CALL_PY_EXACT_ARGS`. Its flags are `0x1c00000f`, including virtual kwargs
  eligibility; materialization is confined to the untaken enabled-logging arm.
- `Logger.isEnabledFor` uses shape-local attribute reads and a dictionary
  subscript on its warmed successful path.
- **All three functions have empty constant-home tables.** Borrowing constant
  homes therefore has no direct per-iteration savings on these hot frames.
  Do not select that redesign to close logging's gap on the strength of the
  illustrative entry counts above.

This confirms the selected method variant and the ten-general/ten-exact call
shape. It does not establish every branch assumption in the native ledger,
dynamic cache-hit counts, or a measured allocation/retirement budget.
