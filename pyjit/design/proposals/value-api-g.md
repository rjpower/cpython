# Value API step G: the R27 gate owners (proposal)

Status: proposal, revision 2, 2026-11-04, after
[review 1](../reviews/value-api-g-review-1.md) ("accept with changes") and the
coordinator's rulings. Step G of [value-api-v2a0.md](value-api-v2a0.md) (§1.4
row G, §3). It turns §3.2 into a design: the box argument, owners per kind,
the owner table and its process accounting, the stats build,
`Tools/build/check_r27.py`, the lint rules, the reports and the three-part
check at W5. The kind and the site become arguments of the box primitive in
stats builds, and V2a-0's thread-local swaps go.

Base: V2a-0 part 2 (`T_2f` = `61bbf1593f7`, `build/worktrees/v2a0`) on the
shared tree with A5b-3, which makes `PyValue_AsObject` inline in
`pycore_value.h`. Counts are from the V2a-0 tree; review 1 recounted them on
the staged tree (`_PyBorrow_TupleItems` 221 in 78 files, `_PyValue_AsObjectAt`
84 in 25), and every claim holds. `B` is `build/pyjit-link-wave1`.

## Changes after review 1

| # | Finding | Change | § |
|---|---|---|---|
| 1 | must-fix: "external" was decided by object path | Classes come from the DWARF compile units of the object, found by GNU build-id. An object is in-tree if any of its CUs lies under `$SRC`, wherever it was loaded from. An address in no object is unresolved, which is debt. A dump keeps a copy of every unknown object that it references. `test_cext` and `test_cppext` are compat-test. configure refuses `--enable-shared` with `--enable-pystats`. Offsets come from `dl_iterate_phdr`'s load bias (finding 12) | 1.4, 1.8 |
| 2 | must-fix: lost processes and interpreters | Interpreter deletion merges counters and table into a runtime accumulator under a runtime mutex. `PyOS_AfterFork_Child` clears both. `os._exit` and `os.exec*` dump first. Each counting process leaves a `.run` marker that its dump replaces, and leftover markers fail the gate unless the crash list names them | 1.4, 3.1 |
| 3 | must-fix: a boundary owner could be stated anywhere | A `boundary-site` rule allows `_Py_BOX_OWNER_FOREIGN` in `Modules/_ctypes/` and one `_testinternalcapi` helper only. It allows the `For` forms and `_PyValue_AdapterBox` in 10 named files only. §6 states the claim exactly | 1.7, 6 |
| 4 | should-fix: G10 broke `test_embed` | **Ruling: Q2 rejected.** `-E` and `-I` keep ignoring `PYTHONSTATS`. Isolated children count through `-X pystats` and `-X pystats_dir`, which `subprocess` already propagates and which `script_helper` now passes. An audit hook lists the children that still do not count, and the gate checks that list. `test_embed` keeps `PYTHON_PYSTATS_DIR` | 1.5 |
| 5 | should-fix: stats resets mid-run | Every dump prints `Stats resets` and the gate lists them. `test_sys`'s and `test_pyvalue_box_sites`'s in-process measurements move into subprocesses that read the dump directory | 1.4, 4 |
| 6 | should-fix: the test class failed V2a-0's adapter tests | The TEST rule applies to CORE boxes sited in `_testinternalcapi*`. Boundary boxes owned there count as class "test" and pass | 1.7 |
| 7 | should-fix: two marks for one set | **Ruling: one mark, two strengths.** `Py_VALUE_ENTRY` goes on a declaration. `Py_VALUE_COMPAT(R)` is `Py_VALUE_ENTRY` plus `unavailable` in value-native units. A wave replaces the token on the same line | 1.3 |
| 8 | should-fix: the `.text` test was narrow and clang's `annotate` was not free | `Py_VALUE_ENTRY` is empty in every non-stats build. Every edited object must have identical release `.text` on gcc and clang, except `getargs.o` | 1.1 |
| 9 | should-fix: `memset` erased the table pointer | The table lives in `PyInterpreterState` beside `pystats_struct`, and `_Py_StatsClear` clears it explicitly. configure refuses `--disable-gil` with `--enable-pystats` | 1.4, 1.6 |
| 10 | Q8: the NULL sentinel | Dropped. The two borrow entries build their own `at`. NULL always means "no owner", which is unresolved | 1.2 |
| 11 | Q6: the box in `object.__hash__(1000)` | Removed in G-1 by `_PyValue_Identity`. No exemption is needed | 1.2 |
| 12–16 | notes | `dl_iterate_phdr` bias (12). `_PyCall_BoxOwner` runs no Python and counts nothing (14). The compat rules run as a second pass with `-DPy_STATS` (15). compat-capture reports one entry calling another (16) | 1.2–1.4 |
| 17 | note outside G: GCC 15.2 at `-Og` drops a value live across a `no_callee_saved_registers` call | For the coordinator (A3) | – |
| Q1, Q3 | | Q1 accepted. Q3: one configuration, which refuses `--enable-shared`, `--disable-gil` and `--with-lto` | 1.6 |
| Est. | 8.0 h | 10.0 agent-h: the review's 9.5 plus 0.5 for Q2's replacement and object preservation; 5.5 h wall | 8 |

## 0. Decisions

| # | Decision |
|---|---|
| G1 | **Owner = the code a box serves.** CORE: none; the box site's own address is recorded. THUNK: the call instruction in the caller of the compatibility entry. SLOT_ADAPTER: the object-slot function. CALL_ADAPTER: the C entry of the object-form callee. Foreign: `_Py_BOX_OWNER_FOREIGN` |
| G2 | **One argument, stats builds only.** `_PyBoxAt {owner, site, kind}` is a parameter of `_PyValue_BoxInlineValue` and of the helpers that box under `Py_STATS`. Elsewhere it is a macro argument that expands to nothing, so non-stats builds compile today's code |
| G3 | **The primitive records two addresses:** `at.owner` and its own call site, into a fixed-size per-interpreter owner table. Overflow is counted per kind |
| G4 | **Addresses point inside the owning code.** Return addresses are stored minus one; function addresses are entries. The resolver never adjusts |
| G5 | **One mark, two strengths.** `Py_VALUE_ENTRY` declares a compatibility entry: under `Py_STATS` it is `noinline, noipa` (GCC) or `noinline, annotate` (clang), and elsewhere it is empty. `Py_VALUE_COMPAT(R)` is the same plus `unavailable` in value-native units |
| G6 | **Capture only in entries.** `_Py_BOX_CALLER()` and the THUNK forms are legal only in the bodies of marked functions (`compat-capture`). compat-body checks those bodies |
| G7 | **The stats build is the gate build.** `--enable-pystats` adds `-fno-optimize-sibling-calls` to `CFLAGS`, selects G5's attributes, and refuses `--with-lto`, `--enable-shared` and `--disable-gil` |
| G8 | **Classes come from DWARF compile units.** An object is found by build-id, and its unit by `.debug_aranges`. A unit is classed by `check_value_api.py`'s `unit_kind` and the compat-test list. An address in no object is unresolved |
| G9 | **Owner classes:** core, core-pending, test, compat-test, external, foreign, unresolved. Core, core-pending and unresolved are debt |
| G10 | **Every counting process is accounted for:** a `.run` marker at stats start, a dump at exit, `os._exit` and `exec`, a runtime accumulator for subinterpreters, and a cleared table after fork. Isolated children count through `-X pystats`; an audit hook lists the children that do not |
| G11 | **`BOX_SITE_TEST`** (class 19) for `_testinternalcapi`'s deliberate boxes; `test-site` and `boundary-site` text rules |
| G12 | **`box_cached`** counts boxes that returned a cached small int (Q5). They stay boxes |
| G13 | **G moves no box except Q6's.** Its counter changes are kind moves (§5); the kernels' per-class totals are equal before and after G |
| G14 | **The call adapter names its callee** through `_PyCall_BoxOwner(callable)`. Sites that do not know the callee name themselves, which resolves as core |

## 1. Interfaces

### 1.1 The box argument (G2)

```c
/* Include/internal/pycore_value.h */
#ifdef Py_STATS
typedef struct _PyBoxAt {
    const void *owner;   // G1; NULL: no owner (CORE), else unresolved
    uint8_t site;        // BOX_SITE_*
    uint8_t kind;        // BOX_KIND_*
} _PyBoxAt;
#  define _PyBox_AT(SITE, KIND, OWNER) ((_PyBoxAt){(OWNER), (SITE), (KIND)})
#  define _PyBox_AT_PARAM , _PyBoxAt at
#  define _PyBox_AT_ARG(AT) , AT
#  define _Py_BOX_CALLER() ((const void *)((const char *)__builtin_return_address(0) - 1))
#  define _Py_FUNC_ADDR(F) ((const void *)(uintptr_t)(F))
#else
#  define _PyBox_AT(SITE, KIND, OWNER)
#  define _PyBox_AT_PARAM
#  define _PyBox_AT_ARG(AT)
#endif
#define _Py_BOX_OWNER_FOREIGN ((const void *)(uintptr_t)1)

PyAPI_FUNC(PyValue) _PyValue_BoxInlineValue(PyValueBorrow ref _PyBox_AT_PARAM);

static inline PyObject *
PyValue_AsObject(PyValueBorrow ref _PyBox_AT_PARAM)  /* body as now; passes at */
#define PyValue_AsObject(V) (PyValue_AsObject)(PyValue_Borrow(V) \
        _PyBox_AT_ARG(_PyBox_AT(BOX_SITE_UNATTRIBUTED, BOX_KIND_CORE, NULL)))
#define _PyValue_AsObjectAt(V, SITE) (PyValue_AsObject)(PyValue_Borrow(V) \
        _PyBox_AT_ARG(_PyBox_AT(SITE, BOX_KIND_CORE, NULL)))

/* Boundary boxes: a function of its own name, so that the lint can tell it
   from the CORE forms (§1.3); it inlines to the same code. */
static inline PyObject *
_PyValue_AsObjectFor(PyValueBorrow ref _PyBox_AT_PARAM)
{
    return (PyValue_AsObject)(ref _PyBox_AT_ARG(at));
}
#define _PyValue_AsObjectFor(V, AT) (_PyValue_AsObjectFor)(PyValue_Borrow(V) _PyBox_AT_ARG(AT))
```

A macro argument that expands to nothing is never evaluated, so owner
expressions cost nothing outside stats builds. Every boxing helper follows
the same pattern:

| Form | Kind, site, owner | Calls | Change |
|---|---|---:|---|
| `PyValue_AsObject(v)`, `PyValue_Box(v)` | CORE, unattributed | 55, generated handlers included | none; the macro supplies `at` |
| `_PyValue_AsObjectAt(v, site)` | CORE, site | 75 in 22 files | none |
| `_PyValueTemps_Object(t, v)`, `…ObjectAt(t, v, site)` | CORE | 53 + 7 | none |
| `_PyValue_AsObjectFor(v, at)` (new) | boundary, stated | 9 at G (§1.2, §1.5) | replaces `_PyValue_AsObjectKind` and `AtKind` (8 calls) |
| `_PyBorrow_TupleItems(t, n)` | THUNK, site call adapter, its caller | 217 in 77 files (152 in `Modules/`, mostly Clinic) | none at the callers; becomes an entry |
| `_PyBorrow_TupleItemsFor(t, n, at)` | stated | 4 in `call.c`, 5 in `getargs.c` | replaces `_PyBorrow_TupleItemsAt` (6) |
| `_PyBorrow_ValueVector(v, n)`, `…For(v, n, at)` | CALL_ADAPTER, its caller / stated | 5 | `begin_vector` states its callee |
| `_PyValue_AdapterBox(v, site, slotfn)` | SLOT_ADAPTER, the slot function | 8 in 3 files | gains the owner argument |
| `_PyValue_THUNK_EXIT(result, site)`, `_PyValue_THUNK_NEW(v, site)` (new) | THUNK, `_Py_BOX_CALLER()` | 29 exits; 10 container getters | the exit gains its family site |

The two borrow entries do not use a sentinel (finding 10). Their own body
builds `_PyBox_AT(BOX_SITE_CALL_ADAPTER, kind, _Py_BOX_CALLER())`, then calls
a static helper that it shares with the `For` forms.

Deleted: the three swap functions and their thread-locals,
`_PyValue_AsObjectKind`, `…AtKind`, `_PyBorrow_TupleItemsAt`, and the rule
"site class call adapter means kind CALL_ADAPTER". `entry_unbox[site]`
becomes one total, since entry never ran inside a swap window. `_PyValue_RichCompareObject` boxes as CORE: its callers are the
interpreter and `tuple_richcompare` (`generated_ir_cases.c.h:8292`, `:8548`,
`tupleobject.c:853`). The site is an argument too because the swaps existed
only to carry it through helpers, and the helpers now take `at`.

**Cost in non-stats builds: none (finding 8).** Every signature, inline body
and call outside `#ifdef Py_STATS` is the one compiled today, and
`Py_VALUE_ENTRY` is empty there for both compilers. Release code changes in
one unit: `getargs.c`'s four keyword reads (§1.5) call the value getter and
box directly instead of through `PyDict_GetItemRef`'s thunk. The test:
`objdump -d --no-addresses` of every object G edits (about 20), in a gcc
release build and in a clang release build, is identical before and after G
except for `getargs.o`.

### 1.2 Owners by kind (G1)

| Kind | Owner | Captured in | Sites |
|---|---|---|---|
| CORE | none; the primitive records its call site | `_PyValue_BoxInlineValue` | every `At` form, `PyValue_Box`, temps |
| THUNK | caller's call instruction | `_Py_BOX_CALLER()` in a marked body. `_PyValue_THUNK_EXIT` and `_PyValue_THUNK_NEW` are `Py_ALWAYS_INLINE` in stats builds, so `__builtin_return_address(0)` is the entry's | 43 dispatcher thunks, 13 value wrappers, 8 thunk macro families, 9 container getters, 2 borrow entries |
| THUNK (parser) | caller of the `PyArg_*` entry, passed down | 19 entries pass `_Py_BOX_CALLER()`; `freelist_t` gains a stats-only `owner` field, and `convertitem` and `converttuple` already receive it | `getargs.c:388`, `:600`, `:1781`, `:2438`, `:3008`; keyword reads `:1867`, `:1990`, `:2471`, `:2714` |
| SLOT_ADAPTER | the object-slot function | `adapt_*`, `call_richcompare`, `call_repr`, `call_bool` pass `_Py_FUNC_ADDR(f)` | 8 adapter calls; NOTIMPL ternary modulus (`objfn`) |
| CALL_ADAPTER | the callee's C entry, `_PyCall_BoxOwner(callable)` | `begin_vector`, `_PyNativeScope_Objects` (2), `_PyVectorcall_Call`, `_PyObject_Call_Prepend`, `call.c:555`, `:671` | 7 sites. `_PyStack_UnpackDict` (`call.c:1124`) names itself. `classmethoddescr_call` and `ceval.c`'s `_PyValueArray_ToObjectVector` reach the borrow entries, whose owner is their caller. All three resolve as core: I5's debt |
| foreign | `_Py_BOX_OWNER_FOREIGN` | ctypes, at W4 (§1.7) | 1 test helper at G |

`_PyCall_BoxOwner` (stats only, `call.c`; it runs no Python and counts
nothing, but runs on every adapter call, finding 14) returns `ml_meth` for a
`PyCFunction` or method descriptor, `d_wrapped` for a wrapper, the owner of
`im_func` for a bound method, a type's `tp_vectorcall` (else a non-`object`
`tp_init`, else `tp_new`), and otherwise the vectorcall function or
`tp_call`. A Python callee's owner is `_PyFunction_Vectorcall`: core.

**Q6, removed in G-1.** `_PyValue_GenericHash` boxed an inline operand to
hash it. A new `_PyValue_Identity(PyValueBorrow)` in `pyvalue.c` mirrors
`_PyObject_ValueIdentity` (R26 holds). The hash becomes
`Py_HashPointer((void *)_PyValue_Identity(v))`, which equals
`PyObject_GenericHash` of the box. The test compares the two for 1000,
-2**50, 2.5 and -0.0.

Only level 0 of `__builtin_return_address` is used; higher levels need frame
pointers. V8 and LuaJIT attribute deoptimisations and trace aborts to a
source position the VM knows; G's owners include C code with no run-time
position, so G records code addresses and resolves them offline, as `perf`
does.

### 1.3 Entries: one mark, two strengths (G5, G6)

```c
/* Include/pyport.h */
#if defined(Py_STATS) && defined(__clang__)
#  define Py_VALUE_ENTRY __attribute__((noinline, annotate("Py_VALUE_ENTRY")))
#elif defined(Py_STATS) && defined(__GNUC__)
#  define Py_VALUE_ENTRY __attribute__((noinline, noipa))
#else
#  define Py_VALUE_ENTRY
#endif
/* Py_VALUE_COMPAT(R): Py_VALUE_ENTRY, plus unavailable("…use R") in
   value-native units (Py_VALUE_CORE), as today. */
```

The mark goes on the declaration. GCC applies `noinline` and `noipa` to the
definition, and clang's `annotate` carries to redeclarations. `noipa` also
stops IPA-split clones and ICF from moving an exit. One function carries one
mark, at one of two strengths:
- **`Py_VALUE_ENTRY`**: the function is a compatibility entry, and its boxes
  belong to its caller. Core callers still compile.
- **`Py_VALUE_COMPAT(R)`**: core callers must also use R.

G marks the existing entries `Py_VALUE_ENTRY`. A wave's marking replaces the
token on the same line, for example `Py_VALUE_ENTRY PyAPI_FUNC(int)
PyTuple_GetItemRef(…)` becomes `Py_VALUE_COMPAT(PyTuple_GetValue)
PyAPI_FUNC(int) …`. A declaration still marked `Py_VALUE_ENTRY` at W5 is a
static-check failure if the ownership table has a row for it. Functions
without a header declaration keep `Py_VALUE_ENTRY` on their definition
permanently: the thunk macro families and the value wrappers, whose units
take their addresses.

| Entries G marks (script from this list) | Count | Where the mark goes |
|---|---:|---|
| Dispatcher thunks (number, compare, truth, hash, repr) | 43 | `Include/abstract.h`, `object.h`, `pycore_value_ops.h` |
| Value wrappers (`wrap_*value`, `wrap_concatvalue`) | 13 | definitions, `typeobject.c` |
| Thunk macro families | 8 | `pycore_value_slots.h` |
| Container getters: `PyCell_Get`, `PyList_GetItemRef`, `PyTuple_GetItemRef`, `PyStructSequence_GetItemRef`, the `PyDict_GetItemRef` family (through `dict_getitemref_owned`), `PyDict_SetDefaultRef`, `_PyDict_NextEntryRefKnownHash` (2 sites), `_PySet_NextEntryRef` | 9 (10 sites) | their headers |
| `PyArg_*` entries | 19 | `modsupport.h`, `cpython/modsupport.h`, `pycore_modsupport.h` |
| `_PyBorrow_TupleItems`, `_PyBorrow_ValueVector` | 2 | `pycore_borrow.h` |

The two dict helpers are static and shared by several public entries. Each
entry passes its `_Py_BOX_CALLER()` down, as `getargs.c` does. The 10 getter
sites move from `_PyValue_AsObjectAt` (CORE) to `_PyValue_THUNK_NEW`. The
other container sites (`list_item`, `tuple_item`, `list_pop_impl`,
`set_pop_impl` and the iterators) are core paths, and stay CORE until M1b,
M3 or W2.

The rules in `check_value_api.py`. Because `Py_VALUE_ENTRY` and
`_Py_BOX_CALLER()` exist only under `Py_STATS`, compat-body and
compat-capture run as a second clang-query pass with `-DPy_STATS`, over the
units that define a marked function, about 20 (finding 15). The first pass
and its rules are unchanged.
- **compat-body.** In a definition with `attr::Annotate`, it flags the CORE
  forms (the callees `PyValue_AsObject`, `PyValue_Box`,
  `_PyValueTemps_Object` and `_PyValue_BoxInlineValue`) and
  `_PyValue_AdapterBox`.
- **compat-capture.** It flags `__builtin_return_address`, and calls of
  `_PyValue_THUNK_EXIT` and `_PyValue_THUNK_NEW`, outside an annotated
  definition and outside the THUNK forms' own definitions. It also flags a
  call from one annotated definition to another (finding 16), since the
  outer entry would become the owner.

A falsely marked entry hides no debt: its owner is its real caller.

### 1.4 Owner table, process accounting and dump (G3, G10)

```c
/* Python/pystats.c, Py_STATS */
typedef struct {
    const void *owner, *pc;   // G4: inside the owning code
    uint64_t count;
    uint8_t kind, site;
} _PyBoxOwnerEntry;           // 32 bytes
#define BOX_OWNERS_BITS 16    // 65,536 entries, 2 MiB, PyMem_RawCalloc at the first box
#define BOX_OWNERS_PROBE 64   // then dropped
/* PyInterpreterState, beside pystats_struct (finding 9): */
    struct _PyBoxOwners *box_owners;      // entries, dropped[BOX_KINDS], resets
/* _PyRuntimeState: */
    PyMutex pystats_accum_mutex;
    PyStats *pystats_accum;               // counters of deleted interpreters
    struct _PyBoxOwners *box_owners_accum;
/* ObjectStats: */
    uint64_t box_cached;                  // G12
```

**The table.** Open addressing with linear probing on the four key fields.
The interpreter's GIL serialises it, as it serialises every counter in
`PyStats`, so it needs no lock. Free-threaded stats builds are refused,
because their per-thread `PyStats` would race the table. A suite process has
at most about 10,000 keys (2,000 thunk call sites, 3,000 callees, 500 slot
functions, 500 core sites), a 15% load. The checkpoint's 65,536 entries
dropped nothing on the kernels. After 64 probes the box still counts in
`box[]` and `box_kind[]`, and `dropped[kind]` grows. A gate run fails on any
drop. `_Py_StatsClear` clears the table's entries explicitly, because
`stats_zero_all`'s `memset` covers only `PyStats`.

**Process accounting (finding 2).**

| Event | Under `Py_STATS` |
|---|---|
| Stats start (`_pystats` at startup, or `sys._stats_on()`) | Creates `<rand>.run` in the dump directory, once per process. It holds the pid, the ppid, the command line and `PYTHON_PYSTATS_TAG` |
| Interpreter deletion (`pystate.c:480`) | Merges its counters and table into the runtime accumulator under `pystats_accum_mutex`. Subinterpreters can have their own GIL, so the interpreter lock is not enough |
| `PyOS_AfterFork_Child` | Clears the counters, the table and the accumulator, then creates the child's own `.run` |
| `os._exit`, `os.exec*` | Dumps first. These are the multiprocessing fork and forkserver workers and `concurrent.futures` pools |
| `sys._stats_clear()`, `sys._stats_off()` | `resets` grows. `_stats_dump()`'s clear loses nothing, since it dumps first |
| Finalization | Dumps the main interpreter's stats plus the accumulator, writes `<rand>.tmp`, renames it over `<rand>.txt`, and removes the `.run` |

A worker that crashes, times out or overflows the C stack leaves its `.run`
(§3.1).

**The dump**, in `print_stats`. Objects come from `dl_iterate_phdr`: a
stored address minus the object's load bias is its ELF virtual address,
which is right for PIE and non-PIE objects alike (finding 12). The dump
names each object referenced by an entry once, with its path and GNU
build-id. Suppose an object's build-id is not that of a build product
(`python`, `Programs/_testembed`, `build/lib.*/*.so`, recorded by
`check_r27.py` from the build directory). Then the dump copies the file to
`<dir>/objects/<build-id>` if no copy exists yet. Extensions that a test
builds in a temporary directory therefore stay resolvable after the test
deletes them.

```
Box object [0]: /…/v2a0-stats/python build-id=3f1a…
Box object [1]: /tmp/…/venv/bin/python build-id=3f1a…     (a --copies venv: same build-id)
Box object [2]: /tmp/…/test_cext/_test_c11_cext.so build-id=91c0…
Box owner (thunk, parser) 2+0x1b2c3 at 0+0x4d5e6: 52010
Box owner (core, list) - at 1+0x12345: 812
Box owner (call adapter, call adapter) foreign at 0+0x2222: 3
Box owner (thunk, tuple) ? at 0+0x777: 4        (owner in no object: unresolved)
Box owners dropped (thunk): 0
Stats resets: 0
Object box cached: 4113
```

Keys are unique and keep `run.py`'s `key: value` form. G keeps from
`$B/checkpoint-box-callers.diff` the table shape, symbolization at dump time
and the drop count. It replaces the patch's global table, side file and
second return address.

### 1.5 `getargs.c`, and isolated children

**`getargs.c`.** The five tuple sites become `_PyBorrow_TupleItemsFor(t, n,
_PyBox_AT(BOX_SITE_PARSER, BOX_KIND_THUNK, fl->owner))`. The four keyword
reads become `PyDict_GetValue`/`PyDict_GetValueString` (`:1990`:
`_PyDict_NextValue`), then a `_PyValue_AsObjectFor` box of the same kind and
owner, then `_PyBorrow_Keep`. Six static functions take a stats-only owner
parameter: `vgetargs1`, `vgetargs1_impl`, `vgetargskeywords`, its `_impl`,
`vgetargskeywordsfast` and its `_impl`. `_PyArg_UnpackKeywords` and
`PyArg_UnpackTuple` capture the owner directly.

**Isolated children (ruling on Q2).** `-E` and `-I` keep ignoring
`PYTHONSTATS`, so `test_embed`'s five isolated-configuration tests stand. A
child counts in one of three ways:

| Route | How | Covers |
|---|---|---|
| Environment | `suite` sets `PYTHONSTATS=1`, `PYTHON_PYSTATS_DIR`, `PYTHON_PYSTATS_TAG`; the regrtest worker sets the tag to the test name (stats builds) | children that inherit the environment |
| `-X` options, kept under `-E`/`-I` | new `-X pystats_dir=DIR` (config `_pystats_dir`) beside `-X pystats`; `suite` starts regrtest with both; `subprocess._args_from_interpreter_flags` already copies every `-X` option; `script_helper` adds the two in stats builds | regrtest workers, multiprocessing spawn and forkserver, `script_helper`'s `-I` children |
| None: listed | regrtest workers install an audit hook (`subprocess.Popen`, `os.posix_spawn`, `os.exec*`) that writes each launch of a build product that will not count to `<dir>/<tag>.spawn` | `_testembed`, `env={}`, raw `-I` launches; `--gate` fails on a test not in `Tools/build/r27_uncounted.txt`, seeded from the first gate run with a reason per entry |

`.run` markers cannot replace the audit: a child that never starts stats
leaves none. `test_embed`'s `remove_python_envvars()` keeps
`PYTHON_PYSTATS_DIR`, so `_testembed`'s dumps stay out of `/tmp/py_stats`.

### 1.6 Stats build (G7)

In the `--enable-pystats` branch of `configure.ac`: append
`-fno-optimize-sibling-calls` to `CFLAGS` (not `CFLAGS_NODIST`, so extensions
that tests build inherit it through `sysconfig`; a sibling call would hand an
entry's return address to its caller's caller; `musttail` still compiles and
runs, finding 13); `AC_MSG_ERROR` on `Py_LTO` (cross-unit inlining), on
`--enable-shared` (the self-test does not cover libpython yet) and on
`--disable-gil` (a non-goal; the table is per interpreter). `-g` is already in
`OPT`; `check_r27.py` refuses a build product without CU ranges. The existing
recipe (`--with-mimalloc --with-computed-gotos --with-shapes
--enable-pystats`, gcc) is unchanged, so every stats build can run the gate;
stats builds are never timed. `make check-r27` runs `static` and `suite` and
refuses a build without `Py_STATS`; the kernels run from the documentation
checkout (§3), since `pyjit/bench` is not in the runtime tree.

### 1.7 Test sites, cached ints, foreign exits, boundary sites (G11, G12)

- **`BOX_SITE_TEST`** is class 19, named "test"; `BOX_SITE_KINDS` becomes
  20. `_testinternalcapi`'s 23 sites take it: 8 in `_testinternalcapi.c`,
  6 in `dict.c`, 3 each in `list.c` and `native_scope.c`, 2 in
  `borrow_scope.c` and 1 in `typecache.c`. They use
  `_PyValue_AsObjectAt(v, BOX_SITE_TEST)`, or the borrow `For` forms with
  CORE kind. The sites in V2a-0's `value_slots.c` are recounted at
  implementation. Under finding 6 there are two classes:
  - A CORE box whose site lies in `_testinternalcapi*` must be of class
    TEST, and a TEST box must have its site there.
  - A boundary box owned in `_testinternalcapi*` (its object-slot test types
    driven through the adapter) is classed "test" and passes.
- **`box_cached`.** An int in `[-_PY_NSMALLNEGINTS, _PY_NSMALLPOSINTS)`
  counts once more under `box_cached`. Q5 stands: it is a box.
- **Foreign exits.** ctypes receives objects until W4 ports it. Its exits
  then box with `_PyValue_AsObjectFor(v, _PyBox_AT(BOX_SITE_CALL_ADAPTER,
  BOX_KIND_CALL_ADAPTER, _Py_BOX_OWNER_FOREIGN))` at three places:
  - `O_set` (`cfield.c:1035`), as in `py_object(1000)`;
  - `ConvParam`'s `py_object` argument in `callproc.c`, for `pythonapi`
    calls;
  - a callback's `py_object` result (`callbacks.c:303`).

  W4's `_ctypes` batch writes them. G tests the form through
  `_testinternalcapi/value_boxes.c`.
- **Text rules** (`check_value_api.py`, scanning `Include/`, `Objects/`,
  `Python/`, `Parser/` and `Modules/`):

| Rule | Token | Allowed only in |
|---|---|---|
| `test-site` | `BOX_SITE_TEST` | `pystats.h`, `pystats.c`, `Modules/_testinternalcapi*` |
| `boundary-site` | `_Py_BOX_OWNER_FOREIGN` | `pycore_value.h` (definition), `Modules/_ctypes/`, `Modules/_testinternalcapi/value_boxes.c` |
| `boundary-site` | `_PyValue_AsObjectFor`, `_PyBorrow_TupleItemsFor`, `_PyBorrow_ValueVectorFor`, `_PyValue_AdapterBox` | `pycore_value.h`, `pycore_borrow.h`, `pycore_value_slots.h`, `Python/borrow.c`, `Python/getargs.c`, `Python/native_scope.c`, `Objects/call.c`, `Objects/abstract.c`, `Objects/object.c`, `Modules/_ctypes/`, `Modules/_testinternalcapi/value_boxes.c` |

Adding a file to a list takes a line in this design. Each rule has a
self-test that fires and one that does not.

### 1.8 `check_r27.py`, `summarize_stats.py`, `run.py` (G8, G9)

`Tools/build/check_r27.py` (new, standard library only):

| Subcommand | Does |
|---|---|
| `static --cdb DB --ownership TABLE` | Part 1 of the check (§3) |
| `collect DIR -o S.json` | Folds the dumps (object indices mapped per file), lists leftover `.run`, `.spawn` and resets, then resolves |
| `suite --build DIR [-j N] -o S.json [-- regrtest args]` | Makes `DIR/r27-dumps` and runs regrtest with the environment and `-X` options of §1.5. Every 30 s it folds and deletes finished dumps, then it collects |
| `kernels R.json` | Reads `run.py`'s raw counters, resolves, and checks part 2 |
| `report [--kernels R.json] [--suite S.json]` | The per-step tables of §3.2, as Markdown |
| `--gate` | Exits 1 on any violation; without it, the subcommand only reports |
| `--self-test` | Resolves a real dump: a stats process boxes through `_testcapi.number_add`. The resolver must name `Objects/abstract.c` for a runtime `PyNumber_Add` address. It also runs synthetic dumps for each class |

**Resolution**, once per object, by build-id. The object is read from the
build directory, else from `objects/<build-id>`, else from the dumped path.
`readelf --debug-dump=aranges` gives the CU address ranges, and
`readelf --debug-dump=info --dwarf-depth=1` gives the CU names and
directories. On `v2a0-stats/python` (52 MB) this took 0.5 s for 212 CUs. If
an object has no aranges, `llvm-dwarfdump --debug-info --recurse-depth=0`
lists each CU's `DW_AT_ranges`. A bisect maps an address to its unit.
Clinic and generated `.c.h` code belongs to the including unit, and `.cold`
parts lie in their CU's ranges. `addr2line -f -i -C` names functions, for
display only.

| Class | Rule | Debt |
|---|---|---|
| core | the CU lies under `$SRC`, and `unit_kind()` is `core` or `adapter` | yes |
| core-pending | the CU lies under `$SRC`, `unit_kind()` is None, and the unit is not compat-test | yes (W4) |
| test | CU in `Modules/_testinternalcapi*` (§1.7) | no, per §1.7 |
| compat-test | CU in `COMPAT_TEST_UNITS` (`check_value_api.py`): `_testcapi*`, `_testlimitedcapi*`, `xx*`, `_testbuffer`, `_testmultiphase`, `_testsinglephase`, `_testimportmultiple`, `_ctypes_test`, `Lib/test/test_cext/`, `Lib/test/test_cppext/`; W4 shares the list | no |
| external | in an object none of whose CUs lies under `$SRC` | no |
| foreign | `_Py_BOX_OWNER_FOREIGN` | no |
| unresolved | in no object (`?`, anonymous memory, a perf trampoline); in an object without readable DWARF; in an in-tree object but no CU; a NULL boundary owner | yes |

An object counts as in-tree if any of its CUs lies under `$SRC`, wherever it
was loaded from. `_testembed`, a `--copies` venv and the build's `python`
therefore all resolve as core. `unit_kind()` reuses the compile database of
`make check-value-api`: the defines come from the tree, not from the build.

**`summarize_stats.py`** gains "Boxes by owner object" (kind × object
basename; no DWARF), "Box owner table" (top 30 keys) and "Box owner health"
(entries, drops, `box_cached`, resets). **`run.py`**'s boxes metric reads the
`Boxing by …`, `Box owner (`, `Box object [` and `Object box cached` lines
(its current `Object boxes (` keys match nothing the dump prints), adds
`by_kind`, `box_cached` and per-loop `owners` to each entry, and sets
`PYTHON_PYSTATS_DIR` to a private directory, ending its shared-`/tmp/py_stats`
race.

## 2. Files

| Area | Files | Lines (est.) |
|---|---|---:|
| Headers | `pycore_value.h`, `pycore_value_api.h`, `pycore_value_slots.h`, `pycore_borrow.h`, `pyport.h`, `cpython/pystats.h`, `pycore_interp_structs.h`, `pycore_runtime_structs.h`, and the mark on about 80 declarations | +170 −60 |
| Runtime | `pystats.c` (table, accumulator, markers, dump, object copies) | 260 |
| Runtime | `pyvalue.c` (including `_PyValue_Identity`), `borrow.c`, `initconfig.c` (`-X pystats_dir`), `posixmodule.c` (`_exit`, `exec*`, after-fork), `pystate.c` | 90 |
| Runtime | `getargs.c` | 70 |
| Runtime | `call.c`, `native_scope.c` | 60 |
| Objects | `abstract.c`, `object.c`, `value_ops.c`, `typeobject.c`, and the six container files | 140 |
| Test modules | `Modules/_testinternalcapi*` | 50 |
| Build | `configure.ac`, `configure`, `Makefile.pre.in` | 30 |
| Tools | `check_value_api.py`, new `check_r27.py`, `summarize_stats.py`, `run.py` (documentation checkout) | 150 + 550 + 90 + 40 |
| Library | `subprocess.py`: none, it already copies `-X`. `test/support/script_helper.py`, `test/libregrtest/worker.py` (tag, audit hook) | 40 |
| Tests | `test_pystats.py`, new `test_tools/test_check_r27.py`, `test_embed.py`, `test_sys.py`, `test_pyvalue_box_sites.py` | 150 + 200 + 30 |

## 3. The check, and the per-step report

### 3.1 The three parts at W5, as commands

`$S` is the stats build. `$D` is the debug build that runs
`check-value-api`. `$J` is the documentation checkout.

```sh
# 1. Static
make -C $D check-value-api      # both passes: variadic, slot-read, compat-body,
                                # compat-capture, test-site, boundary-site; 0 deferred units
$S/python $SRC/Tools/build/check_r27.py static --cdb $D/compile_commands.json \
    --ownership <the waves' ownership table>
#   every Modules/ unit with Py_BUILD_CORE_* is value-native (unit_kind), except
#   COMPAT_TEST_UNITS; every ownership row's declaration carries Py_VALUE_COMPAT;
#   no Py_VALUE_DEFER_MARKS. "No core unit calls a marked function" is the build.
# 2. Kernels
python3 $J/pyjit/bench/run.py --build $S --set kernels --metric boxes -o r27-kernels.json
$S/python $SRC/Tools/build/check_r27.py kernels --gate r27-kernels.json
#   every box counter is 0 in all three processes of every kernel (absolute,
#   startup included); no attribution is needed
# 3. Suite
$S/python $SRC/Tools/build/check_r27.py suite --gate --build $S -j 16 -o r27-suite.json
#   0 boxes resolved to core, core-pending or unresolved (CORE by site, others
#   by owner); 0 dropped; TEST per §1.7; no leftover .run outside the crash list
#   (Tools/build/r27_crashes.txt); no uncounted launch outside r27_uncounted.txt;
#   no stats reset outside test_pystats. Listed: compat-test, external, foreign
```

### 3.2 The intermediate report (every step from G on)

`check_r27.py report --kernels r27-kernels.json [--suite r27-suite.json]`.
Each step pastes the output into its notes. The kernel report is due at
every mechanism step and every wave cleanup. The suite report is due at
each mechanism step's final run and each wave's remnant run; it costs one
extra run of about 10 minutes (§6).

| Table | Rows × columns |
|---|---|
| Boxes by class | 20 classes × (per loop on kernels, total on suite) |
| Boxes by kind | 4 kinds × the same |
| Owner split | kind × {core, core-pending, test, compat-test, external, foreign, unresolved} |
| Debt total | boxes attributed to core, core-pending or unresolved: the number that must reach 0 |
| Top debt | 20 owners or sites: function, unit, class, kind, count |
| Accounting | dumps folded, leftover `.run` files, uncounted launches by test, resets, drops, `box_cached` share |

None of these is a gate before W5 (P7).

## 4. Tests

| Module | New cases |
|---|---|
| `test_pystats` (private `PYTHON_PYSTATS_DIR`, which also unbreaks its existing tests where `/tmp/py_stats` exists) | `operator.add(1000, 2000)`: V2a-0's kinds. `_testcapi.number_add(1000, 2000)`: two THUNK parser boxes and one THUNK exit, owned in `_testcapi`. `_testcapi.ObjAdd() + 1000`: SLOT_ADAPTER owner in `_testcapi`; a `METH_O` call: CALL_ADAPTER owner there. A CORE site in `python`. `operator.add(1, 2)`: `box_cached`. Exactly once: a subinterpreter box, a fork child ending in `os._exit` (without its parent's counts), a forkserver worker. A killed child leaves its `.run`. A `script_helper` `-I` child counts. Resets counted. The foreign helper prints `foreign`. No `.tmp` left |
| `test_tools.test_check_r27` | Classes from synthetic dumps (the build's build-id at another path, an object only in `objects/`, an address in no object); folding; every `--gate` violation; the real-dump self-test |
| `test_tools.test_check_value_api` | Fires / does not fire for compat-body, compat-capture (exit outside an entry; entry calling entry), `test-site`, both `boundary-site` tokens |
| Moved (finding 5) | `test_sys`'s stats test and `test_pyvalue_box_sites` measure in a subprocess and read `PYTHON_PYSTATS_DIR`; `test_embed` keeps that variable |
| Acceptance (lead) | Kernel per-class totals equal before and after G except Q6's site; identical release `.text` on gcc and clang (§1.1); two gate-build suite runs, the second recording the report and lists |
| Existing | `test_value_slots`, `test_capi`, `test_getargs`, `test_call`, `test_ctypes`, `test_sys`, `test_embed`, `test_multiprocessing_forkserver`, `test_concurrent_futures`; debug and release suites |

## 5. Counters G moves

No family row of value-api-v2a0.md §1.2 falls. The one box G removes is
`object.__hash__` of an inline value (Q6). Boxes move between kinds as
follows (counts per pass, from §1.2):

| Boxes | From | To | Owner before W4 |
|---|---|---|---|
| Parser output: 52,010 (`json_*`) | CORE | THUNK | `_json`: core-pending |
| `PyList_GetItemRef`: 256,801. Also the getter part of dict iteration and lookup (part of 232,084), cells and struct sequences | CORE | THUNK | the core caller: core |
| Clinic tuple views (`_PyBorrow_TupleItems`' default; part of the call adapter's 3.57M) | CALL_ADAPTER | THUNK | the Clinic wrapper: core or core-pending |
| `_PyValue_RichCompareObject` (mixed `COMPARE_OP`) | THUNK | CORE | – |
| Keyword-argument reads in `getargs.c` | CORE, class dict | THUNK, class parser | the caller of `PyArg_*` |
| `_testinternalcapi` sites (suite only) | various | class test | test |

The CORE total falls by the parser and getter rows. The debt total does not
change, because the core still owns those boxes until W1b, W2 and W4.
Subinterpreters, `os._exit` children and `-X`-propagated isolated children
add boxes the old counters never saw. This raises the suite's totals, not
the kernels'.

## 6. Cost of running it, and what the gate proves

| Item | Cost |
|---|---|
| Stats build (configure, `make -j16`, gcc) | About 2.5 min. `v2a0-stats`'s `config.status` is 18:07:43 and its `python` 18:10:16 |
| Kernels, boxes metric | Unchanged: three processes per kernel |
| Suite with stats and owner tables on, `-j16` | Not measured; estimated at 8–10 min. The clang release suite takes 6 min 16 s and the gcc debug suite 12 min 21 s (A3-2). The table costs about 30 ns per box |
| Dumps | 88–167 KB each, 120 KB on average (measured over 87 kernel dumps). There is one per process, 3,000–6,000 in a suite run. They are folded and deleted every 30 s, which keeps the peak under 100 MB, and the object copies are a few MB. Folding takes about 5 ms per dump and resolution about 10 s |
| Memory | 2 MiB per interpreter with stats on, from its first box |

**Perturbation.** Counting stays in the primitive. The table is outside the
Python heap: one `PyMem_RawCalloc`, with no GC and no Python. The flags change
code generation, not which paths run. The one channel is C stack depth:
`noinline` entries and the lost sibling calls use more stack per C-recursive
level. A test that probes the stack limit can therefore stop at another
depth, and a worker that overflows leaves a `.run` file. The kernels do not
recurse in C, and the acceptance run checks that their counts are equal.

**What a passing suite gate proves** (replacing revision 1's "cannot pass
falsely"). Every process that started stats contributed every box it made
(subinterpreters, fork children, `os._exit`, `exec`), or left a `.run` that
fails the gate unless its test is on the crash list. Within an in-tree object
every box has the right unit, since in-tree code makes no sibling calls
(finding 13); core code resolves as core from any path; an address outside
every object or without DWARF is debt. Owners can be stated only in the
boundary-site files. The gate still fails falsely when an external extension
built without `sysconfig`'s flags tail-calls an entry. It does not see boxes
in processes that never start stats (listed by the audit, gated by
`r27_uncounted.txt`), boxes before a reset (listed; allowed only in
`test_pystats`), or misuse inside the boundary-site files, which this design
reviews as code.

## 7. Files and collisions

| Work in flight | Shared | Resolution |
|---|---|---|
| A5b-3 (integrating) | `pycore_value.h`, `pycore_value_api.h`, `borrow.c` | G starts after A5b-3 lands |
| V2a-0 part 2 (final, not landed) | `pycore_value_slots.h`, `pycore_value_api.h`, `pyvalue.c`, `pystats.[ch]`, `abstract.c`, `object.c`, `value_ops.c`, `typeobject.c`, `check_value_api.py`, `summarize_stats.py`, `test_pystats.py` | G starts after it lands. `typeobject.c` gets 13 marks and 5 exit sites, and integrates after V2a-1a, M1b and M2 |
| A3 step 2 (`bytecodes.c`, the generator) | none: G edits no handler, no generator and not `ceval.c` | – |
| M3 (`call.c`, `methodobject.c`, `descrobject.c`, Clinic) | `call.c` (4 sites and `_PyCall_BoxOwner`) | G-1 lands `_PyBorrow_*For` and `_PyCall_BoxOwner` first, so M3a's adapter states its owners when it is written; otherwise G-2 edits M3a's adapter. `call.c` is on the boundary-site list |
| W1b (container files and their headers) | 6 `Objects/` files; the getters' declarations | G-1 marks the getters `Py_VALUE_ENTRY` and converts their 10 sites. W1b replaces the token with `Py_VALUE_COMPAT(…)` |
| `configure.ac` (A3's options, D15's SOABI) | the `--enable-pystats` branch | A separate hunk; regenerate `configure` at rebase |

## 8. Steps and estimate

The review's estimate, plus 0.5 h for the replacement of Q2 (the `-X`
option, `script_helper`, the audit hook) and for object preservation:

| Diff | Content | Who | h |
|---|---|---|---:|
| G-1 | §1.1 forms and argument; swaps deleted; `Py_VALUE_ENTRY` and `Py_VALUE_COMPAT` composed; `_Py_BOX_CALLER`; `THUNK_NEW`; 47 exit, getter and Kind sites by script from V2a-0's site table; `_PyCall_BoxOwner`; `_PyValue_RichCompareObject`; Q6's `_PyValue_Identity` | lead | 1.5 |
| G-1m | Mark the about 80 declarations and the definitions of §1.3 | Haiku; the lead checks | 0.25 |
| G-2 | Table in `PyInterpreterState`; accumulator and mutex; fork clear; `_exit`/`exec` dump; markers; resets; dump with build-ids and object copies; `-X pystats_dir`; `box_cached`; `BOX_SITE_TEST`; adapter owners (10) and call-adapter owners (7) | lead | 2.0 |
| G-2w | `getargs.c` pass-down (6 functions, 19 entries, 9 sites); `configure` refusals and flag; `make check-r27` | Sonnet | 0.5 |
| G-3a | `check_r27.py`: build-id and CU classes, `.run`/`.spawn` accounting; `test_check_r27.py` | Sonnet | 2.0 |
| G-3b | `check_value_api.py`: the second pass, compat-body, compat-capture, `test-site`, `boundary-site`; self-tests; `_testinternalcapi` sites and the foreign helper | Sonnet | 1.0 |
| G-3c | `summarize_stats.py`; `run.py`; `script_helper`, the regrtest tag and audit hook; `test_pystats`, `test_embed`, `test_sys`, `test_pyvalue_box_sites` | Sonnet | 1.25 |
| Acceptance | kernels before and after; `.text` on two compilers; two gate-build suite runs; report; debug suite | lead | 1.5 |
| **Total** | | lead 5.25, workers 4.75 | **10.0** |

The wall clock is about 5.5 h. The lead's chain is G-1, G-1m, G-2 and
acceptance. G-3a is written against §1.4's dump format while G-2 runs. G-1
is the part that others wait on (M3a, W1b).

## 9. Risks and open questions

| # | Question or risk | Recommendation |
|---|---|---|
| Q1 | The default kind of `_PyBorrow_TupleItems` is THUNK | Accepted (review): the owner's unit is the same either way, and the family class stays comparable |
| Q2' | `r27_uncounted.txt` may be long on the first run (`test_embed`, `test_cmd_line`'s `-E`/`-I` tests) | Each entry names the test and its reason. A new entry needs a reviewed line. Coverage is measured, not assumed |
| Q3 | One stats configuration, refusing `--enable-shared`, `--disable-gil` and LTO | Accepted (coordinator) |
| Q4 | `noipa` is GCC-only; clang relies on `noinline`, whose partial inliner and hot-cold splitting are off by default. Clang was not verified by the review | `test_pystats`'s owner cases fail if an exit moves; run them on a clang stats build once one exists |
| Q10 | The audit hook runs inside each regrtest worker. A test that removes audit hooks, or `os.fork` followed by a raw `execv` from C, escapes it | Rare in the suite; the C-level `exec*` path dumps the parent's counts first (§1.4). An escaped child is missing from the list, not counted as clean; the first run's `.spawn`/`.run` cross-check shows the size |
| R1 | The owner table fills | Drops fail the gate run; raise `BOX_OWNERS_BITS` |
| R2 | The first gate-build suite run surfaces failures that occur only in stats builds (recursion depth, dump timing) | Budgeted as a second acceptance run (§8) |
