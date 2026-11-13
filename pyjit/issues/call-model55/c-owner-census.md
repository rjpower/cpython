# C owner census for M3

Source: selected54 plus integrated typed pins and reviewed M2, inspected from
`BASE/worktrees/call55-gc`. This is a source inventory, not a closed root gate or
runtime pass. [Helper proposal](helper-roots-proposal.md) owns the next bounded
interfaces. NULL homes and conservative native/arena roots remain.

## Existing scalar records

There are 16 `_PyCValue` records in ten functions: 13 normal pops and three
steal pops. `PushCValueNew` has no source caller outside its own helper. Current
push only writes NULL; normal pop closes before a no-op clear; steal returns
without clearing. The type contains only `ref`. `c_stack_refs` exists only in
FT state, whose collector nevertheless already expects each record's `next`.
Neither configuration has a complete working precise registration interface.

| File / function | Records | Callback and outgoing-owner obligations |
| --- | --- | --- |
| `Objects/call.c:callmethod` | self, method | Descriptor lookup and argument building/call; raw object result survives built-args, borrow-scope, method/self and numeric-box cleanup. |
| `PyObject_VectorcallMethod` | self, method | Method/provider call; raw result survives method/self/box close. |
| `_PyValue_VectorcallMethod` | method | Value call; outgoing result survives method close. |
| `PyObject_CallMethodObjArgs` | self, method | Native variable call; raw result survives method/self/box close. |
| `Objects/object.c:_PyObject_GenericGetAttrWithDictValue` | descriptor | Result `res` survives descriptor, name and type releases. Descriptor itself crosses user getter callbacks. |
| `_PyObject_GenericSetAttrWithDictValue` | descriptor | Descriptor crosses setter callbacks; owned dictionary/name/type pins also need roots through cleanup. |
| `Objects/typeobject.c:has_custom_mro` | two lookup values | Lookup and close; lower record must remain visible while top close runs. Result is an integer status. |
| `find_name_in_mro` | MRO tuple | Non-string dict keys can call Python/change MRO. Output `*out` survives final MRO close. |
| `_Py_type_getattro_impl` | result, meta attribute, attribute | Existing bottom result record already survives upper-record and diagnostic/type/name cleanup. Typed diagnostic and type/name owners remain separate obligations. |
| `Objects/abstract.c:call_special_method` | method | Independent `self` owner and outgoing `*res` survive callback/cleanup outside the one existing record. |

All existing scopes pop in LIFO order. A real linked implementation can cover
these explicit fields without a new INCREF: link initialized storage before
callbacks, unlink and NULL before close/steal. Counted scalar records are
external roots, never additional container subtraction edges or FT deferred
references. Existing type getattro is a useful complete first result consumer;
linking existing declarations alone does not close the other output gaps.

Raw object results can be exact numeric boxes and must retain their object ABI.
Use typed object owner records rather than canonicalizing merely to register a
root. `_PyValueTemps` contains only exact int/float boxes: it cannot hold general
detached owners. These boxes have no outgoing GC edges, but remain live leaf
objects for a tracing verifier; either enumerate the numeric workspace as typed
leaf roots or retain an explicit conservative-native policy for them. No-GC-edge
does not itself establish precise root coverage.

## Generated transfer sites

A textual census of `Python/bytecodes.c` finds 53 TAKE and 13 OWNER occurrences.
These are source sites, not dynamic counts or proof that every generated copy
has a root recipe. Existing `IREmitter` tracks consumed/spent owners and rejects
an escaping statement between OWNER and published retirement. TAKE can detach
an independent owner before a provider runs; a cleanup-only root is too late.

| Family / operations with explicit TAKE or OWNER | Required first authority |
| --- | --- |
| Thirteen OWNER receiver sites: six direct LOAD_ATTR forms, five STORE_ATTR forms and two nondescription LOAD_ATTR forms | No callback before generated `IR_RETIRE_PUBLISHED*`; the reached outlined retirement must root remaining receiver/displaced/result owners. No record per consume is needed. |
| `_BINARY_OP_SUBSCR_DICT`, `_STORE_SUBSCR`, `_STORE_SUBSCR_DICT`, `_CALL_INTRINSIC_2`, `_STORE_ATTR`, `_LOAD_ATTR`, `_BINARY_OP` | Held operands are detached before provider callbacks. Register in an outlined operation/provider boundary before the provider, then retain pending local/discard results through operand cleanup and refresh. |
| `LOAD_FROM_DICT_OR_GLOBALS`, `LOAD_FROM_DICT_OR_DEREF`, `CHECK_EG_MATCH`, `CHECK_EXC_MATCH`, `FORMAT_SIMPLE` | Review each helper's consuming/borrowing/error contract; dictionary/namespace, exception/type or pending result owners cross actual Python/error paths. |
| `LIST_APPEND`, `SET_ADD`, `_STORE_SUBSCR_LIST_INT`, `MAP_ADD`, `_CALL_LIST_APPEND`, `SET_FUNCTION_ATTRIBUTE` | Ownership transfers into a concrete container/function or consuming helper. Prove the actual no-callback commit and helper failure cleanup; do not continue marking a transferred owner as a C edge. |
| `_BINARY_OP_SUBSCR_INIT_CALL`, `_LOAD_ATTR_PROPERTY_FRAME`, `_LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_FRAME` | Owners enter staged child homes; child publication/parent partition must finish before callbacks. No record is required for the callback-free handoff itself. |
| `_LOAD_ATTR_CLASSMETHOD`, three method-self LOAD_ATTR forms | TAKE builds a result/receiver pair; every branch must define callable plus self-or-NULL and publish before callbacks, or root detached pair members. |
| `INSTRUMENTED_POP_ITER`, `END_SEND`, `INSTRUMENTED_END_SEND`, `INTERPRETER_EXIT`, `_MAKE_HEAP_SAFE`, `_RETURN_VALUE_EVENT`, four JUMP_IF_*_OR_POP forms | Monitoring, suspension/completion, outgoing result, saved exception and deferred branch owners have separate semantic exits. New M2 return records cover retirement, not earlier monitoring or all these terminal paths. |

The source emitter's `take_view` distinguishes a mapped local from detached
scratch: TAKE duplicates a local or moves scratch; OWNER is NULL for a local.
Borrowed locals/PRE homes can remain authority only while the callback contract
keeps their views valid. Rebinding after callbacks already invalidates cached
views. The new ledger must use these semantic facts, not spelling or future-read
liveness. Unknown callback ownership blocks family admission before M4.

## Outlined native and operation boundaries

`finish_value_bridges_ex` receives `owners[]` plus a separately retained
`failed_result`; its early interpreter-owner branch also closes that array.
`retire_published_values` and `finish_displaced_local` already own outlined
workspaces and can use the real prefix descriptor plus result root without a
handler-sized automatic bank. `value_reader_complete` and
`value_attribute_complete` publish scratch results but keep local/discard results
in C while closing inputs. Their invalid-frame/output error paths close several
owners in sequence and need the same precise continuation coverage.

Earlier `_BINARY_OP`/LOAD_ATTR/STORE_SUBSCR helpers hold detached owners during
the provider itself. Do not mistake rooting their completion helper for full
provider coverage. `value_operation_refresh` may consume a failed result; its
map/code checks are cold revalidation, not an owner for that result.

`_PyNativeScope` owns receiver, receiver_object, argument_box, callable, staged
result and displaced result, while `argument_objects` can contain a borrowed
mapped input. Its state machine empties value authority on publication and must
keep displaced/pending owners rooted through End. Raw numeric boxes stay typed
objects; registering their aliases as extra owners is incorrect. Begin failure,
abort, invalid C result and End cleanup need explicit exit recipes.

Call transactions additionally hold code/function/argument/name/tuple/kwargs
pins and initialized copied/retired arrays. Keep the caller window mapped until
actual detach and publish a fully bound child before replacing that authority.
Dynamic unpack's outer adapter must retain source, initialized output prefix,
star list and partial results through iterator/source cleanup and output handoff;
an inner helper-only prefix leaves a gap. Saved exceptions fetched from thread
state need their own typed roots across any callback before restoration.

The full generated callback ledger and all helper-specific exits remain open.
This census authorizes no stale-home read, removed scanner, permanent fallback
or claim that M3 is complete.
