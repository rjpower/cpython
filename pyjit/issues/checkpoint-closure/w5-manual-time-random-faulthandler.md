# Time, random, and faulthandler value-call increment

The apply-ready patch is [`w5-manual-time-random-faulthandler.patch`](w5-manual-time-random-faulthandler.patch), based on S worktree `build/checkpoint-bulk-20261105` at `e983958bedeca23a192f3a566d2f27b4a0c9a7db`.

`time.strptime` now has a value-native method entry, keeps fallback arguments as values, and creates fast-path struct-sequence integer fields without boxing. `Random.seed` keeps the absolute seed as a `PyValue` through long-span extraction and preserves the existing unsigned hash seeding path. Faulthandler keeps the startup `enable()` result as a value.

Validation in W5 shadows: the first time/random/faulthandler delta passed the time, random, and faulthandler suites (237 passed, 11 skipped). W5 original sources were restored and the release tree rebuilt successfully.
