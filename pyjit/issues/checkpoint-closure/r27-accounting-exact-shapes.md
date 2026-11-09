# R27 exact process-accounting packet

Apply `r27-accounting-exact-shapes.patch` to the current runtime candidate. It changes only the stats marker emitter, the suite reader/gate, and their focused tests. The patch applies cleanly to the runtime worktree as checked on 2026-11-05.

The existing crash and uncounted-tag lists still identify intended process families. They no longer suffice for admission: every leftover `.run` needs complete pid/ppid/tag/argv metadata and an exact `(tag, argv)` rule; every uncounted `.spawn` needs complete pid/tag/event/reason/argv metadata and an exact `(tag, reason, event, argv)` rule. Each rule has a positive maximum occurrence count and an explanation. Missing rules and excess occurrences fail the gate. Legacy tab-delimited spawn records remain visible for diagnosis but fail admission.

`Python/pystats.c` writes argv as one JSON array on the marker line. It escapes quotes, backslashes, control bytes, and non-ASCII bytes, preserving argument boundaries and embedded newlines. The reader rejects malformed JSON. Normalization replaces only the recorded build, source, and dump directory prefixes; other argv bytes remain exact. The new manifest deliberately has empty `runs` and `spawns` lists. It is fail-closed until the coordinator reviews and admits the finite shapes from a fresh suite on the new emitter. Do not populate it from the old flat markers: they lost argument boundaries.

Validation: 39 focused `test_check_r27` cases pass, one skipped, when the isolated test imports the isolated checker. A standalone C harness extracted the emitter helper, compiled with `-Wall -Wextra -Werror`, and verified an embedded newline serializes to valid one-line JSON. Both Python files compile. `git apply --check` passes. The shared `uv` cache is read-only in this sandbox, so the focused Python run used `/usr/bin/python3`; no cache was redirected.

Integration still needs a normal stats build, a fresh `.run`/`.spawn` capture, and manual admission of the observed exact process shapes. The first strict full-suite gate is expected to report those unreviewed shapes. No box-counter, owner, or crash rule is relaxed by this packet.
