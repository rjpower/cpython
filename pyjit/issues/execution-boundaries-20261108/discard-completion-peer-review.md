# Independent discard-completion proposal review

2026-11-08. Initial source pass against golden `0765e9be47f`; revised general
route proposal pending. No implementation or native validation by reviewer.

The constraint classification is correct. Python permits delayed collection;
frame/region ownership and overwrite retirement remain valid alternatives.
Direct completion disposal is a reasonable bounded RC experiment because it
can avoid an intermediate result home, but its exact release point is a chosen
compatibility policy rather than a language requirement. Supported observer
state and correct counted/borrowed owners remain separate requirements.

The initial exact-only proposal has an admission gap: retained logging outer
calls use guarded CALL_PY_GENERAL. Do not credit its nine POPs to an exact-only
variant. Root requested the actual general child route before admission.

Source requirements sent to the proposer:

1. `call_transaction_run_guarded_python_vector()` can fail to push a child and
   instead complete a native fallback. Tag only successful Python-child entry;
   preserve ordinary native completion and input-retirement behavior. A
   failed eligibility decision after preparation effects must keep the ordinary
   recipient, never DEOPT/replay the original call.
2. General-call generation does not execute its DSL `_SAVE_RETURN_SLOT` suffix:
   `IREmitter.call_completion()` dispatches the returned child and marks the
   handler finished. Identify the real tag publication point, either inside
   the existing non-reentrant helper commit or its generated child!=NULL arm.
   Keep function/code/provider/resource revalidation after kwargs preparation.
3. The full raw return-slot consumer census includes
   `Objects/genobject.c:_PyGen_YieldFromRef`, remote asyncio inspection in
   `Modules/_remote_debugging/asyncio.c`, and suspended f_lineno writes in
   `Objects/frameobject.c`, in addition to ordinary return publication, frame
   copies, initialization and trampolines. Excluding generator-owned callers
   is an adequate first cut; otherwise these readers need a concrete masked
   access/proof. A tag is not a safe slot index merely because its low bits
   retain one.
4. Completion must decide the observer route after child cleanup. Caller
   destination and logical POP identity must be preserved separately; do not
   assume an arbitrary mutable instr_ptr still denotes the original call.
   Specify which f_lineno mutations are supported/declined and test the actual
   reader used to recover the POP. A materialized observer result must exist
   before callbacks; its owner must be retired exactly once on callback error.

The return-time live-boundary check is a real cost and is not automatically
wrong: it can replace dispatch/slot traffic while permitting dynamic observer
activation. Credit the net compiled work only after comparing it with baseline.
Direct disposal must preserve canonical value publication rules; the result
already owns its count across child cleanup in the existing return path.

## Revised general-route and coordinate proposal

The revision now identifies the actual nine general-call/POP pairs and zero
ignored exact-route target calls. It keeps the original numeric destination
through transaction preparation/native fallback and installs the tag only at
successful Python-child commit. Excluding generator-owned callers and callees
closes the raw yield/remote-await consumer concern for this bounded form.
Post-preparation decline retains an ordinary recipient; it never replays a call.

Retaining CALL PC during untraced result disposal is a defensible explicit
implementation choice. The caller is already safely observable at CALL with
consumed arguments and a C-owned result during existing child cleanup. A
finalizer may inspect that frame; no Python language rule requires a dormant
POP PC. Source-line equality must be checked during eligibility, and the
supported instrumentation path must materialize the ordinary result before its
callbacks. Any intended raw f_lasti difference must remain in the report.

The revised minimal coordinate protocol clears a pending tag on a successful
thread-owned f_lineno assignment before changing instr_ptr. Source inspection
confirms the setter can mutate a saved parent from a child LINE callback:
its event check is thread-wide and does not require the target to be the
currently tracing frame. Thus this is a real case, not a hypothetical ban.
Clearing the tag makes existing LOAD_IP(1)/ordinary result publication honor
the new coordinate. Failed setter attempts leave state unchanged. No extra
saved-PC field is necessary if the supported cross-frame mutator census stays
limited to this setter, as inspected.

Additional return-time hardening: check the live POP's actual input field equals
the preserved temporary destination before direct disposal. Any declined
shape must publish normally and retain the existing next_instr, without
redirecting to a guessed original POP. Instrumentation changes handler fields
without relocating the image; ordinary execution only updates its current
frame, so it does not invalidate the saved caller coordinate proof.

Disposition: the revised proposal is suitable for bounded implementation after
coordinator admission of the explicit CALL-PC policy, same-line eligibility,
successful-f_lineno tag invalidation, and live-input validation. Actual code,
release/debug tests (including parent mutation from a child trace event), GCC
net-cost inspection and measured target coverage remain outstanding.
