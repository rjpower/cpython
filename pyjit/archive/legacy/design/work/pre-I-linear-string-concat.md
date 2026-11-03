# Pre-I linear string concatenation

Status: revised design for independent review, 2026-10-29. No implementation,
test, benchmark or build execution is authorized by this document.
Source audit: primary `9dd24bf42c4`; the sources cited below were read,
not changed. The separately qualified verification component is frozen.

Owner: coordinator-assigned string/compiler work before checkpoint I.
ROOTS owns collecting-C/constructor safety; H1 owns frame/root transitions;
H3 owns generated handler properties and executable metadata storage. This
proposal does not take those owners' files or approve their pending work.

The first readiness review returned **REVISE before S1**. The focused
[S0 proof/owner revision](pre-I-linear-string-proof-owner.md) now specifies
conservative transfer/join/exception rules, final-word admission and an
activation-generation state machine. It selects one optional immutable
code-owned pure-data pointer, installed before CREATE and retained until
actual deallocation, preserving lazy execution copies. That revision
supersedes this draft's open certificate-storage choice; independent
review and H3/H1/ROOTS interface decisions remain. The prior drafts and
review are retained with commit/hash identities in S0.

## 1. Decision sought

Review a **private append epoch**, whose only mutable strings are newly
allocated exact strings owned by a certified local of the currently
executing VM frame. Publication irrevocably ends that object's epoch.
Strings always have their ordinary flat representation, logical length
and terminating zero. Publication only revokes permission; it neither
flattens nor allocates.

The proposed capacity representation is concrete but needs a separate
Unicode storage/ABI audit before implementation (section 5). Approval of
the proof does not approve that storage choice or enable a fast path.
The next reviewable prerequisite is S0's proof and owner interface, with
no implementation. S1 would implement proof and revocation machinery
with mutation disabled. Sections 8–9 give the later bounded split and
acceptance conditions. No stage is assigned automatically to tier 1.

Note 08 and 6c-rung4 section B rejected a rope because `PyUnicode_DATA`
cannot fail. That decision stands. No public Unicode accessor acquires a
new allocation/failure path, and public `PyUnicode_Append` keeps its
ordinary copying contract. A different representation/API would require
its own explicit decision.

## 2. What the source establishes

| Source | Consequence |
|---|---|
| `Objects/unicodeobject.c:11708`, `PyUnicode_Append` | Nonempty operands allocate/copy. Empty-left returns the right object; empty-right preserves the left. On error `*p_left` is cleared. None of these shortcuts establishes privacy. |
| `Python/specialize.c:2411` and `Python/bytecodes.c:952` | The specialized instruction checks that its folded destination holds the left operand. Other locals, containers and C references can still hold that object. Its inherited refcount explanation is not a proof in this fork. It supports both `s += t` and `s = s + t`. |
| `Python/regcfg_passes.c:155` and `Python/regcfg.c:1405` | Copy propagation proves a delayed borrowed read safe across selected operations; it is not escape analysis. Pass order is swap renaming, copy propagation, store elimination, pop removal, truth folding, then compaction. |
| `Objects/frameobject.c:46`, `:275`, `:2442`, `:2471` | A locals proxy returns actual string objects and can replace a local. `PyFrame_GetVar` and the locals cache provide more export paths. A proxy captured before the string exists can read it later. |
| `Python/frame.c:23`, `Python/pystate.c:1956`, `:2393` | Frame materialization is shared by Python/C frame access, traceback/back links and current-frame enumeration. Exported frames outlive activation storage. |
| `Python/instrumentation.c:1354`, `:1461`; `Python/ceval_gil.c:1248` | Line/instruction callbacks, pending calls, signals, GC draining and thread detach can expose/rebind locals without the append explicitly passing them. An empty breaker poll is different from entering such code. |
| `Objects/unicodeobject.c:3355`, `:749`, `:14176` | Exact-string constructors and unchanged-result paths can return their input. A constructor result is not necessarily fresh. |
| `Include/cpython/unicodeobject.h:265`; `Doc/c-api/unicode.rst:1071` | Canonical data is directly accessible; UTF-8 buffers are cached and remain valid until the owning object is collected. An exported object must never later grow, move or change its bytes. |
| `Objects/unicodeobject.c:1077`, `:13810` | The writer-only resize owns its object; compact strings have logical length, not capacity. `__sizeof__` currently calculates storage from that length. An unshared bit alone supplies neither capacity nor honest accounting. |

Examples the proof must handle, not benchmark exceptions:

* `alias = s; s += suffix`, storing `s` into a list/cell/global, and
  `alias = str(s)` all can retain the original object.
* Construct `rhs` before the private epoch; its `__radd__` saves and
  returns its left operand. After `s += rhs`, a later exact-string append
  must not change the saved object. The entire binary operation cannot
  be exempt from publication merely because its successful fast path is.
* Capture `p = sys._getframe().f_locals` before initializing `s`, then
  save `p['s']` between two appends. Disabling privacy only when the
  proxy is initially created is insufficient.
* A callee, signal handler or monitoring callback saves the caller's
  `s`; a C helper saves both the object and its canonical/UTF-8 address.
  Later appends must leave that object and those addresses unchanged.

`gc.get_objects()` is not a shortcut to a counterexample exposing every
string: `Python/gc_live.c:2032` filters its snapshot to tracked GC objects.
The relevant hazards are frame exports and actual callbacks/foreign
ingress. Do not claim a plain string appears in that list without proof.

## 3. Ownership certificate and invariant

Privacy means **one externally unobservable logical owner**, not one
machine pointer. The local and transient operands/root records may all
refer to the same object during an approved append.

At a dispatch boundary, each private object must satisfy all of these:

1. It was allocated by the private helper for a successful nontrivial
   exact-string append. It was not adopted from a constant, argument,
   constructor result, empty-concat shortcut or arbitrary C producer.
2. One certified fast local owns it. No other local, cell, container,
   preserved temporary, escaped argument or API observer can retain it.
   The only allowed other reads are consumed transient reads of a
   successful approved exact append, including its right operand.
3. It is exact, non-interned, non-immortal and unhashed, with no cached
   alternate encoding. Its canonical prefix and terminator are valid.
4. Its owner is the currently executing ordinary VM frame. No suspended
   caller, generator/coroutine, externally owned frame or detached thread
   carries private permission. `frame_obj != NULL` disables privacy for
   that activation, including future strings created after the export.
5. Runtime observer/allocator gates and the certificate still hold.
   Unknown forms take the copying path. No permission is inferred from
   address equality, a storage flag, a root or a refcount compatibility API.

As specified in S0, the compiler derives a certificate from actual
register use/definition and implicit reads after copy/store/truth
transforms, then projects and
verifies it against final physical slots and assembled words, including
synthetic jumps. A family-level `_PyIR_RUNS_CODE` bit is a conservative
barrier input, not a uniqueness certificate. Existing copy-propagation
and root maps remain independent contracts.

Initial scope is optimized ordinary VM functions and non-cell fast-local
targets. Cells, generators/coroutines and externally owned frames keep
copying semantics. Constants/arguments may be the initial shared prefix;
the first new private result copies them. This covers a reusable local
append rule, including both spellings above, rather than a fixed loop,
literal, iteration count or benchmark fusion.

Only core-compiled, verified certificates are admitted. The selected
initial path gives code constructors, `code.replace`, marshal, static and
frozen code an explicit NULL certificate, without later upgrade; a
supplied flag/bitmap is never trusted. This does not claim H3-2a's scalar
checks prove a CFG. The initial constructor path
may simply leave this optional optimization unavailable. H3 must review
the selected dedicated code field, canonical handler selection and
renumber consumers before code is written; do not smuggle it into an
assumed `co_extra` slot.

## 4. Revocation and observer closure

**Seal** clears private permission, with no allocation, resizing or byte
change. An object once sealed is never readopted. A later eligible append
can create a different fresh private result.

Only the currently executing frame can own permission, because it seals
before a call/frame switch, suspension, return, unwind or detach. Thus a
per-thread owner-frame plus local bitmap is sufficient in principle;
no object-address alias table or extra field in every frame is proposed.
The bitmap contains no owning object pointers. Locals/root records own
the actual objects. Optional metadata allocation failure disables the
optimization instead of adding a Python exception. ROOTS/H1 must review
thread cleanup, frame movement and all owner retirement paths. S0 defines
enter/resume, seal, pre-write retirement, fresh grant and segment retirement:
bits are never restored after a call or at a join, and a generation check
prevents a reused frame address from inheriting an old grant. Strong copies
always seal; they do not transfer permission.

| Boundary | Required action before exposure |
|---|---|
| Strong local read/copy to another owner; cell/container/global/attribute store; call argument; yield/return | Seal the source before publishing an alias. Retire an overwritten destination before replacing it. |
| Generic operation or unknown Python/C call, even without `s` among its operands | Seal the current owner frame before entry: that code can inspect its caller. All ancestors have already sealed at their own calls. |
| Exact append guard failure, cold/generic dispatch, reflected/subclass action, error formatting | Seal before fallback, not after it returns. Only the successful exact-string path bypasses this barrier. |
| Frame materialization, existing proxy/get-var/cache readers and writers, frame copying/clearing | Revoke the target's owner state before exposing or changing slots. Any materialized frame remains ineligible. Include direct C readers, not just `locals()`. |
| Trace/profile/monitoring callbacks, including line/instruction and exception/return events | Revoke before callback arguments/frame access. Initially any active observer disables private append; registration/removal and reentrant transitions still need the entry barrier. |
| Pending Python/C callbacks, signals, GC finalizer/weakref/gc.callback drain, remote-debug execution | Revoke before actual foreign/Python entry. Merely testing an empty breaker must preserve permission. |
| GIL detach, stop-the-world suspension, frame switch or cross-thread observation | Revoke before making the owner available. The shared-GIL build is the current source contract (note 06 section 17.1); this is no free-threaded proof. |
| Registered reftracer or custom allocator callback | Initial feature is unavailable. Audit the complete installed allocator chain, including debug wrappers; do not identify safety from a marketing allocator name or assume every indirect allocator callback is non-reentrant. |

The finite implementation audit includes `regcfg_passes.c`'s ordinary and
implicit reads/writes; generated guards/deoptimization and instrumented
wrappers; frame proxy/get-var/cache/materialization APIs; call/return/
unwind and generator paths; `_Py_HandlePending`, thread detach/attach;
GC queue callback entry; allocator/reftracer configuration and invocation.
Use generated semantic properties, not numeric handler cutoffs. A new or
unclassified route seals/declines until audited.

Two distinctions are essential:

* Sealing unconditionally on every binary family member would seal each
  successful append. Sealing on every backward-jump poll would seal each
  loop iteration. Both are correct but fail the intended growth result.
  The same loop protocol remains; approved no-escape fast paths preserve
  permission, while guard failures and actual slow entry revoke it.
* Allocation-triggered mark/sweep is not permission to expose a mutable
  string. ROOTS must establish its no-Python/no-existing-edge-publication
  contract, including traversal/opaque cleanup effects, before this helper
  relies on it. Existing unresolved allocation/callback hazards are
  dependencies, not fixed by this proposal. Ordinary callback drains seal.

## 5. Proposed capacity storage: explicit review decision

Do not recover capacity with `mi_usable_size`, guess debug-header offsets,
or overallocate while forgetting the size after a frame returns. The latter
cannot support accurate `__sizeof__` or a normal/debug growth proof.

Proposed representation for helper-created compact exact strings only:

* One currently reserved state bit means **has capacity metadata**. It
  says nothing about uniqueness. The representation is intended to retain
  type/data offsets and struct sizes and leave `PyUnicode_DATA` as its
  existing direct, non-failing calculation. Supported compiler/platform
  offset and size checks are required before claiming that preservation.
* Allocate `header + (C + 1) * kind + sizeof(Py_ssize_t)` checked bytes,
  where `C` is character capacity. Logical length is `L <= C`; the usual
  zero remains at `data[L]`.
* Store `C` with `memcpy` immediately after that logical terminator,
  at byte offset `(L + 1) * kind`. This avoids alignment assumptions.
  The reserved word fits even when `L == C`. Within a private append,
  read it before copying, overwrite the old tail as necessary, and write
  it after the new terminator. String results depend only on logical
  characters and their terminator; retained diagnostic/implementation
  physical reads keep their existing bounds contracts, as distinguished
  in the storage audit. After sealing, both data and metadata are fixed.
* `__sizeof__` reports reserved character storage plus this word for the
  flagged form; ordinary strings retain their existing calculation.
  Allocator/collector accounting charges the full requested allocation,
  including slack. Copies/serialization copy logical characters, not a
  storage flag without its corresponding allocation.

Required producer/consumer inventory: `PyUnicode_New`, subtype creation,
static ASCII/Latin-1/global-string initializers and their generators,
copy/result helpers, interning and hash/UTF-8 caches, public resize,
writer prepare/finish/resize, consistency checking, `__sizeof__`, free,
serialization and C API tests. Audit every Unicode header/state assignment
or copy, not just direct constructor calls. Every ordinary constructor
initializes the new bit to zero; no copy may propagate it without the
corresponding allocation and metadata. Include raw/vectorized consumers
whose physical reads extend past their logical chunk, every kind/ASCII
transition, and normal/debug/ASan requested sizes and free paths after seal.
Public resize already copies; writer-only resize must
retain its actual ownership contract and never accidentally adopt a
sealed overallocated input as mutable storage.

This is **not** claimed ABI-transparent merely because offsets are
unchanged: a reserved bit gains meaning and flagged payloads gain a word.
The coordinator/Unicode/C-ABI owner must approve the representation and
its producer inventory, including compiled inline C access, before its
implementation rung. If rejected, replace this entire subsection with
another fully specified normal/debug capacity scheme. Do not quietly
substitute a rope, global weak table or undocumented allocator query.

## 6. Append transaction, collection and failure

The helper is internal and takes the certified target, not a general
`PyUnicode_Append` mutation privilege. It rechecks the owner bitmap,
destination/operand relationship, exact types and runtime gates. All
generic/cold cases keep existing dispatch and copying behavior.

1. Read both operands and their original lengths/kinds. Preserve empty
   shortcuts and existing constructor aliases without marking returned
   objects private. A private left plus empty right may keep permission.
2. Check length addition with the existing overflow class/message. Compute
   the maximum capacity from header, character width, terminator and
   metadata word using checked arithmetic.
3. If the private buffer has space and its representation can hold the
   right operand, append with no call, allocation, safepoint, GIL release
   or possible error between data/terminator/metadata/length updates.
   Publish a consistent new logical value only at the transaction end.
4. Otherwise allocate a fresh flat result; do not realloc the old object.
   Root left, right and the new result through the existing precise
   value-root/span protocol across every collecting helper. Initialize a
   complete valid prefix before publishing the result or its permission.
   After every possible collection reload objects from updated root slots
   and recompute data/header/metadata addresses. Keep no cached object or
   character-buffer address across collection: the owner bitmap names
   local slots, so a later moving collector need not repair hidden aliases.
   Recheck owner/observer/allocator gates before granting permission to the
   completed result. If they changed, return that fresh result sealed.
   Compute it from the originally captured rooted operands, not a newly
   reread `f_locals` binding; preserve the IR's documented operand timing.
   Collection probes belong at well-formed boundaries, never midway
   through an uninterruptible metadata update.
5. Use `Cnew = max(new_length, 2 * Cold)` with checked saturation at the
   representable maximum, not repeated exact growth near overflow.
   A first private allocation starts from its required length. Widening
   allocates the new ASCII/Latin-1/UCS2/UCS4 representation explicitly;
   it cannot silently move the old header/data pointer.
6. If a speculative reserve fails, revoke and retry the existing exact
   copying path. A successful exact retry returns an ordinary published
   result; it is not linear-growth evidence. If the exact operation also
   fails, propagate its exception and existing binding/ownership outcome.
   Metadata allocation failure likewise falls back. No prefix is modified
   before a failing allocation, and no exported object is ever reallocated.

For `s += s`, both reads refer to the original prefix. With capacity,
copy from that prefix to its appended range. With growth, keep the old
object rooted and copy both pieces into the new allocation. Never retain
a data pointer across a realloc of its owner. For `a += b` with two private
locals, `b` is a consumed read-only operand, not adopted character storage.

Failure must follow the executing path's actual contract. The current
specialized handler clears its target after `PyUnicode_Append`, including
on failure; generic arithmetic can raise without performing its folded
store. Do not normalize these paths to an invented new rule. Allocation,
overflow, fallback, traceback and monitoring controls must assert the
proper local binding, pending exception and retired owner state on each.

## 7. Linear-growth claim and its limits

For one successful private epoch, let `L0` be its initial shared prefix,
`A` the sum of appended character lengths, and `k` the number of appends.
Geometric reservations bound prefix recopying by a constant multiple of
`L0 + A`; only finitely many representation widenings are possible.
Right operands contribute `A` copied units. Required time is
`O(L0 + A + k)`, with logarithmic growth allocations and bounded geometric
slack. Exact constants must follow from the implemented capacity sequence
and character widths, not a fitted benchmark threshold.

Publication creates another epoch. If an observer retains every growing
prefix, the observable output itself can require quadratic storage/copy
work. No universal linear promise is made for that program. Unknown
read-only consumers initially seal too: `len(s)` through a generic call
can therefore remain quadratic even if it retains no alias. Record that
deficit separately; the string/compiler owner must review a reusable
guarded no-escape consumer rule before I if it matters. Existing `CALL_LEN`
checks the callable but invokes the general length protocol and may box
large results (`bytecodes.c:4582`); its name alone is no such proof.

Interrupted/observed epochs and reserve-failure retries must appear as
such in measurements. They cannot be removed from a growth plot or
reported as successful uninterrupted epochs. Memory evidence includes
all unreclaimed old buffers, published slack, owner bitmap/code metadata,
peak RSS and collector pauses; fewer allocations alone is not closure.

## 8. First failing controls and implementation split

Each rung starts with a retained failing oracle or an explicitly labeled
disabled-feature fixture. A positive must witness the actual native
private path at the intended function/site; a green generic test cannot
prove it. Reversible negative patches name their exact barrier/root/
capacity check and source hashes. Where independent guards mask a removed
barrier, assert the owner state at the actual observer entry rather than
claim an unobserved mutation. Restore production before qualification.
S1 may track owner transitions in shadow mode on existing copying results;
those positives establish only certificate/revocation behavior. They do
not establish mutation, capacity or linear growth, which remain S4 claims.

| Rung | Reviewable work and required controls |
|---|---|
| S0: proof/owner prerequisite | The focused companion specifies transfer/join/exception rules, final physical-word verification, one pre-CREATE code-owned plan and current-frame segment/local-bit transitions. Independent review, then H3/code-layout and H1/ROOTS interface decisions; source-only, no implementation. |
| S1: proof only | Certificate derivation/verification, optional per-thread owner bitmap and revocation, mutation disabled. Test ordinary/implicit reads and writes, including `LOAD_FAST_AND_CLEAR`, comprehension save/restore and exception-target clearing; copy/store/compaction remapping; both append spellings; constructors/replace/marshal/freeze; and exact owner retirement at frame export/call/return/unwind/detach. H1/ROOTS/H3 approve their shared interfaces first. |
| S2: flat capacity helper, not enabled by Python append | Unicode owner decides section 5; implement constructor initialization, capacity arithmetic/accounting and private transaction controls. Exercise normal/debug allocators, exact capacity, reserve failure with successful exact retry, actual final allocation failure, overflow, each width transition, embedded zero/surrogates, self-append and two private operands. No API/layout claim based on one ASCII case. |
| S3: closed observer/foreign-entry inventory | Complete section 4 and collecting-root controls before enabling mutation. Aliases through another local/container/cell/global/constructor; pre-existing locals proxy; Python/C frame getters/cache/rebinding; warmed/cold reflected fallback; monitoring/trace/profile enable/disable and exceptions; pending/finalizer/signal callbacks and another thread at detach. C stashes retain object plus DATA/UTF8 pointers across append, growth, widening and collection. Active reftracer/custom allocator modes prove fallback. |
| S4: enable existing exact append path | Use the existing handler protocol; no new opcode/fusion required. Guard failure seals before dispatch. Require actual private hits and exact successful/exceptional ownership states, root-removal controls at collecting growth boundaries, full affected correctness gates and the growth acceptance below. Only this rung may claim interpreter recovery. |

The C stash fixture must itself have a reviewed collector root/traversal
contract; otherwise a stale pointer proves nothing about preservation.
Generator/coroutine/cell/externally owned cases prove correct fallback,
not private-path coverage. Tests also check hash/dict/set membership,
interning, iterators retaining their original string, identity-returning
constructors, pointer/length/terminator stability and ordinary Unicode
writer behavior after publication.

## 9. Acceptance and bounded handoffs

After source review, the coordinator chooses the exact normal/debug/kill/
ASan lanes and integration ladder under PROCESS. Affected areas include
Unicode/string operators, compiler/register IR, code construction/marshal/
freeze, frames/locals, trace/profile/monitoring, C API/writer and collection
root controls. Existing tests are preserved. No tests or benchmarks have
been executed for this design.

Growth evidence must sweep both initial-prefix length and append count,
including empty and long shared starts, varying chunk lengths, all widths,
self-doubling, observed aliases and read-only/generic-call interruptions.
Use the unchanged `str_concat`/`str_concat_loop` workloads as points in
that matrix, not its definition. Record private hits/misses and seals by
reason, reserve failures, capacities/growth allocations, actual prefix/RHS/
widening bytes copied and live/peak storage. Check recorded sequences
against the analytic geometric bound. Removing geometric growth must
restore growing prefix-copy cost and fail the oracle.

Use matched release wall time and instruction measurements with all JITs
off, plus allocation/box/RSS/pause attribution; counter builds explain
mechanism but do not substitute for release timing. Show compiler/handler
and metadata overhead on functions that do not concatenate strings too.
Preserve noisy or slower rows and their uncertainty. The coordinator owns
checkpoint-I whole-workload attribution; this design owns its actual
copy-growth/alias correctness evidence, not unrelated tuning.

Required decisions before S1 implementation: independent S0 readiness;
H3 approval of the selected certificate field/constructor lifetime and
complete semantic decoder; H1/ROOTS owner-segment/pre-write/root interfaces;
exact shadow scope and gates. Unicode capacity/ABI approval belongs to S2,
and full observer/foreign-entry closure to S3, before S4 mutation. These
are pre-I dependencies. No proof, storage choice,
native implementation or gate outcome is preapproved by this document.
