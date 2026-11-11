"""Count real adjacent method/CALL0 activations in unchanged DeltaBlue.

Run explicitly with the candidate's stats binary, -S -B, JIT disabled and the
usual affinity/hash settings. The coordinator supplies an evidence directory.
No profiling hook or benchmark transformation is used. Py_STATS changes runtime
execution; these counts establish admission, not elapsed performance.
"""

import argparse
import importlib.util
import json
import os
from pathlib import Path
import sys
import sysconfig

import _opcode


HANDLER = 'LOAD_ATTR_METHOD_WITH_VALUES_CALL0'


def read_handler_counters(dump):
    prefix = f'opcode[{HANDLER}].'
    counters = {}
    for line in dump.read_text().splitlines():
        key, separator, value = line.strip().partition(' : ')
        if separator and key.startswith(prefix):
            field = key.removeprefix(prefix)
            if field in ('handler_id', 'execution_count', 'specialization.hit'):
                counters[field] = int(value)
    if counters.get('handler_id') != 326:
        raise RuntimeError(f'Expected handler326 in {dump}: {counters}')
    # Zero-valued counters are omitted by PRINT_STAT in Python/pystats.c.
    entries = counters.get('execution_count', 0)
    hits = counters.get('specialization.hit', 0)
    if not 0 <= hits <= entries:
        raise RuntimeError(f'Inconsistent admission counters: {counters}')
    return {'handler': HANDLER, 'handler_id': 326,
            'entries': entries, 'committed_activations': hits,
            'entries_without_commit': entries - hits}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stats_dir', type=Path,
                        help='Task-owned directory for retained native stats dump')
    args = parser.parse_args()
    if not sysconfig.get_config_var('Py_STATS'):
        raise RuntimeError('This census requires the candidate stats build')
    if sys.gettrace() is not None or sys.getprofile() is not None:
        raise RuntimeError('Trace/profile hooks change admission')
    if any(sys.monitoring.get_events(tool) for tool in range(6)):
        raise RuntimeError('Monitoring changes admission')
    if sys._jit.is_enabled():
        raise RuntimeError('Disable JIT for interpreter admission counts')
    stats_dir = args.stats_dir.resolve()
    stats_dir.mkdir(parents=True, exist_ok=True)
    configured = sys._xoptions.get('pystats_dir')
    if configured is not None and Path(configured).resolve() != stats_dir:
        raise RuntimeError('-X pystats_dir conflicts with the census directory')
    os.environ['PYTHON_PYSTATS_DIR'] = str(stats_dir)
    os.environ['PYTHON_PYSTATS_TAG'] = 'deltablue-adjacent-call-census'
    sys._stats_off()
    path = Path(__file__).resolve().parents[2] / 'bench/internal/kernels/deltablue.py'
    spec = importlib.util.spec_from_file_location('_adjacent_call_census', path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    module.run(30)
    before = set(stats_dir.glob('*.txt'))
    sys._stats_on()
    sys._stats_clear()
    try:
        module.run(1)
    finally:
        sys._stats_off()
    families = _opcode.get_specialization_stats()
    if not sys._stats_dump():
        raise RuntimeError('Native stats dump was not written')
    dumps = set(stats_dir.glob('*.txt')) - before
    if len(dumps) != 1:
        raise RuntimeError(f'Expected one new native stats dump, got {sorted(dumps)}')
    [dump] = dumps
    counters = read_handler_counters(dump)
    print(json.dumps({
        'native': sys._git,
        'executable': sys.executable,
        'kernel': str(path),
        'warmup_iterations': 30,
        'counted_iterations': 1,
        'stats_dump': str(dump),
        'adjacent_call': counters,
        'family_specialization': {name: families[name]
                                  for name in ('load_attr', 'call')},
        'limit': ('Real refined-handler entries and committed ordinary activations; '
                  'includes all qualifying sites, not per-function attribution or timing. '
                  'Entry-minus-commit combines pre-capture misses and captured CALL fallback.'),
    }, indent=2))


if __name__ == '__main__':
    main()
