# Option 5 proposal peer review

Approved for isolated implementation. Reviewed the proposal against bf810
`_RETURN_VALUE`, `clear_gen_frame`, `_PyFrame_ClearExceptCode`, and
`clear_slots`. The normal return path already detaches its result and proves
scratch empty; forwarding that explicit capability can avoid the generator's
full scratch scan without changing its ABI or owner state.

Conditions:

- Only the real `_RETURN_VALUE` boundary grants completion. Generic unwind,
  entry failures and suspended clear/deallocation remain on the full non-thread
  path. A close/throw executing RETURN_VALUE is normal completion; a generator
  ownership/state check alone does not grant it.
- Assert scratch empty at the named completed boundary before any retirement;
  retain the existing generator state, exception-link, critical-section and
  generator_return_kind ordering. Do not move callback-capable work after the
  final generator_return_kind assignment.
- Keep one shared frame clear body. Frame-object takeover stays before the
  optimized clear and copies all homes. Preserve reverse NULL-before-close
  local retirement, f_locals/f_funcobj retirement and the distinct code owner.
  The returned canonical owner stays held throughout callbacks.
- Update the generator's exact return-body proof and clearing-call knowledge
  to the new explicit entry without broadening the accepted producer chain.
  Instrumented return still reaches the same proven boundary after its event.
- Tests must include exceptional/suspended scratch owners as negative cases,
  plus normal aliased return, closure/mortal local, escaped-frame and finalizer
  reentry cases. Release native must actually use co_nlocalsplus on normal
  completion and retain the full bound on generic clear. A source boolean
  alone is not evidence of saved work or application benefit.

No ownership or recovery blocker found. Actual release compilation, independent
post-review and coordinator measurements remain required before admission.
