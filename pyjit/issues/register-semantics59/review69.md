# Grouped architecture review69

**Recommend one complete guarded-callee-summary unit for independent source
adoption; stop the rejected retained68 executable.** The five alternatives,
exact proposed interfaces, owner/error/observer contract and finite selection
policy are in [design69](../../design/proposals/interpreter-parity69.md).
This review does not implement or qualify a runtime. Selected57 is unchanged.

Read canonical README, GOAL, STATUS, HANDOFF, PROCESS, active performance56 plan,
design68/review68 and the closed report at canonical
`0fa1cefea53180f1e41d4acade00d12b5a471fa3`. Followed applicable AGENTS and freshly
read the [official CPython AI policy](https://devguide.python.org/getting-started/ai-tools/).
Only the two new review69 documents change. No agents, runtime edits, generation,
builds, target execution, tests, timing, counters, control mutation or push.

## Immutable source and evidence authority

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`R=BASE/worktrees`; `E59=BASE/pyjit-link-wave1/register-semantics59`.

| Authority | Pinned source / binary |
| --- | --- |
| I, selected57 | `R/call55-integrated`, `0dcba75bb3df40c32137db453b001f7e8f101de0`; selected captured ELF `BASE/pyjit-link-wave1/call-model55/comp57-dictionary-native/python`, SHA256 `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2` |
| U, upstream | `R/upstream-3a440b6`, `3a440b65695615e9fd9912342b79cdd54fced0d2`; release `BASE/upstream-release-devlibs-3a440b6` |
| G, frozen rejected68 | `R/call55-gc`, `c02cebf80c48d47a9167dd86dce9bdc0fd11a623`; release `BASE/reg59-results-release`, SHA256 `4b5bd3abfc74eab6a40cd6a16e71ccd31b8d9992d8e3e6b80707d15b8156eb92`; debug `1b6d89a30e6c5aa1683b5b50391a06b79cfe7a6954e64421bb279fd2157dc3d3` |

Canonical/docs-worktree runtime source is stale and is not a source anchor.
The following anchors name actual I/U/G files; numeric positions refer to those
pinned trees. Source inference, static native facts, dynamic mechanism coverage
and performance are separate evidence classes.

## Retained68: success of mechanism, rejection of executable

Actual dynamic receipts in `E59/retained-delegation68-native-controls` include
`active13b-mechanism-summary.json`, `tests14-tree-debug.json`, entry-budget
controls and final15 admission. Native depths2/8/32 each show39 direct entries
and yields with zero intermediate delegate handlers; VM SEND39/FOR38 use actual
count8/debt8. Six forced allocation sizes72/64/128/256/512/1024 preserve40 values,
entry counts, result identity and no replay. The unchanged Tree has one record
through251 events:62 appends,62 pops,63 yields,63 native resumes,1 retirement.
Count1 own-yield/right-append and the older prefix remain intact. These are
actual general mechanism witnesses, not benchmark-name admission.

Both117/79 builds,50 focused methods per role (one release debug-only skip),13
related modules and84 remote/delegated methods per role qualify; exactly the six
inherited jump errors remain. No fresh full-system pass. RUNNING remote sampling
has actual stopped-process controls; SUSPENDED coverage is source,27 synthetic
reader cases and actual Tree images, not a separate suspended-process sample.

`static-readback.md` and `static-final15.md` under the same evidence directory
retain the current decoded functions. All51 release/58 debug captured functions
match the reviewed bf86 image, including addresses/sizes. Final c02 changes four
test files, not runtime/products. This is not whole-ELF or whole-.text identity.

| Static native / representation fact | What it does and does not establish |
| --- | --- |
| Stable72B record,16B link, optional32B loop row,24B plan; common generator-family membership +8B, fixed frame112B | Real storage/initialization costs, including FT objects that never form records. No allocator-class or timing attribution. |
| `gen_send_value`300→878B, own stack24→72; `_ex`523→680B,72→88 | New ordinary ingress shares the retained claim's native frame. The saved listing branches on membership after the prologue. A separate consumer could change this boundary; no proof this causes logging's regression. |
| EvalFrameLoop934→1056B, release own stack216 unchanged; Enter355B/24, Yield797B/56, TryEnter2154B/120 | Charge root entry, changed suffix and cold/materialization machinery. No steady ancestry loop or per-member stores were found. |
| Release .text +13,120B, debug +20,096B | Main executable only, not extension text. No cache/front-end diagnosis. |
| Sampled ordinary release exact/general/native CALL and slot attribute instructions unchanged; RETURN_EMPTY smaller | Limited inspected bodies, not all ordinary/core/native work. Debug slot attribute grows173B. Unchanged samples do not prove zero ordinary runtime cost. |

The raw `E59/retained68-hot8-selected57-screen/selection.json` records stable
identity,32 successful rows and REJECT. Means and pair ranges:

| Workload | Candidate / selected57 | Two-pair range |
| --- | ---: | ---: |
| comp | .989150881 | .962908–1.016109 |
| Delta | 1.101088734 | 1.098108–1.104077 |
| coro | 1.072082993 | 1.068920–1.075255 |
| gen | .708007467 | .707177–.708839 |
| logging | 1.239006537 | 1.187297–1.292968 |
| ray | 1.001915685 | .998443–1.005401 |
| spectral | .892976025 | .891709–.894245 |
| nq | 1.005174110 | 1.001595–1.008766 |

GM .989787857 passes, as does improvement; all-means/logging/all-pairs fail.
These are observed two-pair results, not confidence intervals. Preserve the stop:
no confirmation, all29, PMU, full-suite or timing rescue. Do not multiply gen's
old upstream ratio by .708 to manufacture a current parity result.

## Actual source contrasts and the new removal

| Actual pinned source boundary | Finding and proposed consequence |
| --- | --- |
| I `Python/generated_ir_cases.c.h:11958` exact CALL; `Python/bytecodes.c:4353–4419`; `Include/internal/pycore_interpframe.h:_PyFrame_Initialize` | A transaction begins before exact guards; argument overlap is already present. Header/code ownership, frame/current-state installation and body entry still execute. A complete summary can omit this child lifetime; a new exact binder cannot claim it. |
| I `Python/bytecodes.c:1466–1526`; `Include/internal/pycore_interpframe.h:180–275`; `Python/frame.c:try_clear_except_code` | Result ownership, child unlink/clear and local/discard/scratch publication remain. A summary retains ordinary caller result retirement/redirect but removes child clear/return. Merely renaming EMPTY or adding a return alias removes none of this. |
| U `Python/bytecodes.c:1574` RETURN, `:4418` general Python call, `:4568–4630` exact entry/push | Contiguous stack inputs/recipient and saved offset simplify continuation. U still initializes/clears a frame and owns code. Whole-callee elimination removes shared interpreter work, not just selected overhead. |
| I `Python/bytecodes.c:2710–2850` shape/slot/class reads; `:3177` int compare; `Objects/typeobject.c:8391` value rich compare | Existing read handlers guard, acquire canonical result owners and retire inputs. Extracting checked non-calling reads is a new consumer contract; calling an owning handler and then dropping its result is not the proposal. Base-object EQ can return NotImplemented for unequal objects; identity is valid only with both actual dispatch semantics proved. |
| I `Objects/codeobject.c:3939–3985`; `Python/ir.c:475`; `Include/internal/pycore_ir.h:573–611` | Immutable common admission builds the semantic plan used by execution. Add a complete expression proof here, not a source-line selector or trusted compiler tag. Valid unsupported public images remain ordinary. |
| I `Include/internal/pycore_feedback.h:158–179`; `Python/specialize.c` Python CALL selection | CALL's existing16-byte payload contains live version/binding facts, not a spare pointer. An exclusive scalar summary arm can retain size; actual function/code/plan and globals must still be resolved. Same-code function version does not identify a closure/global environment. |
| G `Objects/genobject.c:1–180,340–405,900–1172`; `Include/internal/pycore_genobject.h:1–220` | Retained state really controls entry, effective frame state, materialization and cleanup. Splitting ordinary/retained ingress is more than deleting a pointer, but does not eliminate universal storage or explain ordinary logging cost. No such source retry is selected. |

These are source-level proposed removals. There is no new summary native result,
coverage count or throughput measurement. The source comparison does not
attribute the selected/upstream measured instruction gap among components.

The independent source-only input
`E59/guarded-summary69-source-peer-note.md`, SHA256
`02226d44ac9d9e0466d8fec08c7d1663d3dbeb6cd59ad5d5988431af32165168`,
confirms the boundary and corrects two tempting shortcuts. I's
`ir_generator.py:return_to_caller` uses `_PyFrame_SetReturnValue` after Python
child retirement, without native CALL's end-periodic/folded-local sequence.
I's `ceval_macros.h:864` compares the full eval-breaker/instrumentation state
before skipping RESUME. Design69 makes an explicit alternative semantic choice:
the complete body becomes a checked atomic operation, with generated B0-rooted
CALL retirement, end-periodic service and destination/discard publication under
the permitted bounded-lifetime policy. It does not preserve exact Python child
finalizer order or pretend the two completion protocols are identical. No new
retirement-root scheme or child frame is added to imitate that order. Actual
caller refresh, B0 error ownership and redirect receipts are hard requirements.
Full RESUME admission still precedes body effects; unsupported platform entry
declines. The plan keeps its 16-byte header with an explicitly owned optional
tail, including summary-only allocation/free/sizeof. Its producer receives real
signature, localsplus kinds and constslot/constants inputs, not just words.
Same-code function versions, arbitrary RHS identity and exact callback-free
attribute/global/equality guards are explicit. This memo is not implementation
GO; the complete source recipe still needs independent adoption.

## Concrete reach without invented hit frequencies

Read retained `E59/resident-selected57-deltablue.json` and
`resident-selected57-comprehensions.json`, including their actual words and
feedback labels. Their monitored instruction visits are semantic observations,
not frequencies of successful specialized guards or future summary hits.

* Delta `BinaryConstraint.input` 351,210 entries and `output` 411,510 entries
  each load a direction, compare a guarded class value, and return one of two
  self projections. The combined 762,720 entries identify a complete select
  boundary, not 762,720 certified future hits.
* `Strength.stronger` 21,240 and `weaker` 36,150 each read two values, compare and
  return; `UrnaryConstraint.output` 10,110 and `is_satisfied` 4,050 are projections.
  The retained name really is `UrnaryConstraint`. Nested-call `weakest_of` is
  excluded from the initial grammar even though it looks small in source.
* Comp `_is_big_spinny` 144,000 entries has a shape read, module-global/class
  read, **generic** equality, short-circuit and optional shape read 72,000 times.
  The last result is the actual RHS. Its equality is not already a proved
  integer comparison. Guarded base-object equality may cover it; no Enum-specific
  admission or coverage is assumed. If it fails that contract, report no hit.

The known comp `_add_widgets` list/dict.get/append cycle remains separate:
six IR words versus upstream eight does not mean less native work. CALL A=B17
already roots its result without the advertised extra move. METH_VALUE dict.get
has no boxing-array adapter to remove. This proposal does not claim those648k
visits as summary traffic or promise the 30.38% comp instruction budget.

## Independent architectural disposition

Rank1 replaces an entire callee with a checked semantic expression while
preserving owning CALL inputs. Its critical distinction from A67 is removal of
child frame/body/clear, not a wrapper around two already owning operations.
Unlike B67, it adds no borrowed frame home, general effect promotion or universal
owner-consumer branch. Unlike compact64, it has no domain entry/exit or private
dispatch loop. The cost of summary guards and ordinary completion remains real.

Rank2's canonical result recipient is a genuine IR convention change, but it
can trade generic completion branches for extra caller dispatch/assignment.
Rank3's predicate regions share useful value proofs but save less per hit than
whole-callee elimination. Rank4 preserves a proven generator mechanism while
separating ingress; its limited savings do not justify retesting a protected
loser. Rank5 would change create/delegate lifetime, not just depth traversal,
but has no cheaper complete identity/observer contract than the rejected await
family. None merits a parallel implementation lane now.

Old reductions remain closed: Invoke59 already removed the duplicate window
sweep and lost; compact64 saved47 internally but added93 at its boundaries;
role63 removed actual branches and lost; A67's complete nominal path grew
228→370 instructions, B67's text grew149,568B and all eight means lost. Code-owner
writer repair remains an unpriced small alternative, not a prerequisite. Native
selected no-reentry clears already fold the apparent duplicated count tests.

Repeated logging losses are not explained by “same ordinary assembly.” Round66's
only authorized diagnostic measured instructions .992588 and cycles1.085426;
changed terminal self shares8.307/8.453%, proved-equal ordinary cleanup
13.416/13.518%, and78.276/78.029% other unproved work. Independent sample shares
do not attribute the cycle difference. No extra counters, compiler, alignment,
padding or flags search is ordered. New candidates must pass broad protection
even if their mechanism succeeds and their local instruction totals fall.

The subsequent lazy-aggregate suggestion was checked against actual I/U and
screened out as existing selected work. I `Python/regcfg.c:81` inserts kwargs
materialization before read folding and declines rebinding/cell operations;
`Objects/codeobject.c:819` independently proves barriers over normal/EH entries
and rejects write/delete/move/cell roles. I `Python/specialize.c:2319` sets the
virtual flag; `Python/ceval.c:4376–4405` omits `PyDict_New` on that direct-plan
hit, installs the empty-tuple singleton and records `pending_kwargs` instead.
`Python/frame.c:245,280` materializes before frame-object exposure, installing
the fresh dict before clearing pending state; `pycore_interpframe.h:399` copies
the pending index. This is canonical NULL plus explicit binding authority, not
a new noncanonical owner. These reads establish the existing mechanism, not a
fresh exhaustive audit of every native/observer consumer.

U `Python/ceval.c:1817` eagerly allocates kwargs, while its empty varargs at1854
already uses the immutable singleton. The retained selected logging JSON has
600,000 `Logger.debug` false-path entries and zero visits to its word11
`MATERIALIZE_KWARGS`; it does not count direct-plan successes or constructor
calls. No new dynamic claim follows. Generalizing to fallback or nonempty
aggregates could retain argument owners until materialization and must close
STORE/DELETE/copy/native-alias/observer semantics; no remaining hot coverage or
net cost is proved. This does not change the five-way ranking or add a lane.

## Handoff and limits

One Sol-high unit implements the common proof plus complete generated CALL
consumer, after independent source peer accepts exact read semantics, actual
function environment guards, all owner/effect exits and atomic observation.
Root owns generated products, both builds, focused tests, actual native hits,
whole-boundary pricing, the single screen and winner-only confirmation/all29/
full diagnostics. Do not enable a proof-only or incomplete consumer stage.
Do not require unfinished M4/bitmap/IRSC2 or resurrect the rejected branches.

The bounded unit is allowed to fail. Failure means stop and return the complete
evidence to a new grouped review, not grow the summary grammar or add a second
callee engine. A winner is an increment: no current combined budget establishes
comp/Delta/gen/coro parity. Last matched selected/U instructions are1.436416,
1.361424 and1.307831 for comp/Delta/coro; generator1.372523 is older elapsed.
No fresh upstream performance or implementation claim is made by this review.
