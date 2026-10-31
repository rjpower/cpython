#!/bin/sh
# Usage: t.sh BUILD ARGS... -- run python from build/BUILD with a fresh
# pyc prefix, so that no stale pyc from an earlier compiler is loaded.
B=$1; shift
rm -rf /tmp/boil3b/pyc-run-$B
cd /home/power/code/cpython/.worktrees/python-boil-the-ocean
PYTHONPYCACHEPREFIX=/tmp/boil3b/pyc-run-$B exec build/$B/python "$@"
