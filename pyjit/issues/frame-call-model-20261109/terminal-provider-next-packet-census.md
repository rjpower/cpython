# Remaining observer provider census

Read-only source census after the first terminal-publication packet; no new
runtime admission. The first packet certifies FOR body, OR_POP and consuming or
fused conditionals. It does not certify the providers below by resemblance.

| Provider | Actual work after observation | Required next contract |
| --- | --- | --- |
| `_YIELD_VALUE_EVENT + _MAKE_HEAP_SAFE + _YIELD_VALUE` | canonicalizes the yielded owner, switches current frame/exception link, publishes suspension and transfers the result | A pre-suspension observation with an owned callback payload; successful cancellation must leave the generator running. A completed suspension is a different state and cannot reuse this authority. |
| `INSTRUMENTED_END_ASYNC_FOR` | `_END_ASYNC_FOR` matches the exception, retires inputs or reraises | One complete semantic outcome with NEXT versus UNWIND. Actual owner site is END_ASYNC_FOR; reported SEND branch source does not select ownership or EH. |
| `INSTRUMENTED_END_SEND` | assigns held result, marks cursor dead, closes receiver | Move result/window publication before branch observation, using the same explicit terminal commit convention. STOP remains a separate PRE callback; its failure must release the held result with the callback exception preserved. |
| `INSTRUMENTED_POP_ITER` | closes iterator after a branch callback | Detach the input owner, commit continuation, then retire the held owner. No cached destination may overwrite a close-time redirect. |
| `INSTRUMENTED_END_FOR` | consumes/closes frame-owned value after STOP | Error initially retains the frame input. Successful redirect must cancel that old consume; unchanged execution must consume exactly once. Do not infer a C-owner leak merely from the callback preceding a close. |
| `INSTRUMENTED_JUMP_FORWARD/BACKWARD` | ordinary jump dispatch; backward includes prior periodic check | Owner-neutral outcome can share the scoped observer adapter, but requires its own terminality proof. No blanket marker rewrite. |

The canonical compiler currently elides paired END_FOR/END_SEND completions.
Their direct handlers still exist and public code construction can select valid
semantic words; either migrate their complete contracts or explicitly reject
unsupported encoded programs. Dead frontend shape alone is not a runtime proof.

The legacy direct `monitor_stop_iteration` helper still uses
`PyErr_SetObject(PyExc_StopIteration, value)`. Unlike the corrected common
completion path, this does not preserve tuple or returned-exception payload
identity. Its migration must use the shared generator StopIteration constructor
and preserve the held owner/error contract, rather than advertise modern STOP
correctness from the common path alone.

## First next packet

Complete the stopped-state view for already certified terminal edges before
widening to suspension. The generated fact is authority for physical outcome,
not authority to restore caught-exception or saved-local context. Preserve the
current PRE-only restoration gate until each additional recipe is reviewed.

Then migrate POP_ITER and whole END_ASYNC_FOR outcomes as separate positive
providers. END_ASYNC_FOR needs a typed terminal action retaining an exception
owner through UNWIND; a generic destination pointer would conceal this state.
YIELD follows only after exact running-versus-suspended frame ownership and
resume-recipient obligations are represented.

## Shared checks for every migration

Capture old local owners before installing replacements in a callback-free
publication block. Publish destination/window and real continuation before
retirement. Close deferred owners with the active exception saved. Never perform
old slot stores or cached-PC restoration after a callback or retirement.

The scope must match actual observed frame, event, generation and outcome.
Unknown nested hooks mask outer authority. Restore callback scope before closing
its arbitrary result; separately audit payload-copy closes, whose finalizers can
reenter. A stale scope must derive a new committed landing through a positive
recipe or refuse; it cannot reuse an old outcome for the new PC.

Tests should cover actual provider selection, normal/error ownership, callback
redirect refusal or approved application, last-owner reentry, modern/legacy tool
ordering and eventual collectability. They must not require historical CPython
decref locations or invent instruction events for eliminated bookkeeping.
