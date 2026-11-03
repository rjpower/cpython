# RC borrowed-header syntax outcome

2026-11-01. **ACCEPT the actual 19 syntax-only checks on dce654018d5ae16ffe5e772976d9ccc2c9d1e01a.** Root confirmed supervisor 27823 terminal 0. Exact executed plan remains `c78771259eef8af39e56f843b70a4642236c005ce1c5bc63e8536988d6ea682c`, under its reviewed 600-second timeout, 30-second kill grace and four nonblocking resource/build locks.

Independent audit matched the actual argv/activation, source/header/tool/configuration snapshot and result hashes. All 9,984 file records still reproduce the frozen plan. The raw log contains exactly the 19 expected successful source invocations, including both decimal variants and all SQLite/zstd files, with no warning/error/FAIL output. The target command returned 0, the result was within its inclusive deadline, and the owned UUID-stub scratch was empty then removed. The earlier missing scratch-directory issue was corrected before activation and is not hidden.

This accepts syntax against the explicitly borrowed headers and configured debug pyconfig only. It supplies no linking, extension import, ABI compatibility or runtime coverage. `_tkinter`, `_dbm`, `_gdbm` and readline remain explicitly unchecked; the UUID declaration stub remains a syntax-only interface.

Evidence is preserved in `build/rc-source-closure-20261101/{activation.json,syntax.log,result.json,run.py,plan.json}` and the independent receipt under `build/rc-source-syntax-outcome-review-20261101-v1/`. The independent review ran only read-only snapshots/hashes and git queries; no compiler, target, probe, rerun or activation occurred. This closes this source-syntax row, not the remaining runtime qualification.
