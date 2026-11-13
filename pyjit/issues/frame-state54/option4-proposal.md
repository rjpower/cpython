# Option 4: locals mapping on the optional frame object

Status: proposal awaiting coordinator and independent field-owner review.
Source audited: `BASE/worktrees/frame54-locals`, selected51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. No source changes, generation,
builds or native execution yet. Canonical startup, process, ABI, inventories,
Astra review and CPython AI policy were read.

## Representation and interface

Remove `_PyInterpreterFrame.f_locals`. Add nullable strong
`PyFrameObject.f_locals_mapping`, distinct from extra locals and the borrowed-API
cache. Keep `frame_obj` as the sole nullable cold pointer. No new sidecar,
cached header mapping, dual ABI or allocating INIT uop.

Change the consuming, nonfallible `_PyFrame_Initialize` locals parameter to
`PyFrameObject *frame_obj`: it consumes the prepared frame-object owner (or
NULL), as well as func, and associates a supplied object with the complete
header. It does not allocate or materialize kwargs. Supplied objects already
hold their mapping owner. The function/code owners remain independent.

Introduce `_PyFrame_GetLocalsMapping(frame)`, returning a borrowed nullable
mapping without allocation. For thread/generator frames it acquire-loads
frame_obj and reads its mapping. For FRAME_OWNED_BY_FRAME_OBJECT, derive the
containing PyFrameObject from the embedded `_f_frame_data` address using
offsetof; its header frame_obj stays NULL, preserving the no-self-cycle
invariant. Define this layout conversion once, with owner/address assertions.
Cold getters/setters and teardown use the same explicit owner helper; no caller
guesses a second mapping location. Interpreter sentinels return NULL.

## Preparation and complete caller census

`eval_frame_push_and_init_code` is the only normal initializer supplied with a
mapping. After `_PyCode_GetExec(code)` succeeds and before reserving thread
storage, allocate `_PyFrame_New_NoTrack(code)` only if locals != NULL. This
object starts with f_frame=NULL and mapping=NULL and remains untracked. On
allocation failure the existing fail block still owns func, locals and args.
On reservation failure, decref the empty prepared object and use that same fail
block. No partial child exists in either case.

After successful reservation, commit without callbacks: transfer locals to the
prepared object's mapping, NULL the preparation locals variable, and pass the
prepared object to Initialize. Initialize installs f_frame and frame_obj after
the header/homes are ready. Track this object before any binder callback or GC
poll. Binder failure uses the usual fully clearable child retirement. No
rollback begins after Initialize consumes func/object. Code stays independently
pinned by the provider through preparation and binding.

All `_PyEvalFramePushAndInit`, eval/exec/code APIs, generic call preparation and
EX argument unpacking funnel through this helper; their preexisting consumption
contracts remain intact. Exact INIT in `bytecodes.c`, header push/overlap helpers
and their generated consumers already supply NULL; update their parameter
meaning without adding fallibility or allocation. Trampoline, interpreter-entry
and thread-base initializers lose f_locals writes; they retain their existing
sentinel roles.

`PyFrame_New` already allocates f: set its mapping to Py_XNewRef(locals), invoke
the direct heap initializer with NULL frame_obj, and establish FRAME_OBJECT
ownership/f_frame before tracking. There is no extra allocation or self owner.
`_PyFrame_New_NoTrack` initializes mapping and f_frame to NULL so failed
preparation can deallocate without reading uninitialized frame storage.

Lazy `_PyFrame_MakeAndSetFrameObject` still materializes kwargs, allocates one
object with NULL mapping, associates it, and retains existing acquire/CAS
publication. CAS losers own no mapping. An eagerly associated object must not
bypass pending-kwargs materialization when published for observation: existing
object returns must establish the same observer precondition. This applies to
frame getters/properties as well as ancestor observation.

## Copy, escape and retirement

`_PyFrame_Copy` moves frame_obj without duplicating the mapping, updates that
object's f_frame at the established copy commit, and clears the source pointer.
Homes/code/function remain complete moves. RETURN_GENERATOR can encounter an
eager object even before RESUME; replace its old NULL assertion with the actual
association invariant. Generator escape/resume therefore retains the same
mapping owner. `take_ownership` starts with the source pointer detached as today;
the surviving object's mapping does not move, and its embedded frame has NULL
frame_obj. Back-link construction and completion fixup remain unchanged.

`gen_new_with_qualname` converts a FRAME_OBJECT header whose frame_obj is NULL:
explicitly give the destination generator a new owning reference to f, point
f_frame at that destination, then consume the input f reference as before.
The mapping stays on f. This makes the ownership transition explicit instead
of expecting Copy to manufacture a self owner.

Split physical-home clearing from mapping clearing. `_PyFrame_ClearLocals`
clears all homes then clears the mapping on its still-valid owner. In
`_PyFrame_ClearExceptCode`, detach frame_obj first. An escaped object takes
ownership, retaining its mapping unchanged. For a unique object, detach mapping
into a C owner and NULL its member before decrefing the object; clear the named
homes for thread frames or all homes otherwise, then close the held mapping,
then close function. The executable remains pinned through callbacks. This
preserves homes-before-mapping-before-function retirement without accessing a
freed object. The retiring frame is unlinked, and each home/member becomes NULL
before a callback-capable close. A weakref callback can inspect valid ancestors;
it cannot recover a dangling frame object. A retained frame forces takeover.

`frame_tp_clear` clears homes then mapping on self. `frame_dealloc` clears its
mapping exactly once regardless of whether its header is embedded; it must not
dereference external stale frame storage (the existing trashcan constraint).
The physical-home/code/function deallocation order otherwise stays selected.
Frame-object destruction never relies on a mapping cache in the header.

## GC and access consumers

Visit the mapping once in `frame_traverse`, before its non-FRAME_OBJECT early
return. Remove mapping visitation from `_PyFrame_Traverse`; that routine visits
frame_obj and ordinary header/home owners. Crucially, mapping-bearing objects
must be GC-tracked after complete association even while their frame lives on a
thread or generator: selected lazy objects can be untracked until takeover,
which would hide the new edge. If a nonoptimized NULL-mapping frame later gets
an empty dict through either locals getter, fully associate and track its
object before exposing the mapping. No tracked prepared object has f_frame=NULL.
Retain existing critical sections and atomic publication in free-threaded code.

Update LOCALS() and all direct readers in frameobject.c, ceval.c and import.c.
This includes both nonoptimized empty-dict creation sites, _PyEval_LoadName,
PyEval_GetFrameLocals/GetLocals and exact globals==locals identity checks at
import.c:4540 and ceval.c:4957. LOAD/STORE/DELETE_NAME, LOAD_LOCALS, imports and
class code obtain the original mapping object through the accessor. Optimized
locals proxies/cache/extra locals keep their distinct behavior. Traversal roots,
generator/frame clear and observers use the one owner; mapping callbacks still
see the same mapping identity and valid home state.

## Files, evidence and limits

Handwritten scope: pycore_frame.h, pycore_interpframe_structs.h,
pycore_interpframe.h, frame.c, frameobject.c, genobject.c, ceval.c,
ceval_macros.h, bytecodes.c, pystate.c and import.c, plus focused tests.
Audit debug offsets, GDB, remote readers and deepfreeze for layout dependencies;
update actual references if present. Root owns regeneration of generated cases,
deepfreeze and all builds/execution. No runtime edits in canonical C.

Predicted GIL header: 112 to 104 bytes, subject to real native offsets,
FRAME_SPECIALS_SIZE/co_framesize, generator tails, chunk arithmetic and debug/
free-threaded alignment. PyFrameObject itself gains a pointer while its embedded
header loses one; mapping entries newly pay allocation of the full variable
frame-object tail before escape. Mapping accesses add indirection. Ordinary
NULL-mapping entry loses one header store/word and allocates no object; teardown
adds conditional record handling. Native inspection must establish both sides.

New discoverable tests will live in test.test_frame.TestFrameLocalsMapping:
test_exec_custom_mapping_identity, test_class_prepare_mapping,
test_mapping_error_traceback_escape, test_mapping_cycle_collection,
test_mapping_finalizer_observes_cleared_homes, test_generator_frame_mapping,
test_trace_mapping_identity, and test_prepare_allocation_failure. Allocation
failure uses an isolated subprocess and the existing
_testcapi.call_with_nomemory(start, stop, exec, code, globals, mapping), which
arms allocation failure in C and removes hooks before returning to Python.
Require recovery, no executed body and no leaked preparation owners.
Extend test.test_capi.test_frame.FrameTest for frame_new mapping and conversion
coverage, using the existing frame_new helper and a bounded C helper if needed.
Root runs these plus existing ClearTest, WeakRefTest,
TestIncompleteFrameAreInvisible, test_capi.test_eval and class/exec coverage in
release/debug. Independent review checks the actual diff before broader gates.

Root must measure class/module/exec entry and startup/import as well as the six
unchanged kernels and all29 where admitted. Reject incoherent preparation/GC/
reentry or repeated breadth losses; do not rescue this lane with a second
mapping location. Proposal review is bounded to about ten minutes; first source
implementation is bounded to about thirty minutes after explicit acceptance.
