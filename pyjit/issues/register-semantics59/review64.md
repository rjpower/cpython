# Grouped interpreter/upstream review64

Status: source and retained-evidence review, 2026-11-14. The
[proposal](../../design/proposals/interpreter-parity64.md) orders the next bounded
units. Selected57 is unchanged. This review owns only that proposal and this
ledger; root owns navigation, status, integration and measurement.

Read the applicable AGENTS.md files and the official
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/)
before proposing changes. Canonical README, GOAL, STATUS, HANDOFF, PROCESS,
active performance56 plan, proposal63, review63, deferred-await63 and the trial
report supplied the coordination/history chain. No agent was spawned. No target
interpreter, build, generator, test suite, benchmark or control mutation ran.
Only host source/JSON reads, static ELF inspection and document checks were used.

## Identity and scope

| Authority | Identity and read boundary |
| --- | --- |
| Canonical documentation | `/home/power/code/cpython/.worktrees/python-jit`, starting at `dff92277963f2e1f696c9a43e194463d7fe8c820`; subsequent coordinator messages/await unit updates are identified below. Its runtime source was not used |
| Selected source I | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`, HEAD independently read as `0dcba75bb3df40c32137db453b001f7e8f101de0` |
| Upstream source U | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6`, HEAD independently read as `3a440b65695615e9fd9912342b79cdd54fced0d2` |
| Selected native control | `BASE/pyjit-link-wave1/call-model55/comp57-dictionary-native/python`, independently SHA256 checked as recorded below; static reads only |

The selected ELF's actual verified SHA256 is
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`E59=BASE/pyjit-link-wave1/register-semantics59`. Release control directories
`BASE/call55-integrated-release` and `BASE/upstream-release-devlibs-3a440b6`
were not executed or changed. Source anchors below resolve to I/U, not the
stale documentation worktree.

Await W is `BASE/worktrees/call55-ir`, based on selected57, concurrently edited.
Its working diff is not a frozen implementation. The inspected inactive terminal
snapshot is [terminal2.patch][terminal-patch], independently hashed
`a1669345766642d523f8cc4ad249af241e81d75d961006eb4ff5b7005642a847`, with
[terminal2-peer.md][terminal-peer]. A later partial [send3.patch][send-patch]
hashes to `5df54aefc80b60d4ddda57f97cdc4563a7264709638a64043cf51cf3e0f7783a`;
its source SEND hooks were read as a partial snapshot, not a complete head.
Root reports send3 source peer GO and five later optional source-protocol host
controls passing. Those are coordinator reports, not tests run by this reviewer.
No complete await admission or native result is claimed.

## Round63 is closed: mechanisms pass, executables lose

The retained manifests, native reviews, disassemblies, screen summaries and
status files were read directly. Build/test counts below are the coordinator's
qualification record, not new reviewer execution.

| Packet | Source and qualification | Retained native result | Decision |
| --- | --- | --- | --- |
| List/FOR+append | `034f2b22f87bb4e9e54e689bb5ead8a252e60a97`; both 117/79 builds, 49 focused methods per role; actual comp hits | FOR 1,153→658 bytes, same 32-byte stack and three getter spills/reloads; append 249→217; ELF text +2,496 | Reject logging loss, despite comp/coro/spectral gains |
| Attributes only | `581c018a34a2c4d521cec789e1261a4b331c7dc8`, from source c1066265668 plus fixture035aa; both 117/79, 54 methods per role; all four native first hits | Successful static paths 54→46, 56→49, 88→71, 73→58 instructions, excluding callees; text +3,328, rodata +1,376; new bodies 1,225 bytes | Reject logging/spectral/generator losses, despite comp/Delta gains |

List ELF: `e8d3f5aa293aa030978b60822a77d63450e771adb1f5818dc14cca26edd0761a`.
Attribute ELF: `0b827d770ba806326b82565bb916e8e19f2b5af7d5ead7e4e81cc71c714349a0`.
Evidence: [list native review][role-native], [attribute native review][attr-native],
[list screen][role-screen], [attribute screen][attr-screen]. Ratios below are
elapsed geometric means versus selected57, not confidence intervals.

| Workload | List roles63 | Attribute roles63 |
| --- | ---: | ---: |
| comprehensions | .958819 | .964506 |
| DeltaBlue | 1.009161 | .968592 |
| coroutines | .963592 | 1.002732 |
| generators | 1.004905, mixed | 1.017802 |
| logging_silent | 1.083074 | 1.222917 |
| raytrace | 1.002200, mixed | 1.000004, mixed |
| spectral_norm | .954570 | 1.063393 |
| nqueens | 1.005140 | 1.010458 |

Attribute logging pairs are 1.250450/1.195991; list logging pairs
1.077502/1.088675. Attribute screen status reports stable identity, no failure
and no postflight error. The four attribute paths remove fixed-role decisions,
not their acquisitions, two method stores or old-owner retirement. Ten inspected
ordinary bodies retain normalized instructions and sizes. First-hit probes
include logging method/store paths; final active-site snapshots do not count
invocation frequencies or steady-state store coverage. Qualification and real
entry do not make a losing executable an implementation failure or a winner.
Root correctly stops without all29, full suite, upstream, PMU or repeat rescue.

## Historical constraints, including one tempting false restart

The full trial report was read across rounds59–63. All five round59 executables,
the monolithic round60 engine, round61 packets and allocation/prepared/method/
resume62 remain rejected. Their source mechanisms are not selected dependencies.

The selected descriptor CALL still has two home sweeps. That observation alone
would be an invalid new unit: isolated Invoke59 `5c60e9664faa87c7f6cf1909609554755d83d5ee`
already removed the second sweep and NO_SELF memset, with 66 focused methods per
role and native mechanism GO. FAST shrank 1,697→1,553 bytes, stack 64→48; its
screen still lost logging 1.125967, coroutines 1.081475 and spectral 1.141652.
Comp .968664 did not justify selection. The earlier checked descriptor provider
also remains rejected. Option 4 therefore needs a different result consumer,
not the same source saving packaged as a new discovery.

Prepared62 retained the argument owner/store, next-word decode and feedback
load. Its two descriptor heads added 16 stack bytes; all four added 5,210 body
bytes and 6,912 ELF text bytes. The [native review][prepared-native] has nine
unchanged ordinary control bodies. Method62 likewise removed a real callable
handoff and still lost logging/Delta. Allocation62 removed staging, but retained
object creation/reentry and lost all29. Resume62 removed one indirect transfer,
yet its counted continuation fell only 29→28 instructions with more conditional
branches. These are reasons to change the unit of work.

Round60's [native review](engine-post-review.md) records a 336,799-byte engine,
4,472-byte explicit native frame, one compiler-merged dispatch and repeated
stack accesses for VM state; all eight workloads lost. Selected already pays
the normal callee-save envelope once in its entry shim. Removing five saves per
opcode is not a valid budget. The compact option must earn actual residence
across helpers and small entry/exit costs, not just emit computed-goto syntax.

The [prepared branch diagnostic][branch-diagnostic] reproduces logging
instructions 1.000317, cycles 1.147451, branches 1.000491 and misses .996456.
Its two miss ratios are .998327/.994588. This whole-process interval includes
imports, warmup and seven kernel calls; it is not the kernel-only upstream PMU
interval. The separate fetch diagnostic runs at about 83% multiplexing and
does not establish cause. Unchanged inspected bodies do not imply an unchanged
whole execution path; fewer retired branch misses do not explain the cycle loss.
No cache/layout/flag attribution or counter-fishing campaign is recommended.

## Selected/upstream paths read for this review

These are source facts unless explicitly marked native. They explain repeated
work and retained obligations, not percentages of workload cycles.

| Actual source anchor | Finding and implication |
| --- | --- |
| [I handler ABI and transport][i-macros] | Five values cross NCSR function boundaries. Local/scratch/discard and canonical publication remain generic in many bodies. The new compact consumer must amortize ABI transitions and helper spills, not assume extra native dispatches from opcode counts |
| [I feedback access][i-feedback] | `IR_FB` uses `frame->ir_feedback` plus entry index; no per-hit `_PyIR_InstrFeedback` decode is being proposed away |
| [I register CFG][i-cfg] and [copy propagation][i-copy] | Fresh values are regional; joins use canonical stack-position temporaries. Local folding already exists. No arbitrary SSA/phi/native owner cache can be assumed |
| [I list body][i-for] and [U list body][u-for] | Both require list bounds, item acquisition and cursor update. I additionally has its out-of-line value getter and generic window publication/recipient path. Role63 removed part of publication but kept getter spills and ownership |
| [I append provider][i-append] | Success stores the consumed value, resizing if needed; failure closes it and may finalize. Allocation-only success differs from callback-capable failure; domain error must keep actual append ingress |
| [I generated descriptor CALL][i-call] | Guard → general value call → result into B0 → callable close → argument close → second vacate/self close → periodic → recipient. Those are distinct phases; losing Invoke59 covers only part |
| [I value dispatcher][i-valuecall] and [descriptor wrapper][i-descr] | Descriptor success rediscovers value/object dispatch and validates self/arguments. METH_VALUE `dict.get` has no object-array adapter. NOARGS/O arity validation is still required even after the current FAST guard |
| [U descriptor call][u-call] and [checked helper][u-helper] | Upstream projects the checked C function directly, but still converts StackRefs and cleans the bridge. Do not erase that cost from comparison |
| [I close-home primitive][i-close] and [generator vacate][i-vacate] | The former takes/clears each argument before CLOSE; the latter emits another range clear and necessary self clear. A reached consumed interval could suppress duplication, but that mechanism already lost as Invoke59 |
| [I exact call][i-exact] | Thread callers already overlap args and consume callable into f_funcobj; heap callers move args. Deferred await disjoint moves are an added cost, not removal of a selected copy |
| [I frame layout][i-layout] versus [U layout][u-layout] | I explicitly owns code/function and caches base/executing/feedback/continuation fields in a 112-byte header. A header change has many later readers. No broad frame shrink is justified by two store counts |
| [I RETURN_GENERATOR][i-created] and [SEND/yield][i-send] | Creation allocates/copies/pops and returns an object; SEND reenters it with exception/recursion/recipient state. Deferred await targets that whole cycle. YIELD's nonraising rule is explicit |

**New static ELF read:** `objdump` of the verified selected ELF's
`_TAIL_CALL_CALL_METHOD_DESCRIPTOR_FAST` shows a 0x40-byte explicit stack
reservation at `0x390a67`; the generic slot selection and indirect provider
call remain at `0x390e66–0x390ea1`. Its argument detach/close loop is
`0x390c89–0x390cb5`; the second NULL sweep is `0x390cf0–0x390d0a`.
Result normalization has an actual `_PyValue_Canonicalize` call site at
`0x390beb`, not evidence that every result takes it. These are control-flow
and static instruction facts, not executed counts. Retained attribute ordinary/
role disassemblies were also read: both method owners and final stores survive
the smaller local-role body. No fresh upstream ELF/native count is claimed.

The independent Sol preliminary native-call source note was read and its
comparison conditions absorbed here and in option 4. It adds no approved runtime
packet and need not remain as a navigation dependency after root integration.

## Whole-path coverage and honest budget

Host JSON readback of [resident comp][resident-comp] gives the actual inner loop:

| Semantic word | Pre-monitor adaptive label | Monitored visits, 6,000 trays | Work retained by compact first scope |
| --- | --- | ---: | --- |
| 43 | FOR_ITER_LIST, local4/window15 | 756,000 | Bounds, item owner, Python local publication, cursor; exhaustion includes 108,000 visits |
| 44 | LOAD_ATTR_METHOD_NO_DICT, pair17/local3 | 648,000 | Type/version guard, callable/self acquisitions and both CALL-home stores |
| 45 | LOAD_FAST_BORROW, home19/local4 | 648,000 | Owning argument snapshot and store; only redundant read/role work may disappear |
| 46 | CALL_METHOD_DESCRIPTOR_FAST, result17/base17 | 648,000 | Entire generic CALL/provider, normalization, input retirement, periodic and result publication |
| 47 | LIST_APPEND, list14/item17 | 648,000 | Item take/clear and consuming helper, resize/failure handling |
| 48 | JUMP_BACKWARD to43 | 648,000 | Original polling point and loop target |

The name/indices identify evidence, never admission criteria. Those five
non-CALL words total 3,348,000 visits, 24.48% of all 13,674,014 comp visits.
They are not 24.48% removable native instructions. Natural fast-path frequency,
last-owner fallback and callback/poll cuts remain unknown. First-hit evidence
from other packets cannot fill that gap.

For one successful mapping iteration the comparison is:

`net = old five-handler envelopes + repeated VM-state setup/spills + removable role/PC/load work - domain entry/return/reentry - callee-save envelope - internal dispatch/decode - eligibility/exit work - remaining spills`.

Add the retained ordinary CALL, getter/append internals, all owning stores and
counts, local retirement and polling to **both** totals. Price exhaustion and
first entry separately: FOR has 126 visits/tray but CALL/append have 108.
In particular, an ordinary CALL cut requires the Python local, method pair and
argument to be published. This first scope does not carry those owners through
the provider or elide their lifetime. One borrowed mirror is not whole-loop
scalar replacement. The broader physical change is a reusable finite executor
over admitted CFG edges, with a different state lifetime across helpers and
effects, rather than fused pairs with copied CALL bodies.

The first matched upstream pair has about 278,531 selected vs 193,880 upstream
instructions per tray, a deficit of 84,651. Merely 20 instructions per 108
calls saves 2,160, 2.6% of that deficit. Removing the whole deficit at that
frequency would need roughly 784 instructions net per call/iteration, which
this small boundary change has not established. These scale checks combine
separate runs and do not predict throughput. A useful smaller broad win remains
eligible; a fabricated full-parity budget does not.

The same limitation applies to anchored self: structural eligibility is not
read-only admission, and an extra owner would erase the benefit. Await has a
larger complete operation to remove, but retains body/ownership/recursion and
adds record/reader costs. Its structural 971,136 cycles per four loops imply
about 375 net instructions per cycle to cover the separate first-pair
91,111,408-instruction-per-loop deficit. No such saving has been measured.

## Await snapshot judgment

**Change/continue**, with the proposal's complete source and cost gates. The
terminal2 source already separates stable identity from storage movement:
`_PyGen_Frame` selects an active pointer, `_PyAwait_PrepareSuspend` prepares all
needed identities before committing the same-evaluator private suffix, and
`_PyAwait_DetachIdentity` moves actual frame/exception owners and retargets
frame objects before releasing the old reservation. `clear_await_frame` roots
the result and unlinks/pops only after callback-capable cleanup. The record
root walk accounts for the retiring exception after it leaves `exc_info`.
These are static partial-source observations, not complete protocol approval.

The authoritative unit correctly retains stable active/C aliases, 112-byte
fixed header plus 96-byte await record, actual exception/name owners, disjoint
args with one move, and an evaluator sentinel. Generic frame exposure need not
create coroutine identity. Executing ancestors stay EXECUTING/`cr_await=None`.
No observer can relocate arbitrary active frames. SEND preparation uses its
already-published yielded owner, not a newly raising YIELD.

Root's send3 correction distinguishes an actual SEND continuation from a
FOR_ITER child yield. Callee admission must also reject unsupported reachable
bare-YIELD CO_COROUTINE images unless their real suspension protocol is proved;
caller adjacency and code flags do not establish that fact. Full writer,
reader, observer, debugger, root, retirement and exceptional closure still
precedes any active head. Ordinary generator APIs gain active-frame selection
and object size; record writes and name owners cost on every private entry.
These costs can defeat the expected removal and must be visible in the native
gate. A surviving raw-pointer reader requiring mid-C relocation blocks entry.

## Disposition and handoff

1. Sol specifies/implements the finite compact executor and one NCSR adapter,
   with ordinary CALL cuts and the exact ownership/effect table in the proposal.
   Finite entry aliases retain each ordinary field/base/shadow/error format;
   a shared function is not a shared heterogeneous format. Ordinary exits
   explicitly unwrap private aliases, avoiding adapter reentry on fallback.
   Independent peer precedes root's tiny actual native artifact. Reject above
   128 bytes simultaneous domain/adapter native stack or 16 KiB hot domain text,
   or if VM state remains stack-backed/whole-path savings disappear. No framework
   expansion precedes that gate.
2. In the other source slot, finish the corrected complete await unit, with a
   frozen hot-cost ledger and callee suspension proof. No active head before
   source closure; no synchronous allocation or pervasive ordinary-opcode checks
   hidden in a supposedly deferred path.
3. After a slot frees, revisit the complete delegation spine's reader contract;
   the result-continuation alternative needs new end-to-end removals beyond
   Invoke/provider history. Borrowed self is deferred, not an automatic fifth task.

Both chosen units need actual generated-contract review, both build roles,
focused lifecycle/error/observer tests, native whole-path evidence and natural
entry before hot8. Larger confirmation, explicit all29, matched upstream and
full diagnostics belong only to a winning coherent integration. No new run or
performance result was produced here. There is no combined evidence-backed
parity budget, no proved cause of the broad logging cycle losses, and no reason
to change selected57 until a candidate earns selection.

[terminal-patch]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-source-host/terminal2.patch
[terminal-peer]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-source-host/terminal2-peer.md
[send-patch]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-source-host/send3.patch
[role-native]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity63-role2-native/native-review.md
[attr-native]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity63-attributes2-native/native-review.md
[role-screen]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity63-role2-eight-selected57/summary.json
[attr-screen]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity63-attributes2-eight-selected57/summary.json
[prepared-native]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity62-prepared4-native/native-review.md
[branch-diagnostic]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity62-prepared4-branch-diagnostic/summary.json
[resident-comp]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/resident-selected57-comprehensions.json
[i-macros]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval_macros.h:43
[i-feedback]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval_macros.h:788
[i-cfg]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_regcfg.h:15
[i-copy]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/regcfg_passes.c:154
[i-for]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:14766
[u-for]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c:3897
[i-append]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/listobject.c:420
[i-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:8479
[i-valuecall]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_value_call.h:108
[i-descr]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/descrobject.c:572
[u-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c:5290
[u-helper]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/ceval.c:871
[i-close]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_interpframe.h:222
[i-vacate]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Tools/cases_generator/ir_generator.py:3165
[i-exact]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c:4353
[i-layout]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_interpframe_structs.h:29
[u-layout]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Include/internal/pycore_interpframe_structs.h:29
[i-created]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c:5270
[i-send]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c:1599
