"""Compare frozen R4/R5 executables on shape reads and folded-local controls."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import statistics
import subprocess
import time


CHILD = r'''
import gc, hashlib, json, os, sys, time
import _testinternalcapi as capi

mode, category, loops, samples = sys.argv[1:]
loops, samples = int(loops), int(samples)
assert not sys._jit.is_enabled()
method_api_absent_before = 'jit_method_natural' not in vars(capi)
assert method_api_absent_before
class Owner: pass
owner = Owner()
value = {'int': 42, 'float': 1.25, 'object': object(),
         'bigint': 1 << 100, 'nan': float('nan')}[category]
owner.value = value
namespace = {}
body = ('        owner.value\n' if mode == 'temporary' else
        '        result = owner.value\n')
source = 'def read(owner, n):\n    result = None\n    for _ in range(n):\n' + body + '    return result\n'
exec(source, namespace)
read = namespace['read']
read(owner, 20000)
def check():
    words = capi.get_ir(read.__code__, True)
    attrs = [word for word in words if word[0] == 'LOAD_ATTR_SHAPE']
    assert len(attrs) == 1, words
    nlocals = len(capi.get_co_localskinds(read.__code__))
    assert (attrs[0][1] < nlocals) == (mode == 'local'), attrs
    return {'attribute_word': attrs[0], 'nlocalsplus': nlocals}
before = check()
gc.disable()
values = []
for _ in range(samples):
    start = time.perf_counter_ns()
    result = read(owner, loops)
    elapsed = time.perf_counter_ns() - start
    assert result is (value if mode == 'local' else None)
    values.append(elapsed / loops)
after = check()
assert before == after
assert not sys._jit.is_enabled()
method_api_absent_after = 'jit_method_natural' not in vars(capi)
assert method_api_absent_after
imports = {}
for name, module in tuple(sys.modules.items()):
    spec = module.__spec__
    if spec is None:
        continue
    origin = spec.origin
    digest = None
    if origin and os.path.isfile(origin):
        with open(origin, 'rb') as file:
            digest = hashlib.sha256(file.read()).hexdigest()
    imports[name] = {'origin': origin, 'sha256': digest}
print(json.dumps({'git': sys._git, 'values_ns_per_loop': values,
                  'imports': imports, 'reader_source': source,
                  'cpu_affinity': sorted(os.sched_getaffinity(0)),
                  'method_api_absent_before': method_api_absent_before,
                  'method_api_absent_after': method_api_absent_after,
                  'before': before, 'after': after, 'gc_during_timing': False}))
'''


def identity(path):
    stat = path.stat()
    return {
        'path': str(path), 'device': stat.st_dev, 'inode': stat.st_ino,
        'size': stat.st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'notes': subprocess.check_output(['readelf', '-n', str(path)], text=True),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', required=True, type=Path)
    parser.add_argument('--after', required=True, type=Path)
    parser.add_argument('--before-revision', required=True)
    parser.add_argument('--after-revision', required=True)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--loops', type=int, default=1000000)
    parser.add_argument('--rounds', type=int, default=6)
    parser.add_argument('--samples', type=int, default=5)
    parser.add_argument('--cpu', type=int)
    args = parser.parse_args()
    assert args.loops > 0 and args.samples > 0
    assert args.rounds > 0 and args.rounds % 2 == 0
    if args.cpu is not None:
        assert args.cpu in os.sched_getaffinity(0)
    args.output.mkdir(parents=True, exist_ok=False)
    executables = {'before': args.before.absolute(), 'after': args.after.absolute()}
    revisions = {'before': args.before_revision, 'after': args.after_revision}
    before_stat, after_stat = args.before.stat(), args.after.stat()
    assert (before_stat.st_dev, before_stat.st_ino) != (after_stat.st_dev, after_stat.st_ino)
    def snapshot():
        return {
            'executables': {role: identity(path) for role, path in executables.items()},
            'source_head': subprocess.check_output(
                ['git', '-C', str(args.source), 'rev-parse', 'HEAD'], text=True).strip(),
            'source_status': subprocess.check_output(
                ['git', '-C', str(args.source), 'status', '--porcelain'], text=True),
            'collector_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        }
    before = snapshot()
    assert not before['source_status']
    assert before['source_head'].startswith(args.after_revision)
    (args.output / 'identity-before.json').write_text(json.dumps(before, indent=2))
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', LD_BIND_NOW='1')
    cells = [(mode, category) for mode in ('temporary', 'local')
             for category in ('int', 'float', 'object', 'bigint', 'nan')]
    imports = None
    pairs = {f'{mode}/{category}': [] for mode, category in cells}
    deadline = time.monotonic() + 15 * 60
    failure = None
    try:
        with (args.output / 'commands.jsonl').open('w') as journal:
            for round_index in range(args.rounds):
                offset = round_index % len(cells)
                for mode, category in cells[offset:] + cells[:offset]:
                    index = cells.index((mode, category))
                    order = ('before', 'after') if (round_index + index) % 2 == 0 else ('after', 'before')
                    medians = {}
                    for role in order:
                        while os.getloadavg()[0] >= 3:
                            journal.write(json.dumps({'phase': 'quiet-wait', 'round': round_index + 1,
                                                      'cell': f'{mode}/{category}', 'role': role,
                                                      'load': os.getloadavg()}) + '\n')
                            journal.flush()
                            if time.monotonic() >= deadline:
                                raise TimeoutError('quiet-load deadline expired')
                            time.sleep(5)
                        assert time.monotonic() < deadline
                        command = [str(executables[role]), '-S', '-B', '-c', CHILD,
                                   mode, category, str(args.loops), str(args.samples)]
                        if args.cpu is not None:
                            command = ['taskset', '-c', str(args.cpu), *command]
                        load_before = os.getloadavg()
                        result = subprocess.run(command, env=env, capture_output=True,
                                                text=True, timeout=60)
                        load_after = os.getloadavg()
                        stem = f'r{round_index + 1}-{mode}-{category}-{role}'
                        (args.output / f'{stem}-stdout.txt').write_text(result.stdout)
                        (args.output / f'{stem}-stderr.txt').write_text(result.stderr)
                        journal.write(json.dumps({'command': command, 'returncode': result.returncode,
                                                 'load_before': load_before, 'load_after': load_after}) + '\n')
                        journal.flush()
                        if result.returncode or load_after[0] >= 4:
                            raise RuntimeError(f'{stem}: child or load guard failed')
                        row = json.loads(result.stdout)
                        native_revision = row['git'][2]
                        assert re.fullmatch(r'[0-9a-f]{7,40}', native_revision)
                        assert revisions[role].startswith(native_revision)
                        if args.cpu is not None:
                            assert row['cpu_affinity'] == [args.cpu]
                        if imports is None:
                            imports = row['imports']
                        assert row['imports'] == imports, 'stdlib/DSO manifest changed'
                        assert len(row['values_ns_per_loop']) == args.samples
                        medians[role] = statistics.median(row['values_ns_per_loop'])
                    pairs[f'{mode}/{category}'].append(medians['after'] / medians['before'])
                print(f'round {round_index + 1} complete', flush=True)
        summary = {cell: {'paired_ratios': values,
                          'geomean_ratio': statistics.geometric_mean(values),
                          'median_ratio': statistics.median(values),
                          'min_ratio': min(values), 'max_ratio': max(values)}
                   for cell, values in pairs.items()}
        (args.output / 'summary.json').write_text(json.dumps(summary, indent=2))
    except BaseException as error:
        failure = f'{type(error).__name__}: {error}'
        raise
    finally:
        after = snapshot()
        (args.output / 'identity-after.json').write_text(json.dumps(after, indent=2))
        (args.output / 'status.json').write_text(json.dumps(
            {'failure': failure, 'stable_identity': before == after,
             'loops': args.loops, 'samples': args.samples, 'rounds': args.rounds,
             'cpu': args.cpu}, indent=2))
        if before != after:
            raise RuntimeError('source/executable/collector changed during timing')


if __name__ == '__main__':
    main()
