# PickleBuffer/SRE sealed-consumer source review

Verdict: **SOURCE_CLEAR**, limited to
`6275905c8c06871299bdfffa6cd48b4fb07d725d` against
`87b6d9c3ccbe790770dbba2a2435cd0ea83c5683`. Independent source review of all six
changed paths in `boil-buffer-sealed-consumers`; no owner-tree edits or native
execution. This is not qualification or a blanket buffer-release proof.
Unpickler was separately authored by this reviewer and is outside this verdict.

## Runtime proof checked

`Objects/picklebufobject.c:15` centralizes every explicit C/Python release,
clear, finalize and deallocation path. The releasing flag is initialized by
both constructors before acquisition, blocks recursive release, and makes
GetBuffer/raw/bf_getbuffer report the existing released error throughout the
callback. The original embedded acquisition stays at the same address until
PyBuffer_Release returns; then its obj is NULL. The exact non-subclassable type
has no reinitialization, so a consumed once-only finalizer cannot later own a
different acquisition. Existing outward acquisitions remain a separate path.

`Modules/_sre/sre.c:3044` sets start=NULL before releasing. Both search and
prefixmatch/match enter scanner_begin and test start before state reset,
pattern dereference or raw input use (`3097`, `3148`). Recursive __del__ is
inert. An explicit __del__ during a scan is a no-op under executing; the active
opaque C call roots the scanner and scanner_traverse visits its pattern,
string, exporter and debug exception. This preserves the existing executing
reentry error. The scanner is immutable, non-subclassable and has no public
constructor/reinit path.

`state_init` zeroes state before fallible mark allocation and acquisition
(`Modules/_sre/sre.c:685`). Its error path consumes any acquired buffer.
Even though buf may retain a non-NULL old address, obj is NULL; later finalizer
release is inert and then clears buf before state_fini. Scratch marks, repeat
pool, stack and string still have their existing deallocation owner. No new
borrowed-pointer access was introduced into matching or Match construction.

The base's finalizer transport queues FINALIZE before DESTROY and roots the
queued closure (`Python/gc_live.c:930`). These sealed holders can therefore
release memoryview registrations before the existing readiness-controlled
memoryview/managed-buffer DESTROY phase. Clear/dealloc fallback is idempotent
after that release. This does not prove arbitrary C release callbacks,
free-threading, shutdown delivery or universal progress through all resource
cycles.

## Tests and fixture changes

The five new test_gc methods exercise cycle release, explicit reentry/errors,
scanner constructor failure and the debug executing boundary. Their assertions
outside the callbacks check actual delivery and reported errors, rather than
crediting an ignored callback assertion. The proposed scanner negative keeps
backing storage globally live and removes only pre-release exhaustion, so its
named observation failure does not depend on a stale-memory crash.

The existing test_re retention test preserves its traversal/data assertions
and now requires one Python release callback. Its junk exporters never acquire
buffers, so they do not add release events. The retention/OOM fixture migration
to an unexported, untransformed ndarray preserves the intended unready-holder
case; it must be revisited when ndarray is ported. It is not a general ndarray
ownership test.

Host-only checks: both changed Python modules and all five new literal child
programs parse; the exact diff passes whitespace checks. The serving tree
remained clean at the reviewed commit. No tests/builds/probes were executed.

## Nonblocking inherited limitation

Live PickleBuffer re-export of a Python-defined buffer is not established by
this patch. `picklebuf_getbuf` (`Objects/picklebufobject.c:143`) still forwards
to view.obj. For `slot_bf_getbuffer` this is `_buffer_wrapper`
(`Objects/typeobject.c:11449`), whose buffer table has releasebuffer but no
getbuffer (`11390`). Thus raw()/memoryview(PickleBuffer) can reject that live
path even though direct acquisition/release works. The same forwarding code
exists in87b6; this is not a regression or a reason to expand this finalization
patch. Keep native-exporter re-export coverage and this Python-wrapper limit
distinct in the handoff.

Native callback/constructor/actual-kill coverage and coherent union gates remain
due. SOURCE_CLEAR grants no native launch.
