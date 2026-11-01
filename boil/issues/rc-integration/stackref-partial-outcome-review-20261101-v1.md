# Independent outcome: stopped RC stackref full run

2026-11-01 08:54 UTC. **FAILED PARTIAL; qualification remains blocked.** Source is RC `dce654018d5ae16ffe5e772976d9ccc2c9d1e01a`. Root confirmed supervisor 15564 terminal exit 1. This review accepts the retained failure evidence and cleanup, not the full suite.

The standard observer selected and scheduled 565 modules. It retained 563 returned results: 449 PASSED, 94 SKIPPED, 12 FAILED and 8 RESOURCE_DENIED. Summing those returned statistics gives 50,707 tests, 43 assertion failures and 3,373 skips. These partial totals do not describe a completed full run. `test_external_inspection` and `test_tracemalloc` have no returned result. There are 566 ordinary start records and 563 end records; no descendant-process census is claimed.

| Failed module | Returned assertion failures |
| --- | ---: |
| test.test_multiprocessing_fork.test_misc | 2 |
| test.test_multiprocessing_forkserver.test_misc | 2 |
| test.test_multiprocessing_spawn.test_manager | 3 |
| test.test_multiprocessing_spawn.test_misc | 2 |
| test_faulthandler | 7 |
| test_embed | 1 |
| test_interpreters | 2 |
| test_repl | 2 |
| test_pyrepl | 1 |
| test_profiling | 11 |
| test_threading | 9 |
| test_regrtest | 1 |

The raw failures include open-stackref reports during shutdown/fork, fatal interpreter-state lookup without a current GIL/thread state during asynchronous traceback reading, and unreadable remote frames or missing profiling samples. Nested children frequently abort with SIGABRT; ordinary workers still return standard FAILED results. The regrtest crash-output assertion observes SIGABRT instead of its intended SIGSEGV. These are actual failed checks, not environment skips or a successful checker run. The evidence does not establish one cause for every module.

Root recorded a pidfd SIGTERM to its owned timeout process at 08:47:32.382692 UTC after the known failures. The timeout subsequently returned 124 after 349.036 seconds; its actual ceiling was 5,368 seconds and the inclusive deadline was 10:14:41.673585 UTC. Thus 124 here records the deliberate root stop, not expiry of the original 5,400-second declared run allowance. No affected-result JSON, verification result or batch completion was synthesized.

I independently rehashed 7,187 plan inputs and all 22 ready artifacts, replayed the existing profile configuration/source/product checks against the 584-product build baseline, and checked current compiler-tool file identities without running compiler queries. Actual returned worker PIDs/configuration hashes, seed 0, no randomization, 16 workers, 1,200-second standard worker timeout, the sole existing remote-exec exclusion, module/extension origins and dynamic traceback source bindings agree. The returned boundaries provide 461 actual test origins; skipped/resource-denied results may lack an import origin. No module-selection, verifier or tracing-root claim has been added.

The retained native sweep targeted this image's exact Python and `_testembed` executables, signalled two remaining Python processes, and reported both idle. Its hash is bound by the post-gate cleanup receipt. The owned short cache was removed; cleanup completed before deadline. I also acquired the exact build lock and independently observed both executables idle. Original raw output, stop decision and outer failure remain unchanged.

The separate upstream-control design review accepts only one fresh checker build and three fixed methods. Shared upstream source paths support a causal comparison; they do not prove upstream native reproduction or excuse RC failures. Any qualification amendment requires the actual comparison and a separate decision. No runtime correction, new exclusion, target invocation or compiler invocation occurred in this review.

Evidence is retained under `build/rc-stackref-partial-outcome-review-20261101-v1/`; original native files remain under `build/worktrees/rc-successor/build/rc-stackref-debug-suite-v2/`.
