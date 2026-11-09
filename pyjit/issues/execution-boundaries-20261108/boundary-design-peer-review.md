# Independent review of bounded boolean edge design

2026-11-08. Reviewed `boundary-design.md` against frozen `0765e9be47f`
bytecodes, generator, specialization and instrumentation. No runtime edits.

The unchanged logical coordinates and static edge mask give a bounded path
that removes actual NOP decode/PC work. Retaining canonical dormant anchors is
an explicit first-stage limit, not physical compaction. Keeping return/discard
work separate is correct: a result release can execute Python; inline bool
retirement cannot.

Two findings require design changes before implementation admission:

1. The claim that the next real handler saves its own PC is false for
   `END_FOR` and `INSTRUMENTED_END_FOR`, declared `no_save_ip`. END_FOR releases
   an owner and its finalizer observes the previous PC. `IR_JUMP` also leaves
   the PC unchanged and can lead to such a handler. A skipped NOP previously
   became that previous PC. Decline edges whose next handler cannot prove a
   PC save before observation. A bounded exclusion of all three no-save
   handlers, checked against both pristine and live successor identity, is
   adequate; an explicit generated capability is more extensible. Include a
   finalizing END_FOR boundary case or a generator/selector rejection check.
2. lltrace is frame-local while the variant is code-shared. Declining selection
   when the current frame traces does not protect a later invocation or resume
   of the same code with lltrace >= 5. `LLTRACE_RESUME_FRAME` refreshes this
   state. For this bounded experiment, disable variant selection for all debug
   builds, or provide and test an invalidation protocol for every entry/resume
   route. A mutable runtime lltrace check would consume part of the saving and
   should be measured if selected.

Other implementation requirements:

- Normalize physical variant identities wherever feedback consumers expect
  canonical bool opcodes, preserving existing fused identities. Generated
  full-domain metadata alone does not make fixed opcode-array indexing safe.
- Instrumentation invalidation must precede target patching and any callback
  that can execute the changed image; place it after successful exec creation
  and before instrumentation mutations. A failure after invalidation can safely
  leave the generic handler.
- Revalidate live successor handlers during selection; an instrumented or
  transparent successor must decline. A branch wrapper that later executes a
  pristine generic word must not recover a stale skip variant.
- Validate that the consumed inline bool is definitely not an owned pointer,
  and retain the guard-before-consumption and no-callback terminal shape checks.

Disposition: changes requested for the two blockers. After they are resolved,
implementation can proceed under the coordinator's admission, with release
compile and observer tests still pending. This review does not establish runtime
correctness or speedup.
