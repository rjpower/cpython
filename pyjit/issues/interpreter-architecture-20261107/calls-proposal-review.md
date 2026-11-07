# Independent call proposal review

2026-11-07. **Accept the three stages as separate implementation experiments,**
subject to the checks below. Source baseline: runtime `7780e32e0e2`. This is a
proposal review, not a diff or performance admission. I made no runtime edit,
build or measurement.

Stage 1 is a sound cleanup. `_PyCallTransaction.effects` has assignments in
`Python/ceval.c` but no reader. The handler's `CALL_EFFECTS` is live: generated
fallback guards use it after preparation or monitoring, so the proposal rightly
leaves that protocol intact. Remove or rewrite the nearby comment saying the
record helpers “record their effects”; after the field deletion that would be
false. A source search and release compile are sufficient to catch residue.

Stage 2 keeps the ownership boundary in the right place. The overlap helper
may decline or fail while the caller still owns its window. The consuming
provider then duplicates inputs and is responsible for its own partial-failure
cleanup; moving that body into a private `Py_NO_INLINE` helper changes no
transfer rule. The wrapper must return `-1` and `0` without invoking the helper,
and call it exactly once only on `1`. Preserve the provider's `done` cleanup,
child publication before retired-owner releases, refresh on decline/error, and
the caller's `*child = NULL` initialization. Review actual generated GCC: source
outlining alone does not prove a smaller common-path frame or faster calls.

Stage 3 has a valid narrow representation proof. `_PyValue_STATIC_IMMORTAL`
encodes the pointer plus `Py_TAG_REFCNT`; the empty tuple is a statically
initialized immortal of exact tuple type. `PyValue_FromObjectBorrow` followed
by `PyValue_DUP` produces the same tagged bits without a count, and
`_PyValue_Publish` cannot turn an exact tuple into an inline number. Use the
initializer only for this singleton and branch. Check the release/debug result
and both GIL-mode definitions before accepting the implementation. This is a
separate cost experiment, not a new general “known immortal” API.

The retained logging annotation shows a whole common-call route: overlap
guards, frame initialization, NULL tail, default filling, and empty varargs.
Its samples justify these bounded experiments but cannot predict the effect
of the 248-byte prologue or any one check. Accept a speed claim only after a
matched same-workload screen and changed assembly are reviewed.
