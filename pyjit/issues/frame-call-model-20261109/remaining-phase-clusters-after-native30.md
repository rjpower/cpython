# Remaining stopped-state clusters after native30

Design/evidence triage only, against committed417a8bd6e2e3. Do not infer phase
from Python event spelling or assume SOURCE_POST from a refusal. The previous
84-row receipt disproved that shortcut: each-LINE failures were legacy LINE
under monitoring JUMP at the actual backward transfer, with zero IRSM rows.

## Four immediate witnesses

Monitoring's test_callback_set_frame_lineno explicitly registers BRANCH_LEFT.
Its function calls s.startswith, then the conditional's fallthrough callback
requests the next source line returning1. The earlier captured actual IR has
POP_JUMP_IF_FALSE pc4 consuming temp1 and target LOAD_SMALL_INT pc5; its source
map is empty. The callback is a non-neutral committed conditional provider,
which the native30 neutral-JUMP consumer deliberately does not authorize.
Current native PC/handler must still be captured before extending the scope.

The three peepholer methods request line3 from line9, preserving public code
bytes and the existing None-binding warning semantics. The older actual
inventory has empty source maps and a real line9 integer assignment. Target
line3 is a conditional followed by range setup/CALL. COPY previously writes a
scratch cell then STORE_FAST takes it; the same home later serves optional
CALL self. The current whole-code never-written NULL proof cannot use a cell
with any historical writer. This suggests a flow-sensitive expected-NULL gap,
not an unfinished source event. It remains a hypothesis until the current
phase/word snapshot is read.

Coordinator-only probe-remaining-debugger-phases.py captures these four cases:
actual stopped handler/PC, target, bound locals, unchanged/committed result,
code words/source records and warnings. It has host AST validation only and
must run on the pinned native image after quiet timing. No C instrumentation or
runtime change is requested to produce the first receipt.

## Two likely structural capabilities, conditional on those receipts

1. Extend the one stopped-state view to positive committed normal-edge
   transfers beyond neutral JUMP. The conditional provider already commits
   its consumed condition and continuation. Derive one exact post-edge row
   with shared transfer/physical facts, cut consumed origins, forbid token
   creation and Analysis parent mutation, then reuse Plan. Keep inherited,
   save-empty/no-target-scratch first. This is not a broad event authorization
   and does not require SOURCE_POST. It is the narrow candidate for monitoring1.
2. Add flow-sensitive expected NULL facts for scratch homes. ABI nullable is
   insufficient: a NULL optional receiver must not replace a required old
   method self. Prove target's NULL expectation from actual ownership transfers,
   and require actual current NULL plus survival to the nullable read on every
   admitted path. Reuse the sparse graph and shared physical transfer contract.
   Explicit register temporary inputs are taken on success; outputs/aliases
   may repopulate them. Window holes, COPY/SWAP and ordinary PRE versus source
   POST errors need their actual transfer, not a rule that every read kills.
   Generic physical writes do not mean bound values. No hot tag or frame scan
   is justified. The three peepholer witnesses are the first candidate gate;
   earlier backward-comprehension failures may share this gap but are not
   counted as repaired without their own actual receipts.

YIELD_READY is a separate third bounded capability with an existing explicit
PY_YIELD witness. A's typed pre-link provider plan can be reviewed independently;
ordinary outgoing frame ownership, payload retirement and unchanged-only
suspension must be proved. Actual suspended frames remain unavailable.

The other24 tracing errors include with/async-with, caught-context exits,
async loop exits, backward comprehensions and yield. This is a list of source
contexts, not a proven phase partition. Retained outer roots need dynamic
survival proof; caught exits need the callback-safe exception-link resolver.
Neither can be replaced by direct exc_info mutation or an opcode blacklist.
Pdb basic_commands still executes never_executed after an unsuccessful jump,
while gh_127321 stops at its explicit entry line rather than the old expected
next line. Those two logical cases require phase/protocol evidence; do not edit
transcripts to conceal them. Prioritize the four immediate witnesses before
selecting a larger context packet.

## Current417 four-witness receipt and24-case source grouping

ownership31-remaining-phases.json now confirms the immediate phase split:
peepholer zero/one/two-undefined stop at actual INSTRUMENTED_LINE17/17/18 and
request line3; monitoring stops at actual INSTRUMENTED_POP_JUMP_IF_FALSE4,
BRANCH_LEFT destination5 and requests the next return line. All four have zero
source records. See nonneutral-post-and-null-requirements-proposal.md for the
concrete two-change proposal. These are actual phase witnesses, not feature
passes.

The24 remaining sys_settrace errors partition by their actual source cases as
follows. This is a context inventory, not an event/owner-phase assertion:

| Source-context witnesses | Count | Required next evidence |
| --- | ---: | --- |
| with/async-with crossing and nested retained scopes | 8 | Exact stopped PC/scope plus retained outer exit owners and any semantic exit actions |
| caught/finally context transitions, including same-caught replay | 8 | Saved-previous lineage; distinguish same-context survival from actual caught exit and resolve callback parent link |
| backward list/async comprehensions | 4 | Target required roots/expected NULL and actual save-generation state, not static producer equality |
| async-for exits | 2 | Actual committed completion versus yield/suspended phase and associated owners |
| jump-from-yield | 1 | Existing explicit PY_YIELD witness, with pre-link outgoing owner and typed cancellation |
| complex nested contexts | 1 | Concrete combined root/link state before choosing an extension |

The8 caught cases are not all caught exits: within_except is a surviving-context
witness and may need a stronger availability slice. The8 with cases cannot be
repaired by merely dropping slots: preserve required roots and any Python exit
protocol actions. Do not count the proposed NULL facts as repairing the4 backward
comprehensions until their exact requirement paths are captured.
