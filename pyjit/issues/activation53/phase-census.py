"""Count observable activation phases in unchanged kernels; never a timing run."""

import importlib.util
import json
from pathlib import Path
import sys
import types


def collect_codes(value, codes, filename):
    if isinstance(value, types.FunctionType):
        collect_codes(value.__code__, codes, filename)
        return
    if isinstance(value, type) and value.__module__ == '_activation_kernel':
        for member in vars(value).values():
            if isinstance(member, (staticmethod, classmethod)):
                member = member.__func__
            collect_codes(member, codes, filename)
        return
    if not isinstance(value, types.CodeType) or value.co_filename != filename or value in codes:
        return
    codes.add(value)
    for constant in value.co_consts:
        collect_codes(constant, codes, filename)


def main():
    path = Path(sys.argv[1]).resolve()
    role = sys.argv[2]
    spec = importlib.util.spec_from_file_location('_activation_kernel', path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    setup = vars(module).get('setup')
    if setup is not None:
        setup(None)
    module.run(1)
    codes = set()
    for value in vars(module).values():
        collect_codes(value, codes, str(path))
    counts = {code: {} for code in codes}
    depth = peak_depth = 0
    def callback(event, delta):
        def record(code, *args):
            nonlocal depth, peak_depth
            row = counts[code]
            row[event] = row.get(event, 0) + 1
            depth += delta
            peak_depth = max(peak_depth, depth)
        return record
    tool = 5
    sys.monitoring.use_tool_id(tool, 'activation-phase-census')
    mask = 0
    for name, delta in (('PY_START', 1), ('PY_RESUME', 1), ('PY_YIELD', -1), ('PY_RETURN', -1), ('PY_UNWIND', -1)):
        event = getattr(sys.monitoring.events, name)
        mask |= event
        sys.monitoring.register_callback(tool, event, callback(name, delta))
    for code in codes:
        sys.monitoring.set_local_events(tool, code, mask)
    module.run(1)
    for code in codes:
        sys.monitoring.set_local_events(tool, code, 0)
    sys.monitoring.free_tool_id(tool)
    rows = []
    for code in sorted(codes, key=lambda code: (code.co_firstlineno, code.co_qualname)):
        row = {'name': code.co_qualname, 'line': code.co_firstlineno, 'events': counts[code],
               'nlocals': code.co_nlocals, 'stacksize': code.co_stacksize,
               'flags': code.co_flags}
        if role == 'candidate':
            row['constant_homes'] = len(code.co_constslots) // 8
        rows.append(row)
    print(json.dumps({'kernel': str(path), 'role': role, 'git': list(sys._git),
                      'loops': 1, 'warmup_loops': 1, 'observed_peak_depth': peak_depth,
                      'observed_final_depth': depth, 'rows': rows}, sort_keys=True))


if __name__ == '__main__':
    main()
