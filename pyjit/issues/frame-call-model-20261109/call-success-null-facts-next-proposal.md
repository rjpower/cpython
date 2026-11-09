# Successful call window facts: next bounded capability

The coordinator's 36 mask capture confirms the classification gap. Root
admitted the cold-query implementation after the finite census below received
H's source peer. The N header and contract test are now source-ready and have
H's actual-diff approval. Native validation remains coordinator-owned and has
not run for this packet.

## Executed evidence at 36

The 24-method diagnostic retained original JumpTracer and unittest assertions.
Every method made one request, with no diagnostic cap in the accepted run.
The initial lower-cap run remains failed evidence: warning assertion machinery
hit the probe cap before some target requests. The accepted run used 20,000.
Only backward_over_listcomp passed; this is a capability census, not a passing
qualification gate for the other 23 methods.

| Original case | Actual stopped/target geometry | What is established |
| --- | --- | --- |
| across_with | Real INSTRUMENTED_LINE at pc4, underlying LOAD_GLOBAL; target's first real word pc21 LOAD_GLOBAL. nlocalsplus1. Prior pc3 is discarded CALL with BASE1. | Output is [1]. The first manager factory CALL is later at7 and its enter CALL at13; no manager has entered at the request. Target21 starts the second factory, not a reused body. CALLs at7/24 use optional self2. |
| back_over_comprehensions | Real INSTRUMENTED_LINE at33, underlying LOAD_ATTR; target's first real word4 LOAD_GLOBAL. nlocalsplus4, zero source records. | Both comprehensions completed. STORE31 takes result5 into y; STORE32 restores i from4. Original target CALL6 uses BASE4/self5 after preceding discarded output.append CALL3. |
| backward_over_listcomp | Real INSTRUMENTED_LINE15 to real word1; zero source records; original test passes. | Completed-comprehension replay is already supported when its incoming facts can be proved. There is no general comprehension-jump prohibition to port into tests. |

The current shared physical query deliberately returns UNKNOWN for all CALL
homes because CALL may enter an overlapping child. Therefore it cannot
re-establish a drained optional-self home after successful completion. In the
first two cases that loses the original target's NULL expectation.

The coordinator executed `probe-null-call-masks36-v2.gdb` against committed
36. Both requests have inherited context, no active saved-local obligation,
and a target that is not strictly safe. The exact blocked homes are:

| Request | Home | Physical bits | Target ALL / VALUE / NULL_ONLY | Current / target must-NULL |
| --- | --- | --- | --- | --- |
| across_with 4→21 | 2 | 0 (NULL) | 1 / 1 / 0 | 0 / 0 |
| back_over_comprehensions 33→4 | 5 | 0 (NULL) | 1 / 1 / 0 | 1 / 0 |

The second request's saved-local column is empty at both endpoints. Thus the
refusal is an incoming VALUE classification, despite the stopped home being
NULL; it is not evidence of a missing live manager or active comprehension
binding. The failed first GDB capture (optimized-out `phase.kind`) is retained
separately. The v2 capture reads analysis rows and physical slots; it does not
grant stopped-phase authority. Evidence is in
`E11/null36-call-gdb-masks-v2/stdout.txt`.

Async-with and async-comprehension stops are also recorded, but their code is
coroutine code. The current pathwise NULL analysis intentionally has no fresh
ordinary-entry seed for it. A CALL fact alone does not authorize those cases.
They need a separate positive resume-entry/recipient transfer argument.

The Pdb basic transcript still reports execution of never_executed after jump8
was refused. Its source skips an unentered second loop after print(bar), so a
drained optional-self fact is plausible. No current36 do_jump PC/mask capture
exists in this receipt; do not report it as the same proven failure yet.

## Shared source proof to admit

Extend the existing cold normal physical query, not the setter policy. An
audited completed vector-call window may establish NULL for consumed homes,
with the actual final result destination overriding that clear. Existing PRE
Plan, save/binding checks, all-alias reader capability, physical NULL checks,
warning reproof and strict ALL projection remain unchanged.

Positive proof must cover the common operation and every admitted specialized
or instrumented route. Native call_transaction_detach explicitly clears each
captured input. The Python push/overlap route moves function and arguments,
and its successful child return must demonstrably drain every input home before
publishing the recipient. A PUSHES_FRAME flag alone proves neither clearing nor
the final state. Unknown routes stay UNKNOWN.

For a proved call, decode exact input count and BASE from the generated role/
extent contract. Establish clears only inside that captured window. If the
temporary result destination aliases any such home, its successful new value
takes precedence: that home is UNKNOWN for must-NULL. Local and NoDst outcomes
do not leave a result in the scratch window. Do not confuse the error fallback
window[0] result with a normal success publication. KW/EX need their own shared
count/binder proof before inheriting the vector-call fact.

Keep homes outside the proved input window UNKNOWN where child overlap could
touch them. Keep ordinary error edges before successful publication; they
continue losing facts conservatively. A cached old scope after a finalizer
redirect cannot certify the old continuation. No old-origin token is restored
by a NULL clear, and a downstream VALUE demand must still refuse through it.

## Actual source ledger and remaining proof boundary

The following successful paths establish the proposed window contract. Their
error exits are not successful normal transfers.

| Path | Actual ownership operation | Result and qualification |
| --- | --- | --- |
| Native transaction | `call_transaction_detach` in `Python/ceval.c` copies every captured input to retired owners and writes NULL to every input home before closing them. | Native success publishes a temporary result at the exact encoded destination; that home overrides the clear. Local/NoDst results leave no result owner in the input window. Refresh after arbitrary closes may select a newer continuation. |
| Python fallback binder | `call_transaction_push_python_fallback` retains parent inputs while binding, then detaches the entire window only after a valid child exists. | Parent recipient publication occurs after child return. Failed binding does not establish this fact. |
| Overlapping Python entry | `call_transaction_overlap_python` and `call_bind_known_python` move the function, self and arguments into the child frame. `_PyFrame_PushOverlapping` requires the remaining shared tail to be NULL. | Normal child retirement clears named argument homes; exposed-frame escape uses `_PyFrame_Copy`, which also NULLs the old source cells. Outside-window shared tail remains UNKNOWN. Recipient publication is a separate final override. |
| Direct exact Python entry | `_INIT_CALL_PY_EXACT_ARGS` moves overlapping thread-frame inputs, or copies then clears them for a non-overlapping generator-owned parent, and clears the function home. | `_PyFrame_SetReturnValue` publishes the parent's actual recipient. A pending redirect completes its own transaction; it does not certify the abandoned old edge. |
| Direct native specializations | Generated `CALL_BUILTIN_FAST` clears arguments/self; `CALL_LIST_APPEND` transfers item home2, retires callable/self, and leaves only the result at window0 before the final move. | These routes do not all invoke the common native transaction. Their generated final move has a narrower temporary-destination contract. |

In particular, `IR_MOVE_TO_DST_OR_DISCARD` moves window0 only for a local or
NoDst destination. A direct specialization with a temporary destination leaves
the result at BASE. Therefore a family-wide fact must require temporary
`A == BASE`, or prove a different actual result mapping for that route. It must
not mark BASE NULL merely because the generic transaction could publish at an
arbitrary temporary A. Alias mutants with A at self/argument homes need explicit
negative query witnesses.

The finite census below covers the complete twenty-row `family(CALL, ...)`,
plus generic and instrumented CALL, against the generated handlers in 36/37.
Each row assumes successful continuation of this word; guard misses retry the
same CALL family and do not create a separate successful transfer. Let W be
`[BASE, BASE + 2 + argc)`. The common final result R is either BASE for a
temporary destination, or the encoded named local; NoDst leaves no R in W.

| Reachable rows | Successful W drain and result |
| --- | --- |
| CALL, INSTRUMENTED_CALL | `call_transaction_run_vector`: native detach, fallback detach, or overlapping child argument retirement described above. Method expansion replaces only callable/self inside W. Instrumented callbacks precede the same transaction; normal success uses the same recipient. |
| CALL_PY_GENERAL, CALL_BOUND_METHOD_GENERAL, CALL_NON_PY_GENERAL | Generated transaction forms use `call_transaction_run_vector`, including effect/fallback paths. Bound expansion changes W0/W1 only; argc and W extent stay fixed. |
| CALL_PY_EXACT_ARGS, CALL_BOUND_METHOD_EXACT_ARGS | `_INIT_CALL_PY_EXACT_ARGS` transfers W arguments/self to child locals (or explicitly clears copied inputs), clears W0, and saves the exact recipient. Bound expansion changes W0/W1 only. Child retirement drains the overlapping input cells before recipient publication. |
| CALL_ALLOC_AND_ENTER_INIT | Generated `_CREATE_INIT_FRAME` executes `IR_CLEAR_SLOTS(&window[0], 2 + oparg)` after binding the init child. The shim separately owns self and returns it to the saved recipient; neither init nor shim stores an old input back into W. |
| CALL_TYPE_1, CALL_STR_1, CALL_TUPLE_1 | Guards require argc1 and NULL self. Generated POP clears W1/W2, publishes R at W0, then the common final move handles local/NoDst. |
| CALL_BUILTIN_CLASS, CALL_BUILTIN_FAST, CALL_BUILTIN_FAST_WITH_KEYWORDS, CALL_METHOD_DESCRIPTOR_FAST, CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS | Successful vectorcall replaces callable by result W0; `_POP_TOP_OPARG` plus POP explicitly clear W1 through the final argument before final move. |
| CALL_BUILTIN_O | `native_call_builtin_o` returns the owned pending result; generated code clears W1 through all arguments, publishes result W0 and performs the final move. Native scope owns temporary boxes separately. |
| CALL_LEN | Generated POP chain clears W2, briefly transports the retired argument through W1, then clears W1; only result W0 remains before final move. |
| CALL_ISINSTANCE | Generated code clears W0/W1/W2/W3 while consuming callable/instance/class, then publishes result W0 and performs final move. Guard requires the NULL-self two-argument shape. |
| CALL_LIST_APPEND | TAKE clears W2; successful POP chain moves callable through W1 and clears it, leaving None at W0 before final move. Error cleanup is separate. |
| CALL_METHOD_DESCRIPTOR_O | Guard proves two actual arguments including possible self. Generated tail clear `W[3..2+argc)` and three POPs clear W2/W1; result W0 precedes final move. |
| CALL_METHOD_DESCRIPTOR_NOARGS | Guard proves one actual self argument. Generated clear `W[2..2+argc)` and two POPs clear W1, leaving result W0 before final move. |

This supports one conservative normal-query contract for pristine CALL and
its actual variants: edge FALL only; encoded roles DST/BASE/IMM, argc at C;
reads `2+argc`, through0, and temporary A exactly BASE. Pristine CALL has
writes1; exact Python and constructor/init variants have writes0 because their
recipient is published on child return. Both encoded shapes are covered by the
physical ledger. A named local or NoDst is also allowed. Return UNKNOWN for
result BASE when A is temporary;
return SET_NULL for the other homes in W. Return UNKNOWN outside W, including
unused child tail. Reject unsupported shapes or destination aliases as UNKNOWN
for every queried home. The query does not classify a committed result as NULL,
does not recreate an old-origin token, and does not discharge a VALUE demand.

A handler name, output format, or PUSHES_FRAME bit alone is not this proof.
CALL_KW and CALL_FUNCTION_EX remain outside this first proposal. Future CALL
family additions require extending this ledger before retaining the fact.

Useful source anchors are `Python/ceval.c:call_transaction_detach`,
`call_transaction_native`, `call_transaction_overlap_python`,
`call_transaction_push_python_fallback`, and `call_bind_known_python`;
`Python/frame.c:_PyFrame_ClearExceptCode`;
`Include/internal/pycore_interpframe.h:_PyFrame_Copy`,
`_PyFrame_PushOverlapping`, and `_PyFrame_SetReturnValue`; and
`Python/ceval_macros.h:IR_MOVE_TO_DST_OR_DISCARD`. Generated handlers are the
physical-store check, not a new source of setter authority.

## Files and first gates

H can own the cold query and its private contract witnesses. A independently
audits native/VM/known-call emission and recipient publication. B can own focused
test_code witnesses and root-run diagnostics. No frameobject change is expected
unless the exact masks show a distinct consumer defect; A's forward-caught
increment owns its separate frameobject slice.

First gates retain across_with and back_over_comprehensions unchanged. Add
result-at-self/argument alias negatives, method receiver and mixed-demand
controls, and native/VM return cases with mortal/displaced owners. Reuse
existing callback error/newer redirect tests for actual changed routes.
Measure no hot-path gain from a cold query. Keep coroutine entry, active with
exits, caught exits and SOURCE_POST outside this increment.

## Source checkpoint

The implementation changes only N's `Include/internal/pycore_ir_outcomes.h`
and `Lib/test/test_ir_contract.py`. No hot handler, metadata or setter transfer
changes are needed. The existing backward analysis still retains a VALUE
requirement through SET_NULL; the new physical fact cannot provide its owner.

The new contract method pins exactly the 22 audited rows and checks temporary,
local and sink destinations, self/argument/arbitrary temporary result aliases,
outside-window homes, bad extent/count/edge geometry, and excluded CALL_KW.
Host AST parsing, source family/role/write census and `git diff --check` pass.
No candidate code or native test was executed by this worker. Root's first
runtime gates should include the two unchanged ordinary trace witnesses above,
the full contract method, and existing required-owner/join/redirect controls.

## First native 38 result and remaining merge

Root's release/debug builds pass their 117-module build gates, and the
three-module focused run passes 303 tests with two skips in each profile.
The unchanged original back-over-comprehensions case now passes. The separate
four-case release run still refuses across_with; that failure is retained in
`E11/call38-release-originals-stderr.txt`. The CALL fact alone has not repaired
that case, and the debug original-case run has not yet executed.

The retained actual CFG explains a specific remaining candidate. Target21 has
the ordinary predecessor20 (a discarded CALL, now known to drain home2) and
the first with-handler's suppression tail, which jumps49→21. That tail executes
POP_TOP5 at44, POP_EXCEPT4 at45, POP_TOP3 at46, POP_TOP2 at47, and POP_TOP1 at48.
The current query treats POP_TOP's input as UNKNOWN; therefore an exception
ingress that forgets NULL facts cannot re-establish home2 before the meet at21.
This is a graph/fact hypothesis pending root's fresh 38 mask capture, not a
reason to bypass that meet or weaken the target requirements.

The actual generated POP_TOP handler reads A, calls `IR_CONSUME_INPUT(A, value)`
and closes the detached owner. That macro NULLs a scratch A before closing it;
a local A keeps its binding. POP_TOP has no specialized family or instrumented
twin. An audited normal scratch-consumption transfer could therefore establish
NULL at A while preserving other homes, with ordinary error facts still
conservative. Such a refinement requires its own actual-source peer and root
admission; it is not part of the frozen CALL patch.
