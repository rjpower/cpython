# Frame arena reservation and retirement census

Coordinator source audit against `6ef45e391aa` plus the pending semantic
completion diff. This supports the reviewed frontier proposal; it is not a
native validation result. Recheck these paths against the implementation.

| Path | Current storage/owner behavior | Frontier implementation obligation |
| --- | --- | --- |
| `_PyThreadState_PushFrameUnchecked` | Reserves separate header/value ranges; caller initializes values | Establish the initialized range before writing the header; cover every direct caller |
| `push_chunk` | Fresh/custom allocation or cached chunk; records previous chunk tops | Initialize only the uncertain suffix; never assume allocator zero fill or reset a cached frontier upward |
| `_PyFrame_PushOverlapping` | Transfers argument homes in place; caller's remaining scratch is NULL; initializes extension beyond caller's physical end | Use the chunk guarantee instead of recomputing the caller end; retain the overlap ownership assertion |
| `_PyFrame_PushUnchecked` | Property/getitem/exact-call fallback reserves then installs arguments | Consume initialized storage; all supplied arguments are installed before frame publication or a callback |
| `eval_frame_push_and_init_code` | Generic/external entry reserves, initializes, then binds locals | Use the same initialized reservation; on binding failure `clear_thread_frame` clears partial locals while scratch is still NULL |
| `_PyFrame_PushTrampolineUnchecked` | Reserves thread storage, currently fills its entire value range | Consume the common reservation guarantee; retain ordinary result ownership and explicit continuation |
| `Objects/frameobject.c:init_frame` | Heap-owned frame calls `_PyFrame_Initialize` directly | Explicitly initialize owned value storage once before the shared header/constant initializer |
| `_PyFrame_Copy` | Writes every destination home and NULLs every source home | No destination prefill; transferred source range is reusable only after the normal pop |
| `RETURN_GENERATOR` | Allocates generator, moves through `_PyFrame_Copy`, then directly pops thread frame | Preserve complete source clearing before pop, including initial input/result phases |
| Escaping frame `take_ownership` | Moves all homes into frame-object storage; gives source another executable owner | Complete move establishes NULL source cells; no early reuse before executable/header retirement |
| Ordinary thread return | Generated terminal transfer consumes return source; `_PyFrame_ClearExceptCode` asserts scratch NULL and clears locals/constants | Do not replace the scratch invariant with an unchecked assumption; retain full debug verification |
| Exception unwind without handler | `_PyFrame_ClearTemporaries` clears all scratch before unlink/clear/pop | Preserve full physical scratch coverage on every error exit |
| Exception with handler | Clears scratch suffix above surviving handler anchors and installs exception/lasti owners | This is an active frame, not reusable inactive arena storage |
| `clear_thread_frame` | Clears/moves homes and executable before `_PyThreadState_PopFrame` | Keep the departing header and values reserved throughout all decref callbacks |
| Init-shim preparation failure | Clears shim temporaries, then clears/pops frame | Route reservation through the same guarantee; retain failure-side cleanup |
| Generator-owned clear | Clears heap-owned homes and generator exception links; no thread-arena pop | Do not alter heap lifetime rules or apply chunk frontier arithmetic |
| `_PyThreadState_PopChunk` | Restores previous chunk pointers and caches retired chunk | Preserve each chunk's own current frontier; never restore an outer saved frontier after re-entry |

`clear_thread_frame` retains `datastack_limit`/`datastack_top` until
`_PyFrame_ClearExceptCode` and executable retirement finish. Although execution
has unlinked the dying frame, reentrant calls therefore reserve beyond its
still-owned storage. The frontier must follow this physical reservation, not
`current_frame` visibility. Nested calls can invalidate or extend it, so an
outer pop must not restore a saved frontier.

The remaining implementation review must inspect all direct pop/initializer
callers, strict header/value separation and chunk allocation overhead, including
the spare `owned_slots` word. Dirty allocator, former-header reuse, heap frame,
binding failure, recursion, generator move/escape and cleanup re-entry require
native coverage. Exported remote chunk offsets must land before layout changes.
