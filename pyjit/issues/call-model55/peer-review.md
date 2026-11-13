# Model55 independent design review

Review date: 2026-11-12. **Current verdict: GO for the design contract** after
the independent post-review below. The initial five MUSTFIX findings and
correction sketch are historical review evidence; the corrected 481-line
proposal resolves them. This is not runtime implementation or performance
approval.

Scope: source and design only. No runtime source,
generated products, builds, tests, profiles or measurements were changed or run.
The reviewed proposal is `design/proposals/register-call-model55.md`, frozen
at 346 lines for this review. The source baseline inspected is
`build/worktrees/frame54-cold-construction` in the sibling
`python-boil-the-ocean` checkout, whose HEAD is
`61015a9452d9069646fb5fede9ff963896833b89`.

## Initial verdict (346-line proposal; superseded)

The proposed direction is sound enough to develop into the implementation
contract. It addresses the whole lifecycle, treats the bitmap as a real rival,
preserves existing fast calls, and separates Python behavior from the selected
RC policy. It does not yet close the requested complete-plan checkpoint.
The five MUSTFIX findings below require concrete contracts before implementation.
They do not require another round of broad architecture research or isolated
runtime experiments. Resolve them in this proposal and have the coordinator
check the resulting interfaces and examples.

No reviewed evidence proves that the static-map representation is faster or
globally optimal. The recommendation is an architectural hypothesis. The same
transfer ABI and lifecycle gates should evaluate both ownership representations.

## What the source supports

The proposal accurately describes the important existing benefits. In selected54,
`Python/bytecodes.c:4331–4395` guards and enters exact calls with overlapping
argument homes, one reserve and one initialize. Heap callers move their inputs.
`Python/frame.c:204–238` clears only named/constant locals at ordinary thread
exit after asserting scratch is empty. `Python/ceval_macros.h:505–546` consumes
scratch by moving its owner to a C local and NULLing its home; local inputs
remain borrowed, and `IR_TAKE` duplicates them when an owner is needed.

`Include/internal/pycore_interpframe.h:210–228` explicitly couples scratch
numbering to reverse frontend stack-pop retirement. Removing that restriction
is justified: required evaluation/control effects remain ordered, while exact
CPython destruction timing is not a Python data-model requirement. The
replacement still needs bounded retention and coherent reentrant publication.

`Python/gc_trace.c:1025–1046` conservatively scans current value/header ranges
and whole older chunks specifically because unlinked retiring frames still
hold owners. The proposal correctly refuses to enable stale scratch while
that arena scan remains. Its independent typed code/function pins also retain
a real requirement: mutable `function.__code__` cannot replace an executing
code lifetime anchor.

## MUSTFIX 1: specify the executable map and phase interface

Proposal lines 86–101, 141–144 and 234 describe static authority but do not
define the state key or its representation. Saying that a hidden condition
must be recorded does not identify where it is recorded, when its write is
needed, or how handlers/root walkers find the matching map. This is the central
cost tradeoff, not a detail that can safely wait until implementation.

Concrete source cases include generic method-form `LOAD_ATTR` with a nullable
second output (`Python/bytecodes.c:2609`), unpack helpers with intermediate
results (`1881–1957`), and SEND/FOR distinct successor states. A generic handler
can have input owners in C locals when an allocation or callback occurs.
A PC alone does not describe these states. Nullable **initialized** mapped
values may be a valid solution; stale unmapped cells must not be read to infer
their state.

Correction: define a minimal record/schema, even if field widths remain
provisional: canonical site ID, finite phase ID, initialized scratch locations,
owning locations, handler/observer successor, and transition-root locations.
Choose a default map encoding (for example interned sorted ranges plus explicit
small exceptions), a direct site-to-descriptor index, and the exact publication
operation. State whether every observable boundary has a statically selected
descriptor address or does a runtime lookup. Define normalization at joins and
the permitted dynamic discriminator for conditional output/partial progress.

Include worked traces for inline arithmetic, callback-capable arithmetic,
conditional method output, partial unpack failure, exact overlap call/return,
exception entry and yield/resume. At each callback show the unique owner of
every affected value and every new executed map/phase operation. This makes
phase explosion, hidden classification and lookup costs falsifiable.

## MUSTFIX 2: make retirement roots a collector interface

Lines 209–217 identify the right obligations but leave the root record and its
lifetime undefined. A fixed K does not itself make a record safe during nested
finalizers, arena growth, collection or escaped-frame mutation. Keeping the
remainder of an unlinked frame reserved does not make it reachable after the
conservative arena scan is removed.

Correction: define a thread-linked retirement/transition record with previous
record, owner source (activation plus phase/cursor), detached bounded batch,
result owner and storage reservation. Nested evaluator entries must push a
distinct record and restore the previous head; one mutable thread-global batch
is insufficient. Both unprocessed mapped owners and detached owners remain
enumerable. Before decref, detach the current batch entry so a reentrant walk
does not visit an owner whose object may already be finalizing. Define rooting
for the independent typed header owners until their final releases as well.

Specify separately how the tracing root walk and cyclic-GC edge subtraction
consume these records. An external strong C/root owner is not automatically a
second container edge. Include heap generators and escaped frames, not just
thread chunks. Pin descriptor/code metadata until the last cursor/map reader
finishes. Give the escaped-local mutation route an explicit state rule: either
it updates a still-authoritative live binding/map or the frame has already
become a separate escaped owner; it cannot insert an owner behind a completed
retirement cursor. This is an interface requirement, not a mandate to preserve
old decref order.

## MUSTFIX 3: choose immutable descriptor and mutable execution boundaries

Lines 86–101 call the execution descriptor immutable, while lines 142–144 and
342 permit specialization/instrumentation changes. The current execution
words are mutable. A map selected by an old physical handler number cannot
remain authoritative if instrumentation or specialization changes its ownership
or callback phases. Conversely, rebuilding generic metadata on every dispatch
would move the current decoding cost rather than remove it.

Correction: give each word a stable canonical semantic site and phase contract.
State which specialized/observed handler variants must preserve that contract,
and how a genuinely different contract gets a new validated execution image.
Define descriptor/image lifetime, guard invalidation and the callback refresh
point, including suspended generators and return continuations. No frame may
retain a map pointer into freed/replaced metadata. Explicitly keep ordinary
destination/operand variants preselected; reserve generic descriptor decoding
for cold observation/unwind or identify and count each hot use.

## MUSTFIX 4: settle operand encoding and public image admission

Constant-pool operands and transfer IR are valuable changes, but lines 135–144
provide no encoding choice or admission rules. The current eight-byte word has
four 12-bit operand fields, reserved discard encoding and overloaded feedback/
immediate roles (`design/register-ir.md`, `Include/internal/pycore_ir.h`).
An extra operand class cannot be assumed free. `code.replace(co_consts=...)`
can change constant type/ownership; malformed public words and serialized
metadata cannot acquire trusted ownership facts from the compiler by assertion.

Correction: choose a bounded encoding strategy now: distinct generated forms,
side operand plans or an explicitly versioned wider format. Give literal/index/
register limits and behavior at their boundaries. Identify a single validation
entry shared by compiler assembly, code construction/replacement and marshal
loading; list the effects verified before image publication. Constant lifetime
must follow the executing code pin, with actual replacement values validated.
Add `Python/assemble.c`, `Python/regcfg*.c`, `Objects/codeobject.c`, marshal,
disassembly and bytecode magic/version ownership to the relevant milestones.
Do not invent a permanent dual ABI to avoid updating these producers/readers.

## MUSTFIX 5: turn migration into reviewable representation checkpoints

M0–M5 has sensible lifecycle gates but M2 bundles register allocation, constants,
GC authority, retirement and observers into one broadly named scope. M4 then
introduces semantic lowering that M2 already depends on. Neither a shadow-map
comparison nor the bitmap comparison has an exact equivalence relation yet.
Comparing every set owner bit against non-NULL cells is wrong for inline values,
initialized non-owning placeholders and transferred overlap aliases.

Correction: provide an ordered dependency table with named outputs and file/
interface owners: (1) transfer/phase schema and complete reader census;
(2) typed metadata and validated image admission under NULL homes;
(3) generated shadow scratch authority and retirement-root collector bridge;
(4) complete observer/escape/generator adaptation with NULL homes retained;
(5) one coherent static-authority switch and stack-order removal;
(6) equivalent bitmap branch; (7) composed lowering/locality and checkpoint.
Move necessary semantic multi-exit contracts before the authority switch; leave
optional release fusion and local-return moves after it.

Define shadow assertions against logical initialized/owner state, including
nullable outputs, C owners and overlap partitions. Compare static and bitmap
branches at the same transfer/lowering/image commit; keep the final integrated
candidate's performance control as selected54, with selected51 a separate
historical comparison. List concrete native path checks and benchmark falsifiers
per representation switch. A failed gate keeps the complete NULL-authoritative
checkpoint selected; it does not allow release builds to guess representation.

## SHOULD FIX

- Give K and retention bounds provisional operational meaning. A batch bound
  alone cannot bound retained bytes; use operation completion/backedge/poll
  limits and include peak retained graphs and resources in the planned evidence.
  Name the intended RC policy without claiming Python requires its timing.
- Add a compact entry matrix: exact overlap, keyword/default plan, heap caller,
  native/vector provider, PEP 523/instrumented, mapping-backed exec/class,
  generator creation and public frame constructor. Mark PRE allocation, commit
  owner transfer, failure authority and cold auxiliary allocation for each.
- Include metadata bytes, phase writes, code-size and instruction-cache effects
  in the ledger alongside removed stores. The static design can lose when a
  NULL store becomes a dependent descriptor load and phase write at every
  release. Typed pointers can reduce classification yet add no native benefit
  if the compiler already proves it; preserve that distinction.
- Retain the strong terminal unobserved-return proof gate. Check return hooks,
  monitoring changes, frame-back discovery and retained generator frame
  association before moving a named local. Do not turn this optional improvement
  into a prerequisite for basic map correctness.

Once the five contracts above are integrated, this architecture can receive a
positive design verdict without a performance claim. Runtime implementation,
semantic qualification and measured selection remain subsequent work.

## Concrete minimal correction sketch

This is a review suggestion for the coordinator to settle, not an implemented
ABI. It bounds the missing choices without requiring a generic map interpreter
on the ordinary instruction path.

```text
Activation hot state:
  typed strong code, function; previous; base; saved_top
  execution_image; canonical_pc; small phase_id
  continuation/recipient; existing direct environment/feedback views
  mapping owner; nullable frame association; storage/entry kind

ExecutionImage (pinned generation):
  immutable canonical site descriptors and phase tables
  mutable handlers/feedback, constrained by each canonical site contract

SiteDescriptor:
  phase_table offset; normal successor(s); exception recipe ID
  preselected operand/result/transfer variant

PhaseDescriptor:
  interned initialized-scratch ranges + small explicit exceptions
  scratch owner ranges (authorized PyValues, not only mortal values)
  transition-root shape; observer recipe; exception successor

TransitionRecord (nested thread chain):
  previous; activation; pinned image; map/phase; release-list cursor
  reserved storage; result root; bounded roots[K]; populated-root mask
```

Local boundness stays canonical local/NULL, including explicit virtual kwargs
state. Scratch maps authorize initialized values even when inline or immortal;
tag classification determines whether they carry a mortal edge. A bitmap branch
adds physical mortal bits, not a replacement for initialized-value authority.
At a publication site, generated C selects a literal phase ID and writes the
semantic PC/phase; walkers perform the cold table lookup. A callback-free inline
handler needs no map lookup or extra phase write per input consume. Its next
publication supplies the successor phase directly. If a genuinely conditional
nullable result is canonical and initialized on every route, map it and store
NULL on the absent route; do not add a dynamic discriminator unnecessarily.
If routes authorize different storage, branch to distinct phase IDs before a
callback or normalize them with explicit edge moves.

| Boundary trace | Published authority and hot work |
| --- | --- |
| Inline ADD of scratch a,b to r | PRE maps a,b; guards prove inline result; callback-free compute/write r; next publication maps r. No a/b NULL stores, bitmap updates or runtime map decoding in the static form. If guards miss, publish the fallback phase before escape. |
| Callback-capable binary helper | Move consumed scratch a,b into transition roots 0,1; publish phase with a,b excluded and roots included; call helper. On success publish r before retiring roots; on error publish the error phase with no r. Borrowed local inputs require an independent pin before a callback that can rebind them. |
| Partial unpack failure | An adapter owns the produced prefix in registered transition roots or an explicit initialized prefix shape. It advances a bounded prefix/cursor before the next fallible step. On failure the published phase owns that prefix and remaining source; cleanup detaches each root before release. The normal exit moves all outputs into the common destination map. Never infer prefix length by scanning stale cells. |
| Conditional method output and join | Either initialize both result homes with canonical callable/self-or-NULL, or select two finite POST phases and normalize at the join. A callback sees one chosen phase, not a union that reads an uninitialized second home. |
| Exact overlap call | PRE caller maps callable/args and survivors; all survivors lie below the clobber base. Commit moves callable to typed child owner and transfers arg cells without stores or count changes. Suspended caller phase excludes that interval; child entry phase owns it. No callback occurs between authority changes. |
| Child return with reentrant retirement | Detach result into the transition record, unlink child, retain its map/cursor and reservation. Finalizers may push other records. When child retirement finishes, fetch the caller destination's current owner, publish result/continuation, then retire the displaced owner under a new defined phase. |

The transition-root contract must include ordinary generated C temporaries,
not only teardown batches. A dedicated adapter can expose a bounded output
prefix when K is insufficient; it cannot silently leave arbitrary intermediate
owners solely in C variables after removing their arena roots. Such roots are
external counted owners: the tracing root visitor marks them, while cyclic-GC
subtraction follows each container edge once according to the existing collector
accounting. The implementation review must verify that distinction.

During retirement, advance the cursor and clear the record's populated-root bit
before decref. Remaining list entries stay in the activation's published map.
Reentrant execution can grow chunks but cannot reuse the reservation. Escaped
frame takeover transfers the map and remaining values atomically into separate
storage, after which mutation targets that separate frame; an unlinked private
retiring activation rejects new local-owner installation. The code/image pin
outlives both map enumeration and root-record removal.

Specialization and monitoring can patch a word only within its canonical
site's certified phase contract. A contract-changing transformation allocates
and validates a new image generation. Active and suspended frames retain the
old generation, or switch only at a declared normalization boundary with their
phase/continuation reconciled. Callback refresh may reload dispatch/feedback
views but cannot replace the frame's descriptor authority accidentally. No
generation counter load is required per inline operation if the image pin and
boundary protocol provide that guarantee.

Field widths, K and the precise range encoding remain implementation choices,
but the coordinator should select initial defaults before closing the design.
The fixed/variable division above is concrete; an exact header byte target is
not an acceptance criterion. The required next review is of the corrected
design and traces, not of unrequested implementation or test results.

For handler hints, use the existing 16-bit handler selector for narrowly emitted
ownership variants; do not assume a spare word bit. For example, a producer
whose destination is known at assembly chooses `new scratch`, `replace local`
or `discard` execution form. The constructor verifies its operands against
geometry. Specialization and instrumentation must preserve/recover that form
through generated canonical/family mappings. Call admission records the chosen
return form in existing continuation state without testing local-versus-temp
geometry again on return. Retain direct return PC/slot caches initially: ordinary
return executes its selected inline completion, not another interpreter dispatch
or C adapter boundary. Introduce only reached variants and measure code growth.

If handler space or combinations make that unsuitable, M0 must deliver a bounded
alternative before implementation: a per-site immutable plan with a specific
direct accessor at each necessary handler, exact dependent-load count, and no
generic per-bytecode operand-plan decoder. Constants likewise need an initial
choice such as distinct pool-index load forms preserving full 12-bit register
indices, with large pool indices in an explicitly assigned existing immediate
field. Out-of-range mixed forms lower to an explicit load plus ordinary register
operation; no truncation or invented spare operand bit is allowed. Verify word,
pool and branch limits and bump format magic when serialized geometry changes.

These changes extend the already guarded cached positional/default/keyword
binder; they do not remove that binder anew. Their claimed additions are trusted
operand ownership/result forms, non-stack scratch allocation, per-code constant
access, authoritative publication maps and fused retirement. Exact calls keep
their current guard and overlap advantages. The ledger must price any newly
introduced plan loads against those specific additions.

## Independent post-review: corrected 481-line proposal

**GO for the design contract.** I read the corrected frozen proposal, including
the seven authority traces, collector interface, field/entry tables, encoding
and validation rules, and M0–M7 dependency chain. All five initial MUSTFIX
findings are resolved at the design level. No additional blocking invariant
was found. This review authorizes no runtime work by itself; the current task
remains research and planning.

| Initial finding | Resolution checked in the corrected proposal |
| --- | --- |
| Executable map/phase interface | A stable canonical word SiteId plus literal uint8 PhaseId selects interned initialized and owner ranges. Shared families use fixed role IDs. Nullable outputs contain canonical NULL; variable unpack has a registered prefix and progress cursor. Seven traces show exact ownership at callbacks, errors, joins, overlap, retirement and suspension. No map decode or phase write is required for each scratch consume. |
| Retirement/collector roots | Lazy nested records cover detached C owners, results, unprocessed mapped/header owners and independent dynamic prefixes. The cursor/populated bit changes before decref; reservations prevent reuse but do not substitute for roots. Tracing marks external owners; cyclic-GC subtraction counts real container edges once. Escaped authority is separated before retiring cursor mutation. The descriptor remains valid until the last reader finishes. |
| Immutable versus mutable execution | The independent existing strong code pin retains descriptor/image authority without a new per-call image RC edge or ownership cycle. Handler patches preserve canonical operand/phase contracts and geometry. Contract-changing transformations require a new validated code/image; callback refresh cannot silently substitute descriptor authority or mutable function code. |
| Actual word format and admission | Narrow variants use the existing low16 handler selector, preserving the four full 12-bit fields. LOAD_POOL CD24 and bounded RP12 forms have explicit overflow/lowering limits. One proposed admission entry covers assembly, public construction/replacement, marshal/frozen loading and new images, including ownership/phase/root facts. Ordinary return remains inline with direct continuation caches. |
| Migration and comparison | M0–M3 establish schema, typed admission, shadow authority, collector roots and every reader while retaining NULL homes. M4 changes authority coherently with non-stack coloring. M5 compares bitmap under the same transfer/image contract; optional constants, release fusion and return moves follow in M6. M7 uses selected54 as current control and preserves diagnostic limits. Shadow equivalence compares logical initialized values and unique owners rather than NULL occupancy. |

The important added cost caveat is now explicit: unbound callee locals can
overlap stale caller scratch and require fresh canonical NULL initialization.
That work was sometimes avoided by the selected runtime's warm initialized
frontier. The proposal must price those stores against removed scratch clears;
its case no longer depends on treating all initialization as removable.

The corrected contract also excludes a bank of borrowed mortal scratch homes.
A materialized mortal scratch value has one owner in its authorized home or
has moved to another authority, after which the old home is unreadable. This
keeps constant/binding borrowing distinct from frame-owned scratch and avoids
a hidden lifetime dependency across callbacks.

One nonblocking scope clarification remains useful for M0: the bitmap section
defines bits for all physical value cells, including initialized locals, while
M5 describes changing only scratch ownership representation. Keep identical
initialized-local semantics on both branches and explicitly count local mortal
bit maintenance if using the stated all-cell bitmap. Do not let the comparison
silently add a different local-boundness design on one branch.

The runtime evidence gates remain substantive work. In particular, release/debug
layout verification must confirm PhaseId placement; generated shadow authority
must exercise actual partial failures and callbacks; collector review must
verify external-root versus container-edge accounting; native inspection must
check the fast-close/outlined-record split and absence of new inline lookups.
Neither these obligations nor provisional K=8 require another broad design
review before this planning checkpoint can close. They are explicit migration
gates with coherent fallbacks.

This is source/design review only. No code was implemented and no build, test,
native job or benchmark was run. The design is ready as an actionable target;
correctness, free-threaded support, achieved speedup and runtime selection are
not established by this verdict.
