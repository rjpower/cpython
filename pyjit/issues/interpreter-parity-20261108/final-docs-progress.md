# Final contract documentation sync

2026-11-08. Updated the four assigned design pages to match integrated
P1/P2 (`0539e0c1ff9`), M2b (`73c20472a9f`) and M1 (`d93b83800a5`). Recorded
M2b's checked single-live-NOP continuation and retained PC/word/wrapper
behavior; M1's owned exact-dict first-probe identity hit and shared ordered
retirement; and the canonical PyValue pointer-borrow domain. P3 is described as
source-reviewed but pending native post-review, with its guarded unbound
`CALL_PY_GENERAL` entry, shared initializer, post-allocation checks and generic
fallback. P3 landed at `10627c339fa` after final native review. Both builds
report 117 modules; both focused P3 runs passed 583 tests with eight skips.
Combined qualification and performance measurement remain pending.

The pages also state that the baseline register-word layout is unchanged and
that M1/M2b regeneration removes entries from the unused legacy optimizing
expansion table, which a future optimizing frontend must lower appropriately.
No parity, performance or full-qualification claim was added. No native work
or tests were run here; scope was documentation only. Next: coordinator review
and merged qualification evidence.
