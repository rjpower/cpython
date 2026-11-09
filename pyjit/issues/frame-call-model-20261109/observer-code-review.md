# Cold observer/source-map review

2026-11-09. Read-only review of B's uncommitted source against `4b3d739d867b`
in `build/worktrees/execution-boundaries-20261108`, branch
`pyjit/frame-call-model-20261109`. No runtime edits, builds or native tests by
this reviewer. The coordinator reported the second release build passed and
provided the failing focused-test/source-map inventory described below.

**Not ready for observer admission.** The source exception recipe and remaining
raw decoders are correctness blockers. Protected/suspended debugger jumps and
remaining POP/loop/send ownership artifacts were already disclosed as unfinished;
they are not new findings here.

## Required fixes

### P1: final source-only pass loses its exception context

Coordinator evidence `E11/merge2-source-error-inventory.json` records the failing
two-pass try-body map as `(from=0,to=1,line=2,handler=-1)`, then line 3 with
handler 3, then line 4 with handler -1. Handler 3 is a real PUSH_EXC_INFO entry.
Both pass statements belong to the same try body; the last pass must retain
handler 3. The coordinator's focused test shows LookupError escaping in both
sys.settrace and sys.monitoring modes.

`Python/assemble.c:assemble_source_edge` serializes `op->except` directly. Fix
the earlier CFG/source-context loss instead of inferring protection from the
landing word: the landing can correctly be outside the try. Retain the existing
two-mode regression and check its traceback line and the next normal exception
after the callback error, to catch stale exceptional observer state.

### P1: live remote and GDB decoders still parse the IRSM header as locations

`Modules/_remote_debugging/code_objects.c:181` parses location entries directly;
its call at line 468 passes the complete cached co_linetable bytes. Every
ordinary stored table now starts with IRSM. The remote reader therefore treats
the envelope header as location data, reporting incorrect source positions or
failing decoding. This is a live external inspection/profiler consumer, beyond
the two raw readers already identified by the coordinator.

Port these consumers to the validated location section:

- `Modules/_remote_debugging/code_objects.c:parse_linetable` and its native
  parser tests, including truncated/oversized envelopes from remote memory.
- `Tools/gdb/libpython.py:708` / `:760`, used for GDB frame line lookup.
- `Lib/test/test_code.py:1816`, whose independent location decoder currently
  iterates the entire envelope; preserve independent payload decoding while
  teaching it the new container format.

The remote debug-offset field still correctly names co_linetable; it does not
make the remote parser envelope-aware. Marshal, deepfreeze, umarshal and the
measurement hash/copy utilities found in the inventory transport the bytes
without decoding location entries, so they do not need the same parser port.
The C address-range initializer and column-removal path already unwrap IRSM.

### P1: fixed location payload bytes are not validated as payload

`Objects/codeobject.c:location_words` checks fixed payload length but advances
over those bytes without rejecting their high bit. For example raw
`b'\x80\x80'` is accepted as one short-form location and normalized. The second
byte is illegal payload: `advance_with_locations` asserts that its high bit is
clear, while the address-range marker scan treats it as another header.

Reject high-bit fixed payload bytes before storing the normalized map. Add a
malformed `code.replace(co_linetable=b'\x80\x80')` regression, together with
one-line fixed-payload cases. Constructor validation must agree with both
co_positions' fixed reader and co_lines' marker reader. This conclusion is
from source, not a native reproduction by this reviewer.

### P2: accepted/emitted entry records have no runtime consumer

The assembler emits `_PyIR_SOURCE_ENTRY` records and the constructor admits
them, but all runtime calls to `source_edge_events` pass FALLTHROUGH or TAKEN.
No caller passes ENTRY. Either execute those records at the observed entry
transition or prove/eliminate the producer case and reject unsupported input.
An accepted entry source span must not silently disappear from LINE events.

## Safety and remaining limits

The envelope uses explicit endian encoding and checks section lengths before
exposing record pointers. Construction checks record PCs and exception-home
bounds; kwargs validation also treats source-only handler targets as pending
entry points. Structural code.replace clears kwargs eligibility. Changed words
with a retained nonempty source map are rejected. These checks are useful, but
do not substitute for the missing payload validation or correct compiler recipes.

Source-only callback failure sets a thread-state frame/record recipe before
returning to the normal error label. Traceback creation consumes the frame's
source-line override before `get_frame_exception_handler` clears the cursor and
selects the source handler. That ordering looks coherent when the emitted
recipe is correct. Nested callback failures and allocation errors should remain
focused tests; this review found no independently proven double release there.

The current debugger analysis reads actual register roles and successors and
requires no incoming temporary owners at source and target. Rejecting protected
and suspended states is a conservative temporary boundary. Cross-frame jumping
while a child owns an argument window remains incomplete; the coordinator's
new test fails. Do not broaden acceptance by deleting the owner check: add a
separate suspended-call continuation/owner recipe first. Ordinary CALL windows
currently cause the liveness guard to reject that case.

Ordinary dispatch does not walk source metadata. Observed source transitions
scan all source records; that is a cold-path cost, potentially quadratic for
long traced sequences, not a claimed hot-path regression. Existing static
cleanup shim code still needs the already-known IRSM normalization before
asserting normalized tables universally.

After these fixes, rerun the focused boundary/traceback/observer tests in release
and debug, remote/GDB location tests, callback disable/re-enable and nested error
cases, then debugger incoming-owner tests. This is source review plus explicitly
attributed coordinator evidence, not a native test receipt or performance result.
