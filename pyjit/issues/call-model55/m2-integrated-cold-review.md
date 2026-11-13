# M2 integrated cold source review

2026-11-12. Reviewed integrated `217fcd4a4f3` and the retirement changes
after `ce71fcfc240`, including generated RETURN handlers. Source disposition:
**GO for the bounded NULL-authoritative retirement and observer-prefix bridge;
no MUSTFIX found.** This review ran no generator, candidate runtime, build,
native job or benchmark. Root owns merged execution and native qualification.
It does not admit static ownership maps or claim complete M2/M3 coverage.

## Concrete ownership review

- `Python/bytecodes.c:_RETURN_VALUE` consumes the result before unlinking the
  child and passes it by value through `_PyEval_FrameClearAndPopWithResult`.
  Both generated ordinary and instrumented RETURN handlers contain that
  result-aware call and publish only after it returns. The caller's current
  destination is therefore selected after retirement callbacks, which can
  mutate it. The retirement helper neither duplicates nor closes the result.
- `Python/frame.c:_PyFrame_TryClearExceptCode` stops before the first possible
  callback. Its positive return is the still-unclosed initialized prefix;
  negative stages distinguish association, locals, function and completion.
  Under the GIL a scalar/immortal/non-last close is callback-free; FT sends
  counted object closes to the slow path. The last-reference test is not used
  as a concurrent proof. Thread scratch remains asserted NULL and is not
  silently treated as a static owner map.
- `clear_thread_frame_slow` and `clear_generator_slow` are reached noinline
  continuations. Their automatic records are absent from the source fast
  function. Prefix decrement, home NULLing and header-bit removal precede
  each close. The code owner remains until all geometry/home reads finish;
  detached result authority spans its possible final destructor. The result
  is disarmed and the record popped before thread reservation release, with
  no intervening callback. Debug chunk/top/limit/saved-frontier assertions
  cover nested arena growth. Actual stack frames, spills and hot branches
  still require root's native inspection.
- Association cleanup is the earliest slow stage, before any slot sweep.
  An externally retained frame moves its owners into heap-frame authority
  through `take_ownership`. `_PyFrame_Copy` NULLs moved homes; a new independent
  code reference remains on the source. The retirement record drops moved
  home/function/mapping/association authority before subsequent callbacks,
  retaining source code and any original generator exception. Later proxy
  mutation targets the heap frame rather than a cursor-passed source home.
- `clear_gen_frame` publishes CLEARED, unlinks exception/current-frame state,
  and uses the shared `_PyFrame_ClearGenerator` contract also reached by direct
  generator deallocation. CLEARED removes home/function/mapping edges from
  `gen_traverse`; retained code and the actual exception field remain its heap
  edges. The transition chain marks remaining owners externally and is never
  passed to cyclic-GC subtraction. These are different uses: marking an
  exception also reachable through its real generator edge does not invent
  another counted container edge. The exception field is NULLed before its
  close; completed generators keep their code pin. The no-callback
  `generator_return_kind` handoff remains after cleanup.
- `_PyGC_TraceVisitTransitionRoots` walks the complete nested chain, precise
  home prefix, header mask, result and optional detached prefix. It never
  reconstructs geometry after code detach. `_PyGC_TraceVisitFrames` includes
  that chain, and the optional FT marking consumer also includes it. No new
  GIL subtraction visitor was introduced. This is source consistency, not
  separate FT qualification.
- `Objects/frameobject.c:retire_redirect_values_slow` registers only the
  already detached `[begin,end)` array. The new continuation is committed
  before any destructor; the cursor advances before NULL/close. A nested
  debugger mutation may change the active frame without changing the detached
  array's ownership. The prefix-only visitor has zero home/header authority
  and does not inspect mutable frame geometry. The array and descriptor outlive
  all callbacks; the chain is popped before the array is freed.
- Generator classification includes the result-aware release helper and its
  exact canonical RETURN predicate matches the merged result identity
  assignment. `Makefile.pre.in` includes the transition header dependency.
  No alternate RETURN token template was added by the pending IR worklist.

## Evidence and limits

The source fixtures directly inspect precise transition IDs, bypassing native
and arena conservative discovery. They cover nested result/remaining-owner
records with arena growth, caller-local mutation, final code destruction,
frame-association destruction, generator completion/deallocation/exception
cleanup and unwind exhaustion. The observer fixture checks an abandoned
expression's remaining payload while a prior owner's destructor collects,
with and without named-local mutation, then checks chain exhaustion.

These fixtures establish useful nonvacuous bridge obligations once root runs
them. They do not cover every callback producer, every creation-failure prefix,
every suspend/throw/close shape or every debugger recipe. They do not justify
removing arena scans or NULL clearing. The fast path adds callback classification;
the slow record now includes a prefix pointer. Source separation alone does
not prove that native hot stack size or latency improved.

Root subsequently supplied retained `m2-roots-native-python` dumps under
`BASE/pyjit-link-wave1/call-model55/m2-native-*`. Read-only inspection confirms
owned slots at offset 112, function at 32 and the prefix-bearing transition
record at 40 bytes; `sizeof(_PyInterpreterFrame)==120` includes its one declared
slot. `_TAIL_CALL_RETURN_VALUE` reserves 16 stack bytes and contains no automatic
transition record. The separate `clear_thread_frame_slow` reserves 48 bytes
after saved registers and contains the stack canary plus record link/unlink;
`clear_generator_slow` reserves 72 after its saved register, also with a canary.
The fast thread clear saves three registers plus 8 stack bytes, retains its
result in a register and tail-jumps to the slow continuation. Fast generator
clear saves two registers and tail-jumps similarly. `_PyFrame_TryClearExceptCode`
has per-home tag/last-count tests, NULL stores and refcount decrements; outlining
does not remove that work. RETURN publishes its result only after clear returns.
These observations support the intended record-placement separation, not a
speedup claim. Root's two-pair hot ratios versus selected54 were Delta 1.010977,
generators 0.985657, coroutines 1.040258, Richards 0.993525,
comprehensions 1.014623 and logging 1.087839; root did not admit this checkpoint.

## Remaining M2: complete semantic coverage before authority

The pending IR table/worklist is **partial admission hardening**. Its temporary
facts are discarded after construction; UNKNOWN deliberately loses proof.
Definite NULL/initialization intersects, while possible owner obligations union.
A possible owner at a join does not establish a readable value on an alternate
path. No persistent static descriptor, complete exception phase proof or runtime
map authority is implemented by that patch.

1. Extend the existing generator ownership events, rather than expanding an
   independent opcode-name table. `IREmitter` consume/TAKE/OWNER/publication,
   external IO, Outcomes and regcfg transfer checks must describe one shared
   semantic contract. Cover every admitted base/specialized/instrumented family
   and fixed entry/InitCleanup image. Failed source certification becomes a
   generator/admission error before M4, rather than a permanent UNKNOWN escape.
2. Make edge outcomes explicit: success/taken/error/redirect, child activation
   commit, yield/resume and return. Calls must describe the transferred input
   window, child clobber extent, survivor constraints, folded local result and
   failure before/after commit. FOR/SEND need body/yield, exhaustion and error;
   method loads must define callable and self-or-NULL. Conditional binding
   states, hidden constant homes and NULL saved bindings retain separate rules.
3. Census ordinary-call NULL producers and sites before selecting an encoding.
   Prefer a certified family that supplies canonical NULL if existing ordinary
   call shapes permit it; compare it with an explicit NULL producer. Elided
   PUSH_NULL plus a physically empty home cannot remain a semantic definition
   when stale scratch is permitted. Update compiler/allocation, public encoded
   admission, marshal/frozen images, metadata and disassembly together.
4. Complete the worklist's all-predecessor proof. Every readable scratch input
   needs a definition on every executable incoming edge; owner obligations
   need compatible unique-owner shapes. Normalize inactive incoming homes to
   canonical NULL or reject/split the shape before using a joined release map.
   Compare virtual regcfg proof with physical emitted transfers and call
   clobbers; future-read liveness alone cannot discharge a dead owner's release.
5. Land the first immutable code-owned retirement plan with its actual consumer
   and shadow comparison. Freeze SiteId/PhaseId/range encoding with frames/GC;
   publish a certified normal-return/error-unwind/heap-clear reason before
   unlink or first callback. RETURN at a PC alone is insufficient because a
   monitoring/helper failure can unwind from the same site. The same exact
   remaining-owner iterator must drive teardown and root enumeration. Code
   stays pinned through the last descriptor read. NULL control remains only
   during development; unsupported coverage must remain visible.
6. Generate finite PRE/STAGED/POST/ERROR/RETIRING/SUSPENDED descriptions at
   callback, exit and observation boundaries. Verify metadata against emitted
   stores and specialization/instrumentation changes, including partial output
   publication and observer redirects. Reject or split a family that cannot
   express its states within the agreed phase domain. Avoid per-inline-consume
   map decoding or mandatory publication work where no observer can run.

## Remaining M3: concrete arena and lifecycle consumers

Use the selected native policy in [collector-policy](collector-policy.md):
counted native scalar/PyMem owners remain under native discovery. Do not add
blanket C-value registration to compensate for missing arena contracts.

The revised [helper proposal](helper-roots-proposal.md) supplies the next bounded
consumer: generic unpack owns the sequence through all iterator/surplus/sequence
cleanup while a validated initialized suffix describes produced arena outputs.
The descriptor must survive outer cleanup, and final POST authority must be
published before disarm once maps become authoritative. Test each produced
prefix and reentrant sequence/iterator destruction with precise IDs.

Unpublished generic/default/keyword/EX child binding homes need the same outer
lifetime proof, beginning before the first binding callback and continuing
through provider/EX cleanup until publication or complete child retirement.
An inner helper's record alone leaves a caller-cleanup gap. Review each actual
owner transfer; borrowed vectors do not become counted owners.

Finish the callback/arena/heap-edge census: native call transactions and provider
refresh, exception/source-error state, observer terminals and local restores,
frame escape/takeover, constructor failures, generator creation/resume/throw/
close, suspended heap traversal and remote/debugger readers. Each boundary
needs a concrete authority row and focused mutation/failure evidence. Dynamic
prefix storage is initialized before publication and removed before close;
container edges move exactly once. No borrowed mortal scratch bank is allowed.

M4 requires that census closed, complete admitted-image proofs, shadow equality
for logical values and unique owning edges, and every lifecycle reader migrated.
Only then may static authority replace NULL interpretation and arena-value
scans together. Root still owns both-role validation, native cost inspection
and performance admission of the complete composition.
