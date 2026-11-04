# Project status

Status as of 2026-11-03 16:20 PST, from the coordinator fact sheet. Update this
page at each landing.

## Outcome and next milestone

The restored universal-value runtime has not linked. There is no executable, no
runtime test and no timing for it. Any performance claim about it is premature.

The next milestone is a linked, qualified universal-value register interpreter,
followed by a matched JIT-off comparison against the interim runtime and upstream.

## Position

- Runtime branch `boil/pyvalue-rc-universal`, HEAD `0ab5bc42f61`. Checkpoint56
  was `6bf198d1ef5`; checkpoint57 (`5e977ec8cbb`) admitted the held type
  graph/MRO/cache projection.
- Wave 1 ported about 50 core files off the deleted borrowed getters. Wave 2
  ported stdlib accelerators, pickle and ctypes. Commit `0b7f39711d0` then
  landed the system modules (remote debugging, select, socket, curses, termios,
  posixsubprocess, resource, multibytecodec).
- `0ab5bc42f61` landed the test and C API modules.

## What blocks the next step

Debug `make -k python` (merged v23) fails only in `Python/getargs.c` (11 errors
plus one `pymacro.h` expansion). A scratch link test showed that getargs.c's
calls to deleted getters are the only undefined symbols. The release build shows
the same core errors. Shared modules fail only in Argument Clinic generated
headers and `Include/internal/pycore_modsupport.h:168`. All of this belongs to
the [native-boundary](design/native-boundary.md) redesign.

## Active work packets

- Native boundary P1 (borrow stack and getargs) and P2 (entry points):
  implementation is running while the design is under independent review. Nothing
  commits until review findings are folded in. P3 (remove owner scope from
  Clinic and nine files) and P4 (FASTCALL boxing onto the stack) follow.
- [PyValue](design/pyvalue.md): under independent review. Pre-baseline
  hardening packet: G2, G3, G4, G6.1, G10, G11 (G1 is fixed).
- [Register IR](design/register-ir.md): spec being written. Its author reports a
  copy-propagation `f_locals` deviation on the interim build (`x + g()` returns
  100 where upstream returns 1).

After P1 and P2: first link, then first execution (`_freeze_module`), then the
bring-up ladder, hardening, Clinic P3 and shared modules, test-suite triage, a
release build and measurement.

## Measurement setup

- Debug build: `build/pyvalue-rc-debug-20261102-v1`. Release build directory:
  `build/pyvalue-rc-release-20261103` (mimalloc, computed gotos, shapes, -O3).
  Neither has produced an executable.
- Upstream control: `build/upstream-release-3a440b6` (`3a440b65695`, -O3). It
  passes `test_int`. The harness time metric works. The instruction metric is
  blocked by `kernel.perf_event_paranoid=4`; the user was asked to lower it.

## Stages

Stage 1 ([runtime integration](stages/01-runtime/README.md)) is the critical
path. Stage 2 ([baseline](stages/02-baseline/README.md)) is blocked by it.
Stages [3](stages/03-method-jit/README.md), [4](stages/04-tracing-jit/README.md)
and [5](stages/05-extensions/README.md) are deferred or waiting on the
native-boundary design. See [HANDOFF](HANDOFF.md), [PROCESS](PROCESS.md) and
[WORKLOG](WORKLOG.md).
