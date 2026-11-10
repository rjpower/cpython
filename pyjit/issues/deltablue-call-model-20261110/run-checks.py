"""Run the established native integration checks into this round's evidence."""

import importlib.util
from pathlib import Path


def main():
    previous = Path(__file__).resolve().parent.parent / 'execution-boundaries-20261108/run-checks.py'
    spec = importlib.util.spec_from_file_location('native_checks', previous)
    checks = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checks)
    checks.EVIDENCE = checks.BASE / 'pyjit-link-wave1/deltablue-call-model-20261110'
    return checks.main()


if __name__ == '__main__':
    raise SystemExit(main())
