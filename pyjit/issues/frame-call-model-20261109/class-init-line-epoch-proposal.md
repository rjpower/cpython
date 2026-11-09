# LINE epoch follows LINE delivery configuration

Read-only source proposal during native25. Original fixture receipts reproduce
candidate base PASS/opcode FAIL and upstream both PASS. The faithful probe uses
the actual inherited test and original Tracer, with captures outside callbacks.

TraceOpcodesTestCase's Tracer sets f_trace_opcodes=True on EVERY event. At CALL,
f_trace is initially NULL, so the setter stores the flag without enabling local
INSTRUCTION. At the first source-only init body LINE, f_trace is installed; the
same setter now enables INSTRUCTION. force_instrument_lock_held unconditionally
bumps line_epoch, even though LINE tools and source masks are unchanged. The LINE
wrapper deliberately treats the resulting stale epoch as a fresh landing LINE.
The receipt shows exactly two identical LINEs before the first opcode. The old
minimal probe set the flag only at CALL, so it did not exercise this transition.

Proposed correction: before any wrapper mutation, derive the effective new LINE
mask from the same local/global union used by force_instrument. Increment the
cold line_epoch only if the effective LINE mask changes, or restart re-enables
an active LINE configuration. Preserve overflow-before-effects. Adding/removing
INSTRUCTION alone leaves the LINE epoch unchanged. Existing source_line_tools
already preserve unchanged per-record LINE masks at the force done block.

This retains new LINE events after LINE off/on and restart while preventing an
unrelated opcode-monitor configuration from invalidating already delivered LINE.
It adds no per-opcode work or public cache state. Callback replacement semantics
remain unchanged; this is not a new general observer version domain.

Gates: original inherited cold/warm class-init fixture unchanged; source callback
LINE off/on/local/global/restart matrix unchanged; source epoch last-callback and
multiple-event regressions; mixed LINE+INSTRUCTION with DISABLE/restart; same-PC
redirect/error source tests. B peer/root admission required before source edits.

## Tool identity refinement required before implementation

Independent peer found that equal masks are insufficient after tool-ID reuse.
update_instrumentation_data clears stale local bits when code tool_versions
differs from interpreter versions. Global LINE may retain the same bit for a
new tool identity. The current code only synchronizes code tool_versions in
SetLocalEvents, so treating its global mismatch as new on every force would
reproduce the same over-invalidation problem.

Two cold options need coordinator review: record last successful tool identities
in existing tool_versions after stale-local clearing and successful force, or
add a LINE-only identity snapshot. The first reuses a field whose only current
read is stale-local invalidation; all usages must be audited and documented
before changing its scope. The second adds cold code-monitoring storage rather
than any hot frame state. Either must invalidate actual LINE identity changes
and restart before old wrapper mutation, preserve failure/overflow semantics,
and leave unrelated INSTRUCTION-only configuration unchanged. No implementation
is admitted by the initial mask-only sketch.

## Simpler existing-version alternative (not implemented)

co_instrumentation_version records the global version of the last successful
force. Monitoring tool identities are assigned monotonically newer global
versions on clear/reuse, with overflow refused. For relevant active/new LINE
bits, an interpreter tool identity newer than that code version therefore
identifies reuse not yet installed in this code, even if SetLocalEvents already
overwrote its local tool_versions field. A code refreshed during tool clear has
already removed stale local LINE bits; a subsequent new LINE request then
changes the ordinary effective mask. This can avoid repurposing local-version
semantics, adding a snapshot field or adding refreshed-tool arguments.

Before mutation, derive effective new LINE union with the same stale-local
filter as update_instrumentation_data; include actual LINE mask changes, relevant
newer tool identity and restart of active LINE. Only then bump the epoch with
overflow-before-effects. Native original fixture and tool reuse/local/global/
restart tests must validate this proof before admission.

Callback replacement is separately unversioned today: RegisterCallback swaps
the callable table without changing global/tool/code versions or forcing
wrappers. Existing line_epoch never detected a same-tool callback replacement.
Do not claim that narrowing force invalidation preserves a capability it never
provided. Exact replacement invalidation needs a separately reviewed contract
and revision mechanism if required; no such source change is in native26.

## Exact cold force plan for independent review

All calculations occur under the existing code lock/world-stop contract before
old source_forced wrappers are removed. No ordinary dispatch change is needed.

```text
old_line = old_monitoring.active_monitors.LINE (or0)
local_line = old_monitoring.local_monitors.LINE (or0)
for each tool:
    if local stored tool_version != interpreter tool_version:
        local_line &= ~toolbit
new_line = interpreter.global_monitors.LINE | local_line
relevant = old_line | new_line
stale = relevant bits whose interpreter tool_version
        > code._co_instrumentation_version
restarted = interpreter.last_restart_version > code._co_instrumentation_version
line_changed = old_line != new_line or stale !=0
               or (restarted and relevant !=0)
if old_monitoring and line_changed:
    reject UINT64_MAX before effects; increment line_epoch
```

The local filtering exactly matches update_instrumentation_data. SetLocalEvents
has already replaced its requested local mask and marked that tool current, so
its new request survives filtering. The separate successful-code-version test
still sees old installed identity; no local-version rewrite or extra argument is
needed. Global versions are monotonic with overflow refusal. A successful force
records the new version at its existing final release store; failure does not
advance that record. Old handoff is already invalidated before any fallible
mutation; an extra epoch bump on retry is safe. First monitoring allocation
initializes its epoch as today, with no earlier handoff to preserve.

In the existing removed/new event calculation, stale LINE identity must be
represented as replacement, even when raw active masks are equal:

```text
removed_events.LINE |= old_line & stale
new_events.LINE     |= new_line & stale
```

This resets actual real-word and source-record DISABLE masks for the new tool
identity through the existing remove/add paths. Mere epoch invalidation would
leave old per-record zeros attached to the new callback, so it is insufficient.
Unchanged identities preserve those masks, including when INSTRUCTION alone is
added/removed. Restart already removes/readds all active masks and remains
independent. The scope is LINE state; other event identity behavior is unchanged.

RegisterCallback replacement alone remains the existing no-automatic-restart
contract: it changes the callback used for subsequent selected events, not
previously delivered events or DISABLE decisions. It does not force code or
create a fresh landing LINE. An explicit restart after replacement re-enables
LINE and invalidates the handoff normally. No new callback revision mechanism
is proposed in this bounded repair.

## Future27 test specification (not source-edited during26 freeze)

1. Keep the original inherited class-init fixture unchanged. Add a direct
   source-pass callback that changes only local INSTRUCTION while LINE remains
   active; the already delivered source line must not repeat at its landing.
2. For global and local LINE separately, disable each target line on first call,
   prove no events on second, free/reuse the ID while target is inactive, install
   a new callback with the same LINE mask, then require every target line again.
   This covers code-version identity proof plus stale per-record masks; local
   setup specifically covers SetLocalEvents overwriting tool_versions first.
3. Replace a LINE callback during a source-pass event without restart: only
   subsequent distinct events use the new callback, with no replay of that pass
   at landing. Separately replace after DISABLE: decisions persist until explicit
   restart, which restores the new callback.
4. Retain existing local/global LINE off/on/restart and last-source-callback
   epoch tests. These must still produce a fresh landing after actual LINE
   reconfiguration, unlike the INSTRUCTION-only case. Keep same-PC redirect
   success/error and multiple source events with a later new-epoch event.

Root/B must admit this exact predicate and mask transition before any source
changes. Native26 contains none of this epoch repair.

## Source27 implementation handoff

Root admitted the exact successful-code-version predicate after independent B
review. The source now applies it only in force_instrument_lock_held, before
wrapper mutation, plus the existing cold removed/new LINE transition. No new
field, callback revision, opcode or ordinary dispatch check was added.

New isolated test_ir_line_epoch.py covers the four future-test categories:
actual source-body INSTRUCTION enable without repeated LINE, global/local ID
reuse after DISABLE, callback replacement without replay, and replacement that
keeps DISABLE until explicit restart. The source body fixture requires a real
IRSM source record. Original inherited class-init and existing epoch/source
regressions remain unchanged. Host AST and diff checks pass; native27 pending.
