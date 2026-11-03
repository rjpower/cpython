# RC compatibility launch failure

2026-11-01 06:46 UTC. **Fail the v3 launch; no selected-test credit.**
Root activated the reviewed two-row tranche at06:43:33, but the debug row
stopped immediately. The author and root missed a malformed argument chain:
`compat_gate.sh` replaced `/bin/sh` while the old gate script remained as the
next argument. The wrapper consequently treated that tracked script as its
lock and truncated it, then invoked the target Python on the `uv` ELF binary.
The retained SyntaxError establishes one uninstrumented target startup, not
execution of the98 selected methods. No target PID is invented.

The batch stopped at its first row. Its post-gate receipt confirms both Python
and embedding executables idle under the build lock. Root separately acquired
both global slots, serialization and both RC build locks, checked `/proc`, and
restored only `boil/tools/gate_run.sh` from exact committed d7df bytes. SHA256
`68bbe5d9914fc3f5a2e788a24328778e4dc52fca88e93719caca622ce79825c0`,
mode0775, original uid/gid. Git is clean again. The empty truncated file, raw
launch failure, cleanup and restoration evidence remain preserved.

Root receipt: `build/rc-compatibility-native-failure-review-20261101-v3/receipt.json`.
Original failed batch: `build/worktrees/rc-successor/build/rc-compatibility-feedback-v3`.
The earlier39-case passes and canonical reproduction are separate observations;
no new compatibility or qualification result is awarded.

Authorize only host correction through07:02/checkpoint06:55. Use one canonical
`timeout /bin/sh compat_gate LOCK PY CMD` argument contract in preflight and
runner before any gate/Popen. A real fixture invocation must prove the declared
lock alone is touched and malformed old arguments are rejected before execution.
Preserve v1–v3, all source and products; fresh helpers need independent review
and separate activation. No retry of v3, source change or target call is granted.
