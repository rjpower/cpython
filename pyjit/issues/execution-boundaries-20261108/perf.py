"""Reuse the sealed warmed-region perf protocol for the logging checkpoint."""

import importlib.util
from pathlib import Path


PROTOCOL = Path(
    "/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/"
    "pyjit-link-wave1/interpreter-parity-20261108/final-diagnostic-perf.py"
)


def main():
    spec = importlib.util.spec_from_file_location("checkpoint_perf", PROTOCOL)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.CASES = ("logging_silent",)
    return module.main()


if __name__ == "__main__":
    raise SystemExit(main())
