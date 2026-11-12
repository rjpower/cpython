# Ordinary calls and return: selected-image audit

Read-only audit, 2026-11-12. R is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/round51-golden`,
verified HEAD `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`; U is the sibling
`upstream-3a440b6`. Native inspection uses the selected
`BASE/round51-golden-release/python` and upstream
`BASE/upstream-release-devlibs-3a440b6/python`, where BASE is their parent build
directory. Canonical checkout C is not evidence for this audit. Repository
instructions, canonical startup/process documents and the
[official AI policy](https://devguide.python.org/getting-started/ai-tools/)
were read. No interpreter execution, native job, build or benchmark was run.

## Finding

The remaining gap is not a generic-object call layered over a register call.
Exact Python calls already transfer directly into final owning homes and return
canonical values directly. The selected runtime pays for a richer activation
record, value classification and a general recipient protocol; it also saves
upstream's argument copying and repeated local initialization. Those opposing
effects need weighted native evidence. The source does not establish that calls
alone explain whole-workload instruction excess.

For generators, the existing selected profile ranks repeated SEND/YIELD above
final cleanup. Coroutine Fibonacci completes recursive awaits without yielding,
so a yield-only change misses that workload. Round52 demonstrated this limit
and lost performance despite successfully forwarding owners. See
[round51 report](../interpreter-round51/report.md) and
[round52 report](../generator-delegation52/report.md).

## Count the work that actually happens

| Transition | Selected golden51 | Upstream control |
| --- | --- | --- |
| Exact admission | PEP523, callable pointer-space/type/version, current code argument count, capacity, recursion | Same semantic checks except general PyValue pointer-space classification |
| Storage | One reservation; warm initialized frontier needs no NULL fill; overlapping thread arguments stay in place | One frame push; explicit arguments copied; locals after arguments NULL-filled |
| Header | 17 named field initializations plus reservation's saved_top; code owner acquired once, function owner transferred once | 12 named field initializations; code owner acquired once, function owner transferred once |
| Arguments | Thread caller: zero argument copies and zero argument refcount changes; callable source NULLed. Heap caller: cold raw move/NULL-source helper | One self-slot write and `oparg` explicit argument writes, no argument DUP |
| Constants | One fill per actual frame construction, proportional to co_nconstslots; inline/immortal values generally count-free | No hidden constant homes |
| Ordinary return | Temp owner moved and NULLed; local result DUP; unlink/clear/pop; direct recipient publication, possible discard/replacement retirement | Result stack owner made heap-safe; unlink/clear/pop; push result into caller stack |
| Generator completion | Same canonical return owner, then cold SEND/FOR semantic completion | Stack return followed by caller's stack-oriented completion protocol |

Counts above are source assignments in the ordinary GIL release configuration,
not instruction or memory-transaction counts. The compiler combines header
stores. The selected header is 14 eight-byte words (native subtraction 0x70),
while upstream's pre-locals header layout is 10 words: compare
`R/Include/internal/pycore_interpframe_structs.h:29–73` and
`U/Include/internal/pycore_interpframe_structs.h:29–55`.

Exact selected body: `R/Python/generated_ir_cases.c.h:7190–7336`.
Reservation/frontier: `R/Include/internal/pycore_interpframe.h:481–498,578–618`.
Initializer and constant fill: same file `:282–332`; executable bases `:58–76`.
Upstream body: `U/Python/generated_cases.c.h:4611–4730`; initializer
`U/Include/internal/pycore_interpframe.h:189–222`.

The 17 selected fields include previous/base, function/code, globals/builtins,
locals/frame object, instr_ptr/ir_words/ir_feedback, default return_pc/return_slot,
pending_kwargs/pending_redirect, owner/visited. saved_top belongs to reservation.
Repeated source conversions of callable/code are mostly CSE candidates, not
independent costs to add together.

`_PyValue_RecheckWindow` is disabled in this release
(`R/Include/internal/pycore_value.h:639–650`). Py_DEBUG, Py_STATS and WITH_DTRACE
are undefined in both pyconfig.h files. `_PyFrame_AssertTemporariesClear` and
home assertions generate no release scan. The exact handler's ir_displaced is
constant NULL, so its apparent finish-displaced call is dead. PyValue_Wrap is
not heap allocation. There is no exact-hit generic transaction, completion
metadata allocation, temporary object-box array, or result normalization.

Actual native selected exact CALL starts at `0x37a660`, has 0x50 C-stack
reservation, and constructs its header at `0x37a7fb–0x37a88b`. It loads the
execution image and writes words/feedback/default continuation state. The
initialized-thread hit contains no helper call. Calls at `0x37a926` and
`0x37a954` are behind heap-caller and uncertain-frontier branches respectively.
Constant fill is conditional at `0x37a885`; no-constant calls skip its loop.
Code-owner immortality/tag/refcount handling is real at `0x37a7ff–0x37a82c`.
The same owner lifetime exists upstream; removing it is unsound because a
function's mutable __code__ is not its executing frame's lifetime anchor.

The retained option3 review counted 149 static instructions and 18 rbp-addressed
operations on one warm path, versus 150/22 before the change, despite shrinking
the handler from 1410 to 973 bytes. Do not equate code shrink with saved dynamic
work. See [option3 post-review](../interpreter-round51/option3-post.md).
That count belongs to option3's retained native image; this audit confirms the
same mechanisms in selected51 without claiming a fresh selected-path count.

## Return and retirement

`R/Python/generated_ir_cases.c.h:16635–16682` and
`R/Python/ceval_macros.h:499–538` show the local/temporary distinction: return
from a local duplicates its owner; return from a temporary moves it. The result
stays C-owned across callback-capable frame retirement. Ordinary return does
not invoke `_PyEval_CompleteGeneratorReturn` or `_PyValue_Publish`.

Native RETURN at `0x381390` confirms: local index classification at
`0x3813be`, temporary NULL at `0x3813c7`, mortal-local increment at
`0x381494`; one common clear/pop call at `0x3813e9`. Ordinary completion checks
pending redirect at `0x3813fc–0x38140e`, discard at `0x381418`, reads/replaces
the actual recipient at `0x381422–0x381426`, then tests/retires the displaced
owner. It has no result-boxing call. Generator completion alone calls the cold
adapter at `0x3814ed`.

`R/Python/frame.c:181–235` clears only co_nlocalsplus for thread frames,
NULL-before-close in reverse order. It does not scan empty register scratch on
ordinary return. Exposed frames can take ownership before this loop. Generator
frames use the full physical-home clear, but the attempt to reuse normal-return
empty-scratch proof there was already rejected on DeltaBlue performance
(round51 option5). Do not revive it.

The cold adapter decodes the immutable caller instruction's A/B/signed-D and
SEND/FOR kind (`R/Python/ceval.c:3095–3117`). `complete_step_impl` inlines with
canonical_input=true, so its source-level Publish arm is not executed. It
detaches receiver/cursor/sent homes, publishes the result/continuation, checks
monitoring, then retires detached owners (`:2995–3080`). With no monitoring,
event machinery is not entered. Ordinary return's pending redirect is a genuine
per-result check; a general observer reconstruction transaction is not.

Upstream uses a collapsed computed-goto evaluator in this build: pyconfig.h
leaves `_Py_TAIL_CALL_INTERP` undefined, and nm finds EvalFrameDefault but no
TAIL_CALL handler symbols. Therefore no upstream per-handler native comparison
is claimed from `objdump --disassemble=_TAIL_CALL_RETURN_VALUE` (which is empty).
Upstream source return is `U/Python/generated_cases.c.h:11474–11520`.

## Ranked contract candidates

These are proposals for review and weighting, not measured wins. None justifies
another exact-setup deduplication or a repeated round52 wrapper-only move.

1. **Final-home activation construction through the shared binder.** Exact,
   known-plan and generic Python binding should share an admission/ownership
   transaction whose final owner can be thread or generator storage. For a
   proved eligible generator/coroutine entry, initialize that final heap frame
   once rather than reserving/initializing a transient thread frame, executing
   RETURN_GENERATOR, copying every physical home and NULLing its source, then
   popping the transient frame. R/Python/bytecodes.c:5256–5282 and
   R/Include/internal/pycore_interpframe.h:240–279 establish that work. The
   expected removal is the transient header, frontier/reservation/pop and
   full physical-home move, not the generator allocation or required owner
   creation. A generator expression may execute GET_ITER before
   RETURN_GENERATOR and retain scratch: final-home construction must either
   run that verified prefix with correct callback/current-frame semantics or
   decline before effects. Also preserve mutable code pinning, provider and
   PEP523 behavior, argument-error timing, instrumentation, allocation-failure
   cleanup and coroutine origin tracking. Test defaults/keywords/variadics,
   pre-entry iteration errors/reentry, zero-argument constants, escaped frames,
   throw/close before first send, and retained coroutine origins. This is a
   shared activation ABI change, not a new call opcode family. It has the
   largest concrete removable work here; eligibility and dynamic counts are
   still unproved.

2. **One semantic continuation contract across call and resume completion.**
   Ordinary CALL already has real recipient/continuation state. Extend its
   producer-known facts coherently to resumed SEND/FOR completion so that return
   does not rediscover operation kind, window, destination and signed exit from
   the immutable caller word. Retain actual canonical homes and observer sites,
   and share one commit/retire protocol. A compact tagged recipient/continuation
   may carry the facts, but stores added on every resume must be weighed against
   decode removed only on completion. A pointer to another metadata record is
   not automatically cheaper. This must cover recursive completion as well as
   yield; round52 forwarding alone already failed. Test FOR exhaustion versus
   SEND result, discard/local/temporary destinations, nested await completion,
   close/throw, local mutation during child retirement, pending redirects,
   tracing and monitoring. Native proof must show removal of actual decode and
   repeated owner handling without duplicating generic completion bodies.
   The following narrower recipient-class optimization fits this contract:
   caller producers know guaranteed-empty temps, so those may omit the current
   displaced-home load/retirement tests. Existing uint16 fields have room beyond
   the 12-bit slot domain, but all consumers need an explicit encoding change.
   Redirects must refresh or invalidate the fact. Another class branch can cost
   more than today's NULL branch; no win is assumed.

3. **Terminal local-owner transfer for unexposed frames.** At the shared return
   boundary, a proven unexposed dying frame could detach a returned named-local
   owner instead of DUP followed by retirement of that same local. Preserve the
   existing path whenever a frame object/observer handle can retain its locals.
   This removes a mortal increment/decrement pair and related classification;
   it does not eliminate the local clearing scan. The admission proof must cover
   tracing/monitoring return callbacks before detachment, frame proxies, frame
   takeover, GC and finalizer reentry. Test returned locals also owned by a
   closure/container, escaped f_locals, self-cycles, finalizers inspecting caller
   chains, exception-preserving cleanup and generator/coroutine return. Static
   selected evidence establishes the duplicate pair; event frequency and the
   extra exposure guard's cost remain unknown. Reject if a simple ownership
   proof cannot exclude all observable-frame cases.

Fresh selected51 coroutine sampling supplied by the coordinator places exact
CALL at 9.28%, RETURN_GENERATOR at 7.62%, CompleteGeneratorReturn at 6.45%,
ClearLocals at 5.58%, SEND_GEN at 4.86% and RETURN_VALUE at 4.47%, with zero lost
samples. Source: BASE/pyjit-link-wave1/activation53/baseline-coroutines-perf/
record-r1-candidate-coroutines-cycles-record-symbols.txt, inspected in this audit.
These shares support examining construction and completion together for
coroutines; they are not removable fractions. The coordinator's phase census
must weight creation, descent/resume, yield and completion before choosing
between the first two contracts. The third is a bounded secondary hypothesis.
The coordinator owns implementation, semantic review, release/debug execution,
matched timings and integration. This audit changed only this document.
