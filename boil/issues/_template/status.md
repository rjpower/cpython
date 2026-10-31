# Issue title

- State: queued
- Updated: YYYY-MM-DD
- Owner: unassigned
- Dependencies: named issues or decisions; otherwise none
- Branch/head: none assigned; name only an actual branch and commit
- Worktree: none assigned; name only an actual owned checkout

Use `queued`, `active`, `blocked`, `review`, `deferred` or `closed`.
For `closed`, add `Outcome: landed`, `reported`, `abandoned` or `superseded`.
Deferred work needs a resume condition; blocked work needs a concrete blocker.

## Now

State the current decision, supported scope and actual implementation or
validation position. Distinguish proposed, reviewed, built, measured and landed
work. Name source, native and integration identities separately when they
differ. Summarize the result; link its detailed record instead of copying logs.

## Next

Name one concrete next action and who must assign or unblock it. If work is
closed, state whether any action remains.

## Completion

Describe the user-visible outcome and the evidence or decision that closes
this issue. Preserve scope limits and link original failed attempts. Do not
turn an implementation checkpoint into qualification or a performance claim.

## References

Link the authoritative design, plan, review and results that exist. Only this
status file is required. Optional design, plan or evidence files should serve
distinct purposes; do not create empty companion documents. Keep immutable
receipts and referenced paths unchanged. This file records current status,
not the chronological history kept in WORKLOG and Git.
