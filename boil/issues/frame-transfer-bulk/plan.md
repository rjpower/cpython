# Finite frame-transfer experiment

Executed 2026-10-31; [results](results.md) reject the implementation after the
declared two-round comparison. The sequence below records the experiment plan.

1. Source review: inspect the single-header patch and all three caller proofs;
   run existing host root checks and diff checks. No commit/build before the
   coordinator reviews the patch. Root owns status; this issue owns the header
   and, only if a real gap appears, `Lib/test/test_ir.py`.
2. Reuse accepted ordinary build recipes with literal base/candidate source,
   tool/config/product bindings and normal locks. Start with the fresh O3 image
   needed to inspect emitted code; no serving-tree mutation or baseline rebuild.
3. Run existing focused frame/generator/coroutine behavior once using the accepted
   raw worker/validator. Include test_ir's resumer/exception/frame controls,
   test_generators escaped mutable frame payload and origin/publication tests.
   Let source review determine additional applicable audits; a losing candidate
   needs no broad qualification campaign.
4. Inspect old/new `_PyFrame_Copy` payload instructions. If the intended change
   survives and behavior passes, propose exact coroutine/generator/Richards
   matched elapsed/instruction vectors under a quiet reservation. Use the
   existing registered workloads/driver, not a new benchmark or runner. Send
   runtime estimate and actual bindings before measurement. No 29-row rerun.
5. Integrate only if correctness and native cost support the simpler expression;
   otherwise report rejected/inconclusive and retain the experiment's evidence.
   No arbitrary performance threshold or automatic repeat-to-green. Stop at
   the 90-minute decision budget and report what remains unresolved.

The completed scope is one fresh O3 build, ten OFF cases and two paired-four
rounds. The mistakenly scheduled release-KILL startup failure remains distinct.
No debug/sanitizer/full qualification follows a rejected performance experiment.
