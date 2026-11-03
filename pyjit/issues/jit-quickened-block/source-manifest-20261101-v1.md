# Proposed exact source cut and stable-ID checkpoint

Read-only handoff for design283386. No source grant or execution. Base must be chosen explicitly; preserve11aef and all older worktrees/results. Proposed45-minute source cap/10-minute metadata-and-entry checkpoint stops rather than invents a cache policy.

## Stable IDs

Actual analyzer.py:1177 allocates specialized IDs in sorted order; adding an ordinary family member naively renumbers others. Current opcode_ids.h has exactly ONE unused positive byte ID:127. Use its existing predefined-ID mechanism (as for BINARY_OP_INPLACE_ADD_UNICODE=3) to reserve BINARY_OP_NATIVE=127 BEFORE allocation, and include it as a BINARY_OP-family specialization. add_instruction already skips predefined names without advancing next_opcode. This should leave every OLD opcode/pseudo/IR-only/NOFB handler ID unchanged, preserve first_ir_only_handler and the302-entry table length, and fill its formerly unknown127 slot.

At the10-minute checkpoint, regenerate and compare the complete old name→ID maps, formats of old handlers and table indices, not just selected IDs. Check MIN_SPECIALIZED_OPCODE=129 consumers:127 is below it, so code relying on a numeric range must recognize actual specialization metadata (the existing ID3 exception is precedent, not proof). New head inherits BINARY_OP base/FB_BINARY format. Objects/codeobject.c:check_words must explicitly reject127 in pristine words; marshal persists pristine co_words, never patched exec words. No old-valid pristine word used127. If complete stability/reader closure cannot be demonstrated, STOP for a new private-cache/marshal decision; do not bump magic silently or renumber the fork.

## Handwritten files

- Include/internal/pycore_ir.h: remove MayEnter, FilteredEntry and TestAdmission declarations; retain/share only owner state still needed. Add actual block-entry/restore declarations as necessary. No global frame admission helper.
- Include/internal/pycore_jit_prefix.h: Plan relative start/exit contract, selected-head interface and output mapping; old file name may stay, but no legacy word1 behavior/fallback API.
- Python/bytecodes.c: REMOVE both old frame-hook bodies entirely (14 generated copies), including prefix_exec loads/branches/counters. Add only the BINARY_OP_NATIVE family body, selected-site warmup and shared original-ADD fallback once. Ordinary frame-entry projection must equal qualified RC.
- Python/specialize.c: optional structural discovery only in existing cold _Py_Specialize_BinaryOp path after temporary-box cleanup. Never add a check to ordinary specialized ADD or unrelated entry.
- Python/jit_prefix.c and Python/jit_prefix_entry.c: one site-keyed descriptor/owner, generic relative-bound block decoder, native compiler/result mapping and interior guards/commit. Remove old first-word/RESUME-entry contract. Preserve no-call arithmetic/rollback; head2 has int output0/scratch18, untouched materialized float2. New leaf bytes are expected; inspect complete output mapping rather than demand old94-byte identity.
- Python/ir.c: BaseOpcode/HandlerFor/RealHandler/feedback closure, exec initialization/clear/size and restore-before-release. No unused frame-filter API or counter remains. Immutable exclusions may remain cold only.
- Python/instrumentation.c: restore the selected ordinary head before observation/disable/owner retirement; permanently disable after actual observation. Existing line/instruction/semantic-base readers must see the original operation.
- Objects/codeobject.c: reject exec-only head from constructors/cache words; retain normal physical-slot/FB bounds validation.
- Tools/cases_generator/analyzer.py: explicit127 reservation with collision/stability assertion.
- Tools/cases_generator/ir_generator.py and ir_metadata_generator.py: same-family head emission/format/base/dispatch and correct generated materialization/fallback, only if their existing family machinery needs adjustment. No new parser/DSL or arbitrary semantic-base head.
- Modules/_opcode.c and Lib/dis.py: verify/update actual new specialized-handler introspection; avoid new behavior if existing family maps already suffice.
- Modules/_testinternalcapi.c and Lib/test/test_ir.py: remove jit_prefix_admission/cold filter truth-table API. Update EVERY current info/run/fail-allocation/native-info/pressure/try fixture to the selected interior Plan/owner contract, including its original head instr_ptr/SP, live float temp, NULL writes, output0, fallback once, natural quickening/restore and relative bounds. Read-only info must not create/patch a site and manufacture automatic activation; explicit direct Plan fixtures remain distinct from natural-hook witnesses. Keep existing12 JIT methods/54 cohort; no new test ladder.

## Owner/layout

ONE optional scalar candidate/owner per exec, keyed by Plan.first_word (lowest eligible pristine offset). Reuse the existing prefix_plan pointer/eligible/state storage: no per-frame fields, global registry or second-site owner. Candidate descriptor has no RX mapping until64 fully guarded selected-head entries; transition to compiled owner must retain that same site key. No borrowed frame/code/PyObject pointers in scalar descriptor. Charge candidate/compiled heap bytes in SizeOf, restore head before freeing either state, and retain code lifetime through execution. Existing exec header size must not grow; heap descriptor layout/accounting can change explicitly.

## Generated products and cold observer

Run all existing six generators/seven products and review actual changes: Include/opcode_ids.h; Include/internal/pycore_uop_ids.h; Lib/_opcode_metadata.py; Include/internal/pycore_opcode_metadata.h; Python/generated_ir_cases.c.h; Python/ir_targets.h; Include/internal/pycore_ir_metadata.h. Extend existing test_generated_cases coverage for stable IDs/family format, not hand-edit output. Confirm14 frame hooks removed and no admission code survives at either canonical entry.

Only the UNSCORED origin wrapper/reader and corresponding host fixtures change in the existing timing helper: observe AFTER normal workload; actual chosen head2/continuation12 for registered eval_A, outputs ONE int home3, float2 untouched, fresh full native bytes/map/accounting and automatic selected-head success. Do not assert old word1/two-output/94-byte facts. Revalidate all58 variant routes with mocked kernels. Scored driver, paired schedule/GM29/290/spectral10/GM28diagnostic/material-disposition rules, CPU/locks/cleanup and116/580+58/174 exposure remain unchanged. No four-workload pilot or extra target probe.
