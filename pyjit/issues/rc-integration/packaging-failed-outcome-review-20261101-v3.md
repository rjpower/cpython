# Independent packaging v3 stopped outcome

2026-11-01. **FAILED regeneration gate; no later packaging or extension credit.**
Root confirmed supervisor 48281 terminal1. The actual first command,
`make -s regen-cases clinic`, returned0; its enclosing gate returned1 after
38.064 seconds because the tracked source snapshot changed. This is a real
failed invariant, not a successful regeneration or a failed extension test.

I independently reconciled all 7,064 tracked snapshot entries. The sole changed
path is `Python/clinic/sysmodule.c.h`; its permission mode remains0664. The diff
removes two blank lines, an empty `SYS_GETREFCOUNT_METHODDEF` fallback guard and
updates the generated output checksum. Its bytes remain changed in the worktree
at historical HEAD `8e132becdad4`. No source was restored or committed by this
review. All other 7,143 plan inputs rehash exactly.

The raw log shows configure reran. Against the sealed four-profile pre-run
inventories, actual debug product drift is confined to `config.log` and
`pybuilddir.txt`. Debug Makefile, pyconfig.h, Python, _testembed and compiled
products remain unchanged. Release, ASan and stackref inventories match
completely. This does not make the old strict product guard pass: its two
changed products need an explicit future disposition and binding. No new
configuration or image equivalence is inferred from make's exit code.

The source-drift assertion prevented the first row's verification. No launch
or result exists for debug/release packaging, the four public foreign-wheel
calls, or the C-extension row. No batch completion exists. The recorded
first-failure stop remains accurate. Cleanup records exact Python/_testembed
paths idle, a successful pidfd sweep, removed owned short cache and completion
before the original deadline. This is the declared cleanup scope, not a census
of all make descendants.

Any adoption of the generated result requires a separately reviewed canonical
source/equivalence proposal, exact changed-product bridge and fresh concrete
grant. This review accepts neither that source correction nor another run.
All failed artifacts and prior qualification remain at their actual scopes.

The independent audit used retained logs/snapshots and file hashes only. No
target, compiler, build, retry, install or source mutation occurred. Its initial
review-only0664/0644 permission assumption was corrected against both snapshots;
the original audit script is retained. Evidence and hashes are in
`build/rc-packaging-failed-outcome-review-20261101-v3/receipt.json`.
