# Source28 correction peer: per-local obligations


The native inventory disproves the earlier GLOBAL-TOP condition. Normal
comprehension restoration stores distinct left then right, while its EH path
restores right then left. Both are valid: different local bindings commute. My
earlier source review missed this normal ordering; the positive native failure
correctly blocked admission. No compiler/test change should force one order.

B's corrected model is approved in direction: one active-top column per distinct
actual LOCAL_MOVE index; each producer's parent links only prior saves of that
same local. STORE requires matching origin, actual LOCAL_OUT and youngest active
SAME-local token. Independent columns restore in either order. Joins are exact
or ambiguous per column; loss remains active; cancellation requires every column
positively0. Bound the N×distinct-saved-locals allocation and validate parent/local
identity. Ordinary PRE-EH and POST-source state distinction remains unchanged.

The negative must genuinely nest two saves of the same encoded local. The old
distinct-local reorder is legal and belongs in positive normal/EH coverage.
Actual-source re-review and native validation of this correction remain pending.

## Actual corrected source disposition

Source review approves the bounded correction for native integration. In
`Objects/frameobject.c`, distinct validated local indices receive separate
columns. The checked allocation bounds both the column map and the PC-by-column
matrix. PRE exception edges retain the incoming vector; normal and source-error
edges carry the operation's resulting vector. Joins meet each column separately.
A matching STORE can pop only its own column's youngest producer, with both
origin and encoded local identity checked. Lost origins do not remove an active
obligation. Source-phase reproof compares the complete resulting vector, and the
cancellation recipe requires every current and target column to be empty.
Temporary vector allocations are released on success and allocation failure.

The five focused tests retain the independent-local positive and missing-restore
negative, and add a genuinely nested same-local ancestor-token negative. The
latter changes the input owner while retaining the same destination local, so it
exercises the youngest-same-local rule rather than imposing an order on distinct
locals. The original executable control remains asserted. The existing callback,
NULL-save and lost-origin coverage remains present.

`git diff --check` passes. This is source admission only: native28v2 and broader
validation remain coordinator-owned. No runtime source was changed by this
review, and the earlier failed-round reviews remain unchanged.

### Fixture container follow-up

Two native harness attempts exposed a distinct constructor opt-in rule. Merely
passing the original `co_linetable` explicitly still fails the guard: codeobject.c
checks its pointer identity against the original container when code bytes change.
My first fixture peer missed that check. The revised fixture makes a byte-equal,
object-distinct `memoryview(...).tobytes()` copy, asserts both properties, and
passes it with the input-home-only code mutation. I inspected the actual guard
and corrected fixture. This supplies the matching unchanged topology explicitly;
it does not relax the constructor or alter the analysis negative. The two failed
harness receipts remain part of the review history. Native retry is pending.

## Corrected native result post-review

The coordinator reports `ownership28-final-small-*` receipts: release and debug
both build with 117 checked modules and pass all 124 focused tests (one skip).
The isolated nested same-local fixture retry also passes in both builds. This
resolves the corrected-model positive and exercises the same-local ancestor-token
negative with its original executable control.

These results qualify this narrow PRE cancellation increment only. The first
GLOBAL-TOP model failure and both source-container harness failures above remain
part of the evidence. No restoration action, source/branch POST cancellation,
suspended-frame redirect or caught-context exit is validated by these results.
The terminal observer ownership repairs remain proposal-only. Wider original
module runs and integration are coordinator-owned.
