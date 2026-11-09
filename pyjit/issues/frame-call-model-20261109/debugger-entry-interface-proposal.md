# Explicit synchronous debugger entry

Implementation admitted by root and independent peer; native qualification is
pending. The matched ordinary Bdb probe confirms candidate
next-line LINE stop versus upstream call-line opcode stop. The public requirement
is immediate debugger entry; no removed instruction/event will be reconstructed.
B peer selected synchronous entry over an ordinary return hook.

## Private runtime interface

Add a private sys builtin `_debugger_entry(frame, callback)` used by Bdb. It
invokes callback(frame) under a distinct internal debugger-entry observer phase.
It is not a public sys.monitoring event, LINE, INSTRUCTION or PY_RESUME. No tool
mask, code metadata, ordinary opcode/return branch or hot frame field is added.

Before callbacks, validate the target is an active interpreter-frame ancestor
of current_frame, complete and not suspended/cleared. Hold the supplied
frameobject and callable normally. Capture current_frame as the actual observer
child, then save/restore existing observed_frame, what_event and tracing state
around callback invocation. Increase the existing recursion-suppression count;
never trace the debugger's own interaction recursively. Restore on success and
all errors, including callback failure. No pending source-error recipe or
monitoring line_epoch changes merely because entry was requested.

Use one private what_event discriminator outside the public event set. Add that
case to the setter's explicit event gate; no bitmask/index uses this value.
Set observed_frame to the captured active child, **not** to the target. This
retains the actual ancestry and overlap phase. The callback's target is explicit,
while the child chain permits the existing deferred-call proof to find its
immediate child. A future dedicated phase enum can replace this private marker
if needed; this increment does not add another thread field.

## Owner and continuation contract

Entry displays the target's current actual call-site position (or its existing
valid source cursor), while its return_pc remains the actual pending
continuation. Preserve all caller/child scratch and locals. Entry alone does
not detach, reconstruct or retire owners and does not increment redirect intent.

A command assigning f_lineno is separately validated. The existing deferred
active-child recipe accepts an ignored ordinary VM CALL only, keeps the overlap
untouched, and completes retirement/redirect after child exit. Used-result
calls, external child phases, FOR/SEND and protected/suspended transfers keep
their existing refusal until their own recipes are proved. Entry does not grant
an owner recipe just because displaying the frame is safe. A successful command
uses the existing generation counter; callback reentry must not overwrite it.

An explicit frame argument may name an older active ancestor, but its actual
child chain must still match. A frame outside that chain is rejected. Do not
pretend a target is a real-LINE instruction boundary to bypass these checks.

## Bdb integration

Bdb.set_trace keeps its frame-trace installation and backend activation, but
replaces the initial set_stepinstr request with a dedicated entry dispatch.
Keep the selected entry frame before the existing loop walks f_back. After
start_trace, call the private builtin with the selected frame and Bdb's entry
dispatch method. That method enters set_enterframe(frame), invokes a dedicated
user-entry hook and applies ordinary quitting/restart handling. It emits no
synthetic trace event. For callback compatibility the default user-entry hook
can delegate to the existing user_opcode interaction hook; that delegation must
not claim that an INSTRUCTION event occurred. Pdb's existing user_opcode alias
already selects its ordinary interaction.

Step/next/continue issued during entry replace the initial stop state; no
opcode-stop flag remains armed to cause a second stop. Do not fabricate a
command/source line to suppress the next real LINE. On entry-dispatch error,
Bdb must stop its newly installed tracing before reraising, because this is an
explicit call rather than a trace callback whose VM error path automatically
disables tracing. The C helper still restores its observer context independently.

## Files and review boundaries

A: Python/sysmodule.c private builtin and existing internal observer marker
header; Bdb entry dispatch in Lib/bdb.py, focused entry tests; any shared frame
setter gate change coordinated with B. B: setter phase/ancestry peer, pending
recipe interactions, protected-context work. Clinic regeneration and all native
builds/tests belong to root. No generated file is hand edited.

Before implementation, root and B must admit the exact private marker/location,
Bdb error cleanup and callback-hook compatibility choice. It is a deliberate
private runtime/stdlib interface, not a runtime performance switch or a second
instruction domain.

## Gates

Keep existing Pdb/doctest/remote-Pdb command transcripts unchanged. Add ordinary
no-interaction Bdb first-stop tests for both backends, including call-site line,
locals before the next assignment, nested step/next/continue and exact one entry.
Cover callback exceptions/quit with restored tracing/observer state; different
existing sys.exception() contexts; f_lineno ignored-call deferred success and
used-result/external/protected refusal without overlap mutation. A private
builtin invalid-frame/ancestry test must fail before callbacks. Then root runs
the affected debugger modules plus existing source/monitoring/reentry tests and
broad qualification. No benefit or full correctness is claimed before receipts.

## Root review refinements required for v1

The target must be a **strict** ancestor of the captured current Python entry
frame. Reject the current frame as well as nonancestors before invoking the
callback. The current frame is executing the native private CALL with C-held or
consumed inputs; equality would incorrectly authorize an ordinary empty/LINE
recipe. Bdb's default caller and older explicit ancestors remain supported.
Used-result ancestor inspection is safe, but its jump remains refused by the
existing deferred recipient gate.

Use `_PY_DEBUGGER_ENTRY_EVENT = _PY_MONITORING_EVENTS` as one-past-public-count
private discriminator. Source audit finds what_event read only by the frame
setter (event gate and real-LINE identity comparison); instrumentation saves,
sets and restores it, and thread initialization uses-1. No array/bitmask reads
use what_event. call_one_instrument indexes its separate explicit event argument,
so the private marker must never be passed to that dispatcher. Add the marker
only to the setter's explicit allowed cases.

Bdb.set_enterframe currently lacks a finally clause. Its shared context manager
must save the previous enterframe, set the selected frame, yield under try and
restore the saved value in finally. This is required for explicit entry quit/
error and preserves nested context rather than leaving enterframe set or merely
stopping tracing. Configure the initial debugger phase before invoking any
user hook. Dedicated entry dispatch delegates to user_opcode for subclass
compatibility without emitting an opcode event; initial opcode-stop flags are
not armed, and commands replace the ordinary stop state.

## Actual source handoff (native25 pending)

The sys builtin uses a typed Clinic frame converter (`PyObject *`) and a native
value callback (`PyValueBorrow`). `_PyValue_CallOneArg` invokes the callback
without an object-call adapter; its owned PyValue result passes through after
the observer/tracing fields are restored. Strict ancestry is checked by walking
the current thread's existing frame chain and comparing frameobject identity
before inspecting the matched ancestor. No caller-supplied interpreter-frame
pointer is followed. Clinic output remains coordinator-owned.

The seven-method ordinary regression suite covers both backends, commands,
callback errors and quit, context-manager nesting, rejected ancestry, pending
ignored-result redirect, used-result refusal and handled-exception inheritance.
Host AST and diff checks pass; this is not a native result.

## Consumer repair proposal after native25

Native25 passes all seven entry methods but exposes three real consumer
assumptions. Doctest redirects stdout only inside trace_dispatch, so dedicated
entry needs the same scope in its entry hook. Pdb async calls synchronous
set_trace and then fabricates opcode callbacks: the new API enters twice. Error
cleanup in Bdb's outer except runs after C restores tracing and can itself stop
in stop_trace. Original command transcripts remain authoritative.

Factor Bdb preparation (stop/reset/install frame trace hooks) from activation
and explicit entry. Public set_trace resolves the caller before the private
prepare helper, then invokes the C transaction. _dispatch_entry starts tracing
inside suppression and stops it on any error before reraising. Validation
failure happens while preparation has left tracing stopped; no outer traced
cleanup is needed.

Async Pdb sets commands/last instance, prepares without synchronous interaction,
and initializes async fields before its first entry. Each interaction invokes
the same C transaction with an async dispatch hook; that hook delegates ordinary
entry and stops tracing in finally while still suppressed. Await runs outside
the transaction, with existing breakpoint activation, then reenters explicitly.
Clear async fields on all exits. Do not fake an opcode event or change an
instruction stream to serve the debugger. B/root peer required before edits.
