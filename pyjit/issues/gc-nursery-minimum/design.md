# Minimal stable-address young collection

Source proposal, 2026-10-31. Base `b3f2bcc605c`; isolated branch
`boil/gc-nursery-minimum`, checkout `../boil-gc-nursery-minimum`.
This is nonmoving generational collection using existing mimalloc storage,
not a bump allocator, copying space or performance claim.

## Hypothesis and useful consumer

One unused header bit can identify opt-in exact cells/nonempty tuples from
allocation until their first surviving collection. While the private test
control enables the experiment, ordinary `gc.collect(0)` runs a real minor:
young garbage can die, surviving young objects promote in place, and every
old object plus its outgoing closure is retained conservatively. Full and
automatic collections remain majors. The default is disabled.

The test control is temporary experiment admission, not a preferred permanent
GC API. It isolates the unsupported verifier/stress modes and leaves the
qualified default intact. At this stage's decision, explicitly retain it for
the next bounded experiment, replace it with a settled default contract, or
reject the path; do not let the knob become permanent by omission.

This deliberately scans all old objects and builds ordinary dead records for
young deaths. It proves a usable generation boundary, not reduced old work or
cheap reclamation. Cells/tuples are a bounded first cohort, not demonstrated
coverage of dominant generator/coroutine allocations.

## Contracts

- Reserve header bit15 for YOUNG, separate from TRACKED/MARKED/DEAD. Set it
  only at exact cell/tuple allocation; empty static tuples and subclasses
  remain old. Existing flag masks must preserve age through tracking and
  marking; block reuse starts with cleared flags as before.
  A non-rooting `young_pending` hint survives disabling, so a subsequent major
  still promotes the cohort. It may remain conservatively set after dead
  candidates are freed; the next successful census retires it. The ordinary
  never-enabled runtime does not acquire an extra promotion walk.
- Before root closure, visit every allocated old object through the existing
  conservative/SHALLOW-safe marking path. This covers old→young stores with
  no barrier and temporarily retains unreachable old owners. There is no
  native-owner registry, moving alias or new allocation ownership protocol.
  The all-old census requires TRACKED before interpreting tuple items: it can
  encounter an unpublished tuple that ordinary roots have never exposed.
  This artificial retention does not rearm old DEL_CALLED/non-GC FINALIZED
  state; a later major determines actual reachability again.
- Use the same mark stack, dead inventory, lifecycle preparation and sweep.
  Promote survivors only after successful tracing; OOM keeps everything and
  clears scratch marks/DEAD through existing cleanup. Partial promotion is
  unnecessary. Major success also clears young age, without changing reachability.
- Pass the minor request explicitly through collection calls. Nested callbacks
  and automatic collections must not inherit a mutable global minor mode.
  Reject mode changes during collecting/sweeping/draining. Stage1 supports
  only main/GIL/mimalloc with verifier/stress off; report unsupported modes
  explicitly rather than silently label a major as a minor.
- Generation0 callbacks/stats identify minors. A minor never resets major
  allocation debt or next-major limit; periodic automatic majors remain active
  when GC is enabled. Explicit full collection still reclaims old cycles and
  resource owners. With automatic GC disabled, the caller owns explicit majors.
  Experimental `gc.get_objects` takes a noncollecting heap snapshot; the normal
  snapshot helper forces a major and would erase the age observation.
  Both paths expose only GC-capable, tracked objects, never unpublished slots.
- Preserve GC debug/SAVEALL, reftracer and ordinary deallocation behavior;
  there is no bulk discard or suppressed observer in this experiment.

## Owned source

`Include/object.h`, internal `pycore_gc.h`, `pycore_gc_trace.h`,
`pycore_interp_structs.h`; `Objects/cellobject.c`, `Objects/tupleobject.c`;
`Python/gc_trace.c`, `Python/gc_live.c`; `Modules/_testinternalcapi.c`;
`Lib/test/test_gc.py`. No frame/genobject, interpreter dispatch or allocator
ownership edits. Ten source/test paths; an earlier handoff counted nine.

## Acceptance and stopping rule

Observe normal data/cycles across minor and promotion, old→young cell updates,
old unreachable owner retention then eventual major reclamation, young garbage
reclamation, and honest generation callbacks/statistics. Check major debt
preservation and invalid/reentrant mode rejection as critical internal units.
Use normal GC inspection, not a raw-address memory fixture. Existing major GC
tests must preserve default-disabled behavior.

Independent source review precedes native work. First feedback is one fresh
O1/Py_DEBUG build and bounded mode-off controls; no broad full campaign before
the small consumer works. If publication, verifier boundaries or accounting
require a second collector/framework, stop and narrow. The first-hour outcome
is a reviewed working candidate, a concrete rejection, or one bounded remaining
implementation batch—not a promise that qualification finishes within an hour.
