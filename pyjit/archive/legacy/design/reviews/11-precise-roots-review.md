## Summary

The attributed walk and shared JIT safoint table give the proposal a coherent structure.  
Borrowing, detached contexts, and the PyMem conversion still contain correctness holes.  
The verifier and promotion order need revision before they can justify removing the existing protections.

## Blocking issues

1. **Non-reentrant calls do not preserve every edge of a reachable object.**

   **Where:** §§2.1, 4.2; [Python/gc_live.c](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_live.c:1086), [Objects/weakrefobject.c](/home/power/code/cpython/.worktrees/boil-10a/Objects/weakrefobject.c:158).

   The rule “across a non-reentrant call any field qualifies” admits weak references. A rooted weakref does not keep its `wr_object` alive. An inline collection clears that field even with `no_python`, then may sweep the borrowed target. The assertion that the collector changes no edge of a reachable object is therefore false. Borrowed caches present the same distinction between a pointer field and an owning edge.

   **Fix:** Require a traced, strong path from a root for every borrowing exemption. Reentrancy adds the requirement that this path remain unchanged. Weak fields and borrowed caches require an independent span unless another owning path is proved. Add a negative checker test and an injected collection between loading a weak target and using it.

2. **A detached snapshot can describe a frame that has returned.**

   **Where:** §3.3; [Python/pystate.c](/home/power/code/cpython/.worktrees/boil-10a/Python/pystate.c:2138), `PyEval_SaveThread()`.

   The named detach section omits transient wrappers such as `_PyThreadState_Detach()` and `_PyThreadState_Swap()`. In an unoptimized build, stopping at the first frame outside that section can save one of these wrappers; it returns before the detached interval ends.

   More generally, an extension helper may call `PyEval_SaveThread()` and return while still detached. The document supplies no C API restriction forbidding this. Today’s linear scan can tolerate starting below the live frames; an unwind using overwritten save slots cannot.

   CFA bounds also do not establish that memory remains readable. [The existing scanner explicitly handles recorded stacks belonging to exited threads](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_trace.c:1885).

   **Fix:** Specify how a persistent context is established for every detach entry, including swap paths and extension wrappers. Where persistence cannot be established, retain a mapped-range conservative scan, with transitive PyMem following, until a named migration removes it. Validate readability before reading unwind slots. Test a returning detach wrapper, an unoptimized swap path, and an exited retained thread.

3. **The PyMem inventory misses a live construction buffer.**

   **Where:** §6.2, rung 3; [Python/modsupport.c::do_mkstack()](/home/power/code/cpython/.worktrees/boil-10a/Python/modsupport.c:225), `_Py_VaBuildStack()`, and `Objects/call.c`’s formatted-call helpers.

   `_Py_VaBuildStack()` allocates a PyMem array when arguments exceed the small buffer. `do_mkstack()` fills it through repeated allocating calls. Previously constructed objects may then exist only in that array. These are core opaque frames, so decision 5 removes their protection before the array reaches a precise call entry.

   The stated prefix conversion also does not directly fit `partial_vectorcall()`: it writes keyword values and temporary keys into suffixes before filling positional entries.

   **Fix:** Add spans inside `do_mkstack()` during construction and in the callers through cleanup. Describe each disjoint initialized range in `partial_vectorcall()`, or reorder initialization to produce a valid prefix. Add deterministic collection during construction of a formatted call with more than five freshly created arguments, plus failure cleanup and keyword-merge tests. A clean suite does not close an inventory already missing this site.

4. **Retain mode can report legitimate weakref access as a missed root.**

   **Where:** §8.3.

   Suppose A considers an object dead, B retains it through a stale word, and a live weakref points to it. Retain mode deliberately leaves the weakref intact. Calling that weakref and storing its result into a list legitimately makes the object reachable from A. The proposed verifier aborts, although no root was missed.

   Retain mode also conflicts with the first-collection reclamation tests: B deliberately scans the stale words those tests require the collector to ignore. “The full suite in retain mode” needs an explicit treatment of these incompatible expectations.

   **Fix:** Treat later A reachability as a candidate requiring provenance, not proof of a missed root. Add the weakref counterexample to the verifier’s demonstrations. Run reclamation assertions under A/kill semantics and define a separate retain-mode validation contract. For kill mode, specify that quarantine begins only after actual sweeping; finalizer and destroy-hook resurrection must retain its normal behavior.

5. **Rung 1 removes protection before its replacement proof exists.**

   **Where:** §§2, 2.1, 4.3, 10.

   Rung 1 marks ceval functions and entry paths precise and deletes the activation and inline-scan protections. The C checker and helper conversions arrive in rung 2. That permits a level-3 landing with unchecked precise helper frames.

   There is also a specification mismatch: §2 includes every function in `ceval.c`, while §2.1 rejects computed goto, which the plain eval loop uses.

   **Fix:** Deliver the checker, negative tests, and required spans before promoting each function. Make promotion conditional on successful checking in the same rung. Define how the generated handler region and the eval loop’s surrounding entry and exit code are checked without accepting an otherwise forbidden function body.

6. **The suspension-only barrier rule omits running heap frames.**

   **Where:** §§4.4, 11; [Python/bytecodes.c::_SEND](/home/power/code/cpython/.worktrees/boil-10a/Python/bytecodes.c:1530), `Include/internal/pycore_interpframe_structs.h`.

   A resumed generator keeps its slots in `owned_slots`; it does not move them onto the thread’s data stack. An old generator can store a young object and trigger a minor collection before yielding. Enumerating data-stack frames and remembering the generator only at suspension does not expose that young reference when the old generator is skipped by the minor marker.

   **Fix:** Enumerate executing generator and coroutine frames directly as roots from resume through suspension or clearing. This can preserve the proposed barrier once per suspension. Otherwise, heap-frame stores need barriers. Add a minor-collection test inside a resumed old generator before its next yield, including a coroutine case.

7. **The capture contract applies x86-64’s stack adjustment to arm64.**

   **Where:** §§3.2–3.4, rung 1 and the arm64 scope cut.

   Section 3.3 specifies entry SP plus the return-address size for both architectures. On AArch64, `BL` places the return address in LR and does not push it onto the stack. Adding eight bytes produces the wrong caller SP. Return-address validation must likewise support register-held addresses. [AAPCS64 §6.5](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst#65-subroutine-calls)

   **Fix:** Write architecture-specific capture contracts: x86-64 uses `[rsp]` and `rsp + 8`; arm64 uses LR and unchanged SP. Define register and memory return-address rules separately. If arm64 execution tests are cut, keep its precise walker disabled and use the conservative fallback until those tests pass.

## Non-blocking issues

- **The inline audit needs a fail-closed build contract (§2).** Missing or reduced DWARF must not look like “no unchecked inlining.” Specify required debug information, coverage checks for every precise range, and behavior under stripping, LTO, and compiler cloning. Add a deliberately missing-debug-information case.

- **Test raw box preservation (§6.2).** Include a `PyObject *` span holding a non-NaN float box. Canonicalizing that pointer would preserve the inline value while losing the actual C box. Also test initialized NULL entries, span rebasing, and allocation failure.

- **Add a direct C API measurement (§9).** Pure Python kernels do not isolate the new public-entry spans. Measure repeated `PyObject_Vectorcall()` calls with small and heap argument arrays. Use the actual merged predecessor as the comparison baseline if 6c lands after the cited commit.

- **Keep the adopted plan accurate.** The opening scope change is clear, but note 06 §6 and note 08’s retirement rows should be updated when this specification is adopted. Partial landings need to record which mechanisms actually retired.

## Questions

- Is the descriptor cache per interpreter/GIL domain, or process-wide? What synchronizes cache flushing and JIT range registration across interpreters with separate GILs?
- Does every fallback restore transitive PyMem scanning when frame linkage cannot be classified?
- How will tier 1’s stale physical slots and virtual frames be materialized for `f_locals`, frame inspection, and reentrant callbacks?
- What are the supported-platform guarantees after deleting activation exclusions and clears, given the full conservative fallback elsewhere?

## Verdict

revise