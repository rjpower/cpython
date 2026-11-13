# Typed pins peer review

Disposition: GO for M1 implementation, with the consumer obligations below.
Reviewed selected54 source and [proposal](pins-proposal.md), 2026-11-12.
No native/build/test evidence is claimed by this source review.

The ownership argument matches the current value API: `PyValue_DUP` increments
mortal object references, while immortal tagging suppresses unnecessary counts.
Typed strong `Py_NewRef`/`Py_CLEAR` pins preserve that policy, including FT.
The independent executing code pin is necessary when function code changes.
Frame copy/escape must retain the explicit source code duplicate until its
caller clears it; no replacement through mutable function code is allowed.

Consumer obligations included in the approved boundary:

- Heap frame traversal visits typed code and function once with `Py_VISIT`.
  Thread/transition roots mark external owners; they never add extra container
  subtraction edges. FT deferred-stack accounting must stop interpreting code
  as a value and must not add a deferred count for either strong typed pin.
- Optional FT stack marking must visit both typed pins. The existing loop marks
  code but omits function; after typing it should enumerate both explicitly.
- NULL interpreter sentinels are recognized by owner before code dereference or
  remote NULL rejection. GDB construction likewise checks shim before reading
  code metadata. Cleared, half-built generator state tolerates NULL code.
- M1 retains full initialized/NULL home readers and conservative retirement
  scanning. Existing frame-deallocation release order needs no new map lifetime
  in M1; M2/M3 must keep code/map authority through the last map reader.
- Safe traceback access retains freed-pointer checks; asynchronous remote
  readers retain range checks for reused/torn samples and read direct pointers.

No must-fix design issue remains for M1. Final admission still requires a full
consumer census, focused lifetime/construction/observer checks, release/debug
compile and post-review. This review does not authorize stale scratch, removal
of conservative roots, or FT qualification from GIL evidence.
