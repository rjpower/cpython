# RC release full-suite first row: seed clarification

2026-11-01. Source-only proposal; no activation. Run this first while the remaining broad-qualification profiles are reviewed. Reuse exact clean RC `d7df12b1d81722927e71d9734c70eeafdccfee73`, existing `build/worktrees/rc-successor/build/release/python` and its immutable build/product/configuration receipts. No rebuild or narrowed test cohort.

The literal target selection is the existing gate2 default full suite:

```text
RC_RELEASE/python -m test -q --timeout 900 -j16 \
  -i test_remote_exec_deleted_static_executable \
  --python ABS_RC_FULL_WORKER --randseed 0 --no-randomize
```

No named test modules, positive match filters, resource sweep, rerun or stress variables. The only explicit exclusion is the existing machine-local `test_remote_exec_deleted_static_executable`; retain its reason and exact spelling. Standard platform/resource/optional-module skips and all result states remain visible for independent outcome review. `test_cext` normally requires CPU resource and remains an explicit later row; its ordinary resource skip cannot become CPU-gate credit.

Reuse the accepted gate/serialization/timeout/cleanup executor. One3600-second run ceiling plus the existing210-second verification/cleanup reserve; declare a3810-second inclusive root window, downward-clamp the run to remaining minus210, no automatic retry. Use both global slots and full-suite serialization, release build lock, and dual `python`/`Programs/_testembed` cleanup. Effective target parallelism is exactly16 as in the existing full gate. No concurrent compiler, suite or measurement window.

The smallest necessary helper delta is a thin full-suite RC entry derived from the existing guard and worker shell: assert actual release/GIL/RC ownership, GCC computed/mimalloc/boxed shapes, ABI/cache/image/CAPI/source/config origins, seed0 and diagnostic OFF request. Keep driver scheduling/returned standard module results and worker start/end observations. Bypass the targeted per-method/result/child installers: a full suite creates nested runners/subprocesses beyond that cohort's census. This row makes no complete nested-process census or per-method execution claim. Record actual default selection and all returned module states; missing scheduled worker/result/end records fail closure.

Retain existing image/product/tool identity verification and frozen ordinary RC observations. Do not call tracing mode/stress/rootstats getters, export PYTHON_GCROOTSTATS, fabricate zero root counters or run legacy tracing-root assertions. The historical verifier count/holes fields remain diagnostics only. Do not invoke `gate.sh`2 unchanged: its unconditional roots() check is inapplicable. A reviewed RC binding copies its exact full-suite command and accepted gate body, omits only that collector-specific postcondition, and retains the ordinary full-suite result contract.

Before activation: complete argv-role validation before any lock/Popen, actual output/import smokes, missing activation/timeout-stream controls, source/product freeze and independent helper review. Preserve the current source and all prior failures. After execution require root-confirmed terminal0, exact full selection/actual module states, retained raw output and locks/dual executables idle before deadline. Any failure stops this batch and becomes a concrete integration correction; do not create another arbitrary selected cohort.

This successor preserves the original row plan unchanged at its original path
(SHA25618baa79e022f78a84109cb18433cf9980244ac1d8bb67553ed84539c2dea3a3e).
Root approved the explicit seed clarification before execution: --randseed takes
the separate integer argument0, and --no-randomize preserves deterministic order.
No test module, resource, timeout or exclusion scope changes. No helper may
silently force a seed by mutating the actual runner configuration.

The full-module outcome must include real non-skipped execution in test_ir,
test_gc, test_generators, test_type_cache and test_capi. This is a positive
coverage assertion on the unchanged default selection, not a selected cohort.
The full-profile gate acquires both existing machine slots in fixed order,
releases a partial pair before retrying, and retains both through build cleanup.
