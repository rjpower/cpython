# Independent interpreter parity design review

2026-11-08. Runtime source base `0953757f4e7`. Review of `v8-review.md`,
`call-audit.md`, `implementation-proposal.md`, warmed-IR evidence and actual
runtime source. Read project instructions and the CPython AI policy. No
native build, workload, test, benchmark, event capture or commit was run by
this reviewer. Runtime changes remain owned by the implementation worker.

**Verdict:** accept the eight-recommendation comparison and the concrete
unbound call proposal for bounded implementation design. Accept P1 and the
corrected P2 proposal, subject to actual code and coordinator-run validation.
The initial P2 helper contract was rejected and corrected before approval.
No performance result or parity outcome follows from these approvals.

## Facts versus layout

The source supports the central distinction. The current 64-bit word spends
its full bit budget, cannot encode local/temporary class separately, and
requires evaluated owning temporary call windows. Those are present IR
constraints. Register compaction, emitter reloads, provider discovery,
duplicate validation and generic scalar canonicalization at known-code sites
are implementation choices. Changing the latter does not require a new
encoding, accumulator, call ABI or ownership model.

Observable Python locals and reference-count retirement impose semantic
constraints beyond the chosen encoding. Fresh `**kwargs`, destructor timing,
monitoring and hook behavior cannot be removed on the strength of a V8
comparison. The existing overlap route already reuses argument homes and
known-NULL slots. The review correctly avoids recommending that work again.

I independently checked the pinned V8 [call
handlers](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/interpreter-generator.cc#L1446),
[register-equivalence implementation](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-register-optimizer.cc#L322)
and [operand definitions](https://github.com/v8/v8/blob/63481c19453c72221531058be6d2e94adf3ebc66/src/interpreter/bytecode-operands.h).
They support the bounded analogies: receiver/arity facts reach call entry,
observable registers and register lists require materialization, and operand
scaling trades density against decoding. They do not prove a throughput
benefit for this Python runtime or require a new optimizing baseline IR.
Other V8 citations were not independently re-audited in this round.

The warmed capture identifies ten outer `CALL_PY_GENERAL` sites and the
`Logger.debug` exact inner call, consistent with prioritizing the allocating
general-call route. It remains a capture of sites, not route frequency.
The original `call-audit.md` proposal for nonallocating overlap remains a
conditional secondary experiment; it does not cover the primary outer call.
The coordinator should update its stale statement that warmed IR is absent.

Recommendations 1–3 have concrete source costs to remove. Recommendation 4
needs a separate emitter dataflow proof before editing. Recommendations 5–7
are correctly evidence-first or deferred: copying, encoding and backend
changes need dynamic denominators and ownership/observation proofs.
Recommendation 8 correctly stays outside the JIT-disabled parity claim.
The coordinator must retain each disposition in the acceptance ledger and
must not report all eight as implemented.

## Specialized call facts and the shared commit

`Python/generated_ir_cases.c.h:7288` checks PEP 523, function type, cached
function version and recursion before the successful `CALL_PY_GENERAL`
invocation. `Python/specialize.c:specialize_py_call` installs that cache only
for `_PyFunction_Vectorcall` and a valid version.
`Objects/funcobject.c:PyFunction_SetVectorcall` clears the function version
before changing vectorcall. This is a source-backed proof for the held-GIL,
nonescaping successful edge. It is bounded manual source reasoning, not
full C provenance or arbitrary extension-write proof.

The private entry may skip method expansion and redundant provider discovery
only on that edge. Carry an explicit prepare contract at the invocation
producer; do not infer it from a handler-name table. Keep `CALL_PREPARED`'s
existing method/tuple preparation meaning distinct from any additional
provider fact. The generic invocation remains the entry for guards that did
not complete, effects fallback, keyword/EX paths, monitoring and other
providers. Under `Py_GIL_DISABLED`, keep the existing generic behavior unless
a separate synchronization proof is reviewed.

The current emitter uses `call_completion()` for both normal invocation and
the generated fallback label. A global change to `self.call_run` or to
unconditional completion flags would accidentally give the fallback the
successful-edge proof. This is an implementation admission condition:
regenerate, inspect both emitted edges, and exercise emitter coverage that
checks the successful guarded entry and generic fallback independently.

`call_transaction_overlap_python` currently pins the selected code before
`PyDict_New`, refreshes the caller, and rechecks actual callable identity,
code identity, standard vectorcall, PEP 523, executable, stack capacity,
defaults coverage and self offset. All must survive the new entry. Code
flags and argument shape remain valid only after confirming code identity;
defaults and caller homes require fresh loads. A failed final guard releases
prepared resources, refreshes, and reaches the existing fallback once. An
error remains an error. The consuming binder itself must retain its current
post-allocation provider check.

The first window detachment starts a no-failure, no-reentry commit. It may
reuse the existing overlap primitive and fill checked defaults, empty
varargs and the prepared dict, then publish the caller return destination
and child. Parent owners must survive every fallible step. Child cleanup,
monitoring and recursion protocol must remain shared. No lazy/shared kwargs
or last-read local retirement is licensed by this proposal.

## P1: successful refresh

Accepted source proof. `value_operation_refresh` returns zero only after
checking the active frame, executable/words and current PC range, checking
feedback in debug, and restoring the current instruction. No escaping work
occurs before `call_transaction_refresh` repeats `value_frame_is_current`.
An immediate zero return removes only that duplicate successful validation.
The original failure branch must remain: error construction or failed-result
cleanup can reenter. Ordinary Python tests do not prove invalid-frame fault
branches; the implementation proposal correctly states that limit.

Actual W diff inspected in this round: exactly a three-line zero-status
early return in `Python/ceval.c:call_transaction_refresh`, with the old failure
branch intact. **Source review accepts that diff.** Integration acceptance
still requires the coordinator's release compile, applicable debug checks,
focused results and generated-code inspection. This review is not a build
receipt.

## P2: canonical typed code home

The two pointer constructors are `_PyFrame_Initialize` and
`_PyFrame_PushTrampolineUnchecked` in `pycore_interpframe.h`. Moves, copies and
`PyValue_None` sentinel writes are different cases. The code pointer's static
type and `PyCode_Check` prove it cannot take the exact numeric inline route;
the frame must still acquire one owner for mortal code and the canonical
no-count tag for immortal code. `PyValue_DUP` already supplies this rule.

**Resolved initial finding:** the first proposal made the generic constructor
call a helper asserting nonnumeric type after numeric canonicalization.
`PyValue_FromObjectBorrow` can reach that pointer tail for exact boxed numbers
such as a large int or NaN. That assertion would have broken debug behavior.
The corrected proposal factors only pointer/tag construction into a lower
primitive whose domain includes those already selected boxed numbers, and
puts the nonnumeric proof in the typed frame wrapper. Accept that corrected
contract. Document the lower primitive's restricted use; it is not a general
license to put exact inline-capable numeric boxes into homes.

Preserve generic NULL handling, numeric identity conversion and stats; inspect
both constructors and all new primitive uses. Keep count acquisition shared,
without passing a borrowed pointer to an owning steal operation. Verify
mortal code lifetime independently of the function's owner, suspended
generator and exception cleanup, trampoline entry, immortal tags and replaced
constants. A focused internal helper check can establish tags/counts; Python
lifetime tests establish frame use. The coordinator must inspect optimized
release output and measure before claiming removal or benefit.
