# Typed pins independent post-review

Disposition: **GO at source level** for M1a
`dd3c9fc8d9e`, compared with selected54
`61015a9452d9069646fb5fede9ff963896833b89`. Reviewed all 23 changed files and
the remaining code/function field census. No must-fix finding remains.
This review ran no candidate tests, builds, generators or native jobs.

The two header fields now have typed independent strong ownership. Entry
acquires the code owner independently of mutable function code, and consumes
the callable owner at the existing value-call boundary. Direct accessors add
no image owner, side allocation or new hot cache dependency. Field order,
reservation geometry, argument overlap and initialized/NULL homes remain the
same. Header-size and native instruction conclusions still require root evidence.

Frame copy transfers both pins with the value homes. Escaped-frame takeover
retains the required separate source code count until the source caller clears
it. Ordinary clear drops the function once and keeps code through local cleanup;
completed generators retain code through their public code property and release
it at deallocation. Half-built generators start cleared with NULL code, so
allocation failure traversal/deallocation remains bounded. The selected54
public-constructor ordering is not changed by this patch.

Heap traversal visits each typed pin as an object edge. The FT deferred-reference
adjustment no longer interprets the counted code pin as a value-tagged deferred
edge; optional stack marking visits both code and function explicitly. GIL
conservative scans and NULL-home readers remain. Frame-object deallocation still
uses its existing release order; this is safe under M1 NULL authority, but code
must remain fixed-last once M2/M3 readers depend on code-owned owner maps.

Interpreter/base sentinels carry NULL pins with explicit interpreter ownership;
code-backed trampolines retain code and no function. Remote readers keep the
high-pointer rejection before recognizing a sentinel, then recognize owner before
rejecting NULL code. Direct pointer reads replace tag stripping. Safe traceback
access retains freed-pointer checks, and GDB recognizes shim frames before code
decoding. The cold unstable GetCode API now explicitly returns None for NULL
sentinel code, preserving that behavior; its internal regression checks the case.

Generator source and generated cases agree on the accessor and NULL comparisons.
The analyzer's added `_PyFrame_GetFunction` nonescaping classification matches
the accessor's assertion/direct-load implementation. The exact
`canonical_frame_transfer_input` source template changed only its typed code
sentinel comparison. The generated diff adds no transfer, spill, callback or
publication machinery; metadata/handler numbering remains unchanged.

The revised tests exercise mortal/immortal code counts, active code replacement,
escaped independent pins, completed-generator code retention and sentinel API
behavior. The escaped-frame test now checks the existing frame lifetime policy:
both pins remain until the frame object is freed. Root reports 932 focused tests
passing per GIL role. Native work, actual header geometry, timing and any wider
qualification are coordinator evidence, not established by this source review.
No FT execution qualification or scratch-map authority is claimed.

## Existing native evidence inspected

Read root-produced `E/m1-native-{frame54-cold-construction,call55-integrated}-`
`{_PyFrame_ClearExceptCode,_TAIL_CALL_CALL_PY_EXACT_ARGS,`
`_TAIL_CALL_RETURN_VALUE}.txt` and header logs; no native tool was run here.
Both role headers report owned-slots offset 112 and function offset 32.

The release clear path has a concrete metadata saving: selected54 loads code,
shifts by 48, branches to the fatal-object path and masks bit zero before
reading code geometry (`3cf848`–`3cf860`). M1a reads geometry directly from
the code pointer (`3ceb3e`–`3ceb59`). Escaped-frame copy and frame-back traversal
likewise remove code tag decoding. Local NULL stores and value closes remain.
Function-pin close changes from tag-mask classification to object-refcount
load/sign classification; its RC branch/load pattern is different, not free.

Exact-call construction also adds work at the callable conversion boundary.
M1a tests the callable owner against the value count mask (`379853`–`379864`)
before storing the typed function pin; its non-counted route loads/checks
the object's refcount (`379958`–`37996f`). Code-pin acquisition uses a refcount
load and `0xbfffffff` comparison (`37986d`–`37987e`) instead of selected54's
sign test plus value-mask/immortal-tag construction (`37a5ff`–`37a62a`).
The initial callable type/tag guards, overlap, capacity checks and argument
transfer remain. The child RESUME check drops a second code tag decode.
These are real exchanged costs; the type change does not establish a net win.

RETURN_VALUE's shown release instruction/control sequence is unchanged after
address relocation, including the 16-byte stack reservation, result transfer,
frame-clear call, recipient publication and displaced-value retirement. Its
called cleanup now reaches the changed pin operations described above.

Root's provisional two-pair hot screen reports selected54 ratios 0.9921
(DeltaBlue), 1.0847 (generators; 0.9877–1.1912), 1.0454 (coroutines; both pairs
lose), 0.9629 (Richards), 0.9652 (comprehensions) and 1.1835 (logging;
1.0945–1.2797). The wide ranges and separate confirmation gate remain. These
measurements do not identify which instruction change causes any timing shift;
they do not select M1a as a new golden checkpoint or establish a speedup.
