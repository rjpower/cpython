# Linear string append: pre-S1 proof and owner prerequisite

Status: revised design for independent review, 2026-10-29. **No native
implementation, build, test, probe or benchmark is authorized.** This is
S0, the smallest prerequisite to designing mutation-disabled S1. It does
not approve the Unicode capacity representation or close S3 observers.

This revises sections 3–4 and the certificate choice in the
[parent](pre-I-linear-string-concat.md), responding to
[readiness review](reviews/pre-I-linear-string-concat-design-review.md).
Preserve the first parent at commit `db96fc12f15`, SHA256
`1616f77fe3ce3317f44f5c4ca07d3404d192a40433982d38bb54b3c91cdf47be`,
and storage audit at `33c5fdc0f30`, SHA256
`73fae302bfa39a9901eff583ffd8242981faa3dc6b520a3ccc9ed40723c23a52`.
Those drafts and the first review remain historical; this revision does
not turn source analysis into executed evidence or self-approval.

## 1. Decisions and initial admission

Use one optional, immutable, **code-owned pure-data plan**, installed
before CREATE. Keep execution-copy construction lazy. Do not use
`co_extra`, eager execution storage, dual backing, caller flags or an
object-address ownership table. H3 must approve the code-layout change;
H1/ROOTS must approve the thread/frame interface below before S1 code.

There is no private permission on entry. Only a successful helper-created
fresh exact-string result can acquire it. Never transfer permission
through a strong copy, including a move that clears its source. A local
can later own a different fresh result after its old epoch ends.

Initially admit only ordinary optimized thread-owned VM functions and
non-cell fast-local destinations. An admitted append has a folded local
destination equal to its left input slot, with both operand reads covered
by the approved exact append semantics. `s += t` and `s = s + t` qualify
by this common physical form, not source spelling. An unfused result in
a temporary followed by STORE_FAST remains ordinary; it cannot carry
permission between words. Do not alter the compiler just to manufacture
this form in S0/S1.

The currently specialized folded handler is
`_BINARY_OP_INPLACE_ADD_UNICODE` (`Python/bytecodes.c:952–996`). Pristine
words retain generic handlers; use H3's reviewed semantic/canonical
relationship to identify which executing exact handler implements the
certified site. Canonical-family membership alone is insufficient: the
approved member must preserve the named operand and failure contract.
No handler-number ordering participates in admission.

Unknown effects may be supported by a **seal-all-before-entry** action
only when the dispatcher can actually execute that barrier before any
effect. Unknown word format, control flow, hidden slot effect, or a route
that can bypass that action rejects certification for the code object.
An unknown append site declines just that site when the surrounding
code is otherwise fully decoded. No partial or guessed effect table is
called a proof. Missing runtime barriers keep writes disabled in S1.

## 2. Compiler facts and finite transfer rules

Run derivation after `_PyRegCfg_Build` has finished copy propagation,
store elimination, truth folding and physical compaction
(`Python/regcfg.c:1405–1444`). The input is the surviving register CFG,
not source names or pre-pass LOAD_FAST/STORE_FAST pairs.

`_PyRegOp` distinguishes nread, npop, nimplicit, pass-through outputs,
folded dst, jump-only outputs and exception destinations
(`Include/internal/pycore_regcfg.h:38–80`). In particular nimplicit
contains exception-live values, not ordinary consumed reads.
Copy propagation folds one eligible consumed read and records its reader
(`Python/regcfg_passes.c:269–286`); that fact does not prove ownership.

The abstract state at a word boundary has:

- `MayOwn`: a set of eligible local slots that might have a live runtime
  owner bit. A member is a possibility, never permission to set a bit.
- For every live temporary, `Shared`, or `Borrow(local, use)` confined
  to a single approved append's operand evaluation. No Borrow may cross
  a word boundary, exception edge or branch. Ordinary temporary aliases
  are Shared after sealing their source.
- Actual physical-slot liveness and defined/NULL facts used to check
  read/clear order and distinguish ordinary, pass-through and exceptional
  effects. These are independently checked against final words below.

All locals/arguments/constants begin Shared, all owner bits clear, and
`MayOwn` is empty at function entry. There is no assertion that a Shared
value is unique or that equal pointers have equal permissions.

| Actual operation | Required transfer and runtime action |
|---|---|
| Approved append reads | Local operands may be borrowed for this operation only. Every consumed temporary operand must already be Shared. No other ordinary, preserved or implicit use of an owned operand may escape this operation. |
| Fresh successful append result stored to its certified local | Retire that local's old epoch before its first destructive slot write. Grant a new bit only after the result is complete, rooted in that local and all runtime gates still pass. Add the local to `MayOwn`. A copying fallback never grants. |
| Strong copy/read to a retained local or temporary | Seal the source before the copy. The copy is Shared. This includes LOAD_FAST_AND_CLEAR, swaps/save/restore and comprehension temporaries: clearing one slot does not transfer an epoch. |
| Ordinary/implicit preserved read of a possibly owned local | Seal that local before the read becomes an alias outside the approved append transaction. A preserved exception-live operand is not accepted as a consumed append read. If its exact timing is unclassified, seal all before the word. |
| Write/delete/NULL-clear of a local | Clear its owner bit before the first write, including early stores, exception-target clearing and multi-output/window writes. New ordinary contents are Shared, even if their pointer equals the previous value. |
| Cell/container/global/attribute store, return/yield or another publishing use | Seal the source before publishing; routes that can run code seal all before entry. Return/yield also retire the activation. |
| Call, slow guard exit, opaque/foreign entry or other callback route | Seal all before entry; ordinary results remain Shared. Existing `_PyIR_RUNS_CODE` is an input to this decision, not its completeness proof. |
| Pure control edge or reviewed no-escape fast path | Carry the existing bits unchanged. Actual error/callback/slow edges have their own seal actions. An empty backward-jump poll is not itself a seal. |
| Exception from any word | Seal all before error handling, traceback/exception callbacks or unwinding. Apply that member's actual local-write timing; handler entry has empty `MayOwn` and no owner bits. |

For two private operands, the RHS's bit may survive an approved read-only
exact append; no pointer to it is retained by the result. The LHS epoch
is retired/regranted as prescribed by the transaction. For `s += s`,
two consumed reads may name the same owner slot; both denote its old
prefix. There is one local bit, not two owners. A third preserved or
implicit alias disqualifies that borrow unless it is sealed first.

At a join, combine `MayOwn` by **union**. No join instruction sets or
restores a runtime bit. Join temporary provenance only when predecessor
facts agree; disagreement is Shared after predecessor sealing, or rejects
the affected site if a required borrow cannot be discharged. Use a
monotone worklist over the finite local-set/liveness domains to a fixed
point; do not stop after an iteration count. Unreachable blocks carry no
permission but still receive structural word validation.

For a simple local append loop, entry has `MayOwn={}` and shared `s`.
The first non-aliasing successful append produces a fresh result and
sets the bit for `s`; the backedge contributes `{s}`. The loop header's
fixed point is `{s}`, representing the entry path with bit 0 and later
private paths with bit 1. The exact append checks the actual bit on each
iteration. An admitted no-escape loop condition and empty poll preserve
it. If one branch saves `s` in `alias`, that branch clears the bit before
the copy. Rejoining cannot restore it; its next append must allocate a
different fresh result before setting a new bit. An actual generic
iterator/call/condition slow path seals and begins another epoch. S3
must approve any claimed no-escape fast path; a loop shape alone does not.

Derivation emits candidate sites and expected transfers, never a trusted
permission bitmap. The verifier recomputes these rules independently on
the assembled program. Failed proof or optional proof-memory allocation
declines certification and retains normal compilation behavior; suppress
only the optional allocation's own error, not a pre-existing error or
an unrelated compiler failure.

## 3. Final physical-word verifier and plan

The verifier runs after handler assignment, layout relaxation, emitted
words, feedback initializers, local kinds and exception table are final,
and before `_PyCode_New` publishes the object. Constant merging must
already have occurred; constants are Shared regardless of their identity.
No representation-changing pass may run afterward without rerunning it.

The following is a **required new reviewed interface**, not a claim that
`_PyIRFormat` already supplies it. H3/compiler owners provide a complete
semantic decoder of each admitted word/member: ordinary slot reads and
their consumed/preserved roles; hidden local accesses through immediates;
all writes/clears, pass-through and jump-only outputs; ordered early and
late writes; normal/exceptional exits; call/observer effects; and the
exact append's guard/success/error protocol. Window counts are decoded
from actual operands, not assumed to be ninputs. Current format roles,
borrow_mask and `_PyIR_RUNS_CODE` do not encode all these facts.

The source of truth for that interface is the instruction DSL and its
generated IO/effect paths (`Python/bytecodes.c`,
`Tools/cases_generator/ir_generator.py:1147–1309,1740–1785`, and
`Python/ceval_macros.h:530–588`). An explicit reviewed registry may
initially cover only the supported semantic forms. Each omitted form
fails closed as section 1 specifies. Do not infer implicit exception
uses from visible encoded input fields alone.

Admission algorithm:

1. Decode **every actual pristine word**, feedback operand and exception
   entry; check bounded slots, local kinds, targets, window extents and
   arithmetic before indexing. Build its actual normal and exceptional
   CFG. Exception entries' range/depth/lasti and handler targets must
   match the decoded effects and the register CFG's independently
   projected live/NULL state. An unrepresentable or unsupported mapping
   declines certification.
2. Check a one-to-many projection from each surviving CFG op to its
   emitted words and physical slots. Deleted ops emit none. Verify
   actual fields, all projected reads/writes including nimplicit,
   feedback operands and all outgoing edges. Do not compare only two
   copies of an unchecked annotation. A mismatch rejects the plan.
3. Decode assembler-inserted words as real CFG nodes. In relaxed
   FOR_ITER the `w+1`, `w+2`, `w+3` jumps go respectively to `w+4`, exit,
   exit+1; relaxed SEND has jumps to `w+3` and exit
   (`Python/assemble.c:830–887`). Verify those exact decoded destinations
   and each edge's output/NULL state. No assumed zero-length synthetic
   op or identity mapping may hide a jump. An edge entering an unproved
   interior declines certification.
4. Recompute liveness, alias/borrow and `MayOwn` fixed points from this
   physical CFG and the complete semantic decoder. Check all virtual
   candidate sites against recomputed facts; extra/reordered/implicit
   reads, overlapping compacted slots and different exception paths
   must change the result. Recompute every required sealing/retirement
   action rather than accepting proposed action lists as axioms.
5. Admit an append only when its actual destination is an eligible local,
   equal to its left-input slot; all transient reads meet section 2;
   success writes only the certified result/consumed slots; and guards,
   generic exit and exceptions have the required actions. A temporary
   alias of the left operand cannot substitute for local-slot equality.
6. Serialize only the checked result into the immutable plan. Before
   each private transaction, match the current code plan, actual word
   index, unchanged operand fields and approved executing member, then
   apply runtime owner/type/observer/allocator gates. A mismatched member
   or absent descriptor seals/declines before any effect. Rewriting the
   executing handler does not upgrade an uncertified pristine site.

The proposed pure-data schema contains a format version, total byte size,
word/local/slot counts, eligible-local mask, a word-indexed action/site
index, and checked append descriptors. Each descriptor records word,
canonical semantic identity, destination/left/right physical slots,
consumed-read mask, guard/error protocol identity, and bounded normal
and exceptional successor descriptors. Action data records seal-local,
seal-all and retire-written-local operations with their required timing.
All arrays use checked counts/offsets in one allocation; no Python/C
object pointers, handler addresses, callbacks or variable allocator
ownership are stored. A schema version is a format check, not a substitute
for checking the source facts. Exact field widths/alignment and generated
semantic identity representation require H3 review before S1.

## 4. Runtime owner state and failure timing

Proposed operations below name interfaces to review; they do not name
existing APIs. One optional thread-owned record has mode `idle`, `active`
or `disabled`, current frame identity, a monotonically advancing segment
generation, plan identity, local-count/capacity and an integer bitmap.
There are no string pointers. Code/plan identity is borrowed only while
the current frame's executable root keeps it alive. ROOTS must prevent
the cached frame/plan identities themselves from becoming hidden roots;
the bitmap backing may use the existing NoScan allocation family once
its allocation/gating contract is reviewed.

| Operation | Required state change before continuing |
|---|---|
| Enter/resume current frame segment | Retire any previous segment, advance generation, set all bits to zero, then bind current frame and immutable plan. Enable only for an admitted frame, frame_obj NULL and approved gates. Never restore a caller bitmap after a call. |
| Seal local(l) | Clear bit l before the publishing use. Idempotent; no allocation or byte mutation. |
| Seal all | Clear all bits before the first exposure or foreign effect. It may retain the current segment identity for later fresh grants. |
| Retire written local(l) | Clear bit l before the first actual store/delete, including direct handler writes and early/window writes. Equality of old/new pointer bits does not preserve permission. |
| Grant fresh result(l, generation) | Require the same still-active segment/generation, exact current frame/plan/site, slot l containing the fully initialized fresh result, and all gates. Set l only at successful commit. No grant to an arbitrary supplied existing pointer. |
| Retire segment | Clear bits, clear borrowed identities and advance generation before frame switch/unlink/clear/copy/pop, suspension, unwind, detach or thread cleanup. |
| Optional record/bitmap growth failure or generation exhaustion | Retire, then make this thread's feature disabled until thread-state destruction. Preserve pending exception state. No partial bitmap, retry that retains old bits, or wrapping generation. Correct copying execution continues. |

Generation advances on every entered execution segment and retirement;
an append holds its generation only within that segment. No stack/frame
address comparison alone validates a grant. Reuse of the same frame
storage for recursion, a later activation or a returned caller begins
with zero bits and a different generation. A call seals the caller before
switching to its callee; return starts a fresh caller segment, not a
restored epoch. Retained bitmap capacity is reusable only after clearing.
Allocation failure's sticky thread-wide disable deliberately avoids a
hidden per-suspended-frame failure state.

For the specialized folded append, copy/reload the original operands
through reviewed roots, validate guards, and perform the helper. Retire
the old destination bit before its actual NULL-clear. On success the
generated result store completes before a new grant; failure leaves
NULL and no bit, as the existing member requires. A private buffer reused
without allocation gets a renewed permission only for that same sealed-
from-observers transaction. A private-left/empty-right identity shortcut
may preserve its existing permission after revalidation; an empty-left
alias of an ordinary RHS never gains one. Other alias-returning shortcuts
remain Shared. The mutation-disabled S1 uses copied fresh results and
records these state transitions without claiming in-place coverage.

On generic or reflected fallback, seal **before** entering it. Generic
arithmetic's raised exception may leave the folded destination binding
unchanged; its bit is nevertheless clear. A successful generic result
stays Shared. Never clear a binding early merely to simplify ownership.
All exceptional exits seal all before callbacks/unwind, including
overflow, allocation failure, error formatting and exception-target
clears. An exception handler in the same frame may begin another segment
with empty bits; it cannot recover the interrupted epoch.

Growth may collect and later discover an export/gate change. Reload
operands from reviewed roots and compute from their captured values, not
from a possibly rebound local. A changed generation forbids the old
grant; return a sealed result under the executing handler's binding
contract. Collection is not automatically a callback-free permission
boundary. The actual ROOTS contract and S3 barriers remain prerequisites
to enabling mutation.

## 5. Code-plan construction, lifetime and source handoffs

Propose an owned optional plan parameter to the private `_PyCode_New`
boundary, forwarded to `init_code`; update **all** private callers with
an explicit plan or NULL. This separate parameter avoids silently
defaulting an added `_PyCodeConstructor` field. The exact C spelling is
for H3/code-layout review, not an existing API.

At call entry the constructor consumes a non-NULL plan on every outcome.
Before any fallible work, establish local ownership. If validation,
constant interning, location-table replacement or code allocation fails,
free it exactly once. Immediately after allocating code storage initialize
its optional pointer to NULL, before any cleanup can inspect it. Only
after complete normal initialization and final compatibility validation
transfer the plan to the code field, then call CREATE. No fallible
operation may leave ambiguous ownership. A CREATE watcher sees either
a complete certified object or NULL; attachment after `_PyCode_New`
returns is forbidden. Plan allocation/proof failure happened earlier and
declined the optimization; genuine code-construction failures still fail.

Current `init_code` initializes `_co_exec=NULL` before CREATE
(`Objects/codeobject.c:459–485`); retain that laziness. The plan is pure
immutable data and cannot reference `_co_exec`. A CREATE watcher may
execute or retain code immediately. It cannot supply, replace or upgrade
the plan. `code.replace`, public/unstable constructors, marshal, static
code and deepfreeze all explicitly start NULL even if words equal those
of certified code. The instruction-sequence assembler may pass a plan
only after this same complete verifier succeeds; sharing a compiler call
path is insufficient provenance.

`_PyCode_Destroy` invokes DESTROY watchers before sweep, and retained code
can survive (`Objects/codeobject.c:2366–2375`). It must **not** free or
clear the plan. Free only in actual `code_dealloc`, or matching actual
static finalization for any supported statics; initial statics have NULL.
No arbitrary freefunc or traversal visit is needed for pure data. Include
the new field through actual code struct size, and add actual plan bytes
to `code_sizeof`; keep compiler scratch, execution-copy bytes and retained
thread bitmap accounting separate. Do not double charge the fixed field.
Code equality/hash, marshalled bytes and deduplication remain semantic;
they do not compare or serialize the optimization payload.

| Interface / source consumer | Owner decision needed before native work |
|---|---|
| `_PyCode_DEF` in `Include/cpython/code.h`; private constructor declaration in `Include/internal/pycore_code.h`; `Objects/codeobject.c` init/New/dealloc/sizeof and constructors/replace | H3/code-layout: one optional field and owned parameter, explicit NULLs, exact failure/CREATE/DESTROY/accounting contract; review full layout and ABI consequences. |
| `Python/compile.c`, `regcfg.c`, `regcfg_passes.c`, `assemble.c`; `pycore_regcfg.h` | Compiler/string: after-pass derivation and final verifier/projection; source-sized scratch; no change to existing copyprop correctness contract. |
| `Python/bytecodes.c`, IR generators/metadata, `pycore_ir.h`, `ceval_macros.h`, generated handlers; `specialize.c`, `instrumentation.c` | H3: complete proposed effect decoder, canonical/member permissions, timing of guards/stores/errors and wrappers. Existing H3 ownership stays in force; no concurrent shared-file edit is granted here. |
| `Python/marshal.c`; `_Py_InitCleanup` in `Python/specialize.c`; `_PyCode3` declaration; static init/fini helpers; `Tools/build/deepfreeze.py` and generated frozen code | H3/code-layout: enumerate constructors/static aggregates, explicitly NULL certification, field order/extent and actual deallocation. Retained deepfreeze generator is a source consumer, not a qualified execution path. |
| `pycore_debug_offsets.h`, remote debug offsets validation and code readers, gdb code/frame readers; non-limited compiled code-layout consumers | H3/C-ABI: changed struct size/co_words offset, version/offset exports and regenerated readers. Do not claim ABI neutrality or blanket binary compatibility. |
| Thread-state backing/init/clear in `pystate.c` and relevant headers; frame entry/copy/export/clear and eval switch/return/unwind paths in frame/ceval sources and generated handlers | H1/ROOTS: exact segment identity lifetime, bitmap NoScan backing, no hidden retained frame/plan root, all pre-write and retirement points, current-frame invariant. This requests specific interfaces; it does not grant the entire files. |

The source consumer inventory must be checked again on the actual H3/
ROOTS integration head before assignment. Current primary source is the
inspection baseline, not proof that an unmerged code-layout change will
compose unchanged. S2 retains the separate Unicode bit/trailer/writer
inventory; S3 retains the complete observer/foreign-entry inventory.

## 6. Falsifying controls and acceptance

Future S1's final verifier must reject distinct fixtures for an extra
implicit preserved use; a strong copy hidden on one predecessor; an
incorrect exceptional successor/depth; a changed physical destination or
overlapping live compacted slot; and a changed synthetic relaxed jump.
Construct each defect in the actual verification input. Rejection of a
different earlier scalar error does not establish that named oracle.
Also verify the simple loop fixed point above remains admitted.

Shadow runtime controls must directly observe bit/generation state at
strong-copy entry, overwritten-slot store, specialized error NULL-clear,
generic error with retained binding, exception handler entry, caller
return, detach and reused frame-address entry. Removing the corresponding
transition must fail its named state oracle, even if copying output is
still correct. Test self-append and a distinct private RHS, empty aliases,
record/growth failure and generation exhaustion. An observer barrier
that is not installed is reported unavailable, not assumed covered.

Constructor controls must exercise a CREATE watcher observing complete
metadata with `_co_exec` still NULL; failure before/after allocation with
exactly one payload free; DESTROY retention followed by continued valid
plan use and eventual single free; NULL on replace/marshal/constructors/
static paths; and fixed-field plus payload `__sizeof__` accounting.
These are future obligations, not executed fixtures.

S1 is a development checkpoint with mutation disabled. Do not land its
permanent production barriers as a speedup without a useful qualified
path, unless retained proof tooling has zero production cost. Before
committing to production machinery, separately authorize a matched
release experiment comparing the actual enabled certificate/barrier
path with its parent: non-concatenating functions, calls/returns, tight
loops, observer/frame transitions and eligible appends. Report compile/
first-call cost, field/payload/bitmap bytes, barrier and slow-entry counts,
wall time and instructions with uncertainty and slower rows. Disabled
barriers and instrumented shadow timing cannot substitute for that path.
No arbitrary opcode or performance-ratio threshold is introduced.

Independent S0 review decides whether these proof/owner rules are ready
for H3 and H1/ROOTS interface review. Then a bounded S1 work plan names
exact hunks, shadow oracles and gates. S2 storage/ABI and S3 observer/root
closure remain separate prerequisites to S4 mutation. None is closed by
this document.
