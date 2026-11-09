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

## Current PMU qualification and static inspection

The accepted current capture changes the priority of the source questions
above. `ninth-workload-perf-stat-v2` at the same current/golden identities reports
cycles1.056129, instructions0.986232 and CPI1.070872. The three nbody instruction
ratios are identical. The workload now retires fewer instructions while taking
more cycles. Therefore the earlier c142 added-instruction explanation and a
simple count of added discard tests cannot explain this binary's net loss.
The source observations above remain historical hypotheses about particular
routes, not a current whole-workload added-work finding.

Read the accepted raw flat cycle records with no children/call graph. Current
versus golden shares are MULTIPLY_FLOAT15.50%/15.70%, LOAD_FAST_BORROW9.56%/11.56%
and list subscription9.68%/8.67%. Both records have zero lost samples. These are
location shares in one approximately2,000-sample capture per role; they are not
per-handler cycle ratios. The flat symbol-offset report is retained in
`nbody-flat-locations-after32.txt` with its exact read-only report commands.

Static `nm -S`/`objdump` inspection of those retained ELFs gives:

| Handler | Golden/current bytes | Golden/current static conditional jumps |
| --- | ---: | ---: |
| MULTIPLY_FLOAT, ADD_FLOAT, SUBTRACT_FLOAT each | 1276 / 1134 | 38 / 33 |
| LOAD_FAST_BORROW | 461 / 310 | 17 / 13 |
| BINARY_OP_SUBSCR_LIST_INT | 932 / 1225 | 40 / 47 |
| STORE_SUBSCR_LIST_INT | 589 / 589 | 19 / 19 |
| COPY | 218 / 218 | 6 / 6 |
| STORE_FAST | 159 / 159 | 3 / 3 |
| UNPACK_SEQUENCE_LIST | 822 / 672 | 28 / 22 |
| UNPACK_SEQUENCE_TUPLE | 822 / 672 | 28 / 22 |
| FOR_ITER_LIST | 1231 / 1216 | 35 / 34 |
| JUMP_BACKWARD | 116 / 116 | 2 / 2 |

The full selected-symbol table, addresses, alignments, instruction-line counts
and ELF hashes are in `nbody-static-handler-layout-after32.json`. Static jump
counts include cold/error/sink paths, not just executed hot branches. The list
subscription's generated C body is unchanged, but shared macro expansion and
compiler block layout differ. Thus unchanged source bodies are controls, not
proof of identical machine paths. The smaller numeric/load functions also
change addresses and block placement. Most compared function addresses change
their offset within a64-byte line; that observation supplies no cache or branch
predictor diagnosis.

The leading MULTIPLY_FLOAT sample offset is +0xd8 in both ELFs (current5.50%,
golden4.27%). In both, +0xd4 is the same MULSD and +0xd8 is the same LEA of the
next instruction, followed by the same saved-pointer move and NaN check. A
sample at the LEA cannot be read as the latency of that instruction or proof
that a newly added store branch caused it; sampling skid and predecessor work
remain relevant. COPY also peaks at the same +0xb5 despite its unchanged size.

The entire executable .text grows from4,872,313 to4,951,993 bytes, while several
hot functions shrink. The dispatch table grows by one8-byte entry and is
64-byte aligned in both ELFs. These are placement/footprint facts, not evidence
of instruction-cache, BTB or data-cache misses. The coordinator's current
branch group can test branch counts/misses next. A lower miss count would not
identify a different stall source; a higher miss count would still need actual
path evidence. No layout experiment or destination-specific opcode variant is
proposed from these static facts.

### Current branch counts close the added-branch question

Independently recomputed all six raw branch counters divided by their own hot
loop counts in `ninth-nbody-branches`: identities are stable and every event
runs at100%. Mean branches candidate/golden are0.976533757215, so the current
binary executes fewer branches as well as fewer instructions. Mean misses are
1.014469706509, but the three candidate normalized miss counts are
1,171,909 /1,120,788 /1,120,641, versus golden1,120,833 /1,122,803 /1,121,017.
Only the first candidate is elevated. The median miss ratio is0.999795409222;
this diagnostic does not replace or hide the accepted mean, but it demonstrates
that the campaign has no consistent miss increase across rounds.

Extra branch count or a persistent branch-miss penalty therefore is not a
supported explanation of the measured nbody loss. Stop this measurement branch
rather than tune alignment, collect arbitrary counters or create destination
variants. Prioritize the generator/coroutine completion route with its measured
instruction excess. A later bounded source improvement could audit list-child
canonical duplication through the existing checked canonical-home producer
mechanism; it must preserve temporary publication before receiver retirement,
folded-local reentry behavior and all sink semantics. That would be a source
proof experiment, not an established repair for this higher-CPI observation.
