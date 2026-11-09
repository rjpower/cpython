# Direct `__new__` boxing probe

The stable stats executable was `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyvalue-rc-repr-stats/python`, ELF build ID `99fd50733810eff6d596cabe7d25efee3e019acb` before and after the three runs. Its embedded `sys._git` reports dirty source at `357baeea402`; the working runtime source was at `42696e9fcca`, so these counts prove the behavior of the retained executable, not the exact current source commit. The matching `tp_new_wrapper` and generated long/float code was inspected in the runtime source. Each command used `-I -S -B` and a private `-X pystats_dir` to avoid installed packages and cross-run dumps.

| Loop body, 100 iterations | Inline int boxes | Inline float boxes | Kind/site | Owner |
| --- | ---: | ---: | --- | --- |
| `pass` | 0 | 0 | none | none |
| `int.__new__(int, 17)` | 100 | 0 | thunk/call | `tp_new_wrapper` `0x277e6f`, at generated `long_new` thunk `0x1bcb9c` |
| `float.__new__(float, 1.5)` | 0 | 100 | thunk/call | `tp_new_wrapper` `0x277e6f`, at generated `float_new` thunk `0x18c615` |

Commands had the form `python -I -S -B -X pystats -X pystats_dir=<this directory>/<case> -c 'for i in range(100): <body>'`. Each child exited zero and wrote one raw `.txt` dump in its own case directory. `addr2line -afiCe` against the same ELF identified the PCs above. This is a bounded path proof; it does not estimate full-suite prevalence or benchmark performance. Both direct calls cross from a value method back into an object `tp_new` slot, then re-canonicalize the result. The proposed paired value-new row removes that internal round trip while retaining the object C ABI.
