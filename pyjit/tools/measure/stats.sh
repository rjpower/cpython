#!/bin/sh
# Usage: stats.sh BUILD BENCH OUT.json -- one pyperformance worker at -l 2
# under Py_STATS, summarised to JSON.
set -e
cd /home/power/code/cpython/.worktrees/python-boil-the-ocean
SP=$(ls -d build/pyperf-venvs/default/venv/*/lib/python3.16/site-packages | head -1)
rm -rf /tmp/py_stats /tmp/boil3b/pyc-$1 && mkdir -p /tmp/py_stats
PYTHONPYCACHEPREFIX=/tmp/boil3b/pyc-$1 PYTHONPATH=$SP build/$1/python -X pystats \
    $SP/benchmarks/bm_$2/run_benchmark.py --worker -l 2 -w 0 -n 1 -p 1 > /dev/null
python3 Tools/scripts/summarize_stats.py --json-output $3 > /dev/null
