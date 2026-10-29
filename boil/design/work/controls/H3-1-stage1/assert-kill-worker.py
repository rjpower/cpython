"""Run one regrtest worker only after witnessing the active kill verifier."""

import json
from pathlib import Path
import sys

import _testinternalcapi


actual = _testinternalcapi.gc_verify_mode()
assert actual == (2, 0), ("active verifier", actual, "expected", (2, 0))
record = {"mode": list(actual), "executable": sys.executable,
          "ignore_environment": sys.flags.ignore_environment,
          "isolated": sys.flags.isolated}
path = Path(sys.executable).parent.parent / "H3-1" / "kill-workers.jsonl"
with path.open("a") as stream:
    stream.write(json.dumps(record) + "\n")

assert sys.argv[-3:-1] == ["-m", "test.libregrtest.worker"], sys.argv
from test.libregrtest.worker import worker_process

worker_process(sys.argv[-1])
