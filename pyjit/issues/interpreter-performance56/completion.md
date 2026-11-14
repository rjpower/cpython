# Direct native completion entries

Status: source implementation `40b2858cf50` is integrated and regenerated in
native checkpoint `f6ef4a81a87`, with both builds at 117 modules/79 shared and
84 completion/observer methods passing per role. Optimized entry costs are
recorded below; there is no isolated throughput result.
Root owns full generation, builds, diagnostics and measurements. This is a
bounded follow-up to [exact-tuple preparation](call-ex-tuple.md), using the
[combined design](../../design/proposals/interpreter-performance56.md).

## Baseline evidence and applicability

The isolated P branch `pyjit/perf56-direct-completion` starts from frozen I
`196284139a1`. The prior tuple and retained-runtime branches are preserved.

The retained baseline assembly is
`E55/scalar-results-baseline-native/_PyEval_CompleteStep.txt`. It shows actual
incoming stack loads at `rbp+0x10` (value) and `rbp+0x18` (kind), several
kind-dependent tests/branches, a `0x48` local stack reservation, five saved
general-purpose registers and a frame pointer. Which branches execute depends
on the kind, monitoring and outcome. This evidence is from the retained
baseline ELF, not a native measurement of the new source.

The sole generated caller is `IREmitter.complete_step`, reached at an actual
`IR_COMPLETE_STEP` terminal. Its source already supplies a literal kind:

| Entry | Reached operations | Frequency unit |
| --- | --- | --- |
| FOR | Generic/instrumented and list/tuple/range/virtual FOR exhaustion | One terminal exhaustion, not every item |
| SEND | Generic native/virtual SEND completion and async-generator-asend completion | A native completed step, not every yield/resume |
| SEND_THROW | CLEANUP_THROW handling StopIteration | A completed delegated throw, not every throw |

Normal generator RETURN is different: `_PyEval_CompleteGeneratorReturn`
already calls the same implementation with a canonical input and literal
SEND/FOR kind after decoding the immutable caller word. Its native-entry
branch, decoder and canonical paths stay unchanged. Ordinary Python RETURN
already publishes inline and is also unchanged. Thus this packet is not a
generator whole-call redesign, and a generator benchmark's total resume count
cannot be used as its applicability count.

## Interface and invariant

Replace `_PyEval_CompleteStep` and its sole generated use with:

- `_PyEval_CompleteForIter(tstate, frame, site, base, exit, value)`;
- `_PyEval_CompleteSend(tstate, frame, site, base, destination, exit, value)`;
- `_PyEval_CompleteSendThrow(tstate, frame, site, base, destination, exit, value)`.

Each entry calls the existing `complete_step_impl` with its literal kind and
`canonical_input=false`. FOR passes `IR_NO_DST`; the common body never reads
the destination on any FOR route. On x86-64 SysV, the incoming argument count
changes from eight to six/seven, potentially eliminating two/one stack
arguments. No new return record is introduced: the existing 16-byte result
still returns through registers. Other platforms must price their actual ABI.

The common implementation body is unchanged. Keep native-result Publish,
PRE/POST observation, source/error origins, redirects, window detachment,
recipient displacement, publication before cleanup, exception preservation,
one-pass retirement and continuation reload. No early local steal, descriptor
lookup, frame field, tail-call parameter or handler-body copy is added.

The emitter selects a direct entry only for the three supported literal kinds;
unsupported marker kinds fail source generation. This does not add a semantic
grant: the existing reached source, normal-completion, abrupt and error
receipts remain mandatory. Changed target geometry still loses the existing
proof. Helper spelling establishes no new canonicality or ownership fact.

## Review and checks

GC and IR give design agreement with the unchanged-body/receipt boundary.
GC additionally reviewed the retained disassembly evidence and gives actual
source post-review GO. Static comparisons confirm that the common completion
body and GeneratorReturn remain byte-identical. The isolated source commit
contains no generated products; root regenerated the combined source. The
actual completion product change replaces the 11 generic helper calls with
their literal entries without changing adjacent origin/dispatch handling.

The existing source test
`test_generated_cases.TestIRCases.test_completion_result_local_does_not_capture_held_value`
now emits only the 16 selected handlers, in both record modes. It checks exact
direct signatures, held-result identity, FOR's absent recipient parameter,
absolute CLEANUP_THROW target, unchanged generator-entry/RETURN routes, and an
unsupported-kind mutation. The focused host method passes. Its first attempt
used the ordinary-instruction dictionary for an IR-only EMPTY return; the
fixture now uses the shared `all_instructions` lookup. No full generation,
native execution or build was run in this lane.

Root should retain current SEND/FOR normal/error/observer/source-recovery
checks, alias and discarded-recipient cases, finalizer/GC and frame-escape
coverage, and generator send/throw/close behavior. In particular, PRE redirect
must preserve untouched inputs, POST failure must retain committed state and
its actual error origin, and source recovery must continue selecting the same
transfer. Existing helper/action receipts must not erode in regenerated
metadata. The separate native-result normalization path must remain fallible.

## Root counts and falsifiers

Count these boundaries separately in a diagnostic process:

- Old baseline `_PyEval_CompleteStep`, split by `_PY_IR_FOR_ITER=0`,
  `_PY_IR_SEND=1`, `_PY_IR_SEND_THROW=2`.
- Candidate `_PyEval_CompleteForIter`, `_PyEval_CompleteSend`, and
  `_PyEval_CompleteSendThrow` by symbol, plus the unchanged
  `_PyEval_CompleteGeneratorReturn` as a separate denominator.
- PRE/POST monitor paths and terminal failures separately from successful
  unmonitored terminal completion. Ordinary returns and yielded items are not
  direct-entry hits.

At the exact entry address of the retained x86-64 baseline symbol, argument
seven is at `rsp+8` and argument eight at `rsp+16`; the prologue's `rbp` loads
confirm that layout. A debugger breakpoint placed after the prologue cannot
reuse entry `rsp` offsets. Root may instead use a retained diagnostic counter
patch; ordinary timed binaries carry no counters.

The predicted removal is incoming kind/destination traffic and dead kind
branches, not one less helper call or less ownership cleanup. Inspect all
three optimized entries and their callers, total text size, stack reservations,
spills and calls to the observer/retirement machinery. Three specialized bodies
can cost more code space than the generic body. If optimized native already
eliminates this work, the mechanism has no cost result. If terminal hit counts
are negligible, make no broad claim. Parent/hot6 confirmation and all29 gates
still decide selection; nominal argument count alone cannot admit it.

## Optimized native cost

Root retained the baseline generic entry under
`E55/scalar-results-baseline-native` and the candidate ELF, symbol sizes and
disassembly under `E55/perf56-recipient-native`. The latter manifest pins source
and native `f6ef4a81a87`. The measured x86-64 entries are:

| Entry | Symbol text bytes | Local stack reservation | Incoming stack arguments |
| --- | ---: | ---: | --- |
| Baseline generic CompleteStep | 1573 (`0x625`) | 72 (`0x48`) | value and kind |
| CompleteForIter | 889 (`0x379`) | 40 (`0x28`) | none |
| CompleteSend | 1301 (`0x515`) | 56 (`0x38`) | value |
| CompleteSendThrow | 891 (`0x37b`) | 56 (`0x38`) | value |

All retain five saved general-purpose registers plus the frame pointer. The
three direct entries total 3081 bytes, **1508 bytes more** than the former
generic entry. This prices these helper symbols, not the whole interpreter or
instruction-cache effect. The reduced incoming argument traffic and stack
reservations are observed native changes; cleanup and observation remain.

The separate f6ef/golden54 composition screen gives generators 0.949424 elapsed
time and coroutines 1.065409, with two balanced pairs and stable identities.
It includes scalar replay and generator-recipient changes as well as these
entries, so it cannot attribute either ratio to completion specialization.
There is no overall selection; see [the checkpoint report](report.md).

## Frequent generator handoff

The recipient correction `0888` is integrated in f6ef. Its generated change
removes only the five-line local/discard deoptimization from FOR_ITER_GEN;
SEND and the metadata programs are unchanged. The following diagnosis describes
the earlier 6a00 baseline.

The next independent slice is based on I `6a00c6a08d4`, compared with pinned
upstream `3a440b65695615e9fd9912342b79cdd54fced0d2`. Both already enter
specialized SEND/FOR generator frames directly, link the exception stack and
return yielded values without a new Python call. The fork also skips an inert
RESUME. The upstream stack interpreter pushes to a known stack position;
the fork publishes to an explicit recipient, supporting pending redirects,
discard and displaced named-local owners. Those semantics must survive.

There is a concrete admission gap: `names_return_slot()` does not recognize
the actual terminal `IR_ENTER_GENERATOR`. Consequently `FOR_ITER_GEN` selects
`STORE_LEAVE`, and its generated prologue deoptimizes whenever A names a local
or `IR_NO_DST`. Store elimination can give an ordinary `for item in iterator`
that local A. The intrinsic already writes the correct parent return slot
before linking the child, and a child yield uses `SetReturnValue`; the missing
compiler capability prevents that implemented path from being used.

Root's retained generator profile has the matching generic outer path:
`FOR_ITER` → `VirtualIteratorNext` → `gen_iternext_value` → `EvalFrameValue`.
`SEND_GEN` accounts for 27.91% physical-symbol self samples and
`YIELD_DELEGATED` 13.94%; these identify frequent boundaries, not removable
percentages. Evidence is under `E56/call-ex-perf-record`. Root's complete
generator comparison reports about 49.3% more native instructions and 47.2%
more cycles than upstream. The admission gap is a mechanism hypothesis for
part of that difference, not an attribution of the whole gap.

The isolated proposal/implementation uses a small shared `GeneratorEntryRecipe`
from the actual top-level terminal intrinsic, literal kind, scalar IO and
unchanged two-input prefix. Its legacy frame output has only the unreachable
stack-model consumer. Preceding guard parts use the shared typed effect walk:
no arena write, callback, origin adoption or unknown helper. No opcode-name
permission or second full-body table is introduced. The same recipe is
consumed by store selection and actual intrinsic lowering. FOR yields to A;
SEND yields to B+2 independently of completion A. Existing normal/error/WAIT
receipts remain separate and unchanged.

For FOR, select the existing `STORE_RETURN` protocol and remove the local/
discard fallback. Do not steal an old local early or cache it across child
execution. The eventual yield publishes first, then retires the actual current
displaced owner. A finalizer or observer can mutate that binding. Discard closes
the yielded owner through the same established protocol. Keep child, parent
continuation and exception-state commit before any failing entry or owner
cleanup. A type/state/PEP-523 miss still falls back before that handoff.

The source expectation is removal of the unconditional local/discard deopt and,
for affected sites, the nested native iterator/evaluator call chain per item.
The source is unchanged for scratch recipients. Measure actual entered
`FOR_ITER_GEN` sites and failed local-recipient guards, rather than just seeing
a specialized handler in feedback: the old handler can remain installed while
deoptimizing every time. Falsify the hypothesis if sites use scratch recipients,
the direct entry count does not rise, or optimized native/workload costs do not
improve. Extra source checking adds no hot runtime descriptor or frame field.

Focused source fixtures emit actual FOR/SEND handlers in both record modes and
reject changed literal/IO, conditional/duplicate/suffixed terminals and poisoned
prefix guards. Runtime fixtures cover L replacement with a finalizer that
observes the published value, mutates locals and collects; trace callbacks;
scratch tuple-unpack and constructed discard; and a different iterator type at
the same warmed site. Native qualification remains root-owned.

## Publication caller audit

At I `6a00c6a08d4`, `SetReturnValue` already receives a canonical owning value;
it does not call Publish. `PublishReturnSlot` is used only by that helper.
Its home check is diagnostic, and its live work is recipient selection,
publication and displaced-owner retirement. Removing normalization from these
two helpers would therefore remove no existing normalization.

| Actual producer | Value boundary and publication |
| --- | --- |
| `IR_ENTER_GENERATOR` SEND | Moves the already canonical sent home; FOR supplies canonical None; both use `SetReturnValue` |
| Delegated suspension | TAKE of the canonical outgoing home, then `SetReturnValue` after suspension commit |
| Ordinary YIELD | Checked canonical frame transport selects `IR_STORE_CANONICAL_RETURN`; no Publish |
| Ordinary/EMPTY RETURN | Checked canonical frame transport, or explicit Publish for an uncovered native C result, then the same final publication |
| `CompleteGeneratorReturn` | Canonical input; native interpreter entry uses `SetReturnValue`, ordinary SEND/FOR uses literal canonical completion |
| `gen_send_value_ex` | Canonical None or an owning duplicate of its borrowed API argument, followed by `SetReturnValue`; the duplicate cannot be dropped without argument-lifetime proof |
| Remaining `IR_STORE_RETURN` expansions | RETURN_GENERATOR and instrumented ordinary YIELD retain explicit publication for source transport not certified canonical |
| Native CALL result transaction | Has its own checked result construction and generated recipient publication; it does not call `SetReturnValue` |

Rare redirect/displacement paths may still force register liveness, branches or
spills in every SEND/yield handler. That needs optimized native comparison;
flat inline attribution alone cannot establish which instructions disappear.
Any later outlining or once-admitted recipient geometry is a separate measured
increment, preserving all public constructors, native entry, callbacks and
storage authority. Do not add a universal per-yield descriptor switch.

Root also observes recurring `FillConstSlots` attribution in coroutine exact
calls. This is follow-up inventory only: count existing constant-slot folding
and entry stores before proposing pool/rematerialization changes, and charge
any added per-read decoding. No constant or frame-encoding change is included.

## Exact-coroutine awaitable transfer: unselected source experiment

The next source experiment starts from P `d531d993d30`, after the recipient
and discard-fixture packets. At an accepted exact coroutine, GET_AWAITABLE
currently obtains a new reference to the same object, closes its input owner,
then reconstructs a PyValue. Upstream has the same reference round trip.
A callback-free type/state predicate followed by TAKE can instead transfer a
scratch owner. A folded binding still needs TAKE's duplicate, so it saves no
reference pair. Root's retained GET_AWAITABLE self attribution of 7.40% selects
the investigation; it is not an estimate of removable time.

The predicate must reject exactly SUSPENDED_YIELD_FROM and its LOCKED variant,
matching the existing helper. EXECUTING and closed coroutines remain accepted
here and fail later through their existing SEND behavior. The relaxed FT state
load is an observation, not an execution claim. Custom awaitables, rejected
states, inline inputs and errors retain the original helper/close/error/result
sequence. The accepted arm has no callback or borrowed escape before TAKE.

The following records the worker source freeze, before root integration and
native qualification described at the end of this section.

The first source attempt emitted both record variants, but exposed a format
regression: `iter = TAKE(iterable)` marks the output as a value alias, removing
STORE_DST and adding NO_FOLD. That loses existing destination folding and may
add a surviving STORE_FAST. The current bounded prototype restores those
format fields while preserving the alias metadata. It is committed at
`a852d11faf6`, with GC and IR actual eight-file source review GO. It is
**not selected or native-qualified**.
Do not disguise the alias with an opaque helper or expression spelling.
Keep value identity separate from an owning-result certificate. Both TAKE
and the fallback's guarded FromObjectSteal must prove an owning result on
every normal arm before store selection; actual Storage capture/publication
must remain a backstop. Reuse the shared typed source/fact walk rather than a
GET_AWAITABLE exception or another C parser. If that distinction needs a broad
framework, close this bounded experiment with the folding loss recorded.

The hot-code gate includes unchanged STORE_DST, NO_FOLD, input borrowing,
allocated slots and surviving IR words. The generic fallback still requires
boxing preparation and the value-temporary reservation; this packet cannot
claim their removal. Native inspection must establish whether the exact arm
actually removes helper and reference traffic after those costs. Required
controls include same-input/destination and discarded results, displacement
with finalizer mutation/GC, already-awaited/closed/executing coroutines, custom
__await__ mutation/errors, and a poisoned branch that revokes owning-result
certification. No native execution or full generation has run in this lane.

The bounded join prototype has two independent checks. First, the existing
typed source walk exports the final owning output names, intersecting actual
reachable normal arms. TAKE needs a current declared owning scalar input;
FromObjectSteal needs the existing native-pointer owner and non-NULL proof.
This projection does not certify normal, abrupt or error home transport.
Second, a Python-only `Local.owns_value` fact records actual acquisition for
the sole final output. The first slice permits only direct assignments to
that output: reading, aliasing or addressing it before publication rejects the
certificate. Writes and kill revoke the fact, branch clones preserve it, and
existing structured joins intersect it. First publication checks and consumes
the fact and rejects an inplace borrow. Thus distinct branch token identities
can each supply the same owning C result variable without pretending to be
one logical value. Acquired extra VALUE owners, overwrite without retirement,
and pending VALUE owners on an explicit error edge reject the source proof.

The existing scalar-home graph remains uncovered when its token identities
cannot meet. This join grants no readable home shape, new replay operation,
runtime field or descriptor. Unproved branches retain the old alias/folding
restriction. The callback-capable GetAwaitable helper has one descriptor in
the shared table: owned native OBJECT result, possible failure/callback and
UNKNOWN home effects. Its ownership does not upgrade ordinary home authority.

Two narrow host methods pass, covering the actual GET_AWAITABLE handler in
both record modes, source poison cases and removal of an actual acquisition
receipt. An all-format comparison against the prior source/alias policy has
no changed fields; GET_AWAITABLE keeps STORE_DST, input borrow mask 1 and no
NO_FOLD. Runtime fixtures are AST-checked only. Root still owns complete
generation, allocated-slot/word comparison, runtime qualification and native
cost inspection. No performance claim follows from these source checks.


Root integration82e857384f8 has unchanged-format comparison, full regeneration,
two product approvals, both 117/79 builds,154 focused methods per role and5+27
bootstrap/compile probes passing. One new fixture initially expected the wrong
executing-coroutine exception: a frozen upstream/golden/candidate probe gives
ValueError in all three; its corrected assertion preserves state and cleanup.

Retained native review confirms the exact scratch-input arm bypasses
_PyEval_GetAwaitable and its nested INCREF plus the old scratch DECREF. Folded
local inputs retain their necessary duplicate. GET_AWAITABLE text grows1170 to
1408 bytes (+238), and stack reservation112 to128 bytes (+16); generic bridge
setup, stack canary and recipient checks remain. This is actual pair/helper
removal, not prologue removal. First hot6/golden composition gives coroutines
1.010237 (range.993797–1.026950), so no isolated coroutine throughput gain is
claimed. Evidence: E55/perf56-transport-await-native and
E56/transport-await-hot6-golden. Broad comparison and six-pair confirmation are complete at82e857; coherent
qualification and selection remain pending. The report owns the later results.
