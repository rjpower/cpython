# Cold public construction: handwritten handoff

Source ready and frozen for independent review in
`BASE/worktrees/frame54-cold-construction`, branch
`pyjit/frame54-cold-construction`, based on immutable golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. Changes are uncommitted;
root owns review, builds, execution and commits. Proposal and independent
scope approval are `cold-construction-proposal.md` and
`cold-construction-peer.md`.

Exactly three files differ: 138 insertions and 35 deletions.

* `Objects/genobject.c`: the public common constructor computes enabled
  coroutine origin before moving the input frame, frees an untracked empty
  allocation and consumes the stolen frame on origin failure, and installs
  raw IR_NO_DST before the existing two-argument frame copy. Under the input
  frame critical section it establishes CREATED/GENERATOR ownership, acquires
  the owning frame-object reference, initializes exception/name/subtype fields,
  associates f_frame and GC-tracks only after coherent initialization. It
  releases the stolen frame reference after unlock. PyCoro_New and
  PyAsyncGen_New directly return that common helper, avoiding duplicate origin
  allocation and postpublication mutation.
* `Modules/_testcapi/frame.c`: `frame_new_generator` uses the existing frame
  factory and selects the three public constructors by code flags, exercising
  their stolen-frame contract directly.
* `Lib/test/test_capi/test_frame.py`: three new regression methods cover
  generator first-input rejection/discard, yield/completion/close, association,
  namespace lifetime through escaped-frame takeover and frame.clear;
  coroutine first input and origins at depths 0/2; and async-generator first
  iteration hooks, first input, asend, close and finalizer behavior.

The source is the reviewed R2 cold extraction, with its comment adjusted to
describe golden51's actual default slot zero. `_PyGen_YieldFromRef` remains
the original raw-index implementation. No header, activation default,
recipient encoding, result publication, debugger/remote code, bytecode DSL,
handler family, instruction format, opcode, magic or generated product changed.
The ordinary frame initializer still stores raw zero; public construction
explicitly establishes the existing raw discard contract before its first
input. No NONE state or R/S optimization was extracted.

Reviewed the complete diff; `git diff --check` passes. Host Python invoked
through shared-cache uv parses the edited test module and finds exactly the
three intended new `test_capi_` methods. This worker ran no native interpreter,
test, inferior, build, regeneration or measurement. No runtime result is
claimed for this new identity; prior R2 results are not reused as qualification.

Before integration, independently review the actual three-file diff, then
root should run matching release/debug constructor and frame/GC qualification.
Existing `_testcapi.call_with_nomemory` can support a bounded allocation probe;
the new tests do not inject allocation or origin failure. Preserve the
distinction between frame allocation, subtype allocation and origin failure,
and check stolen-frame/namespace/code ownership. Any throughput or native
placement claim requires its own evidence; this handoff makes none.
