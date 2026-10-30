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
