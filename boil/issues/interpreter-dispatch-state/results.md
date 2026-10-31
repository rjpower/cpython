# Research results

Final research record for 2026-10-31 23:14:20–23:44:20 UTC. Source and evidence
reads only; no compiler experiment, runtime edit, build, target or timing exposure has occurred in this
research scope. The proposed design is held for independent review, not accepted
for implementation.

## Bound baseline and negative

Primary HEAD observed at 23:22:45 UTC:
`ff874a9048564748d6e428c3ec74acc62dae90fc`. The tracked runtime/generator paths
Include, Python, Objects, Modules and Tools/cases_generator have no diff against
the accepted native parent e9e10d5. Later primary prose commits do not change
that source binding. The rejected operand source remains a84965e, with its
native image and all failures/results retained.
The same runtime-path bridge was rechecked at 23:35:35 UTC against primary
HEAD `5163eadffafeaa82fdb8d37e9ef65bfa239e6630`, with no changed paths.

The author evidence capsule from the completed quiet campaign is
`build/constant-operands-quiet-v1/author-analysis/author-evidence.json`, SHA256
`1b940d06b4b735d7131b9894a6bd4b2df091a2c740cb8575cd94e8ca38151748`.
It binds twelve bounded native control captures and six matching generated C
body hashes. Root independently reproduced the captures. Coroutine
candidate/parent median ratios were 1.04465 and 1.11638 across the two rounds;
generators and unpack_sequence regressed in both. Independent outcome review
quiet-outcome-20261031-v1.md rejects that increment on actual elapsed results,
not the historical size proxy. Nothing in this issue retunes or reopens it.

## What the existing images show

The six normalized generated bodies are unchanged: UNPACK_SEQUENCE_TUPLE/LIST,
FOR_ITER_RANGE, YIELD_VALUE, LOAD_FAST_BORROW and STORE_FAST. In the parent,
sampled entries keep instruction pointer in rbx and frame in r14; slots is
loaded from a C stack slot. The rejected candidate reloads IP, frame and slots
from rbp-relative storage and stores next-IP state again. These are observable
extra memory operations at common entries. They do not prove a particular GCC
allocation pass, executed handler frequencies, cycle cost or cache cause.

Concrete anchors include parent/candidate tuple-unpack entries 0x457c9e and
0x4626be, and LOAD_FAST_BORROW entries 0x45ddca and 0x457856. The candidate's
simple load handler forms next IP with a late LEA before its stack store; an
early/late source increment rewrite is therefore not known to remove another
live pointer. Full function control flow and compiler allocation diagnostics
have not been reconstructed.

## Existing policies already address pressure

- `configure.ac:8075` adds GCC `-fno-gcse` for computed gotos specifically to
  avoid cross-handler shared-value lifetimes spilling frame/IP/slots. Both
  saved Makefiles include it, so proposing to enable it again is invalid.
- `ceval.c:1180` applies no-tree-slp-vectorize and no-omit-frame-pointer to the
  GCC x86-64 evaluator. `_PyEval_EvalFrameDefault` is also hot, noinline and
  Py_GC_PRECISE. The precise marker selects a section; it is not a command to
  spill these state variables.
- `ceval_macros.h:162` already reads fields through narrow memory loads on the
  little-endian GIL computed-goto backend, explicitly avoiding a long-lived
  decoded word. The existing native dispatch loads a 16-bit handler. Tail and
  free-threaded decoding keep their separate existing policy.
- The actual image producer is GCC Ubuntu 15.2.0-16ubuntu1. Its saved compile
  policy is GCC O3, NDEBUG, frame pointers, zero auto-variable initialization,
  no PGO/LTO configuration, ordinary computed gotos and no tail backend.
  Both configurations and compiler/tool hashes matched before timing.

## C state and authority

`ceval.c:1275` declares next_instr and slots as function-wide cached values;
frame is the mutable evaluator argument. The searched evaluator/generated
sources do not take the address of these scalar state variables. This does not
identify why GCC allocated them as it did, but there is no source requirement
that their storage be those stack slots.

`_PyFrame_Slots` at pycore_interpframe.h:182 returns only frame->base. The IR
generator says only a frame switch changes its cached slots pointer and emits
the canonical assignment in IRStack.emit_fullreload at line 1471. The maintained
generated file has 26 such assignments. `IR_SLOT`, GETLOCAL and LOCALS_ARRAY
all use the same cache. Error/unwind/start-frame labels and initial throw entry
also participate; a handler-entry-only patch that overlooks them is incomplete.

Generator lines 1814–1828 place the direct PREDICTED label after the ordinary
prologue for target heads. New local derivation must come after that join;
otherwise a legal specialized miss could skip its initialization. Instrumented
wrappers and frame switches likewise require actual entry/reload reasoning,
not a count of handler names. Existing per-uop effect/root analysis remains the
authority; an alias does not change the anchored values held in frame storage.

The collector skips precise C frames and requires values across collecting
calls to be anchored in traced storage. The current frame links and slot roots,
publication, clears and early return/yield roots must remain intact. No root
scan relaxation or new noncollecting declaration follows from this research.

## Actual accepted-baseline work

Read-only native extraction found three normal frame-resumption exits whose
captured continuations have an outgoing base-cache load/store and no subsequent
use of the alias before dispatch. These are maintained costs, independent of the rejected candidate's
additional common-entry spills.

| Handler | Frame base load | Outgoing cache store | Relevant ordering |
| --- | --- | --- | --- |
| YIELD_VALUE | 0x457acb, `mov 0x10(%rdx),%rax` | 0x457ad2, `mov %rax,-0x628(%rbp)` | After publishing the yielded value to the caller and switching frames; dispatch at 0x457af0 |
| RETURN_VALUE | 0x452bf3, `mov 0x10(%r12),%rax` | 0x452bf8, `mov %rax,-0x628(%rbp)` | After early result publication and FrameClearAndPop at 0x452bee; dispatch at 0x452c19 |
| RETURN_GENERATOR | 0x45c95c, `mov 0x10(%r13),%rax` | 0x45c96f, `mov %rax,-0x628(%rbp)` | After returning the generator to the previous frame; remaining value-tag checks do not consume slots before dispatch |

Local derivation could eliminate the outgoing transport pair; the next
handler's existing C-stack base load would become a load of frame->base.
That may change dependencies or other allocations, so this is useful work to
test, not a proved speedup. Early result publication, frame clearing and root
ordering are not candidates for removal.

The new CALL_PY_EXACT_ARGS and SEND_GEN captures stop at branches leading to
their fast paths. They bind actual entry code, but do not establish the
frame-entry transport cost beyond those branches. No complete evaluator CFG
or dynamic frequency is claimed.

## Source entry and reload inventory

The host-only inventory is
`build/interpreter-dispatch-state/source-native-inventory.json`, SHA256
`368aed5fdaf8ab510d8a32edff540df1da81aeb65532986a361a6ce9fc710d0f`.
It binds the accepted image SHA256
`289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`,
generated source SHA256
`f27a64827c0b071996f573b981348099f313b7bc3b6869c66c54d7c8c19fa939`,
four bounded objdump captures with literal argv/hashes, 287 TARGET bodies,
19 predicted joins, five common labels and all 26 canonical reload sites.
YIELD_VALUE is retained in the earlier author-analysis capsule above; the
other two proved exits are newly captured in this research directory.

The exact reload inventory in Python/generated_ir_cases.c.h is:

| Group | Sites and source lines |
| --- | --- |
| Frame entry (13) | BINARY_OP_SUBSCR_GETITEM 2640; CALL_ALLOC_AND_ENTER_INIT 4171; CALL_BOUND_METHOD_EXACT_ARGS 4330; CALL_BOUND_METHOD_GENERAL 4475; CALL_EX_PY 5177; CALL_KW_BOUND_METHOD 6128; CALL_KW_PY 6332; CALL_PY_EXACT_ARGS 7203; CALL_PY_GENERAL 7305; FOR_ITER_GEN 9153; LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN 12708; LOAD_ATTR_PROPERTY 13112; SEND_GEN 16895 |
| Plain resumption (3) | RETURN_GENERATOR 16550; RETURN_VALUE 16587; YIELD_VALUE 18910 |
| Instrumented (6) | INSTRUMENTED_LINE 11163; INSTRUMENTED_RESUME 11939; INSTRUMENTED_RETURN_VALUE 12012; INSTRUMENTED_YIELD_VALUE 12072, 12077, 12113 |
| Common labels (4) | exception_unwind 18984, 19001; exit_unwind_notrace 19034; start_frame 19057 |

These are reload sites, not 26 dead stores. In particular, exception_unwind
uses the first refreshed alias to write lasti and the raised exception.
The design preserves every reload and derives the local alias after each
legal entry join. The normal prologue before predicted labels does not use
slots. The five actual generated labels cover initial throw, recursion entry,
exception self-jumps, error and unwind. The bytecodes source's unused dummy
resume labels are not additional generated entries. Switch dispatch remains
in scope; the tail backend's existing passed slots parameter is unchanged.

## Disposition

Investigate removing one dispatcher-persistent derived slot-base cache in favor
of handler/label-local derivation, with existing reload boundaries preserved.
This is a small source hypothesis supported by actual baseline transport work,
not a promise of stable GCC registers. It is ready for independent design
review and a separately authorized bounded prototype. Compiler-only allocation
evidence is optional if a concrete uncertainty requires it, not a prerequisite
for testing the explicit work removal. Native mechanism evidence and actual
elapsed acceptance remain separate. No source, compiler or runtime grant is
implied by these documents. H1 remains a separate deferred Clang-tail question.

No optimization flags, source representation, instruction domain, arithmetic
semantics, benchmark counts or runtime products changed in this investigation.
The old thirteen-handler increment remains rejected on measured elapsed.

Source hashes read in this scope:

| File | SHA256 |
| --- | --- |
| Python/ceval.c | 0acac231ff96a2ff5637b5185ae2c35374cb38e0eb9ec494524f9e3da2a953f5 |
| Python/ceval_macros.h | 3fed8e8a1b7ae9a8cfa34e0a61e82debc808062ac949b3a5a59fa7ef29789539 |
| Tools/cases_generator/ir_generator.py | 0a24d1cd83b7dbeac95abdfe018ed93b9bfd6198f19be136c0b890fddeb93cdd |
| Include/internal/pycore_interpframe.h | 357d605743eb9de68028ee6ebcf9634ebed915c11580d86ed889c7f9e76fd37a |
| Include/internal/pycore_gc_walk.h | 64c72b7406c2690a2a5d819c62505370c4df50fc3f5e7accb1dcdc5ca5a75caa |
| configure.ac | d1c7c7f06bb550513bf92e48b0c66250f690bc9cff2cbc2e753fe345a6d6414e |
