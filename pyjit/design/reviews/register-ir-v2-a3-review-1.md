# Review 1: register IR v2 steps A3a and A3c, tail calls as the only backend (proposal)

Reviewed: [register-ir-v2-a3.md](../proposals/register-ir-v2-a3.md) (605
lines, uncommitted, revision 1 of 2026-11-04), with its scripts and probes in
`build/pyjit-link-wave1/a3/`. Code: runtime tree
`build/worktrees/pyvalue-rc-universal`, read-only, at HEAD `36aa4a0345a`. The
proposal counts at `faddfd1454d`, one commit earlier. Compilers: GCC 15.2.0
and `/usr/bin/clang-21`, x86-64.

Probes, in one scratch directory that has since been removed:

- **P1.** Three handler shapes under each convention, compiled at `-O2` and
  `-O3` with `-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer
  -fstack-protector-strong -fcf-protection=full`: a cold C call, a hot C call
  whose result is stored, and a frame switch. The same file was compiled at
  `-O0` and `-Og`.
- **P2.** A caller in the shape of `_PyEval_EvalFrameLoop`, holding a 544 B
  record.
- **P3.** `__has_attribute` on each compiler and Clang target.
- **P4.** HEAD's `Python/ceval.c` compiled as the tail-call build on both
  compilers. These were copies patched by `irv2/tcpatch.py`'s two edits, with
  `-D_Py_TAIL_CALL_INTERP=1`, the release flags and both hardening flags, and
  `pyconfig.h` from `pyvalue-rc-repr-release` with computed gotos off. The
  objects were read with `irv2/tcframes.py`. GCC was also given
  `-Wmaybe-musttail-local-addr`.

Nothing was linked or timed.

## Verdict

**Accept with changes.** The main decisions hold:

- one backend;
- `frame` as a parameter;
- the attribute selection;
- a record reached from `tstate`;
- a generator rewrite rather than hand edits;
- the landing order.

P1 confirms the register claims and `musttail` on every shape. But the G9
recount, on which §3.4, §6 (A3.4) and §7 rest, counts only addresses visible
in handler source. On the compiled objects, **200** tail-call functions carry
a canary on both compilers, not 133. The 67 that the recount misses take
**41.8%** of entries, and `LOAD_FAST_BORROW` alone takes 13.8%. Nearly all of
them come from one inlined helper (finding 1). The fix is small, but the
counts, the expected effect and the A3.4 plan must be redone from the object.
GCC's hot-path cost is also understated for handlers whose C call is on the
hot path (finding 2).

## Findings

| # | Severity | Finding | Evidence | Fix |
|---|---|---|---|---|
| 1 | **must-fix** | **The G9 recount misses addresses taken inside inlined helpers.** §3.4 says 133 functions and 14.9% of entries, and §3.4's "inline-only" bullet says that addresses passed only to `static inline` code "go away when the call is inlined". At HEAD, 200 of the 298 tail-call functions have a canary, on GCC and on Clang alike. All 133 of the proposal's list are among them, and 67 more are not on it. Sixty-six of the 67 use `finish_displaced_local`, a `static` helper that takes `displaced` by value and passes `&displaced` to `finish_value_bridges_ex`. Once it is inlined, `displaced` becomes a handler local whose address escapes. The 67th, `LOAD_GLOBAL_BUILTIN`, is there because GCC did not inline `_PyValue_Publish` (`call _PyValue_Publish` with `lea -0x10(%rbp),%rdi`). Weighted with `weights.py`'s method, the 200 take **56.7%** of entries: the 133 take 14.9%, and the 67 take 41.8%. The largest of the 67 are `LOAD_FAST_BORROW` 13.8%, `PUSH_NULL` 3.0%, `LOAD_SMALL_INT_INLINE` 2.9%, `LOAD_COMMON_CONSTANT` 2.7%, `LOAD_GLOBAL_BUILTIN` 2.2% and `BINARY_OP_ADD_INT` 1.5%. The packet already had the figure: `tcvt` showed 200 canaries (packet:1120). | P4. `ceval.c:2128–2137` (`finish_displaced_local`); `ceval_macros.h:704–734` (`IR_RETIRE_LOCAL` and `IR_STORE_DST`); proposal:336–352 and :524 | Recount from the object, and make the object list the work list. Name the helper fix as a step: `finish_displaced_local` passes `displaced` without taking its address, or becomes `Py_NO_INLINE` so the address is taken in its own frame on the cold path. Make `_PyValue_Publish` `Py_ALWAYS_INLINE`, or have it take a value. Then redo the following: §3.4's table and entry shares; §7's A3.4 row, since about 8 instructions on 56.7% of entries is about 4.5 instructions per word, not 1.2; the size of A3.4's fan-out; "eight hand edits". Move the helper fix into A3.2: it is two lines, and without it A3.2's measurement mostly measures the canary in `LOAD_FAST_BORROW`. |
| 2 | should-fix | **GCC's hot-path cost is understated when the C call is on the hot path.** Every shape in §2.2 puts its call on a cold path. Shape `hotcall` in P1 has the form `r = op(tstate, slots[A], slots[B]); if (!r) error; slots[C] = r; DISPATCH()`. GCC `no_callee_saved_registers` takes **37** instructions to the dispatch: 5 moves into callee-saved registers, 3 moves back, and a spill and reload of `next_instr`. Clang `preserve_none` takes **26**: one move, of `ir_word`, and no stack references. That is +11, outside §2.2's "−1 to +7". With the attribute dropped, GCC takes **48** with 6 pushes and Clang 46 with 7. So §2.5's "GCC, attribute dropped: unchanged on most shapes" holds only for cold calls. The `coldcall` shape reproduces the proposal: 28 against 21, 4 moves in and 3 out. | P1 (`-O2` and `-O3` give the same counts); proposal:155–161, :239–241 | Add the hot-call shape to §2.2 and §2.5. Weight R1 and Q2 by the entries to handlers that make a C call on their hot path (the generic `CALL_*`, `BINARY_OP` and `LOAD_ATTR` forms). State the GCC penalty as 0 to 11 instructions per handler. |
| 3 | should-fix | **The base has moved, and A3.1 misses `sys._jit`.** `36aa4a0345a` makes `sys._jit` report the fork's JIT. `is_enabled()` calls `_PyJit_NaturalAdmissionEnabled()`, which is defined in the fork's JIT objects, and `Lib/test/test_sys.py` changed with it. If those objects leave the build as §1.1 plans, the link fails. The commit also deletes `jit_publish.c` and `jit_unwind.c`, so §1.1's sentence about them is stale. | `Python/sysmodule.c:25`, `:4250`, `:4263`; `git show --stat 36aa4a0345a` | Recount §1.1 at `36aa4a0345a`. Add a row for `sysmodule.c` and `test_sys`: `is_available()` and `is_enabled()` become constant `False` with no JIT object behind them. Drop the `jit.c`/`jit_publish.c`/`jit_unwind.c` sentence, and list the `PCbuild` entries for consistency. |
| 4 | should-fix | **R4's gdb fix breaks for nested activations.** `py-bt` visits every gdb frame named `_PyEval_EvalFrameLoop` and reads that frame's Python frame. If each one reads `tstate->current_frame`, every outer activation reports the innermost Python frame. The `frame` parameter of a handler frame does not help either: GCC builds most small handlers with no frame (finding 9), so gdb often has no such frame to read. The record gives the correct answer. Python frame k lies one activation below the next-inner activation's entry frame, so it is `act[k−1].entry.frame.previous`, and `tstate->current_frame` for the innermost activation. | proposal:568; `Tools/gdb/libpython.py:106`, `:1885–1894`; `ceval.c:2303` (`entry.frame.previous = tstate->current_frame`) | In A3.2, have `libpython.py` walk the `eval_activation` chain, and give the record a fixed local name in `_PyEval_EvalFrameLoop` for gdb to read. Until a gdb host runs `test_gdb`, record it as unverified, as proposed. In A3.4, restore the two `assert(frame == &entry.frame)` checks (`bytecodes.c:1477`, `:5695`) as `frame == &act->entry.frame` rather than deleting them. |
| 5 | should-fix | **The C-stack claim rests on stale frames.** §3.2 calls the net change "about zero" against a largest handler frame of 512 B on GCC and 464 B on Clang, measured at `48bb4d6d1d3`. At HEAD, the tail-call frames are at most **320 B**, with a median of **64 B**, on both compilers. At each nesting level, the handler that calls out keeps its own small frame and also pays for the record, so each nested activation costs about 400–480 B more than HEAD's tail-call build. Against the qualified build, computed goto with its 3,304 B loop frame, it costs about 2.7 KB less. | P4 (`tcframes.py`: max 320, median 64 on both); proposal:294–297 | State the change against computed goto, where it is a gain. Record `sizeof(_PyEvalActivation)` and the depth that `test_call`'s `TestRecursion` reaches before and after. |
| 6 | should-fix | **The plain-`musttail` arm builds silently.** It is the arm for GCC off x86 and for Clang before `preserve_none` arrived, and it costs +10 to +20 instructions on handlers with a call (finding 2; §2.5). §2.4 says no configuration builds it, but nothing stops a measured build from landing on it. The §2.4 snippet also tests `_Py__has_attribute` first, which is 0 on MSVC, so as written MSVC falls to the empty arm. Upstream's MSVC arm (`ceval_macros.h:107–110`) has to come first. | P1; P3; proposal:211–233 | Keep the arm. Have `configure` print the chosen convention and record it as a config variable, and have the measurement harness refuse an unqualified one. Put the MSVC test before the `__has_attribute` tests. |
| 7 | should-fix | **GCC already names some escapes at compile time.** With `-Wmaybe-musttail-local-addr`, HEAD's GCC tail-call build gives 1,111 warnings in 115 handlers, naming `value_owners` (955), `call` (111), `err` (28), `pending_self` (8), `retval_o`, `publication`, `method_found`, `dupkey` and `temp`. It names neither `displaced` nor `held_*`/`operation`, so it complements the object check but cannot replace it. A stored address, like `method_found`, still live at a `musttail` is a use-after-return bug, not only a cost. | P4 (GCC diagnostics) | After A3.4, build the GCC configuration with `-Werror=maybe-musttail-local-addr`. In A3.4's fan-out, give workers the warning output, which names source locals, alongside `tcframes.py`'s list. |
| 8 | should-fix | **The estimates omit known items.** Missing are: the helper fix (finding 1); `libpython.py` (finding 4); widening the sampler classifier before measuring (§7's own note); `sys._jit` (finding 3); and regenerating `configure` (R9). The 5 h for the generator rewrite covers declaration parsing, splitting declarations with several names, the union, rewriting each use, hot copies, refusals and tests, which is tight. | proposal:491–498 | Cost the missing items, about 2–3 h, giving 24–26 agent-hours. Keep the landing order, with the helper fix in A3.2. The bulk style fits A3.1 (a script plus the linker), A3.3 (an `#if` resolver plus the generator) and A3.4 (the generator plus the object list). |
| 9 | note | **GCC drops the frame pointer in handlers whose only call is the tail call**, even with `-mno-omit-leaf-frame-pointer`: P1's `h_switch` and the proposal's `copy` show it. In a frame-pointer call chain, a sample in such a handler skips `_PyEval_EvalFrameLoop`. Clang keeps the frame. Inclusive attribution therefore differs between the compilers. `-X perf` trampolines sit below the loop and are not affected. | P1; proposal:150–154 | Have the sampler's widened bucket count the `_TAIL_CALL_*` self symbols, not loop-inclusive time. Note this in R5. |
| 10 | note | **The activation semantics hold.** Re-entry from C (`__del__`, trace and monitoring callbacks, `gen.send`, PEP 523 hooks and the signal handlers that `_Py_HandlePending` runs) goes through `_PyEval_EvalFrameDefault`, so each gets its own record. The 232 B scratch is per activation and dead at dispatch, so nesting cannot clobber it. Labels reached by `JUMP_TO_LABEL` share the union only after the jumping handler's members are dead, and `finish_value_bridges` reads `owners` before the jump (`ceval_macros.h:127–137`). In P2, the caller saves all six callee-saved registers and puts a canary on the record once per activation, which is fine. The weak point is a stack switch that does not save `eval_activation`. The handler then writes into another stack's record, which corrupts memory silently rather than crashing. | P2; `ceval.c:2513–2537` | Add a debug assert at record use: `act` lies between the current stack pointer and `act->previous`, in the stack's growth order. It catches a stale pointer cheaply. |
| 11 | note | **L1 holds for every field listed.** But the generator's refusal of a `PyValueBorrow` field has no remedy: nothing says what a worker does when it fires. The record is never scanned, so L1's reason (homes are read by others) applies only weakly to the scratch. | proposal:306–319 | State the remedy: `TAKE` before the escape, or a slot-addressed parameter (G9.6). |
| 12 | note | **Selection by `__has_attribute` is sound for x86-64.** GCC 15.2 has `no_callee_saved_registers` and `musttail` but not `preserve_none`. Clang 21 has `preserve_none` and `musttail`. Clang also reports `preserve_none` on i686 and AArch64. A future GCC `preserve_none` would silently move GCC's state into r12–r15. | P3 | Record the compiler version and the chosen attribute with every measurement. |
| 13 | note | **Q1: delete the source.** This agrees with the proposal. History keeps it at `faddfd1454d`, and §1.2 rewrites rather than restores. | – | Put the deletion in its own commit inside A3.1, after the hook removal, and name the commit in the JIT-stage notes. |
| 14 | note | **Free threading stays unbuilt** after A3c, with nothing to say so. | proposal:480–483 | Have `configure` reject `--disable-gil` with a message, rather than leave a path that is neither built nor tested. |
| 15 | note | **Remote debugging and `_Py_DebugOffsets`.** The entry frame stays a valid `_PyInterpreterFrame` on the C stack, now inside the record, and the frame chain is unchanged. The debug offsets are `offsetof` values, so a new `_PyThreadStateImpl` field changes nothing. CET is also unaffected: handlers are reached only by indirect `jmp`, all have `endbr64`, and the shadow stack sees only the loop's `call` and `ret`. | P1, P2; `ceval.c:2303–2305` | Keep `test_external_inspection` in A3.2's test set, as proposed. |

## Claims checked and confirmed

| Claim | Result |
|---|---|
| GCC state in rdi, rsi, rdx, rcx and r8; Clang in r12–r15 and rdi | Confirmed by P1. In `h_switch`, `instr_ptr` is stored through rdi or r12, slots are read through rsi or r13, and `tstate->current_frame` is written through rdx or r14 |
| `musttail` on every shape: cold call, hot call, frame switch, error tail call | Compiles at `-O0`, `-Og`, `-O2` and `-O3` on both compilers (P1) |
| `copy` 14 on GCC and 17 on Clang; `coldcall` 28 and 21 | Reproduced |
| 292 `TARGET`s and 5 `LABEL`s; box capacity at most 20 | Reproduced from `generated_ir_cases.c.h` |
| 133 functions and 558 sites in the TSV | Reproduced from `addr-faddfd1454d.tsv`, but incomplete (finding 1) |
| Five `_Py_TAIL_CALL_INTERP` sites in `bytecodes.c` and three slot-addressed `null_or_index` sites | Confirmed at `bytecodes.c:1476, 1866, 5670, 5694, 5729` and `:1630, 3764, 3807` |
