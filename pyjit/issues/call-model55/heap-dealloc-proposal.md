# Private heap-frame deallocation

2026-11-12. Proposed against pins merge `56a8a38fcf8` (runtime identical to
integrated `de95475accb`). GC design peer agrees with the root policy below.
Files: `Objects/frameobject.c`, `Lib/test/test_frame.py`; no shared schema change.
IR owns other producer/reset ports; this body will reset its own phase before
callbacks once the agreed phase field is integrated.

The existing deallocator closes code before function/mapping/homes. Code must
instead stay independently pinned through the home walk and every later owner
close. The heap frame is refcount zero and untracked, unlike live `tp_clear`.
A Python locals proxy or callback retaining it would prevent this entry. Its
weakrefs are cleared before object-owner closes. Unsupported resurrection via
a raw borrowed pointer to a refcount-zero object is not a local mutation API.
Keep live clear's one-pass observer-write behavior unchanged.

Use a reached cold owned-frame helper with the existing 40-byte transition
record, initialized over the full admitted home range and typed headers before
weakref callbacks. It is an external marking bridge only, never another cyclic
container edge. Remove each header bit before its close; preserve function then
mapping close positions; use `_PyFrame_FinishClearExceptCode` for the reverse
remaining cursor, with those fields already NULL. There is no association or
takeover in this private frame. Keep CODE until all home and metadata closes
complete, clear it before final code close, then pop the exhausted record before
free. No thread reservation or generator exception state is involved. Literal
HEAP_CLEAR remains uncovered by the normal-return shadow certificate.

Before the first callback, move f_back, f_trace, f_extra_locals, f_locals_cache
and f_overwritten_fast_locals into a five-element owned `PyObject *` C array and
NULL their fields. Do not increment counts or decode them as PyValue: a trace
field can be an arbitrary object. Release them at their existing positions,
after homes, in the existing order. This is the deliberately retained native
stack/register policy for actual detached C owners. It is not a claim that an
untracked frame pointer makes its heap fields discoverable: tracing marks an
untracked conservatively reached GC object shallowly. No global native head or
typed-object-prefix ABI is needed. Associated/delayed-trashcan frame objects
retain the existing embedded-address ownership gate and never dereference a
possibly expired external interpreter frame.

Focused fixtures use a distinct code.replace image and FunctionType so only
the private heap frame pins the selected code. A local finalizer holds weak
function/code/frame references only, checks that weak frame/function references
are dead but code remains alive, and collects/nests cleanup. A companion precise
root query checks a lower remaining home during a higher-home finalizer. An
observer-field finalizer checks fixed-last code and native metadata lifetime.
Keep callback observations outside unraisable assertions and assert afterward.
Existing weakref/cyclic clear tests cover live resurrection paths; do not turn
this into a repeated-drain change. Root owns generated composition, both-role
execution and native costs.
