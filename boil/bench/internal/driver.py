"""Run one benchmark module inside the interpreter under test.

Usage: PYTHONHASHSEED=0 python -S -X pycache_prefix=DIR driver.py MODE PATH LOOPS [RUNS [VARIANT]]

The module at PATH exposes ``run(loops)``, the work that is measured, and
may expose ``setup(variant)``, the untimed preparation, called once first
with VARIANT (None if not given); a module holding several benchmarks picks
one by its variant.

MODE ``run`` calls ``run(LOOPS)`` once; the runner counts the whole process
(instructions, peak RSS) and differences or reports it.  MODE ``time`` makes
one warm-up call, then RUNS calls of ``run(LOOPS)`` timed with
``time.perf_counter_ns``, and prints the RUNS durations in nanoseconds as a
JSON list on the last line of stdout.

The module is loaded from its path under a private name, so a kernel named
after a stdlib module (pickle, logging) does not shadow it.  This file uses
only what every build under test has: no third-party module, no fork API.
"""

import importlib.util
import json
import sys
import time


def load(path):
    spec = importlib.util.spec_from_file_location("_boil_bench_module", path)
    module = importlib.util.module_from_spec(spec)
    # Registered so pickle and copy can find classes defined by the module.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def main(argv):
    mode, path, loops = argv[1], argv[2], int(argv[3])
    runs = int(argv[4]) if len(argv) > 4 else 1
    module = load(path)
    variant = argv[5] if len(argv) > 5 else None
    setup = getattr(module, "setup", None)
    if setup is not None:
        setup(variant)
    if mode == "run":
        module.run(loops)
        return 0
    if mode != "time":
        raise SystemExit(f"driver: unknown mode {mode!r}")
    module.run(loops)
    durations = []
    for _ in range(runs):
        t0 = time.perf_counter_ns()
        module.run(loops)
        durations.append(time.perf_counter_ns() - t0)
    print(json.dumps(durations))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
