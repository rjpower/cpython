"""Count semantic Python calls in one unchanged DeltaBlue iteration.

Profiling changes execution and disables optimizations. Counts guide general
call-shape selection; they are neither timings nor fast-path hit counts.
"""

from collections import Counter
import dis
import importlib.util
import json
from pathlib import Path
import sys


def main():
    root = Path(__file__).resolve().parents[2]
    path = root / 'bench/internal/kernels/deltablue.py'
    spec = importlib.util.spec_from_file_location('_call_shape_census', path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    calls = Counter()

    def profile(frame, event, arg):
        if event == 'call' and frame.f_code.co_filename == str(path):
            calls[frame.f_code] += 1

    sys.setprofile(profile)
    try:
        module.run(1)
    finally:
        sys.setprofile(None)
    rows = []
    for code, count in calls.most_common():
        rows.append({'function': code.co_qualname, 'line': code.co_firstlineno,
                     'calls': count, 'positional_args': code.co_argcount,
                     'locals': code.co_nlocals, 'flags': code.co_flags,
                     'words': [instruction.opname for instruction in
                               dis.get_instructions(code, adaptive=False)]})
    print(json.dumps({'native': sys._git, 'loops': 1, 'total_calls': sum(calls.values()),
                      'limit': 'Observed semantic counts, not timings or fast-path coverage',
                      'functions': rows}, indent=2))


if __name__ == '__main__':
    main()
