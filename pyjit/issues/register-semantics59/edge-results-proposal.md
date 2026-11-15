# Separate builder input spans from result allocation

Status: proposal for lane 2, before runtime implementation. Source reviewed:
selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0` in
`BASE/worktrees/call55-integrated`. The coordinator must approve this contract
before the worker changes `pyjit/reg59-edge-results`. No native execution,
build, generation or test was performed for this proposal.

Implementation update: root approved the proposal. The uncommitted source in
`R/call55-ir` now implements the complete builder mapping and shares lane 1's
reviewed final-output API, including its post-emission owner receipt. Four
focused host source methods pass (1.034 seconds), and the runtime fixture parses;
no worker native execution, full generation or build occurred. IR's actual
compiler/admission/lowering/fixture peer is GO, with final real-jump fixture
readback pending. Root owns the coherent generation, native and extent gates.

Use **BUILD_LIST and BUILD_TUPLE as the first complete family**. FOR_ITER follows
only when its pristine, specialized, instrumented and generator fallback paths
all support the same geometry. Lane 1's direct local FOR publication still
vacates B+2; it is not allocator freedom or a newly discovered BODY outcome.

## Existing coupling and concrete opportunity

Both builders already have a fresh result value in RegCFG `out[0]`
(`regcfg.c:498–575`). Their checked helper contract steals the input owners on
success and leaves them intact on failure (`bytecodes.c:2294–2311`,
`ir_generator.py:5992`; `tupleobject.c:610`, `listobject.c:3010`). No new value ID,
borrowed-frame ABI or container helper is needed.

Physical lowering nevertheless treats their one output as window position zero.
For scratch results, `compact_op` assigns `base+j`; `_PyRegCfg_OpSlots` derives A
from `base+fmt->dst` (`regcfg.c:812,962`). Folded local results already bypass the
scratch assignment, but the final frame high-water pass still charges
`base+max(nread,nout,1)-1` (`:1214`). Admission bounds the window by
`max(reads,writes)` and requires scratch A to equal B (`codeobject.c:1834,1942`).
The generator clears the input tail, publishes the result in B, then moves it
to local/discard A (`generated_ir_cases.c.h:4629`; `ir_generator.py:6485`).

A concrete positive case is `first, second = [], []`: the first result can remain
in scratch while the second builder folds to a local. The latter currently
reserves a fictitious scratch output at its empty input span. The existing
`test_parallel_empty_list_assignment_then_call` is a semantic witness, but its
later call already sets a four-cell high-water, so its whole frame cannot shrink.
Root's selected baseline capture under `E59/builder-small-baseline-*` confirms
that `first, second = [], []; return first` has two scratch cells, including
the second builder's fictitious output. Use that smaller shape for the proposed
two-to-one extent assertion. Nonempty builders still need their input extent,
and a single empty builder retains the runtime's minimum scratch allocation.

## Contract and representation

Keep the eight-byte word and its existing roles: A is the result recipient, B
is the input-span base, and C is the count. Conceptually:

```
InputSpan(B, count) --steal on success--> EdgeResult(out[0], A)
```

Reuse the existing `_PyIR_OUTPUT_REGISTER_DST` mapping **with** `_PyIR_WINDOW`
for the checked builders. WINDOW describes their contiguous inputs; REGISTER_DST
describes their one independent result. Keep `reads=count`, logical `writes=1`,
`through=0` and the existing frontend result index. Do not add a second result
ID or an unused runtime descriptor. The current `out[0]` and folded `op->dst`
provide the compiler reference and recipient.

Use the lane 1 declarative final-output binding at the fallthrough commit,
with result index 0, destination ROLE_DST and **no staging source home**. This
policy differs from FOR's VACATE_SOURCE: a zero-input builder owns no B cell
that must be cleared. The checked source contract and actual final-publication
receipt must agree; a generator annotation alone grants no ownership authority.

Add one common physical-window-extent query. For an admitted window with
REGISTER_DST outputs its footprint is the input/retained span, here `reads`;
legacy window/output mappings retain their current extent. Logical result
counts remain available through `writes-through`. Route bounds and high-water
consumers through that query rather than changing `writes` to zero or scattering
BUILD_LIST/TUPLE name checks. No format-row or hot frame/header growth is needed.

## Compiler and publication

For the migrated mapping, `compact_op` allocates `out[0]` independently of the
input span. A folded local or discarded result has no scratch result allocation.
A scratch result uses its existing required canonical home, or the existing
compatible consumed-input/top choice; `_PyRegCfg_OpSlots` reads its actual slot.
The verifier stops requiring this result to equal B and still checks consecutive
inputs, live-owner exclusion, canonical region entry and prefix ordering.

The first packet does not scan arbitrary holes, repack future arguments or add
edge moves. Canonical names already selected for EH prefixes and CFG joins stay
fixed. A future placement into a different hole needs a real next-consumer or
materializing-transfer consumer; it cannot simply cause the next window to
fail verification. This first change admits independent scratch geometry and
removes real local/discard phantom reservations without claiming general SSA
allocation or producer-to-consumer coalescing.

On helper failure, leave inputs and recipient unchanged and take the existing
ERROR_NO_POP path. On success, retain the acquired container owner in C, clear
**exactly** the consumed input span without closing its transferred owners, and
publish directly to A using the existing normalization and displaced-local
retirement protocol. A may alias any consumed input, since every input has
already been stolen and detached before publication. Local A replacement is
visible before its displaced owner can finalize. Discard closes the acquired
result through the existing release/error protocol. No input close loop or new
per-element owner acquisition is added.

For count zero, form no B+1 pointer and perform no home read, clear or write
through B. In particular, replace the old tail-clear shape rather than retaining
`&window[1]` with a negative/zero count. The empty span may use an encodable
one-past-frame B; all helpers and generated pointer expressions must honor it.
Input allocation can run GC, so the current pre-call phase publication and
canonical EH-prefix materialization remain. Do not move local commit before
the last fallible operation or infer sparse EH authority from the result binding.

## Admission and cold readers

Update these as one contract, not separate optimizations:

- `check_words`: validate the real input span and A independently. For count
  zero allow B at the frame limit, including the early role-field check;
  reject negative/overflowing counts, out-of-frame spans, invalid A and
  unsupported sentinel use. No nonempty span may start at the limit.
- Semantic reproof: A inside the consumed span is allowed on successful steal.
  A outside it needs positive NO_HOME with no MAY_OWNER obligation. Its fresh
  scratch publication writes without reading the previous bits. An unrelated
  live owner, retained protocol cell or unknown incoming state is rejected.
  Local A uses replacement semantics; discard is not a physical home. NO_HOME
  does not establish an initialized or physically NULL read.
- `_PyIR_DefinesHome`, `_PyIR_ObservedHomes`, physical transfer and normal
  absence: result is A; consumed inputs other than an overlapping A are empty
  after success; unrelated B for an empty span is untouched. Remove the old
  NORMAL_WINDOW_RESULT source-vacancy claim for these builders. Retain honest
  UNKNOWN readability/effect coverage where the current graph has no proof.
- Read validation, observation/recovery and kwargs analysis must not treat
  logical `writes=1` as a B write. `_PyIR_ReadsHome` continues to read only the
  input span. Audit every `fmt->writes`, BASE extent and window-output reader,
  including `_opcode`/disassembly metadata and raw-image fixtures.

Version the serialized bytecode contract and update explicit magic fixtures:
the same bytes may now name independent A and an empty, one-past B. Keep
constructor, replace, marshal, optimizer and feedback admission in agreement;
no legacy-format fallback. FOR and all other nonmigrated windows retain their
current geometry and source-vacate behavior.

## Bounded files and gates

Implementation files: `pycore_regcfg.h` comments/query interface, `regcfg.c`
compaction/slots/verifier/high-water, affected store/discard passes only if their
mapping query changes; generator `IRFormat`/binding selection, `SlotLayout`,
final flush and actual receipts; `pycore_ir.h` extent query and generated format
metadata; `pycore_ir_outcomes.h`; `codeobject.c` bounds/semantic reproof/kwargs
reader; relevant `_opcode`/disassembly format consumers; magic and affected
fixtures. `assemble.c` must encode the actual chosen A, not recompute B+offset.
No changes to generic CALL staging, frame layout, getters, tags or container APIs.

Focused source checks must emit both builders in both record modes and assert:
all success inputs detached once, one result publication, no staging move,
no zero-count B dereference/B+1 expression, no artificial result extent, and
unchanged failure input ownership. Poison the helper/source contract, insert an
early output spill/address escape or extra fallible operation, and require the
binding to revoke or generation to fail.

Runtime/admission coverage should extend window-result and container/owner tests:
zero and multiple inputs; local, scratch and discard; A at each consumed input
and at a separately proven vacant home; live/unknown unrelated A rejected;
empty-span B at limit accepted and nonempty/out-of-range B rejected. Exercise
constructor/replace/marshal routes, allocation failure, mortal/inline inputs,
result/input/local aliasing, finalizer local mutation+GC, tracing/monitoring,
try/finally and source recovery while a result is live. Retain exact failure and
retirement witnesses; do not loosen them to admit the new geometry.

Before measurement, retain pre/post CFG/word/frame-extent witnesses for the empty
parallel assignment and at least one independent scratch destination. Native
review must show no staged B result and no new helper/spill on ordinary paths.
Count any retained or added materializing move. The coordinator owns generation,
117/79 builds, focused runtime gates, unchanged hot/sentinel screens and broad
selection. If compiler outputs/frame extents do not change, or canonical/prefix
materialization cancels the savings, this packet has not met lane 2's benefit
gate; do not expand it into the FOR family on representation alone.

## Fresh publication correction after bootstrap admission

The first builder packet used the legacy `IR_STORE_DST` scratch assertion,
which reads the previous word and requires NULL or the same result. Its
admission therefore also demanded SEM_NULL. Bootstrap exposed legitimate
compiler recipients with positive SEM_NO_HOME but no physical NULL fact after
uncovered normal transfers. An independently certified SET_NULL receipt must
be consumed correctly, but expanding old physical-transfer coverage is not a
prerequisite for an independent fresh result.

Use `IR_STORE_FRESH_DST` only at the mandatory final builder binding. A
compile-time FRESH parameter in the existing publication implementation skips
only the scratch old-word assertion. It retains result normalization, the
new-value check, local publication before displaced-owner retirement, and
discard cleanup. Existing publication macros pass FRESH=0 unchanged. The fresh
macro adds no runtime mode branch or intermediate NULL store.

This is a write capability: a scratch result outside the consumed input span
must have positive NO_HOME and no MAY_OWNER; an overlapping input has been
consumed by the successful helper and detached before publication. Neither
case authorizes reading the previous word. Unknown state and live owners stay
rejected. Cold readers must describe A as the new result only after the commit;
they acquire no incoming NULL, initialization, canonicality or readability fact.
The frame arena's existing physical NULL cleanup contract remains unchanged.
