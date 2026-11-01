# Word-selected first native tier: seven integer sites

2026-11-01. **Design amendment only.** Preserve `design.md`, the direct Fable
review and `coordinator-disposition-20261101.md` unchanged. This selects one
candidate before implementation or elapsed scores; it grants neither. The
qualified source for the observation is RC `dce654018d5ae16ffe5e772976d9ccc2c9d1e01a`.
Outstanding RC qualification failures remain separate landing blockers.

## Observed candidate and hypothesis

The accepted G0 v2 observation captured the actual registered
`spectral_norm.eval_A`, before and after exactly 64 varying calls. It found
14 words, two argument locals, no cells/freevars or exception ranges, and the
same pristine bytes in both snapshots. Afterward the seven integer sites have
integer-specialized executing handlers. This is a final-state observation,
not a native entry count, execution-frequency census or speed result.

Select the entry region **words 1 through 11 inclusive**, continuing at word
12. Use pristine words, full handler IDs, generated operand roles, borrow masks
and feedback arguments; never recognize a function name or benchmark filename.
The exact nominated home-version graph is:

| Word | Actual operation | Physical home | Region version |
| --- | --- | --- | --- |
| 0 | RESUME, outside region | — | existing entry protocol |
| 1 | inline float literal 1.0 | 2 | constant bits `one_float` |
| 2 | `i + j`, folded bound locals 0/1 | 3 | `s = checked_add(i,j)` |
| 3 | `i + j`, same folded locals | 4 | second use of `s` after exact guards |
| 4 | inline integer literal 1 | 5 | constant 1 |
| 5 | home4 + home5 | 4 | `t = checked_add(s,1)` |
| 6 | home3 * home4 | 3 | `p = checked_mul(s,t)` |
| 7 | inline integer literal 2 | 4 | constant 2 |
| 8 | home3 // home4 | 3 | require `p >= 0`; `q = p >> 1` |
| 9 | home3 + folded local0 | 3 | `r = checked_add(q,i)` |
| 10 | inline integer literal 1 | 4 | constant 1 |
| 11 | home3 + home4 | 3 | `d = checked_add(r,1)` |
| 12 | float true division home2 / home3 | 2 | existing interpreter |
| 13 | RETURN home2 | — | existing interpreter |

This is **seven integer source sites**: five ADD, one MULTIPLY, one FLOOR_DIVIDE.
Only after proving both inputs exact compact ints and that no effect/observer
occurs between the duplicate additions may local value numbering share `i+j`.
The resulting leaf has six arithmetic operations: four checked additions,
one checked multiply and one nonnegative shift. Literal materialization is
transport, not floating-point arithmetic. No native true division, Python call,
return or heap operation is added. The selected consumer replaces eleven
interpreter words with admission, one leaf call and two live-out stores; the
net cost is unknown. Source-derived registered exposure is 676,000 `eval_A`
calls per workload loop, **not** 676,000 successful native entries.

The hypothesis is that keeping these intermediates in machine registers and
sharing the duplicate pure addition saves enough dispatch/home traffic to pay
the universal entry tax and transaction cost. There is one selected seven-site
candidate. A loss does not authorize a four-site rescue, more operations,
another consumer, an entry-frequency forecast or additional timing samples.

## Entry and value-home contract

Use one shared admission decision in both `_PUSH_FRAME` and `start_frame`, after
their existing recursion/RESUME/notification work, including DTrace and debug
lltrace setup. `_PUSH_FRAME` and `start_frame` have different notification
ordering; retain each ordering rather than moving notifications. Require the
ordinary initial entry state, `next_instr == frame->ir_words + 1`, the correct
executing block, and the unchanged pristine descriptor. A call for which
RESUME was not skipped simply declines on this path; the interpreter services
it. Generator/send/resume/throw entries, PEP 523 replacements and noninitial
entries are excluded. Do not add a pre-notification hook in `IR_SKIP_RESUME`.

Admission requires Linux/x86-64/GIL, enabled inline values, no tracing function,
profiling function or active tracing depth, no active global/local monitoring
events, no code instrumentation, no DTrace/lltrace observer, and no pending
eval-breaker work. Check actual observer state, not merely instrumentation
version equality. Invalidate/disable the descriptor at instrumentation changes
and repeat guards after any cold compilation/publication work. A conservative
decline for retained instrumentation state is acceptable and must be reported.
Pystats/instruction instrumentation builds are outside the first native fast
path; do not silently omit their required instruction events or counters.

Live-in slots 0/1 must be checked **non-NULL before** calling
`PyStackRef_IsCompactInt()`, which otherwise can dereference pointer-space NULL.
Accept existing inline int32 values or exact boxed compact ints through
`PyStackRef_CompactIntValue()`. Reject bools, int subclasses, floats, large ints,
unbound values and other objects without conversion, `__index__`, arithmetic,
boxing, ownership changes or a new exception. Only folded, proven-bound local
reads qualify; separate `LOAD_FAST_CHECK` remains unsupported. Borrowed scalar
extraction leaves the original local references/IDs and their owners unchanged.

The capture's `co_framesize == 19` is **not nineteen IR homes**.
`_PyFrame_NumSlotsForCodeObject()` subtracts `FRAME_SPECIALS_SIZE`; the selected
x86-64/GIL struct layout contributes thirteen header words, leaving six value
homes 0–5. Use that source helper, not a hardcoded subtraction in the compiler.
`_PyFrame_Initialize(..., null_locals_from, ...)` NULLs the rest of the actual
slot range, including fresh temporaries 2–5. Descriptor validation must verify
all home indices against the actual slot count, locals boundary and operand
roles. The entry wrapper also checks that the four overwritten temporary homes
are NULL before execution; unexpected owned/inline contents decline intact.
These checks are part of the measured admission cost, not nineteen-home scans.

Track local versions of finalized homes and source consumption, not physical
home numbers as globally unique values. This graph contains no local stores,
branches, joins, exception range or owning pointer constant. Word12 reads only
homes2/3. At successful region exit, homes4/5 are dead and remain their initial
NULL values; no clearing stores are needed. Locals0/1 retain their exact bits,
reference ownership and identity. The compiler must prove this live-out map
from words/roles/continuation, rather than infer it from expression spelling.

## Pure transaction, rollback and arithmetic

The wrapper extracts two `int32_t` scalars and supplies caller-owned scalar
scratch. The leaf receives scalar-input and scratch/result pointers, **no
frame, PyObject or borrowed-reference pointer**. It makes no calls, allocation,
INCREF/DECREF, Python error creation, observer callback or frame/IP write.
Only private scratch may change before success. It returns SUCCESS or DECLINE;
an output is usable only on SUCCESS. It cannot raise a Python exception.

Every addition and multiply checks signed int32 overflow before its result is
used. C staging uses checked builtins without signed-C overflow; native staging
uses 32-bit instructions and immediate overflow exits. Division lowering is
only by the proven inline positive literal **2**, with a nonnegative dividend
guard before logical right shift. Arbitrary divisors, negative dividends or
unsupported forms decline; they never use C truncation as Python floor.
No division-by-zero or INT32_MIN/-1 instruction is emitted. If any check fails,
leave all homes, local owners, IP, pending exception state and continuation
unchanged, and interpret again from word1. Repeating guarded exact arithmetic
has no Python-visible effect. CSE is forbidden before those exact-type/effect
proofs: custom `__add__` must execute twice in its original order on fallback.

On success, the C wrapper writes exactly two live-outs: the validated inline
1.0 bits to home2 and encoded int32 `d` to home3. Then publish the interpreter
continuation at word12 using its normal frame/dispatch convention, and dispatch
there once. No owner is overwritten or released. Homes4/5 stay NULL. Word12
handles all floating arithmetic, including a zero denominator and its error IP;
word13 handles ownership/return. Do not fold true division or special-case the
observed 0–7 argument domain. Arguments vary at runtime.

The existing frame/IP stay valid at entry throughout this bounded, no-safepoint
interval. There is no per-word IP traffic inside the leaf and no claim of
per-word asynchronous profiler visibility. A signal may set the breaker while
it runs; this finite straight-line region cannot loop or reenter Python, and
normal interpreter breaker service remains in place afterward. This contract
does not extend to calls, loops, native helpers, yield or ownership releases.

## Implementation stages and concrete cost accounting

First implement and review the scalar **C leaf** contract with the shared
entry, descriptor and commit path. Then replace that leaf with native code and
repeat the same tests. These are source checkpoints of one implementation,
not selectable C/native runtime modes. Build the descriptor from pristine words
and generated formats; do not depend on warm specialized handler IDs. Keep the
existing bounded scope (at most sixteen words and a finite arithmetic/literal
menu); the selected additions are local value numbering of the proven duplicate
and this literal-two floor rule. Do not persist a global SSA graph or add loops.

For the SysV leaf, reserve RDI/RSI for input/workspace pointers and EAX for
status; allocate scalar versions among ECX/EDX/R8D–R11D. Never touch RSP/RBP or
callee-saved registers. A small local linear scan may reuse dead scalar values;
bounded spills go only to a declared caller-owned int32 scratch area, with
checked capacity. IR home2 is a frame destination, not a machine GPR. Carry its
validated float literal bits in the descriptor/wrapper; do not introduce an XMM
float calculation. Retain the complete encoding/disassembly, loads, register
moves, spills, guards, exits, entry and two-store commit before a speed claim.

One descriptor/hot counter in `_PyCodeExec`, the proposed64 eligible-entry cold
threshold and `_PyCode_ClearExec` ownership remain the starting policy. Cold
descriptor/emitter/mapping failure must leave the interpreter usable, avoid
partial installation, and preserve any pre-existing exception; it is not an
excuse to clear arbitrary errors. Add no runtime enable switch. Emission needs
real checked return/load/store/branch forms, checked relocations and capacities,
then a bounded W→RX owner with no RWX window or stale executable address. Failure
cleanup and code teardown are correctness gates before native execution.

Actual `_Py_JIT` publication is currently compiled away and the existing CFI
describes other frame layouts. As disposed after Fable, a first no-call,
RSP-preserving experiment may defer profiler publication/leaf CFI, explicitly
reporting absent coverage. Correct registration/unregistration and leaf unwind,
plus broader lifetime qualification, are later landing obligations after useful
elapsed. Do not install old stencil/trampoline frame rules or claim anonymous
pages already have profiler support.

## Finite tests and unchanged elapsed decision

Before any native grant, freeze literal cases against an independent operator
call oracle that cannot compile this region. C-leaf and native stages must both
cover:

- Actual graph, exact home/version/live-out map, inline arguments and fresh boxed
  compact arguments, changing inputs and a consumed checksum. Test the full
  registered0–129 argument domain behavior separately from the64-call G0 sample.
- NULL, bool, subclass, noncompact and arbitrary-object rejection; duplicate
  `__add__`/reflected effects and exception ordering on fallback; no conversion
  or destructor during decline. Owned local arguments retain identity/refcounts.
- Overflow at each distinct add/multiply exit, zero/negative continuation
  denominators and forced failed-stage scratch writes, proving unchanged frame
  state before interpreter replay. Test the nonnegative-floor guard directly
  with scalar-leaf contract cases even if the nominated product proves it.
- Aliased/reused temporary versions, prefilled/owned writable-home rejection,
  exact two-live-out commit and dead4/5 NULL. Reject protected ranges, local
  stores requiring release and malformed word/role/home metadata.
- Trace/profile/monitor activation before and after compilation, cleared-but-
  retained instrumentation, entry-notification order, PEP523, pending breaker,
  generator/coroutine/send exclusion and code teardown. No assertion of native
  activity when an admission guard deliberately declines.
- Compile/map/protect/capacity failures, exact encoding/overflow exits, no RWX
  and owner lifetime. An actual native-path witness must show varying admitted
  inputs and returned results; C-leaf success is not native-execution evidence.

Seek a separately reviewed ninety-minute source ceiling with30/60-minute
contract/emitter checkpoints, freezing incomplete work if necessary; the
implementation grant may instead separate those stages. A subsequent finite
fresh debug/O3 feedback batch runs the exact reviewed tests before scoring.
No current sentence grants implementation, compilation, execution or timing.

Preserve the original fixed comparison: **two rounds, five values** for the
unchanged registered spectral_norm primary and coroutines/Richards/
unpack_sequence controls against the qualified RC parent, rotating order and
binding actual origins. Primary and four-workload aggregate elapsed ratios must
improve in each round and remain improvements under leave-one-value median
sensitivity; inconclusive earns no extra samples. Control losses over2% require
explicit disposition. Retain raw vectors, RSS, compilation counts/time and
whole-process startup/warmup cost alongside warmed work. Universal rejected-
entry checks are charged to controls. Hardware counters/text are diagnostics,
not prerequisites or proxy vetoes. One registered primary loss rejects this
increment before expensive broad qualification/publication closure. There is
no fallback to a shorter region after seeing those scores.

Evidence: accepted `word-capture-outcome-review-20261101-v2.md`
(SHA `5eea20af698b0b84736e5bdf1cf46ad228cd0da6c9b838642017218463e87469`,
receipt `build/jit-word-capture-outcome-review-20261101-v2/`),
actual `build/jit-first-native-tier-word-capture-20261101-v2/affected/`
`before.json`, `after.json`, `child.json` and maps; source anchors in selected
RC `pycore_stackref.h:1115`, `pycore_interpframe.h:139,274`,
`pycore_interpframe_structs.h`, `pycore_ir.h`, `Python/ir.c`,
`bytecodes.c:286,302,676,695,746,4386,5930`, and
`ceval_macros.h:625–664,740`. The original Fable review and coordinator
disposition remain the authority for the amended pure-transaction contract.
