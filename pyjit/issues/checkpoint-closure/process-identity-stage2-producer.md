# R27 finished-process identity producer

Frozen source patch: [`process-identity-stage2-producer.patch`](process-identity-stage2-producer.patch), based on `af82c03c4e5`; only `Python/pystats.c` changes. It is not applied to the shared runtime. The matching reader and strict fixtures are separate work.

After a successful `.txt` publish, the producer renames the same-stem `.run` marker to `.done`, preserving the marker's start-time pid, ppid, tag and byte-escaped argv. The zero-stats branch renames `.run` to `.zero` rather than deleting it. A rename failure leaves `.run` evidence; a later stats interval gets a fresh stem so it cannot overwrite a prior dump. The sidecars are reporting metadata and do not change Python exceptions or stats counters.

`git apply --check` passed against the current runtime source. A syntax-only GCC compile of the isolated C file passed against the existing stats-build `pyconfig.h` and current source headers with `-Wall -Wextra -Werror -Wno-unused-parameter`. This is not linked or runtime-tested. Integration must pair it with a reader that retains folded dump stems and rejects missing, orphan, malformed or mismatched `.done`, every `.zero` without a reviewed disposition, and `.run` alongside a completed dump. Minimal real probes should cover normal exit, zero stats, fork child and repeated `sys._stats_dump()` intervals.
