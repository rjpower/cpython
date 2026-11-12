# Consumer, constructor and observer proposal

Proposed; no runtime edits or execution. Read the repository guidance, CPython
AI policy, canonical process, design, source audit and grouped Astra review.
Candidate R is `BASE/worktrees/delegation52`, from golden51 `0e5e61f4c6b`.

## Admission and shared geometry

Validate immutable constructor words, including `code.replace` and marshal paths.
A DELEGATE at s has SUSPEND at s+1 and RESUME_DELEGATE at s+2. Its receiver
window occupies three scratch homes; SUSPEND.A and B equal DELEGATE.B+2,
SUSPEND.C is delegated kind, D is zero; RESUME_DELEGATE.B is zero, A uses only
LOCATION_MASK (0x7) and DEPTH1_MASK (0x8), its location is AFTER_YIELD_FROM (2)
or AFTER_AWAIT (3), and CD equals s. All field and
destination extents retain existing completion/observer owner verification.
Standalone delegated SUSPEND or RESUME_DELEGATE is invalid. Plain SUSPEND keeps
the existing plain-yield home/recipient contract and following ordinary RESUME.

Reject normal branch and exceptional ingress to either middle word. The only
normal incoming edge to s+1 is the logical yielded DELEGATE edge; s+2 follows
SUSPEND and is the implicit native-resume landing. Source records do not grant
execution ingress. Reject source edges that claim a different normal edge.
Require CO_GENERATOR, CO_COROUTINE or CO_ASYNC_GENERATOR and RETURN_GENERATOR
entry shape for new suspension words, allowing the compiler's established
COPY_FREE_VARS/MAKE_CELL prefix. This is constructor filtering, never runtime capability:
runtime must require actual FRAME_OWNED_BY_GENERATOR before owner movement.

Preserve distinct EH contexts: SUSPEND selects the synthetic CLEANUP_THROW
recipe; provider errors select DELEGATE's outer recipe. Reject malformed
handler ingress and retain completion/observer handler ownership checks. The
far adapter is outside the synthetic region and uses completed owner facts.

Compiler-owned inline API in `pycore_ir.h` is agreed:
`int _PyIR_DelegateDoneTarget(const _PyIRWord *words, int count, int site,
int physical)`. It bounds checks, recognizes only exact triple plus s+3 IR_JUMP
with DELEGATE.D=2, and returns adapter.CD; otherwise returns physical. It reads
field geometry only and does not chase arbitrary jumps. Constructor admission
separately proves the triple, valid physical/normalized targets and adapter
ingress/EH. Consumers preserve physical completion PC and use normalized done
for branch/source coordinates. Never extend `instr_length` across the triple.

Cold owner analysis retains DELEGATE's logical B+2 yielded output followed by
consuming SUSPEND. This is an allocation/recovery model; physical fused yield
has B+2 NULL and no observable intermediate stop. Compiler/runtime own the
bounded executable-body proof and terminal owner's phase; consumer analyses
must not expose a logical DELEGATE POST yield as physically published output.
Frame redirection retains current exclusion for suspension/frame-pushing effects.

## Observer and provider interfaces

Map SUSPEND/INSTRUMENTED_SUSPEND to PY_YIELD and RESUME_DELEGATE/
INSTRUMENTED_RESUME_DELEGATE to PY_RESUME. The latter is always resume, never
PY_START, even though its A contains location flags while CD contains target.
DELEGATE has STOP_ITERATION PRE and BRANCH_LEFT POST via the shared semantic
completion helper and has no instrumentation twin. Its yielded logical edge has
no completion branch. New real words remain separate LINE/INSTRUCTION sites;
the real far adapter retains INSTRUCTION observation and physical dispatch.
Instrument/remove/restart, event-tool selection, forced source wrappers and
source normalization must treat the new operations deliberately.

Agreed cold helper in instrumentation.c:
`int _PyIR_PrepareSuspendLanding(PyThreadState *, _PyInterpreterFrame *,
_PyInstr *site)`. Runtime owns declaration in its header. Preconditions:
ordinary recipient transaction has consumed the held owner; parent is current
and coherent; winning continuation is the actual SUSPEND. Helper publishes site
and synchronizes stale instrumentation with `_Py_Instrument` under existing
tracing rules. Returns -1 on failure with site selected and homes intact, 0 on
success; runtime uses site+1 for error attribution and dispatches refreshed site
on success. It does not move owners or grant propagation permission. Pending or
same-PC redirect authority is resolved before calling it. Free-threaded fusion
is disabled for this increment.

Runtime owns pure per-parent fusion eligibility and rejects stale epochs,
observation/tracing/profile/source state, pending redirects, wrong active
continuation, nonempty B+2 and non-generator ownership. Conservatively rejecting
non-NULL monitoring/frame objects is acceptable. No source-map scan, callback,
allocation or instrumentation synchronization belongs in that predicate.

`specialize.c` adds `_Py_Specialize_Delegate(PyValue receiver_st, _PyInstr *instr)`
for exact-generator DELEGATE specialization only, preserving
guard behavior. Generic DELEGATE retains virtual/asend/custom/PEP523 provider
semantics, completion kind and error PC. Runtime confirmed the helper and
provider scope; there are no DELEGATE_VIRTUAL/ASYNC_GEN products. Existing SEND remains for async
iteration; do not globally rename it. `dis` keeps physical jump targets and
separate words; its contract text explains DELEGATE completion and SUSPEND's
next-resume recipient. Opcode metadata is generated by root.

## Files and focused evidence

Owned source: Objects/codeobject.c, frameobject.c, Python/instrumentation.c,
ir.c, specialize.c, Lib/dis.py and affected handwritten opcode consumers.
Remote/GDB/stat readers are audited for hardcoded SEND/YIELD/RESUME. Initial
search finds stats' iterator family list; remote and GDB consumers need their
actual declarations checked rather than guessed paths. Root regenerates all
products. Runtime owns generator/analyzer/proof tests; compiler owns test_ir.py
and test_ir_contract.py. No native builds/tests, regeneration or commits here.

New `Lib/test/test_ir_delegation.py` will assert actual emitted triples and test:

- deep next/send/return and discarded values; custom send/throw/close and
  missing/raising methods; exact coroutine await, async-asend and PEP523 scope;
- provider errors versus injected throw, thrown StopIteration/CLEANUP_THROW,
  GeneratorExit, reentry, finally/with and handled-exception preservation;
- gi_yieldfrom/cr_await, callback running/frame chains, escaped frames, cycles
  and last-owner finalizers inspecting coherent owners;
- monitoring installed by a child while parent is stale; LINE/INSTRUCTION,
  PY_YIELD/PY_RESUME and completion events; tool disable/reuse; callback error,
  redirect and same-PC intent; near/far normalized completion and real adapter;
- constructor mutation of flags/entry, slot/recipient/kind/resume target,
  branch/EH ingress and adapter shape. Runtime malformed-owner cases must give
  safe SystemError rather than invalid generator conversion.

Reuse existing IR word mutation helpers and monitoring cleanup conventions.
Update affected old completion/disassembly fixtures without weakening behavior.
Root runs meaningful focused release/debug tests at integration; source review
must independently confirm bounded fusion and generated transfer-loop shape.
