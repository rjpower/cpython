# Round54 architecture review: five bounded experiments

Status: proposed, source-reviewed architecture; no implementation, build, native
execution or timing was performed by this reviewer. The coordinator owns those
steps. All five start independently from selected51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`, in
`BASE/worktrees/round51-golden`; upstream is `upstream-3a440b6` at
`3a440b65695`. BASE and canonical-document paths follow [plan](plan.md).
The CPython AI policy and the canonical coordination/design/activation53 packet
were read. [Field inventory](field-inventory.md) supplies the complete field and
consumer census; its geometry is from existing selected/upstream ELF metadata.
This reviewer also disassembled the existing selected release ADD_INT handler;
that static inspection is recorded under S and did not execute the runtime.

## Decision

Test the five contracts below, with independent peer review before code. Prefer
carrying a proved recipient/storage fact over maintaining a second mutable
validity representation. Retain NULL homes and the initialized frontier in all
five. A dynamic bitmap is assessed below, but is not among the first five:
its update frequency and integration surface are much larger than the remaining
warm-arena initialization surface. Their relative cycle cost is unmeasured.
These are falsifiable options, not predicted wins.

| Rank / lane | Exact proposed saving | Added recurring work | Principal risk |
| --- | --- | --- | --- |
| 1 / R | Empty-recipient proof removes old temporary-home load and close/classification at ordinary result/resume publication | Classify recipient when recorded; test encoded kind when publishing | Saving is small; recording/checking costs can exceed it |
| 2 / F | Remove `ir_feedback`: one header word and initialize/copy/refresh stores | Dependent entries load from execution prefix per feedback access | Feedback-heavy loops lose more than activation setup wins |
| 3 / N | Remove `f_globals` and `f_builtins`: two header words and their setup/copy work | Decode pinned function and load namespace on each actual use | Global-heavy code and GC teardown contracts |
| 4 / L | Put `f_locals` owner in existing optional frame object: one header word and ordinary NULL store removed | Eager frame object only for supplied mapping; extra indirection on mapping operations; changed clear bookkeeping | Class/module/exec allocation and retirement overhead |
| 5 / S | Certified same-home numeric variant removes a temporary NULL store and its generic owner-path work | Additional executable handler text and specialization choice | Compiler may already remove the store; admitted sites may be scarce |

R and S are different: R carries a recipient class across a call/resume and
still writes NULL on every consumption; S suppresses one intermediate NULL
write inside one non-reentrant commit. Neither uses future-read liveness as an
ownership test. No lane revives round51 slot-base reconstruction, round52 chain
forwarding, or round53 completion/construction adapters.

## What the comparison does and does not support

V8's safepoint table records tagged stack slots and registers at a PC; entries
have lookup/decompression machinery. This is evidence for static root location
metadata, not for replacing every RC ownership transition with a root bit.
A root map says where the collector should find references at an admitted stop;
it does not say who owns a count, when a moved source becomes unavailable, or
which old local a callback can replace. See the primary
[V8 safepoint implementation](https://raw.githubusercontent.com/v8/v8/main/src/codegen/safepoint-table.h).

LuaJIT encodes frame kind with aligned PC/delta information and derives previous
Lua frames from call operands. Its stack collector walks tagged values and
frame boundaries. This supports asking which state can be reconstructed from
existing authority, but does not prove Python can remove independent function
and executing-code owners, overlapping-stack checkpoints, or exception links.
See [LuaJIT frame definitions](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_frame.h)
and [collector](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_gc.c).
These are design comparisons, not borrowed performance claims.

### NULL, bitmap, static map and check bit

| Scheme | What it establishes | Dynamic cost and missing contract |
| --- | --- | --- |
| Selected NULL plus initialized frontier | Every physical home is safely readable; NULL owns nothing; other tags determine owner behavior | Consumption clears one home; fresh/uncertain arena suffix initializes, warm suffix does not; teardown scans actual owners |
| Dynamic validity/occupancy bitmap | A set bit authorizes reading a canonical home; unset permits arbitrary stale bits | Set after publication, clear before transfer/retirement callbacks, test before uncertain reads; range transfer for overlapping arguments; copy/traverse/unwind/redirect must all honor it |
| Static future-read liveness | Which values subsequent program instructions may need | Does not cover dead-but-owned values, observable locals, callback edits or exceptional phases; insufficient for RC or safe raw loads |
| Static ownership map | Which homes may own values at a precise semantic phase, including exceptional and observer edges | Requires phase authority at every callback/GC/poll/unwind/suspend; dynamic local occupancy and callback mutation still need representation |
| Encoded recipient/check class | A narrow producer-verified property of one operation | Cheap if selected once into handler/recipient; a generic per-operation mask test still costs work; does not license reading stale bits |

A single bit cannot simultaneously mean “initialized,” “has an RC count,” and
“live.” Inline numbers are initialized/readable and may be future-read live but
have no RC count. A boxed dead temporary can still own a count. An unbound local
and pending empty kwargs can both be NULL with different Python meaning. A
bitmap that tracks only mortal roots cannot substitute for safe-value validity.

A correct arena bitmap would preferably describe physical cells, since exact
calls overlap caller and child homes. Per-frame maps instead must explicitly
remove the caller window and install the child view during the non-reentrant
commit, then restore the correct parent view. Heap frames require maps sized to
their physical homes; escaped copies must carry occupancy, and raw scans must
never inspect unset cells. All local writes, kwargs materialization, debugger
restoration, binder defaults, hidden constants, returns, SEND/FOR completion,
exception clearing and C helper writes are producers. This is a representation
migration, not a clear-loop patch.

It saves uncertain-suffix or heap initialization and some NULL stores/scan
loads, but adds mask address/shift/RMW work on ordinary transfers. A byte tag
avoids word RMW but spends more storage and still adds traffic. Multi-bit tags
can cache value classes only by maintaining them on every publication. Neither
has a credible first-order advantage over selected warm thread reuse without a
weighted mutation/scan census. Never benchmark saved clear-loop loads without
counting all map writes. Static ownership maps remain a possible later project;
precise GC stopped-state maps alone cannot authorize stale homes between stops
when decrefs can reenter Python. No universal bitmap mode is added in this round.

## R: encode replace-required versus empty recipient

**Thesis and boundary.** `_PyFrame_PublishReturnSlot` currently loads the old
home and calls `PyValue_XCLOSE` even when the recipient is a temporary proved
NULL. `_PyFrame_SetReturnValue` already checks pending redirect and discard.
Carry the local-versus-empty fact in unused high bits of the frame's existing
`uint16_t return_slot`, removing that old-home load/close from the empty route.
This does not replace the generator completion adapter or add a continuation
record. Ordinary, send-input and yielded-result publication can use the same
contract where their existing producer establishes the class.

**Encoding.** Physical IR slots are 0..4094 and IR_NO_DST is 4095. Keep the
low 12 bits and the untagged discard sentinel; reserve bit 12 for
REPLACE_REQUIRED. Bits 13..15 remain zero. A raw `return_slot` is henceforth a
recipient descriptor, never an array index. Introduce explicit encode/index/
discard/replace accessors; convert every direct consumer, including
`genobject.c`'s yield-from `return_slot - 2`, the remote asyncio debugger's raw
uint16 recipient/receiver-delta reader, debugger comparisons, initializers,
copy and test APIs. Do not steal bits from the already full eight-byte IR word.
Assert extent and sentinel constraints in constructors and internal tests.

**Ownership and observation.** Encode local/cell/free-variable destinations as
replace-required regardless of static last use: observers can change their
owners. A temporary descriptor means its current physical home must be NULL at
publication; maintain the existing overlap and freshness proof. On redirect,
run `_PyFrame_CompleteRedirect` first, then reread the descriptor; a callback
must not leave a cached old destination/class authoritative. Discard closes the
held result once. Replace installs result before closing the actual displaced
owner. Empty stores directly, with a debug NULL assertion. Held result/input
owners remain in C across retirement exactly as selected. No change to GC,
exception recipes, frame links, recursion, pending kwargs or allocation failure.

**Work and scope.** Adds one recipient-class calculation at commit and one
kind test at publication; masks may affect yield-from/debugger readers. The
producer must not perform code-owner lookup when it can use existing local
boundary authority. Keep recording cost visible in native CALL/SEND/YIELD.
Scope: `pycore_interpframe.h`, DSL/generator recipient producers, ceval result
macros/helpers, `frame.c`, `genobject.c`, `frameobject.c`, generated products and
internal tests. This lane changes no field size. Do not add handler families
just to precompute this bit in the first version.

**Proof and gate.** Exercise empty/local/discard destinations through ordinary
calls and SEND, first send and resume, exception/close/throw, nested last-owner
finalizers, caller locals mutation, pending debugger redirects and escaped
frames; test index 4094 and discard 4095 distinctly. Native output must remove
the empty-route old-home load/classification and show every added producer
instruction. Count recipient classes separately. Require the common timing
gates below. Bounded fallback: restrict optimized publication to ordinary
returns, leaving all descriptor consumers consistently decoded and send
publication on checked replacement; if bookkeeping still wins no work, reject.

## F: derive feedback from the existing execution prefix

**Thesis.** Remove `ir_feedback` at selected offset 88. Real executing words
follow `_PyCodeExec`, whose `entries` member already supplies the feedback base.
Use `((_PyCodeExec *)frame->ir_words - 1)->entries`, the same prefix authority
already used by `IR_NLOCALSPLUS`. Remove initialize/copy/refresh stores and
associated setup load. Expected GIL header: 112 to 104 bytes.

**Added work.** Each IR_FB can gain an entries load dependent on ir_words,
instead of a direct frame member load. Existing prefix loads may share the
address, but source similarity is not proof of CSE. Do not replace this with
`function->code->_co_exec`: the function's code is mutable and that adds a longer
chain. Never count this as an unconditional instruction reduction.

**Contracts.** The executing-code owner pins the actual image throughout calls,
callbacks and suspension. Refresh changes ir_words and instr_ptr together using
existing authority. Ordinary allocation and `_PyIRStaticExec` init-cleanup
images have the prefix. The interpreter entry trampoline also has a real prefix
with NULL entries; it must not request feedback. The thread base sentinel has
NULL words and cannot be prefix-accessed. Cold diagnostic access must handle
that sentinel before prefix access;
normal feedback handlers need no new universal guard. No new owners,
allocations, GC edges, value-home changes, exception paths or field ranges.

**Files and proof.** Update struct/header setters and copies, IR_FB/checks,
ceval's fallback refresh, generated code, deepfreeze/frame-size calculations,
debug offsets, tools and direct test users. Check feedbackless code, static
init cleanup, sentinel unwind, instrument/deinstrument, changed function code,
closures, generator copy/resume, frame escape and free-threaded compilation.
Native proof must show the absent setup stores and the new hot dependency
chain, plus actual header offsets and allocation arithmetic. Weight feedback
uses against frame starts. Bounded fallback is the full eight-byte removal
with one shared accessor; do not introduce a fallback cached field or ABI mode.
Reject if feedback-heavy elapsed losses outweigh gains.

## N: derive namespaces from the pinned function

**Thesis.** Remove the two borrowed frame caches `f_globals` and `f_builtins`.
Their ordinary initializer reads `func_globals` and `func_builtins` from the
already retained function. Those attributes are Python-readonly; selected
writes are construction and function clear. Normal accessors decode f_funcobj
and read its actual member. Expected header: 112 to 96 bytes, or 88 with F.

**Costs and contracts.** Every globals/builtins consumer may add a function
PyValue decode and dependent member load. LOAD_GLOBAL, name/import operations,
frame APIs and precise root walking are real consumers; this is not merely
cold observer reconstruction. Function and executing code remain separate
owners, so changing `function.__code__` must not change an activation's code or
namespace authority. Do not derive the namespace from a newly looked-up
callable. Init-cleanup/interpreter sentinels have non-function values; cold API
and diagnostic readers need explicit sentinel behavior without burdening each
normal handler with a type check.

The implementation review must trace cyclic-GC/function-clear order, escaped
frame inspection and finalizers. “Readonly attribute” alone does not prove a
borrow survives `func_clear`. Preserve selected behavior at every actually
reachable inspection phase; if an environment must outlive the retained
function's members, this full removal fails its owner contract and must be
rejected, not repaired with borrowed dangling caches. Precise root walking must
retain its required edges through the new accessors. No namespace ownership
may be silently dropped or reconstructed after a callback from mutable code.
No new allocation or exception behavior is otherwise needed.

**Files and proof.** Update all direct readers, GLOBALS/BUILTINS macros, safe
frame/code heuristics, initialization/copy, GC tracing, frame/eval/name/import
APIs, GDB/debug offsets, generated code and frame-size/deepfreeze geometry.
Tests include changed code, class/module/eval/exec mappings, altered builtins
contents, closures, escaped frames, cycles with finalizers, teardown inspection,
static init shims and profiling. Native proof must show both removed stores and
hot function decode/member loads. Benchmark global-heavy code and logging as
well as activation-heavy cases. Bounded fallback: remove only f_builtins in an
independent revision if evidence isolates that cache as beneficial to remove;
do not claim the full two-word thesis for that narrower result.

## L: move the locals mapping owner into the optional frame object

**Thesis.** Keep `frame_obj` as the only nullable cold-state pointer. Move the
strong `f_locals` mapping owner into a new, separate PyFrameObject member. Do
not reuse `f_extra_locals` or `f_locals_cache`: they have different semantics.
Optimized calls with NULL incoming mapping create no object. Entries supplied
with a non-NULL mapping allocate the frame object before ownership commit and
install that mapping there. Later lazy frame publication creates the same
record with a NULL mapping. This removes one pointer and its ubiquitous NULL
initialization from the fixed header (112 to 104 bytes), while retaining
variable owning homes and the opposing arena stacks unchanged.

**Preparation and errors.** `_PyFrame_Initialize` is currently nonfallible and
consuming. Do not allocate halfway through it. Prepare the optional frame
object at the caller's existing fallible preparation boundary while function,
code and mapping owners remain declared; on failure release preparation owners
once and do not publish a partial child. Commit assigns the record and consumes
the mapping without callbacks. Direct PyFrame_New construction already has a
frame object and must reuse it, avoiding a recursive allocation. Do not invoke
ordinary lazy materialization on an incomplete unpublished frame or materialize
pending kwargs early merely to acquire storage.

**GC and teardown.** The new member owns the mapping; its sole GC visitation
belongs to PyFrameObject traversal even when f_frame points into an executing
thread/generator frame. Interpreter-frame traversal visits frame_obj, not a
second mapping edge. Copies transfer frame_obj and update its frame association
at the same lifecycle point as selected takeover/generator copy; never create
a second mapping owner accidentally. Escaped takeover retains the mapping in
the surviving object. Non-escaping clear currently destroys a uniquely owned
frame object before clearing slots, then clears f_locals after slots. Preserve
the mapping's required lifetime/order: detach it into an explicit C owner
before dropping that unique frame object, clear homes with valid published
state, then close the held mapping once. Reentrant finalizers must never see a
dangling frame_obj or half-cleared mapping. Frame-object clear/dealloc handles
its own new member exactly once. Free-threaded publication/critical sections
must retain the existing acquire/CAS discipline.

**Costs.** Mapping-heavy exec/class/module paths pay an eager frame allocation
and mapping indirection; potentially the existing variable frame-object backing
tail is allocated even before escape. This is a real cost, not a free cold
sidecar. Ordinary NULL-mapping frames remove a store and eight bytes but their
clear path may add record handling. No new universal sidecar allocation is
allowed. This is the only lane moving ownership to variable/cold storage;
F/N derive values instead. `base`, `saved_top`, previous, current/return PCs,
code/function owners and exception/recursion state stay eager.

**Files and proof.** `pycore_frame.h`, interpreter-frame struct/initialization/
copy, generic preparation in ceval/call providers, `frame.c`, frameobject and
mapping consumers, GC, debug tools and geometry/deepfreeze. Test optimized and
nonoptimized locals, custom mapping callbacks/failures, class bodies, exec/eval,
PyFrame_New, generator frame escape, cycles, mapping finalizers, traceback,
tracing and allocation failure before commit. Native proof includes ordinary
NULL-store removal, no new ordinary allocation, actual eager mapping-route
allocation and changed clear path. Run class/module/exec focused throughput
alongside all29. Bounded fallback: keep one complete ownership location and
reject the lane if prep/clear cannot be made coherent; do not add an ad hoc
header/sidecar dual-map mode to rescue the experiment.

## S: certify a same-home numeric overwrite

**Thesis.** Add a very small execution-only refinement for ADD_INT and, only
after that works, SUB_INT where A == B and B is a temporary. The selected
ADD_INT handler guards both operands, computes a canonical result, checks its
error, then calls IR_CONSUME_INPUT for B/C and finally stores A. On the proved
route, the B-to-NULL store is immediately replaced at the same physical home
without a callback between; B contains an inline integer, so it owns no count.
Omit that intermediate store and the B owner/close machinery, then overwrite B
with the canonical result. Preserve C consumption unless independently proved.
Emit the result store using the same B home expression in this variant, so the
compiler knows the physical alias; a debug assertion equating independently
decoded A and B is insufficient in release code.
The result may be boxed after overflow; the proof concerns the input and
post-success commit, not a promise that all results are inline.

**Native feasibility.** Static `objdump -d --disassemble=_TAIL_CALL_BINARY_OP_ADD_INT`
of `BASE/round51-golden-release/python` shows the B NULL store at handler
offset `+0xa6` (`0x355f76`), following the B/local comparison at `+0xa2`;
C is separately cleared at `+0xb9`, and the result store is at `+0xe5`.
Thus GCC has not eliminated the proposed B store from this generic handler.
Existing inline guards already remove actual input decref work: do not count
nonexistent native closes as an extra saving. The variant can remove the B
classification/store and A local/discard selection under its alias proof.
This establishes a surviving native target, not weighted alias-site hits.
The coordinator must count reached A==B temporary sites before throughput
claims. Do not broaden into callback-crossing store sinking. The current execution-variant
framework has LOAD_ATTR_SHAPE_LOCAL_TEMP; extend its established machinery,
not a parallel instruction interpreter.

**Proof and checks.** A constructor/specializer checks A == B, C != B, B below
IR_MAX_SLOTS and at/above the local boundary, with C's unchanged ownership
contract. Install an execution handler variant rather than spending an operand
bit: all four 12-bit fields are already occupied. The handler bank must remain
within 16-bit handler encoding. Pristine words remain ordinary; deoptimization
and instrumentation restore the canonical family and its full checks.

All guards and fallible result production occur before changing any input
ownership. Overflow allocation/error sees selected PRE homes. After successful
result construction, the proof requires that no observer callback, decref that
can deallocate, allocation, poll, dispatch or exception exit intervenes between
adoption and overwrite. Preserve actual semantic PC and source-event recovery.
At the next possible observation, A owns the result and any consumed C home is
NULL. Do not apply the optimization to locals, discard, alias-uncertain inputs,
non-inline input classes or arbitrary provider results. No persistent stale
home, root map, new allocation, cleanup mode or error recipe is introduced.
Debug assertions must check the variant's true before/after invariant rather
than temporarily requiring the omitted NULL write.

**Files and gate.** Bound changes to execution-variant metadata/generator,
specializer/constructor validation and generated cases/targets, plus focused
negative tests. Exercise alias and nonalias, local/temporary C, integer
boundaries and overflow/error, instrumentation enable/disable, failed guards,
code replacement and hostile constructed IR. Add generator proof negatives
that inject callback/fallible work into the commit suffix. Native proof must
show fewer instructions/stores on reached sites and report extra handler text.
Count variant hits outside timing. If there are no surviving native stores or
no meaningful hits in unchanged workloads, record that rejection; a synthetic
microbenchmark does not qualify throughput. This lane changes neither frame
geometry nor general home validity.

## Shared admission and composition

Run discoverable focused tests on candidate release/debug runtimes; proof and
source tests supplement them. Required shared coverage includes function/code
ownership, frame escape, GC/finalizers, exception unwind, generator/coroutine
send/throw/close, observation/redirects, recursive resource failure and supported
free-threaded builds. Preserve documented debugger/LLTrace limitations; do not
broaden the GDB quarantine to make a lane pass. Root owns native inspection,
builds, generation and execution; the isolated static ADD_INT inspection above
is not evidence those gates passed.

Use the unchanged generator, coroutine, DeltaBlue, Richards, comprehension and
logging kernels: two balanced pairs to screen, six to confirm promising lanes,
then all29 and final direct selected/upstream comparisons. Record instructions,
cycles, elapsed time, code/stack size and reached-route counts separately.
Extra instructions of roughly 35–41% and SEND/YIELD sample weight motivate
investigation, but do not allocate removable cycle budgets to headers or stores.
A smaller header can make operand loads worse; fewer instructions can still
lose elapsed time, as round53 established.

| Pair | Composition issue |
| --- | --- |
| R + F/N/L | Shared initializer/copy/header files; retain descriptor semantics, regenerate once; no semantic dependency |
| R + S | Separate recipient versus single-instruction output contracts; do not infer one proof from the other |
| F + N | Independent derivations; combined extra hot loads may accumulate; predicted GIL header 88 bytes |
| F + L | Both remove one pointer; predicted header 96 bytes; cold-record and prefix lifetimes remain separate |
| N + L | Mapping remains a strong record owner, namespaces remain function-derived borrows; GC traversal must distinguish them; predicted header 88 bytes |
| F + N + L | Predicted header 80 bytes on measured GIL layout; this is geometry only, not upstream equivalence or a speed prediction |
| S + F/N/L | Static proof unaffected, but machine placement and dependencies change; inspect generated native code again |

Recompute actual debug/free-threaded alignment, FRAME_SPECIALS_SIZE,
co_framesize, deepfreeze output, generator/frame-object tails, chunk capacity,
debug offsets and tools for every layout composition. No lane may conflate
fixed header savings with removal of variable home capacity or heap allocation
per resume. Measure composed candidates directly; never multiply individual
ratios. A near-neutral independent field removal may justify a bounded geometry
composition if its owner proof is complete, but rejected breadth is not erased
by an unmeasured combined-size argument.
