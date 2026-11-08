# W5 preserved kernel increment: initial post-review

Independent review of the full staged and unstaged 28-file diff against
`e1229448be3`, captured in `w5-preserved.diff`, 2026-11-05. **No must-fix
finding** in this bounded review. This report covers the kernel increment;
the later static and suite R27 gate is a separate packet.

## Challenged findings, resolved

Two initial review concerns were wrong and are withdrawn:

- In `Modules/_pickle.c:save_long_inline`, the descending signed-width loop
  checks the sign bit of one *fewer* byte. For `2**31` it breaks at five bytes
  (`val >> 31 == 1`), before reaching four. The negative boundary breaks at
  five too. The independent focused probe round-tripped protocols 2–5 and
  matched C and pure Python pickle bytes at ±`2**31` and ±`2**39`.
- In `Objects/rangeobject.c:compute_slice_compact`, the call to
  `range_compact` restricts start, step and length to signed 32-bit values.
  `PySlice_AdjustIndices` bounds each endpoint index by the length (or -1).
  Thus the magnitude of each index-times-step is at most `2**62`; adding a
  signed 32-bit start stays within `int64_t`. The new step already uses checked
  multiplication. The alleged reachable overflow does not exist.

No code rewrite or extra test cycle is required for either concern. A small
pickle boundary regression test would be useful if added during normal
integration, but it is not a landing condition.

## Scope reviewed and evidence

I inspected the value slot and thunk pairs for array and ElementTree; the
method vectorcall bridge and the `_io` call sites; math, random, JSON and
pickle value conversions; byte search helpers; `isinstance`, range slicing,
exception value homes, warnings and the generated interpreter case. The
normal pattern passes a `PyValueBorrow` into a value form and closes an owned
result on all ordinary and error paths. I found no other must-fix issue in
this initial pass and no box-then-wrap bypass in these changes. The W5 worker's
private release build compiled 117 modules and its touched-module run passed
4,290 tests with 106 skips. Its 29 measured **per-loop** box values are zero.

That per-loop result does not satisfy the accepted R27 kernel gate: the
checker must read the startup-inclusive raw counters from every declared
kernel process. The worker has since found nonzero startup boxes, including
238 CORE and 19 THUNK on `richards`. Their exact sites need attribution and
removal before gate admission. The stats full-suite survey from before this
diff cannot qualify it. The coordinator owns merged validation.
