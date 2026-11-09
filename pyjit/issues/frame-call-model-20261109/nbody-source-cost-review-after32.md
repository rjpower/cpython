# Nbody source cost questions after Native32

Read-only comparison of current `488e85fa3a8` with golden `0765e9be47f`.
The coordinator's broad screen reports nbody1.05754 candidate/golden. This is
an elapsed observation, not an attribution. No runtime, benchmark or build was
changed or executed by B.

## The repeated work is inside one arithmetic frame

The unchanged kernel calls `advance(0.01, 20000)`. Its pair loop performs float
arithmetic and list element updates; each outer iteration also unpacks and
updates the five bodies. Python frame creation is outside these inner loops.

The retained fourth-nbody-census is from `192cfbe8aa0`, not Native32. It records
17,560,695 entered handlers per benchmark outer loop and three CALL_PY_GENERAL
and RETURN_VALUE executions. Its dominant selected routes include 3,200,042
LOAD_FAST_BORROW, 2,700,120 MULTIPLY_FLOAT, 1,300,070 ADD_FLOAT, 1,200,080
SUBTRACT_FLOAT, and 1,500,000 each list subscription, list store and COPY.
The retained candidate/golden selected route counts differ only by removed
POP_ITER40005 and POP_TOP2. POWER and the total census fail the strict linearity
check, so these receipts must not be presented as a new current instruction
ledger or exact linear cost model.

This workload shape puts per-result and per-loop-exit work ahead of frame
reservation as an added-work hypothesis. The initialized-frontier reservation,
extra header fields and pending-return handling do add checks at call/return,
but the historical call frequency is tiny beside the arithmetic count. Header
size can also affect placement; this source review supplies no cache-cost
measurement. Hot frame fields before the newly inserted return_pc retain their
source layout offsets, and IR_IS_LOCAL still reads nlocalsplus from the executing
code header.

## Two concrete added-work hypotheses

1. **Temporary result discard checks.** Golden IR_STORE_DST distinguishes local
   and temporary destinations. Current IR_STORE_DST_IMPL adds an IR_NO_DST test
   on the nonlocal path. Likewise FOR_ITER_LIST/RANGE body publication now uses
   IR_MOVE_TO_DST_OR_DISCARD, adding a discard test where golden only moved a
   local destination. Ordinary temporary results pay this check even when they
   have a normal recipient. This is a source-visible added branch, not proof
   of an emitted or mispredicted machine branch. Current four float producers
   and LOAD_FAST/LOAD_FAST_BORROW also use canonical final stores, removing
   publication checks. Net instructions and layout cannot be inferred by
   counting the added condition alone.

   Current PMU should distinguish an instruction/branch increase from a CPI
   increase. Inspect the actual compiled hot local and temporary routes for
   arithmetic and LOAD_FAST_BORROW, with destination categories from the same
   current IR identity. Compare complete store paths, including the publication
   checks golden executes and the input/displaced-owner retirement both retain.

2. **FOR exhaustion's common completion helper.** The generated list/range
   handlers now call _PyEval_CompleteStep at exhaustion. Golden jumped to a
   POP_ITER which consumed its two homes and closed the iterator. Current
   completion additionally enters the common eight-argument adapter, publishes
   its held value, checks monitoring, detaches the window, writes continuation
   state, interprets status and retires held owners. The helper handles SEND
   and THROW as well as FOR; a normal FOR completion passes None even though
   it defines no item destination. The helper's initial _PyValue_Publish is
   therefore a concrete value-classification site to inspect in this route.

   The historical40005 removed POP_ITER executions make this a repeated inner
   loop-exit question, unlike the three Python returns. Removing a dispatch
   does not prove lower total work. Count/profile the current FOR-kind path
   and compare it with golden's exhaustion plus POP_ITER, including actual
   closes. A larger generic helper in source does not establish its cycle cost.

The second question overlaps A's
[generator/coroutine review](generator-resume-cost-source-review.md): those
workloads repeatedly reach shared creation/resume/completion paths, while nbody
principally reaches FOR completion. Their distinct frequencies and ownership
routes require separate PMU evidence; no common percentage budget is inferred.

## Useful controls

The current generated BINARY_OP_SUBSCR_LIST_INT, STORE_SUBSCR_LIST_INT, COPY and
STORE_FAST bodies are textually unchanged from golden. UNPACK_SEQUENCE_LIST and
TUPLE change their folded-local helper to the canonical transfer form, replacing
publication with an already-canonical home move. That is not a source-added
normalization loop. Their actual emitted assembly remains a valid profile
control because shared macros and surrounding interpreter layout can change
without changing a handler body.

Native32's debugger analysis and copied stopped rows are cold and do not run
for an unobserved nbody loop. A profile must verify observation remains inactive
rather than assign that source's size to ordinary dispatch. The older c142
instruction/cycle samples locate historical arithmetic/subscription work only;
they do not quantify the current canonical stores or helper path. The
[current PMU plan](current-pmu-plan-after32.md) is the next discriminator. No
runtime optimization is admitted by this review.
