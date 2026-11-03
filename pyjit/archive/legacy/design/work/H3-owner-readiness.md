# H3 storage union: owner readiness checklist

Source-only checkpoint, 2026-10-29. The existing worktree is clean at
`81417aff0971fe621fec296a324c7a48315a6082` on `boil/h3`. No merge, checkout,
native/generated edit, build, target execution or new qualification occurred.
This checklist applies the existing [handoff](H3-post-ROOTS-handoff.md);
it adds no design or gate requirement. Applicable repository/parent AGENTS
and the previously read CPython AI policy remain in force.

## Source and evidence retained

- Existing owner path: `/home/power/code/cpython/.worktrees/boil-compiler-copyprop`.
  Preserve frozen `boil/h3-monitoring-storage-land` at
  `ffc548603b823484fdf106b1a9de06785017c682`, native `76274ed06ec`.
  Live 81417 and ffc have identical Include/Objects/Python/Modules/Lib/Tools/
  Makefile source; the later commit is the statistics design.
- Read stage 2a admission/scalar ownership, stage 2b.1 names/NOFB, stage
  2b.2a immutable SafeAddr2Line, stage 2b.2b storage/work record and the
  three eligibility guards. Their full combined storage candidate remains
  the unit of integration; do not extract the eligibility fix as a new rung.
- Rechecked final-provenance `4a432b9a...`: all 240 retained artifact hashes,
  13 changed source hashes, 46 unchanged source hashes, seven generated
  current products and eight historical binary/config/CAPI hashes match.
  `combined-sources.json` still hashes to
  `a6c07b4ce7b2932a82c2bc8bcc187f8940359fe1674951f075ee1b98773e8557`.
  These are byte checks of retained evidence, not new native results.
- Primary is `57b5a61a6f2`; qualifying ROOTS candidate is `ce2ac8991ee`.
  Neither is presumed to be the eventual integration base. Read-only
  three-way source preflight at base `6c724c29408` still reports the same
  five changed-in-both paths and no conflict markers. Full merge-tree output
  hashes to `7842d447470265a1de9ee47dec8a489aa9927137b3d303ee9832f575b60da05b`.
  Clean textual composition is not semantic or generated closure.

## Ownership and first feedback after the coordinator's handoff

1. **Name the actual base and edit grant.** Wait for ROOTS qualification,
   detached landing review and fresh integration gates. The first quiet
   elapsed diagnostic is an early scheduling intent, not a source dependency:
   root may release semantic composition while timing preparation continues
   and reserve native builds/tests around the bounded quiet window. Retain the
   frozen parent build so a noisy or incomplete timing attempt cannot block
   H3 indefinitely. Root names the stable primary BASE, author branch and
   shared-file/generator/build ownership. Keep ffc and its receipts immutable;
   no merge is authorized by this readiness record.
2. **Compose the five shared paths.** Preserve both sets of generated/root
   test classes and every CAPI method/helper. H3's three storage VARARGS hooks
   and scalar/safe-line fixtures retain their reviewed signatures; ROOTS's
   METH_O Park helper in `gc_walk.c` retains PyValue input/result and its cast.
   In ceval.h keep ROOTS's precise annotations/exception span together with
   H3's no-monitor handled guard. In bytecodes.c keep ROOTS's two-argument
   `_Py_Check_ArgsIterable(tstate, callargs_o)` and H3's guarded RESUME refresh.
   Regenerate generated_ir_cases from that composed source; never hand-merge it.
3. **Check the storage contracts before building.** Preserve aligned uint16
   displaced LINE/INSTRUCTION IDs, absolute int lines, canonical wrapper
   relations, all production GetLine callers, nested-array teardown and
   private prepare-before-publish/OOM retry. Keep immutable SafeAddr2Line.
   Retain all three eligibility guards: RESUME version mismatch, refresh
   entry and cold monitor_handled. No version acknowledgment or shared/static
   writes substitute for those guards.
4. **Freeze exact first-feedback inputs.** Review the composed source and
   generated/domain closure, imported current checker/worker helpers and
   actual compiler/configuration. Use standard admission/build locks and
   bounded -j4. Root must name the debug configuration: historical H3 Og
   receipts do not authorize or qualify an O1 union. Capture complete new
   binary/CAPI/products before execution.
5. **Get bounded feedback, then use the existing ladder.** First smoke both
   builds; run the eleven H3 affected modules and actual debug-kill with the
   current worker.main/collector/CAPI witnesses. Add the exact ROOTS/Park/
   queue integration selectors and discovered current marked/linked audits
   named in the handoff. Rebind the existing eligibility watchdog, line,
   restoration and allocation/OOM controls to union products. Counts are
   discovered anew. The handoff/PROCESS governs subsequent fulls, ASan,
   stress, detached landing review and any integration identity bridge;
   historical H3 receipts are not union gates.

After storage union lands, the accepted sequence remains stage 2b.3
statistics, bounded H1, remaining H3 widths/transport/renumbering. H1 remains
technically independent of H2/maps/frame-pointer policy. Existing op>=256
statistics are omitted by a guard; this is not evidence of an out-of-bounds
write. Generated semantic properties replace category thresholds as each
consumer moves; no arbitrary operation-count cutoff is introduced.
