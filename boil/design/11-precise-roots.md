# 11: Precise roots: an attributed stack walk over precise and opaque frames

Status: design, 2026-10-27, for step 10a (note 06 section 6, note 08
sections 3 and 6). It decides what the collector reads as roots after 10a,
how it tells a root from a stale word on a C stack, and what tier 0 and
tier 1 must provide; it retires the hack-register rows assigned to 10 and
10a. Moving and the handle discipline for C code remain 10b's.

## Decisions

1. **Every C stack is walked through its unwind tables.** Each word of each
   stack is assigned to exactly one owning frame. A callee-saved register
   that a callee spilled is owned by the frame whose value it is. The walk
   replaces the activation ranges and their saved registers, the stack
   clears and the inline collection's full scan (section 3).
2. **Frames are precise or opaque, by the code range of their function.**
   The collector never scans a word that a precise frame owns, and scans
   the words of opaque frames conservatively, as today. Precise code is the
   eval loop and its helpers, the call layer, the collector's entry paths
   and JIT code; other core C and every extension are opaque (section 2).
3. **A precise frame's promise.** At every call that may collect, which
   means every call that may allocate an object or run Python code, every
   object the function uses afterwards is anchored. It is in a frame slot,
   in a `_PyValueRoot` span pushed on the thread state, in a field of an
   object or state struct that the mark visits, or borrowed from such a
   value. root_check checks it for handlers, root_check's C mode for the
   listed precise C functions, and the quarantine verifier for all of them
   at run time (section 2).
4. **Core C stays opaque in 10a.** 4,938 of the 15,603 functions in
   Objects/, Python/, Modules/ and Parser/ hold an object local across a
   call that may collect (section 12). Making them precise is the handle
   migration, which is 10b's. 10a promotes the call layer (about 80
   functions), the eval-loop helpers (about 25) and the entry paths, where
   the stale roots observed through 6b live.
5. **PyMem blocks are followed from extension frames only.** Core C that
   keeps objects in a C local's PyMem array roots it as a `_PyValueRoot`
   span, or registers a `_PyGCRoot` for a heap-lived block; the verifier
   finds the sites (section 6.2).
6. **Allocator slack is rejected.** One word of slack per object block
   costs a geomean of 8.5% of peak RSS on the memory group, and 24% on
   collector_live128. The one-past-the-end hazard is retired for precise
   frames, because they are not scanned. It remains for opaque frames until
   10b (section 6.1).
7. **The struct scan goes**; the thread and interpreter structs' objects
   and the data stacks are visited precisely. Data segments stay to 10b.
8. **Tier 0 keeps every value in a frame slot at every call.** Tier 1
   records one safepoint table per call site and per eval-breaker check:
   where each live interpreter slot is, read by both the collector and the
   deoptimiser. At a safepoint a tagged value is in a frame slot or a listed
   spill slot, never a register. Both tiers register descriptors with the
   walk (section 5).
9. **Step 7 inherits an enumerable root set and one barrier rule**: slots of
   a frame on the chain need no barrier, and a heap-owned frame remembers
   its owner when it leaves the chain (section 11).

## 1. What is scanned after 10a

| Root | Before 10a | After 10a |
|---|---|---|
| interpreter frames on the frame chain, generator frames, frame objects | precise | precise; the eval loop's entry frame slot is visited too (4.3) |
| thread state and interpreter fields, static types, immortal objects, the finaliser queue, `_PyGCRoot` | precise | precise |
| `_PyValueRoot` | precise, one value | precise, a span of values (section 6.2) |
| eval-loop C frames | skipped by activation range, except in an inline collection | owned words never scanned, in every collection |
| registers the eval loop saved for its caller (`act.regs`) | scanned from the activation record | owned by the caller; scanned only if the caller is opaque |
| callee-saved registers of precise frames, spilled by any callee | scanned | not scanned |
| other precise frames (call layer, entry paths, JIT) | scanned | not scanned |
| opaque frames, core and extension | scanned | scanned |
| PyMem blocks a scanned word points into | scanned, transitively | scanned only from extension frames |
| thread and interpreter structs, data stacks | scanned (`_PyGC_TRACE_ROOT_STRUCTS`) | not scanned |
| writable data segments | scanned | scanned (10b) |

Note 06 section 14.3's conservative rules apply unchanged to what is still
scanned: interior pointers, shallow marking, the NOSCAN heap.

How a collection inside a C call finds the eval loop's live values: they
are the frame slots, through the frame chain; the values that handlers and
precise helpers pushed as `_PyValueRoot` around escaping calls, including
the boxes of inline arguments; the exception state in the thread state; and
the entry frame's slot, which a returning frame writes (4.3). A
value returned in a register to the eval loop's C caller is that caller's:
scanned if it is opaque, rooted by its promise if it is precise. Nothing
else in the eval loop's C frame or registers is live.

## 2. Frame classes

| Class | Members | Promise | Check |
|---|---|---|---|
| eval loop | `_PyEval_EvalFrameDefault`; every generated handler in the tail-call build; every function defined in Python/ceval.c, including the about 25 helpers that are now `Py_NO_INLINE` | decision 3 | root_check on handler bodies, as now, with allocating calls as safepoints (4.2); root_check's C mode on the ceval.c functions; the verifier |
| call layer | Objects/call.c's call and vectorcall dispatch (`PyObject_VectorcallValue`, `_PyObject_VectorcallTstate`, `_PyObject_MakeTpCall`, `_PyVectorcall_Call`, `_PyObject_Call`, `PyObject_Call`, `_PyStack_UnpackDict`, `_PyFunction_Vectorcall`, the `_PyObject_Vectorcall*Prepend*` forms, `_Py_CheckFunctionResult*`); `cfunction_vectorcall_*` and `cfunction_call`; `method_vectorcall_*`, `wrapperdescr_call` and `classmethoddescr_call`; `method_vectorcall`; `context_run`; the generator send path (`gen_send_ex2`, `gen_send_ex`, `gen_iternext`, `gen_throw` and the coroutine wrappers); typeobject.c's `slot_*` functions and `vectorcall_method` family, which call back into Python. About 80 functions | decision 3 | root_check's C mode over the listed functions, run by the build; the verifier |
| entry paths | the collector (Python/gc_live.c and the mark in Python/gc_trace.c); `gc.collect()`'s clinic wrapper and impl; `_PyGC_Collect`; `_Py_HandlePending` and `_Py_RunGC`; the allocation path from `PyObject_Malloc`, `gc_alloc`, `_PyObject_GC_New`, `_PyObject_GC_NewVar`, `PyType_GenericAlloc` and the boxing retry to `_PyGC_LiveOnThreshold` and `_PyGC_LiveCollectHere`; the GIL-release capture (`_PyGC_TraceCaptureThread`, `PyEval_SaveThread`, the detach path) | decision 3; most hold no object at all | the same C check; the verifier |
| JIT | tier 0 blocks, tier 1 code | section 5 | the template generator applies root_check to tier 0's templates; the tier 1 safepoint table is checked against a full conservative scan in debug builds (section 8) |
| core opaque | every other function in the interpreter's image (the `python` binary or libpython) | none | none until 10b |
| extension opaque | every function in any other loaded object, including the stdlib's shared modules | none | none until 10b |

**Marking a precise function.** `Py_GC_PRECISE` on its definition, or on a
prior declaration for generated code such as the clinic wrapper, places it
in the section `py_gc_precise`. The linker provides
`__start_py_gc_precise` and `__stop_py_gc_precise`, and a return address
in that range classifies its frame. The cases generator emits the macro on
every handler in the tail-call build. A precise function inlined into an
opaque caller, or a `.cold` part split out of one, is classified opaque,
which is safe; rung 1 checks with `nm` that no precise function has a
`.cold` part.

**root_check's C mode.** The checker that guards handlers (note 06 section
17.2) works on the token stream of a C statement block. Its C mode reads
the definitions marked `Py_GC_PRECISE` in a listed set of files, adds
`switch` and `case` to the statement parser (a switch body is checked as a
block whose cases are alternatives), and treats as a safepoint every call
except those in the non-collecting list that analyzer.py already keeps.
A static inline function that may collect is checked through its body. A
struct local that holds objects (`_PyUnicodeWriter`, `_PyBytesWriter`,
`Py_buffer`) is rejected unless its objects are anchored. The build runs the
C mode as a stamp rule over the listed files, so a violation fails `make`.
A debug scan asserting that a precise frame holds no heap pointer cannot
work, because precise frames hold copies of anchored pointers and stale
words. The run-time check is instead the quarantine verifier (8.3): a
skipped word was the only reference to an object the program later used.

## 3. The walk

**Ownership.** The walk goes outward from a thread's innermost frame. For
each frame it has the return address (pc), the stack pointer at the call
(sp), and from the unwind descriptor the canonical frame address (CFA) and
the slots where the frame saved each callee-saved register. The frame's extent is `[sp, CFA)`. Every word of the extent is
owned by the frame, except its save slots. A save slot holds the caller's
value of a register. That value belongs to the first frame, going outward
from the caller, that itself saves the register, because the frames between
never modify it. The walk keeps, per callee-saved register, the locations
whose owner is not yet known. The innermost set is the slot of the captured
register array. When a frame saves the register, it becomes the owner of
those locations and the save slot replaces them. At the base of the stack
the remaining locations are owned by the outermost frame. Each location
therefore has one owner, and a word is scanned if and only if its owner is
opaque.

This is HotSpot's register-map rule for compiled frames. It is exact for
every frame with unwind information, whatever compiler or calling convention
made it, and needs neither frame pointers nor a prologue shape.

**Descriptors.** A descriptor gives, for a return address, the CFA rule (a
register and an offset), the location of the return address, and the save
slot of each callee-saved register. On x86-64 those are rbx, rbp and r12 to
r15. On arm64 they are x19 to x29 and d8 to d15. A JIT descriptor adds a
list of tagged slots (section 5). For C code a descriptor is derived from
the object's `.eh_frame`. `_dl_find_object()` (glibc 2.35 and later; the
machine has 2.43) gives the object and its `PT_GNU_EH_FRAME`. A binary
search of `.eh_frame_hdr` gives the FDE, whose call-frame instructions are
interpreted up to the pc (the register and offset rules GCC and clang emit;
an expression rule counts as no descriptor). Descriptors are cached by
return address, validated by the object's load address, so a reloaded
object never reuses a stale entry.

The python binary has 8,000 FDEs (a 315 KB `.eh_frame`). The eval loop's is
regular: CFA `rbp+16`, the six registers at `CFA-16` to `CFA-56`, and a
1,024-byte frame (968 bytes of locals and spills, six pushes, the return).

**Start states.** The current thread starts at the collection's entry,
which captures the callee-saved registers, its caller's pc and its CFA
(`_PyGC_TraceCaptureEntry()` captures registers and sp today). Another
thread, or a kept daemon thread state, starts at what it captured when it
released the GIL, which now records the pc and CFA too. Frames from there
outward are stable while the thread is detached, as today.

**No descriptor.** If a pc has no descriptor (code without unwind
information, a foreign JIT, a corrupt return address), the walk stops. It
scans the rest of the stack, from that frame's sp to the stack base, and
every location still pending, conservatively. It counts the event. The
count is a statistic in release builds. Debug builds print the pc and its
object once; the suite must record zero. Conservative scanning is the
failure mode of the one walk, not a second path.

**Cost.** A warm step is a hash lookup, a few loads and a range test: about
a microsecond per hundred frames. On collector_live16 today's whole
conservative scan is 0.79% of cycles, about 0.34 ms of a 19 ms collection
(35 collections in 1.52 s, `perf record`), mostly the data segments.

## 4. The eval loop

### 4.1 Its C frame, in both builds

**The plain build** keeps the dispatch state (`next_instr`, `slots`,
`frame`, `tstate`) in C locals of one 1,024-byte frame. The walk classifies
the frame by pc and skips it. Its prologue saves the caller's callee-saved
registers inside the frame's extent, and the walk assigns them to the
caller. `_PyEvalActivation`, `_PyEval_EvalFrameEntry`, `act.regs`,
`scan_activations` and the record of `lo` are deleted. A PEP 523 hook's
frame is opaque and the eval loop it calls is precise, however entered.

**The tail-call build** keeps the dispatch state in registers across
handlers, and each handler is its own function with its own frame. Every
handler is precise by `Py_GC_PRECISE`, so its frame is skipped. When a
handler calls a C function, that function may save the handler's registers
in its own frame; the walk assigns those slots to the handler and skips
them. Both builds get one rule, with no record per entry or per call.

### 4.2 Allocations are safepoints

An object allocation past the second threshold collects at once, without
the eval breaker (note 06 section 15.2). Today that collection scans the
eval loop's frames, because the handler that allocated may hold a fresh
object only in a C local. After 10a no collection scans them, so the root
rule must hold at allocating calls too. A probe ran root_check with the
allocating calls that analyzer.py lists as non-escaping (`PyCell_New`,
`PyLong_FromLong`, `PyFloat_FromDouble`, `_PyTuple_FromValues`, the boxing
functions and others) as safepoints, and the `ALLOCATING` exemption for
`PyType_GenericAlloc` removed. It flagged two handlers, both false
positives:

- `MAKE_CELL`: `initial` is still in its slot;
- `_ALLOCATE_OBJECT`: `init_func` is borrowed from the type's spec cache,
  and the type is in a slot.

The fix is two anchoring rules in root_check: a value read from a slot
whose slot is not overwritten before the call, and
`FT_ATOMIC_LOAD_PTR_ACQUIRE` of a field of an anchored object. Then
allocators are safepoints and `ALLOCATING` is deleted; the analyzer's
comment that boxing "cannot start a collection" is corrected. An inline
collection then walks and marks as one at the eval breaker does, and only
defers finalizers (`no_python`).

### 4.3 The entry frame and the helpers

`_PyEval_EvalFrameDefault` keeps an `_PyEntryFrame` in its C frame; a frame
returning to it writes `entry.stack[0]`, which `INTERPRETER_EXIT` returns.
The frame visitor skips interpreter-owned frames, so that slot is in a
skipped C frame. No collecting call falls between the write and the exit,
and 10a makes that explicit: the visitor visits interpreter-owned slots.

The about 25 helpers in Python/ceval.c and Python/ceval.h that are
`Py_NO_INLINE` exist so that their frames are scanned: they hold the boxes
of inline arguments (`VALUES_TO_OBJECTS`), call results and argument
arrays in C locals. Each becomes precise, pushes a span for what it holds,
and loses `Py_NO_INLINE` where scanning was its only reason. root_check's C
mode checks each.

## 5. The JIT

**Tier 0** (note 05) compiles a block into a tail-call handler whose inline
paths store every result in its slot and whose other instructions call
handlers. Its registers (r12 to r15: frame, slot pointer, thread state,
`next_instr`) hold no object.
The rule: at every call every live value is in a frame slot. The templates
come from the handler definitions, so root_check checks them with
allocations as safepoints; an inline path that boxes (an overflowing int
add) stores the box before the next call. A block registers one
descriptor for its range: CFA rule, saved rbp, no tagged words, class JIT.
A C callee's saves of r12 to r15 are then the tier 0 frame's and skipped.

**Tier 1** keeps unboxed numbers and object references in registers across
instructions and inlines callees whose frames are virtual. Its safepoint
table entry, one per call site and eval-breaker check, lists every live
interpreter slot of the physical and inlined frames with its location: a
frame slot, a spill slot in the JIT frame (CFA-relative), a constant, or an
unboxed register value. The rule at a safepoint is that a tagged value is in a frame
slot or a listed spill slot, never in a register. The collector visits the
listed spill slots that hold tagged values, and 10b updates them in place.
The deoptimiser rebuilds interpreter frames from the same entry: both ask
"where is slot n here", and two tables would have to agree. Tier 1
registers descriptors (frame rule, save slots, table) by code range. Debug
builds check the tables against a conservative scan of the JIT frame at
each collection (8.3).

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
  block. The cost falls on lists, 1-tuples, instances and boxed ints, the
  collector benchmark's units. Rejected: up to a quarter of the heap for a
  retention case.
- **A scan rule.** The word is an exact block start, the most ordinary form
  of a real pointer; no rule tells the two apart. Rejected.
- **Not scanning the frame.** A precise frame's words are never scanned,
  so the hazard is retired for precise frames. `context_run` is in the call
  layer, so the observed case goes. The two asyncgen tests are its proof.

For opaque frames the hazard remains until 10b stops scanning them. A tuple
layout with the hash after the items would close it for tuples at no memory
cost; it is not taken, because every other trailing array (a generator's
slots) stays exposed. The verifier counts end-aliased words (8.3), so the
remaining exposure is a measured number.

### 6.2 C-local PyMem arrays

The 6b audit (note 06 section 19.11) lists core functions whose objects are
reachable only through the transitive scan of a PyMem block that a C local
points to:

- `list.sort()`: the keys array, `saved_ob_item` while `ob_item` is NULL,
  and the merge temp array (Objects/listobject.c);
- `_PyObjectArray_FromValues()`: the block of boxes for a long argument
  list (Python/ceval.c);
- `PyEval_EvalCodeEx()`: the argument copy (Python/ceval.c);
- `partial_vectorcall()`: the stack (Modules/_functoolsmodule.c);
- `mro_implementation_unlocked()`: `to_merge` (Objects/typeobject.c).

Each pushes a `_PyValueRoot` span over its array for as long as it holds
the array. `_PyValueRoot` becomes a span (`PyValue *base`, `Py_ssize_t n`,
an inline `value`, `next`); one value is a span of one over `value`, and a
`PyObject *` array is a valid span because a value that refers to an object
is its pointer (pycore_value.h). Spans are the one LIFO mechanism for
frame-scoped roots; `_PyGCRoot` stays for roots that outlive a C frame
(arenas, thread bootstate, cross-interpreter data).

The public writers that keep an object in a PyMem block of their own,
`PyUnicodeWriter` and `PyBytesWriter`, register a `_PyGCRoot` for their
buffer from creation to finish or discard, for core and extension callers.

The verifier's PyMem mode (section 8.3) reports every object that is
reached only through a PyMem block that a core opaque frame points to,
with the owning function; each is rooted or shown stale, and then the
transitive scan from core opaque frames is turned off. Extension frames
keep it: under the conservative contract (note 06 section 14.3) a C
function's buffers are its locals, until 10b.

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
| `-ftrivial-auto-var-init=zero` for the whole build | precise frames are not scanned; opaque frames' stale locals are retention only, and promotion is the remedy for one that a test meets (section 10, rung 3) | flag removed from configure.ac; stale-root probe 100/100 on both builds; the retention tests of section 8.2 20/20 |
| 16 KB clears before and after a collection, and in `gc.collect()` | the entry paths are precise; the walk starts at the entry's captured state | `_PyGC_LiveClearStack`, `_PyGC_ClearStackBelow` and `_PyGC_ClearStackBuffer` deleted; the same probe |
| 1 KB clear at each C entry to the eval loop | the eval loop's frame is precise; its prologue's save slots are the caller's | `_PyGC_LiveClearEvalStack` deleted; the same probe |
| `_PyGC_CLEAR_ARRAY` after handler calls; scratch arrays zeroed | the arrays are in precise frames; what they hold is anchored or in a span | the macro, its three source uses and the `= {0}` in `VALUES_TO_OBJECTS` deleted; suite |
| popped `_PyValueRoot` slots cleared; saved registers cleared after use | value roots are pushed only by precise functions, whose words are not scanned; saved registers are read only as a walk's start state, and the struct scan is gone | the clear in `_PyThreadState_PopValueRoot`, the `entry_regs` memset and the clear on attach deleted; the opaque pushers in Objects/ (the 6b uses in typeobject.c, abstract.c, object.c and call.c) either become precise or drop the push, which an opaque frame does not need |
| eval-loop activation ranges; `Py_NO_INLINE` on about 25 helpers | the walk's classification; helpers precise (4.3) | `_PyEvalActivation`, `scan_activations` and `_PyEval_EvalFrameEntry` deleted; the attribute dropped where it served the scan |
| the about 12 tests that skip or allow a survivor | the walk, the call layer and the clinic wrapper of `gc.collect()` being precise | each test in section 8.2 without its skip, 20/20 on both builds |
| one-past-the-end pointer in a callee-saved register | precise frames not scanned (6.1) | the two asyncgen tests pass on release builds; the verifier counts no end-aliased word owned by a precise frame |
| C-local PyMem arrays reachable only through the transitive scan | `_PyValueRoot` spans; `_PyGCRoot` for the writers (6.2) | the suite passes with the transitive scan off for core frames, and the verifier's PyMem mode reports nothing |
| inline-fallback collection scans everything | allocations are safepoints (4.2) | one scan path: `scan_activations` gone and the `ALLOCATING` exemption deleted |
| release calls are a keep-alive barrier | not retired (6.3) | 10b |

Also deleted: the struct scan (`_PyGC_TRACE_ROOT_STRUCTS`, `scan_structs`,
`scan_thread_struct`) and its asyncio list-head skips. The data stack is
reached through the frame chain; a frame unlinked before its slots are
cleared needs no backstop, since clearing a slot runs no finaliser since 5c.
A frame in transit that the verifier finds (`take_ownership()`) is fixed
by ordering the link and the copy.

Not retired in 10a, and listed for 10b: `_PyGC_CLEAR_LOCAL` in
`finalize_remove_modules()` (an opaque shutdown frame), the data-segment
scan, and the cache skips in it.

## 8. Tests

### 8.1 The stale-root probe

`Lib/test/test_gc.py`'s `test_eval_loop_frame_is_not_a_root` runs the dead
local and the dead list item 20 times. The probe runs them 100 times, on
both builds. It adds three cases that cross the precise frames 10a adds:

- the dead object as the argument tuple of a call through `tp_call`
  (`_PyObject_MakeTpCall`);
- the dead object through `context.run()`, the 6b alias;
- the dead object from a generator resumed by `next()` (the generator send
  path).

Each runs through a Python callback of a C function (`sorted(key=...)`),
so that a core opaque frame lies between two eval-loop frames, and must
free the object at the first collection 100 times in 100. test_gc gets it
as `test_precise_frames_are_not_roots` at 20 runs.

### 8.2 Skips and allowances removed

Each must pass 20 times in 20, alone and in its module's full order, on
dev-default and dev-debug.

- test_weakref: `test_clearing_weakrefs_in_gc`,
  `test_weak_keys_destroy_while_iterating`,
  `test_weak_values_destroy_while_iterating` (release builds). Cause: eval
  loop registers saved by its first C callee.
- test_asyncgen: `test_async_gen_asyncio_gc_aclose_09`,
  `test_async_gen_asyncio_shutdown_exception_02` (release). Cause: one past
  the end (6.1).
- test_context: `test_hamt_gc_2` (release).
- _test_multiprocessing: `test_finalize` (release; a padding slot in a call
  frame) and `_TestPool.test_release_task_refs` (all builds; a blocked pool
  thread's call frame).
- test_set `test_container_iterator` and test_memoryview `test_gc` (debug).
  Cause: the alignment slot of `gc.collect()`'s clinic wrapper, which is
  precise.
- test_descr `test_slots`'s allowance of one survivor in the `g == g`
  count, and test_ctypes `test_issue_7959`'s allowance of one live `X`:
  both become exact.
- test_gc `TracingCollectorTests.ATTEMPTS`: the seven tests that retry five
  times with fresh objects try once.

If one still fails, `_testinternalcapi.gc_live_roots_of()`, which now
reports the root word's owner (function by `dladdr()`, class, offset),
names the function to promote. An extension owner keeps the skip, named in
its comment, and the row moves to 10b with that evidence.

### 8.3 The verifier

`PYTHON_GCVERIFY=precise` in debug builds compares the precise root set
with the conservative one over the suite, as step 4 did for reachability.
Before it can free anything, each collection marks twice:

- **A**, the roots of section 1, which are what the collection uses;
- **B**, A plus every word 10a skips: the words owned by precise frames,
  the struct scan, and the PyMem blocks that core opaque frames point to.

Objects in B but not A were reached only through skipped words. Such an
object is quarantined, not freed: its header points at a quarantine shape
whose type's slots report and abort, and a side table keeps the original
header, the word's address, its owning function and class, and the
collection number. A later collection that B no longer reaches it from
frees it and counts it stale. If A reaches it later, or a slot call or
`Py_TYPE` check touches it, it was live: a missed root, reported with its
record, and the process aborts. The dev-debug suite must finish with zero
missed roots; stale counts per test file are recorded.

Two more modes:

- **End aliases.** Conservative words that start a block, are the only
  reason it is marked, and equal the end of a marked block are counted by
  owner: zero for precise owners by construction, recorded for opaque.
- **Stress.** `PYTHON_GCSTRESS=N` collects at every Nth object allocation
  through the inline path, with the verifier on. It runs test_gc,
  test_weakref, test_generators, test_coroutines, test_asyncgen,
  test_descr, test_capi, test_sort, test_functools and test_context at
  N = 1,000 at each rung end, and the full suite at N = 100,000 once at the
  step end. It exercises the safepoint rule at allocations in handlers,
  helpers and the call layer, and later tier 0's and tier 1's maps.

### 8.4 New tests

- test_gc, through `_testinternalcapi` functions: precise frames are not
  roots (8.1); an opaque C frame between two eval-loop frames keeps an
  object only its local holds across a collecting callback; a span keeps
  an array's objects (as `gc_verify_with_pymem_array` does now); a frame
  built with `-fno-asynchronous-unwind-tables` makes the walk fall back and
  count it.
- test_capi: the two writers keep their buffers across a collection.
- test_generated_cases: root_check rejects a handler holding a fresh object
  across `PyLong_FromLong`, and its C mode a precise function holding one
  across `PyObject_VectorcallValue`.

## 9. Measurement

Recorded, not gated, as every step records (note 10):

- **Root phase per collection:** frames walked, words scanned by class
  (opaque stack, data segments, PyMem), walk time, cache misses and
  fallbacks, as counters in `_PyGC_LiveGetStats()` exposed through
  `_testinternalcapi.gc_root_stats()`; on collector_live16, 128 and 512 and
  summed over the suite, before (ac59664de47) and after.
- **The internal set** as label `step10a`: the collector benchmark (time
  and instr), the memory group's RSS, kernels instr with perf attribution
  as in note 06 section 19.10.

Expected direction, without a target:

- Root-phase time per collection down slightly: eval frames' 1 KB each and
  the 16 KB clears go, the walk costs a microsecond per hundred frames, and
  the data segments stay; the pause moves little.
- Instructions per loop go down where the eval loop is entered from C:
  generators and coroutines resumed by `send` or `next()`, and callbacks.
  Each entry loses a 1 KB `rep stos`. Removing `-ftrivial-auto-var-init`
  returns what it cost (note 06 section 15.4: +0.3% on richards and float,
  +0.65% on go).
- Instructions go up a little on the call layer's `tp_call` and keyword
  paths, which push spans: a few stores per call.
- The eval loop's own section may change its layout: measured, attributed,
  not tuned. Peak RSS flat or down, from less retention.

## 10. Rungs

Five days, hard stop at the end of day 6. Each rung has a reviewed work doc
(`boil/design/work/10a-<n>.md`) and ends at gate level 3 (boil/PROCESS.md).

| Rung | Days | Work | Ladder (smallest first) |
|---|---|---|---|
| 0 | 0.5 | Baseline: root counters (9), the c_roots probe of section 12 committed as `boil/tools/measure/c_roots.py`, measurements at ac59664de47 | `python -c pass`; counters on collector_live16 |
| 1 | 1 to 2 | The walk: descriptor cache from `.eh_frame`, ownership, classification by `py_gc_precise`, fallback; `Py_GC_PRECISE` on ceval.c, the handlers and the entry paths; allocations as safepoints in root_check; the entry frame's slot visited; activations, both clears, `_PyGC_CLEAR_ARRAY` and the inline full scan deleted; the verifier and stress modes | `nm` shows no `.cold` part of a precise function; a C unit test of the walk's ownership on a synthetic stack in `_testinternalcapi`; the stale-root probe; test_gc; test_weakref, test_generators, test_coroutines, test_asyncgen, test_capi, test_ctypes; both suites; stress on the rung's set |
| 2 | 1 | The call layer and the helpers precise: spans where they hold boxes, tuples or kwargs dicts; root_check's C mode with `switch`; the skips of 8.2 removed | the C mode on call.c alone; the probe's call-layer cases; test_call, test_capi, test_descr, test_context, test_asyncgen, test_weakref, multiprocessing; both suites; 20/20 loop on the 8.2 list |
| 3 | 1 | PyMem: spans at the five sites, the writers' `_PyGCRoot`, the verifier's PyMem mode, transitive scan off for core frames; `-ftrivial-auto-var-init=zero` removed; the struct scan deleted; the clear on pop and the saved-register clears deleted | test_sort, test_functools, test_descr's mro tests, test_json, test_unicode; both suites; the verifier over the debug suite; the probe and 8.2 at 20/20 with the flag removed |
| 4 | 0.5 | Gate level 4: the internal set recorded as `step10a`, note 06 section 20 with measurements and attribution, the hack register updated | `boil/bench/run.py --set all --metric instr,rss` and the memory group's time metric on an idle machine |

**Scope cut**, in this order, if a rung passes its deadline by more than
half a day: the typeobject.c `slot_*` functions leave the call layer (they
stay opaque and their tests keep skips naming the owner); the struct scan
stays to 10b; arm64 descriptors are compiled but not run (x86-64 is gated;
an untested platform's walk falls back conservatively). Never cut: the
walk, the eval loop, entry paths and call-dispatch core precise, the
verifier and the probe.

**Hard stop.** Commit what passes level 3, write the hand-off list in note
06 section 20, and report.

## 11. What step 7 gets

- **An enumerable root set**, in classes the minor collection visits in
  full each time: frame slots on the chain, value-root spans, `_PyGCRoot`,
  thread and interpreter fields, static types, immortals, the finaliser
  queue, opaque frames' words and data segments. None needs a barrier;
  conservative words mark in place, and pin in 10b.
- **Where the remembered set hooks.** The write barrier goes on every store
  into a heap object: the container store macros, instance slots, cells,
  and globals dicts. A frame's slots are written with no barrier while the
  frame is on the chain, because the chain is a root. A frame whose storage
  is a heap object adds its owner to the remembered set when it leaves the
  chain: a generator or coroutine at yield and at return (`owned_slots`),
  and a frame object at `take_ownership()`: one barrier per suspension,
  not per slot store, which tier 0 and tier 1 inherit.
- **The verifier pattern.** Step 7's barrier verifier (a full mark after
  each minor collection) quarantines the difference rather than reporting.
- **The header.** 10a changes no header bit; step 7 decides side bitmaps
  for the mark flags (note 09 section 2).

## 12. The 10b migration list

A token-level probe (rung 0 commits it as `boil/tools/measure/c_roots.py`)
lists every function in the core tree that holds an object-typed local or
parameter (`PyObject *`, `Py*Object *`, `PyValue`) across a call that may
collect (any call outside the non-collecting list). It is
flow-insensitive, so it overcounts loops and dead paths, and misses
pointers in structs:

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
makes precise, and are 10b's handle-migration work list. The verifier adds
the confirmed subset, the functions whose frames were seen holding the only
root of a live object; 10b's first rung starts from it.

## 13. Open questions

1. **Core and extension opaque frames differ in one rule** (decision 5):
   PyMem blocks are followed only from extension frames. The alternative
   follows them from every opaque frame and leaves the PyMem row to 10b.
   Recommendation: the split, because the verifier can audit the core now
   and cannot audit extensions.
2. **Clinic wrappers.** Only `gc.collect()`'s wrapper becomes precise.
   Making every generated parser precise (clinic emitting spans for boxed
   `object` parameters) would remove stale padding in core call frames.
   Recommendation: not in 10a; it belongs with 10b's handle signatures.
3. **The tail-call build** is covered by the same mechanism but is not a
   gated configuration (boil/PROCESS.md gates `CC=gcc` builds).
   Recommendation: rung 2 runs the stale-root probe and the 8.2 list on
   build/dev-tailcall once and records the result; it is not a gate.
4. **The verifier in release builds.** Recommendation: the suite gate on
   debug builds, plus one release run of the 8.2 list at the step end,
   where most retention was found.
5. **If removing `-ftrivial-auto-var-init=zero` makes a test of 8.2 fail
   with an extension owner.** Recommendation: the flag goes; the test keeps
   a skip naming the function, owned by 10b.

## 14. Sources

- Note 06 sections 3.4, 6, 14.2, 14.3, 15.2, 15.4, 16.3, 17.2, 18 and 19;
  note 08 sections 3 and 6; note 09 section 2; note 10; note 05.
- Code at ac59664de47: Python/gc_trace.c (`scan_stack`, `scan_structs`,
  `live_mark_roots`), Python/gc_live.c (`collect_from_entry`,
  `_PyGC_LiveCollectHere`), pycore_gc_trace.h, pycore_tstate.h,
  pycore_value.h, pycore_structs.h, Python/ceval.c, Python/ceval_macros.h,
  Objects/obmalloc.c, Tools/cases_generator/root_check.py and analyzer.py.
- Probes on build/default at ac59664de47 (gcc 15.2.0), not committed: the
  root_check safepoint probe (4.2), allocator slack (6.1), c_roots (12),
  `readelf --debug-dump=frames-interp` of the eval loop, `perf record` of
  collector_live16.
- DWARF 5 section 6.4 (call frame information); the x86-64 psABI's
  `.eh_frame` conventions; glibc's `_dl_find_object()`; HotSpot's
  `RegisterMap`; V8's safepoint tables and Maglev's deoptimisation data;
  Boehm-Demers-Weiser for the conservative rules opaque frames keep.
