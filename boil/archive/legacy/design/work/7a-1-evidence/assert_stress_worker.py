import hashlib
import json
import os
from pathlib import Path
import runpy
import sys

import _testinternalcapi

assert sys.argv[1] == '-m', sys.argv[:3]
module = sys.argv[2]
worker_config = json.loads(sys.argv[3]) if module == 'test.libregrtest.worker' else None
tests = worker_config['tests'] if worker_config is not None else sys.argv[3:]
worker_config_hash = hashlib.sha256(sys.argv[3].encode()).hexdigest() if worker_config is not None else None
repo = Path(sys.executable).parents[2]
source_hashes = {
    name: hashlib.sha256((repo / name).read_bytes()).hexdigest()
    for name in ('Include/internal/pycore_tstate.h', 'Python/gc_live.c',
                 'Modules/_testinternalcapi.c', 'Lib/test/test_gc.py')
}
config_hash = hashlib.sha256((Path(sys.executable).parent / 'Makefile').read_bytes()).hexdigest()


def witness(phase):
    actual = _testinternalcapi.gc_get_stress()
    record = {
        'run': os.environ['BOIL_STRESS_RUN'], 'phase': phase, 'pid': os.getpid(),
        'module': module, 'tests': tests, 'stress': actual,
        'verify_mode': _testinternalcapi.gc_verify_mode(),
        'ignore_environment': sys.flags.ignore_environment, 'isolated': sys.flags.isolated,
        'executable': sys.executable, 'source_hashes': source_hashes,
        'config_sha256': config_hash, 'flags': str(sys.flags),
        'match_tests': worker_config['match_tests'] if worker_config is not None else None,
        'worker_config_sha256': worker_config_hash,
    }
    with open(os.environ['BOIL_STRESS_EVIDENCE'], 'a') as evidence:
        evidence.write(json.dumps(record) + '\n')
    assert actual == 1000, (phase, actual, tests)


witness('start')
sys.argv = sys.argv[2:]
try:
    runpy.run_module(module, run_name='__main__', alter_sys=True)
finally:
    witness('end')
