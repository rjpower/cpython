# Observer retirement prefix: next bounded adapter

Status: coordinator and [frames peer](observer-prefix-peer.md) approved;
implemented in `BASE/worktrees/call55-gc` after component `530f26f084e`.
Source post-review and merged candidate execution remain pending. NULL frame
homes and conservative roots remain unchanged.

`Objects/frameobject.c:frame_lineno_set_impl` already detaches surplus scratch
and displaced local owners into a `PyMem` array, commits the continuation, then
closes that array. Nested callbacks may redirect or mutate the frame. The array
is not a heap container and its detached owners need precise external roots;
native/PyMem conservative discovery is current fallback. The array's existing
allocation and frame-owner commit are preserved.

Use the first actual dynamic-prefix adapter to extend transition roots with an
optional pointer to an outlined adapter-owned prefix descriptor:

```c
typedef struct {
    PyValue *base;
    int capacity;
    int begin;
    int end;
} _PyTransitionPrefix;
/* _PyTransitionRoots gains: _PyTransitionPrefix *prefix; */
```

Only `[begin,end)` is initialized/authorized; `0 <= begin <= end <= capacity`.
The prefix record lives in the reached noinline continuation, with capacity/end
fixed to the already detached `nretired`. It is separate from the eight-root
bounded dead-owner batch in the target design. This pointer adds one word to
other reached slow records; no ordinary instruction initializes a large record.
An inline descriptor would save a cold lookup but enlarge every retirement
record by three integers and a pointer; root should inspect that tradeoff.

The current loop remains callback-free until the first `_PyValue_MayDealloc`
owner (all mortal owners route slow under FT). It then calls the outlined
continuation with array/count/next index and current frame. The continuation
links a transition record with no frame-home/header/result authority and the
detached prefix, advances begin before each close, and pops before freeing the
array. Closed entries may be set NULL for development redundancy. The active
frame remains independently traced through its normal authority; prefix-only
walking must never read its geometry or stale old bindings after nested writes.

The precise visitor enumerates this prefix, including inline/immortal/nullable
initialized values correctly. It propagates callback failure for diagnostics.
Push/pop validate prefix bounds/exhaustion. Existing producers' designated
initialization leaves prefix NULL. The prefix is external marking authority;
cyclic subtraction must never visit its owners as extra frame/container edges.

Files: GC lane owns transition header/visitor and `Objects/frameobject.c`;
root integrates shared header changes and executes focused tests. Tests belong
with the existing register line-jump ownership tests, using the precise-ID helper
introduced by the first M2 bridge. One candidate fixture builds an expression
with temporary Trigger and Payload values on separate source lines, jumps to a
valid line after the expression before construction completes, and verifies
Trigger's finalizer sees Payload in precise prefix roots while it collects.
Also cover nested redirect/mutation, no reentrant closes, and prefix exhaustion.
Fixtures must use admitted observer recipes; do not weaken jump validation or
disable the known debugger failures to make a new fixture pass.

This adapter does not claim dynamic unpack coverage. Later unpack production
can use the same descriptor with end advanced only after an initialized item is
installed and a validated capacity/window. Stale scratch and M4 selection still
require all active/error/suspended maps and the complete producer/reader census.

Implemented files: transition header, `Python/gc_trace.c`,
`Objects/frameobject.c`, and `Lib/test/test_sys_settrace.py`. The actual line-jump
fixture abandons Trigger/Payload expression inputs and checks Payload's precise
ID during Trigger collection, with and without mutation of the active named
local. It checks empty roots afterwards. `git diff --check` and host AST parse
pass; no candidate compile/test/native evidence is claimed by this lane.
