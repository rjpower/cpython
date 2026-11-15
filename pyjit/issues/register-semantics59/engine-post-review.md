# Resident engine: actual native post-review

The captured engine implements the reviewed computed-goto source contract, but
does **not** keep the common interpreter state in registers across dispatch.
GCC also merged dispatch into one indirect branch. Eliminating the tail-call
ABI therefore does not establish a reduction in hot native work. No native
correctness blocker was found in this bounded review. Root subsequently
rejected this engine after the eight-case screen; selected57 remains unchanged.

This review compares candidate `ec230e55641f646681d4961583595236465efe62`
against selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`. The candidate
ELF SHA256 is
`ef23fed1a41afbd165e940c8c118e173994c9837d7c5fd7a86cc1936e2ef5794`;
the control is
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
Inputs are `E59/parity60-engine1-native/{manifest.json,engine-targets.json,
run_handlers.txt,_PyEval_EvalFrameLoop.txt,python}` and the retained selected57
ELF. Bounded excerpts, selected disassemblies and numerical readback are in
`E59/parity60-engine1-static-review/`. Only static ELF/text inspection was
performed here. Root reports both builds at 117/79 and 465 focused methods in
17 modules passing in each role. Root's completed screen had stable native
identities and no execution errors.

## Engine, table and hardening

`run_handlers` is one 336,799-byte function at `0x360940`, without a separate
`.cold` fragment. The actual `instruction_targets.5` table is 2,936 bytes:
367 rows, all resolved through supported RELATIVE relocations and checked
inside the engine. Its 330 distinct target addresses all begin with
`endbr64`. Repeated rows are not distinct functions. The selected function
pointer table also occupies 2,936 bytes. Table capacity has not shrunk.

The engine contains exactly **one indirect jump**, at `0x360a3a`. There are
1,131 static direct jumps to the shared dispatch block at `0x360a20`. These
are static references, not dynamic counts. That block reloads the current
PC, loads a 16-byte constant into `xmm0`, forms and spills PC+8, then jumps
through the selected label address. The native result is a shared dispatch
site, despite source-level computed gotos at many exits. CET landing pads
remain; this experiment did not remove that hardening cost. Neither branch
prediction benefit nor harm follows from these facts without measurement.

## Stack and text costs

The normal-ABI engine prologue saves `rbp` and five other registers, reserves
`0x1178` (4,472) stack bytes, probes the first 4 KiB, and installs a stack
canary (`0x360940–0x36097b`). It runs once per engine activation, not once
per IR word. The outer evaluator reserves another 184 bytes, excluding its
saved registers and return addresses. Reentrant native-to-Python execution
can enter another engine; ordinary inlined Python calls change its active
frame without adding an engine activation.

Selected57 already saves the five normal callee-saved registers once in its
52-byte `enter_handlers` shim. Its handlers use NCSR registers without saving
those registers individually. Representative handlers still push/pop `rbp`;
ADD_INT reserves 32 bytes and the exact no-self Python-call handler reserves
64. Thus the change removes repeated handler frame setup/teardown and ABI
handoff, not five register saves per word. The new fixed stack reservation,
stack probe and canary must be charged at activation entry.

| Actual text accounting | Selected57 bytes | Candidate bytes |
| --- | ---: | ---: |
| Handler functions / resident engine | 279,035 (332 functions) | 336,799 |
| Outer evaluator | 934 | 862 |
| Normal-to-handler entry shim | 52 | absent |
| Sum of the above symbols | 280,021 | 337,661 |
| Entire ELF `.text` section | 5,079,417 | 5,134,841 |

The symbol sum grows 57,640 bytes; the complete `.text` section grows
55,424 bytes (about 1.09%). The first sum excludes outlined helpers and
inter-function padding; the second includes all code. It is not valid to
report the primary engine symbol as the entire interpreter implementation.
Unwind sections shrink (`.eh_frame` 381,844→364,548;
`.eh_frame_hdr` 74,308→71,740), so aggregate ELF `size` text growth is a
different number, 35,272 bytes. Label-to-label distances are not handler
sizes: bodies, slow blocks and shared tails are scattered through the engine.

## Common state and representative paths

The engine's persistent state is repeatedly accessed through stack slots:
frame at `rbp-0x10c8`, thread state at `-0x10e8`, current PC at `-0x10d0`,
encoded word at `-0x10e0`, slots at `-0x1100`, and dispatch table at
`-0x10f0`. This is actual generated code, not an inference from C variable
names. Frame instruction-pointer publication remains in both versions;
it is not new backend overhead.

| Path | Actual candidate behavior versus selected57 |
| --- | --- |
| `LIST_APPEND` (`0x37dd90`) | Same take/duplicate policy and `_PyList_AppendTakeValue` call. Candidate reloads frame, slots and word from engine spills; selected has these in incoming registers. Candidate successful dispatch suffix has 12 instructions including the shared block, versus 10 in selected57; selected also pays its entry `push rbp`. These suffix counts are not whole-path totals. |
| `LOAD_COMMON_CONSTANT` (`0x38288a`) | Same interpreter constant lookup, conditional owner acquisition, destination classification and numeric normalization. Candidate reloads thread state and encoded word, and advances PC in memory. No constant rematerialization or owner pair was eliminated by the backend change. |
| `BINARY_OP_ADD_INT` (`0x36b03e`) | Same two inline-integer guards, arithmetic/overflow handling, publication and operand retirement. Candidate repeatedly loads word/frame/slots and spills successor PC; selected starts with word/slots/frame in ABI registers and uses a 32-byte handler frame. This change does not fuse words or remove type guards. |
| `LOAD_ATTR_SHAPE_LOCAL_TEMP` (`0x37bc16`) | Same exact type-version and receiver-shape checks, field read, conditional duplicate and canonical temporary publication. Candidate prefix/tail reloads frame/slots/word repeatedly. Its successful dispatch suffix has 14 instructions including the shared block, versus 8 in selected57; that comparison excludes the bodies and selected entry setup. |
| `FOR_ITER_LIST` (`0x364fd0`) | Same list/type guard, cursor/bounds test, out-of-line `PyList_GET_VALUE` (`0x392825`), item acquisition, cursor/item normalization and local/scratch publication. Those provider/normalization costs remain. The BODY path is physically separated from the entry and shares distant exits, so an adjacent label range would miss much of it. |
| Exact Python calls (`0x379798`, no-self `0x37500e`) | Existing function/version/arity/arena/recursion guards, argument overlap, header setup and continuation fields remain. The successful no-self transition writes child frame/slots/PC/word into engine spills (`0x375261–0x375300`) before shared dispatch; selected sets incoming handler registers and tail-jumps. There is no new argument-copy loop or general call helper in that fast transition. |
| `SEND_GEN` (`0x370a83`) | Same generator/coroutine state guards, caller continuation, exception linkage and incoming-value retirement. Candidate calls the now-outlined `_PyFrame_SetReturnValue` at `0x370bd9`; selected57 inlines that redirect/publication/displaced-owner body and has no outlined symbol. The helper is 207 bytes. Candidate then spills child frame/PC/word and uses shared dispatch. |
| `YIELD_VALUE` (`0x36690d`) | Same source take, suspension/exception unlink, return-slot recipient classification, displaced local retirement and post-redirect continuation reload. Its ordinary local publication remains inlined (`0x366a19–0x366aa5`); receiver frame/PC/word then flow through engine spills. No continuation-action experiment from rejected round59 is present. |

For the two suffix counts, the exact ranges are selected LIST_APPEND
`0x3540f8–0x354119` versus engine `0x37de2a–0x37de4d` plus
`0x360a20–0x360a3a`; selected SHAPE `0x378b2e–0x378b48` versus engine
`0x37bcfe–0x37bd2f` plus that same shared block. Each count includes its
final indirect jump. Candidate direct jumps to the common block count too.

## Weight and disposition

The earlier [all-word census](resident-coverage.md) makes these differences
worth measuring: comprehensions recorded 972,000 LIST_APPEND, 1,686,000
FOR_ITER_LIST and 1,260,000 SHAPE_LOCAL_TEMP visits; coroutines recorded
971,142 LOAD_COMMON_CONSTANT and 971,141 exact no-self Python-call visits;
spectral norm recorded 3,380,000 ADD_INT visits. These weights assign monitored
semantic visits to pre-monitor adaptive labels, not natural guard successes.
They cannot be multiplied by the suffix deltas to claim total native savings
or regressions. The generator kernel predominantly uses SEND_GEN and
YIELD_DELEGATED; the requested YIELD_VALUE example is not a substitute for
that delegated path's dynamic cost.

The backend conversion is real and its source ownership/error/publication
contracts are preserved in the inspected paths. The compiled result also
introduces common stack traffic, one merged indirect-dispatch site, greater
code size and a changed inlining decision on SEND. These are concrete costs
to compare with removed ABI moves and handler setup. They are not a causal
performance diagnosis, a promotion recommendation, or permission for a flag,
alignment or layout search. No runtime, tool, generated product or benchmark
was changed for this review.

Root's balanced two-pair screen rejected every case: candidate/selected57
elapsed ratios were comprehensions 1.218648, DeltaBlue 1.338757, coroutines
1.185821, generators 1.438227, logging 1.599627, Ray 1.300208, spectral norm
1.291620 and nqueens 1.302795. Qualification passed; performance did not.
There is no fresh full-suite or all-29 admission for this rejected engine.
The shared dispatch, stack traffic, code growth and outlining are native
observations consistent with costs the source proposal needed to charge;
this review does not quantify their contribution to any particular loss.
