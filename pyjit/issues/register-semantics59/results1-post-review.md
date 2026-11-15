# First result binding: actual source review

Reviewed commit `f61edc21a4efc3e3a09ee99bbca9d031ee9565dc` in
`R/call55-gc`, including all five changed files and the generated C diff.
Conclusion: **source GO; no must-fix finding**. Native semantic and performance
qualification remain coordinator gates. This is a post-implementation review,
separate from the earlier [direction review](peer.md).

## Ownership and emitted behavior

The generated change is confined to FOR_ITER_LIST's final BODY commit
(`Python/generated_ir_cases.c.h:14833`). Acquisition, type/index/bounds guards,
cursor update and exhaustion completion are unchanged. The cursor is published
first. For local A, the new code explicitly writes NULL to B+2, then uses the
existing `IR_STORE_DST` normalization/replacement protocol. That protocol publishes
the new local before retiring its displaced owner; `finish_displaced_local`
retains its existing continuation, refresh and error handling. No callback was
inserted between acquisition and the committed shape.

Scratch and discard A retain their original window publication and move/close.
In particular, the source binding does not advertise a physical publication to
the IR_NO_DST sentinel. The separate local and nonlocal predicates are real
emitted work; native optimization and cost still need inspection. The retained
B+2 vacancy store is intentional. This packet neither removes that home from
the frame extent nor proves its incoming bits NULL.

## Generator and authority boundaries

`FinalOutputBinding` is resolved against external IO and the existing BODY
outcome (`ir_generator.py:2433,6623`); destination_field comes from the format's
actual ROLE_DST. It does not redirect `IRStack.address`. Only the final flush
installs the placement binding, after the existing completion receipt matches.
The output must be the current, unspilled, unaliased owning Local without a
reload or deferred-observer commit.

The selected direct store returns False, so Storage receives no false claim
that B+2 contains the result on both runtime arms. Finalization kills the Local
before subsequent Storage use (`:6651`). A copied stack explicitly drops the
candidate and blocks its placement (`:2736`), preventing cross-path Local reuse.
Callbacks, spills, output reads/aliases/addresses and control joins revoke the
candidate. Pre-acquisition output address checks recover the actual identifier
from the existing validated address site; unrelated earlier exhaustion
addressing does not poison BODY.

PyValue_XDUP's one shared HelperEffect now states its actual owning-result
contract, preserving its existing nullable and noncanonical/non-NULL summaries.
The receipt query runs after a reached ordinary statement (`:4059`), checks the
complete direct RHS and arity, and refuses helper shadowing/preprocessing.
Ordinary native calls are not generator replacers; nested XDUP in
IR_WRITE_OUTPUT therefore retains normal source emission. No new normal/error,
canonicality or frame-readability authority follows from this receipt. Window
scalar graphs remain UNKNOWN, and exported final_output_binding requires the
actual conditional publication receipt. The commit changes no runtime format,
ownership-map or admission header.

## Coverage and remaining gates

The source fixtures cover actual FOR_ITER_LIST in both record modes, the
conditional local/nonlocal publication order, mandatory source vacancy,
callback/spill/alias/control/address mutations, complete-call identity, helper
shadowing and declaration renaming. The actual UNPACK_SEQUENCE_LIST nested-call
control addresses the generation failure found during integration. Root reports
the full generation gate passes with only this handler's C output changed.

The runtime fixture warms and verifies the LIST specialization, then replaces a
last-owned local while its finalizer reads the committed item, mutates another
local, clears the source list and runs GC. It also checks exhaustion preserves
the previous local. This is meaningful lifetime coverage; it is not evidence
that every image/observer route has executed. Root subsequently reports both
117/79 builds and 37 focused methods per role pass. The reviewer ran no commands
that execute target Python, generate code, build or run tests.

Followups, not source blockers: execute temporary/discard and instrumented
controls through existing completion tests, verify the retained NULL poststate
under observation, and inspect whether the native local path actually removes
staging stores without harmful branches/spills. Preserve the selected57 control
and unchanged workload gates. Do not describe this packet as allocator freedom
or an overall interpreter performance result before those gates finish.

## Retained native result and disposition

The retained native comparison is
`E59/results1-native/_TAIL_CALL_FOR_ITER_LIST.txt` against
`E59/results1-selected57-for-list.txt`. The handler grows from 1,153 to 1,225
bytes, and its stack allocation grows from 32 to 48 bytes. The list getter and
its three saved argument-state values remain unchanged. The local arm removes
the intermediate item store to B+2, while retaining its explicit NULL write.
This does not remove item owner acquisition.

The generated mode split also changes normalization placement. The new local
arm loads the displaced owner before item normalization, with five saved values
around its out-of-line normalizer call (`0x395b8d`). Recipient classification
occurs at `0x395949` and again at `0x3959aa`; native compilation did not reduce
the two source predicates to one classification. These are costs of the actual
binary, not an attribution of the measured regressions to a particular branch,
spill or layout change.

Root's first stable hot-eight screen rejects this candidate: coroutine ratio
1.087467 and spectral-norm ratio 1.315221 both lose in both pairs, alongside
mixed or smaller changes elsewhere. Preserve this source and evidence as a
rejected experiment. The next independent builder packet must remove the FOR
opt-in and its conditional staging/move code, retaining only the owner-receipt
and final-commit infrastructure that the builder contract actually uses.
