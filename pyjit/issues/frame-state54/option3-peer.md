# Option 3 independent peer review

Source reviewed against selected51, not executed. Approve the proposed
implementation with the following explicit teardown and diagnostic boundaries.

The normal frame owns `f_funcobj`; the function owns both environment objects.
An externally reachable executing, suspended, or escaped frame therefore makes
the function and its environments reachable. Function `__code__` mutation does
not change these environment identities, and the independent executable owner
must remain. Removing the borrowed caches changes neither this graph nor its
counted edges. Keep explicit environment visits in the custom precise root
walker through the new accessors, as proposed; do not silently change its root
contract in this lane.

`func_clear` NULLs the environments before decref and can trigger callbacks.
An unreachable function/frame cycle can consequently contain cleared members
while garbage is being broken. That does not authorize dereferencing them in
deallocation: frame deallocation must continue using only executable geometry
and owned specials/homes, and no environment accessor may be added there. An
externally retained frame prevents this cycle from being unreachable. The
collector finalizes and checks resurrection before clearing; cleared weakrefs
cannot supply a new owning frame reference during `tp_clear`. The cited GC test
checks prevented weakref resurrection, not a surviving cleared function. No
counterexample within the supported strong-reference/traversal contract was
found in the selected collector paths.

Special entry/base/cleanup frames are not ordinary functions. Keep sentinel
handling at API/debug boundaries and avoid deriving an environment from None.
`_PyFrame_SafeGetCode` is a freed-memory heuristic: validate the retained
executable without first decoding an unvalidated function pointer. It may drop
the two old namespace heuristics; replacing them with a blind function decode
would weaken fault tolerance. GDB needs its separate safe decode and sentinel
checks. No universal hot function/type guard or duplicate environment owner is
needed.

No unresolved source-contract blocker found. Existing frame.clear specials,
escaped-frame identity and finalizer tests in the proposal are appropriate.
Root's native gate must report added function decode/environment dependencies
at GLOBALS/BUILTINS uses, as well as removed construction/copy stores and new
layout. Performance admission remains open.
