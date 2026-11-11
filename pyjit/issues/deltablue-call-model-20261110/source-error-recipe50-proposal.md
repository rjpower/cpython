# Explicit recovery for failed source events

Implemented in selected runtime `bf810ec1a33` after proposal and actual-source
review. Release/debug focused checks pass; the [report](report50.md) owns final
qualification and measured tradeoffs. The following text records the design.

The source observer already decodes the event and its exception region before
calling Python. The unwinder currently decodes that event again through
`co_linetable`. Copy its recovery recipe when recording the failure instead.

Move the existing four-int `_PyExceptionHandler` definition from `Python/ceval.h`
to `Include/internal/pycore_ceval.h`, which both evaluator and instrumentation
already include. Keep its existing fields and ordinary exceptiontable ABI.
Extend `_PyEval_RecordSourceError` to receive this recipe by value. Store it in
the existing evaluation activation beside the frame validity pointer. Remove
the activation's now-unused event ordinal and its recording API argument.
Existing frame-object source cursors and display ordinals retain attribution
separately; they do not select execution recovery.
`source_line_event` constructs the recipe from its already decoded event, only
on the existing nonredirected error path, including frame-allocation failure.

`get_frame_exception_handler` consumes the stored recipe only for the matching
frame in the current activation, copies it out and clears validity and recipe
before display cleanup and return.
Its existing display-cursor reset remains. The ordinary exceptiontable route is
unchanged. No allocation, reference ownership, callback or map access is needed
to record or consume the recipe. Nested C evaluation keeps its separate local
activation, so cleanup reentry cannot overwrite or consume its parent's recipe.
Existing redirect invalidation scans the activation chain and clears matching
frame validity and recipe. Initialization clears both.

Files: `Include/internal/pycore_ceval.h`, `Python/ceval.h`, `Python/ceval.c`,
`Python/instrumentation.c`, and `Lib/test/test_ir_source_reentry.py`. No compiler,
serialized format, frame ABI, generated handler or mode changes.

Validation: retain existing same-PC redirect/error, fused eliminated-span handler
and nested cleanup/reentry tests. Add focused monitoring cases for caught versus
uncaught source-event failure and repeated disable/re-enable, checking cleanup
and traceback attribution separately. Review allocation-failure and redirect
branches directly. Root owns release/debug builds and runtime tests; this lane
will run only cheap host syntax/source checks after root closes quiet timing.
No DeltaBlue speedup is expected. Acceptance requires no source-map access in
the unwinder, preserved once-only recovery and unchanged ordinary lookup.

Peer approved this boundary before implementation in
`C50-performance-debug-review.md`, pending the coordinator's source unfreeze.
Root and peer subsequently approved removing the write-only activation ordinal;
the existing frame-object attribution state remains unchanged.
