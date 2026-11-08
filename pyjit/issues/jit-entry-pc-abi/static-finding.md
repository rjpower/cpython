# Rejected native prefix: interpreter entry tax hypothesis (NULL-decline design)

The fixed campaign rejected source67f5 under the aggregate gate. This host-only diagnostic grants no rescue, source change or new sample. It identifies one independently testable compiler-interface hypothesis; it does not identify the cause of the measured regressions.

## A low-call control still executes millions of interpreter words

The unchanged unpack_sequence source defines400 straight ten-target tuple assignments in do_unpacking's one range loop. The registered run(4000) invokes bench_all, each tuple/list wrapper once and do_unpacking twice: six ordinary Python function entries including run itself, against3,200,000 unpack assignments and32,000,000 target stores per registered call. These are source-path counts, excluding incidental interpreter callbacks/reentry, not dynamic entry telemetry. Calls to range/list/tuple use built-in objects on this source path. The loop body contains no Python call expression. All six code entries encounter unsupported prefixes and can cache structural rejection; a TryEntry call remains at each entry.

This large work-to-entry ratio makes six direct rejection calls alone a weak explanation for a repeated unpack-loop slowdown. It does not make a universal entry mechanism free: adding its call sites can change code generation throughout the evaluator.

## Actual frozen machine code shows an IP-home change

Matched GCC/O3 images are the accepted parent1561b0a9 and candidatee548d267; their exact paths/hashes, reviewed302-slot PIE relocation decoder, tool hash and thirteen static objdump commands are in capture.json. No image was executed. Both complete named evaluator ranges are retained; the symbol tables expose no separate named cold partition. Size changes from169325 to173097 bytes (+3772, 2.228%), a diagnostic, not a size veto.

All14 compiled calls to TryEntry are present. At candidate0x2a17e1, GCC passes lea -0x160(%rbp),%rdx as the address of next_instr; the call is0x2a17fa and dispatch reloads it at0x2a17ff. The new source API gives the callee this interpreter-local pointer through _PyIRWord **.

Crucially, changed traffic also appears where there is no entry hook:

- Parent LOAD_FAST_BORROW0x2b01a3 uses r15 as IP, computes the next word in rbx and transfers it back to r15 at dispatch. Candidate0x2a8712 loads IP from -0x160(%rbp), writes its increment back at0x2a8739 and reloads it at0x2a877a before dispatch.
- Parent STORE_FAST0x2b4538 similarly advances IP in registers. Candidate0x2a52cc stores the increment at0x2a52ed and reloads it at0x2a5380.
- Parent FOR_ITER_RANGE0x2b229f uses r15/rbx for the same continuation; candidate0x2a7004 stores its increment at0x2a7025 and reloads it at0x2a70ba.
- The actual tuple fast path is branch-followed: parent guard at0x2a8fe1 reaches0x2b91f5 and dispatches from0x2b9253 with rbx/r15. Candidate's fast path reaches0x2ab570 and reloads the stack-resident IP at0x2ab5d3. Prefix windows alone are not presented as complete handler CFGs; the complete evaluator captures and followed fragments are retained.

The frame and slot-base were already stack-resident in these parent examples. The new observation is IP traffic and changed layout; claiming every VM value newly spilled would overstate the evidence. Actual timed handler frequencies, retired loads/cycles and cache effects are unmeasured. The input shapes make these handlers plausible consumers, but there was no per-PC census in the timing processes.

## One new hypothesis: pass and return the continuation by value

Hypothesis: giving the universal interception call an escaping pointer to the evaluator's IP local induces ordinary-handler IP home traffic that materially contributes to control regressions. The direct rejection cost, helper register clobbers and unrelated layout changes remain competing explanations.

A separate source experiment could replace the out-parameter entry ABI with a by-value current IP and by-value continuation result (NULL on decline; the caller updates its IP only on non-NULL). It would retain the same generic plan,64 eligible threshold, leaf, guards, frame-home transaction, observer/root/exception/breaker semantics and all entry positions. The fixture derives success from a non-NULL result and preserves its local continuation on NULL. This introduces no mode/selector, smaller region, new compiler profile or second execution implementation, and the rejected source remains frozen.

The discriminating native precondition is concrete: check the same matched compiler's complete ordinary handler paths for recovery of an IP register and removal of repeated IP-home loads/stores while all entry calls remain. If the spill persists, that proposed mechanism has not been isolated. A separately approved finite semantic/native and fixed elapsed experiment would then test whether controls recover while the spectral mechanism still executes. Source appearance, byte count or an unchanged call count cannot establish a performance win. This is an architectural interface hypothesis for future review, not authority to modify or remeasure the rejected candidate.
