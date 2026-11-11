# Option 5 source/native post-review

Approved for isolated measurement, with debug and merged qualification still
pending. Independently reviewed runtime commit 5e08ce56cff against bf810 and
the following test expectation adjustment in the working tree. No blocker found.

The new capability is supplied only by the real `_RETURN_VALUE` boundary, after
its result owner is detached and scratch is proved empty. Generic eval/frame
entries pass false. Shared clear bodies preserve profile updates, generator
state and exception unlinking, critical sections, reverse NULL-before-close
retirement and f_locals/function/code lifetimes. The generator-return marker
still follows all callback-capable retirement. Generic suspended close/throw,
deallocation and exception unwind retain full non-thread cleanup. A throw/close
that actually returns receives the normal-return proof appropriately.

Escaped-frame takeover still precedes the bound selection and copies the full
frame. Completed entry assertions precede retirement; the canonical return
owner remains C-owned through retirement and unchanged recipient completion.
The exact generator return proof changes only its clearing entry name, and
release classification includes the new callback-capable completed entry.

Inspected the actual option5-release binary. `_PyFrame_ClearCompletedExceptCode`
at 0x3d0420 supplies completed=1 to the shared body; its 0x3d0204 capability test
reaches the co_nlocalsplus load at 0x3d0225 (code offset 0x48) and the existing
reverse clear loop. Generic entry supplies zero; a non-thread owner then calls
`_PyFrame_ClearLocals` at 0x3d02e5, which retains co_framesize (0x4c) minus 14
header words. The completed generator route in
`_PyEval_FrameClearAndPopCompleted` calls the explicit completed frame entry at
0x381ce3. Full frame takeover still copies the co_framesize-derived slot count.
This demonstrates the intended bound change, not an application speedup.

Coordinator build log `E/option5-release-build.log` checks all 117 modules.
Coordinator `E/option5-release-focus.log` reports SUCCESS: 349 tests, eight skips,
six modules (test_ir_generator_completion, generators, coroutines, frame,
monitoring and pyvalue). New tests cover aliases/closures, escaped locals,
retirement reentry and suspended scratch close/throw/unwind. Debug checks,
matched generators/coroutines/comprehensions and logging measurements, and final
merged qualification remain coordinator gates. Extra capability routing exists
in native code and must be assessed by those measurements.
