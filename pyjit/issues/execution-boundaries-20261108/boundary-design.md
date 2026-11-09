# Logical boundary execution: bounded first increment

Proposed 2026-11-08 against frozen runtime `0765e9be47f`. Runtime untouched.
The coordinator admits retained logical coordinates for this first increment.
Physical compaction and removal of allocated anchor storage are deferred.

## First implementation: checked boolean edge continuations

The existing POP_JUMP_IF_FALSE_BOOL checks its selected successor word for NOP,
publishes that NOP's PC, then advances. Replace that traversal with three
execution-only variants: skip one NOP on the taken edge, on fallthrough, or on
both. Their handler identities encode a two-bit edge descriptor. Selection is
once during specialization, from immutable canonical successor coordinates and
the current executable handlers. There is no boundary handler, boundary table
load, NOP decode, or NOP PC publication during admitted ordinary execution.
The result is execution elimination, not physical word compaction.

Keep canonical words, source ranges, exception ranges, jump target coordinates,
monitoring positions and debugger stack reconstruction unchanged. An admitted
NOP remains a dormant logical event record at its original coordinate. Other
incoming edges retain their original semantics; skipping is a property of this
checked edge, not a global deletion of the target. At most one NOP is skipped,
as in the baseline. A NOP at the end without a following executable word is
ineligible. Forward and backward targets use the existing decoded target.

The three variants inherit the boolean guard and branch-history update. On
false/taken they set next_instr to target + the static taken skip; on
true/fallthrough they increment next_instr by the static fallthrough skip.
Inputs remain unchanged before a guard decline. A checked boolean is inline;
its retirement cannot call Python. The region between the branch guard and
successor dispatch has no callback, poll, allocation, error or frame switch.
Eligibility additionally requires the successor after the skipped NOP to
have a generated SAVES_IP capability, derived from the actual instruction
property used by emit_handler. This excludes no_save_ip END_FOR (which can
finalize), its instrumented form, and IR_JUMP chains. Check both immutable and
live successors; future handler changes automatically update the generated
capability. The admitted successor saves its own PC before observable work.

## Instrumentation invalidation and observation

A target can be patched independently of the branch. Before any instrumentation
update, scan the executing words and reset all three boundary variants to the
ordinary generic conditional jump. This happens before target patching, even
when monitoring adds no event to the branch itself. Later specialization may
select a skip only when each skipped target is currently an ordinary NOP.
Wrapped, instrumented or otherwise changed targets decline that edge. The
normal branch path then dispatches the dormant anchor and preserves its exact
line/opcode event and legal/illegal f_lineno jump behavior.

Free-threaded builds never select these variants. All debug builds decline these variants, preserving lltrace across
frames that share a code object. Release-only selection is deliberate in this
first increment. Dynamically enabled tracing and
monitoring already update executing code objects. Code-local event changes run
the same invalidation. Suspended generator frames keep the same pointers and
coordinates; instrumentation updates their shared executing code, so they
resume through the ordinary path whenever a skipped anchor becomes observable.
No executable image moves and no frame pointer reconciliation is needed.

When a boundary is not observed, keeping instr_ptr at the branch until the next
real instruction is safe only because this checked interval cannot execute
Python, raise, poll or suspend. It must not be generalized to arbitrary truth
conversion or to return-result retirement, which can run finalizers. Exception
attribution in the successor remains at that successor; the bool guard declines
before modifying owner state. Ordered anchors and all source/exception metadata
remain distinct even when ordinary execution traverses neither.

## Source interfaces

- `Python/specialize.c`: select a physical boundary handler after selecting
  POP_JUMP_IF_FALSE_BOOL; compute edge eligibility once, validate live target
  handlers, and retain canonical opcode accounting.
- `Python/ceval_macros.h`: replace the ordinary NOP traversal macro with
  constant-parameter edge continuation generation, or delete it once the
  generated variants emit direct control flow.
- `Tools/cases_generator/ir_generator.py`: three explicit execution-only
  descriptors inheriting the bool instruction, guard, format and conservative
  contract. Generate the constant continuations and validate their no-callback
  shape. No inference from arbitrary names.
- `Tools/cases_generator/ir_numbers.txt`: reserve 320–322; operand worker owns
  319. Physical handler IDs exceed 255; stats/deopt/opcode arrays must receive
  canonical POP_JUMP_IF_FALSE_BOOL, never the physical ID.
- `Tools/cases_generator/ir_metadata_generator.py`: shadow is
  POP_JUMP_IF_FALSE_BOOL, base is POP_JUMP_IF_FALSE; full-domain metadata,
  dispatch and specialization flags. No pristine serialized form.
- `Tools/cases_generator/ir_metadata_generator.py`: add a generated per-handler
  SAVES_IP capability from actual instruction properties, used only at selection.
- `Python/instrumentation.c`: invalidate dependent edge continuations before
  patching any target. Preserve specialization pause/restore rules.
- Generated products from the normal producers; `Lib/test/test_ir_boundaries.py`
  owns focused tests. No overlap with operand worker's runtime test file.

Descriptor representation should reuse the operand worker's explicit variant
mechanism if admitted. These are independent worktrees; the coordinator merges
shared generator edits and regenerates once after reviewing their composition.

## Consumer census

| Consumer | This bounded representation |
| --- | --- |
| Jumps, relaxation, taken/fallthrough edges | Original word coordinates; only selected bool edge skips |
| Line table, exception endpoints, traceback, lasti | Original coordinates and distinct anchor records |
| f_lineno stack-kind reconstruction | Original canonical instructions and stack effects; observer path executes anchor |
| Global/local instrumentation, tracing from callbacks | Invalidate dependent branch before changing target; respecialization checks live wrapping |
| Exceptions in instrumentation callbacks | Original instrumented NOP path and error PC |
| Frame escape, GC, copying, suspended generators | No new owners, roots, moved image or changed slot layouts |
| Monitoring changes while suspended | Shared image invalidation; unchanged suspended logical PC |
| lltrace, free-threading | Decline until independently supported |
| Specialization/deopt, stats, feedback | Physical handler maps to canonical bool opcode; same feedback layout |
| Disassembly, marshal, code validation | Canonical bytes unchanged; execution-only descriptor never serialized |

## Separate discarded-return proposal, not admitted by this document

Existing retained call-result POPs release a register owner; they are not stack
machine pops. The first useful removal must separately encode the post-return
observation and retirement boundary. A tempting unconditional return helper
that decodes POP merely relocates work. A future branchless ordinary protocol
can mark the caller's return recipient as a post-call discard, keep the owned
result in C while retiring the child, and only then publish the logical POP PC
and release it. An instrumentation update while the callee runs must repair the
pending recipient to materialize the result and execute the observer POP path.
Native calls need an equivalent pending recipient protocol across callbacks.
The original POP coordinate and reconstruction slot must survive in metadata.

That protocol is not yet implementation-ready: code-local monitoring updates
must reach suspended recipients, callback errors and f_lineno jumps must own
and retire the held result exactly once, and native input-cleanup ordering needs
separate proof. Do not delete the compiler's retained POP gate or reuse bool's
no-PC-publication proof for a potentially finalizing result. Finish the bool
increment and measurement first if those proofs exceed the first checkpoint.

## Gates

Independent design review before edits. Cheapest first: number/domain and
source-model/generator checks; generated handler inspection proving no dynamic
NOP work; coordinator release/debug compile; focused runtime tests showing
warmed variant selection and all taken/fallthrough masks, wrapped targets,
tracing enabled in a callee, global/local monitoring changes, event order,
f_lineno jumps, callback exceptions, generator suspension and respecialization.
Then independent diff/result review and merged integration. Coordinator owns
native builds and timing. Measure logging against golden and upstream plus the
required spectral/broad controls. Fewer branches are a hypothesis until GCC
assembly and timing establish the result.

## Result lifetime versus frame ownership

Python's language model permits delayed or omitted garbage collection; prompt
reference-count finalization is CPython behavior, not a universal Python language
requirement. See the [language data model](https://docs.python.org/3/reference/datamodel.html#objects-values-and-types).
A frame can therefore own unused results until a bounded cleanup point if the
representation, tracing roots and language observation semantics remain sound.
A standalone POP/register-release instruction is not required by frame ownership.

For this selected reference-count architecture, direct call-completion discard
is likely cheaper than storing each ignored result until frame exit: it avoids
a new root, slot traffic and unbounded retention in long-running loops. Exact
existing finalizer ordering remains a chosen CPython compatibility target for
this bounded optimization, pending a separately justified behavior change. It
must not be described as a language-level proof that POP is necessary.

Distinguish four constraints in the next proposal: language observation and
reachability; chosen CPython compatibility behavior; bounded memory and lifetime
quality; current owner/root/cleanup safety invariants. Arguments and locals have
existing frame cleanup responsibility. Ignored results can instead stay in a
single C owner through child retirement, then complete at an explicit boundary.
A completion-time guard can materialize the original result slot and dispatch
the observer POP when instrumentation changed during the child body or child
cleanup. Ordinary completion can publish only the logical retirement PC and
release directly, without a result-register write/clear or standalone POP
handler. This uses bounded storage and preserves the current compatibility
behavior without claiming that every future implementation must do so.
