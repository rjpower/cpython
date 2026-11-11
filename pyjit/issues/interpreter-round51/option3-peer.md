# Option 3 proposal review

Approved for isolated implementation with the conditions below. Reviewed the
proposal against bf810 exact-call uop, `_PyFrame_PushOverlapping`,
`_PyFrame_PushUnchecked`, `_PyStackChunk_EnsureFrameStorage`,
`_PyFrame_Initialize`, stack-space admission and pop restoration.

Selecting final storage before one header initialization preserves the current
activation contract. Both current branches already reserve initialized cells
before header writes; sharing that reservation need not change function/code
ownership or argument overlap. Outlining suffix fill is acceptable because it
cannot run callbacks or fail and is reached only when storage is uncertain.

- Keep capacity admission before reservation. The selected physical end must
  cover max(callee end, saved thread top), with initialized frontier truncated
  before any reused header. Save/restore the original thread top exactly once;
  never restore a frontier that crosses a retired header.
- Restrict caller-tail NULL and pointer-order assertions to actual thread-owned
  callers. A generator's heap window must never enter thread-stack subtraction,
  ordering or overlap tests. Non-thread reservation starts at thread top and
  leaves its heap argument owners live until the infallible move.
- The shared initializer consumes callable once and acquires code once, using
  the code selected by the guarded uop. Preserve `frames_pushed` accounting once
  for both the exact path and unchanged `_PyFrame_PushOverlapping` callers.
- The cold transfer helper moves only argument/self owners, then clears exactly
  their source window cells; common commit clears callable. For no-self zero-arg
  calls it must not overwrite slot zero, which may hold an initialized constant.
  Constant homes and unpassed local homes remain initialized and owned normally.
- The outlined frontier helper must not allocate, retire, select function code
  or call Python. Existing initialization/truncation semantics apply to every
  `_PyStackChunk_EnsureFrameStorage` caller, including new-chunk setup. Keep the
  initialized fast path inline and inspect native code for accidental common-path
  helper calls.

The proposed deep narrow/wide frames, generator callers, mortal constants,
frontier reuse, escaped frames, unwinding and reentry tests cover the main
semantic risks. Root must compile release/debug and inspect actual successful
hot-path instructions/spills. A single source initializer or smaller symbol is
insufficient if GCC duplicates its native body or taxes ordinary overlap. No
performance or executed qualification is established by this approval.
