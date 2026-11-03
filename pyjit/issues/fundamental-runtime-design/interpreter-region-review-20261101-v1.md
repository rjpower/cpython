# Independent interpreter-region design review

2026-11-01. **ACCEPT executable design for a bounded host prototype, subject to
the evidence and source-review gates below.** This is not source, native,
measurement or integration authority. No design blocker was found. The exact
consumer remains provisional; the live shared baseline has not yet supplied
registered-site attribution. No compiler, target, probe or extra census was run
for this review.

Reviewed frozen documents:

| Document | SHA256 |
| --- | --- |
| `interpreter-region-execution-20261101.md` | `b7be061bbe0ce61c81ca604b38cb0fea36406774c4ae6c0de8639405b7930e3e` |
| `ir-region-hypothesis.md` | `b72b5e668dd9799198071cbea61ccea179de4cac42477301052cf251feccc9a2` |

The execution proposal controls sequencing: a coordinator must grant each
bounded stage. The hypothesis's old 04:18:11 wording does not impose or extend
that deferred nursery dispatch's clock on this proposed experiment. Existing
native work keeps its own accepted deadline.

## Compiler and generator boundary

The proposed placement is concrete. `_PyRegCfg_Build` currently converts,
executes `run_passes`, compacts, then runs physical verification in debug
builds (`Python/regcfg.c:1433`). Planning between the existing rewrites and
compaction preserves their canonical/local/fresh-register distinctions and
avoids invalidating plans through subsequent copy/store/truth rewriting.
Compaction retains op indices but assigns slots and can reuse an input's slot
for its result. Keeping the original ops and boolean home is therefore the
right first scope; no frame-size reduction follows from this design.

Admission must use both `nimplicit` and the handler live-in contribution in
`transfer` (`regcfg.c:626`), not just explicit read counts or `last[]`.
`last[]` is a last-read property, not proof of exactly one reader. Require an
independent count/proof of the fresh result's unique consuming read, no
canonical/live-out/exception use, and no folded local destination. Op indices
must be owned by the CFG and all planning allocations freed on every compile
exit; no pointers into a reallocatable operand array may survive a mutation.

Post-compaction region checks must run in release compilers too. The existing
debug-only `_PyRegCfg_Verify` is useful additional coverage, not a production
admission authority. Check actual `_PyRegCfg_OpSlots`, same-block adjacency,
all operand/result aliases, source/exception context and final word placement.
Negative host fixtures must distinguish a genuine rejected nomination from a
test which merely never reaches the planner. Include a result reusing a
consumed operand slot, equal input slots, borrowed-local inputs, and a separate
live temporary; final clearing must not destroy the latter or a local.

The generator work is necessary, not cosmetic. `generate_handler` calls
`write_uop` and then sets `IRStack.final` before flushing
(`Tools/cases_generator/ir_generator.py:1798`). Its current compact-int case
consumes both operands, stores the boolean home and dispatches; the ordinary
branch loads and clears that home (`Python/generated_ir_cases.c.h:7979`,
`:15508`). Concatenating these handlers retains the very round trip being
tested. The explicit internal-value binding and two logical layouts must
compose existing ownership/clear operations. Generic comparisons retain their
current collecting/reentrant behavior; the exact admitted guards/comparison
do not allocate or run Python. Existing effect and precise-frame checks must
continue rejecting an internal value across a collecting call. No handwritten
error unwind or generic-family purity annotation is justified.

## Execution, restoration and full-domain closure

The specified fallback is essential. `unspecialize` restores the pristine
handler (`Python/specialize.c:548`); with a pristine region head, deoptimize and
redispatch alone can loop. Every type, pair, observer, epoch or breaker rejection
must enter a real ordinary materializing comparison body with original homes
and comparison IP, then execute the ordinary branch. Reusing the generator's
ordinary semantic body is preferable to copying comparison semantics. Host and
native tests must exercise repeated rejection and successful return, including
generic comparison/truth callbacks that mutate locals, allocate or raise.

Monitoring currently records canonical semantic base tokens, and restores
through `_PyCode_HandlerFor` (`Python/instrumentation.c:71`, `:290`, `:614`;
`Python/ir.c:204`). Preserve `COMPARE_OP` as semantic base and use the pristine
word to recover the region form. Do not make a region ID its own semantic base
to bypass canonical-token assertions. Widening the current comparison
selector's `uint8_t specialized_op` is necessary but insufficient: generated
domain/kind/state/format tables, specialization and feedback rows, pristine
assembly, installer checks, disassembly, monitoring and code validation all
need explicit coverage above 255. Shared feedback entry zero must select a
safe ordinary/no-feedback form or be rejected for region nomination; it cannot
become a private specialization entry.

`Objects/codeobject.c:671` currently validates each word independently. New
region checks must validate bounded lookahead, semantic branch form, physical
home relationship, forward target and relevant fields before any second-word
read. Crafted/truncated code is not covered by compiler admission. Retain the
ordinary second word and its actual feedback. Requiring its executing handler
to be exactly `POP_JUMP_IF_FALSE_BOOL` makes cold/adaptive branch warmup real;
direct entry must remain ordinary execution from its actual home. Do not fake
a skipped second dispatch in handler-entry or pair counters. Reused comparison,
truth-hit and branch-history bookkeeping must be reviewed separately from
physical entry counting.

The fast interval preserves both IP publications, contains no call, observer,
collection or existing service boundary, and crosses no backedge. Deny entry
for pending breaker work and applicable tracing/monitoring, instrumentation
and debug instruction observers. Keep next service points unchanged. Branch
history and NOT_TAKEN behavior must come from the existing operations. Direct
entry and `f_lineno` tests must establish a legal reachable state; a same-line
pair alone does not prove a debugger can select its second word. The C boolean
is immortal, but that fact does not authorize general unrooted residency or
weakening asynchronous frame readers' existing contract.

## Unmet gates and decision

1. Before installing the consumer, obtain current executing/pristine IR and
   relevant comparison/branch activity for the actual registered kernel. The
   synthetic three pairs and zero-call capture remain insufficient. Aggregate
   pairs cannot establish one PC if other sites contribute. If the live shared
   baseline lacks this attribution, hold installation pending a separately
   granted minimal observation; do not infer it from workload names.
2. Grant at most the proposed 45-minute host prototype with 15/30-minute
   checkpoints. Freeze actual partial work if composition, metadata restoration
   or physical proof does not fit. Require generated output outside admitted
   forms to remain unchanged, plus negative planner/validator fixtures and
   strict call/root/linked audits. Source review precedes fresh execution.
3. Fresh behavior must prove ordinary warmup/direct entry and all slow exits,
   callbacks/errors, monitoring restoration, aliases, full IDs and malformed
   pairs. Emitted complete-path evidence must show the removed home write/read
   and second dispatch, preserved IP stores and real control operations.
4. Only then compare parent, region and source-only materializing control under
   the frozen 29-kernel protocol. Verify the volatile intervention survives
   compilation; its timing is not a cycle-perfect subtraction. Keep compiler
   cost and memory/resource reporting separate from kernel warmup. Elapsed and
   the declared variability/regression rules decide; static instruction counts
   neither accept nor reject the architecture by themselves. Qualification is
   a later gate on one selected source, not evidence supplied by this review.

This is a meaningful compiler-to-interpreter experiment: it tests reusable
materialization composition with one small nonreentrant consumer. It does not
establish that SSA, wider regions or a new backend will win, and it does not
replace the independently owned integrated allocator/collector experiment.

Source bytes inspected for this review:

```text
Python/regcfg.c 659a0fc199cdde78aba70fdd0ebb40d433e8ce1d32b48752003d94ac668da9ee
Python/assemble.c e1e9eb4bfd9eac3b6f7d02cc04a5f820d567b58e81b58e59d6a169f340dc264b
Include/internal/pycore_regcfg.h 9e76c09b2fd7d40272ddc2ff6c1670d9b2a8868522c2fc4488f9f46368b12ad6
Tools/cases_generator/ir_generator.py 0a24d1cd83b7dbeac95abdfe018ed93b9bfd6198f19be136c0b890fddeb93cdd
Python/bytecodes.c 3d3fd8758b0962abbaa0c647da23fcca86904ce2fb4b9bd6ebd74dde4963e3f9
Python/specialize.c 9d87cb5fdf1b550720873fd645c649a0ff609ec8e04687395ae71bb19d8ac8e0
Python/instrumentation.c cc70ec7ae3a45fddf98096a592b194e7cb9c7e65b489d861679fd1bda668479a
Python/ir.c dff203b41537191a5fa16978cc5e69aad9f5c144cf4ac155a47453a455335c04
Objects/codeobject.c 20208af12a66053422ca6ff9780e7ffc67f1064cbd624ff50404680398fa2eae
Python/generated_ir_cases.c.h f27a64827c0b071996f573b981348099f313b7bc3b6869c66c54d7c8c19fa939
```
