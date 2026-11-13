# Option 3 independent source review

Reviewed candidate `frame54-globals` against golden51 after implementation.
No generation, compilation, runtime, tests, or timing performed by this reviewer.
No source-contract blocker found; root retains validation ownership.

The field-name census leaves no handwritten interpreter-frame namespace
dereference. Normal getters derive from the retained function; executing-code
authority remains independent. Initialize/copy/base/entry paths remove the two
borrow caches without changing owners. Global/name/import/warning/API reads use
the getters. Public frame getters retain their nullable namespace behavior.

The custom root walker retains explicit environment edges and first checks
function kind, covering the internal None-function init-cleanup frame. That
frame remains incomplete for both of its words (`_co_firsttraceable=2`), so the
existing complete-frame walk skips it at normal APIs. Entry/base owners remain
incomplete. The hot getter needs no universal sentinel guard.

SafeGetCode removes the old namespace heuristics and validates executable bits
without decoding a potentially invalid function. GDB's environment path checks
None/NULL and function kind before reading the function environment. No new
environment access appears in frame/function clear or deallocation. The two
new escaped-frame tests cover changed function code, distinct mortal namespace
identity, GC, explicit clear, and finalizer inspection during local retirement.

Generated and native closure remains required: rebuild all header geometry,
verify no accidental DSL escape effects, inspect added LOAD_GLOBAL dependency
chains and removed initialization/copy stores, and run the agreed semantic and
performance gates. This source review does not imply performance admission.
