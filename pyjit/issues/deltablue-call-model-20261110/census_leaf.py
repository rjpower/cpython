"""Count real leaf admissions during one unchanged, unprofiled DeltaBlue run."""

import importlib.util
import json
from pathlib import Path
import sys
import types

import _testinternalcapi


def functions(namespace):
    for value in namespace.values():
        if isinstance(value, types.FunctionType):
            yield value
        elif isinstance(value, type):
            for member in vars(value).values():
                if isinstance(member, (classmethod, staticmethod)):
                    member = member.__func__
                if isinstance(member, types.FunctionType):
                    yield member


def main():
    assert sys.gettrace() is None and sys.getprofile() is None
    assert not any(sys.monitoring.get_events(tool) for tool in range(6))
    path = Path(__file__).resolve().parents[2] / 'bench/internal/kernels/deltablue.py'
    spec = importlib.util.spec_from_file_location('_leaf_admission_census', path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    targets = {function.__code__ for function in functions(vars(module))
               if function.__code__.co_filename == str(path)}
    module.run(1)
    before = {code: _testinternalcapi.ir_leaf_info(code) for code in targets}
    module.run(1)
    rows = []
    for code in sorted(targets, key=lambda item: item.co_firstlineno):
        after = _testinternalcapi.ir_leaf_info(code)
        if not after[0]:
            continue
        rows.append({'function': code.co_qualname, 'line': code.co_firstlineno,
                     'hits': after[1] - before[code][1],
                     'probe_misses': after[2] - before[code][2],
                     'owner_misses': after[3] - before[code][3],
                     'entry_misses': after[4] - before[code][4],
                     'build_support': bool(after[5])})
    print(json.dumps({'native': sys._git, 'warmup_iterations': 1,
                      'counted_iterations': 1,
                      'limit': 'Actual admission counters, not timing or all semantic calls',
                      'functions': rows}, indent=2))


if __name__ == '__main__':
    main()
