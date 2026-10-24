# 03: Deferred reference counting in the default build (plan item 1c)

Status: design, 2026-10-23. Line numbers refer to `8dc2e138d6d`, whose
`Python/`, `Objects/` and `Include/` trees are identical to `321925c92c9`, the
base of notes 01 and 02. Measurements come from `build/baseline-default/python`
(gcc 15, -O3, no PGO) and the free-threaded `build/baseline-ft/python`.

## Decisions

1. Model C: deferred reference counting for long-lived objects, plus the
   existing compiler borrows for locals. 1c adds no borrows from the heap.
2. A deferred object has bit 3 of `ob_flags` set and a bias of 2^30 in
   `ob_refcnt`. Stack references to it carry `Py_TAG_REFCNT` and own nothing.
   This applies to 64-bit hosts only.
3. Code objects stay untracked by the GC. `f_executable` borrows from
   `func_code`; assigning `__code__` converts affected frames to owned
   references.
4. Full collections account for deferred objects exactly: they subtract the
   bias and add one per stack reference found by scanning every thread's frame
   chain. Young and middle collections, and any collection while `gc.freeze()`
   holds objects, treat deferred objects as roots.
5. A tagged reference to a deferred object may live only in a scanned frame
   slot, a `_PyCStackRef`, or a heap frame, never in a bare C local across an
   escaping call. `RETURN_VALUE` hands its result to the caller before it
   clears the dying frame.
6. `sys.getrefcount` and `Py_REFCNT` report the raw count, bias included, as
   the free-threaded build already does.
7. Expected gain: under 1 percent of instructions and no measurable time on
   the five target benchmarks. 1c is justified as Phase 2 root enumeration and
   as fewer writes to shared objects, not as a speed item.
8. Land 1c before IR step 4.

## 1. What the code does today

Bit 0 of a pointer stackref means "owns no count"
(`Include/internal/pycore_stackref.h:532-549`). `PyStackRef_FromPyObjectNew`
tags immortals and increfs everything else (`:593-611`). `DUP` and `CLOSE`
test bit 0 only (`:637-645`, `:698-705`). `PyStackRef_IsHeapSafe` accepts
untagged, NULL or immortal references (`:657-669`).

The free-threaded build already has the model. `_PyObject_SetDeferredRefcount`
sets `_PyGC_BITS_DEFERRED` and adds `_Py_REF_DEFERRED` (2^60) to
`ob_ref_shared` (`Objects/object.c:2815-2824`,
`Include/internal/pycore_object.h:28`). The GC subtracts the bias
(`Python/gc_free_threading.c:998-1001`) and adds one for each tagged reference
on any thread's frame chain or C stackref list (`:437-481`). Unreachable
deferred objects lose the bias before finalisation, and generator and frame
objects in the garbage get strong references (`:249-300`). In the default
build `_PyObject_HasDeferredRefcount` returns 0
(`Include/internal/pycore_object_deferred.h:19-27`). Every setter call is
therefore inert there.

The default GC needs no stack scan today. Every stack reference to a mortal
object either owns a count or borrows from a local that owns one, so the count
appears as external in `update_refs` (`Python/gc.c:397-437`).
`_PyGC_VisitStackRef` already skips tagged references during `visit_decref` and
follows them during `visit_reachable` (`gc.c:459-471`).

The cost of one owned pair against one tagged pair, measured with `perf stat`
as in note 02 (per inner iteration, skeleton subtracted, cycles ±2):

| Loop body | Instructions | Cycles |
|---|---|---|
| `z = x; z = y`, owned floats (two owned pairs) | 63.2 | 12.7 |
| same, `None` and `True` (two tagged pairs) | 57.1 | 13.8 |
| `gm; gm`, mortal global (two `LOAD_GLOBAL_MODULE` + `POP_TOP` pairs) | 111.9 | 29.2 |
| same, global bound to `None` | 107.9 | 30.2 |
| `g()`, empty top-level function | 294 | 76 |

An owned pair costs 2 to 3 instructions more than a tagged one and no
measurable time.

## 2. Model

| | A: deferred objects | B: compiler borrows | C: both (chosen) |
|---|---|---|---|
| Locals | no | yes, exists | exists |
| Call arguments into the callee frame | no | yes, exists | exists |
| Deferred globals, module attributes, methods | yes | only until the next escape | A |
| Callable in `f_funcobj` during the call | yes | no: the callee is an escape | A |
| `f_executable` | needs GC-tracked code | borrow from `func_code` | borrow |
| Attribute values, `FOR_ITER` items | no | only until the next escape | no |
| Finalisation timing | changes for deferred objects | unchanged | deferred objects only |
| GC change | stack roots | none | stack roots |
| Fate in Phase 2 | scan survives | dies with refcounts | scan survives |

B cannot cover heap loads. A borrowed heap value is safe only until arbitrary
code runs, and a call's callable stays live across the callee. Every hazard
below arises during argument evaluation or inside the callee:

| Hazard during the borrow | B: heap borrow | A: deferred |
|---|---|---|
| `del globals()['f']` or `global f; f = g` | `f` freed while on the stack | bias keeps the count above zero; the scan keeps `f` alive in a full collection |
| `C.m = other` while `self.m(...)` evaluates arguments | old function freed | same as above |
| `del m.f`, `del sys.modules['m']` | freed | same as above |
| `obj.__dict__.clear()` in a callee | value freed | value is ordinary, so it stays owned |
| `obj.__class__ = D` | old heap type freed | heap types are deferred |
| `f.__code__ = c2` while `f` runs | old code freed | fix-up in `func_set_code` (section 4) |

Immortalisation is not an alternative. It never frees dynamically created
classes and functions.

Hot instructions, default build:

| Instruction | Refcount ops today | After 1c |
|---|---|---|
| `LOAD_FAST` (`Python/bytecodes.c:285-288`) | +1, consumer −1 | unchanged |
| `LOAD_FAST_BORROW`, `..._LOAD_FAST_BORROW` (`:290-312`) | 0 | 0 |
| `LOAD_GLOBAL_MODULE`, `_BUILTIN` (`:2270-2307`) | +1 unless immortal, consumer −1 | 0 for deferred values |
| `LOAD_ATTR_MODULE` (`:2889-2919`) | +1 attribute; `POP_TOP` −1 module if owned; consumer −1 | 0 when both are deferred |
| `LOAD_ATTR_METHOD_*` (`:4200-4292`) | +1 descriptor, `self` moves | 0 for the descriptor |
| `LOAD_ATTR_CLASS`, `_PROPERTY` | +1 | 0 for deferred values |
| `LOAD_DEREF` (`:2395-2402`) | +1 via `PyCell_GetRef` (`pycore_cell.h:52-70`) | 0 for deferred contents |
| `CALL_PY_EXACT_ARGS`, `_PUSH_FRAME` (`:4573-4629`) | `f_executable` +1 (`pycore_interpframe.h:195`); exit −1 `f_funcobj`, −1 `f_executable` | 0 |
| `CALL_BOUND_METHOD_EXACT_ARGS` (`:4540-4548`) | +1 `im_self`, +1 `im_func`, −1 method | `im_func` 0 |
| `CALL_ALLOC_AND_ENTER_INIT` (`:4726-4773`) | type −1, `__init__` +1/−1, code +1/−1 | 0 |
| `CALL_BUILTIN_*`, `CALL_LEN`, `CALL_METHOD_DESCRIPTOR_*` | callable −1 | 0 |
| `RETURN_VALUE` (`:1567-1594`; `Python/frame.c:99-135`) | +1 if the result is borrowed and mortal; −1 per owned slot, `f_funcobj`, `f_executable` | +1 only for borrowed non-deferred results; −1 per owned slot |
| `FOR_ITER_LIST`, `_TUPLE` (`:3935-4016`) | +1 item, `STORE_FAST` −1 | unchanged |
| `FOR_ITER_RANGE` (`:4053-4066`) | allocation | unchanged here; note 02 inlines it |
| `FOR_ITER_GEN`, `SEND_GEN` (`:4074-4094`, `:1725-1739`) | 0; `YIELD_VALUE` +1 if borrowed | unchanged |

The `FOR_ITER` family is out of reach. Its items are ordinary objects, and
the loop body can mutate the container.

The deferred set follows the free-threaded build, minus objects whose prompt
death matters or that the default GC does not track:

| Object | Setter | Default build |
|---|---|---|
| Functions, except nested non-methods | `Objects/funcobject.c:227-236` | yes; nested closures stay prompt |
| `classmethod`, `staticmethod` | `funcobject.c:1566`, `:1828`, `:1995` | yes |
| Method, member, getset and wrapper descriptors | `Objects/descrobject.c:923` | yes |
| Module-level builtin functions, `__new__` wrappers | `Objects/moduleobject.c:312`, `Objects/typeobject.c:10465` | yes |
| Modules | `moduleobject.c:231` | yes |
| Heap types | `typeobject.c:4444`, `:5704`, via unique IDs | yes, via a direct call |
| Code objects | `Objects/codeobject.c:751-754` | no: untracked (`:2896-2900`); borrowed instead |
| Module dicts, MRO tuples, thread-locals, opportunistic globals | `dictobject.c:1852`, `typeobject.c:667`, `_threadmodule.c:1567`, `specialize.c:398-409` | no |

`PyUnstable_Object_EnableDeferredRefcount` stays a no-op in the default build
(`object.c:2827-2859`), so the set stays closed.

## 3. Representation

On 64-bit hosts `ob_flags` shares the word with `ob_refcnt`
(`Include/object.h:126-148`). Bits 0 and 2 are taken (`:580-583`); bit 3 is
free outside free-threaded debug builds. The bias of 2^30 stays below the
immortal threshold of 2^31 (`Include/refcount.h:47-48`), so no deferred object
looks immortal. `_PyObject_SetDeferredRefcount` writes the flag and the bias
directly and leaves `_Py_RefTotal` alone. A 3.11-era extension that decrements
the whole 64-bit word never reaches zero either. `PyStackRef_FromPyObjectSteal`
keeps stolen references owned, because the stolen count is real.

The fast path of `_LOAD_GLOBAL_MODULE`, `_LOAD_ATTR_MODULE` and the method
loads keeps its single load of the refcount word:

```c
static inline _PyStackRef
_PyStackRef_FromPyObjectNew(PyObject *obj)
{
    /* Immortal: bit 31 of ob_refcnt. Deferred: bit 3 of ob_flags, bit 51 of the
       word on little-endian hosts. One movabs and one test, as today's js. */
    if ((uint64_t)obj->ob_refcnt_full & _Py_TAGGED_ON_LOAD_MASK) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_REFCNT };
    }
    _Py_INCREF_MORTAL(obj);
    return (_PyStackRef){ .bits = (uintptr_t)obj };
}
```

`PyStackRef_IsHeapSafe` also accepts tagged deferred references.
`_PyCell_GetStackRef` calls `PyStackRef_FromPyObjectNew` in place of
`PyCell_GetRef`. The predicates live in pointer space, as note 02 section 6
requires. Inline values are never deferred.

## 4. GC integration

Full collections (generation 2 with an empty permanent generation) are exact.
All other collections pin deferred objects by leaving the bias in `gc_refs`.

```c
/* Python/gc.c */
static Py_ssize_t
update_refs(PyGC_Head *containers, int exact)
{
    ...
        Py_ssize_t refs = Py_REFCNT(op);
        if (exact && _PyObject_HasDeferredRefcount(op)) {
            refs -= _Py_REF_DEFERRED_GIL;          /* 2^30; may reach 0 */
        }
        gc_reset_refs(gc, refs);
        _PyObject_ASSERT(op, refs > 0 || _PyObject_HasDeferredRefcount(op));
    ...
}

static inline void
add_stack_root(_PyStackRef ref)
{
    if (!PyStackRef_IsPointerSpace(ref) || PyStackRef_IsNull(ref)
        || PyStackRef_RefcountOnObject(ref)) {
        return;                     /* owned refs are already in ob_refcnt */
    }
    PyObject *op = PyStackRef_AsPyObjectBorrow(ref);
    if (_PyObject_HasDeferredRefcount(op)) {   /* borrows and immortals: nothing */
        PyGC_Head *gc = AS_GC(op);
        if (gc_is_collecting(gc)) {
            gc->_gc_prev += (uintptr_t)1 << _PyGC_PREV_SHIFT;
        }
    }
}

static void
add_thread_stack_roots(PyInterpreterState *interp)
{
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        for (_PyCStackRef *c = ((_PyThreadStateImpl *)p)->c_stack_refs; c; c = c->next) {
            add_stack_root(c->ref);
        }
        for (_PyInterpreterFrame *f = p->current_frame; f; f = f->previous) {
            if (f->owner >= FRAME_OWNED_BY_INTERPRETER) {
                continue;
            }
            assert(f->stackpointer_valid);        /* Py_DEBUG */
            add_stack_root(f->f_funcobj);
            add_stack_root(f->f_executable);
            for (_PyStackRef *r = f->localsplus; r < _PyFrame_ScanEnd(f); r++) {
                add_stack_root(*r);
            }
        }
    }
    _Py_FOR_EACH_TSTATE_END(interp);
}

/* deduce_unreachable(base, unreachable, reachable, exact) */
    Py_ssize_t candidates = update_refs(base, exact);
    if (exact) {
        add_thread_stack_roots(_PyInterpreterState_GET());
    }
    subtract_refs(base);
    *reachable = move_unreachable(base, unreachable);

/* gc_collect_main() */
    int exact = generation == NUM_GENERATIONS - 1
                && gc_list_is_empty(&gcstate->permanent_generation.head);
    stats.candidates = deduce_unreachable(young, &unreachable, &reachable, exact);
    ...
    move_legacy_finalizer_reachable(&finalizers);
    undefer_unreachable(&unreachable);   /* clear flag and bias; frames get strong refs */
```

`undefer_unreachable` ports `disable_deferred_refcounting` and
`frame_disable_deferred_refcounting` (`gc_free_threading.c:249-300`). It runs
before weakref callbacks and finalisers, so a resurrected object is an
ordinary refcounted object. Cyclic garbage that contains deferred objects
(a top-level function and its globals dict) is collected as before:
`gc_refs` falls to zero, the bias goes, and `tp_clear` frees the cycle by
refcount.

**Frames.** Executing frames, generator frames included, are on
`PyThreadState.current_frame` chains: `SEND` and `FOR_ITER_GEN` link the
generator frame through `previous` (`bytecodes.c:1694`, `:4083`). Suspended
generator, coroutine and frame-object frames are not enumerated. They are
containers, and `gen_traverse` (`Objects/genobject.c:82-105`) reaches their
slots through `_PyGC_VisitFrameStack`. A tagged slot there is an uncounted edge
that `visit_reachable` follows. That is exact when every container is in the
collected set, which is why only full collections are exact. In a young
collection an old suspended generator is not traversed, so a young deferred
object it references could look dead. Pinning covers this at no fast-path
cost. The alternative, converting tagged slots at every yield, adds 6 to 8
percent to a 166-instruction yield round trip. Frozen objects are never
traversed, so a frozen generator forces pinning in full collections too.

The scan omits frames owned by the interpreter (entry frames). It needs a
valid `stackpointer`: the generator spills it before every escaping call, and
automatic collections run only at eval-breaker checks (`gc.c:1986-2024`). The
free-threaded scan does not visit `f_funcobj` (`gc_free_threading.c:459-477`).
After `CALL_PY_EXACT_ARGS` that slot is the only reference to the callee, and
`func_clear` releases `func_globals` (`funcobject.c:1179-1187`). This scan
visits it.

**Cost.** One predicate per slot, only in full collections: about 10 to 50
frames of 10 to 20 slots, a few microseconds. A full collection traverses every
tracked object and takes milliseconds. Young collections do no scan.

**Code objects.** `_PyFrame_Initialize` stores
`PyStackRef_FromPyObjectBorrow(code)`. `func_set_code` already stops the world
(`funcobject.c:674-724`). Before releasing the old code it walks every thread's
frame chain and converts each `f_executable` that borrows that code into an
owned reference. Heap frames already own theirs, because `_PyFrame_Copy` makes
them heap-safe (`pycore_interpframe.h:134-162`). `take_ownership` duplicates
`f_executable` only if the source frame owned it (`Python/frame.c:61-64`).

## 5. Safety

**The existing borrow analysis.** `optimize_load_fast`
(`Python/flowgraph.c:2903-3190`) runs once per code object, after jumps are
normalised and before superinstructions are formed (`:4164-4175`). It
interprets each basic block over a stack of `(instruction, local)` references
and leaves a load owned if any of three flags is set (`:2824-2831`):

- `SUPPORT_KILLED`: the local is overwritten or deleted while the reference is
  on the stack. `STORE_FAST`, both `STORE_FAST_*` superinstructions,
  `DELETE_FAST` and `LOAD_FAST_AND_CLEAR` kill (`:2833-2852`, `:2959-3004`). In
  `f(x, (x := 2))` the first load of `x` stays owned.
- `STORED_AS_LOCAL`: the reference is popped by a store into a local.
- `REF_UNCONSUMED`: the reference is still on the stack at the end of the block
  (`:3155-3162`). References never cross blocks (`:2944-2950`), so the iterable
  of a `for` loop and the operands of conditional expressions stay owned.

It fires for `LOAD_FAST` and `LOAD_FAST_LOAD_FAST` only, never for
`LOAD_FAST_CHECK`. `CALL` consumes its inputs (`:3127-3145`), so borrowed
arguments enter callee locals. A method `LOAD_ATTR` pushes `self` back with its
original provenance (`:3096-3110`). No other code can store into the frame:
writes through `f_locals` keep the old value alive in
`f_overwritten_fast_locals` (`Objects/frameobject.c:237-262`, `:303-311`).
`frame.clear()` refuses executing frames. Frames that move to the heap convert
borrows in `_PyFrame_Copy`. In the measured benchmarks 97 to 99.8 percent of
local loads are already borrowed.

**The rule for 1c.** A tagged reference to a mortal object is one of two
kinds. A borrow is supported by an owned reference in the same or a calling
frame; the flowgraph proves it. A deferred reference is supported by the bias
and the scan. Conversion to an owned reference (`PyStackRef_MakeHeapSafe`)
happens:

| Where | Emitted by | Converts |
|---|---|---|
| `_PyFrame_Copy`, `take_ownership`, `_SEND`, `_SEND_GEN_FRAME`, `_RETURN_VALUE`, `_YIELD_VALUE` | runtime, existing sites | borrows only |
| Stores into the heap: `STORE_ATTR*`, `STORE_GLOBAL`, `STORE_DEREF`, containers, `INTERPRETER_EXIT` | runtime, through `PyStackRef_AsPyObjectSteal` or `Py_NewRef` | both |
| `func_set_code` | runtime | borrowed `f_executable` |
| Unreachable generators and frames in a full collection | GC | both |
| Load sites | compiler | none new in 1c |

A tagged deferred reference must not sit only in a C local across an escaping
call. The cases generator spills live inputs and outputs before such calls
(`Python/generated_cases.c.h:2133-2139` shows `CALL_BOUND_METHOD_EXACT_ARGS`).
`_RETURN_VALUE` is the exception: it holds the result in `temp` across
`_PyEval_FrameClearAndPop`, which runs finalisers
(`generated_cases.c.h:11494-11508`). The fix pushes `temp` onto the caller's
saved stack before the clear, so the scan sees it. The free-threaded build has
the same window. C code that holds stackrefs uses `_PyCStackRef`
(`Objects/call.c:621`, `Objects/object.c:1917`, six other sites). Its `next`
link and the thread's list head are free-threaded only
(`Include/internal/pycore_structs.h:77-82`, `pycore_tstate.h:71-73`); 1c
enables them in both builds.

Borrowing heap loads until the next escape waits for the IR, as a separate
decision.

## 6. Finalisation semantics

Ordinary objects keep exact refcount timing. Deferred objects die at the next
full collection. Classes and builtin functions already die only in the GC:
types reference themselves through `__mro__`, and builtin functions reference
their module, whose dict references them. Modules with C functions form the
same cycle. What changes:

- A top-level or method function whose last reference drops outside a cycle,
  after `del f` or `del C.m`. Its defaults, `__dict__` (including
  `__wrapped__`), annotations and closure cells stay alive until the
  collection. Weakref callbacks and `PyFunction_EVENT_DESTROY` move there too.
- `weakref.ref(f)()` returns the dead function until the collection, because
  `_Py_TryIncref` sees a positive count (`pycore_object.h:727-738`).
- A dead module's `m_free` runs at the collection.
- Short-lived classes and module-level functions created in a loop wait for a
  full collection instead of a young one.

The suite already tolerates this. The free-threaded build defers a larger set
and passes; tests that observe
deaths call `gc_collect()` (for example `Lib/test/test_code.py:1611`,
`:1654`; `Lib/test/test_capi/test_watchers.py:420`). Most functions defined in
tests are nested, hence not deferred. No test relies on a young collection to
free a function or class: the `gc.collect(0)` calls in `test_gc.py:413`,
`:863`, `:916`, `test_gc_stats.py` and `test_weakref.py:1444` concern lists
and instances. Tests that need changes:

- `test_capi.test_misc.CAPITest.test_c_subclass_of_heap_ctype_with_del_modifying_dunder_class_only_decrefs_once`
  (`test_misc.py:769-781`) compares `Py_REFCNT` in C with `sys.getrefcount`.
  Its free-threaded branch must apply whenever heap types are deferred.
- `test_capi.test_object.EnableDeferredRefcountingTest`
  (`test_object.py:176-214`) keeps passing because the API stays a no-op. New
  assertions for `_testinternalcapi.has_deferred_refcount` go in the new test
  file.
- None for `test_gc.GCTests.test_function` (`test_gc.py:221-229`): the
  deferred `f` sits in the cycle `f -> d -> f`, so it is still collected, and
  code objects stay untracked, so the count stays 2.

## 7. Observable surfaces

| Surface | Result for deferred objects |
|---|---|
| `sys.getrefcount`, `Py_REFCNT` | 2^30 + heap references; no temporary for the argument. Free-threaded reports 2^60 + n. Relative comparisons keep working |
| `_PyObject_IsUniquelyReferenced`, `PyUnstable_Object_IsUniqueReferencedTemporary` | always false (`object.c:2862-2886`) |
| `_Py_IsImmortal` | false; 2^30 further references would make the object immortal by count, a leak and not a crash |
| `gc.get_referrers`, `gc.get_referents` | unchanged: frames on thread stacks were never containers |
| `gc.is_tracked` | true; deferred objects must stay tracked |
| weakref callbacks | run during the collection, like cycle members |
| `Py_TRASHCAN` | unchanged; deallocation still goes through `tp_dealloc` |
| `-X showrefcount`, `sys.gettotalrefcount` | unchanged: the bias bypasses `_Py_RefTotal` |
| `_remote_debugging`, gdb | already mask tag bits (`Modules/_remote_debugging/_remote_debugging.h:127`) |
| 32-bit hosts | no deferral |

## 8. Phase 2

The tracing GC deletes reference counts. These parts of 1c die with them: the
flag and bias, the setter calls, the deferred test in `FromPyObjectNew`,
pinning, `undefer_unreachable`, `IsHeapSafe` and `MakeHeapSafe`, and the
`f_executable` borrow. The flowgraph borrow analysis dies too, because note 02
deletes `DUP`, `CLOSE` and `Borrow`.

The scan survives, and so does its discipline. A tracing collector must find
every root precisely, and interpreter frames are the largest root set.
`add_thread_stack_roots` becomes the frame part of root enumeration;
`_PyCStackRef` becomes the C handle list that the plan's handle API needs. The
`RETURN_VALUE` fix, the debug check that every frame on every chain has a valid
stack pointer at each collection, and the gc-stress runs carry over. Root
enumeration bugs found while refcounts still back ordinary objects cost less
than the same bugs under a tracing collector.

## 9. Register IR

The IR's borrow bit reproduces `LOAD_FAST_BORROW`: it binds a local as
`bits | 1` (note 01 section 5). That removes dispatches, not refcount
operations. It contributes nothing to globals, attributes, methods or frames,
so none of 1c comes from the IR. Two interactions remain:

- The scan bound. IR temporaries live in `localsplus[nlocalsplus + d]`, dead
  ones hold NULL, and `stackpointer` keeps only the result-slot role (note 01
  section 4.2). `_PyFrame_ScanEnd` returns `stackpointer` in the stack
  interpreter and `localsplus + co_nlocalsplus + co_stacksize` under
  `Py_IR_INTERP`.
- A fused `RETURN_LOCAL` can move an owned local into the result and clear the
  slot, saving the incref in `_MAKE_HEAP_SAFE` and the decref in frame clear.
  `return <local>` is 3 (deltablue) to 56 (go) percent of returns.
  Instrumented code runs the 1:1 form (note 01 section 10.5), so tracers still
  see the local. This belongs in IR step 5 or 6.

Land 1c before IR step 4. It touches the helpers in `pycore_stackref.h`,
`gc.c`, three object constructors and one instruction body, not the DSL or the
generator. IR step 4's differential runs then cover deferred references, and
`_PyFrame_ScanEnd` is one function to update.

## 10. Measurement

Dynamic counts come from `sys.monitoring` INSTRUCTION and CALL events over one
benchmark run on the baseline; machine instructions per run come from
`perf stat` with two run counts. Pairs removed are deferred `LOAD_GLOBAL`s,
method and module-attribute loads, one `f_executable` pair per Python call,
and two per class call. Each load pair saves 3 instructions and each code pair
6, from the calibration in section 1.

| Benchmark (one run) | Bytecodes | Machine instructions | Deferred `LOAD_GLOBAL` | Method loads | Python calls | Class calls | Pairs removed | Instructions saved |
|---|---|---|---|---|---|---|---|---|
| richards | 10.30 M | 533 M | 132 k | 481 k | 481 k | 0 | 1.09 M | 4.7 M (0.9%) |
| deltablue, n = 100 | 0.76 M | 38.8 M | 38 k | 54 k | 51 k | 3 k | 0.15 M | 0.6 M (1.6%) |
| raytrace, 40 × 40 | 6.66 M | 459 M | 123 k | 308 k | 313 k | 93 k | 0.93 M | 4.0 M (0.9%) |
| go | 29.70 M | 1156 M | 131 k | 996 k | 921 k | 60 k | 2.17 M | 9.4 M (0.8%) |
| chaos, 256², 5000 | 13.12 M | 762 M | 185 k | 190 k | 175 k | 186 k | 0.92 M | 3.9 M (0.5%) |

More than half of richards' `LOAD_GLOBAL`s (238 k) and of go's (504 k) already
load immortals. The remaining refcount traffic is in instance-attribute loads:
1.0 M in richards, 4.3 M in go. Deferral cannot reach it. The calibration rows
show no cycle difference between owned and tagged pairs, so the expected time
change is within ±1 percent on all five benchmarks. The step 3 gate is no
regression above 1 percent.

Statistics for `--enable-pystats`: `object_stats.stackref_new_owned`,
`_immortal` and `_deferred` counted in `_PyStackRef_FromPyObjectNew`;
`stackref_close_owned` and `_tagged` in `CLOSE`; `gc_stats.stack_roots` and
`gc_stats.pinned` per collection. With the existing per-opcode counts of
`LOAD_FAST_BORROW` against `LOAD_FAST`, these give borrowed, deferred and owned
loads per opcode.

## 11. Implementation sequence

| # | Step | Tests | Benchmarks |
|---|---|---|---|
| 1 | Roots without deferred objects: `_PyCStackRef` linking in both builds; `add_thread_stack_roots` with its debug stack-pointer assertion; `_RETURN_VALUE` pushes its result before clearing; `f_funcobj` added to the free-threaded scan | full suite on default, debug and free-threaded builds; a debug knob that makes every automatic collection full | neutral |
| 2 | Flag, bias, exact and pinned accounting, `undefer_unreachable`, `IsHeapSafe`, `FromPyObjectNew`, `_PyCell_GetStackRef`; deferral enabled only through `_testinternalcapi.set_deferred_refcount(obj)` | new `Lib/test/test_gc_deferred.py` (below); `test_gc`; `test_capi.test_object`; `-X showrefcount -c pass` reports 0 refs and 0 blocks | neutral |
| 3 | Enable the deferred set of section 2 | full suite; the `test_misc` fix; `test_sys` cases for `getrefcount`; finalisation-timing cases | gate: no regression above 1% on richards, deltablue, raytrace, go, chaos |
| 4 | Borrowed `f_executable`, `func_set_code` fix-up, `take_ownership` fix | `__code__` reassignment during recursion, in generators and from another thread; `test_code`, `test_funcattrs`, `test_capi.test_watchers` | neutral to slightly up |
| 5 | Statistics, the `sys.getrefcount` documentation, What's New | pystats smoke run | none |

`Lib/test/test_gc_deferred.py`:

| Scenario | Expected |
|---|---|
| The running function deletes its own global, then calls `gc.collect()` | it finishes; freed after return and a full collection |
| A callable's global is deleted while its arguments are evaluated | the call completes |
| `C.m` reassigned during `self.m(...)`; `del math.sqrt` during a call | the old object survives the call |
| A finaliser run by `RETURN_VALUE`'s frame clear deletes the global bound to the result and collects | the result survives |
| A suspended generator holds a deferred local; generator promoted to the old generation; global deleted; `gc.collect(0)`, `gc.collect()`, resume | the local survives both collections |
| Same, after `gc.freeze()` | the local survives (pinned) |
| A function in an `exec` namespace dict | freed by a full collection |
| A nested function | freed at once |
| Each kind in the deferred set | `has_deferred_refcount` is true |

GC test strategy: run the full suite with `python -m test -t 1` and the
full-collection knob, because young collections pin deferred objects and would
not exercise the scan. Run it once more under `PYTHONMALLOC=debug`, which fills
freed memory, and once in a `Py_STACKREF_DEBUG` build, whose table rejects
stackrefs to freed objects. Run refleak hunts (`-R 3:3`) on `test_gc`,
`test_capi`, `test_sys`, `test_descr`, `test_funcattrs`, `test_weakref`,
`test_generators`, `test_coroutines` and `test_asyncio`.

## 12. Open questions for the coordinator

1. **Purpose.** 1c removes under 1 percent of instructions and no measurable
   time. Keep it in Phase 1 as root-enumeration groundwork with a
   no-regression gate, or move it to the start of Phase 2?
2. **Pinning.** Deferred objects are reclaimed only by full collections, and
   never while `gc.freeze()` holds objects. Is that acceptable, or should
   suspension pay 6 to 8 percent per yield to make young collections exact?
3. **`sys.getrefcount`.** Report the raw count with the bias, as the
   free-threaded build does (recommended), or subtract the bias?
4. **Free-threaded fixes.** Step 1 closes two holes the free-threaded build
   shares: `f_funcobj` is not scanned, and the `RETURN_VALUE` result sits in a
   C local during frame teardown. Fix them in both builds, as proposed, or
   leave the free-threaded build alone under the no-performance-work rule?
5. **Extension opt-in.** Should `PyUnstable_Object_EnableDeferredRefcount`
   work in the default build, or stay a no-op so the deferred set stays closed?

## Coordinator decisions (2026-10-23)

1. **Scheduling.** Item 1c moves from Phase 1 to the start of Phase 2. In
   Phase 1 it buys about one percent of instructions and no measurable time,
   while it adds a GC accounting mode that Phase 2 deletes again. Its lasting
   value is the stack-root enumeration, which is the first thing the tracing
   collector needs, so it lands as plan step 2e's opening move. Phase 1 is
   now: IR and feedback (1z), tagged values (1a, 1b), interpreter mechanics
   (1d), tier 0 (1f), shapes (1e).
2. **Pinning.** Accepted. Young and middle collections keep deferred objects
   alive; only full collections are exact. Paying 6 to 8 percent per yield
   for exact young collections is not worth it for code we will replace.
3. **`sys.getrefcount`.** Reports the raw count including the bias, matching
   the free-threaded build.
4. **The two free-threaded holes** (`f_funcobj` not scanned, the
   `RETURN_VALUE` result held in a C local while finalisers run) are fixed in
   both builds in step 1 when 1c lands. They are recorded here so the IR
   interpreter (note 01 step 4) is written with the correct ordering from
   the start: hand the return value to the caller before clearing the frame.
5. **`PyUnstable_Object_EnableDeferredRefcount`.** Stays a no-op in the
   default build until 1c lands.
