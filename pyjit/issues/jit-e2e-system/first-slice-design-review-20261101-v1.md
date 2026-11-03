# First-slice independent design disposition

**ACCEPT for a separately authorized bounded source attempt.** Exact proposal:
`7c598fd92dd6ebdf2515a5bd3f6601b0bf0238d430463d905175707e3858d629`.
The proposed60/90-minute stages are root decisions, not authority from this
review. No native/compiler/timing or implementation claim follows.

The cut is useful: complete `work` and `bridge` CFGs contain a varying loop,
branch, Python call, real `abs` C bridge and optional Python `__abs__` reentry,
with return/raise and actual compiled continuation. It reuses the existing
evaluator as trampoline, generated semantic bodies and one owner/backend.
Unsupported adapters exit before the operation for the activation remainder;
they do not bounce through one interpreted instruction at a time. Automatic
tiering is correctly separated from a cold compile/install bring-up fixture.

I checked the proposed phases against actual `_PUSH_FRAME`, `start_frame`,
`_RETURN_VALUE` and IREmitter return-store source. PREPARED consumes existing
start-frame effects; STARTED does not repeat frame publication, recursion or
RESUME skipping. RETURN is consumed after output flush/IR_STORE_RETURN; the
caller CodeUnit is freshly resolved after reentry. Error-log-pending and
unwind-pending remain distinct, including the logical raising word. These are
the right contracts; their generated implementation remains unproved.

The main source-stage stop is an unclassified raw goto/frame change or an
unbalanced owner/error path. The plan makes that explicit and freezes partial
work at its cap. Keep specialization scaffolding distinct from execution of
the Python operation, flush homes before terminal actions, validate generations
after helpers, and retain active code during C callback reentry. Source review
must inspect actual generated bodies and control flow; a coverage count or
Action-shaped mock cannot establish them.

The sealed validation contract supplies four program families and eight
source-checked inherited regression methods. Apply boundary cases to the actual
cut; generator suspension may remain exact fallback. Actual native entry and
compiled-after-call continuation need executed-path evidence, separately from
registration or inspection. First diagnose cold/first/warm costs on calls,
loops and mixed objects. No old54-scalar roster or per-component all29 veto is
required. No critical design blocker remains.
