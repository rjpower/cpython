# Residual attribution and selected child experiment

## Completed investigation

1. Read the qualified four-row disjoint analysis and raw perf-script stacks.
   Preserve their files and original interpretation.
2. Recompute residual innermost helper/physical symbol shares with the existing
   PID, hot bounds and period weights; inspect captured coroutine source lines
   and instructions offline.
3. Trace every `_PyFrame_Copy` caller and initialization/publication path.
   Compare other residual costs and arithmetic evidence without turning them
   into unmeasured optimization claims.
4. Record one selected implementation question and its source proof obligations.

No fresh profile, benchmark, test, build, configure or measured interpreter
process is needed for this decision. A new capture would need an explicit
unresolved question, actual capsule bindings and coordinated quiet admission.

## Proposed child: full-range frame transfer

The smallest candidate replaces the scalar payload move in the existing helper
with nonoverlapping copy and clearing operations over **all** `nslots`, retaining
the existing header assignments. Capture source and destination bases before
the operations; `PyValue_NULL` is currently the all-zero value. Do not remove
source clearing, change the returned slot, shorten the range or add allocation.
There is no size-based alternate protocol and no manual SIMD implementation.

First prove the disjointness of every current caller:

| Caller | Destination storage | Required proof |
| --- | --- | --- |
| RETURN_GENERATOR | Newly allocated generator's embedded slots | Distinct from transient thread frame/caller window; no collecting call during move |
| `take_ownership` | Escaped frame object's owned data | Source is thread/generator owned; destination is separate frame-object storage; preserve `f_back` and later OOM handling |
| `gen_new_with_qualname` | Newly allocated generator | Separate from source frame object's data; preserve its frame-object rebinding and ownership |

This changes implementation of a move, not its lifetime contract. The source
remains rooted while copying; destination visibility and CREATED/owner state
follow the existing order. A complete payload exists before any later collecting
call. Source/destination aliases within a logical frame remain distinct from
overlap of these two storage ranges. If arbitrary supported callers invalidate
this proof, stop and report the exact gap.

Minimum meaningful controls cover values held only in generator locals, captured
cells, a generator expression's eagerly acquired iterator, async generators and
coroutines, thread-owned overlapping calls versus generator-owned callers, and
escaped frames retained through return/exception. Exercise origin tracking and
collection during construction; retain the existing publication regression.
Existing test_ir resumer/instrumentation and exception controls should carry
their true coverage. A small omission control must demonstrate loss of payload
or stale-source retention at the boundary, without introducing an unsafe broad
fault campaign.

Use the existing source/checker and build recipes. Qualify applicable ordinary,
kill, sanitizer and linked backend obligations through the normal finite plan;
do not invent a new ladder here. After source/behavior review, inspect emitted
code before timing. If the compiler still emits the same scalar loop or the
bulk operations add overhead, reject that implementation. Only then propose a
small matched coroutine/generator elapsed and instruction comparison, with
Richards as a non-GC/control row, under the existing locks and quiet reservation.
Report code size and regressions; do not repeat the 29-row campaign to decide
this local question. No performance acceptance threshold is set in advance.

## Deferred arithmetic witness

The captured arithmetic witnesses execute invariant `x + y`, not `i += 1`.
Source and existing IR tests establish the recurrence's compact-int guards,
constant load, folded destination and overflow fallback, but this investigation
does not claim a freshly observed recurrence disassembly or elapsed result.
If arithmetic becomes the selected next question, use one bounded warmed IR/
disassembly witness on the qualified image before attributing its cost. Keep it
outside the 29-workload score and preserve int32 overflow and bool/subclass cases.
