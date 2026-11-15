# Cross-word scalar: reject the descriptor framework; retain a fixed rival

Coordinator disposition after source review: **defer scalar implementation**. The initial
allocated-plan proposal is rejected on design cost, before coding. Its per-code
records, reverse index, generation, mutation hooks and proof reconstruction are
disproportionate to a two-word trial. Proceed with the common-constant dialect
packet first. This note preserves a cheaper, concrete rival for later admission;
it does not authorize it or claim a native saving.

Base any future scalar trial on selected57 `0dcba75bb3df`, not the rejected
computed-goto engine. No runtime source, generation, build or probe accompanies
this proposal. The [round60 design](../../design/proposals/interpreter-parity60.md)
and [review ledger](review-next.md) retain the larger context.

## Actual consumers

E59's monitored spectral census contains ADD_INT → MULTIPLY_INT at `eval_A`
words 5–6, 676,000 visits each: ADD A4/B4/C5, then MULTIPLY A3/B3/C4. The
intermediate is the consuming right operand; all three external homes are
scratch and the final destination aliases the surviving second operand.

The next admission would be FLOAT multiply → FLOAT add at words 15–16 in
`part_A_times_u` and `part_At_times_u`, 338,000 visits each. In the retained
first function these are MULTIPLY A9/B9/C5, then ADD A3/B8/C9: C5 is a folded
local input and A3 is a local recipient. Thus float is not the same all-scratch
commit and must be separately proved. Names identify evidence, never admission.
These are structural counts, not successful-region counts or parity budgets.

## Smallest fixed execution variant

Use one learned-only two-word INT variant first; add a FLOAT instance only if
its additional commit/rounding proof is economical. Preserve immutable original
words, operands, PCs and semantic metadata. The learned head has the original
first word's format. The second word remains an executable ordinary word.
Public images cannot directly name the learned variant; generate that boundary
from its source-declared executing-only requirement, as for existing learned
capabilities. Unsupported images remain valid ordinary images.

No allocated descriptors, reverse index, new frame/exec fields or new feedback
payload are needed. Native entry uses the current first word and reads the next
word once. It checks that the latter is the exact expected specialized handler,
then decodes its other operand/final destination. This **retains** a second-word
load, handler extraction/comparison and operand decode; only its dispatch and
the intermediate transport can disappear. Charge those checks explicitly.

Installation is an optional pure check after ordinary binary specialization,
when both adjacent handlers are available. It checks the immutable operand
relationship and observation state before changing only the head handler. On
an entry mismatch, restore/execute the ordinary first handler at the same PC.
No descriptor survives a callback. The expected-second-opcode guard catches
instrumentation/deoptimization of site 2; instrumenting site 1 uses its ordinary
first-family identity. Decline the variant while tracing/profiling/relevant
monitoring or pending instrumentation refresh is active. Original wrapper
saving/removal must restore an ordinary head, never invent an interior state.
Audit these few existing writer paths; a need for a new global invalidation
framework is a reason to defer, not add that framework.

The initial INT geometry is deliberately finite: bound right input equals the
first scratch destination; first destination aliases a consumed first scratch
input; the other second scratch input is distinct and survives site 1; final
destination aliases that other scratch input. Reject other alias/recipient
modes initially. This reaches the retained INT consumer without storing a
per-site role mask or querying dynamic local partitions in the hot variant.

## Proof is real work, not an existing API

There is no existing common cross-word last-use certificate to call. Compiler
regcfg liveness uses pre-encoding value IDs, unavailable to constructed images.
`codeobject.c`'s private `owner_handler_safe` worklist is not that certificate.
A new bounded two-site check must validate actual ordinary read/kill roles,
physical aliases, and no jump/EH/recovery entry into site 2, using admitted
immutable words and source-derived contracts. The intermediate must be killed
by the second consuming input and not retained by an exceptional edge. Initial
admission can reject protected/recovery-bearing pairs. Compiler IDs may
nominate, but cannot bypass this common check. The check must have bounded
storage and no allocation on the specialization path; if it needs a persistent
whole-code analysis product, defer this small trial.

Likewise `ScalarStorageGraph` is currently producer-local. Reuse its actual
SAVE/VACATE/PUBLISH receipts and current Local identities: substitute each
site's fields with resolved physical homes, bind the producer result token to
one consumer input, then coalesce aliases before deriving commits. No
concatenation of opaque C strings, fabricated INIT/owner facts or opcode-name
body permission. A small source-owned scalar kernel primitive, consumed by
both ordinary and two-word lowering, would preserve expression semantics and
produce reached arithmetic receipts. Changed/missing receipts deny the variant.
If that requires a general region interpreter or broad parser work, stop.

## Exact exit contracts

Guard the first external INT inputs as inline ints, compute the existing scaled
addition, and use the scaled sum directly in the multiply's right operand.
Guard the surviving second operand; account for scaled-left × unscaled-right
without constructing/reloading/retagging a PyValue intermediate.

First guard failure or add overflow leaves homes untouched and executes the
ordinary first word. After successful addition, second guard failure or multiply
overflow commits the **whole first poststate** before entering the ordinary
second word: input scratch vacates, inline first result in its original home,
unrelated homes unchanged, correct second PC. Never repeat site 1. On full
success, clear consumed scratch homes, omit the dead intermediate publication,
publish the final inline result and dispatch after site 2. In the retained INT
example home 3 owns the product and homes 4/5 are NULL. Removing a publication
never permits omitting those final vacates. Preserve both semantic hit counts.

The first destination's alias is positively consumed and inline. An unrelated
scratch destination would need a positive physical-NULL/no-owner proof;
NO_HOME alone cannot supply it. No invisible heap owner, allocation, poll or
callback exists inside this GIL-only fast interval. Slow exits use the ordinary
site's existing error, owner and observer protocol after materialization.

FLOAT admission preserves `PyValue_FloatOperandValues`: two floats or one float
and one inline int; never silently accept two ints or boxed NaNs. Preserve two
separately rounded operations, signed zero and infinity; root must verify an
explicit local contraction barrier and separate native multiply/add with the
unchanged flags. On first NaN, exit to ordinary site 1 before boxing. On second
NaN/guard failure, commit the non-NaN first result and its input vacates, then
run ordinary site 2. The local final recipient needs the existing exact late
retirement path, with all homes materialized before its old owner can finalize.
No plan/projection is read after that callback. This additional proof is why
FLOAT is a second admission rather than a promised first-packet extension.

## Source footprint, tests and falsifier

Potential source closure is binary DSL/kernel lowering in `bytecodes.c` and
`ir_generator.py`, its existing helper/graph contracts, a small common two-site
admission query in `ir.c`, and binary specialization installation. Executing-only
variant admission, instrumentation mapping and generator/source fixtures must
close with it. No new container format, code-exec allocation or frame field.
Do not implement this until the shared-kernel/commit consumer is independently
peered and this actual footprint fits the expected native budget.

Minimum controls: missing/changed kernel receipts; first/second guard and
overflow exits; huge ints, bool/subclass/custom numeric fallback; exact prefix
vacancies; rejected binding sides/aliases, local/discard/unrelated destinations,
interior entry and EH retention; all public image routes/forged learned heads;
LINE/INSTRUCTION/trace enable/disable and site-1/site-2 handler changes. FLOAT
adds mixed operands, NaN allocation failure, separately rounded results and
local-old-owner finalizer/GC/local mutation.

Native acceptance requires omitted intermediate home store/load/retag and no
second dispatch, after charging next-word guard/decode, pointer/operand loads,
range checks, vacates, final publication, spills and code size. Compare selected57
with and without the variant on all eight screens; only a coherent winner
advances to all29/full validation. If the fixed shape cannot share actual
source receipts cheaply, or added guards absorb its transport saving, close
scalar rather than escalate to descriptors. The common-constant packet takes
priority now; broader role-dialect work remains a separate reviewed rival.
