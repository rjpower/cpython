"""Run one regrtest worker only after witnessing the active kill verifier."""

import json
from pathlib import Path
import runpy
import sys

# Verbose regrtest probes the host separately from its test workers.
if sys.argv[1:] == ["-m", "platform"]:
    sys.argv = ["platform"]
    runpy.run_module("platform", run_name="__main__")
    raise SystemExit(0)

import _testinternalcapi


actual = _testinternalcapi.gc_verify_mode()
assert actual == (2, 0), ("active verifier", actual, "expected", (2, 0))
record = {"mode": list(actual), "executable": sys.executable,
          "ignore_environment": sys.flags.ignore_environment,
          "isolated": sys.flags.isolated}
path = Path(sys.executable).parent.parent / "H3-worker-main" / "h3-workers.jsonl"
with path.open("a") as stream:
    stream.write(json.dumps(record) + "\n")

assert sys.argv[-3:-1] == ["-m", "test.libregrtest.worker"], sys.argv
from test.libregrtest.worker import main

# Preserve regrtest's worker cwd and exit timeout.
sys.argv = ["test.libregrtest.worker", sys.argv[-1]]
main()
