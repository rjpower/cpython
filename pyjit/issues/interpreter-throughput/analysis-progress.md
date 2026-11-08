# Results analyzer progress

The read-only analyzer now uses the explicit archived stage names and parses
the real pyperf layout: eight module files per role, with benchmark metadata
merged over file metadata (including the five-result SciMark file). Kernel
rows are reconciled against their round-median cells, and abbreviated report
git tuples are checked against the preflight probe identity.

The archived 6a self-check passes: kernel C/U geomean 1.2867708613,
pyperformance subset C/U 1.09643692, and the three instruction ratios match
the retained reference values. Treating 6a as both current and baseline gives
1.0 for both sensitivity ratios. The perf-stat parser currently validates the
54-cell/event matrix and normalized values; independent checks of its campaign
identity and collection journal remain. No current campaign numbers have
been generated, and this artifact will not be used for reporting until that
review is complete and the current stages are complete.

The independent review accepts the arithmetic, stage layout, and raw-to-derived
value reconciliation for the declared campaign. It explicitly does not treat
this analyzer's perf-stat checks as validation of campaign identity or the
collection journal; those checks remain a separate audit.

Self-check command:

```sh
python3 pyjit/issues/interpreter-throughput/analyze-results.py \
  --baseline-root /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/checkpoint-closure \
  --output /tmp/publication-analysis-selfcheck --self-check
```

It exited 0 and wrote `analysis.json` and `analysis.md` under that temporary
output directory; the directory was removed after checking the derived values.
