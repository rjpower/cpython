# Selected51 frame field inventory

This is a source inventory and experiment proposal, not an implementation or
performance result. R is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/round51-golden`
at `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`; U is the adjacent
`upstream-3a440b6` at `3a440b65695`. Canonical checkout C files are older.
The CPython AI policy, canonical README/STATUS/GOAL/HANDOFF/PROCESS,
round54 plan and activation53 report/call audit were read before this proposal.
Only this document changes. No runtime, compiler, tests or performance jobs ran.

## Physical geometry and allocation

Existing release ELF DWARF, read with `gdb -q -nx -batch -iex 'set auto-load
python-scripts off' -ex 'ptype /o struct _PyInterpreterFrame' .../python`,
confirms the following offsets. Selected release has a **112-byte header**,
`sizeof=120` including the one declared trailing home. Upstream has an
**80-byte header**, `sizeof=88` including its declared trailing home. These are
GIL build results, not a claim about every configuration.

| Field | Selected byte offset / size | Upstream offset | Owner / authority | Reader class and derivability |
| --- | --- | --- | --- | --- |
| `f_executable` | 0 / 8 | 0 | Owning PyValue executing-code pin, independent of function | Hot code/layout/flags, exception/observer/GC; cannot derive from mutable `func_code` |
| `previous` | 8 / 8 | 8 | Nonowning dynamic frame link | Hot switch/unwind, frame walking, GC and profiling; not just observer state |
| `base` | 16 / 8 | absent | Nonowning pointer to owning value homes | Every operand and binder/clear/GC operation; thread homes can overlap, heap homes equal `owned_slots` |
| `saved_top` | 24 / 8 | absent | Nonowning prior thread value-stack frontier | Pop/chunk restoration; thread allocation only, not copied to heap |
| `f_funcobj` | 32 / 8 | 16 | Owning callable PyValue | Closure/COPY_FREE_VARS, super/context, observers, traverse/clear; cannot reconstruct from code |
| `f_globals` | 40 / 8 | 24 | Borrow from retained function | Hot GLOBALS/LOAD_GLOBAL/name/import operations, eval API, observer, precise roots; derivable through pinned function with added load/decode |
| `f_builtins` | 48 / 8 | 32 | Borrow from retained function | Hot BUILTINS/LOAD_GLOBAL/name/import, eval API, observer, precise roots; same tradeoff |
| `f_locals` | 56 / 8 | 40 | Nullable owning mapping | LOAD/STORE_NAME, class/module/eval execution as well as observers; not globally cold |
| `frame_obj` | 64 / 8 | 48 | Nullable owning materialized frame object | Hot retirement checks and tracing/monitoring/escape, lazy observation, GC; nullable does not mean dispensable |
| `instr_ptr` | 72 / 8 | 56 | Nonowning current semantic instruction in executing image | Dispatch publication, exception and instrumentation recovery, all source attribution APIs; cannot replace with return PC |
| `ir_words` | 80 / 8 | absent | Nonowning executing-image word base | Hot jump/offset/local-boundary/feedback ownership checks, source mapping, remote unwinder; derives from code exec but replacing it adds dependent owner/code/image loads |
| `ir_feedback` | 88 / 8 | absent | Nonowning executing-image entries base | Hot `IR_FB`, assertions and initialization/copy/refresh; derives from the execution header immediately before `ir_words` with one dependent entries load |
| `return_pc` | 96 / 8 | absent | Nonowning executable continuation, not attribution PC | Every call/resume/return, completion and debugger redirect; usually next word, but branch exits/redirects differ |
| `return_slot` | 104 / 2 | absent | Semantic destination or `IR_NO_DST` | Hot ordinary return, send/yield, discard, observer redirects; destination in caller/current suspended frame, not a code owner |
| `pending_kwargs` | 106 / 2 | absent | Virtual empty kwargs slot or `IR_NO_DST` | Binding producer; MATERIALIZE_KWARGS and observation reader; cannot infer from NULL alone |
| `owner` | 108 / 1 | 74 | Frame-storage/lifetime discriminant | Hot allocation/overlap, resume, clear, GC, observer completeness; independent from value ownership |
| `visited` | 109 bit 0 | 75 bit 0 | Legacy walk bookkeeping | Initializers/copy reset only in searched selected source; no selected consumer found |
| `external` | 109 bit 1 | 75 bit 1 | External-entry provenance flag | Debugger suspended-child jump reader (`frameobject.c:3515`); no explicit selected assignment/copy found; do not assume a useful initialized proof |
| `pending_redirect` | 109 bit 2 | absent | Deferred debugger-continuation commit bit | Hot result publication checks plus debugger writer; needed because overlapping child homes must retire first |
| padding | 109 bits 3–7; 110–111 | 75 bits 2–7; 76–79 | None | Removing a small flag alone does not shrink the header |
| `owned_slots[1]` | 112 / 8 declared | `localsplus` 80 | Variable owning homes on generator/frame object | Physical tail, not an extra mandatory thread value home |

Upstream also has `stackpointer` at 64/8 and `return_offset` at 72/2.
Selected's extra five pointers (`base`, `saved_top`, `ir_words`, `ir_feedback`,
`return_pc`, relative to the corresponding common fields and upstream's
`stackpointer`) are a net four pointer words; compact metadata still rounds to
8 bytes. This accounts for the 32-byte header difference without claiming that
all 32 bytes can safely be removed.

Under `Py_GIL_DISABLED`, `tlbc_index` is an extra `int32_t` after `return_pc`,
initialized/copied and checked by RESUME skip logic; it is absent from both
measured release ELFs. Under `Py_DEBUG`, `lltrace:5` occupies the spare five
bits after `pending_redirect`; initialized/copied and read/set by LLTrace.
No measured release LLTrace scan or assertion work should be counted.

R `pycore_interpframe.h:140–165,481–618` and `Python/pystate.c:3100–3208`
implement the arena: fixed headers grow downward, owning homes upward in shared
chunks. A frame is **not** normally one malloc of header plus slots. Warm homes
up to `chunk->initialized` are already NULL; reservation fills only an uncertain
tail, truncating the frontier before header reuse. Exact calls already reserve
and initialize once, with thread arguments overlapping the caller window.
`saved_top` can exceed `base`: the callee can occupy a suffix of an already
reserved caller window, and its physical end is max(old top, new end). Chunk
push records the preceding chunk top; pop restores its saved value frontier.
Neither `base` nor caller slot count alone replaces this state.

Heap generators/coroutines and escaped PyFrameObjects allocate variable home
tails sized by code locals plus scratch. `_PyFrame_Copy` sets `base=owned_slots`
and transfers **every physical home**, NULLing its source. `saved_top` is not
copied because heap retirement does not pop a thread reservation. A cold
sidecar replacing one pointer needs its own pointer/allocation/owner and may
save no header space. Derivation from an already retained owner is the bounded
candidate here. `co_framesize`, `FRAME_SPECIALS_SIZE`, deepfreeze output,
PyFrame/Gen type sizes, debug offsets, remote unwinding and GDB must agree with
any actual layout change.

## All lifecycle producer and consumer families

The field table covers each member; the following list supplies the common
lifecycle operations and reader families instead of repeating them per row.
Paths/lines are selected source, with generated handlers sourced from the DSL.

- **Ordinary initialization:** `_PyFrame_Initialize` in
  `Include/internal/pycore_interpframe.h:300–332` consumes function and locals,
  acquires executing-code owner, borrows globals/builtins, clears frame object,
  installs image/current/default continuation state, kwargs sentinel and flags,
  then fills hidden constant homes. Reservation separately writes `saved_top`.
  Producers are exact CALL (`Python/bytecodes.c:4344`), generic binder
  (`Python/ceval.c:3152`), push/overlap helpers (`pycore_interpframe.h:559–639`),
  and direct heap `init_frame` (`Objects/frameobject.c:4066–4080`). Direct heap
  initialization NULL-fills homes first; copy destinations need no preliminary
  home fill.
- **Special initialization:** `_PyFrame_PushTrampolineUnchecked`
  (`pycore_interpframe.h:645–669`) uses function None, owns code, has known
  return slot and no globals/builtins use. Release leaves those unused pointer
  fields unspecified. Evaluator C-stack entry frame (`ceval.c:2265–2291`)
  uses executable None, static interpreter-exit instruction/word base, NULL
  feedback, owned result homes, interpreter owner and dynamic previous link;
  it cannot be blindly cast to a normal function/image. Embedded thread base
  sentinel (`pystate.c:1481–1503`) uses None code, NULL function/current PC,
  interpreter owner and embedded NULL home. Generator allocation
  (`Objects/genobject.c:1143–1170`) starts FRAME_CLEARED with executable None;
  fields are not generally readable until the complete copy.
- **Copy/move:** `_PyFrame_Copy` (`pycore_interpframe.h:240–278`) transfers
  executable/function/locals/frame-object owners without incrementing,
  borrows globals/builtins and image pointers, copies PC/recipient/kwargs/
  redirect, resets visited, copies debug/TLBC fields, sets previous NULL and
  moves/NULLs every home. It does not copy owner, saved_top or external.
  `RETURN_GENERATOR` (`bytecodes.c:5256–5282`) sets heap owner/state and
  pops the transient thread header; `gen_new_with_qualname`
  (`genobject.c:1215–1255`) converts a frame object; `take_ownership`
  (`Python/frame.c:93–136`) converts an escaped frame into its object's tail,
  gives the source an extra code owner for the caller's eventual clear,
  installs frame-object owner, reconstructs completeness/back link and tracks
  GC. Complete owned slots are preserved for escaped locals, not destroyed.
- **Image refresh:** `_PyFrame_InitIRInstrPtr` / `_PyFrame_SetExec`
  (`pycore_interpframe.h:58–76`) install the executing image. Generic fallback
  after callback-capable binding (`ceval.c:3430–3443`) explicitly reloads
  code->_co_exec, writes words/feedback/current PC and resumes at word zero.
  `IR_CHECK_BLOCK` and call/observer validation assertions compare the bases
  with the current image (`ceval_macros.h:749–756`, `ceval.c:1348–1357,
  1602–1603,2024–2045,2088–2089`). Do not derive a continuation from a mutable
  function code or subtract an old PC from a newly selected image.
- **Continuation publication:** call and SEND/FOR producers set `return_slot`
  and `return_pc`; YIELD sets incoming destination. Shared result macros
  (`ceval_macros.h:805–820`, `pycore_interpframe.h:169–203`) check pending
  redirect/discard, replace the actual recipient and retire its old owner.
  `_PyEval_CompleteGeneratorReturn` (`ceval.c:3095–3117`) decodes caller
  instruction and computes a SEND/FOR exit from return PC; completion
  (`:3055`) can update that PC. Debugger setters (`frameobject.c:3477,
  3515–3526,3666–3797`) inspect or change both PCs, destination and redirect.
  `_PyFrame_CompleteRedirect` (`frame.c:12–21`) commits PC before callback-
  capable scratch cleanup after child retirement. These are execution fields.
- **Virtual kwargs:** known-plan binding (`ceval.c:3755`) marks the slot;
  `_PyFrame_MaterializeKwargs` (`frame.c:74–90`) allocates/installs the dict and
  clears the marker. The MATERIALIZE_KWARGS DSL (`bytecodes.c:149–150`) and
  observer materialization (`frame.c:40–45`) use it; the internal test API
  inspects it (`Modules/_testinternalcapi.c:3960`). NULL can also mean unbound,
  so a bare NULL test cannot replace the marker.
- **Clear/pop:** `_PyFrame_ClearExceptCode` (`frame.c:204–235`) tests frame
  object escape first, possibly moves all state, otherwise clears thread
  locals only (full heap homes for nonthread frames), clears locals mapping
  and function owner. The code owner remains valid through callbacks and is
  separately released by `_PyEval_FrameClearAndPop` in ceval; pop uses frame
  base/saved_top and fixed header geometry. `frame_dealloc`/`frame_tp_clear`
  (`Objects/frameobject.c`, including executable/function clears near 3909)
  and generator clear/dealloc (`Objects/genobject.c`) complete heap retirement.
  Borrowed globals/builtins are not independently decref'd. Their validity
  must survive exactly as long as required by all consumers.
- **Hot non-lifecycle readers:** `IR_SLOT` uses cached slots/base,
  `IR_NLOCALSPLUS` reads the `_PyCodeExec` prefix immediately before ir_words
  (`ceval_macros.h:499–507`), `JUMP_TARGET`/`INSTR_OFFSET` use ir_words
  (`:228,240`), `IR_FB` uses ir_feedback (`:756`), `GLOBALS`/`BUILTINS` use
  direct frame fields (`:293–294`). `_PyFrame_GetCode` and GetFunction decode
  their independent PyValue owners. Closure/super and name/import DSL and
  eval APIs need those actual owners/environments, not observer snapshots.
- **GC/observers/tools:** `_PyFrame_Traverse` (`frame.c:25–33`) visits frame
  object, locals mapping, function, code and actual owning homes.
  `_PyGC_TraceVisitFrames` (`Python/gc_trace.c:1062–1075`) additionally visits
  globals/builtins roots, skips interpreter sentinels and walks previous.
  Free-threaded GC also scans base/slot extent. Frame properties/locals
  proxies/tracing/jumps use all frame lifetime and source state;
  `_PyFrame_SafeGetCode`/SafeGetLasti use globals/builtins heuristics plus code
  and image (`pycore_interpframe.h:82–129`). Debug offsets export layout
  (`pycore_debug_offsets.h:390–406`); `_remote_debugging/frames.c:226,287`
  reads ir_words; `Tools/gdb/libpython.py:1147–1206` reads homes,
  globals/builtins/code/PC/previous/owner. Stack unwinding, sys._current_frames,
  traceback, profiling and monitoring also require valid published state.

The search found no selected read of `visited` and no selected assignment to
`external`; the latter still has a debugger read. Removing these bits alone
saves no aligned word. Treat their cleanup/provenance repair as a separate
correctness task, not the work-removal thesis or an admission proof.

## Three bounded experiments and a deferred alternative

**1. Derive feedback base from the retained executing image.** Remove
`ir_feedback`, retain `ir_words`, and use the existing contiguous image-prefix
contract to obtain `((_PyCodeExec *)ir_words - 1)->entries`. The contract is in
`pycore_ir.h:351–390`; ordinary image allocation and the static init-cleanup
image put their header before words. This is already the method used for the
local-boundary read. Interpreter-exit/base sentinels are special and must never
request feedback; assertions must distinguish them before prefix access.

This removes one 8-byte field (112→104-byte header), its initialization/copy/
refresh stores and the construction load of entries. It adds an entries load
after the ir_words load on each feedback access, rather than obtaining feedback
from frame directly. It adds no universal guard and preserves the executing
code owner. It must update IR_FB/checks, source declarations/comments,
initializers/copy/fallback refresh, layout/debug offsets/deepfreeze and generated
products. Hot feedback-heavy DeltaBlue/Richards can lose more than construction
wins; inspect native dependency chains and count feedback accesses versus frame
starts before claiming a net instruction reduction. Using `_PyFrame_GetCode()->
_co_exec->entries` instead would add the code-owner decode/load chain and is a
different, weaker proposal.

**2. Derive globals and builtins from the retained function.** Remove both
borrowed environment pointers (112→96-byte header, or104→88 composed with #1).
Normal function members are Python-readonly (`funcobject.c:678–679`); searched
writes are constructors and `func_clear` (`:1213–1230`), while function code is
mutable and remains irrelevant to this derivation. A retained owning function
pins the environment in ordinary execution. A candidate must explicitly prove
cyclic-GC/function-clear and escaped-frame teardown behavior, where func_clear
can NULL these members; Python readonly alone is insufficient proof. Preserve
trampoline/sentinel API behavior with their known owner class at the cold API
boundary, not a new normal-handler guard. Existing arbitrary rebuilt C callers
must use the new accessors rather than write borrowed caches.

Remove normal setup's two environment stores and copied pointers, and remove
two words from thread/heap geometry. Every GLOBALS/BUILTINS access gains function
PyValue decode plus a dependent member load; the compiler may share some, but
that must be established in actual native handlers. Update precise root walking,
frame properties, safe-dump heuristics, eval/name/import paths, GDB and all direct
consumers together. This is particularly exposed to globals-heavy workloads and
requires escaped-frame, changed-__code__, cyclic-GC, reentrant-finalizer,
trampoline and PyFrame_New checks. Do not conflate preserved environment with
the executing-code pin.

**Deferred: compact/cold continuation state.** `return_pc` is usually
`instr_ptr+1`, but it is also the real SEND/FOR exit and debugger target while
instr_ptr retains source/exception attribution. A 32-bit continuation index
alone may not reduce aligned header size: 12 pointers plus a 4-byte PC index,
two 16-bit slots and owner/flags round back to112 bytes. A real contract must
pack or remove additional metadata and prove index bounds, special sentinels,
all exceptional branches and pending redirects. Moving f_locals/frame_obj to
an allocated sidecar needs its own header pointer and adds hot retirement
indirection; f_locals also serves ordinary name execution. Neither is a complete
small experiment yet. `saved_top` reconstruction from the previous frame is
similarly blocked by overlap, max-frontier and chunk cases.

**3. Store the locals mapping on the existing lazy frame object.** This is a
more specific alternative to an extra cold sidecar: retain the existing
`frame_obj` pointer, remove `f_locals`, and place the nullable strong mapping
owner on PyFrameObject. An optimized frame with no supplied mapping keeps
`frame_obj=NULL`; no new allocation or generic-handler guard is needed there.
Nonoptimized/module/eval/PyFrame_New entries with a nonNULL mapping must create
the frame object before publishing a runnable frame. This can remove one
8-byte header word and its ordinary NULL initialization; it adds an allocation
to ordinary nonNULL-mapping entry and an indirection to actual name operations.

The new owner must be visited/cleared exactly once by PyFrameObject traversal,
not both frame traversal and object traversal. A live frame's existing owner of
frame_obj can form the same retention cycle as today, but teardown order needs
care: clear_slots currently clears the locals mapping after values and before
the function owner. `_PyFrame_ClearExceptCode` detaches/decrefs frame_obj before
that loop; a moved mapping owner must remain available until its prescribed
retirement point, and callback-visible frame state must already be coherent.
`take_ownership` must leave the mapping in the same PyFrameObject while moving
all homes and code/function owners; no duplicate transfer or early clear.

Do not invoke `_PyFrame_GetFrameObject` on an incomplete frame without changing
its assertion/publication contract. Its current `_PyFrame_MakeAndSetFrameObject`
also materializes kwargs and can fail allocation. A construction helper must
avoid reading uninitialized fields, reentering mapping initialization or
leaking consumed function/locals on failure. Update PyFrameObject layout,
`init_frame`/PyFrame_New, all binder failures, LOAD/STORE/DELETE_NAME/import,
eval builtins APIs, mapping creation in `frame_getlocals`
(`frameobject.c:1146–1155`) and `_PyFrame_GetLocals` (`:4281–4290`), frame
clear/dealloc, precise GC and safe observer APIs. The `import.c:4540`
globals==locals identity check and `ceval.c:4957` must retain exact identity.
Measure startup/import/logging as well as optimized calls: frame-object
allocation on module/class/eval entry can dominate the intended NULL-store
saving. This third contract needs a producer/retirement review before coding.

The first two are complete bounded state-removal hypotheses, conditional on
independent ownership review. They are not measured wins. Native dependency
cost and semantic evidence decide admission; source store counts and smaller
headers do not establish a throughput gain. Activation53 already rejected
three changes that removed real construction/completion work.
