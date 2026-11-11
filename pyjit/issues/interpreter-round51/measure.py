"""Use the established balanced measurement gates for round51 subtrees.

Pass explicit --control/--candidate build paths and --control-head for bf810.
See qualification-policy.md for frozen-baseline and upstream commands.
"""

import importlib.util
from pathlib import Path


def main():
    previous = Path(__file__).resolve().parent.parent / 'deltablue-call-model-20261110/measure.py'
    spec = importlib.util.spec_from_file_location('round51_measure', previous)
    measurement = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(measurement)
    return measurement.main()


if __name__ == '__main__':
    main()
