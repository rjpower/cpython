# M3 helper, native and binder owners

Status: bounded proposal for peer review, not a complete root migration.
Source inspected: integrated M1 plus the reviewed M2 producer/collector patches.
NULL homes and conservative scans remain required until the full census closes.
Frames lane owns this proposal; root owns interface integration and execution.

## Facts from the current code

`IR_CONSUME_INPUT` moves a scratch owner into `ir_own_<field>` and NULLs its
home; locals remain owned by their homes. `IR_TAKE` transfers that scratch owner
or duplicates a local. The generator already tracks consumed/spent owners,
borrowed object views, stale views after callbacks, output roles, escaping
statements and frame switches. These are the starting facts for external roots;
do not create a record at every consume or assume every input was detached.

The existing `_PyCValue` API is storage, not working precise registration:
`PushCValue` only writes NULL; pop only closes/returns its `ref`; its struct has
no link. The thread has a `c_stack_refs` head and FT code expects a `next` member
that is absent. Reusing this interface requires implementing and auditing the
registration, not claiming that the old calls already root values.

`_PyValueTemps` owns exact int/float boxes. Their release cannot call Python;
they have no GC edges and are not GC containers. Keep their existing bounded
workspace and count discipline. They are not a general owner bank, and raw
numeric boxes must not be mislabeled as canonical PyValue homes. The borrow
stack already enumerates its owned entries and skips borrowed vector entries;
retain that distinction rather than registering all vector words as owners.

| Actual boundary | Current independent owners that cross callbacks | Existing authority that can remain mapped |
| --- | --- | --- |
| `finish_value_bridges_ex`, `retire_published_values`, `finish_displaced_local` | Remaining `owners[]`, displaced locals and `failed_result` | Already published result homes and unaffected inputs |
| `value_reader_complete`, `value_attribute_complete`, operation restore/abort | Detached first/second/receiver; result when destination is local or discard; pair/error results on failure | A committed scratch result or method pair after their callback-free publication |
| `call_transaction_native` | Callable pin, monitoring argument, provider result, delayed local/discard result, displaced owners, detached retired array | Original call window until `call_transaction_detach`; new scratch result after publication |
| Method/EX/monitor preparation | Function/first/args pins, boxed objects, new tuple and obsolete wrapper/args | Parent window until commit; replacements after commit |
| `call_transaction_push_python_fallback`, `eval_frame_push_and_init_ex_code` | Copied initialized args, code/function/locals/names/tuple/kwargs and retired input prefix; partially constructed child | Parent IN_CALL window until detach; fully bound child after publication |
| `_PyNativeScope`, `native_call_builtin_o/finish` | Receiver, staged/displaced result, actual owned object fields, detached argument/callable and pending local result | Borrowed args before detachment; result home after actual publication |
| `_PyEval_UnpackIterableValue` and generated unpack adapter | Sequence/iterator, initialized output subset, starred list/extra step owner during failures | Other PRE homes; committed output map only after complete handoff |
| Generated OWNER/TAKE and observer terminal paths | Actual consumed input owners, unsaved outputs, `ir_displaced`, deferred observer inputs/results and saved exceptions | Local/PRE borrows with verified callback contract; committed POST homes |

This table names paths, not a closed census. The implementation must enumerate
every callback statement and exit, including early errors and C-level adapters.

## Proposed shared interfaces

Make `_PyCValue` an actual linked owner record: `{next, ref}` using the existing
thread `c_stack_refs`. Its field is the owner storage, not a copy of another
live owner. Push initializes an empty field before linking. Pop unlinks and
sets the field NULL before close; steal unlinks/NULLs and transfers the count.
All existing Push/Pop callers must retain LIFO order, including error exits.
The precise GIL external-root visitor and optional FT marker enumerate the
chain. FT deferred adjustment must not add a count for its counted references.

Use a typed `_PyCObject {next, PyObject *ref}` analogue only where an owned raw
object must persist as an object across callbacks. A separate nullable thread
head and typed visitor avoid interpreting exact numeric boxes as canonical
values. Both APIs transfer existing owners, with no extra INCREF to register.
Do not add a generic memory scanner, per-field runtime kind tag or universal
handler prologue. Existing containers/typed pins stay explicit typed roots.
Shared layout/API is subject to peer approval before edits.

For variable arrays, reuse the reviewed transition prefix descriptor. Array
storage and current initialized bounds must outlive callbacks; existing array
allocation stays unchanged. A descriptor may describe a decreasing initialized
suffix: with fixed `end=capacity`, install `base[begin-1]` before decrementing
`begin`, then advance `begin` before retirement. Do not read spare capacity.

`_PyNativeScope` is already addressed in a reached outlined helper: migrate its
actual owner fields to the typed records while preserving borrowed cache fields
and its result state machine. Register only independent owned fields. Publication
moves result authority into its final home or a registered delayed-result field;
the original entry is NULL before another callback. Exact numeric boxes remain
ordinary RC workspaces with no GC edges. Count each distinct aliasing owner in
the shadow census while marking an object idempotently.

## Generated callback contract

Generate a ledger per escaping statement from the existing consumed/spent/role
analysis plus explicit helper transfer effects. At each statement list: mapped
PRE/POST inputs, detached canonical value owners, typed object owners, staged
results, initialized variable outputs and the exceptional successor. A callback
that only borrows still-mapped PRE owners needs no new record. Unknown ownership
at a callback is a generator error; token spelling is not a sufficient proof.

For genuinely detached owners, an outlined operation/retirement adapter receives
them by value and installs typed root fields before the first callback. It owns
cleanup or returns the surviving owner explicitly. Avoid pointers to arbitrary
handler C locals and avoid a large automatic bank in every handler. Existing
outlined bridges and native scope should be extended first. Family-specific
wrappers follow only after their ledger is reviewed; don't combine compiler
allocation changes or permanent dual authority into this work.

Saved exceptions are owners too: `PyErr_GetRaisedException` removes the thread
root until restoration. Every callback between fetch and restore needs its
typed object record. A result returned by a provider must reach a registered
field before instrumentation, operand cleanup, refresh or another provider call.
Clear/transfer record authority before consuming APIs and revalidate mapped
locations after callbacks. No saved pointer to a local result home crosses one.

Dynamic unpack needs an outer outlined adapter, not only an inner iterator
record. Its output prefix remains linked through sequence/iterator cleanup and
through conversion into named locals or the admitted POST map. Popping inside
`_PyEval_UnpackIterableValue` and then closing `seq_o` would leave a gap before
outputs are mapped. Cover insufficient/excess input, iterator exceptions, star
list creation and callback-capable cleanup of every initialized partial result.

## Typed call ownership fact

The M1 `_PyFrame_Initialize` boundary receives a canonical owned function value.
Native review found generic `PyValue_AsKnownObjectSteal` adds classification for
borrowed/deferred cases outside that contract. Add a narrow typed owner-transfer
accessor after auditing every initializer caller: assert canonical home, actual
function type, and counted mortal or immortal policy in debug; return the typed
pointer without a release-build borrowed/deferred acquisition path. The helper
must live with the value representation, where pointer extraction belongs.
No raw borrowed cast appears at frame callers. Code stays independently pinned.

Audit unchecked/overlap pushes, generic/ex binders, `PyFrame_New`, constructor and
provider paths together. A borrowed/deferred mortal input is invalid at this
boundary and must be fixed at its producer, not silently upgraded by a fallback.
Immortal function transfer retains a valid strong pointer whose later DECREF is
immortal-aware. This is part of the common call ownership contract, not an
isolated mask/branch experiment.

## Bounded implementation sequence and ownership

1. Freeze typed C owner APIs and the callback-ledger schema. GC lane ports
   visitors; frames lane ports the existing CValue users and thread lifecycle;
   root owns shared-header integration. Prove exact count transfer and nested
   registration with the precise helper before adding more users.
2. Port outlined bridge/read completion and native transaction/scope owners.
   Frames lane owns ceval/native_scope and bounded object-call users. Use the
   prefix adapter for retired call arrays and preserve delayed result behavior.
3. Port binder preparation/partial failure, EX object arrays and dynamic unpack.
   Record each consume and publication boundary; keep parent/child ownership
   partitions explicit. Add typed function transfer with its full caller audit.
4. IR lane and root generate callback ledgers/wrappers and enumerate all remaining
   OWNER/TAKE, observer, exception and frame-switch cases. Frames/GC peer each
   family. Only a complete matrix, shadow unique-owner checks and all lifecycle
   readers can close M3 and permit the coherent M4 representation switch.

Files include `pycore_{structs,value,tstate,interpframe,native_scope}.h`,
`Python/{ceval,native_scope,pyvalue,borrow,pystate,gc_trace,gc_free_threading}.c`,
`Objects/{call,object,typeobject,abstract}.c`, bytecodes, IR generator/cases and
focused native/helper/binder tests. Freeze nonoverlapping file ownership before
parallel edits; request frame.c and generator changes through their owners.

Root executes precise-ID fixtures for nested descriptors, borrowed PRE inputs,
owned provider results, C_RETURN/monitor errors, caller-local mutation, native
scope abort/End, binder failure at each initialized item, dynamic unpack failures
at every prefix, saved exceptions, recursion/GC/escape and direct C API callers.
Use an external-root helper that enumerates transition plus both typed C chains;
existing retirement-only helper remains accurately scoped. Verify actual owner
counts and logical values, not just nonNULL words or object survival.

Source/native cost ledger must include linked scalar record stores, array bounds,
new typed-object head, workspace growth, wrapper call ABI, register spills and
all added callback publication. Exact/inline mapped paths should gain no large
record or allocation. No removed scratch clear/scan or speedup is claimed until
M4; a complete physical-bitmap rival must use the same call/boundness policies.
