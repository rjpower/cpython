**Verdict: AMEND.** The candidate is correct in its core transaction and matches the observed words exactly. Three amendments are needed before implementation: state the exact observer rule, drop redundant admission checks, and list the emitter gaps as stage-2 work.

## Source-proven facts

- **Word graph matches.** The decoded words in `before.json` give offsets 1 to 13 with the same handlers, destinations, sources and literal values as the candidate's table. Code flags are 3, so no generator flag.
- **Thirteen header words is right.** The frame struct has twelve pointer-size fields plus four bytes of small fields before `owned_slots`, so the header is 112 bytes and the macro yields 13. Six value homes remain.
- **Temporaries start NULL.** `_PyFrame_PushOverlapping` passes the argument count as the null boundary to `_PyFrame_Initialize`, which clears every remaining slot (`pycore_interpframe.h:274`). The four entry NULL checks on homes 2 to 5 are therefore provably redundant for an initial entry. Make them debug assertions.
- **Skipped RESUME proves breaker and version only.** The skip compares the whole breaker word against the code version (`ceval_macros.h:740`), so a pending breaker blocks it. It does not prove absence of instrumentation.
- **The instrumentation hole is real.** Line-only monitoring never rewrites RESUME (`instrumentation.c:1620`), and RESUME only carries the PY_START event (`instrumentation.c:1898`). With LINE events alone, word 0 stays plain RESUME and is skipped while word 1 becomes INSTRUMENTED_LINE. The candidate's "actual observer state" check is mandatory, not conservative.
- **Overflow and floor semantics.** Add and multiply box on int32 overflow (`bytecodes.c:695`), so declining to the interpreter replays correctly. Floor by literal 2 with a nonnegative guard and logical shift is exact.
- **Compact-int accessor dereferences NULL** (`pycore_stackref.h:1115`). Arguments of an exact-args call are never NULL, so the two checks are cheap insurance.
- **Emitter gaps.** `jit_x86.h` has 32-bit add, sub, imul, mov, load, and a conditional jump with the overflow condition. It has no `ret`, no `sar`, no 32-bit store, and its immediate arithmetic and shifts force REX.W (`jit_x86.h:319`). `_Py_JIT` is defined nowhere in configure inputs, so publication compiles away.
- **Lifetime.** `_PyCode_ClearExec` is the single free site (`ir.c:366`), and words follow the exec header directly, so appended fields are safe.

## Amendments, in priority order

1. **Freeze the observer rule exactly.** Admit only when the RESUME was skipped, `tstate->tracing` is zero, and the code's instrumentation version is zero. Zero code version plus a skipped RESUME implies global version zero, so no monitoring has ever been enabled. This is sound with one compare. It declines forever after any monitoring in the process. Report that as the stated starting policy with a decline-reason counter.
2. **Order the universal check so controls pay one load and one branch.** Read the descriptor from the exec header, which `IR_NLOCALSPLUS` already touches. Use a sentinel for "never eligible" set at first analysis. Do not load the code object or its version until the descriptor is non-sentinel.
3. **Drop the temporary NULL scans** per the fact above.
4. **Check tagged values at descriptor build.** When inline values are off the executing copy runs boxing handlers (`ir.c:303`), and an inline commit would be wrong. The config is per-interpreter and set at startup, so build-time suffices.
5. **Optional simplification.** Arithmetic shift by one is exact floor division by two for every int32, including negatives. Using `sar` removes the nonnegative guard and one exit. Either form is correct. If kept as proposed, keep the guard test.
6. **List stage-2 emitter additions explicitly:** `ret`, 32-bit immediate add, `sar` or 32-bit `shr`, 32-bit store, and a non-REX.W immediate path. Small, but each is a new encoding needing a disassembly check.

## Decline-all and overhead risks

- **Boxed arguments.** In the registered benchmark, `i` and `j` reach `eval_A` from tuple unpacking and `enumerate`, so they are boxed compact ints, not inline. Accepting both forms through the compact accessor is what makes real calls admissible. This is a hypothesis from the benchmark's structure, not something I verified in its source.
- **Monitoring at startup.** If the harness enables any monitoring or trace hook before the workload, the version rule declines every call. Record admitted and declined counts in the run so a null comparison is detectable before scoring.
- **Hot counter policy.** If analysis is deferred until 64 entries, every function pays a counter increment for its first 64 calls. Settle eligibility at first entry so unrelated code pays only the branch.
- **Where the win would come from.** Per admitted call the interpreter runs seven handlers with fourteen compact-int guards and home traffic. Native runs two guards, six register operations and two stores. The net is unknown, as the candidate says.

## Breakdown to a falsifiable elapsed result

1. **Stage 1, C leaf, about 30 minutes.** Descriptor builder over pristine words for this shape, shared entry in both hooks, the frozen admission rule, checked-builtin leaf, two-store commit, teardown in `_PyCode_ClearExec`. Tests: operator oracle, full 0 to 129 domain checksum, overflow replay at each site, NULL/bool/subclass/noncompact rejection, line-only monitoring before and after compilation, settrace, pending breaker, teardown, and a focused refleak test on the decline path.
2. **Stage 2, native, about 60 minutes.** Emitter additions, leaf emission, W to RX owner, retained disassembly. Same tests plus encoding and overflow-exit witnesses and a native-entry count with varying inputs.
3. **Elapsed.** The frozen two-round, five-value comparison. One precondition, not a sample: spectral_norm must show nonzero admitted entries, or the comparison measures nothing.

Defer CFI, profiler publication, ASan and broad suites to after a win. The leaf never moves RSP or hosts Python, so their absence does not invalidate native execution or the decision.

## Unsupported or hypothetical claims

- The 676,000 call figure is source-derived and not in the capture.
- The ninety-minute ceiling remains a hope. The prior disposition already declined to promise it fits.
- Everything about speed is unknown.
