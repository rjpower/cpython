# RC composed-launch correction review

2026-11-01 06:51 UTC. **Accept exact v4 for a separately activated two-row
compatibility tranche.** The [failed v3 launch](rc-compatibility-launch-failure-20261101-v3.md)
stays failed; restored d7df source and existing images remain unchanged.

Capsule `09f614da8cea793f3e121835783741be870ec2f92b92f864b708e2582a70ec3f`,
plan `cf8e7ed8e1e8593c6956eb6e7e2ea7c8524e77970f19f455e2aebc53ee499b83`,
launch `5ce43c91967e6e73a5512de8f15679aaf2e77e2ae0bf8d95cfe0f3814bca8cff`.
Evidence: `build/worktrees/rc-successor/build/rc-compatibility-feedback-v4`.

Root checked the complete literal command: timeout, `/bin/sh`, compatibility
gate, build lock, build Python and host uv command occupy their correct roles.
One pure validator binds the exact entire command, verification command,
row identity, working directory and exclusive outputs. Preflight, runner entry
and row execution call it before activation checks, source queries or gate
execution. The runner then changes only the allowed downward timeout ceiling.

Independent replay passed four new controls: real debug/release rows, every
positional substitution, the actual malformed v3 command rejected before Popen,
and real canonical wrapper prefixes executed in fixtures with harmless commands.
Only fixture locks changed; tracked source bytes/mode/ownership stayed intact.
Thirty-nine bindings and7,159 current inputs, source/product/tool/import/output
and dual-executable idle checks passed. All unchanged child/result contracts
retain their separately verified eleven component controls; these were not
reported as eleven new executions. One old timeout mock uses deliberately
noncanonical arguments, so the new early validator correctly prevents that
mock from entering execution. Underlying timeout/cleanup code is unchanged.

Independent receipt `build/rc-compatibility-helper-root-review-20261101-v4/receipt.json`,
SHA256 `ea4f104a45b08e70372756354844ea484bd43b2438f34e7b9a16fc2a4b951e98`.
The first reader referenced an earlier metadata file; that failed reader is
preserved, and the final check uses actual v4 schema and frozen v3 artifacts.

Fixed scope remains50 debug/48 release methods, expected37 target programs,
zero skips, existing source/images,30-minute inclusive window and210-second
cleanup reserve. Stop at first unexpected failure. Root activation still
requires idle global/build locks. No native result, collector selection,
full qualification or integration follows from this review.
