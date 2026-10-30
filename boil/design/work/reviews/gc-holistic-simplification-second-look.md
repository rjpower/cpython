# GC simplification: independent second look

2026-10-30, source/design only. Reviewed holistic draft `a462d7d9` against
primary `8ebcf7a5effa` and the unchanged buffer interfaces in candidate168f.
No runtime, build, test, probe or qualification artifact changed. The168
record remains six passing profiles plus the failed root-stats stress
assertion; test-only proposal46c8d397 stays unapplied by this review.

**Verdict: simplify delivery/shutdown policy; hold the generic cleanup
capability and dependency DAG. Revise the recommended Python-buffer ban
to a fallback support choice after a small resource-owner feasibility proof.**
The draft correctly says neither restriction nor serialization repairs
native release. There is a finite useful path before choosing either a
large lifecycle system or removal of Python buffer interoperability.

## What can be simpler

One interpreter-wide drainer with enqueue-and-return on contention can
replace concurrent same-owner FIFO delivery without a completion graph.
`gc_live.c:475–549` currently has only a per-thread recursion guard and
pops the running call into an opaque C local. A drain claim must cover
that call, exception reporting and detachment; its running object closure
must remain traceable. No queue/registry mutex may survive into callbacks.
Nested collection must enqueue and return, not wait for the callback
that invoked it. This is a bounded change in delivery policy, not proof
that all finalizers finish: a callback awaiting another automatic callback
behind itself can block forever. It also changes the current source's
collect-and-drain behavior. State that cost instead of promising independent
progress and rebuilding a scheduler to supply it.

Main-only operation is an explicit experimental product boundary, not a
necessary property of mark/sweep. It removes cross-owner close/adoption
obligations, but not buffer cycles, callback reentry or single-owner native
cleanup. Rejecting unsupported creation/embedding/fork paths before side
effects is honest; silently treating existing supported paths as outside
qualification is not. Do not enlarge this restriction merely to avoid a
small ordinary-runtime ownership repair.

## Resource ownership before a new type capability

The current defects are ordinary-runtime defects. `PyBuffer_Release`
(`Objects/abstract.c:823–844`) skips a release callback after DEAD|SWEPT;
memoryview deallocation and queued release force its external export count
to zero (`memoryobject.c:1146–1155,1187–1200`). Neither is justified by a
nonmoving heap: intact object storage does not preserve destroyed native
export state or balance external allocations. A C callback can allocate,
detach, call Python indirectly or free acquisition metadata; C syntax is
not a leaf-safety contract.

A local first repair is available: struct's unpack iterator is neither
subclassable nor reacquiring, and exhaustion sets `so=NULL` before release
(`_struct.c:2221–2313`). Its existing finalization transport and an
idempotent exhaustion/release transaction deserve evaluation before a
general type slot. This is one family, not a five-family solution. In
particular, a Python Unpickler subclass can override `__del__`, and a
resurrected/reinitialized consumer can acquire again. Five once-per-object
finalizers do not solve that.

For the common direction, **prefer evaluating one sealed per-acquisition
owner in the existing memoryview/managed-buffer subsystem** over a new
type-wide pending/run capability. `_PyManagedBufferObject` already owns a
stable master view, traverses its exporter and records RELEASED
(`memoryobject.c:74–145`). A new acquisition can have a new owner; explicit
release consumes that acquisition rather than rearming a user finalizer.
This avoids Python-subclass replacement and per-object reacquisition state
in the cleanup mechanism. It still needs an idempotent releasing state,
valid original callback metadata, and consumer access checks: an owner
does not invalidate a scanner's cached raw pointers by itself. Acquire
directly into stable owner storage; copying a Py_buffer can leave shape,
stride or internal pointers into the old consumer field. Do not add a
second owner allocation where an existing memoryview already owns the lease.
Every outward view must also keep that metadata owner alive, rather than
only retaining the original exporter while borrowing the owner's arrays.

This has a concrete package precedent: NumPy's `PyArray_FromBuffer`
wraps exporters with release callbacks in a memoryview, then retains that
object as the array's base. That centralizes acquisition lifetime rather
than adding cleanup slots to every consumer. It does not prove NumPy is
ported or that exporter-only tests cover imported buffers.
[NumPy source](https://github.com/numpy/numpy/blob/main/numpy/_core/src/multiarray/ctors.c#L3541-L3572)

## Export counts are the remaining proof boundary

For H→owner→M→managed-buffer→wrapper→E→H, a strong object SCC need not be
an acquisition SCC. Releasing H's acquisition can reduce M's outward
exports to zero before M releases upstream. Current `_memory_release`
already decrements mbuf registrations and releases the master at the last
one (`memoryobject.c:1113–1124`). This supports examining ordinary lease
readiness before inventing generic SCC completion tokens.

However, checking `get_exports(M)==0` or `mbuf.exports==0` and simply
skipping other hooks is unsafe: an unqueued DEAD object can still reach
sweep. In the ordinary case, an eligible consumer-owner finalizer must
retain/resurrect its entire exporter chain before DESTROY discovery.
Otherwise the unready object and its dependencies need an explicit
retention reason. Establish that every outstanding acquisition has a
traceable active holder or queued/running owner. Unported consumers,
forgotten releases and opaque external holdings invalidate that inference.

Immutable acquisition-completion order may make ordinary memoryview
forwarding acyclic. Raw object allocation age does not prove this:
getbuffer callbacks can allocate proxies, and arbitrary native exporters
can mutate dependencies. Prove the finite H/M/E case, then forwarding to
the original returned view U; do not assert a universal no-leaf-cycle
theorem. Preserve balanced release and no premature exporter destruction
with ordinary API observations. No generic graph implementation follows
from this review.

## Primary precedents and shutdown boundary

PEP442 separates finalization of intact objects, resurrection checking and
later clearing. That is a useful phase-separation precedent; its reference
counted disposal is not a drop-in non-refcount implementation. Existing
tp_clear likewise cannot be called indiscriminately: it may clear much
more than a resource, and some consumers lack it.
[PEP442](https://peps.python.org/pep-0442/#disposal-of-cyclic-isolates),
[type-slot contract](https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_clear)

Boehm's primary guidance supports deferred resource finalization and
explicit close for prompt effects, while warning about synchronization
and retaining dependency cycles. It does not establish arbitrary native
deallocators are safe during sweep. Python's buffer protocol also requires
balanced acquisition/release; delaying collection is not permission to
omit release forever during supported execution.
[Boehm finalization](https://www.hboehm.info/gc/finalization.html),
[PEP688](https://peps.python.org/pep-0688/#python-level-buffer-protocol)

An actual process-exit cutoff after exit handlers/stream flushes can leave
memory to the OS, provided no active callback resumes into freed runtime
state. This is different from Py_FinalizeEx returning to a live host or
repeated interpreter deletion. Retaining each deleted heap or acquisition
there causes persistent growth/resource exhaustion. Either reclaim it or
reject that support mode explicitly; shutdown caveats do not license an
unbounded normal-runtime leak.
[finalization contract](https://docs.python.org/3/c-api/interp-lifecycle.html#c.Py_FinalizeEx)

The next decision should therefore be a finite owner/readiness proof and
one irreversible consumer repair, alongside the small drainer policy.
Keep Python buffer support as the preferred compatibility direction until
that proof finds a concrete unavoidable cost. If it fails, choose and
document a narrower port profile; do not call forcing export counts or
skipping callbacks a completed collector. General cleanup slots, adoption,
borrow leases and dependency scheduling remain held.

## Main-interpreter-only admission: source audit and implementation cut

Policy follow-up at primary `7cd98794c981`: PROCESS now selects one main
interpreter, including ordinary threads. **This policy is not enforced in
the inspected source.** No source, test selection or qualification receipt
was changed by this audit. Process workers remain an explicit application
choice; interpreter IDs, thread states and shared native state must never
be silently replaced with subprocess handles.

| Creation entry | Actual route and required failure |
|---|---|
| `Py_NewInterpreterFromConfig` | `pylifecycle.c:2692` calls `new_interpreter:2564`. Return an error `PyStatus` containing the unsupported explanation and set `*tstate_p=NULL`; no Python exception is required, since the caller may have no attached state. Reject before disabling `gilstate.check_enabled`, detaching a thread or creating interpreter state. |
| `Py_NewInterpreter` | `pylifecycle.c:2700` uses the same helper, but currently calls `Py_ExitStatusException` on error. For this deliberate unsupported operation, return NULL nonfatally instead. Preserve the caller's current-thread state; callers cannot assume a Python exception exists. Merely adding a failing status to the common helper would terminate legacy callers. |
| `PyInterpreterState_New` | `pystate.c:663` directly calls the state allocator, bypassing full initialization, and also exits on error today. Explicitly return NULL for unsupported creation, including callers without an attached state; do not unconditionally call `PyErr_SetString`. Its documented precondition is that Python is initialized (`Doc/c-api/subinterpreters.rst:430–445`). |
| `_interpreters.create` | `Modules/_interpretersmodule.c:901` → `_PyXI_NewInterpreter` (`crossinterp.c:3266`) → configured C API. Existing XI code restores the saved thread state and converts the failing status to a chained `InterpreterError`; the module chains it again. This already supplies a nonfatal Python rejection with the unsupported cause. No new Python creation backend is necessary. |
| `concurrent.interpreters.create` / `InterpreterPoolExecutor` | The first delegates directly to `_interpreters.create` (`__init__.py:63`). Pool workers call it in `WorkerContext.initialize` (`futures/interpreter.py:59–61`); unsupported creation therefore breaks worker initialization, rather than spawning a process. Document this consequence; importing these modules need not fail. |

The smallest runtime cut is **two files**, `Python/pylifecycle.c` and
`Python/pystate.c`: reject full creation before its state changes, handle
the two legacy pointer-returning APIs nonfatally, and enforce a second
admission check inside `_PyInterpreterState_New:579`. That internal check
must hold `HEAD_LOCK` and reject an already-existing main interpreter
**before** incrementing `next_id`, allocating or publishing another state.
It must inspect runtime interpreter membership, not `tstate!=NULL`: XI
deliberately swaps the current thread state to NULL before calling the
configured API, and the low-level public API permits unattached callers.
Keep audit-attempt behavior/error documentation explicit; do not hold the
head lock while invoking an audit hook or raising a Python exception.

Main bootstrap remains `_PyInterpreterState_New(NULL, &interp)` in
`pycore_create_interpreter` (`pylifecycle.c:665–675`), installing the static
`runtime->_main_interpreter` when no main exists. Do not reject that call
or use a blanket ban on the shared allocator. The public low-level API is
not an alternative bootstrap path. This change does not decide whether
reinitialization after `Py_FinalizeEx` is supported; that remains a separate
embedding contract.

Ordinary threads use different machinery: `_thread` creates a thread state
for the current interpreter (`_threadmodule.c:462–464`);
`PyThreadState_New`/`_PyThreadState_New` allocate thread states
(`pystate.c:1546–1570`), and `PyGILState_Ensure` obtains the main interpreter
guard and creates a thread state (`pystate.c:2606–2637`). Leave these paths,
the GIL, interpreter guards and main initialization intact. Main startup
also calls `_PyXI_InitTypes` and `_PyXI_Init` (`pylifecycle.c:819,1012`);
removing all cross-interpreter support code would break more than creation.

The repository-wide C call search found no other production interpreter
allocator: the full lifecycle helper, bare public state API and XI route
above cover it. Remaining direct callers are `_testcapi`,
`_testinternalcapi` and `_testembed`. Some testing helpers assume creation
succeeds and immediately dereference/end the returned state, for example
`_testcapi.test_current_tstate_matches:1075` and `_testembed:160,1382`.
Do not run those blindly against a rejection-only patch. A focused follow-up
must test the public rejection contracts and explicitly classify existing
subinterpreter-only tests as unsupported; no current protected selection or
historical result is modified here. `_testinternalcapi.create_interpreter`
already covers XI, bare-state, legacy and configured origins (`:2337–2436`).
Use those existing seams where their failure handling is sound, repairing
only unsafe test helpers needed for the rejection tests.

No creation dependency was found in `ThreadPoolExecutor`,
`ProcessPoolExecutor` or multiprocessing's spawn/forkserver paths. Spawn
forms a fresh executable command (`multiprocessing/spawn.py:93–96`), and
forkserver likewise starts an executable (`forkserver.py:204–210`), each
with its own main interpreter. This audit neither qualifies fork safety
nor changes those APIs. Keep ordinary buffer release, main-interpreter
finalizer delivery and process-exit versus live-host teardown separate.

Before implementation is accepted, bounded controls should check all three
C entries (including no attached thread), both Python creation routes,
unchanged main identity/list and caller thread attachment after rejection,
and working ordinary threads/GIL-state attachment. An explicit status/NULL
or Python error is the oracle, not a crash or process exit. The planned
documentation cut is the existing C subinterpreter reference and Python
interpreter/pool references; no subprocess wrapper or lifecycle framework
belongs in this increment.

Inspected runtime SHA256 identities at that HEAD:
`pystate.c=e5c5634e9719406f1dd43aae8be0eb317b35dbf482f7b12d4307884f229b572e`,
`pylifecycle.c=47dc4c6bcf869b0fb9d769c93e689f43bbabff4a349f29b8f678ed3285e60093`,
`crossinterp.c=64b273b88b0a86d71048eab504d41bda321d991909e3318655d51e816a0f9f17`,
`_interpretersmodule.c=5c7443a3130664d53636d4c972231f9439365a5e27dee0d1fc10bffe06f8de9e`.
