"""Four balanced component timing pairs with retained native identities."""

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import statistics
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]
sys.dont_write_bytecode = True


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def gcc_version(probe):
    match = re.search(r"\[(GCC [^]]+)\]", probe['version'])
    assert match, probe['version']
    return match.group(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('before', 'after', 'source', 'baseline', 'baseline-identity', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--before-revision', required=True)
    parser.add_argument('--after-revision', required=True)
    parser.add_argument('--module-log', required=True, action='append',
                        help='before=PATH or after=PATH')
    parser.add_argument('--cases', default='nbody,logging_silent,pickle')
    parser.add_argument('--rounds', type=int, choices=[4], default=4)
    parser.add_argument('--cpu', type=int, choices=[15], default=15)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    try:
        measure(args)
    except BaseException as error:
        status_path = args.output / 'status.json'
        status = json.loads(status_path.read_text()) if status_path.exists() else {'phase': 'preflight'}
        status.update(failure=f'{type(error).__name__}: {error}', complete=False)
        status_path.write_text(json.dumps(status, indent=2))
        if not (args.output / 'commands.json').exists():
            (args.output / 'commands.json').write_text('[]\n')
        raise


def measure(args):
    checkpoint = load('component_checkpoint', ROOT / 'issues/checkpoint-closure/measure.py')
    frozen = load('component_identity', ROOT / 'issues/interpreter-architecture-20261107/measure-canonical-home.py')
    bench = load('component_registry', ROOT / 'bench/run.py')
    executables = {'before': args.before.resolve(), 'after': args.after.resolve()}
    revisions = {'before': args.before_revision, 'after': args.after_revision}
    assert all(re.fullmatch(r'[0-9a-f]{40}', revision) for revision in revisions.values())
    assert revisions['before'] != revisions['after']
    assert args.cpu in os.sched_getaffinity(0)
    assert executables['before'].parent == executables['after'].parent
    assert (executables['before'].stat().st_dev, executables['before'].stat().st_ino) != (
        executables['after'].stat().st_dev, executables['after'].stat().st_ino)
    build = executables['after'].parent
    source, makefile = checkpoint.source_tree_for(build)
    assert source == args.source.resolve()
    roster = {item.name: item for item in bench.BENCHMARKS if item.group == 'kernels'}
    names = args.cases.split(',')
    assert len(names) == len(set(names)) and set(names) == {'nbody', 'logging_silent', 'pickle'}
    assert set(names) <= roster.keys()
    module_logs = {}
    for entry in args.module_log:
        role, path = entry.split('=', 1)
        assert role in executables and role not in module_logs
        module_logs[role] = Path(path).resolve()
    assert set(module_logs) == set(executables)
    baseline = json.loads(args.baseline.read_text())
    sealed = json.loads(args.baseline_identity.read_text())
    assert not sealed['errors']
    historic = sealed['builds']['candidate']
    assert baseline['source_commit'] == historic['git']['head'] == revisions['before']
    historic_native = historic['probe']['git'][2]
    assert re.fullmatch(r'[0-9a-f]{7,40}', historic_native)
    assert revisions['before'].startswith(historic_native)
    assert not historic['git']['status']
    assert Path(baseline['retained']).resolve() == executables['before']
    assert historic['source'] == str(source) and Path(historic['python']).parent == build
    extension_hashes = {path: value for path, value in baseline['input_hashes'].items()
                        if path.endswith('.so')}
    assert len(extension_hashes) == 213
    assert all(Path(path).parent.parent.parent == build for path in extension_hashes)
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', LD_BIND_NOW='1')

    # Preserve the registry, setup, warmup and timing boundaries of the native driver.
    driver = (ROOT / 'bench/internal/driver.py').read_text()
    driver = driver.replace('import importlib.util\n',
        'import importlib.util\nimport os\nimport hashlib\nimport _testinternalcapi as capi\n')
    gate = (f'        assert sorted(os.sched_getaffinity(0)) == [{args.cpu}]\n'
            '        assert not sys._jit.is_enabled()\n'
            "        assert 'jit_method_natural' not in vars(capi)\n"
            '        assert sys.flags.hash_randomization == 0\n'
            "        assert sys._git[2] == os.environ['PYJIT_COMPONENT_NATIVE_REVISION']\n")
    assert driver.count('        t0 = time.perf_counter_ns()\n') == 1
    driver = driver.replace('        t0 = time.perf_counter_ns()\n',
                            gate + '        t0 = time.perf_counter_ns()\n')
    metadata = '''    imports = {}
    for name, module in tuple(sys.modules.items()):
        if module is None:
            continue
        files = {}
        for key in ('__file__', '__cached__'):
            filename = vars(module).get(key)
            if filename and os.path.isfile(filename):
                with open(filename, 'rb') as stream:
                    files[key] = {'path': os.path.abspath(filename),
                                  'sha256': hashlib.sha256(stream.read()).hexdigest()}
        imports[name] = files
    print(json.dumps({'git': sys._git, 'imports': imports,
                      'affinity': sorted(os.sched_getaffinity(0)),
                      'jit_enabled': sys._jit.is_enabled(),
                      'hash_randomization': sys.flags.hash_randomization}))
'''
    assert driver.count('    print(json.dumps(durations))\n') == 1
    driver = driver.replace('    print(json.dumps(durations))\n',
        ''.join(line[4:] + '\n' for line in gate.splitlines()) + metadata +
        '    print(json.dumps(durations))\n')
    driver_path = args.output / 'native-driver.py'
    compile(driver, str(driver_path), 'exec')
    driver_path.write_text(driver)

    def snapshot():
        logs = {role: checkpoint.module_log_info('candidate', path)
                for role, path in module_logs.items()}
        identities = {role: frozen.identity(path) for role, path in executables.items()}
        assert digest(makefile) == historic['makefile_sha256']
        assert identities['before']['sha256'] == baseline['sha256']
        assert identities['before']['notes'] == baseline['readelf']
        assert checkpoint.elf_build_id(executables['before']) == historic['python_build_id']
        git = checkpoint.git_identity(source)
        assert git['head'] == revisions['after'] and not git['status']
        extensions = sorted(build.glob('build/lib*/*.so'))
        actual_extensions = {str(path.absolute()): digest(path) for path in extensions}
        assert actual_extensions == extension_hashes
        probes = {role: checkpoint.interpreter_probe(path, env)
                  for role, path in executables.items()}
        for role, probe in probes.items():
            native = probe['git'][2]
            assert re.fullmatch(r'[0-9a-f]{7,40}', native) and revisions[role].startswith(native)
            assert not probe['jit']['enabled'] and not probe['method_jit']['available']
            assert probe['debug_flag'] == probe['config']['Py_DEBUG'] == 0
            assert probe['config'] == historic['probe']['config']
            assert gcc_version(probe) == gcc_version(historic['probe'])
            assert all(row['error'] is None for row in probe['modules'].values())
            assert logs[role]['modules'] == logs[role]['expected_modules'] == 117
            assert logs[role]['missing'] == logs[role]['failed_import'] == 0
            assert checkpoint.elf_build_id(executables[role]) is not None
        assert probes['before']['modules'] == probes['after']['modules']
        files = sorted((source / 'Lib').rglob('*.py')) + sorted((source / 'Lib').rglob('*.pyc'))
        files += sorted((ROOT / 'bench').rglob('*.py'))
        files += sorted((ROOT / 'bench').rglob('*.pyc'))
        files += sorted(build.glob('build/lib*/*sysconfig*.py'))
        for directory in sorted(build.glob('build/lib*')):
            files += sorted(directory.rglob('*sysconfig*.pyc'))
        files += [makefile, build / 'pyconfig.h', args.baseline, args.baseline_identity,
                  Path(__file__), Path(checkpoint.__file__), Path(frozen.__file__), driver_path]
        assert len(files) > 1000
        return {'executables': identities, 'source': git, 'probes': probes, 'module_logs': logs,
                'cpu_affinity': sorted(os.sched_getaffinity(0)),
                'shared_inputs': {str(path.resolve()): digest(path) for path in files},
                'extensions': actual_extensions}

    journal, values, imports = [], {role: {name: [] for name in names} for role in executables}, {}
    before = None
    failure = None
    try:
        before = snapshot()
        assert before['executables']['before']['sha256'] != before['executables']['after']['sha256']
        (args.output / 'identity-before.json').write_text(json.dumps(before, indent=2))
        deadline = time.monotonic() + 30 * 60
        for round_index in range(args.rounds):
            for name in names:
                item = roster[name]
                order = tuple(executables) if round_index % 2 == 0 else tuple(reversed(executables))
                for role in order:
                    while os.getloadavg()[0] >= 3:
                        journal.append({'phase': 'quiet-wait', 'load': os.getloadavg()})
                        if time.monotonic() >= deadline:
                            raise TimeoutError('quiet-load deadline expired')
                        time.sleep(5)
                    assert time.monotonic() < deadline
                    command = ['taskset', '-c', str(args.cpu), str(executables[role]), '-S', '-B',
                               str(driver_path), 'time', str(item.path), str(item.loops),
                               '7', *([item.variant] if item.variant is not None else [])]
                    stem = f'r{round_index + 1}-{name}-{role}'
                    row = {'round': round_index + 1, 'case': name, 'role': role,
                           'command': command, 'load_before': os.getloadavg(), 'started': time.time()}
                    journal.append(row)
                    child_env = dict(env, PYJIT_COMPONENT_NATIVE_REVISION=before['probes'][role]['git'][2])
                    with subprocess.Popen(command, env=child_env, stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE, text=True, start_new_session=True) as child:
                        try:
                            stdout, stderr = child.communicate(timeout=120)
                        except subprocess.TimeoutExpired:
                            os.killpg(child.pid, signal.SIGKILL)
                            stdout, stderr = child.communicate()
                            (args.output / f'{stem}-stdout.txt').write_text(stdout)
                            (args.output / f'{stem}-stderr.txt').write_text(stderr)
                            row['failure'] = 'timeout'
                            raise
                    row.update(returncode=child.returncode, load_after=os.getloadavg(), finished=time.time())
                    (args.output / f'{stem}-stdout.txt').write_text(stdout)
                    (args.output / f'{stem}-stderr.txt').write_text(stderr)
                    assert child.returncode == 0 and row['load_after'][0] < 4, row
                    info, durations = map(json.loads, stdout.strip().splitlines()[-2:])
                    assert info['git'] == before['probes'][role]['git']
                    assert info['affinity'] == [args.cpu] and not info['jit_enabled']
                    assert info['hash_randomization'] == 0
                    assert len(durations) == 7 and all(type(v) is int and v > 0 for v in durations)
                    if name not in imports:
                        imports[name] = info['imports']
                    assert info['imports'] == imports[name], 'native imported-input manifest changed'
                    median = statistics.median(durations) / item.loops
                    values[role][name].append(median)
                    row.update(metadata=info, values_ns=durations, loops=item.loops,
                               median_ns_per_loop=median)
                    (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
            print(f'round {round_index + 1} complete', flush=True)
    except BaseException as error:
        failure = f'{type(error).__name__}: {error}'
        raise
    finally:
        after, postflight_error = None, None
        try:
            after = snapshot()
            (args.output / 'identity-after.json').write_text(json.dumps(after, indent=2))
        except Exception as error:
            postflight_error = f'{type(error).__name__}: {error}'
        (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
        (args.output / 'status.json').write_text(json.dumps(
            {'failure': failure, 'postflight_error': postflight_error,
             'stable_identity': before is not None and before == after,
             'complete': False}, indent=2))
        if failure is None and (postflight_error or before != after):
            raise RuntimeError(f'postflight identity failed: {postflight_error}')
    summary = {}
    for name in names:
        a, b = values['after'][name], values['before'][name]
        summary[name] = {'before_ns_per_loop': b, 'after_ns_per_loop': a,
                         'paired_ratios': [x / y for x, y in zip(a, b)],
                         'before_median_ns_per_loop': statistics.median(b),
                         'after_median_ns_per_loop': statistics.median(a),
                         'ratio_of_medians': statistics.median(a) / statistics.median(b)}
    (args.output / 'summary.json').write_text(json.dumps(summary, indent=2))
    (args.output / 'status.json').write_text(json.dumps(
        {'failure': None, 'postflight_error': None, 'stable_identity': True,
         'complete': True, 'cpu': args.cpu, 'rounds': args.rounds, 'samples': 7}, indent=2))


if __name__ == '__main__':
    main()
