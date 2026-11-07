"""Bounded whole-kernel admission check for the retained R4 and current R5."""

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import statistics
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', required=True, type=Path)
    parser.add_argument('--after', required=True, type=Path)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    frozen = load('shape_measurement', Path(__file__).with_name('measure-canonical-home.py'))
    checkpoint = load('checkpoint_measurement', ROOT / 'issues/checkpoint-closure/measure.py')
    bench = load('kernel_measurement', ROOT / 'bench/run.py')
    executables = {'before': args.before.absolute(), 'after': args.after.absolute()}
    revisions = {'before': 'b58036e9718b87b95ad83b72e10d4578942a61d6',
                 'after': '0953757f4e7eb442896dcf42f6547009528defaa'}
    assert args.before.parent == args.after.parent
    kernels = [item.name for item in bench.BENCHMARKS if item.group == 'kernels']
    assert len(kernels) == len(set(kernels)) == 29
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', LD_BIND_NOW='1')

    def snapshot():
        probes = {role: checkpoint.interpreter_probe(path, env)
                  for role, path in executables.items()}
        for role, probe in probes.items():
            native = probe['git'][2]
            assert 7 <= len(native) <= 40 and revisions[role].startswith(native)
            assert probe['jit']['enabled'] is False
            assert not probe['method_jit']['available']
            assert all(row['error'] is None for row in probe['modules'].values())
        assert probes['before']['modules'] == probes['after']['modules']
        files = sorted((args.source / 'Lib').rglob('*.py'))
        files += sorted((ROOT / 'bench').rglob('*.py'))
        extensions = sorted(args.after.parent.glob('build/lib*/*.so'))
        soabi = probes['after']['config']['SOABI']
        assert len([path for path in extensions if path.name.endswith(f'.{soabi}.so')]) == 79
        files += extensions
        assert len(files) > 1000
        source = checkpoint.git_identity(args.source)
        assert source['head'] == revisions['after'] and not source['status']
        return {'executables': {role: frozen.identity(path) for role, path in executables.items()},
                'source': source, 'probes': probes,
                'cpu_affinity': sorted(os.sched_getaffinity(0)),
                'shared_files': {str(path): digest(path) for path in files},
                'drivers': {str(path): digest(path) for path in
                            (Path(__file__), ROOT / 'bench/run.py',
                             Path(frozen.__file__), Path(checkpoint.__file__))}}

    before = snapshot()
    assert before['executables']['before']['sha256'] != before['executables']['after']['sha256']
    (args.output / 'identity-before.json').write_text(json.dumps(before, indent=2))
    values = {role: {case: [] for case in kernels} for role in executables}
    journal = []
    failure = None
    deadline = time.monotonic() + 30 * 60
    try:
        for round_number in range(1, 5):
            order = tuple(executables) if round_number % 2 else tuple(reversed(executables))
            for role in order:
                while os.getloadavg()[0] >= 3:
                    journal.append({'phase': 'quiet-wait', 'load': os.getloadavg()})
                    if time.monotonic() >= deadline:
                        raise TimeoutError('quiet-load deadline expired')
                    time.sleep(5)
                assert time.monotonic() < deadline
                stem = f'r{round_number}-{role}'
                output = args.output / f'{stem}.json'
                command = [sys.executable, '-B', str(ROOT / 'bench/run.py'),
                           '--build', str(executables[role]), '--label', role,
                           '--set', 'kernels', '--metric', 'time', '--time-runs', '5',
                           '--max-load', '4', '-o', str(output)]
                load_before = os.getloadavg()
                with subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE, text=True,
                                      start_new_session=True) as child:
                    try:
                        stdout, stderr = child.communicate(timeout=300)
                    except subprocess.TimeoutExpired:
                        os.killpg(child.pid, signal.SIGKILL)
                        stdout, stderr = child.communicate()
                        (args.output / f'{stem}-stdout.txt').write_text(stdout)
                        (args.output / f'{stem}-stderr.txt').write_text(stderr)
                        journal.append({'command': command, 'failure': 'timeout',
                                        'load_before': load_before, 'load_after': os.getloadavg()})
                        raise
                    result = subprocess.CompletedProcess(command, child.returncode, stdout, stderr)
                load_after = os.getloadavg()
                (args.output / f'{stem}-stdout.txt').write_text(result.stdout)
                (args.output / f'{stem}-stderr.txt').write_text(result.stderr)
                journal.append({'command': command, 'returncode': result.returncode,
                                'load_before': load_before, 'load_after': load_after})
                (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
                if result.returncode or load_after[0] >= 4:
                    raise RuntimeError(f'{stem}: child or load guard failed; campaign rejected')
                report = json.loads(output.read_text())
                assert revisions[role].startswith(report['build']['git'][2])
                table = report['results']['time']
                assert set(table) == set(kernels)
                for case in kernels:
                    assert len(table[case]['values']) == 5
                    assert table[case]['value'] == statistics.median(table[case]['values'])
                    values[role][case].append(table[case]['value'])
                print(stem, 'complete', flush=True)
        summary = {case: {'paired_ratios': [a / b for a, b in
                                         zip(values['after'][case], values['before'][case])],
                          'ratio_of_medians': statistics.median(values['after'][case]) /
                                              statistics.median(values['before'][case])}
                   for case in kernels}
        (args.output / 'round-medians.json').write_text(json.dumps(values, indent=2))
        (args.output / 'summary.json').write_text(json.dumps(summary, indent=2))
    except BaseException as error:
        failure = f'{type(error).__name__}: {error}'
        raise
    finally:
        after = None
        postflight_error = None
        try:
            after = snapshot()
            (args.output / 'identity-after.json').write_text(json.dumps(after, indent=2))
        except Exception as error:
            postflight_error = f'{type(error).__name__}: {error}'
        (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
        (args.output / 'status.json').write_text(json.dumps(
            {'failure': failure, 'stable_identity': before == after,
             'postflight_error': postflight_error}, indent=2))
        if failure is None and (postflight_error or before != after):
            raise RuntimeError(f'identity postflight failed: {postflight_error}')


if __name__ == '__main__':
    main()
