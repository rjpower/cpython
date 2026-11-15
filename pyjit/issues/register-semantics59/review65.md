# Grouped review65: structural await success, ordinary execution debt

Decision: **CHANGE/continue await through the two ordered units in
[proposal65](../../design/proposals/interpreter-parity65.md); STOP compact64 and
reject await9 as a standalone executable.** This is a broad architecture review,
not a component implementation peer or new qualification run. Selected57 is
unchanged. Five concrete alternatives, contracts, Mermaid flows and falsifiers
are in the proposal; this ledger records the deciding evidence and its limits.

## Authority and review method

Canonical navigation was read in `/home/power/code/cpython/.worktrees/python-jit`:
README, GOAL, STATUS, HANDOFF, PROCESS, the active performance56 plan, report,
review64 and deferred-await63. Root published the completed round64 record at
`f56c28268e0e6c8be6ea0913d7c1a9943662e029`. That checkout's runtime is stale.
Runtime source authority is the following three trees, whose HEADs were read:

| Role | Actual source tree | Commit |
| --- | --- | --- |
| I, selected57 | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated` | `0dcba75bb3df40c32137db453b001f7e8f101de0` |
| G, completed await9 | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc` | `0bbb42ef46ab9c55996e9b507a528a12705eedf0` |
| U, upstream | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6` | `3a440b65695615e9fd9912342b79cdd54fced0d2` |

G was clean at review. `BASE` is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`; `E59` is
`BASE/pyjit-link-wave1/register-semantics59`. The selected and candidate copied
ELFs were independently SHA256-read, without execution:

| Artifact | SHA256 |
| --- | --- |
| `BASE/pyjit-link-wave1/call-model55/comp57-dictionary-native/python` | `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2` |
| `E59/parity64-await9-native/python` | `1c8f91ebf882e01250dd2c8a32a96039b76a4eff129d6b45208085412cbc68f8` |

Applicable AGENTS.md and the official [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/)
were read before proposals. Work was source/evidence reading and two documentation
files only. No subagents, generation, build, target runtime, GDB, benchmark,
profiler, control mutation or push. Native listings/JSON are retained evidence;
reading them is not a newly executed measurement. V8/LuaJIT primary sources
were consulted for architectural comparison, not for benchmark claims.

## Completed outcomes

### Compact64 is closed on net work

[Native review][compact] and [explicit paths][compact-paths] identify source
`9de7005b670b1acc0350c22f53f43dc91ccc9f14`, ELF
`d3246e84e58f812cc30241d36db6365708d5372af7a512942c62331e991906b7`.
Both 117/79 builds, eight new plus 21 control methods per role pass in root's
record. This review did not rehash or execute that ELF.

| Complete static path | Selected instructions | Compact instructions |
| --- | ---: | ---: |
| FOR-local, NO_DICT method, ARG, ordinary CALL entry | 173 | 227 |
| APPEND, BACK, FOR-local, NO_DICT method, ARG, CALL entry | 234 | 280 |
| APPEND singleton, unsupported-word exit | 38 | 128 |
| Inline-ID first segment | 169 | 219 |
| Inline-ID append/backedge segment | 230 | 272 |

These include actual branch choices, entry/exit, ENDBR, executed NOPs and dispatch;
provider interiors and the ordinary CALL body are excluded equally. They are
static path counts, not dynamic frequencies or cycles. Internal envelopes save 47;
59 entry plus 34 exit instructions produce a net 46 loss on the full nominal loop
segment. Stack 112 B and hot text 1,665 B pass. Whole text grows 4,800 B; thirteen
inspected ordinary bodies remain normalized equal. No target-hit/timing campaign
is warranted, and no size/layout cause is inferred.

### Await9 qualifies the mechanism and loses breadth

The [source peer][cold], [native review][native], [receipt peer][receipts],
[natural receipt][natural], [terminal receipt][terminal] and [arena-OOM receipt][oom]
agree on the frozen source/ELF above. Root's retained qualification is:

- Both 117/79 builds and all 16 lifecycle methods pass per role.
- 1,066 ordinary runtime controls per role retain only six inherited
  sys_settrace debugger-jump errors.
- 140 reader controls per role pass, including C API frames, remote inspection
  and async graph, with three skips.
- Unchanged `coroutines.run(4)` actually commits a private child and retires
  **the same child** normally without identity creation.
- A real terminal transfer moves a two-frame private suffix within one evaluator.
- Actual private `_PyObject_VirtualAlloc` is forced to return NULL. CALL owners,
  object refcounts, semantic word/PC, current frame, arena and exception links
  are unchanged at child-NULL; helper returns -1/NULL. The Python error word
  matches, and finally, retry and eventual release checks pass.

These are bounded actual transactions. They do not establish hit frequency,
materialization frequency, every retained native alias scenario or a per-call
instruction saving. The broad timing result is independent evidence.

[Stable32-row balanced hot8][screen], candidate/selected57:

| Workload | Geometric mean | Two-pair range | Reading |
| --- | ---: | --- | --- |
| comprehensions | .987018 | .964970–1.009570 | Mixed |
| DeltaBlue | 1.003049 | 1.000341–1.005765 | Both slightly slower |
| coroutines | .571378 | .570239–.572518 | 42.9% elapsed reduction |
| generators | 1.051852 | 1.049759–1.053949 | Both slower |
| logging_silent | 1.276929 | 1.272406–1.281468 | Large repeated loss |
| raytrace | .983750 | .977447–.990093 | Both faster |
| spectral_norm | .924742 | .922693–.926795 | Both faster |
| nqueens | 1.024620 | 1.024589–1.024650 | Both slower |

Reject this standalone executable. There is no await9 all 29, full-system or
fresh upstream result. Neither a geometric mean across these eight nor an old
upstream ratio multiplied by .571378 establishes parity. The mechanism did
not fail implementation; the qualified executable failed selection.

## Actual source and native cost ledger

| Boundary | Source/native fact | Implication for review65 |
| --- | --- | --- |
| [G frame/record][g-layout] | Header 112 B, record 72 B; AWAIT owner and active identity pointer | No object allocation is not no activation state. Private reservation 184 B plus homes remains. |
| [G entry][g-entry] | Disjoint args moved once, function moved, separate code owner, two name references, guards and exception link | Do not credit overlapped private args or zero-cost metadata. |
| [G generator entry][g-gen] | A successful state claim excludes active views; source asserts active_frame NULL, then uses general selectors | Embedded access is a real source-proved specialization boundary. It is not safe for arbitrary getters. |
| [G RETURN recipe][g-return] | Computes GENERATOR-or-AWAIT before clear, then selects completion versus direct publication | Representation work is paid by ordinary returns even without a private head hit. |
| [G GENERAL clear][g-general] | Checks non-THREAD first, otherwise directly clears thread extent | Ordinary GENERAL does **not** enter the shared AWAIT-first classifier. Do not charge it that branch. |
| [G EMPTY/shared clear][g-shared] | `_PyEval_FrameClearAndPopWithResult` enters AWAIT-first shared classifier | EMPTY has a distinct opportunity. Keep its local-only authority separate from GENERAL. |
| [G complete return][g-complete] | Decodes immutable SEND/FOR geometry and distinguishes WAIT from SEND_CHILD | A storage owner is not a semantic continuation kind. Private-child completion must not steal a FOR yield. |
| [G YIELD emitter][g-yield] | Captures private-parent SEND_CHILD across return-slot publication; terminal prepare follows publication | The capture is needed in this implementation. Moving it needs a new executable continuation with complete reader closure. |
| [G line/redirect readers][g-lines] | Interpret return_pc as an in-image semantic location; compare it with observer context | A private trampoline is a representation change, not a harmless pointer substitution. |
| [I descriptor call][i-call] | Result B0 publication precedes callable/input retirement, periodic and destination move | Distinct ownership phases remain after role/copy propagation. Native-result option must replace the whole region. |
| [U return/yield][u-return] | Caller stack/offset and fixed yield successor handle completion | Fewer generic recipient operations are a source explanation of work, not a matched cycle attribution. |

The actual retained RETURN_VALUE listing at 0x39a625 loads owner, then at
0x39a62e–0x39a63b compares1 and 4, executes two setcc and OR, preserves the
classification through clear, and tests it afterward. GENERAL's helper
still has its own legitimate THREAD-first fast branch. EMPTY reaches shared
clear. These paths must be counted separately in unit A.

The `gen_send_value_ex` listing at 0x17a14f–0x17a15e loads active_frame and
selects it against the embedded address; at 0x17a1ba–0x17a1d1 it repeats a
selection for exception state. They survive release compilation despite the
source's admitted-state assertion. Replacing them by direct embedded access
removes established work. Their dynamic share of generator time is not measured.
[Retained native listings][native-dir]

| Complete ELF symbol | Selected B | Await9 B | What the numbers do and do not establish |
| --- | ---: | ---: | --- |
| CALL_NO_SELF_PY_EXACT_ARGS | 871 | 871 | Equal counts/calls/local reservation; residence masks differ |
| CALL_NO_SELF_PY_GENERAL | 1378 | 1378 | Equal counts/sizes; same qualification |
| CALL_PY_GENERAL | 1461 | 1461 | Equal counts/sizes |
| call_bind_known_python | 3115 | 3115 | Equal counts/sizes |
| RETURN_VALUE | 596 | 650 | Real expanded classification/control; local reservation stays16 B |
| RETURN_VALUE_EMPTY | 780 | 674 | Smaller symbol from changed factoring; no automatic dynamic saving |
| CompleteGeneratorReturn | 1856 | 1962 | Private WAIT/observer completion; local 40→56 B |
| SEND_GEN | 776 | 808 | Parent residence/phase selection; no added stack |
| SEND | 2388 | 2594 | Yield outcome test and prepare/error/alias-reload paths; local 224 B retained |
| YIELD_VALUE | 525 | 732 | Residence and parent handoff; local 16 B retained |
| YIELD_DELEGATED | 556 | 756 | Parent continuation capture/handoff; local 16 B retained |
| gen_send_value_ex | 523 | 555 | Actual frame/exception selections; local 32 B retained |
| gen_traverse | 196 | 264 | Active-view owning-edge distinction |
| async_gen_traverse | 204 | 287 | Shared view-layout closure |

Generic CALL and CALL_NO_SELF normalize exactly equal. Generator embedded offsets
shift by 8 B; that is not itself an extra instruction. Identity/GC/remote accessors
have real selection costs, but cold in the proposed organization means a
semantic boundary, not a measured invocation rate. All byte sizes include
cold arms/padding; none is an instruction-frequency or instruction-cache claim.

Private entry has a 450 B head, 999 B helper, 982 B reservation helper and 128 B
link helper. `_PyAwait_EnsureIdentity` is 255 B, preparation 869 B and
`clear_await_frame`807 B. Actual helper register saves/locals count at their C
ABI boundaries; they are not a callee-save failure on every NCSR dispatch.
Synchronous omission is coroutine allocation, created-frame copy/return/pop,
GET_AWAITABLE/None and initial SEND reentry. Real owner clearing remains. Unit A
must preserve this omission while pricing any new return-helper boundary.

## Corrections and rejected interpretations

The final source keeps separately admitted ordinary SEND backedges, normal-only
global-result receipts and source-checked compatible SEND quickening. The terminal
emitter stops automatic parent-phase invalidation after the checked alias switch:
16 generated deletions across four RETURN and two YIELD variants. That is a
control/owner correction, not assertion relaxation or permission to restore stale
WAIT over a callback. Review65 preserves all of it.

The shared active-frame pointer is necessary for generic observation today.
It is unnecessary after an entry claim has proved an embedded representation.
Conversely, removing every pointer test would break active views. An ordinary
code flag is not a residence proof either: public frame/generator construction,
modified flags, alternate ingress and observers must be accounted for by the
common code-image/activation contract before a specialized terminal is selected.

Prior role63 native savings, prepared62 unchanged ordinary bodies and the losing
Invoke/provider packets show why static removals are not selection. Prepared62's
matched whole-process logging ratios were instructions 1.000317, cycles 1.147451,
branches 1.000491 and misses.996456, with both pairs' misses lower. Separate fetch
counts at 83% multiplexing do not establish a cause. No branch-miss, cache-layout,
code-size or flag explanation is established for await9's logging loss either.
No such explanation is needed to reject its screen or to demand a changed design.

The comp and Delta remaining costs are broader than frame clears or one owner
pair. Local borrowing/copy propagation, exact argument overlap, inline normal
completion, initialized frontier, canonical PyValues and shape PICs are selected.
IR_FB is direct cached-base-plus-index. Review65 neither reintroduces those as
proposals nor requires all unfinished bitmap/M4/IRSC2 work first.

## Ordered implementation decision and review limits

1. **Unit A GO:** THREAD-first lowering in existing complete terminal recipes
   and state-proved embedded access. No new return aliases/classification pass: a
   code-only classification cannot remove the public-frame residence guard.
   Keep GENERAL versus EMPTY costs separate and retain complete non-THREAD
   retirement. Source and generation peers must port complete terminal consumers
   and close all public-image/activation ingresses before eliding a test.
2. **Unit B CHANGE:** one private SEND-yield executable continuation, with semantic
   PCs kept distinct and all return-PC readers/writers ported. This moves private
   handoff work out of ordinary YIELD; it does not create a universal call
   descriptor, new ordinary header field or another interpreter.
3. **Defer spine, result ownership region and anchored argument view** under the
   proposal's separate numerical gates. Old smaller-step implementations stay
   rejected. Comp/Delta's parity budget is still open; no claim that A+B fills it.

A and B are complete correctness units. A receives its own hot8 screen after
source/native gates and may advance to selection without B. An A native failure
stops it; an A screen loss permits only the already reviewed B isolation attempt.
The A+B integration receives its own screen, not a repeat rescue of identical
code. A's actual native gate precedes B's shared-source integration. Contract work
may overlap, but at most two units, and root serializes generation/native work.
Tests must exercise the actual generated products, not annotations or helper
names. The minimum includes the existing 16 lifecycle methods, ordinary runtime
and 140 reader controls, plus new terminal/admission/redirect negatives. A
winning coherent integration alone advances to all 29, direct upstream and full
supported diagnostics. Root decides selection; this review creates no native
or implementation claim and changes no selected architecture.

Only `pyjit/design/proposals/interpreter-parity65.md` and this file are owned by
this review. Local-link targets, fences, arithmetic and the final two-file diff
are checked before commit. The proposal's instruction/stack/text and screen
thresholds are prospective falsifiers, not observed outcomes or statistical
confidence bounds. The next grouped review must use actual resulting source,
native and broad-screen evidence, including a failed result if that is what
these bounded attempts produce.

[compact]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-compact2-native/native-review.md
[compact-paths]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-compact2-native/static-path-counts-final.json
[cold]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-native-controls/cold-review9.md
[native]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-await9-native/native-review.md
[native-dir]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-await9-native/await9-static
[receipts]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-native-controls/native-receipts9-peer.md
[natural]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-native-controls/natural9b.native.json
[terminal]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-native-controls/terminal9b.native.json
[oom]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/deferred-await-native-controls/arena-oom9d.native.json
[screen]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-await9-eight-selected57/summary.json
[g-layout]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Include/internal/pycore_interpframe_structs.h:91
[g-entry]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:1996
[g-gen]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Objects/genobject.c:298
[g-return]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/bytecodes.c:1467
[g-general]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:3785
[g-shared]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:3762
[g-complete]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:3732
[g-yield]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Tools/cases_generator/ir_generator.py:3674
[g-lines]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Objects/frameobject.c:3574
[i-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:8479
[u-return]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c:1574
