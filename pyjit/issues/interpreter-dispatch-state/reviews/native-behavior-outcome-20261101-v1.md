# Slot-base first native behavior outcome

**ACCEPT exact first native behavior feedback** for source
`3ddc437679ebbb9f8629c1670655c58776075dfc`. Emitted-work interpretation,
elapsed acceptance, final backend qualification and landing remain separate.
This source does not include mandatory nursery adoption or the rejected
constant-operand increment.

The slot-base worktree's
`build/dispatch-slot-base-feedback/feedback-batch-completion.json` is SHA256
`3d445bd3f7c5fb2024e06d000196d6108ae30eabad6685697b0e758ae7733bf2`.
All 17 rows completed at **2026-11-01 00:19:10.994248 UTC**, within the fixed
**00:12:00.978874–00:57:00.978874 UTC** window. The coordinator confirmed
session 60831 terminal exit 0 at 00:19:12; this reviewer did not poll that session.

Independent host receipt
`build/interpreter-dispatch-state-review/native-behavior-outcome-20261101-v1.json`
is `31db441e6b9a0957db2a4c8246d46c6ae24b505590013de6068c8fe1c23e9df1`.
It rehashes all 7,262 completion artifacts and all 7,055 inputs, replays each
unchanged job collector and pure whole-module/stale validator, and verifies
the current source/configuration/tool/products and actual build-locked idle
state. No target, compiler, workload or probe ran in this review.

## Executed scopes

The debug exact 20 selection passed 20 tests without skips and 399 collections.
The nine full debug OFF/N0 module rows returned:

| Module | Tests | Skips | Collections |
| --- | ---: | ---: | ---: |
| test_ir |63 |0 |100 |
| test_generated_cases |79 |0 |409 |
| test_generators |72 |0 |119 |
| test_coroutines |100 |0 |135 |
| test_asyncgen |100 |0 |104 |
| test_sys_settrace |442 |1 |68 |
| test_monitoring |108 |0 |73 |
| test_dis |147 |0 |76 |
| test_code |47 |1 |107 |

Every module returned PASSED, with zero unwind fallbacks and zero observed
report-only unpinned visits. The generator-case row ran 79 tests: 78 ordinary
passes and one existing expected failure. The exact two skips
are `test.test_sys_settrace.JumpTestCase.test_jump_extended_args_for_iter`
(`resource 'cpu' is not enabled`) and
`test.test_code.CodeConstsTest.test_interned_constants`
(`does not intern all constants`). No new filter or skip was introduced.

The separate O3 exact seven IR cases passed without skips, with 52 collections,
zero fallback and zero observed unpinned visits. Exact selected ID multisets,
whole-module returned cohorts, protected cases, actual worker mode/stress,
seed, source/library/extension origins and auxiliary identities replay to the
stored summaries. The exact 20 row contains six actual driver/workers; each
single-module row has two. These are separate executions with overlapping
methods, not a summed unique-case count. Unobserved transient modes of all
test-created children are not inferred from worker startup.

The six registered stale functions each passed 100/100 on debug OFF, debug
KILL and release OFF. Their actual child PIDs map to positive real collection
counters; row totals are 713/713/709 respectively, with zero fallback/unpinned.
The actual start/end checker/extras/stress and source/CAPI origins match the
strict contract. These are three six-by-100 stale controls, not ordinary suite
methods or full KILL-module coverage. Raw stale streams remain retained.

## Build, receipt and cleanup proof

Fresh GCC O1/Py_DEBUG and O3 builds both pass their build/import/linked audits
under the reviewed effective configuration. Each freezes 588 products; all
current indexed bytes, resolved paths and symlinks match, with no regular pyc
additions. Source remains clean 3dd. Compiler/tool aliases and hashes match the
frozen toolchain; Makefile/pyconfig hashes match actual build results. The O3
Makefile equals the accepted baseline after the explicit source/build-path
bridge, and pyconfig bytes match exactly. Debug image SHA256 is
`1ee3456a70bd52e90a7cc8d0acc687de2cdde6a552891a7b37302867f6dc0156`;
release is `d3d9843ee1a6d441f22d753d96d1d3224c9fa0799f14503f4d71f3fd792ef035`.

All 17 literal launch/gate/verification/cleanup receipts replay, including
allowed downward timeout clamps, successful terminal return codes, raw hashes
and result-to-cleanup bindings before deadline. Final source/products/plan
remain frozen; fresh independent lock observations find both IR targets idle.
The separate GC batch does not use these products. There is no failure
envelope or retry in this native attempt.

Static emission completed and its 18 captures are hashed and bound. That is
artifact collection, not acceptance of a speedup or register-stability claim;
the separate emission review assesses actual branches and receiving loads.
This batch supplies no tail/switch/free-threaded/ASan/full-suite qualification,
nor any elapsed score. The held performance plan and one coherent final
qualification remain necessary before a broader acceptance claim.
