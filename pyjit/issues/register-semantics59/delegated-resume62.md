# Delegated resume62 source proposal

Root approved the contract and initial source diff. Initial native qualification
found two test/source-analysis issues: the exact monitoring fixture has identical
candidate/selected57 events (LINE can be absent), and an unsupported SEND body
must omit this optional variant instead of rejecting ordinary analysis. Both
corrections pass the three focused host checks, the existing altered-SEND source
method, and fixture AST parsing. Root approves the correction diff; both
corrected builds complete 117/79 and all 397 focused methods per role pass.
The stable two-pair screen improves generators 4.3% but loses DeltaBlue 6.5%,
logging 20.3% and spectral_norm 9.3%. Root rejects the standalone packet;
the [trial report](report.md#delegated-resume62-rejected) owns full evidence.
A is on branch
`pyjit/parity62-delegated-resume`, directly from selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0`. The losing ascent branch and its
evidence remain intact. Root integration is `17139aa0166`, with byte-identical
generated runtime products after the source-analysis correction.
RESUME_DELEGATED uses coordinated private handler 375.

## Executable contract

Add one execution-only handler, RESUME_DELEGATED, with RESUME's original A
argument and semantic base/shadow/twin mappings. Admit only this exact compact
cycle in the common executing-image constructor, after semantic admission:

```text
R-2: SEND
R-1: YIELD_DELEGATED
R:   RESUME 2
R+1: JUMP_BACKWARD_NO_INTERRUPT -> R-2
```

Existing image admission proves the SEND/yield receiver and outgoing roles;
the selector additionally matches their fields and the yield's resume A to its
outgoing B. Require the two bypassed operations' checked zero-home normal
effects and the actual jump body, including its assert/JUMPTO footprint.
The existing RESUME normal-transfer receipt checks both complete source uops;
reuse it. A changed source body or delegated transport invalidates generation
of the refinement instead of retaining permission from its opcode name.
Original source positions do not participate in admission.

Representation is deliberately fixed-relative: target is R-2, so no target
payload, lookup, allocation or word extension is needed for this first shape.
RESUME currently uses only A; B/C/D provide 36 unused bits (CD alone can encode
a 24-bit absolute target), but this bounded cycle needs none of them. All
operand fields remain byte-identical to the semantic RESUME. Nonadjacent SEND
cycles remain ordinary; do not add target storage just to generalize them.

RESUME_DELEGATED is added to the existing ExecutionVariant machinery, with a
small generator-side continuation receipt consumed by its emitted prefix.
Generated base and shadow are RESUME, twin is INSTRUMENTED_RESUME; event and
source identity are the original RESUME. It is specialized/non-pristine and
public image construction explicitly rejects it. Constructor/replace/marshal
images must present canonical words and pass existing semantic admission before
the same executing-copy selector can refine them. Malformed images retain
their specific ordinary rejection; valid unproved shapes remain ordinary.

## Hot consumer and attribution

Keep the current ordinary-RESUME arm of IR_SKIP_RESUME first and unchanged.
Only its otherwise path recognizes RESUME_DELEGATED and applies the same
eval-breaker/instrumentation equality, TLBC and Emscripten gate. Ordinary call
entry loads no target or plan and acquires no additional successful-path test.
Initial installation is disabled for Py_DEBUG, Py_GIL_DISABLED, WITH_DTRACE and
Emscripten: those builds retain existing behavior, including LLTrace callbacks.

For the admitted refinement, require no pending_redirect, set instr_ptr to
the original R, then to R+1 (the ordinary jump), and select next_instr=R-2.
Dispatch decodes the actual current SEND-family handler and runs its existing
adaptive guards. No SEND target/function/receiver is cached. Account the
bypassed jump in Py_STATS, as selected execution would; selected already skips
RESUME accounting through IR_SKIP_RESUME.

The generated RESUME_DELEGATED handler reuses this same admitted prefix before
the ordinary RESUME body. If the observation gate declines, execute that
ordinary body and its original R+1 continuation. Reuse the original error/
instrumentation paths, rather than implementing a second RESUME operation.
This handles direct entry when IR_SKIP_RESUME declined or was not reached.

Do not change suspension: YIELD_DELEGATED still publishes R, its original
recipient and delegated phase. Generator entry first calls SetReturnValue,
which can dispose a sent owner and invoke a finalizer/redirect. Its existing
LOAD_IP follows that effect and reloads the actual frame PC. Only then does
IR_SKIP_RESUME inspect the current executing word and epoch. Never retain a
pre-disposal target/refinement across the effect. Entry recursion, exception
links, current/previous frames and downward SEND remain unchanged.

## Mutable writer closure

Only R and its immediately following jump are covered execution sites. Add a
cold defusion helper used by instrumentation's set_word_opcode: before writing
site i, restore an installed head at i or i-1 to its original RESUME handler.
Operands never changed, so no saved payload or second word copy is required.
The low-level adaptive _PyIR_SetHandler and ordinary specialization success
paths remain unchanged: RESUME/jump have no adaptive family, and SEND target
specialization is intentionally read at dispatch, not part of this proof.
The preceding YIELD's immutable roles nominate the cycle; its instrumented
twin still executes before suspension and publishes the original R/phase.
Entry refresh after input disposal, the actual PC and the existing epoch gate
therefore remain authoritative; no pre-suspension observation decision is cached.

Audit store_opcode, line/instruction wrapper installation, displaced-opcode
restoration and direct word stores. Wrapper saved identities are ordinary
RESUME/JUMP event opcodes, never RESUME_DELEGATED. _PyCode_HandlerFor returns
ordinary RESUME when removing a wrapper; it cannot resurrect a stale head.
After a complete successful whole-code instrumentation update, a cold scan may
reinstall only when both actual executing R and R+1 handlers are ordinary and
the immutable admitted cycle still matches. Instrumentation at either skipped
site therefore prevents installation. A callback disabling one event locally
may leave the head defused; it need not reinstall it eagerly.

No general reverse index/span framework, per-hit neighbor/geometry checks,
activation outcome or ascent loop is introduced. An escaping writer in this
two-site audit is an implementation blocker, not permission for a hot guard.

## Files and gates

Source boundary: ir_generator.py/ir_metadata_generator.py and ir_numbers.txt;
Python/ir.c for cold selection/defusion, instrumentation.c for writer closure,
ceval_macros.h for the existing skip consumer; pycore_ir.h for declarations;
Objects/codeobject.c for private-handler rejection; focused generated-source
tests and new runtime continuation fixtures. Root owns all generated products,
full generation, native builds, target tests and timing.

Host checks mutate the actual RESUME, jump and delegated source contracts,
check the unchanged ordinary arm, generated base/twin/private admission and
absence of an extra owner/effect. Runtime fixtures cover canonical/compiler,
constructor, replace and marshal routes; differing source positions; changed
resume args, interrupting jumps and unsupported shapes; send/throw/close,
exception/lasti, nested/custom iterators and discarded resume-input finalizers.
Enable monitoring/LINE/INSTRUCTION/trace while suspended at both skipped sites,
clear observation, redirect, and verify both event opportunities reappear.

Native gates compare complete resume-to-current-SEND entry against selected57,
plus ordinary CALL entry to detect unrelated cost. Require removal of the
no-interrupt jump handler transfer without a geometry lookup, new frame field,
duplicated SEND guard or replacement staging. Charge the new handler/selector,
skip gate and text growth. A source/host pass is not native qualification or a
performance result; root screens all eight workloads before selection.
