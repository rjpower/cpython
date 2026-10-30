# Main-only GC union: integration source review

**Disposition: SOURCE_CLEAR for bounded actual-union validation.**

Reviewed clean candidate `3c6b8daa5065406138879e928c7a6a7ef1fe3aa2`
in `boil-gc-simple`, relative to primary `ff6ed9d6bfb`. This is a source
composition review, not native qualification or a claim that all GC contracts
are closed. No build, dry-run, compiler, target interpreter or native probe was
executed for this review; the candidate was not modified.

The reviewer authored the PickleBuffer/scanner and ndarray changes. Their
independent approval is explicitly excluded here: their component source
clearances come from the coordinator and the separate consumer/ndarray source
reviews. This review checks their interfaces with independently reviewed code
and the assembled collector phases, without substituting for those reviews.

## Findings

No additional supported-runtime defect requiring a source fix was identified
in the reviewed composition. The supported boundary is one main interpreter,
shared GIL and ordinary threads. The following facts support that conclusion.

| Boundary | Source finding |
| --- | --- |
| Interpreter admission | `Python/pystate.c:_PyInterpreterState_New` rejects an existing main interpreter under HEAD lock before allocation/publication and leaves the output NULL. `Python/pylifecycle.c:new_interpreter` rejects before changing attachment or GIL-state bookkeeping. Neither a detached caller nor a NULL supplied thread state bypasses the locked membership check. `sys.implementation.supports_isolated_interpreters` is false. |
| One drainer | `Python/gc_live.c:481` claims `queue_draining` under the GIL before delivery. The flag remains claimed while a callback releases the GIL. Nested or competing collectors may collect/enqueue but return without waiting for that callback; they cannot become a second drainer. This deliberately removes parallel callback delivery, not ordinary thread progress. |
| Delivery roots | Before unlinking/freeing a queue node, `run_queue_ex` publishes its object and callback in the existing value span. The same span roots the interrupted raised exception. Payloads remain rooted through callback, finalizer and unraisable reporting; the saved exception is restored into the thread state before the span is popped. No allocating handoff follows span removal. |
| Busy finalization | `Python/pylifecycle.c:2397` rejects an already active drainer before teardown. The second check in `make_pre_finalization_calls` is under pending/HEAD locks after preparation, before the finalization-guard latch. Rejection releases those locks and restores entry attachment/finalizing state on the supported main-thread path. Preparation may already have run exit callbacks; this is not transactional rollback of their effects. Unsupported non-main/foreign rollback paths fail explicitly. Void `Py_Finalize` does not pretend a rejected live runtime was finalized, and `Py_RunMain` avoids global cleanup after such rejection. |
| Fork and Popen | `Modules/posixmodule.c:719` checks the drainer after Python before-fork hooks and import-lock acquisition; rejection unwinds the import/world preparation. HEAD acquisition then cannot detach. In this GIL build the world-stop helpers are empty (`Python/pystate.c:2221`), so the GIL and non-detaching HEAD acquisition are the actual exclusion. The warnings hook records lock state rather than acquiring a Python-running lock. `_posixsubprocess` sets `need_after_fork` only after successful preparation, so rejection does not run an unmatched after-fork path. Immediate exec without a Python preexec callback remains a distinct supported route. |
| Unpickler owner | `Modules/_pickle.c:1293` detaches the owner and raw cache together. New input acquisition is prepared before old input is consumed, and replacement publishes coherent new state before a release callback. `running` rejects reentrant load/init and memo mutation. Automatic clear detaches and drops the owner rather than releasing its acquisition during sweep; traversal visits that owner. The sealed owner has no Python release entry point, so a subclass finalizer cannot replace its cleanup or release an acquisition behind an active cached reader. |
| Same-owner XID | `Modules/_interpretersmodule.c:99` checks interpreter identity before accessing shared storage and constructs an ordinary view of the original exact memoryview. Existing XID root/disposal keeps that source registration alive through construction; receiving success and failure both dispose the temporary XID acquisition. There is no receiving wrapper finalizer that could invalidate a resurrected view. Foreign restoration rejects explicitly. The prior independent XID review, including the corrected pre-pop fault controls, remains applicable. |
| Safeguard removal | `Objects/memoryobject.c:1212` now requires zero outward exports rather than overwriting the count. `Objects/abstract.c:823` checks DEAD-and-SWEPT before dereferencing an exporter instead of silently dropping release. Checking the conjunction matters because SWEPT aliases the live mark bit. The removals rely on the reviewed owner ports and the collector's retained closure, not a blanket assertion that native release callbacks are leaves. |

## Collector/owner composition

`queue_destroy_hooks` first selects all ready memory registrations, including
native registrations without release callbacks, before retaining any unready
closure. This avoids retaining a ready native registration solely because its
managed buffer is not yet ready. Still-dead unready memoryviews and managed
buffers are resurrected with their reachable closure; skipping a hook never
permits its storage or exporter to be swept. Work-stack failure propagates to
the existing rollback before sweep. Queued objects also retain their closures
under the existing queue-root rules.

Delivery rechecks acquisition/readiness. A skipped release is not notified or
counted as completed. Actual consumption is counted only after error reporting
and marks notification accordingly. This composes with the existing one extra
explicit collection pass; it does not establish a universal resource-chain
depth bound or promise that one `gc.collect()` exhausts every native lease.

The Unpickler's original master remains stable until its guarded release.
Automatic consumer clear only drops that owner. The separately reviewed ND
port retains original acquisition metadata in its owner, keeps transformed
arrays separate, mirrors real outward exports and gates closed-owner reads.
Those contracts are the relevant interfaces for removing the two safeguards.
This review does not independently approve the ND implementation it authored.

## Limits and actual validation state

- Candidate `3c6b8daa506` is not yet natively qualified by this note. The current
  O1 assertion/mode-checked focused and affected jobs belong to the native
  operator and must be assessed on their actual terminal results.
- Candidate925d's focused17 and whole9 passes are component evidence. The dc7b
  consumer evidence combines eleven original focused passes, the corrected
  scanner case, six original whole-module passes and fresh whole `test_re`.
  Preserve the original scanner/Re acquisition-accounting failures and their
  test-only bridges; none becomes a uniform old all-pass receipt.
- Real ND acquisition/export cases and same-owner queue/channel transfer are
  distinct from the synthetic exact-memoryview export blocker. That blocker
  proves retention/OOM mechanics only; count1 does not prove provenance.
- Inherited live Python-buffer-wrapper re-export failures for PickleBuffer/ND
  and producer push/pop conversion reentry in the private ndarray test helper
  remain explicit limits. No new claim for these paths is made here.
- Free-threading, foreign interpreters, arbitrary external unported persistent
  buffer holders and Windows acquisition families are outside this milestone.
  Current hybrid C roots and transient-view contracts remain necessary.
- Busy embedding finalization is rejected, not equated with process exit.
  This cut does not establish repeated initialize/finalize reclamation,
  generalized fork recovery, cross-owner callback progress or resource-DAG
  scheduling. None is needed to justify this bounded source clearance.

Next step: finish the actual-union invariant/affected validation, preserve any
failure without relabeling component receipts, then qualify the resulting
coherent source under the coordinator's existing integration plan.

## Four copied causal omissions: source follow-up

Reviewed `boil-gc-simple/build/gc-simple-final-validation/negative4-proposal.diff`
SHA256 `27f71f8f3493b7719bc5cd0555164e1d35a2a14c900b602a700f3e91d9212037`
and its four-selector JSON against the unchanged candidate above. All four
declared before-file hashes match. No copied branch, build or target execution
is credited here. The proposal changes four runtime expressions/statements;
neither test file changes.

**Independent causal source clearance: drainer and Unpickler.** Scanner and ND
rows below explain the author's own controls and require the coordinator's or
another reviewer's independent approval.

| Omission and exact target | Attributable failure and safe observation |
| --- | --- |
| `gc_live.c`: replace the interpreter-wide drainer admission test with the old thread-local `running_queue`; `SingleDrainerTests.test_worker_collect_returns_without_overtaking` | The waiting outer callback releases the GIL. The worker's thread-local flag is clear, so its collection delivers an inner callback before returning. The outer then records `inner callback overtook outer callback`, and the external `assert not errors` fails. Existing event waits/join remain bounded; a timeout is not the intended causal result. Crucially the negative retains the new active-call value span and interrupted-exception root, so it tests overtaking rather than loss of the executing outer payload. |
| `_pickle.c`: consume old input before preparing its replacement; `CUnpicklerBufferOwnerTests.test_failed_replacement_keeps_old_acquisition` | First input `b'I'` is consumed before the second acquisition callback collects and raises `ValueError('acquisition failed')`. The expected ValueError is caught; `self.assertNotIn(('release', 'first'), events)` then detects the premature release. **There is no literal `premature old release` assertion message.** The negative detaches/zeros the old raw cache, and the test checks release events without reading freed input. The callback-backed byte strings and stream remain live; no invalid-memory access is needed. |
| `_sre/sre.c`: omit pre-release `state.start = NULL`; `BufferLifetimeTests.test_scanner_cycle_release` | During the original acquisition's release callback, the scanner can still return a match instead of the required three None results. Recursive `__del__` is still suppressed by `releasing`. The external assertion names `release must observe an exhausted scanner`. Backing is a child-module global bytearray, and original acquisition release has not returned, so these scanner reads do not manufacture stale storage. This is explanation, not independent approval of the reviewer's scanner code. |
| `_testbuffer.c`: omit only `ndarray_tobytes`' active-owner gate; `BufferLifetimeTests.test_ndarray_cycle_release_and_resurrection` | The owner is marked released before the exporter callback, while the ND and its cached metadata remain reachable. The callback's readers record successful `tobytes` at its fixed index, making the external assertion `released ndarray remained readable` fail. Module-global backing is still allocated and the ND still owns its transformed arrays. The omission is not applied to an uninitialized object. This is explanation, not independent approval of the reviewer's ND code. |

The four targets use disjoint mechanisms: the drainer case has ordinary Python
finalizers and a worker, the scanner case has neither Unpickler nor ND, the
Unpickler case has neither scanner nor ND/concurrent collector, and the ND case
has neither scanner nor Unpickler/concurrent collector. Running each selector
in a fresh process permits one copied four-omission build without carrying
mutated queue/error/acquisition state between cases. No new runner is needed.

The eventual copied source must bind the exact four omissions and unchanged
tests. Current test-file SHA256 values are
`test_gc.py=0581047de11ebfcdaaf20a80f31fdce9a7b4673a618d1be09a6b7bb7ab4b5c35`
and
`test_pickle.py=3d404ce48e075d6663b9cc63daa4baa98e0abac92579e4e5ddd2a84788467ae5`.
Require each exact selector's structured unittest failure and intended named
assertion/event, with native failure status, rather than treating any nonzero
exit as success. Crash, timeout, import/build/guard failure or a different
assertion establishes no causal credit. Preserve the positive products and
all original receipts; this source review neither authorizes nor qualifies a
native negative run.
