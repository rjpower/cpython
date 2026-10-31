# Ordinary watcher events: metadata and execution context

Source-only follow-up, 2026-10-30. Queue2b source-v1 remains immutable.
This note recommends a contract; none of it is implemented or granted.

Coordinator design ruling: accept synchronous mutator context and mutator
unraisable reporting for owner-selected dict/type user callbacks; retain
function MODIFY4's mutator watcher set, CREATE allocation and DESTROY owner.
The owner-table lifetime/active-work hold, including OS-thread self-close
checks, is mandatory before code. This is design acceptance only.

## What the current source guarantees

Dict `_PyDict_SendEvent`, type `_PyType_Modified_Unlocked` and function
`handle_func_event` select callbacks through the attached interpreter.
Callbacks execute synchronously with that thread state, outside HEAD.
Errors are reported by that interpreter's unraisable machinery; pending
exceptions must be preserved according to each watcher API's contract.

The references do not separately promise an object's allocator owner as
the execution interpreter of an ordinary event. Function AddWatcher/Clear
explicitly describe registration for the current interpreter. Nevertheless,
the source currently couples table selection and execution context. Calling
a foreign table's C callback under the unchanged mutator breaks that
coupling: it is an explicit shared-heap compatibility decision, not a
mechanical table fix. A callback may resolve module state through current
tstate; selecting its owner's callback does not select its owner's state.

| Family | Events and actual boundary |
| --- | --- |
| Dict | ADDED/MODIFIED/DELETED/CLEARED/CLONED notify before publication; DEALLOCATED is a distinct queued destruction event (reserved callbacks stay in sweep). |
| Type | Modification invalidates the type and recursively its subclasses; destruction is a separate queued event. A subtype may have a different allocator owner from its mutable base. |
| Function | CREATE after initialization; MODIFY_CODE/DEFAULTS/KWDEFAULTS/QUALNAME before assignment; DESTROY is separate. Function watcher IDs are interpreter-global; no watcher bit/index is stored on each function. |
| Code | CREATE after allocation/initialization and DESTROY only; there is no ordinary modification watcher event. Code-extra writes have their separate guard. |

Function MODIFY callsites are the public SetDefaults/SetKwDefaults and
the __code__/__qualname__/__defaults__/__kwdefaults__ setters. The same
helper increments the mutator's func_modification rare-event counter.
Do not silently change the counter's meaning when deciding event ownership.

## Recommended contract to rule explicitly

For ordinary events, retain synchronous execution in the mutating
interpreter; do not implicitly switch thread state or queue a notification.
For dict/type, resolve user watcher bits through the OBJECT owner's table,
not the mutator's colliding IDs. Document that the registered callback may
run under a different interpreter in a supported shared-heap domain, and
that its error is reported through the MUTATOR's unraisable hook. It must
not assume registration selects current interpreter/module state. Reserved
dict callbacks retain their reviewed common-table contract.

For function MODIFY4, recommend retaining the current mutator's watcher
set: it observes operations in the interpreter where it registered, and
no per-object watcher ID is being misinterpreted. This differs deliberately
from allocator-owner DESTROY delivery and needs an explicit documented
event-context table. CREATE remains allocation context. If coordinator
instead wants function lifetime-wide owner observation, apply the same
foreign-table/context and lifetime contract as dict/type; it is not forced
by the bit-index bug. Code CREATE/DESTROY need no additional event family.

This choice avoids changing synchronous pre-mutation semantics, void API
error contracts or cross-owner inheritance. It is acceptable ONLY after
the callback-context compatibility rule and foreign table lifetime below
are accepted. If callbacks must always execute attached to their
registration owner, this recommendation is insufficient: choose a larger
checked-mutation/rejection design before implementation. Do not call the
wrong table, drop the event, or mislabel mutator errors as owner execution.

## Concrete lifetime prerequisite

### Function version caches are a distinct invariant

Foreign MODIFY_CODE/DEFAULTS/KWDEFAULTS calls `func_clear_version` with
the mutator. That clears the function's SHARED scalar func_version, so
the actual specialized-call/property guards that compare that scalar
observe invalidation in every interpreter of the shared-GIL domain.
QUALNAME does not call this invalidation helper; it is not one of the
critical call attributes. Closure/vectorcall changes also invalidate the
scalar although they do not have watcher events.

There is nevertheless an owner-side cache bookkeeping gap: MAKE_FUNCTION
publishes (func,code) in its creating interpreter's func_version_cache.
Foreign `func_clear_version` only removes the mutator's matching slot and
sets the shared scalar CLEARED. The owner's slot can retain the live
function with its old code, violating the documented cache invariant.
Subsequent mutations return early on CLEARED and do not repair that slot.

At this source there is no function-version cache lookup consumer: exact
field/callsite search finds publication/invalidation and collector cleanup,
not the historical optimizer lookup described in its comment. This limits
the current behavioral claim; it does not make the stale invariant sound.
`live_clear_weak_caches` scans ALL domain members and removes dead function/
code pointers before sweep, so wrong-current dealloc cleanup alone does
not establish a dangling-pointer bug. It also does not remove a LIVE
function's stale owner slot when that function changes code.

Before restoring a cache consumer or claiming cache owner correctness,
invalidate the matching function identity in every relevant domain cache
under the domain-GIL/lifetime rules (or provide one proved cache-owner
mapping). Do not infer it from the mutable watcher's chosen table, or
blindly pass function allocator owner: a new function may use foreign code,
whose co_version is copied at MAKE_FUNCTION. Preserve scalar invalidation
and test code replacement/default changes with both creator and mutator
caches populated. This is an identified source invariant, not a reproduced
specialization error or a runtime expansion of Queue2b source-v1.

An unpinned owner lookup cannot supply a table for later invocation.
Under HEAD identify the owner and acquire a finite callback/table lifetime
hold before copying callback pointers; release HEAD before invoking any
callback. Retirement/deletion must wait for all such holds through callback
return AND unraisable reporting, including GIL release and nested events.
Keep the preexisting slot-change/reentrant watcher rules explicit: a copied
callback is the callback selected for this event, not a reused-ID lookup.

This hold must participate in close's kind-independent active-work and
same-OS-thread reentrant-close checks. It cannot be a borrowed raw interp
pointer or a claim that routing queue_pins already protect active Python.
The evidence for this need is concrete: type/function callbacks can execute
Python, and PyErr_FormatUnraisable can enter sys.unraisablehook even when a
dict watcher obeys its no-Python rule. Either can detach or close/reenter.
No callback, boxing or Python error formatting under HEAD. Closed/adopted
heaps must have had user metadata retired before foreign dispatch uses
main's tables; missing/retired ownership is not the mutator by default.

## Why blanket rejection is not a small equivalent fix

Dict NotifyEvent/SendEvent, replace_value, clear_lock_held and public
PyDict_Clear are void. Merely setting an error in NotifyEvent does not
stop the write; returning early from PyDict_Clear silently changes the
void C API's cleanup behavior. Rejection would need checked internal
mutation paths, propagation/cleanup at all notification sites and an
explicit contract for public void operations. Int-returning insertion,
deletion/pop/merge paths do not cover clear or every internal fast path.

PyType_Modified is also void and is documented for use AFTER manual field
changes. Even an early top-level type-owner check does not cover recursive
notifications of foreign watched descendants. Preventing cross-owner
base/subtype dependencies would be a further compatibility restriction.

Function MODIFY4 setters can return errors before publication, so checked
foreign rejection is individually feasible there. Preserve validation,
audit ordering and owned new-value cleanup. It is optional policy, not a
solution to dict/type's void/cross-owner cases or the function cache policy.

## Bounded future controls and files

After the rule/lifetime design is reviewed, use existing stash and legacy
interpreter APIs with colliding dict/type IDs. Mutate a watched main object
from sub and the reverse; record which callback was selected, actual
tstate/current interpreter, pre-mutation visible state and unraisable hook.
Include Clear/clone/delete and a foreign watched subclass of a local base.
For function MODIFY4, distinguish owner-only and mutator-only registrations
and verify the explicitly chosen context and unchanged pre-assignment data.
An error callback and a nested event must keep the original exception and
report once; close while a callback is detached must not retire its table.
No raw memory, generic observer framework or implicit tstate switching.

Likely files: dictobject/typeobject/funcobject plus internal notification
declarations; narrowly shared lifetime state and lifecycle integration;
watcher C API docs and focused existing watcher/interpreter tests. This is
a separate review/implementation from frozen Queue2b registration and
destruction. It is not permission to activate completion graphs, retire
metadata or run a probe.
