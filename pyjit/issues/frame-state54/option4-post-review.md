# Option 4 independent source and native review

Corrected source and actual GIL release native paths match the reviewed owner
contract. No unresolved source/native blocker; this is not a performance
admission. The [source review](option4-post-source.md) records the publication,
visibility, effect-inference and C-API findings and their corrections.

Source is `b48fe2d39896fef83d795bba3ec9c95af68cc235` in
`BASE/worktrees/frame54-locals`. Both `locals-effects-fix-{release,debug}-build.log`
end with117 modules checked and0 failed imports. The coordinator separately
reports current per-role84 frame/C-API tests (8 skips),640 core and2 effect-proof
tests passing. This reviewer only read source/logs and existing ELF metadata/
disassembly; no build, inferior, tests or timings were launched.

Release ELF SHA256:
`a8622abb409198618908b7720e9d2911d53b8a306447c51e9995221952688bb1`.
Debug ELF SHA256:
`2d2b4b6fe6b6b3a2dee04267fd609a5b4193dfe83ce6b4c782861068a88385d5`.
Source metadata after the semantic helper fix has no IR/opcode metadata diff
from selected51. The actual RETURN_GENERATOR remains SUSPENDS, not PUSHES_FRAME.

## Actual storage and construction

Non-running GDB DWARF confirms a104-byte header/sizeof112 in both GIL builds,
against selected112/sizeof120. frame_obj is56, instr_ptr64, ir_words72,
feedback80, return_pc88, return_slot96, owner100 and flags101; debug LLTrace
uses the remaining five flag bits. Owning heap homes begin104. This removes
one header word while retaining base/saved_top, owners and all physical homes.

PyFrameObject gains f_locals_mapping at96 and its embedded-data start moves
120→128. The lost embedded header word cancels this added pointer:
`PyFrame_Type.tp_basicsize` remains232 and itemsize remains8 in both selected
and candidate release ELF initialized type data. `_PyFrame_New_NoTrack`
disassembly at `0x18e135–148` adds code nlocalsplus and stacksize and calls
`_PyObject_GC_NewVar`. Therefore a new record reserves232+8*nslots bytes of
object payload, plus GC/allocator overhead, even when its embedded home tail
will only become useful if the frame escapes. Do not describe this as an
8-byte cold sidecar or a smaller materialized PyFrameObject allocation.

- **Exact ordinary NULL mapping:** actual CALL_PY_EXACT_ARGS reserves at
  `0x37ac7b` using header subtraction0x68 and converts co_framesize by13 words.
  Its setup at `0x37acce–ad44` has one NULL frame_obj store and no locals-mapping
  pointer store. It has no allocation/track/helper branch for the NULL argument:
  that initializer condition folds away. Warm thread/frontier entry makes no
  helper call; only heap-caller transfer and uncertain-frontier branches call
  their existing helpers. No new ordinary-call mapping object is allocated.
- **Generic NULL mapping:** `eval_frame_push_and_init_code` tests the incoming
  mapping at `0x36d524–527`; NULL branches to `0x36e07b`, sets prepared object
  register to0, pushes thread storage and rejoins ordinary initialization.
  This generic route pays the new mapping test but bypasses record allocation.
- **Generic nonNULL mapping:** the same test falls through to
  `_PyFrame_New_NoTrack` at `0x36d531`, before `_PyThreadState_PushFrame` at551.
  The mapping owner is installed on that object at562. Header and association
  complete at `0x36d566–60c`; no GC-track list mutation occurs at staging
  initialization. Failure paths are separate from this complete commit.
  This allocation is a real added cost for class/module/exec/eval mappings.

The smaller arena header does not remove a malloc per ordinary call; those
headers occupy the descending side of the same shared chunks as selected.

## Actual recurring read, move and clear costs

**Mapping reads become more expensive.** Selected `_PyEval_LoadName` begins
with one frame mapping load at `0x38cb36`, then its NULL test. Candidate
`0x38d19d–1bd` first branches on owner class, loads frame_obj for a live frame,
tests it, then loads its mapping and tests that mapping. The containing-object
case computes frame-128; interpreter sentinels take the missing-map route.
The normal class/module route therefore adds owner branches and a dependent
object-to-mapping load. LOAD_LOCALS and STORE_NAME also grow, as tabled below.
These are hot execution reads, not merely observer reconstruction.

**Generator construction gains a real object-present branch.** Candidate
RETURN_GENERATOR reads/tests source frame_obj at `0x34701a–03b`, branching to
an unmapped copy or the complete mapped-object commit. Both loops still move
and NULL every physical home (`0x347100–11b`, `0x3473e0–3fb`). The unmapped
branch still writes source frame_obj=NULL at `0x347370`, despite the preceding
NULL proof; that is an actual additional store relative to selected's copy.
The mapped branch installs destination owner/association, tests tracking at
`0x347138`, and links an untracked record into GC lists at13f–17c before the
CREATED state store at184. Unmapped construction skips this tracking block.
No general per-RESUME check was introduced, and no physical-home move vanished.

**Retirement retains the mapping in C until homes are clear.** Candidate
`_PyFrame_ClearExceptCode` has the existing object/escape decision, then the
unique-object path reads and NULLs the mapping at `0x3d0160–16d`, keeps it in
rbx and destroys the record at178. It clears homes, tests/retires that held
mapping at `0x3d01f5–205`, then clears function at20b. NULL-record retirement
reuses the zero rbx value; it does not reload a nonexistent header mapping.
Selected instead loads f_locals after the home loop (`0x3cfab5`) and tests it;
its NULL-before-decref store exists only for a nonNULL mapping. There was no
unconditional selected teardown NULL store to count as removed.

The candidate clear prologue saves r14/r12/rbx rather than selected's rbx,
adding16 bytes of callee-saved-register stack and the corresponding pushes/
pops; both reserve24 additional bytes with `sub rsp`. Source-level owner
locality therefore has a real compiled register cost. Escape/takeover and
frame observation also include the new complete-record/kwargs/tracking logic.
The source free-threaded locks, current-association use and release-state
ordering were reviewed, but no free-threaded native build or execution is
claimed from these GIL ELFs.

| Release symbol | Selected bytes | Candidate bytes | Interpretation |
| --- | ---: | ---: | --- |
| CALL_PY_EXACT_ARGS |973 |967 | No new NULL-map allocation;80-byte C-stack reservation unchanged |
| RETURN_VALUE |546 |546 | Calls the changed common retirement;16-byte reservation unchanged |
| RETURN_GENERATOR |1002 |1376 | Object-present split, complete move/tracking publication;16-byte reservation unchanged |
| eval_frame_push_and_init_code |7484 |7725 | Mapping preparation/allocation/error paths;200-byte local reservation unchanged |
| _PyFrame_ClearExceptCode |1080 |1421 | C-held mapping and complete-object observation; two more saved GPRs |
| _PyEval_LoadName |542 |590 | Owner/object/mapping access chain |
| STORE_NAME handler |312 |351 | Mapping access chain |
| LOAD_LOCALS handler |783 |818 | Mapping access chain |

Sizes are full `nm -S` symbols, including cold code/padding; they are not dynamic
instruction counts. Header reduction, byte savings in one handler, and absent
NULL-map allocations establish mechanisms only. They do not establish elapsed
improvement or a hardware explanation for any measured change.

## Separate mapping-path diagnostics

Alongside the unchanged29 workloads, use short precompiled, repeated mapping
entries to expose the added eager allocation/read path. Report them separately,
with direct L/selected ratios and raw balanced pairs:

1. Repeated exec of one precompiled `answer=17`, with globals/locals reused
   within each sample. This emphasizes entry plus one mapping store while
   excluding parse/compile work. A precompiled `pass` is a still narrower
   optional entry-only diagnostic.
2. Repeated minimal `class C: pass` creation from precompiled timing code.
   This includes class/type/dictionary obligations common to both runtimes;
   it measures the complete class path rather than attributing all time to the
   extra frame record. Explicitly collect old class cycles between samples
   outside the measured interval, since timeit disables automatic cyclic GC.
3. If more read weighting is needed, repeat a precompiled exec body with a
   bounded sequence of mapping reads/stores; keep the same entry count and
   code in both runtimes. Treat this as diagnosis, not a replacement workload.

Compilation, warmup, GC policy, sample length, hash, affinity, JIT state and
interleaving must be identical. The proposed small diagnostic script is
separately source-reviewed; this reviewer wrote or executed no benchmark
machinery. Startup/import impact remains an additional boundary if class/exec
measurements reveal a material loss. Do not add these cases to the29 geometric
mean or infer a removable cycle fraction from their ratios.
