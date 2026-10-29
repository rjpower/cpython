# 11: Precise roots: an attributed stack walk over precise and opaque frames

Status: design, 2026-10-27, revised after review rounds 1 and 2, for step 10a
(note 06 section 6, note 08 sections 3 and 6). It decides what the
collector reads as roots after 10a, how it tells a root from a stale word
on a C stack, and what tier 0 and tier 1 must provide; it retires the
hack-register rows assigned to 10 and 10a. Moving and the handle
discipline for C code remain 10b's.

**Scope against the plan.** Note 06 section 6 and note 08 section 6 give
10a every core C frame and leave conservative scanning to extensions. This
note narrows that: 10a makes the eval loop, the call layer, the entry
paths and JIT code precise, and leaves the rest of core C opaque, scanned
as now, because 4,938 core functions hold objects in C locals across
collecting calls (section 12). Their migration is 10b's, which already owns
the handle discipline they need. Note 08's rows are retired as section 7
lists; where a row named "core C frames" it now means precise frames, and
the opaque remainder is 10b's, listed in section 12.

## Decisions

1. **Every C stack is walked through its unwind tables**, from a start
   context captured at a call boundary. Each word of each stack is
   assigned to exactly one owning frame, including the slots where a callee
   saved a caller's register. The walk replaces the activation ranges and
   their saved registers, the stack clears and the inline collection's full
   scan (section 3).
2. **Frames are precise or opaque, by the code range of their function.**
   The collector never scans a word that a precise frame owns, and scans
   the words of opaque frames conservatively, as today. Precise code is the
   eval loop and its helpers, the call layer, the collector's entry paths
   and JIT code; other core C and every extension are opaque (section 2).
3. **A precise frame's promise.** At every call that may collect, every
   object the function uses afterwards, and every argument of that call, is
   anchored: in a slot of a pushed interpreter frame, in a pushed
   `_PyValueRoot` span over its initialized part, in a field the mark
   visits, or borrowed through a strong traced edge that cannot change
   during the call (section 2.1). Object parameters are anchored by the callee, unless every
   caller is precise and checked. root_check enforces it on handlers, its C
   mode on precise C functions, an inline audit on the linked binary, and
   the verifier at run time.
4. **Core C stays opaque in 10a**, except about 130 functions: the call
   layer (about 80), the eval-loop helpers (about 25) and the entry paths,
   where the stale roots observed through 6b live (section 12).
5. **PyMem blocks are followed from extension frames only**, once the
   verifier reports no core site over the suite and the stress runs. Core C
   that keeps objects in a C-local PyMem array covers its initialized part
   with a span; a heap-lived block registers a `_PyGCRoot` (section 6.2).
6. **Allocator slack is rejected.** One word of slack per object block
   costs a geomean of 8.5% of peak RSS on the memory group, and 24% on
   collector_live128. The one-past-the-end hazard is retired for precise
   frames, because they are not scanned. It remains for opaque frames until
   10b (section 6.1).
7. **A frame is a root from push to pop, and while it runs.** The
   collector visits every frame on each thread's data stack by its header,
   linked into the frame chain or not, and every frame on the chain,
   running generator frames included; a returning frame stores its result
   in the caller's slot before it is unlinked (section 4.4). The struct scan goes; data segments
   stay to 10b.
8. **Tier 0 keeps every value in a frame slot at every call.** Tier 1
   records one safepoint table per call site and per eval-breaker check,
   read by both the collector and the deoptimiser: every live tagged value,
   interpreter slot or compiler temporary, is in a frame slot or a listed
   spill slot, never a register (section 5).
9. **Step 7 inherits an enumerable root set and one barrier rule**: slots of
   a pushed or running frame need no barrier, and a heap-owned frame
   remembers its owner when it stops running (section 11).

## 1. What is scanned after 10a

| Root | Before 10a | After 10a |
|---|---|---|
| interpreter frames | the frame chain | every frame on each data stack, by header, and every frame on the chain, running generator frames included (4.4); suspended generator and frame-object frames through their objects |
| the eval loop's entry frame | skipped | its return slot, by a visitor of its own (4.3) |
| thread state and interpreter fields, static types, immortal objects, the finaliser queue, `_PyGCRoot` | precise | precise |
| `_PyValueRoot` | precise, one value | precise, a span over an initialized prefix (6.2) |
| eval-loop C frames | skipped by activation range, except in an inline collection | owned words never scanned, in every collection |
| registers the eval loop saved for its caller (`act.regs`) | scanned from the activation record | owned by the caller; scanned only if the caller is opaque |
| other precise frames (call layer, entry paths, JIT) and the registers they own | scanned | not scanned |
| opaque frames, core and extension | scanned | scanned |
| PyMem blocks a scanned word points into | scanned, transitively | scanned only from extension frames |
| thread and interpreter structs | scanned (`_PyGC_TRACE_ROOT_STRUCTS`) | not scanned |
| writable data segments | scanned | scanned (10b) |

Note 06 section 14.3's conservative rules apply unchanged to what is still
scanned: interior pointers, shallow marking, the NOSCAN heap.

## 2. Frame classes

| Class | Members | Check |
|---|---|---|
| eval loop | `_PyEval_EvalFrameDefault`; every generated handler in the tail-call build; every function in Python/ceval.c, the about 25 `Py_NO_INLINE` helpers from rung 2 (opaque and scanned until then) | root_check on handlers with allocating calls as safepoints (4.2); the C mode on ceval.c; the inline audit; the verifier |
| call layer | Objects/call.c's dispatch (`PyObject_VectorcallValue`, `_PyObject_VectorcallTstate`, `_PyObject_MakeTpCall`, `_PyVectorcall_Call`, `_PyObject_Call`, `PyObject_Call`, `_PyStack_UnpackDict`, `_PyFunction_Vectorcall`, the `*Prepend*` forms, `_Py_CheckFunctionResult*`); `cfunction_vectorcall_*`, `cfunction_call`; `method_vectorcall_*`, `wrapperdescr_call`, `classmethoddescr_call`; `method_vectorcall`; `context_run`; the generator send path (`gen_send_ex2`, `gen_send_ex`, `gen_iternext`, `gen_throw`, the coroutine wrappers); typeobject.c's `slot_*` and `vectorcall_method` family. About 80 | the C mode, run by the build; the inline audit; the verifier |
| entry paths | the collector (Python/gc_live.c, the mark in Python/gc_trace.c); `gc.collect()`'s clinic wrapper and impl; `_PyGC_Collect`; `_Py_HandlePending`, `_Py_RunGC`; the allocation path from `PyObject_Malloc`, `gc_alloc`, `_PyObject_GC_New`, `_PyObject_GC_NewVar`, `PyType_GenericAlloc` and the boxing retry to `_PyGC_LiveOnThreshold` and `_PyGC_LiveCollectHere`; the detach path (3.3) | the same |
| JIT | tier 0 blocks, tier 1 code | section 5 |
| core opaque | every other function in the interpreter's image (the `python` binary or libpython) | none until 10b |
| extension opaque | every function in any other loaded object, including the stdlib's shared modules | none until 10b |

**Marking.** `Py_GC_PRECISE` on a definition, or on a prior declaration for
generated code, places the function in section `py_gc_precise`; the linker
provides `__start_py_gc_precise` and `__stop_py_gc_precise`, and a pc in
that range classifies its frame. The cases generator emits the macro on
every handler in the tail-call build. A precise function inlined into an
opaque caller, or a `.cold` part split out of one, is classified opaque,
which is safe. The build's stamp rule runs the C mode over every
`Py_GC_PRECISE` definition in the tree, so a function marked before it
passes fails the build: promotion is conditional on the check by
construction.

**The inline audit.** The converse is not safe: an opaque function inlined
into a precise one puts its locals in a precise frame, unchecked. The
compiler may do it within a file, and under LTO across files. A build step
reads the linked image's DWARF, and for every precise function lists its
`DW_TAG_inlined_subroutine` entries. Each inlined function must be precise
(its body was checked), in the non-collecting list, or a static inline
function that the C mode checked through its body; otherwise the build
fails, naming the pair. The step also fails on a `.cold` part of a precise
function. It is exact for any optimisation level and LTO, because it reads
what the compiler did. It fails closed: the gated builds carry `-g`, and a
precise symbol with no DWARF entry, an image without `.debug_info`, or a
clone (`.isra`, `.constprop`, `.part`) of a precise function not itself in
the section fails the step; a test builds one file without `-g` and
expects the failure.

**Outlined callees** (10a-2b.md). The same step reads the image's code: a
static function that the precise sections call or jump to, that the C mode
classifies as collecting, and that the build left outlined outside the
sections has a frame of its own between the precise caller's and the
collector's. The walk scans it, so a stale word there keeps a dead object
alive, on exactly the builds that do not inline (debug and ASan, at
`-Og`). Each such callee is marked `Py_GC_PRECISE`, or is listed in the
audit's `OUTLINED_OPAQUE` with the reason its frame may stay opaque; the
step fails on any other. `Py_GC_PRECISE` is defined in Include/pyport.h,
so that the static inline functions of the public headers can carry it;
outside the core it is empty. A rung that marks functions builds
build/default, build/debug, build/asan and build/tailcall at gate 3 (gate
0 on each), and each build runs the step.

### 2.1 root_check's C mode

The handler checker (note 06 section 17.2) assumes that a `PyValue *` local
points into rooted slots, that declaring a `_PyValueRoot` roots its value,
and that a dereference of a held pointer is anchored. Those hold in
generated handlers and are false in general C. The C mode is a separate
analysis over a supported subset, sharing the lexer.

**Subset.** Function bodies made of declarations, expression statements,
`if`, `for`, `while`, `do`, `switch` with fallthrough, `break`, `continue`,
`return`, and `goto` to a label in the same function. It rejects computed
`goto`, `setjmp`, statement expressions, inline assembly, an object-typed
local whose address escapes (`&obj` passed to a call, other than the
listed out-parameter forms such as `PyDict_Next`), and object pointers in
struct locals other than the span type. A rejected function cannot be
marked precise.

**The eval loop's body.** `_PyEval_EvalFrameDefault` is hand-written entry
and exit code around the generated region (the cases include). The C mode
checks the hand-written parts, and treats the generated region, whose
dispatch uses computed `goto`, as one statement whose safepoints root_check
has checked handler by handler. No other function may use computed `goto`.

**Analysis.** It builds the control-flow graph, gotos and cleanup labels
included, and computes liveness of object-typed locals and parameters. At
each safepoint every live object local, and every argument of the call,
must be anchored on every path reaching it. An assignment to a local ends
its anchor; an assignment to an anchoring field or slot ends the anchors
borrowed from it.

**Three kinds of call.** analyzer.py's `NON_ESCAPING_FUNCTIONS` means
"cannot run Python" and includes allocators, so it is not a non-collecting
list. The C mode keeps three:

- *non-collecting*: neither allocates an object nor runs Python (field
  accessors, `PyTuple_GET_ITEM`, the value predicates). A new list, each
  entry a macro or static inline function whose body is checked, or an
  extern function with a reviewed comment;
- *collecting, not reentrant*: allocates but runs no Python. An inline
  collection defers finalizers and weakref callbacks, and the collector
  changes no strong edge of a reachable object; it clears weak references
  to unreachable objects. The analyzer's allocating functions;
- *reentrant*: every other call, the default.

**Anchors.** A value is anchored by:

- *a frame slot*: a `PyValue *` whose provenance is a frame's `base`, the
  eval loop's slot pointer, or a parameter declared `Py_GC_SLOTS`, offset
  by an index. The type alone proves nothing;
- *a span*: the checker tracks `_PyThreadState_PushValueRoot` and its pop
  as a scope that must be closed on every path, and anchors a local stored
  into the span's covered prefix while the span is pushed;
- *a borrow*: a load through a strong field of an anchored object, one
  that the type's traverse visits. A weak field (`wr_object`), a borrowed
  cache and any field the mark does not trace never anchor; their target
  needs a span of its own. Across a non-reentrant call any strong field
  qualifies, since no mutator runs and the collector clears only weak
  fields. Across a reentrant call only the listed immutable strong edges
  do: a tuple's items, a code object's fields, a bound method's `im_func`
  and `im_self`; `ob_type`, `func_code`, list items and dict values do
  not. `FT_ATOMIC_LOAD_*` is ordering, not ownership, and adds nothing;
- *a parameter*: not anchored by default, because an opaque caller may hold
  an argument only in an argument register (`f(PyLong_FromLong(5))`). A
  precise function that uses an object parameter after a collecting call
  covers it with a span on entry. A function marked
  `Py_GC_ARGS_ANCHORED` is exempt; the C mode then checks that it is
  static, that every call site is in a precise function, and that the
  arguments are anchored there. The hot internal dispatch is static and
  takes that form; the public entries pay a span.

**Negative tests** in test_generated_cases must fail the C mode before any
function is promoted: a value held across a call with no span; one stored
into a span after its pop; a span popped on one path and not on a cleanup
`goto`; a `PyValue *` from `PyMem_Malloc` treated as slots; an anchor
overwritten before the use; a list item borrowed across a reentrant call;
a weakref's target borrowed across an allocation; a parameter used after a collecting call without a span; and an opaque
static function that the inline audit finds inlined into a precise one.

## 3. The walk

### 3.1 Ownership

The walk goes outward from a thread's start context (3.3). For each frame
it has the pc, the stack pointer at the call (sp), and from the descriptor
the canonical frame address (CFA) and the slots where the frame saved
callee-saved registers. The frame's extent is `[sp, CFA)`; every word of it
is owned by the frame, except its save slots. The walk keeps, per register,
the locations holding a value whose owner is not yet known; the innermost
set is the start context's register array. Going outward, the pending
locations of register r are owned by the first frame that saves r, whose
save slot then becomes pending for r, or that does not preserve r under its
calling convention. The second clause covers `preserve_none` handlers in
the tail-call build, which use r12 to r15 without saving them: a value
pending for r12 when the walk reaches a handler is the handler's. The
convention comes from the descriptor (handlers and tier 0 blocks are
"preserves nothing"; all other code SysV). At the stack base the remaining
locations are owned by the outermost frame. Each location has one owner,
and a word is scanned if and only if its owner is opaque.

This is HotSpot's register-map rule for compiled frames, extended by the
convention clause. It needs neither frame pointers nor a prologue shape.

### 3.2 Descriptors and their cache

A descriptor gives, for a pc, the CFA rule (a register and an offset), the
return address's location, the save slot of each callee-saved register
(x86-64: rbx, rbp, r12 to r15; arm64: x19 to x29, d8 to d15), the preserved
set, and for JIT code its tagged slots. The return address rule is per
architecture: on x86-64 it is always a CFA-relative memory slot; on arm64
it is x30's rule, a save slot or, in a frame that has not saved it, the
register itself, pending like any callee-saved register. JIT descriptors come from a range
table that tier 0 and tier 1 register into and that is consulted first. For
C code the descriptor is decoded from the object's `.eh_frame`:
`_dl_find_object()` (glibc 2.35 and later; this machine has 2.43) gives the
object and its `.eh_frame_hdr`, a binary search gives the FDE, and its
instructions are interpreted up to the pc. Register and offset rules are
supported; an expression rule, a signal frame (`S` augmentation) or an
unknown instruction counts as no descriptor. The python binary has 8,000
FDEs (a 315 KB `.eh_frame`).

**Lookup pc.** A return address is looked up as `ra - 1`, so that a call
that ends a function or an unwind row selects the call's own row (DWARF 5
section 6.4.4). The start context's pc is a return address too (3.3).

**Cache.** Decoded descriptors are copied into a cache keyed by lookup pc;
nothing points into an object's unwind data. The cache is valid for one
loader generation, the pair `dlpi_adds`, `dlpi_subs` that
`dl_iterate_phdr()` reports. Each root phase reads it under the GIL; a
changed `dlpi_subs` flushes the cache, so an object unloaded and another
mapped at its address never meets an old entry. Decoding reads only the
unwind data of objects with a pc on a walked stack, which cannot be
unloaded while their code is on a stack. The JIT unregisters a range
before it frees or reuses the code, at the point where its executor
lifetime already requires that no stack returns into it. The descriptor
cache is per interpreter, read and flushed by its collector under its own
GIL; the JIT range table is process-wide, updated under a mutex and read
under it by the root phase.

### 3.3 Start contexts

A start context is one frame stopped at a call: the pc, the sp, and the
callee-saved registers' values at that point. `_PyGC_CaptureCaller(ctx)`
produces one. It is an assembly function (Python/gc_capture.S), so no
prologue runs before it reads anything, and it stores the callee-saved
registers as at entry, which are its caller's by the ABI. On x86-64 the
pc is `[rsp]` and the caller's sp is `rsp + 8`; on arm64 the pc is x30 and
the sp is unchanged, since `BL` pushes nothing. The context describes the
caller at the call instruction.

- **The collecting thread.** The collection's entry calls it and stays on
  the stack for the whole collection, so the context and the frames
  outward from it are valid throughout.
- **A detached thread.** No opaque frame is known to persist: a wrapper
  such as `_PyThreadState_Swap()`, or an extension helper that calls
  `PyEval_SaveThread()`, may return while the thread is still detached.
  What persists is the innermost precise frame P, because precise code
  runs attached: the eval loop, the call layer or an entry path below
  which the detaching C code was called. On detach, in every detach entry
  (`PyEval_SaveThread`, `PyEval_ReleaseThread`, the swap paths and
  `detach_thread`), the thread captures its context and at once unwinds,
  while every save slot is live, to P. It stores P's pc and sp, the
  register values pending at P, and the capture's sp and registers. A
  collection then scans `[capture sp, P's sp)` and the captured registers
  conservatively, as one opaque region whose PyMem blocks are followed,
  which is today's scan for that part, and walks from P outward. If the
  unwind finds no P, or fails, the whole stack is that region. It replaces
  today's capture, whose comment ("before this function can change a
  callee-saved register") the compiler does not guarantee.

### 3.4 Validation and fallback

Each step checks that the CFA is above the frame's sp, strictly above the
previous CFA, and at most the thread's stack base (`c_stack_top`), and that
the return address and save slots lie within the frame's extent. The walk
reads only `[sp, stack base]` of a thread that is alive; a recorded stack
of a thread that has exited is scanned conservatively, clipped to the
readable mappings, as today. The walk
ends at a frame whose return address rule is undefined (`_start`,
`start_thread`) or at the stack base. A failed check or a pc with no
descriptor stops the walk: it scans the rest of the stack from that frame's
sp, and every pending location, conservatively with PyMem blocks
followed, as an extension frame is, and counts the event. Debug
builds print the pc and its object once; the suite must record zero.
Conservative scanning is the failure mode of the one walk, not a second
path. Platforms other than ELF on x86-64 and arm64, and arm64 until its walk
tests pass, scan conservatively in full, as today: the activation ranges'
exclusion and the clears are deleted for them too, so they keep the
pre-10a retention without its mitigations, and correctness, which never
depended on the mitigations.

**Cost.** A warm step is a hash lookup, a few loads and range tests: about
a microsecond per hundred frames. On collector_live16 today's whole
conservative scan is 0.79% of cycles, about 0.34 ms of a 19 ms collection
(35 collections in 1.52 s, `perf record`), mostly the data segments. The
detach capture adds two or three warm steps to each GIL release.

## 4. The eval loop

### 4.1 Its C frame, in both builds

**The plain build** keeps the dispatch state (`next_instr`, `slots`,
`frame`, `tstate`) in C locals of one 1,024-byte frame (968 bytes of
locals and spills, six pushes, the return; CFA `rbp+16`, saves at `CFA-16`
to `CFA-56`). The walk classifies the frame by pc and skips it; the saves
in its prologue are the caller's. `_PyEvalActivation`,
`_PyEval_EvalFrameEntry`, `act.regs`, `scan_activations` and the record of
`lo` are deleted. A PEP 523 hook's frame is opaque and the eval loop it
calls is precise, however entered.

**The tail-call build** keeps the dispatch state in registers across
handlers, each its own `preserve_none` function. Every handler is precise
by `Py_GC_PRECISE`. A C callee that saves the handler's r12 to r15 saves
values the convention clause (3.1) assigns to the handler, which is
skipped. Both builds get one rule, with no record per entry or per call.

### 4.2 Allocations are safepoints

An allocation past the second threshold collects at once, without the eval
breaker (note 06 section 15.2). Today that collection scans the eval
loop's frames, because the allocating handler may hold a fresh object only
in a C local. After 10a no collection scans them, so the root rule must
hold at allocating calls too. A probe ran root_check with the allocating
calls that analyzer.py lists as non-escaping (`PyCell_New`,
`PyLong_FromLong`, `PyFloat_FromDouble`, `_PyTuple_FromValues`, the boxing
functions and others) as safepoints, and the `ALLOCATING` exemption for
`PyType_GenericAlloc` removed. It flagged two handlers:

- `MAKE_CELL`: `initial` is still in its slot, and the slot is not written
  before the call;
- `_ALLOCATE_OBJECT`: `init_func` is loaded from the spec cache of a type
  that is in a slot.

Both are borrows across a non-reentrant call (2.1), which root_check gains
as one rule: a value loaded from a slot or a field of an anchored object is
anchored across calls of the second kind, until the slot or field is
written. `ALLOCATING` is deleted, and the analyzer's comment that boxing
"cannot start a collection" is corrected. An inline collection then walks
and marks as one at the eval breaker does, and only defers finalizers and
callbacks (`no_python`).

### 4.3 The entry frame and the helpers

`_PyEval_EvalFrameDefault` keeps an `_PyEntryFrame` in its C frame. Its
executable is `None` and debug builds poison several header fields, so it
does not go through the ordinary slot count. A visitor of its own visits
exactly `entry.stack[0]`, which a returning frame writes (4.4) and
`INTERPRETER_EXIT` returns.

The about 25 helpers in Python/ceval.c and Python/ceval.h that are
`Py_NO_INLINE` exist so that their frames are scanned: they hold the boxes
of inline arguments (`VALUES_TO_OBJECTS`), call results and argument
arrays in C locals. Each becomes precise, pushes a span for what it holds,
and loses `Py_NO_INLINE` where scanning was its only reason, subject to the
inline audit.

### 4.4 Frame transitions

Two windows hold live values outside the frame chain today:

- `_PyEvalFramePushAndInit()` pushes a frame and initializes it, and
  `initialize_locals()` stores fresh `*args` and `**kwargs` objects in its
  slots and allocates more, before the frame is linked into
  `current_frame`;
- `_RETURN_VALUE` copies the result to a C local `temp`, unlinks the frame
  and calls `_PyEval_FrameClearAndPop()`, which can allocate through
  `take_ownership()` when a frame object exists, before it stores `temp`
  in the caller.

**Push to pop.** The collector visits each thread's data stack instead of
the chain's thread-owned frames. Frame headers are fixed-size records from
`datastack_limit` to the end of the current chunk, and from each earlier
chunk's saved `top` to its end (`_PyStackChunk`); each header gives its
slots (`base` and the code's slot count). `_PyFrame_Initialize` sets every
slot past the arguments to NULL before anything that may collect, and the
push stores a NULL executable that the visitor skips until it is set; no
call between push and initialize may collect, which root_check and the C
mode check as an adjacency rule. Generator and frame-object frames are
visited through their objects, and the entry frame as in 4.3. A running
generator or coroutine frame lives in its object's `owned_slots`, not on
the data stack; the collector also walks each thread's chain and visits
every frame on it, so a running heap frame is a root from resume to
suspension or clearing. A frame is therefore a root from push to pop and
while it runs, linked or not.

**Return order.** `_RETURN_VALUE`, `_RETURN_GENERATOR`, `_YIELD_VALUE` and
the generator return path store the transferred value in the caller's
destination slot, or the entry frame's return slot, before the source frame
is unlinked or cleared. The destination is dead in the caller until the
call completes, so the early store changes nothing observable.
`_PyEval_FrameClearAndPop()` and `_PyFrame_ClearExceptCode()` are
collecting calls for root_check.

## 5. The JIT

**Tier 0** (note 05) compiles a block into a tail-call handler whose inline
paths store every result in its slot and whose other instructions call
handlers. Its registers (r12 to r15: frame, slot pointer, thread state,
`next_instr`) hold no object. At every call every live value is in a frame
slot. The templates come from the handler definitions, so root_check checks
them with allocations as safepoints; an inline path that boxes stores the
box before the next call. A block registers one descriptor for its range:
CFA rule, saved rbp, "preserves nothing", class JIT.

**Tier 1** keeps unboxed numbers and object references in registers across
instructions and inlines callees whose frames are virtual. Its safepoint
entry, one per call site and eval-breaker check, lists every live tagged
value: each interpreter slot of the physical and virtual frames, and each
compiler temporary, with its location (a frame slot, a CFA-relative spill
slot, a constant, or an unboxed register value). At a safepoint a tagged
value is in a frame slot or a listed spill slot, never in a register. The
collector visits the listed spill slots in addition to the physical
frame's slots, which the frame visitor still visits; a physical slot that
tier 1 has not written back holds an older valid value or NULL, never
garbage, so it costs retention at most. Object constants embedded in the
code are kept by the executor's constant list, which the executor object
traverses. 10b updates spill slots in place. The deoptimiser rebuilds
interpreter frames from the same entry: both ask "where is slot n here",
and two tables would have to agree. Debug builds check the tables against
a conservative scan of the JIT frame at each collection.

## 6. The hazards found in 6b

### 6.1 One past the end

`_PyObject_MakeTpCall`'s saved r13 held `context_run`'s `args + 1`: the end
of a live one-item argument tuple's items, which is the start of the next
32-byte block, a dead object that the scan then keeps.

mimalloc gives objects blocks of 8, 16, 32, 48, 64, 80 bytes and so on. A
1-tuple is 32 bytes and a 3-tuple 48: objects whose size is a multiple of
16 end at their block's end and are the exposed ones. Three remedies:

- **Allocator slack.** A probe added one byte to every object-domain
  request (`PyObject_Malloc`, `Calloc` and `Realloc`), which moves each
  exactly fitting object up one class. Peak RSS on the memory group, on
  build/default at ac59664de47:

  | benchmark | base (KiB) | slack / base |
  |---|---:|---:|
  | startup | 13,484 | 1.012 |
  | imports | 18,024 | 1.024 |
  | tuples_10m | 859,072 | 1.000 |
  | instances_1m | 127,268 | 1.118 |
  | dicts_200k | 65,836 | 1.000 |
  | collector_live16 | 56,524 | 1.096 |
  | collector_live128 | 160,180 | 1.241 |
  | collector_live512 | 584,468 | 1.215 |
  | geomean | | 1.085 |

  tuples_10m does not move because a 2-tuple is 40 bytes in a 48-byte
  block. The cost falls on lists, 1-tuples, instances and boxed ints.
  Rejected: up to a quarter of the heap for a retention case.
- **A scan rule.** The word is an exact block start, the most ordinary form
  of a real pointer; no rule tells the two apart. Rejected.
- **Not scanning the frame.** A precise frame's words are never scanned,
  so the hazard is retired for precise frames. `context_run` is in the call
  layer, so the observed case goes; the two asyncgen tests are its proof.

For opaque frames the hazard remains until 10b stops scanning them. The
verifier counts end-aliased words (8.3), so the exposure is a number.

### 6.2 C-local PyMem arrays

**Inventory.** The union of the 6b audit (note 06 section 19.11), a grep of
core C for PyMem arrays of `PyObject *` or `PyValue`, and the review:

| Site | Array | Span |
|---|---|---|
| `list.sort()` | `saved_ob_item` while `ob_item` is NULL | `[0, saved_ob_size)` from the detach to the restore |
| `list.sort()` | keys | `[0, i)` as key calls fill it |
| `list.sort()` | merge temp (`ms->a.keys`, `ms->a.values`) | the copied run, rebased by `merge_getmem` |
| `_PyList_AsTupleAndClear()` | the list's items after `Py_SET_SIZE(self, 0)` | none: reordered to build the tuple first, then clear |
| `_PyObjectArray_FromValues()` and its callers | boxes of inline arguments | `[0, i)` as boxes are made |
| `PyEval_EvalCodeEx()` | `newargs` | `[0, n)` as copied |
| Objects/call.c `values_of()`, `_PyStack_UnpackDict()`, the two `*Prepend*` buffers | argument copies | `[0, n)` as copied |
| `partial_vectorcall()` | `stack`, and its realloc | two spans, the keyword suffix and the positional prefix, each over what is written, rebased after realloc |
| `mro_implementation_unlocked()` | `to_merge` | `[0, n)` |
| `_Py_VaBuildStack()` and `do_mkstack()` (Python/modsupport.c), with their callers in Objects/call.c (`PyObject_CallFunction`, `PyObject_CallMethod` and the `_SizeT` forms) | the built argument stack | `[0, i)` in `do_mkstack()` as values are built; the caller's span over `[0, n)` through the call and cleanup |
| `PyUnicodeWriter`, `PyBytesWriter` | their buffer objects | `_PyGCRoot`, creation to finish or discard (found by rung 1's stress mode: `PYTHON_GCSTRESS=10 ./build/default/python -m test test_capi.test_bytes -m test_resize` crashes in `finish()` after a resize, the buffer object freed while the writer held it) |
| the compiler's units (`struct compiler_unit` and its `u_metadata` dicts, held in PyMem across collections by Python/compile.c, Python/flowgraph.c and Python/assemble.c) | the unit's dicts and lists | `_PyGCRoot` per unit, from its creation to its free (rung 3's first core site; found by the verifier's kill mode, 10a rung 1: `PYTHON_GCVERIFY=kill:pymem` crashes at start-up in `dict_keys_inorder()`, assemble.c) |

Heap-owned arrays (a list's `ob_item`, the unpickler's `data`) are visited
through their objects and are not on the list. The first grep missed
`do_mkstack()`, whose size is written `n * sizeof(stack[0])`, so rung 3
classifies every PyMem allocation in core C by its result's type (127
calls in Objects/ and Python/ alone) in its work doc, and the PyMem mode
(8.3) with injection at each listed site is the dynamic check. Decision 5
holds only once both are clean; until then the transitive scan stays on
for core frames too.

**Span rules.** `_PyValueRoot` becomes a span: `PyValue *base`,
`Py_ssize_t n`, an inline `value`, `next`. One value is a span of one over
`value`. The span covers the initialized prefix only: `n` is raised after
each element is stored, never before. A replaced buffer (realloc,
`merge_getmem`) is rebased in the span after the copy and before the old
buffer is freed, with no collecting call between. The span is popped
before its buffer is freed, on every path, failure paths included. A
`PyObject *` array is a valid span because a value that refers to an
object is its pointer (pycore_value.h): the visitor marks the pointer as
stored, boxes included, and never re-tags it; 10b updates it in place.
Spans are the one LIFO mechanism for frame-scoped roots, in precise and
opaque functions alike; `_PyGCRoot` stays for roots that outlive a C frame.

### 6.3 The release barrier

`Py_DECREF` and the other release calls keep their object alive up to the
release (note 06 section 19.5). In an opaque frame it keeps the released
pointer in a register or slot that the walk scans, as now. In a precise
frame it adds nothing, since the promise anchors every object the function
uses. 10a does not change it; it stays until 10b's handles, whose proof is
the `PyUnicode_Join` repro with the barrier compiled out.

## 7. Retirement

| Hack-register row (note 08 section 3) | Replaced by | Proof |
|---|---|---|
| `-ftrivial-auto-var-init=zero` for the whole build | precise frames are not scanned; opaque frames' stale locals are retention, and promotion is the remedy for one a test meets | flag removed; stale-root probe 100/100 on both builds; section 8.2 20/20 |
| 16 KB clears before and after a collection, and in `gc.collect()` | the entry paths are precise; the walk starts at the captured context | `_PyGC_LiveClearStack`, `_PyGC_ClearStackBelow`, `_PyGC_ClearStackBuffer` deleted; the same probe |
| 1 KB clear at each C entry to the eval loop | the eval loop's frame is precise; its prologue's saves are the caller's | `_PyGC_LiveClearEvalStack` deleted; the same probe |
| `_PyGC_CLEAR_ARRAY` after handler calls; scratch arrays zeroed | the arrays are in precise frames, their contents anchored or in a span | the macro, its three uses and the `= {0}` in `VALUES_TO_OBJECTS` deleted; suite |
| popped `_PyValueRoot` slots cleared; saved registers cleared after use | a popped span in a precise frame is not scanned; in an opaque frame it is a stale local, retention under the same rule as the flag's removal; start contexts are read only by the walk | the clear in the pop, the `entry_regs` memset and the clear on attach deleted; opaque pushers keep their pushes |
| eval-loop activation ranges; `Py_NO_INLINE` on about 25 helpers | the walk's classification; helpers precise (4.3) | `_PyEvalActivation`, `scan_activations`, `_PyEval_EvalFrameEntry` deleted |
| the about 12 tests that skip or allow a survivor | the walk, the call layer and `gc.collect()`'s wrapper precise | each test in 8.2 without its skip, 20/20 on both builds |
| one-past-the-end pointer in a callee-saved register | precise frames not scanned (6.1) | the asyncgen tests pass on release builds; no end-aliased word owned by a precise frame |
| C-local PyMem arrays reachable only through the transitive scan | spans and `_PyGCRoot` (6.2) | the suite with the transitive scan off for core frames; the PyMem mode reports nothing |
| inline-fallback collection scans everything | allocations are safepoints (4.2) | `scan_activations` and `ALLOCATING` gone |
| release calls are a keep-alive barrier | not retired (6.3) | 10b |

Also deleted: the struct scan (`_PyGC_TRACE_ROOT_STRUCTS`, `scan_structs`,
`scan_thread_struct`) and its asyncio list-head skips; the data stack is
visited by header (4.4). Not retired in 10a, and listed for 10b:
`_PyGC_CLEAR_LOCAL` in `finalize_remove_modules()`, the data-segment scan
and the cache skips in it.

## 8. Tests

### 8.1 The stale-root probe

`Lib/test/test_gc.py`'s `test_eval_loop_frame_is_not_a_root` runs the dead
local and the dead list item 20 times. The probe runs them 100 times, on
both builds, and adds three cases that cross the precise frames 10a adds:
the dead object as the argument tuple of a call through `tp_call`; through
`context.run()`, the 6b alias; and from a generator resumed by `next()`.
Each runs through a Python callback of a C function (`sorted(key=...)`), so
that a core opaque frame lies between two eval-loop frames, and must free
the object at the first collection 100 times in 100. test_gc gets it as
`test_precise_frames_are_not_roots` at 20 runs.

### 8.2 Skips and allowances removed

Each must pass 20 times in 20, alone and in its module's full order, on
dev-default and dev-debug.

- test_weakref: `test_clearing_weakrefs_in_gc`,
  `test_weak_keys_destroy_while_iterating`,
  `test_weak_values_destroy_while_iterating` (release builds; eval-loop
  registers saved by its first C callee);
- test_asyncgen: `test_async_gen_asyncio_gc_aclose_09`,
  `test_async_gen_asyncio_shutdown_exception_02` (release; 6.1);
- test_context: `test_hamt_gc_2` (release);
- _test_multiprocessing: `test_finalize` (release; a padding slot in a call
  frame) and `_TestPool.test_release_task_refs` (all builds; a blocked pool
  thread's call frame);
- test_set `test_container_iterator` and test_memoryview `test_gc` (debug;
  the alignment slot of `gc.collect()`'s wrapper);
- test_descr `test_slots`'s and test_ctypes `test_issue_7959`'s allowance of
  one survivor: both become exact;
- test_gc `TracingCollectorTests.ATTEMPTS`: the seven tests that retry five
  times try once.

If one still fails, `_testinternalcapi.gc_live_roots_of()`, which now
reports the root word's owner (function by `dladdr()`, class, offset),
names the function to promote. An extension owner keeps the skip, named in
its comment, and the row moves to 10b with that evidence.

Known exclusions, which are not 10a's:

- Under stress (`PYTHON_GCSTRESS=1000`), test_capi.test_misc as a whole.
  B class: shared-heap subinterpreter creation (finding B, question 18,
  owner step 7). A collection during a shared-heap interpreter's start-up
  runs a main-interpreter weakref callback or finalizer there, and its
  unraisable report dies. The creators in test_capi, for step 7 to
  re-enable:
  - test_misc: `SubinterpreterTest` (eight tests, among them
    `test_configured_settings`, which died), `TestThreadState.test_gilstate_matches_current`
    (died, through `_testcapi.test_current_tstate_matches`),
    `InterpreterConfigTests`, `InterpreterIDTests` and
    `TestPendingCalls.test_isolated_subinterpreter`;
  - test_misc's `Test_testcapi`, which runs every `_testcapi.test_*`
    function: six call `Py_NewInterpreter()` (`test_atexit`,
    `test_interpreter_guards`, `test_thread_state_ensure_crossinterp`,
    `test_thread_state_ensure_view`, `test_interp_view_after_shutdown`,
    `test_interp_guard_countdown`). They did not die in 1b's runs, by
    chance: the exclusion of test_misc covers them only since the third
    run;
  - test_getargs `test_gh_119213`: its interpreter has its own obmalloc,
    so it is outside the class; it stays in the stress run.
  - A second creator path, found by 10a rung 2b's extra stress run
    (`PYTHON_GCSTRESS=50` on build/debug, reproduced on the rung 1b base):
    test_import `SinglephaseInitTests.test_basic_multiple_interpreters_main_no_reset`,
    whose `_interpreters.create('legacy')` shares the main allocator.
    The C stack (boil/tools/cstack.sh): `Py_NewInterpreterFromConfig`,
    `pycore_init_types`, `_PyStaticType_InitBuiltin`, `type_add_method`,
    `PyStaticMethod_New`, `functools_wraps`, `PyObject_GetOptionalAttr`,
    `meth_get__qualname__`, `PyUnicode_FromFormat`, `object_str`,
    `PyErr_CheckSignals`, `_Py_RunGC`, `collect_pass`, `run_queue`
    (gc_live.c), `PyObject_CallOneArg` of a queued weak reference
    callback, which returns NULL with no exception set in the new
    interpreter's thread state, and the debug result check aborts. Not in
    the gate of record, whose stress set does not run test_import; extra
    stress runs exclude the class patterns under this class (below).
    7a's test covers the case.
  - A third creator path, found by rung 2b's stress run of groups c and d
    (`PYTHON_GCSTRESS=50` on build/debug) and reproduced 2 of 2 on the
    base before 4c (938c5a5501a): test_import
    `SubinterpImportTests.test_builtin_compat`, in its class's order (it
    passes alone, 3 of 3). Its `_testinternalcapi` creator calls
    `Py_NewInterpreterFromConfig`, and the C stack from `pycore_init_types`
    down to the `run_queue` abort is the second path's, with
    `meth_get__qualname__` reached through `functools_copy_attr`; no import
    helper is on the stack. 10a-2b.md's record has the stack.
  - Extra stress runs exclude `-i '*SinglephaseInitTests*' -i
    '*SubinterpImportTests*'` under this class. Neither is in the gate of
    record.

  Also excluded under stress: the writer row of 6.2, owner rung 3, as
  `-i '*test_resize*' -i '*WriterTest*'`. The first is test_capi.test_bytes'
  resize case. The second was added when step 7a's stress ladder (test_capi
  under `PYTHON_GCSTRESS=1000`, only `*test_resize*` excluded) failed 3 of
  3 runs in the same row: test_bytes `BytesWriterTest.test_singletons` (the
  `byteswriter_check_consistency` assertion, and a segfault) and
  test_unicode `PyUnicodeWriterTest.test_write_char` (a segfault). 7a
  reproduced both without its changes on 10a's build/debug at 288a274f57c:
  test_singletons 4 of 10, test_write_char 5 of 10. 10a rung 2b's end
  gate widened them (the coordinator, 2026-10-28) to `-i '*test_resize*'
  -i '*Writer*' -i '*test_unicode.CAPITest.test_from_format'`. Run sequentially under
  `PYTHON_GCSTRESS=1000`, test_capi failed in writer users that
  `*WriterTest*` does not match: PyUnicodeWriterFormatTest's
  `test_recover_error` (a segfault) and test_unicode's `test_from_format`
  (an object corrupted after PyUnicode_FromFormat calls). The build
  before rung 2b's item 6 fails the same way: test_capi.test_unicode at
  N = 50 fails 3 of 3 on both builds. The segfault is a memory-safety
  finding in the current tree, and it is rung 3's first item. The
  exclusions come off when rung 3 roots the writer buffers.

  Also excluded under stress: test_gc's `*test_verify_kill*`, owner rung
  3. Its `kill:pymem` child traps when a collection lands inside
  `makecode`. The compiler unit's `u_metadata` dicts are held only
  through a `PyMem_Calloc` block from `_PyAssemble_MakeCodeObject`'s
  frame, and `kill:pymem` finds such a PyMem holder by design; the live
  collector has not failed. Whether it traps depends on where the stress
  interval lands: 5 of 5 at c91c4bdb282 with N = 1,000, and at N = 200
  on 10a at ba897d05a0c (10a-2b.md, 7a's hand-off). A stress line that
  depends on the interval is not a gate. The exclusion comes off when
  the compiler unit's metadata has a root or a span.
- ASan runs need `ASAN_OPTIONS=detect_stack_use_after_return=0`. With
  ASan's default, a local whose address escapes lives on ASan's
  heap-backed fake stack, which the conservative scan does not see, so
  its referents are killed. 6d's rung 3 ASan run showed it twice, both
  passing with the option: test_gc `test_injection_sites` at the MKSTACK
  site (`values_of` reading the first argument from
  `_PyObject_CallFunctionVa`'s `small_stack` after `do_mkstack`'s
  injected collection), and test_ctypes' `__pointer_type__` getter, whose
  `%R` read a killed str. Rung 3 decides between scanning the fake frames
  (section 10) and keeping the option as the recorded condition.
- Under ASan, test_capi `test_mem.AddressSanitizerTests.test_buffer_overflow`.
  The test sets `PYTHONMALLOC=malloc` so that ASan sees the overflow. The
  fork's mem domain does not honour the variable, so mimalloc's check fires
  first. Note 08's hack register has the row, owner 10a rung 2.

### 8.3 The verifier

In debug builds each collection marks twice before it frees anything:

- **A**, the roots of section 1, which the collection uses;
- **B**, A plus every word 10a skips: words owned by precise frames, the
  struct scan, and PyMem blocks that core opaque frames point to.

Q, the objects in B but not A, were reached only through skipped words. A
side table records each with the words that reached it: the one B marked
it from, then every other skipped word whose value is the object, up to
eight (the rest counted). Each has its address, its owner (function,
class) and the collection number. A report names them all, since which
word B meets first depends on the stack's layout. Two modes use Q, one
for retention and one for detection.

- **`PYTHON_GCVERIFY=retain`**, a diagnostic. B decides: Q is kept
  intact, header and referents untouched, its weakrefs not cleared and its
  finalizers not run, so the program behaves as under the conservative
  collector. A later collection that reaches a Q object from A's precise
  roots reports a candidate with its record; it is not proof, since a
  live weakref, `gc.get_objects()` or `gc.get_referrers()` can hand the
  program a Q object legitimately, and a stale word of a popped precise
  frame can resurface in a new opaque frame. Retain mode attributes
  retention and gives provenance when kill mode traps. It is not a gate,
  and test_gc's reclamation assertions skip under it.
- **`PYTHON_GCVERIFY=kill`**, the gate. A decides: Q dies with the precise
  collector's full semantics (weakrefs cleared, callbacks and finalizers
  run, destroy hooks run). Quarantine starts after the sweep: an object a
  finalizer resurrects lives on as normal, and only blocks actually freed
  are held. Its object blocks and the PyMem blocks its hooks free are
  then held back from the allocator for 16 collections, filled with the
  debug build's freed pattern and given a trap type whose slots and
  attribute access abort with the side-table record; `Py_TYPE` in debug
  builds aborts on the trap type. In an ASan build (configured with
  `--with-address-sanitizer`, mimalloc's ASan tracking, stack use after
  return off, the scan functions unsanitized) the held blocks are poisoned
  too, so every read traps, a field or a cached interior pointer included,
  and the report's address finds its record.

A process that aborts, a kill trap included, gives its C stack without
gdb through `boil/tools/cstack.sh BUILDDIR/python ARGS`: it preloads
`boil/tools/abortbt.c`, which prints `backtrace()` on SIGABRT after
faulthandler's report, and resolves the interpreter's frames with
addr2line.

**Demonstration.** Before the gate counts on either mode,
`_testinternalcapi.gc_verify_missed_root(kind)` hides an object in a
precise frame across a collection on purpose and then uses it by a field
read (`ob_size`), a cached interior pointer (`ob_item`), a slot call
(`tp_repr`), finalization (a finalizer that touches it), and a store into a
list. Kill mode in the debug build must trap the slot call, the finalizer
and the store; kill mode under ASan all five; retain mode must report the
store as a candidate. A sixth case is the counterexample: an object kept
by a stale word and handed back by a live weakref, which kill mode
handles as dead (the weakref returns None) and retain mode classifies as
weakref provenance.

**Gate.** The dev-debug suite in kill mode must finish with zero traps,
reclamation assertions included, since kill mode has the post-10a
collector's semantics; and the 8.2 list and the stress set likewise under
ASan if that build works at rung 0 (13.6). Stale counts per test file are
recorded.

**Other modes.** *PyMem*: objects reached from B only through a PyMem block
of a core opaque frame, reported with the function (6.2). *End aliases*:
conservative words that start a block, are its only reason to be marked,
and equal the end of a marked block, counted by owner: zero for precise
owners by construction. *Stress*: `PYTHON_GCSTRESS=N` collects at every Nth
allocation through the inline path. It runs test_gc, test_weakref,
test_generators, test_coroutines, test_asyncgen, test_descr, test_capi,
test_sort, test_functools and test_context at N = 1,000 at each rung end,
and the full suite at N = 100,000 once. *Injection*:
`_testinternalcapi.gc_collect_at(site)` arms a collection at a named site,
because allocation intervals do not ensure those transitions are hit. The
sites (`_PyGC_SITE_*`, Include/internal/pycore_tstate.h; 0 is none, and
`_PyGC_SITE_COUNT` is 13):

| Site | Name | Where |
|---|---|---|
| 1 | DETACH | a detach entry, before the record |
| 2 | PUSH_FRAME | `_PyThreadState_PushFrame()`, before the frame is initialized |
| 3 | SPAN_REBASE | `_PyValueSpan_Rebase()`, after it |
| 4 | MKSTACK | `do_mkstack()`, after each value |
| 5 | INIT_LOCALS | `initialize_locals()`, after `*args` and `**kwargs` are stored |
| 6 | RETURN | `_RETURN_VALUE`, after the caller's slot takes the value, before the clear |
| 7 | TAKE_OWNERSHIP | `take_ownership()`, before the caller's frame object is made |
| 8 | SLOT_BOX | `box_owner_for_descr()`, after an inline owner's box is in the caller's span |
| 9 | RECEIVER_BOX | `method_check_args()` and `wrapperdescr_call()`, after an inline receiver's box is in the caller's span |
| 10 | SETTER_BOX | `_testinternalcapi`'s `SetterBox` setter, between its box and the store |
| 11 | MATCH_ARGS | `_PyEval_MatchClass()`, after the `__match_args__` lookup |
| 12 | UNPACK_DICT | `_PyStack_KwNames()`, after the names' allocation |
| 13 | SLOT_LOOKUP | `lookup_method_ex()`, after a slot function's method lookup (10a rung 2b item 5) |

### 8.4 New tests

- test_gc: precise frames are not roots (8.1); an opaque C frame between
  two eval-loop frames keeps an object only its local holds across a
  collecting callback; a span keeps an array's prefix; a frame built with
  `-fno-asynchronous-unwind-tables` makes the walk fall back and count it.
- test_gc, by injection: a collection inside `initialize_locals` with
  `*args` and `**kwargs`; a collection during a return whose frame has a
  frame object (`sys._getframe()` taken), the result used afterwards.
- test_gc, borrowing: a C function borrows a list's item, calls back into
  Python, which clears the list and collects, and then uses the item; it
  must be anchored, and kill mode traps it otherwise. The same with a
  weakref's target across an injected allocation.
- Spans: a `PyObject *` span holding a box of a non-NaN float, which must
  survive as the same pointer; NULL entries in a prefix; a rebase; an
  allocation failure mid-fill; `PyObject_CallFunction()` with six freshly
  built arguments and a collection injected in `do_mkstack()`, and its
  failure cleanup; `functools.partial` with merged keywords.
- `_testinternalcapi` walk tests on compiled stacks, not synthetic ones: a
  chain of C functions, each built to keep a different callee-saved
  register live across a callback, with opaque and precise frames
  alternating; a detached thread blocked in `Py_BEGIN_ALLOW_THREADS` with
  an object held only in a callee-saved register; an extension helper
  that detaches and returns before blocking; the swap path in a `-O0`
  build; a kept thread state of an exited thread. The walk must find each
  opaque value and skip each precise frame.
- Loader: a small shared library built twice with different frame layouts;
  one copy is loaded, a callback through it collects, it is unloaded, the
  other is loaded at the same path, and a callback collects again; the
  cache must have flushed (counter) and the walk attribute correctly.
- test_capi: the two writers keep their buffers across a collection.
- test_generated_cases: the negative tests of 2.1.

## 9. Measurement

Recorded, not gated, as every step records (note 10):

- **Root phase per collection**: frames walked, words scanned by class
  (opaque stack, data segments, PyMem), walk time, cache hits, misses,
  flushes and fallbacks, from `_PyGC_LiveGetStats()` through
  `_testinternalcapi.gc_root_stats()`, on collector_live16, 128 and 512 and
  summed over the suite, before (ac59664de47) and after.
- **Cold and worst cases**: the descriptor cache's memory after the suite;
  first-collection decode time after startup and after importing the
  stdlib's shared modules; a 1,000-deep recursion through C callbacks; 100
  detached threads; the pause distribution (median, p99, max) on the
  collector benchmark; the detach capture's cost on a pipe-read loop;
  repeated `PyObject_Vectorcall()` from C with small and heap argument
  arrays, for the public entries' spans.
- **The internal set** as label `step10a`: the collector benchmark (time
  and instr), the memory group's RSS, kernels instr with perf attribution
  as in note 06 section 19.10. The baseline is the merged predecessor of
  10a's first rung, which replaces ac59664de47 if 6c lands first.

Expected direction, without a target: the root phase down slightly (eval
frames and the 16 KB clears go, data segments stay); fewer instructions
where the eval loop is entered from C (each entry loses a 1 KB `rep stos`)
and from removing `-ftrivial-auto-var-init` (note 06 section 15.4: +0.3% on
richards and float, +0.65% on go); a few more on public call entries that
push spans and on each GIL release; peak RSS flat or down.

## 10. Rungs

Five and a half days, hard stop at the end of day 6. Each rung has a
reviewed work doc (`boil/design/work/10a-<n>.md`) and ends at gate level 3
(boil/PROCESS.md).

| Rung | Days | Work | Ladder (smallest first) |
|---|---|---|---|
| 0 | 0.5 | Baseline counters (9); c_roots committed as `boil/tools/measure/c_roots.py`; measurements at ac59664de47; the ASan build tried | `python -c pass`; counters on collector_live16 |
| 1 | 2.5 | The C mode (2.1) and its negative tests, first; the walk (3.1 to 3.4), the capture stub and detach unwind, classification, the loader generation; the data-stack and chain visits, the entry visitor and the return order (4.3, 4.4); the inline audit; kill and retain modes, injection and stress; then `Py_GC_PRECISE` on the eval loop, the handlers and the entry paths, each passing the C mode; allocations as safepoints; activations, clears, `_PyGC_CLEAR_ARRAY` and the inline full scan deleted | the negative tests; the C mode on the eval loop's hand-written parts; the inline audit; the compiled-stack walk tests; the injection tests; the probe; test_gc; test_weakref, test_generators, test_coroutines, test_asyncgen, test_capi, test_ctypes; the loader test; both suites; stress on the rung's set |
| 2 | 1 | The ceval.c helpers and the call layer precise, with spans and `Py_GC_ARGS_ANCHORED`; the 8.2 skips removed | the C mode on call.c alone; the borrowing test; the probe's call-layer cases; test_call, test_capi, test_descr, test_context, test_asyncgen, test_weakref, multiprocessing; both suites; 8.2 at 20/20 |
| 3 | 1 | PyMem: the spans of 6.2, `_PyList_AsTupleAndClear` reordered, the writers' `_PyGCRoot`, the PyMem mode; kill mode and its demonstration; transitive scan off for core frames once the mode is clean; `-ftrivial-auto-var-init=zero` removed; the struct scan deleted; the raw-buffer owners (the bytes constructors' `const char *`, 10a-2b.md 2b); the stack scan follows words into ASan's fake frames (`__asan_addr_is_in_fake_stack`) and scans them, or `detect_stack_use_after_return=0` stays the recorded condition of ASan runs (8.2), decided in the rung's work doc | test_sort, test_functools, test_descr's mro tests, test_json, test_unicode; the demonstration; both suites; the verifier gate; 8.2 at 20/20 without the flag |
| 4 | 0.5 | Gate level 4: `step10a` recorded, note 06 section 20 with measurements and attribution, the hack register updated; the platform matrix, which records that a `DEBUG_PYDICT` build is outside the precise-roots contract (its `ASSERT_CONSISTENT` hashes keys, which call_effects.py takes for non-collecting) | `boil/bench/run.py --set all --metric instr,rss` and the memory group's time on an idle machine |

**What blocks a rung from landing**, at any time and at the hard stop: a
trap in kill mode, a walk fallback in the suite, a C-mode
or inline-audit failure, or the probe below 100/100. Retention counts and
measurements do not.

**Scope cut**, in this order, if a rung passes its deadline by more than
half a day: the arm64 walk stays disabled, conservative in full; the typeobject.c `slot_*` functions stay opaque, with their
tests' skips naming the owner; the transitive PyMem scan stays on for core
frames (decision 5 moves to 10b); the struct scan stays to 10b. Never cut: the walk, the eval loop,
the entry paths and the dispatch core precise, the transitions of 4.4, the
verifier and the probe.

**Hard stop.** Commit what passes level 3, write the hand-off list in note
06 section 20, and report.

## 11. What step 7 gets

- **An enumerable root set**, in classes a minor collection visits in full
  each time: data-stack frames, value-root spans, `_PyGCRoot`, thread and
  interpreter fields, static types, immortals, the finaliser queue, opaque
  frames' words and data segments. None needs a barrier; conservative words
  mark in place, and pin in 10b.
- **Where the remembered set hooks.** The write barrier goes on every store
  into a heap object: the container store macros, instance slots, cells,
  globals dicts. A frame's slots are written with no barrier while it is on
  the data stack or running on the chain, both visited in full by every
  minor collection. A frame whose storage is a heap object adds its owner
  to the remembered set when it stops running: a generator or coroutine at
  yield and at return (`owned_slots`), and a frame object at
  `take_ownership()`: one barrier per suspension, not per slot store,
  which tier 0 and tier 1 inherit. Step 7's test: a minor collection
  inside a resumed old generator, and an old coroutine, after it stores a
  young object and before its next yield.
- **The verifier pattern.** Step 7's barrier verifier (a full mark after
  each minor collection) uses retain mode's form on the difference.
- **The header.** 10a changes no header bit; step 7 decides side bitmaps
  for the mark flags (note 09 section 2).

## 12. The 10b migration list

A token-level probe (rung 0 commits it as `boil/tools/measure/c_roots.py`)
lists every function in the core tree that holds an object-typed local or
parameter (`PyObject *`, `Py*Object *`, `PyValue`) across a call outside the
non-collecting list. It is flow-insensitive, so it overcounts loops and
dead paths, and misses pointers in structs:

| Tree | Functions | Hold an object across a call |
|---|---:|---:|
| Objects/ | 3,885 | 1,661 |
| Python/ | 3,252 | 758 |
| Modules/ (without vendored libraries and clinic output) | 7,717 | 2,494 |
| Parser/ | 749 | 25 |
| total | 15,603 | 4,938 |

The largest files are typeobject.c (261), dictobject.c (193),
_cursesmodule.c (146), _decimal.c (134), _datetimemodule.c (124),
exceptions.c (112), _ctypes.c (104), longobject.c (93), _pickle.c (92) and
abstract.c (91). They are opaque after 10a, except the about 130 that 10a
makes precise, and are 10b's work list. The verifier adds the confirmed
subset, the functions whose frames held the only root of a live object;
10b's first rung starts from it.

## 13. Open questions

1. **Extension identity is by linkage.** A stdlib module built into the
   binary is core opaque; built shared, it is an extension, and its PyMem
   blocks stay followed. Recommendation: keep it, as the conservative side
   of the line; record which stdlib modules the gated configurations build
   shared, and let 10b's migration remove the distinction.
2. **Clinic wrappers.** Only `gc.collect()`'s wrapper becomes precise.
   Recommendation: the rest belong with 10b's handle signatures.
3. **The tail-call build** is not a gated configuration (boil/PROCESS.md
   gates `CC=gcc` builds). Recommendation: rung 2 runs the probe, the
   compiled-stack walk tests and the 8.2 list on build/dev-tailcall once
   and records the result.
4. **The verifier in release builds.** Recommendation: the gate on debug
   builds, plus one release run of the 8.2 list in retain mode at the step
   end, where most retention was found.
5. **A test of 8.2 that fails with an extension owner once the flag is
   gone.** Recommendation: the flag goes; the test keeps a skip naming the
   function, owned by 10b.
6. **If the ASan build does not work at rung 0.** Kill mode then traps slot
   calls, type checks and finalizers but not every field read.
   Recommendation: proceed with the debug-build kill mode and the retain
   gate, record the gap, and make the ASan configuration 10b's first item.
7. **The detach capture's cost.** Two or three warm steps per GIL release.
   Recommendation: accept it if the pipe-read loop moves less than 1%;
   otherwise keep the detach frames on the stack until reattach (a
   capture in the caller's macro, `Py_BEGIN_ALLOW_THREADS`), a larger
   change to the public macros. Measured in rung 1 (2026-10-27) on a loop
   of 10^6 one-byte `os.write` and `os.read` pairs on a pipe, 2·10^6
   detaches, instructions on build/default: 3.219G before the walk
   (8abd6f7f185), 5.204G with the eager record that unwound to P at each
   detach, about +1,000 instructions a detach against a 1% budget of about
   16. The test failed; question 16 is the ruling. With it, 3.14G to 3.20G.
8. **Tier 1's frames under inspection.** `f_locals`, `sys._getframe()` and
   reentrant callbacks need virtual frames and stale physical slots
   materialized. Recommendation: deoptimise on inspection from the same
   safepoint entry, as V8 does; 10a fixes only that the map is complete,
   and tier 1's design owns materialization.
9. **Adoption.** Note 06 section 6 and note 08's retirement rows are
   updated when this note is adopted, and each partial landing records in
   the hack register which mechanisms it actually retired.
   Recommendation: the coordinator does both at adoption, the rung reports
   the second.
10. **The C mode's size.** Section 2.1 is a checker over a C subset with a
    control-flow graph, liveness, pointer provenance and span scopes. It is
    the largest single tool in rung 1 and the one most likely to overrun.
    Ruling at adoption: it has one day inside rung 1, built before any
    function is promoted. If its negative tests do not all fail at the end
    of that day, rung 1 promotes the eval loop and the entry paths on the
    kill-mode verifier, the retain gate and the inline audit alone, the
    work doc lists every promoted function as "C mode pending", and the C
    mode is the first item of rung 2 and runs over that list before rung 2
    promotes anything else. No function is promoted without one of the two
    proofs, and the 8.2 skips are not removed on the dynamic proof alone.
11. **Arguments of a collecting call.** Decision 3 says every argument of
    a collecting call is anchored; 2.1 says an object parameter is the
    callee's to anchor. Ruling at rung 0 (coordinator, 2026-10-27):
    arguments to a `Py_GC_ARGS_ANCHORED` callee are anchored by the
    caller. For every other call, a parameter is the callee's to anchor
    across the callee's own safepoints (2.1), and the caller owes nothing
    for it after the call begins. A temporary that is live across a
    collecting call, including a sibling argument's call evaluated before
    it (`f(PyLong_FromLong(5), g())`), is an unanchored object local and
    an error in the caller; the C mode treats temporaries as object locals
    for liveness.
12. **Item and cell readers collect.** In this tree the object-returning
    item and cell readers (`PyTuple_GET_ITEM`, `PyList_GET_ITEM`,
    `PyCell_GET`, the dict value getters) box an inline value, so they
    allocate and are collecting calls; 2.1's example of `PyTuple_GET_ITEM`
    as non-collecting is wrong for this tree. Ruling at rung 0: the
    non-collecting list contains only the value-returning readers and the
    field accessors that do not box; a precise function reads items
    through the value accessors (`PyTuple_GET_VALUE` and the like); an
    object-returning reader in a precise function is a collecting call and
    its result an object local. The same correction moved c_roots's count
    (section 12) from 4,938 to 6,644 of 16,251 functions at the baseline:
    the committed probe treats these readers as collecting, and it counts
    a value from the end of the statement that assigns it, which the
    uncommitted probe's description does not settle (rung 0's work doc).
13. **Stack-passed arguments to opaque callees.** Question 11's parameter
    rule assumes the callee's frame scan covers its parameters. An opaque
    callee's scan covers its own extent, but an argument the platform ABI
    passes on the stack (position 7 or later on x86-64, variadic arguments
    included) lies in the caller's extent, which is precise and not
    scanned. Ruling in rung 1 (coordinator, 2026-10-27): the call is not
    rejected; in a precise function, an object or `PyValue` passed at
    position 7 or later to a callee that is not precise must be anchored in
    the caller at the call, by a slot, a span or a borrow. The C mode
    reports the unanchored case with its own error naming the position.
    It checks collecting calls only, since a call that cannot collect has
    no safepoint at which the argument is read, and it counts from 7 on
    AArch64 too, which checks a superset of that ABI's eight registers.
    Precise callees are unchanged: 2.1 has them span a parameter they use
    after a collecting call. The eval loop keeps the transitional scan of
    `[sp, lo)` in its activation frames until it is promoted.
14. **A saved register its saver has not changed.** Section 3.1 gives a
    pending register value to the first frame outward that saves the
    register. A frame that has saved a register and not yet written it
    holds the caller's value in it, so the walk counts a pending value
    equal to the one the saver saved as the caller's. Accepted in rung 1
    (coordinator, 2026-10-27) as the reading of the CFI and of 3.1 that
    the walk implements.
15. **`DW_CFA_register`.** The walk treats a register saved in another
    register as an unsupported rule and falls back for that walk. Accepted
    in rung 1 (coordinator, 2026-10-27) on the condition that the suite
    never reaches it: a fallback blocks a landing (section 10), so if a
    gate shows one, the walk implements the rule.
16. **The detach record: pinned and unpinned, no walk at detach.** Ruling
    in rung 1 (coordinator, 2026-10-27), after 13.7's measurement. Each
    record kind has one mechanism. A pinned record comes from
    `Py_BEGIN_ALLOW_THREADS`, `Py_UNBLOCK_THREADS` and the core's pinned
    entries (`_PyEval_SaveThreadPinned()`, `_PyEval_ReleaseThreadPinned()`,
    `_PyThreadState_DetachPinned()`): an entry in assembly captures its
    caller's pc, stack pointer and callee-saved registers before anything
    changes them, about 20 instructions. The caller does not return before
    the matching reattach, so every frame outward of the capture persists,
    and the collector walks from the capture lazily, under the GIL, when it
    visits the thread. An unpinned record comes from a direct
    `PyEval_SaveThread()`, `PyEval_ReleaseThread()`,
    `PyThreadState_Swap(NULL)` or `_PyThreadState_Detach()`, whose caller
    may return detached; the collector scans its registers and
    `[cap.sp, stack top)` conservatively, PyMem followed, and never computes
    P. Both rest on one argument: what a detached thread holds it obtained
    attached, so it is in a frame that existed at the capture or in a
    captured register; frames pushed below the capture after the detach are
    not scanned. The loader generation is checked once per root phase and
    before a cache miss is decoded; nothing runs `dl_iterate_phdr()` at a
    detach. The counter `unpinned` reports the unpinned records a root phase
    visits, in the gate output with `fallbacks`.
17. **The code range table's limit.** `_PyGC_RegisterCodeRange()` keeps 64
    ranges; registration past that fails, and a walk through unregistered
    code with no FDE falls back, which `fallbacks` counts (debug builds
    print the pc once). In rung 1 its users are the perf trampoline's
    arenas, 64 KiB each (2,048 trampolines), and the walk tests. A fallback
    blocks a landing (section 10), so the gate prints the suite's
    `fallbacks` and fails when it is not zero.
18. **Callbacks in a shared heap (for step 7).** The interpreters that
    share the main interpreter's allocator are collected as one domain,
    but the queue of weak reference callbacks and finalizers is the
    collecting interpreter's, so a legacy subinterpreter runs the main
    interpreter's callbacks, even inside its own start-up, where the
    unraisable hook's type is not ready and the error path crashes. Found
    by rung 1's stress mode: `PYTHON_GCSTRESS=1000 ./python -c 'import
    _testcapi; _testcapi.run_in_subinterp("x = 1")'`. Latent without
    stress. Ruling (coordinator, 2026-10-27): not a root and not 10a's; a
    queue per owning interpreter, drained at that interpreter's own
    safepoint, needs an ownership attribution the collector does not
    have, and step 7, which decides domains, owns it (note 08 section 3;
    test_gc `test_stress_shared_heap_callbacks` skips until then).
    Rung 1b's stress run found it three times in test_capi.test_misc
    (10a-1b.md, end gates). A main-interpreter `_ModuleLock`'s weakref
    callback (`importlib._bootstrap._get_module_lock.<locals>.cb`) and a
    main-interpreter `__del__` were run on the new interpreter during
    `pycore_init_types`. With `use_main_obmalloc`, any collection during
    a subinterpreter's initialization can run another interpreter's
    callbacks on an interpreter whose unraisable hook is not ready, and
    the process dies. Stress is not needed to reach it, only to make it
    frequent. 10a has no stopgap for it.

## 14. Sources

- Note 06 sections 3.4, 6, 14.2, 14.3, 15.2, 15.4, 16.3, 17.2, 18 and 19;
  note 08 sections 3 and 6; note 09 section 2; note 10; note 05.
- Code at ac59664de47: Python/gc_trace.c (`scan_stack`, `scan_structs`,
  `live_mark_roots`, the capture functions), Python/gc_live.c
  (`collect_from_entry`, `_PyGC_LiveCollectHere`), Python/ceval_gil.c
  (`PyEval_SaveThread`), Python/pystate.c (`_PyThreadState_PushFrame`,
  `_PyThreadState_PopChunk`, `detach_thread`), pycore_interpframe.h,
  pycore_gc_trace.h, pycore_tstate.h, pycore_value.h, Python/ceval.c
  (`_PyEvalFramePushAndInit`, `_PyObjectArray_FromValues`),
  Python/bytecodes.c (`_RETURN_VALUE`), Objects/listobject.c,
  Objects/call.c, Tools/cases_generator/root_check.py and analyzer.py.
- Probes on build/default at ac59664de47 (gcc 15.2.0), not committed: the
  root_check safepoint probe (4.2), allocator slack (6.1), c_roots (12), a
  grep for PyMem object arrays (6.2), `readelf --debug-dump=frames-interp`
  of the eval loop, `perf record` of collector_live16.
- DWARF 5 section 6.4 (call frame information); the x86-64 psABI's
  `.eh_frame` conventions; glibc's `_dl_find_object()` and
  `dl_iterate_phdr()`; clang's `preserve_none`; HotSpot's `RegisterMap`;
  V8's safepoint tables and Maglev's deoptimisation data; Boehm-Demers-
  Weiser for the conservative rules opaque frames keep.

## Review

Round 1 (boil/design/reviews/11-precise-roots-review.md), verdict revise.
Each blocking issue was taken as a change:

1. *Frames outside the chain hold live roots.* A frame is a root from push
   to pop by a data-stack visit, and transitions store before unlinking
   (4.4); injection tests (8.3, 8.4).
2. *Borrowed anchoring across reentrant calls.* Borrows are split by call
   kind: any field across non-reentrant calls, listed immutable edges
   across reentrant ones; `FT_ATOMIC_LOAD` dropped as an anchor; parameters
   are the callee's to anchor unless `Py_GC_ARGS_ANCHORED` (2.1); the
   borrowing test (8.4).
3. *The C checker's assumptions.* A separate C mode with a declared subset,
   a control-flow graph, provenance, span scopes, three call kinds and
   negative tests before promotion (2.1); the inline audit closes the
   inlining question.
4. *PyMem inventory and span lifetime.* The inventory table with
   `_PyList_AsTupleAndClear` reordered; span rules for prefixes, rebasing,
   failure paths and raw pointers; decision 5 conditional on the verifier;
   opaque pushers keep their spans (6.2, 7).
5. *The quarantine verifier.* Retention and detection are separate modes;
   retain mode changes nothing in the graph; kill mode gives Q the precise
   semantics and traps uses, every read under ASan; a demonstration on
   intentional missed roots precedes the gate (8.3).
6. *The walker's start context.* An assembly capture at a call boundary,
   detach frames unwound at capture, `ra - 1` lookup, validation, the
   `preserve_none` clause, compiled-stack tests (3.1, 3.3, 3.4, 8.4).
7. *Load address as a cache generation.* Loader-generation flush, decoded
   copies only, JIT unregistration, and the reload test (3.2, 8.4).

Non-blocking: the scope against notes 06 and 08 is stated at the top; tier
1's map covers temporaries, physical slots and constants (5); the entry
frame has its own visitor (4.3); cold and worst-case costs are recorded
(9). The reviewer's questions are answered by open question 1, the inline
audit (2), the landing rule (10) and injection (8.3).

Round 2 (the same file, overwritten), verdict revise. Each blocking issue
was taken as a change; the step brief allows two rounds, so there is no third, and
what remains is in section 13.

1. *Non-reentrant calls do not preserve weak edges.* Borrows need a strong
   traced field; weak fields and caches need a span; a negative test and
   an injected collection between a weakref load and its use (2.1, 8.3,
   8.4).
2. *A detached snapshot can describe a returned frame.* The detach unwind
   now runs to the innermost precise frame, which runs attached and so
   persists; everything below it is one conservative region with PyMem
   followed, as today; exited threads are clipped to readable mappings;
   tests for a returning helper, the swap path at `-O0` and an exited
   thread (3.3, 3.4, 8.4).
3. *The PyMem inventory misses `do_mkstack()`.* Added with its callers;
   `partial_vectorcall()` gets two spans; rung 3 classifies every PyMem
   allocation by type; injection and failure tests (6.2, 8.4).
4. *Retain mode reports legitimate weakref access.* Retain mode is a
   diagnostic that reports candidates; kill mode, with the post-10a
   semantics, is the gate; quarantine starts after the sweep and
   resurrection; the weakref counterexample is a demonstration case (8.3).
5. *Rung 1 removes protection before its proof.* The C mode and its
   negative tests come first in rung 1, the stamp rule makes every marked
   function pass it, the helpers move to rung 2 and stay scanned until
   then, and the eval loop's generated region is defined for the C mode
   (2, 2.1, 10).
6. *Running heap frames.* The chain is visited too, so a running generator
   or coroutine frame is a root from resume to suspension, and step 7's
   once-per-suspension barrier holds; step 7's test named (4.4, 11).
7. *arm64 capture.* Per-architecture capture and return-address rules; the
   arm64 walk is disabled until its tests pass, which is the first scope
   cut (3.2, 3.3, 3.4, 10).

Non-blocking: the inline audit fails closed (2); raw-box, NULL, rebase and
failure span tests (8.4); a C API measurement and the baseline rule (9);
adoption (13.9). Questions: the cache is per interpreter and the JIT range
table process-wide under a mutex (3.2); every fallback follows PyMem
(3.4); tier 1 materialization is 13.8; unsupported platforms keep full
conservative scanning without the mitigations (3.4).
