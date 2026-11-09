# Pyperformance quiet-wait progress

2026-11-07: Implemented the reviewed prelaunch wait for timed pyperformance
commands. Each quiet-wait and final-guard sample is journaled; a busy final
guard returns to prelaunch waiting within the same deadline. No launched
command is retried. Untimed RSS retains its immediate load guard. Four focused
host-only tests passed via `uv run --no-project python -m unittest discover -s
pyjit/bench/tests -p test_measure_pyperformance.py`; no native benchmarks or
builds were run. Independent review of the actual diff is pending.
