# Candidate 41 call-path regression triage

Read-only source and ELF inspection of frozen39 `4d666e1b482` and candidate41
`0a35a382909`. No source edits, builds, native workloads or commits were run.
The coordinator's first full29 campaign reports candidate/frozen39 elapsed
geomean 1.01024, with coroutines 1.0644, logging_silent 1.0544 and DeltaBlue
1.0389. Those figures motivate this triage; paired confirmations and PMU
attribution remain separate pending evidence.

Root's later six-pair repeat confirms logging_silent 1.04186 (every pair above
1.029), coroutines 1.07871 (every pair above 1.068), and DeltaBlue 1.03502
(every pair above 1.0156). These replace neither the full29 roster nor the
mechanism checks below. The regressions warrant investigation independently of
the two improved micros.

**A real compiled call-path change exists:** the candidate outlines overlapping
frame construction at three sites that were previously inlined at the two
preexisting sites. The smaller positional handler therefore does not establish
less executed work. This is a plausible logging cost, but it does not explain
the dominant recursive coroutine path, which cannot take that overlap route.

## Identities and checks

Inspected existing ELFs below
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/`:

- Frozen39: `golden-20261109-release/python`.
- Candidate41: `execution-boundaries-release/python`.

Root reports equal pyconfig/compiler configuration. Runtime source is
`worktrees/execution-boundaries-20261108`; its inspected HEAD is candidate41.
The overlapping helper and `Include/pyport.h` have no source diff between the
two commits. Commands were read-only `nm -S`, `size`, and
`objdump -d --no-show-raw-insn --disassemble=SYMBOL ELF`. A small UV host script
compared instruction lists after removing relocated symbol addresses and RIP
displacements. This comparison is static, not a dynamic instruction count.

## Frame construction changed from inline to a native call

| Symbol | Frozen39 bytes | Candidate41 bytes | Observed difference |
| --- | ---: | ---: | --- |
| `_TAIL_CALL_CALL_PY_GENERAL` | 0x5c8 | 0x3ff | Former inline frame body becomes a helper call |
| `call_transaction_overlap_python` | 0x7cb | 0x600 | Same outlining at its successful overlap commit |
| `_TAIL_CALL_CALL_KW_PY` | 0x13d | 0x5f8 | New guarded binding body, fixed owner scratch and helper call |
| `_PyFrame_PushOverlapping.isra.0` | Absent | 0x20c | Newly emitted shared native helper |

Candidate calls to the new helper occur at exactly these three ELF addresses:

- `0x347417` in `call_transaction_overlap_python`;
- `0x37f6f3` in `_TAIL_CALL_CALL_PY_GENERAL`;
- `0x381094` in `_TAIL_CALL_CALL_KW_PY`.

The helper starts at `0x33d4f0`. Its native prologue pushes rbp and five callee-
saved registers and reserves 0x48 bytes; its ordinary return restores them.
The caller also prepares native arguments and spills/reloads values across the
call. The six pushes, six pops, frame-pointer setup, stack reserve/restore,
call and return are 17 boundary instructions before counting caller spills.
This is not a net dynamic cost estimate: register allocation elsewhere also
changes, and the old inlined caller carried more spills of its own.

Frozen39's positional handler contains frame-storage reservation/header creation
inline at `0x37f58a` onward; candidate performs the call at `0x37f6f3` instead.
The source helper remains one implementation. The added keyword call site and
larger binder changed the compiler's decisions; their precise heuristic cause
has not been established without compiler inline reports.

## Other added work and unchanged paths

The positional current-default guard is required for correctness. For calls
supplying every positional parameter, candidate adds an arity read/compare and
branch around that guard. Calls needing defaults also read current tuple size,
check the scalar bound and required coverage, and verify cached-offset agreement.
Do not remove this guard to recover timing: same-code functions can share a
version with different defaults. The new kind parameter is constant-folded from
the positional handler; there is no dynamic keyword-kind branch there.
There are also **no** `%fs:` canary loads/checks or `stack_chk` references anywhere
in either positional-general handler. Its rbp push remains at entry+0x37 in
both, and its stack reservation shrinks from 0x90 to 0x50 bytes. Dead keyword
scratch therefore did not add a positional stack protector or an earlier
positional prologue in these ELFs.

The keyword handler now has a stack canary: it loads `%fs:0x28` at `0x380b9c`,
and its exits check the saved value. The fixed three-owner C array changes the
handler's stack frame from 0x10 to 0x80 bytes. That cost also applies to keyword
plan misses. Its generic fallback passes CALL_PREPARED, whereas frozen39 passed
zero. At this checked exact-Python-function site that skips the existing method
expansion check; it does not replay preparation or introduce a second binder.
The positional fallback already passed CALL_PREPARED in frozen39.

These symbols have identical normalized instruction lists in the two ELFs:

| Path | Static instruction count in each |
| --- | ---: |
| Generic `_TAIL_CALL_CALL` | 150 |
| Positional exact-args handler | 339 |
| Bound-method exact-args handler | 500 |
| Vector/keyword transaction runners | 127 each |
| Generic Python fallback provider | 530 |
| SEND_GEN / FOR_ITER_GEN | 170 / 131 |
| RETURN_GENERATOR / RETURN_VALUE | 223 / 127 |
| `_TAIL_CALL_start_frame` | 65 |

The instruction comparison retains symbol names and internal branch offsets;
it ignores relocated addresses, not operations or register operands. Feedback
entry sizes remain unchanged. The stats-only keyword hit counter adds no
ordinary-release counter update. ELF `size` reports text 7,524,691 → 7,527,155
bytes (+2,464); data and bss are unchanged. Small total growth does not exclude
hot layout/front-end effects.

## Application exposure and attribution limits

`logging_silent` issues ten `logger.debug(msg)` calls per outer iteration.
`Logger.debug(self, msg, *args, **kwargs)` is a natural positional-general shape:
two supplied arguments, empty variadics and no missing positional defaults. A
thread-owned caller can use the direct overlapping binder, so the newly outlined
frame call plausibly executes ten times per iteration. Its inner
`isEnabledFor(DEBUG)` is an exact-arity call, whose compiled handler is unchanged.
Actual warmed handler and hit weights still require root's profiles/counters;
source eligibility alone is not evidence that every site hits.

The coroutine kernel recursively awaits `fibonacci(n - 1)` and
`fibonacci(n - 2)` without keywords or defaults. My initial attribution to the
general-call fallback was incorrect. The coordinator's retained41 profile
instead shows CALL_PY_EXACT_ARGS at 10.28% self, RETURN_GENERATOR 8.25%,
GET_AWAITABLE 6.09%, CompleteGeneratorReturn 5.89%, make_gen 5.88% and SEND_GEN
5.52%, with no YIELD samples. `function_kind()` admits optimized simple
signatures regardless of CO_COROUTINE; its exact-argument opcode selection is
separate from the general direct plan's coroutine exclusion.

For generator-owned callers, the exact-argument handler uses
`_PyFrame_PushUnchecked`, copies argument owners and clears the caller window.
Thread-owned callers use overlap. The entire exact-argument handler is
instruction-identical in frozen39 and candidate41 after relocation removal.
Thus the dominant observed call route does not execute the newly outlined
general-call helper or its defaults guard. Profiles, not benchmark-name or
specializer-plan assumptions, determine which path needs explanation.

For a valid function/version with DIRECT clear and a successful generic child
push, a static traversal from handler entry through that fallback into
start_frame visits **67 frozen39 versus 65 candidate instructions**, with four
versus two rbp-relative home accesses. Both call the same normalized vector
transaction. Conditional version/type/error exits were held false; the DIRECT
miss and successful child outcome were selected explicitly. This is a path
ledger for an alternative general-call path, not a ledger of the profiled
dominant coroutine route. It supplies no added-executed-work explanation for
the coroutine regression. Code placement, cache effects or an uninspected hot
path remain hypotheses to test with the retained PMU/profile evidence.

### Complete generator-owner early fallback, when a general call is used

The 67→65 ledger above counts the handler only, including its call instruction;
it does not count generic helper internals. Those were inspected separately
after root requested a shrink-wrapping check. In both builds,
`call_transaction_overlap_python` executes exactly 13 instructions for VECTOR,
uninstrumented, generator-owned callers: kind load/test/branch, instrumented
load/test/branch, two pointer moves, owner compare/branch, move status 1, move
return register, ret. The owner compare is at entry+0x1d, with the taken branch
at +0x21. The native rbp/register-save/stack-reservation prologue starts at +0x27
and is **not executed** on this route. The return island moves from +0x208 to
+0x200 and uses different scratch registers; it gains no instructions or stack
accesses.

The enclosing `call_transaction_run_vector` has the same normalized 127
instructions, including its native prologue, transaction storage and canary,
and makes the same overlap-then-fallback calls. Inspection continued into the
fallback's callees; all of these complete normalized instruction lists match:

| Callee | Static instructions in each |
| --- | ---: |
| `eval_frame_push_and_init_code` | 1,764 |
| `eval_frame_push_and_init_ex_code` | 412 |
| `_PyThreadState_PushFrame` | 224 |
| `call_transaction_refresh` | 93 |
| `_PyEval_FrameClearAndPop` | 78 |

This rules out a newly executed overlap-helper prologue or positional canary as
the coroutine mechanism. It does not establish a whole-program instruction
identity, dynamic call mix, branch prediction outcome or cache residency. Use
the measured PMU/profile evidence to locate the remaining gap; code layout is
a hypothesis rather than a finding.

Most DeltaBlue methods have exact signatures; one constructor has a default.
Richards has no defaulted signatures. nbody's advance/report_energy/
offset_momentum functions use defaults, but their call frequency relative to
loop work has not been measured. Do not assign their entire elapsed regression
to the new guard or outlining without weighted profiles.

## Bounded fix proposal: retain inline frame commit

After root completes frozen41 repeats/PMU, test one source change only:

```
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
_PyFrame_PushOverlapping(...)
```

The change is the existing helper's declaration in
`Include/internal/pycore_interpframe.h`. Use the project's `Py_ALWAYS_INLINE`
policy rather than a raw compiler attribute or copied frame-construction body.
It is already used for `complete_step_impl` in ceval.c and other hot
value primitives. The macro is a no-op under Py_DEBUG specifically to avoid
large debug stack growth with optimization disabled; that behavior remains.
On the tested GCC release build it requests `always_inline`. Other supported
compilers retain the macro's established semantics.

This preserves one frame helper, one owning ABI, initialized storage, default-
shape guards and publication order. It introduces no runtime switch or alternate
binder. Exact-argument call sites already inline this helper; the experiment
restores that policy at generic/direct overlap and admits the new keyword site
under the same rule.

The tradeoff is increased native text at those three sites. Based on removed
inline bodies, expect roughly kilobyte-scale growth after eliminating the
standalone helper, but inspect the actual output rather than treating that as
a size prediction. The enlarged keyword body and its stack canary may remain.
`pyport.h` expressly warns that forcing inline can worsen performance; this
is a falsifiable compiled-code experiment, not a blanket policy recommendation.

Root's small-first decision ladder:

1. Independent proposal review, then real release/debug builds. Inspect actual
   disassembly to confirm the three calls disappear, frame guards remain, and
   no unexpected new spill/stack/canary pattern replaces the saved calls.
2. Run focused frame/call/default/keyword/owner tests and the fixed smoke set.
   No generated changes or new behavioral tests are needed for this annotation.
3. Compare balanced logging_silent and keyword micro timings and instructions
   against frozen41 and frozen39. Repeat coroutine/DeltaBlue as regression
   controls; do not predict coroutine improvement from this mechanism.
4. Keep only if application evidence improves without material new regressions;
   inspect aggregate text and hot instruction footprints. Otherwise revert the
   experiment and use PMU attribution before another change.

Do not fold in keyword scratch redesign, default-cache changes or layout padding
in the same experiment. Those would obscure which mechanism was tested.

## Experiment prepared

After root closed frozen41's six-pair repeats, 18 stat captures, six profiles and
three-role instruction campaign, the independent
[inline-policy review](inline-policy-review41.md) approved this bounded test.
The worker applied only `Py_ALWAYS_INLINE` to the shared overlapping helper's
declaration. Its body, callers, guards and ABI remain unchanged. No regeneration,
build, native run or commit was performed by this lane. Root owns the successor
identity, release/debug compilation, emitted-code inspection and paired
measurements; all experiment outcomes remain pending.
