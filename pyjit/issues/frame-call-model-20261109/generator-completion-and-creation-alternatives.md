# Generator completion and creation: three structural alternatives

Proposal only at clean source32 `488e85fa3a8`. No runtime, benchmark, build or
native changes by A. Root's accepted paired counts show generators 1.056034 and
coroutines 1.052249 instructions/golden. The accepted single cycle recordings
locate SEND_GEN/YIELD_VALUE for generators and CALL_PY_EXACT_ARGS,
RETURN_GENERATOR, CompleteStep/CompleteGeneratorReturn for coroutines. See
`ninth-current-perf-review.md` for identities, counts and sample limits. Symbol
shares are not a cost subtraction or a predicted gain.

All options retain one result-ownership and semantic outcome contract. A result
stays C-owned through child retirement. Completion detaches the input window,
publishes destination and continuation before owner retirement, preserves PRE
versus POST observation/error state and honors newer redirects. Ordinary calls
remain inline; generator yield and completion remain distinct outcomes.

| Option | Repeated work it could remove | Added cost / principal risk |
| --- | --- | --- |
| 1. Specialize the shared completion implementation | Out-of-line metadata decoding, generic kind/destination work and irrelevant held-owner tests | More compiled commit shapes; must keep one publication/retirement implementation and the cold observer/error adapter |
| 2. Save an explicit completion continuation at entry | Caller opcode/argument re-decoding on return | Stores/target calculation on every resume, larger frame header and full lifecycle/consumer migration |
| 3. Bind directly into the owning heap frame | Thread-frame reservation/initialization, RETURN_GENERATOR dispatch, full frame move and pop | Larger call/binding change; partial construction, GC, failure, hooks and immediate-result call ABI |

## 1. Shared completion specialization: smallest first experiment

Current VM RETURN_VALUE calls CompleteGeneratorReturn, which makes real
BaseOpcode and InstrOparg calls and invokes the generic eight-argument Step
helper. Step accepts native noncanonical providers as well as canonical frame
results, and FOR/SEND/THROW share its owner-close tail. The compiled normal
route is recorded in `normal-completion-assembly-review32.md`.

Factor its callback-free commit and held-owner retirement into one implementation
that can compile with known semantic kind and canonical-result provenance. Keep
the observed/error path cold and shared. Do not blindly inline the entire
callback runner or duplicate its exception restoration code. The canonical
frame-return proof already exists; native providers must still publish their
unknown results. A runtime enum parameter in a separately compiled generic
helper does not establish compiler specialization by itself.

An even narrower first subexperiment is removing the two out-of-line decoder
calls using the already bounded SEND/FOR completion contract and actual immutable
word layout. The existing path already reads the word for A/B. A positive helper
can decode the semantic family/relative exit without treating an arbitrary
handler as SEND/FOR or confusing wrappers with the real instruction. Constructor
and generated format facts must agree. This needs no new frame field and may
make option2's extra state unnecessary.

ABI/GC cost is minimal: no new owning roots, frame field, descriptor or cacheline.
Code size can grow if known-kind bodies clone too much; inspect the actual
generated/compiled diff. Same receiver/result aliases, local displacement,
NoDst, mortal return payloads, native provider failures and inactive-but-allocated
monitoring data must keep the current semantics. PEP523 entry stays on its
existing route. Default/keyword binding is untouched.

## 2. Explicit completion continuation: a genuine ABI alternative

Current `return_pc` is the real immediate continuation used when a child yields;
completion takes the SEND/FOR exit instead. Reinterpreting that one pointer as
both targets would break the two outcomes. A minimal alternative keeps it and
adds a non-owning completion exit pointer plus a typed SEND/FOR kind. A/B can
still come from the immutable actual site word; avoiding that extraction too
would require additional saved base/destination fields.

The current GIL release header has 112 bytes before owned_slots. A pointer plus
a kind byte can plausibly use eight extra bytes with the kind in existing tail
padding; verify actual layouts in release/debug/free-threaded builds rather
than assuming this packing universally. Every heap generator/frame and thread
reservation then pays that size, even ordinary frames. Exported remote layout,
frame copying/escape, trampolines, entry frames, GC verifier ranges, deepfreeze
and size tests must migrate coherently. Code serialization carries no live
frame pointer, but code replacement/executable lifetime and f_lineno changes
must never leave a stale continuation.

Install the completion target/kind together with existing caller return_pc and
child/exception links in the callback-free resume commit. Initialize all frame
constructors; reset/replace on every subsequent VM/native entry. Exceptional
child exit and pending debugger redirects must either invalidate it or prove
it remains unused. PEP523/custom resume paths need the same target semantics or
the existing generic fallback. Neither observer offsets nor public f_lasti
change domain.

This moves work: tree delegation resumes a suspended child repeatedly, often
many more times than it completes. Computing/storing an exit on **every resume**
may cost more than return decoding. Known format extraction can be cheap, but
adding an InstrOparg call at every specialized SEND entry would be especially
suspect. Measure resume/completion counts and compiled entry/return routes before
choosing this ABI. Do not assume a saved pointer is free because it removes a
return-side call.

## 3. Direct owning heap frame: creation and binding alternative

Current exact calls reserve a thread frame, bind arguments and enter word0.
RETURN_GENERATOR allocates a generator/coroutine, moves all frame slots and
header state to its heap frame, then pops the thread reservation. Calls originating
from a generator cannot overlap its heap slots with thread storage, so the
current exact-call path explicitly copies the argument window as well.

A common binder could instead choose initialized owning storage before binding:
thread arena for an ordinary function, generator-owned storage for a supported
generator/coroutine. Bind once, return the created object directly and start its
future execution at the actual initial RESUME/recipient. This removes real work
at the creation boundary, rather than adding an execution alias.

The positive admission must include standard Python provider, pinned function/
code version, expected entry semantics and no PEP523 interception. Function
flags alone do not prove that user-replaced code begins with the expected
RETURN_GENERATOR contract. A general call result must distinguish immediate
object result from an entered child frame; measure any new branch/return-struct
cost on ordinary CALL, not just the generator benchmark. Reuse binding logic for
exact/default/kwargs routes rather than introducing another incomplete binder.
Mutable defaults, overlong defaults, varargs, virtual kwargs, cells/freevars and
custom providers retain current checking/fallback behavior.

Partial construction is the main ownership issue. Current make_gen GC-tracks an
object with FRAME_CLEARED and no executable; only after frame copy does it become
CREATED. A binder that moves owners into that invisible frame and then allocates
kwargs/default structures could hide them from GC. The new construction
primitive must provide an initialized, traversable partial frame before any
such move, or keep every owner separately anchored until an atomic commit. All
failure paths must clear exactly those owners without emitting a false
unawaited-coroutine warning or running close on an unstarted invalid frame.

Keep the selected code pinned through allocation and binding; do not let a later
func_code reread select another layout. `_Py_MakeCoro` currently computes coroutine
origin from the temporary incomplete current frame's previous complete frame.
Direct creation needs an explicit origin anchor yielding the same logical caller.
Preserve names, async-generator hook initialization, gi_frame/locals visibility,
profiling/monitoring start-versus-creation order and normal close/throw/PEP479
behavior. An initial unobserved fast route should use the existing observed
creation route when monitoring requires those semantic events; a monitoring
change during preparation needs revalidation before an irreversible owner move,
not a missing-event exemption. Allocation/GC or default retirement can reenter;
parent call owners and its published phase must remain coherent throughout. No prompt destruction
timing guarantee is needed, but reachable owners must remain rooted.

## Selection and gates

Prefer an isolated option1 subexperiment first because it leaves frame/binding
ABI unchanged. That is a sequencing preference, not a performance conclusion.
Review actual source with B/H before admission, regenerate once, inspect ordinary
handler changes and run existing completion/payload/owner/monitoring/reentry
gates in both builds. Then use the same frozen identities and calibrated paired
counts/elapsed cases to retain or reject it.

Option2 needs its complete constructor/copy/redirect/native-entry census and
header-size ledger before implementation. Option3 needs an explicit partial
construction/GC/call-result contract and failure tests before code. Neither
should be combined with option1 in its first measurement. Preserve logging and
the repaired broad controls; no remaining loss is explained solely by a flat
profile percentage.

## Actual source anchors for peer review

All runtime paths below are in the source32 runtime checkout:

- `Python/ceval.c`: `_PyEval_CompleteGeneratorReturn`, `_PyEval_CompleteStep`,
  `call_bind_known_python`, `clear_gen_frame`.
- `Tools/cases_generator/ir_generator.py`: `return_to_caller`, `enter_generator`.
- `Python/bytecodes.c`: `_SEND_GEN_FRAME`, `_INIT_CALL_PY_EXACT_ARGS`,
  `_PUSH_FRAME`, `_YIELD_VALUE`, `RETURN_GENERATOR`.
- `Include/internal/pycore_interpframe_structs.h`: frame continuation/recipient
  layout; `pycore_interpframe.h`: `_PyFrame_Initialize`, `_PyFrame_Copy`,
  `_PyFrame_SetReturnValue`.
- `Objects/genobject.c`: `gen_send_value_ex`, `make_gen`, `_Py_MakeCoro`,
  `gen_traverse`.
