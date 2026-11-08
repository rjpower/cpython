# Competing structural mechanisms for logging parity

2026-11-08. Design proposals, not implementation authorization. Runtime is
qualified R5 `0953757f4e7`; all references below are to that runtime unless
stated otherwise. No runtime source, generated product, build or native
workload was changed for this proposal. The coordinator selects a reviewed
batch after comparing this document with the call and compiler proposals.

## Decision supported by the source

Compare three mechanisms: checked leaf/escaping reader continuations,
observer-aware return/discard execution, and dictionary hit feedback. They
cover different work from specialized-call preparation. They share existing
canonical eight-byte homes and current 64-bit IR words; none requires an
accumulator, a second value representation or permanent experiment switches.

The strongest immediate discriminator is an exact-dictionary first-probe
identity-hit diagnostic, followed by a return-boundary diagnostic. The former
can distinguish dictionary lookup overhead from generic reader completion;
the latter can establish the cost and safe coverage of nine retained hot outer
POPs. Neither observation predicts the roughly 39.7% candidate-time reduction
needed from the new 1.657289 baseline. Sample shares cannot be added as gains.

Source already rejects part of an overbroad phase hypothesis. Generated
`LOAD_ATTR_METHOD_WITH_VALUES` has no reader refresh, and `LOAD_ATTR_SHAPE`
already publishes a canonical temporary result and refreshes only when its
receiver can deallocate. These are implemented mechanisms to reuse, not
missing optimizations. Their per-word PC stores and class/address calculations
remain possible machine-code questions, not proven redundant work.

## Evidence and common accounting

`E8` means the coordinator-owned evidence directory
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-parity-20261108/`.
Its `warmed-ir.json` establishes these sites, not dynamic branch counts:

| Body | Relevant ordinary path |
| --- | --- |
| Benchmark | Ten method loaders, message snapshots and general Python calls; nine retained hot POP continuations; the tenth call already discards |
| `Logger.debug` | Method load, module global load, exact Python call, false branch, None load, return |
| `Logger.isEnabledFor` | Shape load of `disabled`, boolean branch, shape load of `_cache`, dictionary subscription, return |

Correction to the earlier review's static count: `bench_silent` has ten POP
words in the capture, but one follows `truncate_stream` before the loop.
Inside the loop, the first nine calls have POP continuations; the tenth has
destination 4095 (`IR_NO_DST`) and goes straight to JUMP_BACKWARD. Thus the
coordinator's new R5 Py_STATS diagnostic showing nine POP dispatches per loop
agrees with this source. M2 has at most nine removable POP dispatches here;
the tenth call already exercises existing discard support. The diagnostic
also reports 161 total dispatches, 20 frames, ten general and ten exact calls,
20 method and 20 shape reads, ten dictionary subscriptions, ten message loads,
20 boolean branches and 20 returns per loop, with exact 2x scaling from 1,000
to 2,000 loops after empty subtraction. These are counts, not timing evidence.

`E8/frame-layout.json` gives debug 4 locals + 5 temporaries and isEnabledFor
3 + 8, zero constant homes in both, and a 13-word header. Constant-pool
relocation cannot remove per-call constant-home work on this target. Cold-path
temporary capacity is not proof of repeated initialization: count the new
tail after known-NULL overlap before proposing a validity map.

Preserve the following costs unless a particular proof removes them:

- Evaluated argument owners, fresh observable kwargs, parameter/default
  mapping, recursion, frame linkage and final local destruction.
- Mortal result acquisition and receiver/input decrements. A decrement that
  cannot deallocate is leaf work; it still changes an owner count.
- Publication before a possible callback, callback-current local semantics,
  precise error PC, monitoring activation, and refresh after an actual escape.
- Dictionary hash/equality calls that ordinary lookup would execute. A cache
  must preserve their effects, even if the dictionary itself does not mutate.

Keep diagnostic counters outside release timings. Attribute sites, semantic
lookups, input-retirement outcomes, frames, observer boundaries and actual
dispatches need separate denominators. Report overlaps between mechanisms.

## M1. Checked leaf and escaping reader continuations

**Question:** can a provider's proven nonescaping, canonical hit bypass generic
reader completion while retaining the same owner retirement and fault PC?
This applies to the dictionary hot path and later other checked readers; it
does not assert that all specialized handlers are leaf.

Actual before path, `Python/generated_ir_cases.c.h:2899`:

1. Publish the dictionary word as `frame->instr_ptr`; decode homes.
2. Guard the mapping provider against `_PyDict_Subscript`.
3. Consume temporary inputs or duplicate local inputs into held owners.
4. `value_subscript` rediscovers exact-dictionary type and calls
   `PyDict_GetValue`; this may hash, compare, pin inputs and execute Python.
5. `value_reader_complete` checks current frame/output, publishes the result,
   closes inputs/boxes in order, refreshes, and returns the result.
6. Generated code reloads word/slots, then `IR_STORE_DST` publishes again;
   displaced-local retirement follows before dispatch.

`Python/ceval.c:value_subscript`, `value_reader_complete`,
`value_operation_refresh`; `Objects/dictobject.c:PyDict_GetValue`,
`_PyDict_GetValue_KnownHash`, `compare_generic` establish this behavior.

Proposed after path:

1. Keep the current instruction PC published. Retain conservative family
   effects and ordinary instrumentation. First prove a GIL-held exact dict,
   inline-int query, general combined table, and first probe whose stored key
   is the same canonical value and whose current value is non-NULL.
2. A dictionary-owned internal primitive duplicates the canonical current
   value and reports `HIT_LEAF`; it does not consume inputs, allocate, call
   user code, manufacture an exception, or retain table pointers. Every guard
   failure reports `DECLINE`, with no effects or new owner. The unchanged
   generic path handles decline, absence, subclasses and errors.
3. On hit, consume temporary inputs and keep locals borrowed or owned according
   to the existing checked input contract. Publish the result to a temporary
   destination before any retiring owner can deallocate. A local destination
   remains untouched until input retirement completes.
4. Retire key then container in the same order as today. Test `MayDealloc`
   immediately before each ordered close, after any earlier close. When that
   check is false, close this owner inline. On the first possible deallocation,
   move this owner and all remaining owners to the common escaping retirement
   helper, then refresh and reload exactly as today. If every check was false
   at its own close and no boxes exist, omit refresh and decoded-state reload.
   Never precompute all checks: two aliases at refcount two can each initially
   appear leaf, while the second ordered close actually deallocates.
5. Store a folded local only after that phase; retire its displaced owner
   through the existing rule. Omit the second temporary store because the
   checked first-output publication already installed the canonical owner.

This is deliberately not a cross-handler accumulator or wholesale delayed-PC
scheme. PC stores remain until asynchronous inspection, tracing, errors and
deoptimization have a separately reviewed contract. M1 first targets the
helper round trip, generic publication/checks and reloads on a proved phase.

**Shared interface.** Extend the existing result-publication contract in
`Tools/cases_generator/ir_generator.py`, rather than adding unchecked C hints.
One checked provider outcome supplies `canonical_owned_result` and
`no_escape_since_decode`. A common multi-input completion rule takes result,
ordered input owners, destination and boxes; it reuses the shape reader's
canonical temporary publication and conditional retirement. The generator
rejects use after an escaping helper, allocation, poll, frame switch, unknown
release or instrumentation callback. Joining a leaf and escaping edge drops
decoded-state facts unless both reestablish them. Generic whole-instruction
effect metadata remains conservative for copy propagation and recovery.

The first-probe primitive belongs in `Objects/dictobject.c`, with its internal
declaration in `Include/internal/pycore_dict.h`; dictionary layout/probe code
must not be copied into the interpreter. Use the existing inline-int hash
implementation, not a logging-level constant or hand-written hash rule.
Initially support the ordinary GIL build and decline on free-threaded builds.
No feedback layout change is required for this first form.

**Owner and observer proof.** At entry, every input home is canonical or NULL.
The frame owns the local query and any local container; temporary homes own
their inputs until the leaf provider returns. The provider cannot invalidate
those borrows. It creates exactly one result owner from the dictionary home,
including a real increment for mortal results. Consumption moves temporary
owners once and leaves NULL. A temporary result is visible before input
destruction; the local destination follows the old retirement order. The
multi-owner rule checks each owner against its current count immediately
before closing it; a possible callback invalidates leaf facts and routes all
remaining retirement through the existing refresh. Earlier closes may change
later owners' counts even without callbacks, so aliasing needs no exception
to this sequential rule.
An inline query is insufficient on its own: an earlier colliding custom key
could run equality. The first-probe identity guard excludes that omitted
comparison. Non-identity, changed layout, missing value and subclass behavior
all decline before input consumption. Generic errors retain their original
PC, restoration and KeyError behavior. No user local dies at its last read.

**Migration and discriminator.** Medium scope: dictionary primitive, checked
generator contract, one DSL handler, regenerated metadata/product, and focused
tests. Start by counting first-probe identity hits and retirement outcomes.
Inspect fixed-GCC code for the current completion/reload path. If coverage is
material, compare two temporary source candidates with identical hit guards:
(a) hit provider followed by current completion; (b) the same provider followed
by checked phase completion. Their difference isolates the phase mechanism
from lookup improvement. These are sequential source experiments, never
runtime switches in a shipped design.

Reject M1 if the apparent refreshes follow real escapes, first-probe coverage
is poor, guard/helper spills outweigh removed work, or a shared contract
requires handler-specific exceptions. Callback-mutated locals/instrumentation,
temporary versus local destinations, mortal values, last receiver owners,
collision callbacks, misses and exception unwind decide correctness. Include
aliased input owners whose final ordered close deallocates. Existing
shape/method leaf behavior cannot be reported as new savings.

### M1b. Apply the same phase contract to leaf temporary producers

M1 is broader than a dictionary helper. The coordinator also identifies ten
`LOAD_FAST_BORROW` message producers and ten `LOAD_COMMON_CONSTANT` None
producers per loop. Their actual temporary-destination routes copy canonical
values without local displacement. Their general handlers must still support
local destinations whose old owner can deallocate. Whole-handler effects
cannot distinguish these phases, while a checked operand/destination-class
fact can.

A competing bounded generator prototype emits a temporary-only producer form
selected by compiler-known home class, with checked canonical output and no
displaced-local path. Keep the same 64-bit words and owning homes, and preserve
the borrowed-local snapshot increment. Compare its GCC code with the generic
form before adding many class variants. This addresses class tests, publication
and state pressure across short handlers without changing dictionary lookup.
The compiler/interface proposal owns the complete operand-form migration.

Both current forms also store `frame->instr_ptr`. Deferring that store is a
separate phase contract: every escaping release, poll, deoptimization, error,
observer entry and frame switch must publish its own precise PC first. Native
inspection and asynchronous observation need an explicit answer too. Do not
infer permission to remove PC stores merely from a temporary destination. A
source/assembly inventory can determine whether the store is meaningful work;
the first M1 prototype keeps it. If that store dominates, the evidence supports
reviewing the broader PC protocol rather than claiming the dictionary helper
alone is the architectural answer.

## M2. Execute the return/discard boundary without ordinary dispatch

**Question:** can common return execute a retained ordinary POP boundary
directly while preserving its PC and every observer activation case?

Before, `bytecodes.c:_RETURN_VALUE`, `ceval_macros.h:IR_STORE_RETURN`, and
`generated_ir_cases.c.h:RETURN_VALUE/POP_TOP`:

1. Take the child's return owner; emit return hooks; unlink and clear the
   child. Clearing locals/kwargs can execute Python and change observers.
2. Resume at the caller word after CALL, publish the result into return_slot,
   dispatch POP, publish its PC, consume and NULL its temporary home, close
   the result, then dispatch the following word.

`regcfg_passes.c:discard_region` intentionally retains the last POP on a call
line. Tracing enabled inside the callee must observe that line before the
next caller line. Blindly using IR_NO_DST or deleting the POP changes this.

Proposed after path keeps CALL, POP and their source/offset metadata intact:

1. Complete ordinary child return, including all child clearing and hooks.
   Resolve the caller's current continuation P and live return_slot afterward;
   do not carry a pre-callback observer or instruction proof across clearing.
2. The common return continuation checks that P is an ordinary unwrapped POP,
   its input is the same temporary return_slot, instrumentation is current,
   and no supported instruction/line observer, low-level trace or pending
   observation path needs that boundary. Initially decline on any uncertainty,
   generator/interpreter-entry return or special continuation.
3. For a result that cannot deallocate, execute the POP semantics directly:
   publish `frame->instr_ptr = P`, keep the destination NULL, close the held
   result owner once, and continue at P+1. Preserve required result
   canonicalization unless a checked return-result proof already supplies it.
   There is no callback or poll between these guards and boundary completion.
4. Otherwise publish into the original result home and dispatch P unchanged.
   The existing INSTRUMENTED_LINE/INSTRUCTION handler can call observers,
   process an exception or honor a callback's changed instruction pointer.

The first prototype excludes possibly deallocating results even when no
observer is currently active. A result finalizer may enable tracing, inspect
the caller or jump its line; retaining the original path makes that proof
independent. Logging's None result may satisfy the safe tag test, but its
actual count must be recorded. The decrement still occurs for a mortal result
that is known not to deallocate. Skipping its physical home installation is
safe only in this nonobservable span: that home starts and ends NULL, while
the C value owns the result until the ordinary POP retirement point.

**Shared interface and migration.** Add a generated common
`resume_call_result` continuation with explicit call PC, boundary PC and next
PC concepts, even if adjacent words derive the latter two. Initially invoke
it only from ordinary Python return. The compiler retains the POP; no new
word bits or call-window encoding is needed. The shared continuation may
later serve synchronous/native call completion, but only after preserving
its distinct input retirement, monitoring and periodic-check ordering. Do not
move those checks into the POP or implement a logging-specific handler.

Observer checks must use the runtime's actual instrumentation epoch and
handler-wrapper protocol; `tstate->tracing == 0` alone is not sufficient.
Reusing the style of `IR_SKIP_RESUME` needs review against per-code/local
events, instruction tracing and instrumentation-failure recovery. A callback
at the boundary may set f_lineno: fallback must retain the existing changed-PC
path, not call an observer and then force P+1. Exception return/unwind never
executes this normal-success shortcut.

Medium scope: return generator/macros and focused observer/owner tests, with
instrumentation changes only if the existing epoch guard cannot establish a
complete proof. This removes an ordinary dispatch and result-home store/clear;
it does not remove either Python call, frame lifetime, kwargs allocation or
the observer boundary. It is independent of call-preparation proofs.

**Discriminator and rejection.** Count normal returns whose live P is POP,
matching temporary destinations, result MayDealloc outcomes, installed or
changed observers after child clearing, and actual skipped dispatches. First
compare a return-continuation implementation that still stores/clears the
result home with one that omits that traffic; this separates dispatch from
home work. Keep virtual boundary execution counts distinct from dispatches.
Fixed-GCC code must show that checking P costs less than the removed path.

Tests must enable tracing/monitoring inside a callee and inside a child-local
destructor, preserve the caller call-line event and offsets, exercise f_lineno
jumps and observer errors, and return mortal values with weakref/finalizer
callbacks. Include native fallbacks and non-POP return destinations. Reject
if observers require reconstructing a deleted boundary, the fast path hides
another equivalent dispatch, its guard cost cancels the gain, or return-heavy
controls regress. No code or line metadata is deleted in this first design.

### M2b. Explicit no-op line boundaries on branch edges

The ten hot NOPs per loop are a second instance of the same observation
problem. `isEnabledFor` word 5 is the `try` line's NOP; the false `disabled`
branch lands there before the cache-read line. It has a source boundary even
though its normal execution performs no data operation. Removing the word
would remove the observer's location.

An alternative prototype retains that word and has the specific producer/
branch continuation execute its ordinary boundary inline. After evaluating
and retiring the exact boolean condition, resolve the live target P; check
current instrumentation and the ordinary NOP handler, publish P as the boundary
PC, then dispatch P+1. An instrumented target goes through its existing handler
and may change PC or raise. Preserve branch-monitoring behavior as well as the
line/instruction boundary. Any condition retirement that might call Python
must finish and invalidate/reestablish the proof before this check.

Select this edge through a generated branch form or compiler metadata for
explicit boundary targets. Do not add a speculative next-NOP test to every
DISPATCH. The ordinary word, offsets and wrapper protocol remain authoritative
when tracing starts after the branch form was selected. The contract is an
executed semantic boundary with an optional ordinary dispatch, not a deleted
source line. Distinguish ten candidate NOP dispatches from nine candidate POP
dispatches; do not combine their expected gains before separate prototypes.
Reject this variant if added branch/metadata checks replace the saved dispatch,
observer activation cannot recover the exact boundary, or the generalization
needs source-function exceptions. This may be cheaper to test than result-home
fusion because NOP has no result owner, but it does not measure POP home work.

## M3. Dictionary feedback with a no-callback probe-prefix proof

**Question:** beyond M1's direct first-probe hit, does stable keyed feedback
remove enough hashing/probing to justify new cache state?

The current dictionary specialization establishes only a builtin mapping
provider. `value_subscript` and `PyDict_GetValue` still perform full lookup;
the shared `_PyFbBinary` has type versions and an extension descriptor, not
a dict entry cache. A new feedback form is therefore an implementation and
metadata change, not an absent feature already encoded in the IR word.

Competing concrete designs:

| Design | Hit work | Constraint |
| --- | --- | --- |
| M1 direct first probe | Inline-int hash, initial bucket lookup, current key/value guards | No feedback allocation or retained pointer; decline if the first candidate is not identical |
| M3 certified index | Query-value guard, general-table/nonzero keys-version guard, entry bounds/key/value guards | Cache fill must certify that skipped probes would execute no Python |
| Unqualified cached index | Version and final key check | Reject: can omit observable equality calls before the matching entry |

For the certified form, fill a small entry containing inline query value,
nonzero dictionary keys version and entry index only after checking the
ordinary probe prefix. Each skipped occupied entry must have a different
stored hash, or a proved native comparison with the same outcome. Initially
accept only differing hashes and unequal inline-int keys. A same-hash custom
key rejects caching; do not invoke its equality while obtaining a certificate.
The fill can simply decline and let ordinary lookup run once. Its read-only
scan must share dictionary probe code rather than invent another traversal.

An unchanged nonzero keys version then certifies the same probe prefix. Read
the entry's current value on every hit and duplicate its canonical owner;
never cache the result or an unowned keys/entry pointer. A value replacement
must return the new value. Key insertion, deletion, resize and clear invalidate
the proof through the keys-version rules. Version exhaustion (zero) declines.
Audit every structural mutation against `dictobject.c` before relying on this;
`_PyDictKeys_GetVersionForCurrentState` is a provider, not a blanket proof that
every consumer's desired invariant follows. Initially decline for split/
Unicode tables, subclasses, arbitrary object keys and free-threaded builds.

Stable key identity plus version alone is insufficient. A custom key before
the desired entry can have equality whose effects or answer change through
external state while the dictionary remains unchanged. Skipping it changes
Python behavior. The certificate removes this risk by excluding that callback
from the cached path at fill time and invalidating the structural proof on
every relevant table change.

Use the same checked canonical leaf-hit completion as M1; duplicate input
pinning and reader refresh may disappear only because that path cannot
escape. M3's lookup savings and M1's completion savings overlap and require
the two-candidate decomposition above. Dictionary input retirement still
falls back to escaping completion if it can deallocate. Misses and unsupported
cases preserve ordinary hash/equality order, __missing__, KeyError and unwind.

Medium to high scope: one binary-family feedback variant or an explicitly
owned side entry, producers/readers, cache clearing, generated metadata and
specialization/instrumentation tests. Do not reinterpret a live descriptor
pointer or enlarge every binary site's cache without accounting for footprint.
The current D feedback field can address a suitably generated entry; changing
the 64-bit instruction encoding is unnecessary. Prefer M1's smaller primitive
unless observed collisions/probe length show material additional coverage.

The cheapest discriminator counts exact-dict inline-int lookups by first-probe
identity, safe longer prefix, callback prefix, miss, table kind and version
churn. A diagnostic shadow certificate must neither call equality nor pin or
retain objects; coordinator-owned source instrumentation is sufficient.
Reject M3 as the next packet if first-probe hits already cover nearly all
useful cases, cache/version loads cost more than the short probe, safe prefixes
are uncommon, or metadata footprint/control regressions outweigh the benefit.
Adversarial collision equality, changing equality without dict mutation,
delete/reinsert, resize, value replacement and version exhaustion are decisive
correctness cases.

## Comparison with the other alternatives and V8

V8 source is pinned to `63481c19453c72221531058be6d2e94adf3ebc66`, as in the
[independent review](v8-review.md). Its
[interpreter assembler](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-assembler.cc#L751)
marks state invalid at calls and saves the bytecode offset according to call
behavior. This supports an explicit generator phase protocol, not removal of
Python retirement callbacks. Its
[keyed-property handler](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-generator.cc#L606)
passes site feedback to KeyedLoadIC. That motivates assessing feedback below
the interpreter operation; it does not make JavaScript property-cache guards
valid for Python dictionary collision equality. The
[register optimizer](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-register-optimizer.cc)
tracks equivalence and materialization. Python snapshots and destruction
ordering remain extra constraints when applying that idea.

| Alternative | Covered work on this target | Deciding evidence before selection |
| --- | --- | --- |
| Specialized call facts | General outer calls; inner exact route differs | Accepted/declined routes and pre/post-escape checks surviving GCC |
| M1 phase completion | Dictionary hit completion; reuse existing shape machinery | Leaf hit and retirement counts; split lookup/completion prototypes |
| M2 return/discard | Nine retained outer POP continuations; tenth already discards | Current observer state after clearing; None/nondeallocating result counts; branch versus dispatch code |
| M2b explicit NOP boundary | Ten `try`-line branch targets | Target wrapper/epoch state; branch-monitor semantics; selected-edge check versus dispatch |
| M3 dict feedback | Repeated stable keyed hits beyond first probe | Safe-prefix coverage and probe work after M1; cache footprint |
| Compiler/producer fusion | Message snapshot dispatch per outer call | Snapshot remains an owner; quantify only removed dispatch/window traffic |
| Operand-class variants | Local/temporary decisions in short handlers | Actual class tests in fixed-GCC code and observed combinations |
| Dispatch ABI or density | Potentially most words | Spill/front-end evidence; short hot bodies alone do not establish a density problem |
| Constant-home placement | No per-call constant homes in these callees | Rejected as this workload's parity mechanism |

Producer fusion is a credible additional option, but current evidence shows
only one message-local-to-window snapshot per outer call, while the method
loader already produces callable/self in place. Folding that load into call
entry must acquire the snapshot before the first preparation callback and
leave its owner in the ordinary window. It saves a word/dispatch, not the
argument owner, and needs an operand/side-metadata plan if the present four
fields cannot describe both window and source. M2 can test dispatch value
without changing call argument encoding first. The independent compiler
proposal owns the broader design.

The recommended selection step is comparative diagnostics and peer review of
one bounded prototype per materially covered mechanism. Do not bundle all
three and attribute the result to whichever explanation looks attractive.
The tiny successful-refresh and known-code cleanups may accompany a justified
batch, but they do not answer these structural questions. Keep one selected
runtime after each experiment, with all failed experiments and limits recorded.
