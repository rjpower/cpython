# Heap lifecycle census: frames peer

2026-11-12 source GO for the bounded first consolidation in
[lifecycle-readers.md](lifecycle-readers.md). `frame_tp_clear` and existing
`_PyFrame_ClearLocals` both reverse the full initialized home domain, remove
each home before its close, then close the mapping. Keep observer metadata
closes before that call and retain code/function ownership. The helper's extra
debug well-formedness check is valid under current NULL authority; it must be
ported to selected readable homes before stale scratch is admitted.

Preserve one-pass behavior when a callback writes into a home the cursor has
already passed. Such a value remains a heap-frame owner; a shrinking retirement
prefix alone cannot represent its binding. Full initialized-control marking is
safe now, while later selected-home maps must distinguish mutable bindings from
scratch progress. Do not double-subtract an externally marked heap edge.

The proposed collector entry retains backend-specific counted/deferred policy;
sharing geometry does not justify replacing it with an object-only visitor.
The copy, debug, uniqueness, observer and generator readers listed in the census
are real map-switch gates, beyond the first retirement shadow certificate.

`frame_dealloc` needs its own fixed-last code and callback-lifetime proof. Its
refcount-zero/storage state and metadata order differ from thread/gen retirement
and live escaped `tp_clear`; do not port it by calling that API blindly. The
proposed callback mutation, retained pins, nested collection and later-clear
fixtures are appropriate. Root owns candidate execution and qualification.

One census spelling correction is needed: the old
`_PyFrame_ClearExceptCodeSlow` name is now `_PyFrame_FinishClearExceptCode`.
The existing `clear_slots` comment's claim that passed homes are NULL also needs
to allow callback-time writes when that lifecycle code is next changed.

The bounded two-file implementation in GC's lane is source GO: `frame_tp_clear`
now calls `_PyFrame_ClearLocals`; no deallocation or pin order changes. The new
`test_clear_preserves_callback_local_writes` asserts actual named-home order,
checks code visibility during the finalizer, nested clear/collection, retirement
of the new lower binding and retention of the new passed binding until a second
clear. The trigger home is NULL before optional nested clear, so the fixture
has no recursive trigger close. Native execution remains root-owned.
