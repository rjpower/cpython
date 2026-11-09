# Private debugger entry: source review

Reviewed the actual source25 diff after Clinic conversion, before native
qualification. No source blocker identified in this bounded interface.

`sys._debugger_entry` rejects the current executing native-CALL frame and inactive
or nonancestor targets before publishing any observer phase. A strict active
ancestor retains its pending child contract. The callback receives the private
entry phase under tracing suppression; observed frame, event and tracing state
are restored on both success and error. The marker is outside public monitoring
events and is never used as a tool index.

Bdb enters synchronously at the call site. Its separate entry dispatch uses the
existing interaction hook without inventing INSTRUCTION or LINE events.
`set_enterframe` restores nested state through `finally`; callback failure and
quit stop the newly enabled tracing before propagation. The setter change admits
only the private observer phase: it does not bypass pending-child, recipient,
owner or protected-context checks. The existing real-LINE identity gate remains
unchanged.

The new tests cover both backends, continue/step/next, failure and quit cleanup,
ancestry rejection, ignored-call redirect, used-result refusal and preservation
of target/callback handled exceptions. Native behavior remains coordinator-owned.

Context implementation remains separately scoped to inherited and same-caught
moves. This interface does not authorize edits to an active child's overlap or
to a target's handled-exception link.
