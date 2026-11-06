# Shared-module stats receipt exports

The stats build's `_posixsubprocess` shared module could not import because `_PyStats_CopyIntervalStem` and `_PyStats_ChildCreated` were plain internal declarations. [The additive patch](process-identity-stage3-export-fix.patch) changes only those declarations to `PyAPI_FUNC(void)`, following the existing internal API export convention. It applies cleanly on the coordinator's current stage-three source. The coordinator will relink and verify module imports; this source check does not claim a linked build.

The expanded 12-case [process identity probe](process-identity-probe.py) checks normal, zero and repeated intervals, fork and spawn child receipts, a failed spawn, a markerless external image, stats-off fork, late stats activation, parent callback dumping, and exact argv boundaries. It requires actual root and child PIDs and keeps unresolved evidence red. The coordinator will execute it against the rebuilt stats interpreter.
