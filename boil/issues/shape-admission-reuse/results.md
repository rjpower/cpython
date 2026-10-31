# Shape-admission source and O3 feedback

Recommendation: keep this as a small shared-family simplification for
integration review. Reviewed commit `c46f1bef67f58dacac04215765e557f7f3b51523`
combines each existing polymorphic LOAD/STORE admission with its payload so
the existing matcher runs once. One fresh O3 build, two finite behavior rows
and static emission inspection passed. No timing or new profile was collected.
The rejected frame-transfer source remains absent from this checkout.

## Captured importance

The four frozen `0b15b4bbc9b` captures retain their original data and
interpretation. Reparse the existing perf-script texts using accepted PID/hot
bounds and period weights, recover PIE load bias from mmap records, then use
the qualified ELF's inline debug locations to distinguish the two call sites.

| Richards innermost shape-row context | Samples | % total sampled CPU |
| --- | ---: | ---: |
| Required LOAD admission | 33 | 5.108359 |
| Required STORE admission | 6 | 0.928793 |
| Second LOAD payload lookup | 5 | 0.773994 |
| Total `_PyFb_ShapeRow` | 44 | 6.811146 |

There are 646 selected Richards samples. No STORE payload lookup sample or
innermost shape-row sample in the other three captures was observed; absence
does not prove zero executions. The five payload samples establish actual
execution of the redundant emitted path, not its hit distribution or a reliable
speed forecast. Samples may skid. `_PyIR_FbEntry`'s separate 7.276% share spans
other handlers and cannot be charged to shape-admission duplication. Richards
already runs 1.886× upstream in the matched diagnostic. Do not use this finding
as an explanation for generator/coroutine deficits or a new aggregate score.

The [offline replay](../../../../boil-shape-admission-reuse/build/shape-admission-source/replay.py)
is `c19d64a665726b418ff062b6a32338f944d585436ae2f640547811899012dc55`;
[derived attribution](../../../../boil-shape-admission-reuse/build/shape-admission-source/attribution.json)
is `36d1459d2db5e6f2fe9e0f73011a9165271dfeb77ca386434096b375564a29f1`.
Run with host `uv run --no-project python`. It records raw inputs and hashes,
PCs and `addr2line` results; no target process or new measurement is started.

## Source proof and generation

The [source proposal](../../../../boil-shape-admission-reuse/build/shape-admission-source/proposal.diff)
is `e491786e04c6f6ef46eb08d4b698f84586b487bcf47b22a714a5010751463f40`.
The old guard was already a DSL guard: generated object-input conversion
rejected inline owners rather than boxing them. New `inline_ok` plus explicit
pointer admission preserves that rejection before the borrow; it does not
narrow an admitted inline-receiver case. Existing `_PyFb_ShapeRow` checks the
live version and address once; a miss precedes any mutation. STORE loading its
value earlier and LOAD forming its output-array pointer earlier create no side
effects or calls; inputs remain intact on deoptimization.

The initial task-copy spelling nested `Py_UNLIKELY` inside EXIT_IF. Existing
analysis rejected that macro as an unknown escaping condition. The retained V1
proposal was replaced by the ordinary EXIT_IF condition; no effect classifier,
checker exception or generator behavior was changed to admit it.

Seven outputs were generated into task storage. After normalizing only the
generated source-path comment to the canonical path, these are byte-identical:
`Include/opcode_ids.h`, `Lib/_opcode_metadata.py`,
`Include/internal/pycore_opcode_metadata.h`,
`Include/internal/pycore_ir_metadata.h`, `Python/ir_targets.h`.
Thus actual IR numbers, instruction formats/effects and feedback layout are
unchanged by generated evidence. The changes are:

- `Python/generated_ir_cases.c.h`: one match per LOAD/STORE body, receiver
  pointer miss and row miss preserved; no receiver boxing or cross-word state.
- `Include/internal/pycore_uop_ids.h`: remove `_GUARD_SHAPE_POLY`, reducing
  `MAX_UOP_ID` from 514 to 513 and shifting later private uop IDs. This is a real
  generated change, not an omitted file. No serialized IR-handler renumbering
  is observed.

[Generated diff](../../../../boil-shape-admission-reuse/build/shape-admission-source/generated.diff)
is `bfdb37b6582e83ddf34194df7d037454e2d393816647e09864a6f1d99e27720f`.
The [terminal feedback index](../../../../boil-shape-admission-reuse/build/shape-admission-validation/completion.json)
is `7514f700c22c84f3ac130984a2f17351aa233bc78fdafd4286e5ed89c792e566`
with 71 absolute artifact hashes. It includes the successful build/cleanup,
both raw target rows, unchanged-validator replay, source proof and emitted
comparison. Source and products are frozen; no target is active.

Consumer search found no serialization or runtime transport of the shifted
private uop enum. Actual IR handler numbers and metadata are unchanged, so no
pyc magic bump is required for this change. The applied commit contains only
`Python/bytecodes.c` and those two required generated files, +45/−64 lines.

## Actual O3 feedback

One fresh GCC O3 release build completed in 111.38 seconds, including linked
audits, 116 module checks with no import failures, product freeze and cleanup.
Its source is exactly `c46f1bef67f`; the accepted helper changes only its HEAD
binding. Both target rows used normal OFF/N0 on this release image after the
GC lane released its runtime window. Neither row claims debug-kill coverage.

| Row | Tests | Skips | Collections | Fallbacks / snapshot visits |
| --- | ---: | ---: | ---: | ---: |
| Whole `test_shapes` | 156 | 8 | 280 | 0 / 0 |
| Ten IR/generator controls | 10 | 0 | 75 | 0 / 0 |

The eight whole-module skips are exactly existing
`DictionaryModeReasonTests` methods requiring `--enable-pystats`. All 22
polymorphic LOAD/STORE cases, the cached-address reuse case and the poly kind
generalization case were protected and unskipped. The ten controls cover
borrowed-load folding, deoptimization, pushed-frame errors, line/opcode/frame
observers, yield/resume, shared feedback, folded attribute data and escaped
generator-frame payload. Native return, unchanged raw validator, auxiliary
pair/mode checks and post-command/cleanup source/config/tool/product guards
all passed. The shell worker retains mode 0755. No new exclusion was added.

## Emitted work

The qualified parent and fresh candidate inline debug records identify four
old `_PyFb_ShapeRow` call sites, now two: one per LOAD/STORE family. Candidate
later-row success blocks select an integer row and jump directly into the
payload instead of searching again. LOAD's common payload is at `0x46071b`;
STORE's kind check/payload begins at `0x460607`. Their row/version guards and
generic miss exits remain. The evaluator symbol shrinks from 142,684 to 142,471
bytes (213 bytes); this size change includes compiler layout decisions.

[Emission comparison](../../../../boil-shape-admission-reuse/build/shape-admission-validation/emission-comparison.json)
binds both actual ELF hashes, raw disassembly and `addr2line` inline records.
This demonstrates removed emitted comparisons, not a measured elapsed gain.
The five historical payload samples remain a profile-local upper bound;
there is no justification for another timing campaign to chase this fraction.

This finite feedback supports the source simplification. It is not full
landing qualification, an integration result or a refreshed performance
score. Preserve the isolated source/products until the coordinator chooses
the applicable integration checks.
