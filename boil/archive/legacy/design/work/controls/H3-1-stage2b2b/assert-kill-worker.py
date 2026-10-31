"""Assert the collector in each worker, preserving regrtest setup."""

import hashlib
import json
from pathlib import Path
import runpy
import sys

if sys.argv[1:] == ["-m", "platform"]:
    sys.argv = ["platform"]
    runpy.run_module("platform", run_name="__main__")
    raise SystemExit(0)

import _testinternalcapi

actual = _testinternalcapi.gc_verify_mode()
assert sys.flags.optimize == 0 and __debug__
assert _testinternalcapi.gc_get_stress() == 0
assert actual == (2, 0), ("active verifier", actual, "expected", (2, 0))
assert sys.argv[-3:-1] == ["-m", "test.libregrtest.worker"], sys.argv
extension = Path(_testinternalcapi.__file__).resolve()
assert extension.is_relative_to(Path(sys.executable).resolve().parent), extension
record = {"mode": list(actual), "optimize": sys.flags.optimize,
          "stress": _testinternalcapi.gc_get_stress(), "__debug__": __debug__, "executable": sys.executable,
          "capi_origin": str(extension),
          "capi_sha256": hashlib.sha256(extension.read_bytes()).hexdigest(),
          "ignore_environment": sys.flags.ignore_environment,
          "isolated": sys.flags.isolated, "entrypoint": "worker.main",
          "tests": json.loads(sys.argv[-1])["tests"]}
path = Path(sys.executable).parent.parent / "H3-1-stage2b2b/kill-workers.jsonl"
with path.open("a") as stream:
    stream.write(json.dumps(record) + "\n")

from test.libregrtest.worker import main

sys.argv = ["test.libregrtest.worker", sys.argv[-1]]
main()
