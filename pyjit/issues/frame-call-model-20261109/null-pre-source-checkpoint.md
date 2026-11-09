# NULL PRE source checkpoint

Implementation is isolated in null-pre-20261109 at base bfd67cbe4b41.
It is not yet compiled or executed. Root owns applying, native qualification
and integration; workers made no candidate runs or commits.

B changed Objects/frameobject.c and Lib/test/test_code.py. Eligible ordinary
real-LINE PRE requests allocate pathwise must-NULL rows and two demand lanes
within the existing liveness fixed point. Other phases keep the old one-lane
resource shape. The strict lane retains old requirements and unions VALUE and
NULL_ONLY after convergence. The new Plan requires VALUE empty, ALL contained
in NULL_ONLY, empty stopped/target saves, inherited context, stopped must-NULL
and actual NULL cells. It preserves no missing owner.

Forward joins intersect reachable ingresses, including the implicit entry
predecessor on a backedge. Transports read old rows simultaneously. Errors lose
all NULL facts. Backward SET_NULL discharges only NULL_ONLY; it cannot satisfy
VALUE. Ordinary PRE error needs join after successful definition kills. New
allocations are checked together against the existing cold budget and released
by the analysis owner. Warning reproof rebuilds facts and actual reader roles.

H supplies the shared physical query, private raw witness and contract tests.
The query certifies COPY/SWAP transports, STORE_FAST clearing and declared FOR
completion clearing. Other affected homes are UNKNOWN. FOR's generic pristine
word can specialize to a child-entering variant, so all other FOR homes/outcomes
are UNKNOWN rather than relying on one row's PUSHES_FRAME flag.

Four new test_code methods cover cold/warm COPY/FOR replay, method-owner
refusal, warning getter/newer redirect, and two encoded join controls. Those
controls mutate only an untaken branch to lose must-NULL or introduce a mixed
VALUE/NULL_ONLY requirement. The three original peepholer witnesses are
untouched. Root's compile-only receipt
E11/null-pre-join-geometry-before-native pins both branch COPY bases at 8,
their STORE input at 9 and the unique CALL base at 8 (optional self 9).
The test selects COPY pc7 and STORE pc8 on branch line+3; its mutations match
those actual words. Neither fixture function was called. N native behavior
remains unqualified.

The final coding pass names the requirement lanes ALL, VALUE and NULL_ONLY
and their count. It changes no transfer or strict union rule. The mapped
transport buffer is allocated only for the optional multi-lane PRE analysis.

Host AST parsing and git diff --check passed. A independently approved the
consumer callback/phase source; H's consumer pass found no remaining blocker
after VALUE-through-NULL and ALL-subset fixes. B approved H's query/probe/tests
after the FOR variant refinement. These are source reviews, not native passes.

## Merged source and first native gate

Root applied the reviewed five-file packet to the primary runtime at committed
base `df12a12b131384976466310b7a6138417b7a36eb`. Independent review of the
merged diff found the intended bounded contract intact: COPY/SWAP transport
from the old row, STORE_FAST and completed FOR establish only their audited
NULL homes, errors and unproved writes lose facts, and strict ALL requirements
retain both optional demand lanes. The new permission still requires ordinary
PRE, inherited context, empty saves, compatible demands and an actual NULL
cell. The raw-query tests and encoded join controls exercise those limits.

Both release and debug builds check 117 modules. The corrected three-module
minimal gate passes 300 tests with two skips in each profile
(`E11/null36-minimal-v2-{release,debug}-focus.log`). The first minimal run
failed only its new nested-warning fixture: redirecting again while both `x`
and `checkpoint` were unbound emitted a second warning. The fixture now asserts
the first two-local warning and binds both locals before the nested redirect;
the original refusal, getter and result checks remain. `git diff --check`
passes. The broader frame/observer gate and full qualification are still
pending; these results do not establish debugger coverage beyond the tested
PRE cases.
