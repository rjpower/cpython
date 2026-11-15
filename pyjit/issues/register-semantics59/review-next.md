# Grouped parity60 review ledger

Status: frozen, 2026-11-14; all five round59 executable screens rejected their
candidates. This is a broad architecture review, not implementation peer review.
Only these notes and the [new proposal](../../design/proposals/interpreter-parity60.md)
are authored by this reviewer. No runtime edits, generation, builds, tests,
perf runs or subagents.

## Source and evidence identity

Canonical docs: `/home/power/code/cpython/.worktrees/python-jit/pyjit`.
Selected runtime source: sibling python-boil-the-ocean
`build/worktrees/call55-integrated`, verified HEAD
`0dcba75bb3df40c32137db453b001f7e8f101de0`. Upstream source:
`build/worktrees/upstream-3a440b6`, pinned
`3a440b65695615e9fd9912342b79cdd54fced0d2`. Canonical runtime files remain stale.

Retained selected ELF: `E55/comp57-dictionary-native/python`, SHA256
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
Measured upstream executable is
`build/upstream-release-devlibs-3a440b6/python`, as named in
`E57/dictionary-perf-stat/stat-r1-upstream-comprehensions-cycles-instructions-command.json`.
E59 is `build/pyjit-link-wave1/register-semantics59`.

Reviewed canonical STATUS/HANDOFF, round59 reports/proposals/post-reviews,
round58 source/native ledger and actual selected/upstream sources. Applicable
AGENTS, CPython AI policy and project process continue to govern the work.
Source facts, monitored opportunity weights, static native differences and
elapsed measurements are separate evidence types.

## Backend comparison is an actual control difference

Upstream measured `pyconfig.h:1995` defines USE_COMPUTED_GOTOS=1 and `:2240`
leaves `_Py_TAIL_CALL_INTERP` undefined. Its `ceval_macros.h:129–134` selects
local targets; `ceval.c:1274` installs the table inside the evaluator.
`nm -S` reports `_PyEval_EvalFrameDefault` at 0x29b110, size 0xe433. Retained
native disassembly at its entry shows the normal callee-save/prologue once,
including a 0x168 native stack reservation. This is not a per-bytecode cost.

Selected `ceval_macros.h:45–150` defines separate handler functions and five
state parameters. GCC already supplies no_callee_saved_registers; the once-per-
activation shim addresses the documented GCC15.2 entry issue. The
[backend audit](resident-value-proposal.md) identifies several handlers with
four incoming/three outgoing state copies and repeated native frame setup,
while SHAPE_LOCAL_TEMP has little and JUMP_BACKWARD_NO_INTERRUPT has none.
Getter spills and native provider live ranges are not all dispatch overhead.

This supports a same-GCC resident-engine experiment, not a claim that computed
goto is inherently faster. A monolithic function can introduce spills and
larger cold-path live ranges. Upstream's register assignment and object-valued
operations also differ; their total gap is not attributable solely to backend.

## Value/provider boundaries actually reached

Selected `pycore_value_call.h:109` checks pointer space, type vectorcall support
and value entry before falling back to an object adapter. `dict.get` has a
METH_VALUE Clinic entry (`Objects/clinic/dictobject.c.h:115–141`), so the ordinary
route is descriptor value dispatch → self/keyword checks → recursion-protected
`method_value_invoke` → Clinic arity/default parsing → dict implementation.
`descrobject.c:607–644` shows that route. It does not box an object argument
array through `_PyValue_CallAdapter`. The latter is a real separate adapter in
`Objects/call.c:1288`, including borrow scope, vector conversion, object call,
result check and canonical conversion; its frequency has not been measured here.

Upstream's checked descriptor producer (`bytecodes.c:5290`) projects cfunc and
calls `_PyCallMethodDescriptorFast_StackRef` directly. Selected's generic
value-call rediscovery is avoidable in principle, but the rejected57 checked
entry already attempted part of that saving. A new packet needs an actual
different whole-boundary consumer and a fresh, nonoverlapping native budget.

Selected LIST_APPEND (`bytecodes.c:1239`) consumes its item and calls
`_PyList_AppendTakeValue`; it is not a generic Vectorcall. Its retained native
symbol is 0xf9 bytes at 0x354090, append provider 0x12b bytes at 0x1af290.
`PyList_GET_VALUE` is a 13-byte accessor at 0x1ae8f0; caller spills make its
boundary more expensive than its body. Upstream's `pycore_list.h:36` inlines
the spare-capacity append path. Rejected57 already tried that family of changes;
symbol size is neither dynamic cost nor permission to restore it as a winner.

Value constructors already canonicalize: `pycore_value.h:815` dispatches exact
numbers to `_PyValue_FromNumberSteal`; `Python/pyvalue.c:26` returns their
canonical identity and releases a now-private numeric box without Python
callbacks. `_PyValue_FromObjectResult` delegates to this constructor.
`_PyValue_Publish` still performs general pointer/number checks for a C-local
value without a canonical receipt. Ordinary homes already guarantee canonical
values, and selected scalar/SHAPE/container facts skip some publication work.
Do not describe this as incomplete universal-value integration.

`PyValue_AsKnownObjectBorrow` retains a release pointer-space check;
`PyValue_IsObject` is a fuller valid-pointer test. `IR_RETIRE_LOCAL` first asks
whether an old owner can deallocate, then closes a nonfinalizing value or defers
its retirement. Typed owner/representation contracts may remove repeated native
classification, but the release code and dynamic frequency must prove it. These
checks are distinct from debug-only `_PyValue_CheckHome` and window scans, which
are absent from release execution.

## Frequency evidence and limits

All eight retained E59 JSONs complete with stable identities. The
[coverage ledger](resident-coverage.md) is authoritative for its finite packing.
It weights monitored semantic visits by pre-monitor adaptive labels; monitoring
resets specializations. It excludes unrelated libraries/native helpers and
does not establish successful runtime guards.

Selected comprehension rows give 1,686,000 FOR_ITER_LIST, 1,260,000
SHAPE_LOCAL_TEMP, 1,236,002 LOAD_FAST_BORROW, 972,000 LIST_APPEND and 648,000
CALL_METHOD_DESCRIPTOR_FAST visits over 6,000 trays. Thus 281 iteration entries,
210 SHAPE entries, approximately 206 surviving borrow-load entries, 162 append
helpers and 108 dict.get provider calls per tray. These are label weights, not
a native-path sample. Folded reads already removed elsewhere are not added
back into a hypothetical borrowing budget.

Finite pilot motifs remove at most 1.58% of recorded comprehension dispatches,
.88% DeltaBlue, 4.36% nqueens, 11.99% raytrace and 8.65% spectral_norm, with zero
for coroutine/generator/logging in this motif set. Same-line and scratch-only
filters are conservative pilot restrictions. Wider regions with actual observer
maps and nonfinalizing local retirement are not prohibited. No bound for every
possible resident execution strategy follows from this table.

Coroutines have approximately 40% boundary words, generators 56.51% plus 24.87%
no-interrupt backward jumps, logging 28.17%. These actual call/resume/return
populations explain why an ordinary scalar-pair catalogue cannot be the whole
strategy, even when its implementation is correct.

## Completed dispositions

Round59 final publication f61 and standalone builder d458 are rejected. The
builder+Invoke composition582 passes both 117/79 builds and 66 focused methods
per role, with source/native GO, but its screen rejects it: DeltaBlue +5.8%,
logging +12.8%, spectral_norm +11.2%, comprehensions near flat. Actual removal of
duplicate clearing and FAST stack 64→48 is preserved evidence, not a selected
performance saving.

Isolated Invoke candidate5c60 passes both 117/79 builds and 66 focused methods
per role. `E59/invoke-only1-native` retains actual FAST/NOSELF sizes 1,553/1,473
bytes, 48-byte native frames and no duplicate memset, as in the composition.
The stable-identity screen `E59/invoke-only1-eight-selected57` rejects it:
comprehensions 0.968664 (0.949679–0.988028), DeltaBlue 1.008872, coroutines
1.081475 (1.076100–1.086870), generators 0.992578, logging 1.125967
(1.106540–1.145730), raytrace 0.992047, spectral_norm 1.141652
(1.111480–1.172650), nqueens 1.006120. These are coordinator-supplied elapsed
ratios. Removing instructions from these handlers does not explain unrelated
workload losses; no exclusive cache/frontend/layout mechanism is established.

The [recipient post-review](recipient-post-review.md) shows generic binding
three→twelve instructions; scratch publication three action branches/local
four; four sampled symbols grow 85 bytes total. SEND's known-scratch bind is
cheaper than generic classification, but its child has a separate action.
The new local-yield fixture correction is a test-shape issue, not evidence of
a production defect. Corrected candidate14bf4eb passed both 117/79 builds,
112 focused methods per role and three remote-protocol methods per role on
the same production c1da. The stable-identity screen at
`E59/recipient3-eight-selected57` rejects it: comprehensions 0.972637
(0.948980–0.996885), DeltaBlue 1.154853 (1.149620–1.160110), coroutines
0.996981, generators 1.054556, logging 1.115302, raytrace 1.004560 with mixed
pairs, spectral_norm 1.101446 and nqueens 1.019988. These are coordinator-supplied
elapsed ratios, not new reviewer measurements. No tag-layout/retiming rescue.

## Concrete next consumers

The backend source peer requires labels before new handler scopes, per-entry
initialization of bridge boxes/owners/temps, lexical error-temp consumption,
UNKNOWN_OPCODE/table/prototype updates, exact prediction/SAME_OPARG semantics,
and staged-child cleanup on failed inline dispatch. The proposal incorporates
these requirements. A single normal-ABI engine entry replaces repeated handler
ABIs; preflight/debug symbol readers must truthfully describe that change.

The next semantic consumer is cross-word ADD_INT → MULTIPLY_INT in eval_A
words 5–6, then separately rounded MULTIPLY_FLOAT → ADD_FLOAT at the two
spectral multiply-loop sites. Selected `bytecodes.c:634–695` shows inline-int
arithmetic, overflow boxing and per-word guards; `:857–914` shows float operand
checks and potentially allocating NaN publication. `ir_generator.py:2451`
defines the existing ScalarStorageGraph. This graph is producer-local today;
the proposed cross-site binding must preserve its actions rather than invent
an independent owner protocol. The first fast path carries only inline scalar
results, defusing before allocation or publishing a completed first result
before the second ordinary word. No callback holds an invisible heap owner.

These are concrete native hypotheses, not proven savings. Descriptor/guard
loads can erase the omitted intermediate home traffic and second dispatch.
Coverage is substantial only in the documented numeric workloads; it does not
explain the full comprehension/coroutine deficit. Provider retries without a
materially different consumer are closed, and broader operand-cache/ABI/format
changes have explicit reopening rules rather than automatic implementation
admission.
