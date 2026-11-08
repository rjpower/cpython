# Measurement retry progress

2026-11-06: Implemented a per-cell retry in `checkpoint-closure/measure.py`.
Only the runner's exact load rejection with exit 1, empty stdout and no report
is retried. Each numbered attempt retains command, load samples, post-load,
stdout/stderr, report path and classification in the invocation journal. The
fixed order, quiet threshold, maximum-load guard and global deadline are
unchanged. Six focused host-only fixtures passed through `uv run`; no benchmark
or runtime build was run. Independent review of the actual diff and timeout
process cleanup is pending.

The final source uses a separate process session, stops the entire group on
exit timeout or interruption, and atomically replaces the attempt journal.
Eight focused fixtures passed with
`UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -m unittest discover -s pyjit/bench/tests -p test_checkpoint_measure.py`.
An earlier development run exposed a fake-clock fixture exhaustion and was
corrected before the passing run. No native benchmark or build was run by this
packet. Final source is frozen for independent review.
