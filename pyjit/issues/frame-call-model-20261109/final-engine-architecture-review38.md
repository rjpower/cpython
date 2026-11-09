# Independent engine review at the close of this round

Reviewed committed38 `24f4d270ced`, final test-only39 `4d666e1b482`, and H's
deferred `E11/coroutine40-frozen-source.patch`. Production runtime39 is the
runtime38 implementation. The mixed N worktree is not the40 snapshot. B's
retained-owner draft and A's unfinished GDB port are excluded. This review makes
no native or performance claim beyond the retained
coordinator receipts. It applies an optimizing-engine design lens; it does not
claim authorship or affiliation with V8.

The main architecture is coherent: one canonical eight-byte value format,
explicit frame homes, overlapping call windows, real executable continuations,
source metadata distinct from execution, and transactional owner publication
before callback-capable retirement. The strongest repairs in this round were
structural: one semantic completion contract, initialized storage, exact
PRE/POST error ownership, and cold observer scopes whose authority survives
reentry correctly. These are useful foundations for an optimizing compiler.

The next five opportunities concern the execution model, not expanding the
CPython debugger's accepted jump matrix. They are future experiments, not five
remaining implementation requirements for closing this round.

## 1. Make return recipients a producer-derived ownership contract

`_PyFrame_SetReturnValue` first resolves a real pending redirect, then handles
discard or publishes to a slot. `_PyFrame_PublishReturnSlot` now only captures
old, installs new and closes old; it does not canonicalize. Golden already paid
the old-owner load/close. The retained36 plain YIELD shrank500→453 bytes while
SEND_GEN grew474→743, so neither a sampled publisher nor handler size identifies
an isolated excess cost.

A single derived recipient descriptor could distinguish a proven empty scratch
cell from replacement and discard. VM SEND's detached B+2 is a first positive
producer. This could remove an old-owner load/test on delivery while retaining
local replacement and exactly-once disposal. It must not become a speculative
hot flag or alter the IR encoding. A reviewed encoding in the existing recipient
field would still require every reader, frame constructor/copy, native entry,
debugger and remote consumer to decode it coherently. Pending redirects remain
first and replace invalidated recipient facts at actual commitment.

**Cost and gate:** classification happens per CALL or SEND resume, often one
update per delivery. It is not automatically amortized. Count updates and
deliveries by producer/class in the unchanged kernels; compare the entire
entry+return assembly ledger, including masks and code/nlocals loads. Measure
paired elapsed, instructions and branches. Current38 generators are0.992624/
golden in four pairs, so the earlier36 regression is not a reason to force this
ABI experiment now. See `return-recipient-options-after36.md`.

## 2. Construct generator/coroutine frames where they will live

Exact calls still prepare a thread frame; RETURN_GENERATOR allocates the owning
heap frame, copies the initialized frame and pops the thread reservation. The
retained36 creation handler is1002 bytes and exact-call handler1426 bytes. This
is real work shared with golden, not automatically a newly introduced loss.
Direct binding could remove reservation/copy/pop for creation-heavy coroutines
and eliminate an avoidable cross-storage argument move.

**Proof:** retain the selected-code pin/revalidation, defaults/kwargs/errors,
PEP523, monitoring entry, recursion, initialized GC roots and partial-construction
cleanup. A generator flag is insufficient: codegen can place GET_ITER/GET_AITER
and closure setup before RETURN_GENERATOR. Execute or positively certify the
required prolog. An escaped partial frame may not expose uninitialized owners;
failure must consume each argument once. RC with cyclic GC cannot simply adopt
a tracing-GC engine's loose temporary-root discipline.

**Cost and gate:** first count creations versus resumes/completions. Isolate a
certified exact-call/entry shape, compare allocation, bound argument moves and
copy bytes, inspect compiled text and measure recursive-coroutine and long-lived
generator workloads separately. Allocation may dominate after copies disappear.
This is a larger packet than recipient specialization and should not be bundled
with it. The current normal calling path remains a correct baseline.

## 3. Retire owners at real semantic transitions

Standalone POP_TOP/POP_ITER dispatches still survive lowering in loop exits,
matching, finally override and shared-producer paths. The old corpus count is
static, not a dynamic call-result count. The existing sink lowering already
removes many unused results; do not assume the remaining words are all missed
discarded CALLs. Attach bounded retirement sets to the actual branch, match,
return or restore outcome where its ownership changes.

**Proof:** ordinary thread clear currently asserts scratch is NULL and clears
named locals; it does not already close arbitrary retained scratch. Removing a
release word without a replacement owner transition would leak or break reuse.
Any retained-region alternative must define overwrite, NULL optional inputs,
window overlap, GC, unwind, frame escape and bounded loop retention. A finally's
pending payload is distinct from the active handled-exception link. Preserve
the actual continuation before any last-owner callback and keep its real EH
phase. Renaming a release instruction is not this change.

**Cost and gate:** obtain dynamic retirement/semantic-edge counts before selecting
one class. Start with a bounded semantic return/branch cleanup, not a whole-frame
scan on every return or a hot metadata walk. Compare removed dispatch work with
added operand decoding/closing, and test eventual collectability, alias/reentry,
loop reuse and observed error paths. Python does not require the old exact
decref point; safe finite retention and correct protocol effects still matter.

## 4. Give producer proofs a structured IR home

Current canonical_result_source and canonical_frame_transfer_input deliberately
fail closed on complete C/uop chains, binding/shadowing, cache effects and raw
CPP versus emitted AST identity. This is safe for the small admitted producers,
but the growing exact-body templates are an expensive maintenance boundary.
Likewise encoded observer_outcome facts and the constructor consume physical
output/retirement contracts that must match the emitter exactly.

The long-term direction is one structured producer/transfer fact at the existing
uop/IR analysis boundary, verified after composition and lowering, then consumed
by final publication, cold validation and future JIT recovery. Trusted helper
boundaries remain explicit. A declaration alone must not let an arbitrary C
suffix steal/close/rebind a result or skip an owner. Do not introduce a competing
tag scheme, handler-name whitelist or runtime proof dispatch.

**Cost and gate:** migrate one existing exact proof with identical generated
output and mutation rejection before adding coverage. Keep CPP/binding/alias/
spent-owner/early-exit negatives and emitted-body checks. The benefit is less
duplicated semantic knowledge and safer optimization growth; no immediate
throughput improvement is implied when generated text is identical.

## 5. Treat interpreter layout as a measured optimization dimension

The round's counters repeatedly separated instruction cost from CPI. For
example, accepted34 nbody counts were0.986041 instructions/golden but1.036633
cycles; this did not prove a source culprit. Shared completion specialization
removed VM generic calls/publication but increased combined named helper text
by1194 bytes. An unchanged source operator can also grow after shared macro
expansion or surrounding allocation changes. Copying source or symbol sample
percentages is not a reliable speedup budget.

Keep the common execution/commit blocks compact and outline genuinely cold
observation/error preparation only when the compiled path shows a benefit.
Retain one owner contract rather than duplicating entire helpers to remove a
branch. Feedback locality and the placement of specialized handlers deserve
the same care as arithmetic instruction count; no permanent tuning switch is
needed.

**Cost and gate:** use exact retained ELF/build IDs, total and hot text, normal
register/spill paths, branch counts, cache/front-end evidence where available,
and balanced paired workloads. Reject layout changes that merely move cost to
calls or another important kernel. Repeat with unchanged source and compiler
before attributing a CPI movement. Do not keep optimizing a past generator loss
after the current repeated workload no longer shows it.

## Semantics and qualification boundary

Python requires correct results, exception/control protocols and reachable
object lifetimes. It does not require immediate finalization, old object
addresses, legacy bytecode presentation or every CPython-accepted cross-context
debugger jump. This does not permit missing owners, a callback observing partial
commit, skipped normal context exits, or blanket refusal of useful compatible
ordinary jumps. Public supported frame/Pdb position behavior is a separate API
contract that should be stated and tested explicitly.

The39 tests strengthen eleven deliberate crossing refusals by continuing the
unchanged original programs and checking normal cleanup; they add no runtime
restriction. Their Pdb adjustment checks documented immediate entry and real
continuation rather than accepting two transcripts. The nine remaining ordinary
trace errors remain reported capability gaps, not reclassified successes.
H40 proposed a cold, positive first-successful-send NULL ingress for one exact
coroutine shape; it excluded prologues, created-throw handlers and alternate
entries. It is not integrated. Its release minimal check passed, but debug
hit instrument_line's RESUME assertion on the constructor-valid fabricated
EH0-to-RESUME1 negative. The real first-send positive passed before that error.
Root reversed the exact patch and froze39 rather than expand the wrap-up into
another optional debugger capability. Preserve that constructor/instrumentation
boundary issue and its receipt; it is not evidence of a hot resume regression.

Full38 release ran54,641 tests with three failing modules: GDB pretty-printing,
Pdb presentation and trace transfer tests. Later focused39 results are separate;
neither they nor117-module builds replace final merged qualification. The
unfinished GDB patch is tooling work and is explicitly excluded from the runtime
or throughput gate under the user's close-round direction. Root owns final39
identity rebuilding, debug/full qualification and final unchanged benchmark
campaigns. The five opportunities above do not postpone that close-out.
