#!/usr/bin/env python3
"""Fixed regrtest launcher with actual process-state witnesses."""

import argparse
import _imp
import dataclasses
import fcntl
import hashlib
import importlib
import json
import os
import re
import runpy
import shlex
import sys
import sysconfig
from pathlib import Path

# -I/-P omit the script directory; load the fixed sibling validator.
sys.path.insert(0, str(Path(__file__).resolve().parent))
import verify

PLAN_FIELDS = {
    'version', 'run', 'profile', 'manifest', 'manifest_sha256', 'build',
    'launcher_sha256', 'worker_sha256', 'validator_sha256', 'evidence', 'stage', 'selection_index',
    'inventory', 'inventory_sha256', 'jobs', 'identity', 'xml', 'pycache_prefix',
}
DIGEST_FIELDS = {
    'manifest_sha256',
    'launcher_sha256', 'worker_sha256', 'validator_sha256',
}
WORKER = Path(__file__).with_name('verify_worker.sh').resolve()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require_digest(value, where):
    if not isinstance(value, str) or len(value) != 64:
        raise ValueError(f'{where}: expected a SHA256 digest')
    int(value, 16)


def load_plan(path, expected_hash):
    require_digest(expected_hash, 'plan hash')
    if digest(path) != expected_hash:
        raise ValueError('launch plan changed')
    plan = verify.read_json(path)
    verify.object_fields(plan, PLAN_FIELDS, 'launch plan')
    if type(plan['version']) is not int or plan['version'] != 2:
        raise ValueError('unsupported launch-plan version')
    for field in DIGEST_FIELDS:
        require_digest(plan[field], field)
    for field in ('run', 'profile'):
        verify.nonempty_string(plan[field], field)
    for field in ('manifest', 'build', 'evidence', 'xml', 'pycache_prefix'):
        if not isinstance(plan[field], str) or not Path(plan[field]).is_absolute():
            raise ValueError(f'{field}: expected an absolute path')
    verify.integer(plan['jobs'], 1, 'jobs')
    if plan['stage'] not in ('main', 'cext'):
        raise ValueError('unknown launch stage')
    if digest(plan['manifest']) != plan['manifest_sha256']:
        raise ValueError('manifest changed after planning')
    inventory = None
    if plan['inventory'] is not None:
        if not isinstance(plan['inventory'], str) or not Path(plan['inventory']).is_absolute():
            raise ValueError('inventory: expected an absolute path')
        require_digest(plan['inventory_sha256'], 'inventory_sha256')
        if digest(plan['inventory']) != plan['inventory_sha256']:
            raise ValueError('inventory changed after planning')
        inventory = verify.read_json(plan['inventory'])
    elif plan['inventory_sha256'] is not None:
        raise ValueError('inventory hash without inventory')
    identity = verify.load_identity(plan['identity'])
    if str(Path(plan['build']).resolve()) != identity['build']:
        raise ValueError('plan build disagrees with execution identity')
    data = verify.validate_registry(verify.read_json(plan['manifest']),
                                    repo=Path(identity['runtime_root']), inventory=inventory)
    shown = verify.show_profile(data, plan['profile'], plan['manifest_sha256'], plan['build'], inventory)
    if shown['blocked']:
        raise ValueError(f'profile unavailable: {shown["blocked"]}')
    profile = data['profiles'][plan['profile']]
    selection = profile['selection']
    index = plan['selection_index']
    if index is not None:
        verify.integer(index, 0, 'selection_index')
        if selection['kind'] not in ('modules', 'cases'):
            raise ValueError('selection index on an unindexed profile')
        if index >= len(data['sets'][selection['set']]['items']):
            raise ValueError('selection index out of range')
    elif selection['kind'] == 'cases':
        raise ValueError('this profile requires an individual selection index')
    if plan['stage'] == 'cext' and profile['gate'] != 3:
        raise ValueError('test_cext stage requires gate3')
    if selection['kind'] == 'stale':
        raise ValueError('stale probes require their separate child contract')
    if (digest(__file__) != plan['launcher_sha256']
            or digest(WORKER) != plan['worker_sha256']
            or digest(verify.__file__) != plan['validator_sha256']):
        raise ValueError('launcher changed after planning')
    return plan, data, profile


def driver_arguments(plan, data, profile):
    if plan['stage'] == 'cext':
        # One isolated module worker supplies the same cext selection and
        # resources, with the worker identity/results contract attached.
        args = ['-q', '-u', 'cpu', '-j1', 'test_cext']
    else:
        timeout = {1: 600, 2: 900, 3: 1200}[profile['gate']]
        args = ['-q', '--timeout', str(timeout), f'-j{plan["jobs"]}']
        selection = profile['selection']
        if selection['kind'] in ('modules', 'cases'):
            items = data['sets'][selection['set']]['items']
            index = plan['selection_index']
            if selection['kind'] == 'cases':
                item = items[index]
                args.extend([item['module'], '-m', item['selector']])
            else:
                args.extend(items if index is None else [items[index]])
        for key in profile['exclusions']:
            args.extend(['-i', data['exclusions'][key]['selector']])
    return args + ['--junit-xml', plan['xml'], '--python', shlex.join([str(WORKER)])]


def write_record(plan, record):
    with Path(plan['evidence']).open('a') as stream:
        fcntl.flock(stream, fcntl.LOCK_EX)
        stream.write(json.dumps(record) + '\n')


def actual_state():
    import _testinternalcapi

    return _testinternalcapi.gc_verify_mode(), _testinternalcapi.gc_get_stress()


def module_origin(module, identity):
    return origin_record(module.__name__, module.__spec__.origin, identity)


def origin_record(name, origin, identity):
    if ((origin == 'built-in' and name in sys.builtin_module_names)
            or (origin == 'frozen' and _imp.is_frozen(name))):
        return {'module': name, 'origin': origin}
    if not isinstance(origin, str) or origin in ('built-in', 'frozen'):
        raise ValueError(f'unverified module origin: {name}: {origin}')
    alias = Path(os.path.abspath(origin))
    path = alias.resolve()
    root = Path(identity['runtime_root'])
    if path.is_relative_to(root / 'Lib'):
        base = root / 'Lib' / name.replace('.', '/')
        if path not in (base.with_suffix('.py').resolve(), (base / '__init__.py').resolve()):
            raise ValueError(f'substituted module origin: {name}: {origin}')
        snapshot = verify.read_json(verify.read_reference(identity['runtime_snapshot']))
        key = str(path.relative_to(root))
    elif alias.is_relative_to(Path(identity['build'])):
        if not path.is_relative_to(Path(identity['build'])):
            raise ValueError(f'external actual build origin: {name}: {origin}')
        if not path.name.startswith(name.rsplit('.', 1)[-1] + '.'):
            raise ValueError(f'substituted extension/config origin: {name}: {origin}')
        snapshot = verify.read_json(verify.read_reference(identity['outputs']))
        alias_key = str(alias.relative_to(identity['build']))
        if snapshot['files'].get(alias_key) != verify.file_record(alias):
            raise ValueError(f'module alias/hash mismatch: {name}: {origin}')
        key = str(path.relative_to(identity['build']))
    else:
        raise ValueError(f'unexpected actual module origin: {name}: {origin}')
    if snapshot['files'].get(key) != verify.file_record(path):
        raise ValueError(f'module origin/hash mismatch: {name}: {origin}')
    return {'module': name, 'origin': str(alias), 'resolved_origin': str(path),
            'sha256': digest(path)}


def runtime_identity(plan, profile, role, target):
    identity = verify.load_identity(plan['identity'], complete=role == 'driver')
    executable = Path(sys.executable).resolve()
    expected = Path(identity['build']) / 'python'
    outputs = verify.check_outputs(identity['outputs'], identity['build'], complete=False)
    for name in ('python', 'Makefile', 'pyconfig.h'):
        if outputs['files'].get(name) != verify.file_record(Path(identity['build']) / name):
            raise ValueError(f'changed interpreter/configuration: {name}')
    if executable != expected:
        raise ValueError('unexpected or changed interpreter')
    makefile = (executable.parent / 'Makefile').read_text()
    configured_source = next(line.split('=', 1)[1].strip()
                             for line in makefile.splitlines() if line.startswith('abs_srcdir='))
    actual_source = sysconfig.get_config_var('abs_srcdir')
    if (str((executable.parent / configured_source).resolve()) != identity['source_root']
            or not isinstance(actual_source, str)
            or str((executable.parent / actual_source).resolve()) != identity['source_root']):
        raise ValueError('actual configured source root mismatch')
    if sys.pycache_prefix != plan['pycache_prefix']:
        raise ValueError('actual fresh pycache prefix mismatch')
    if (Path(__file__).resolve().parent != Path(identity['tools_root'])
            or Path(verify.__file__).resolve().parent != Path(identity['tools_root'])
            or WORKER.parent != Path(identity['tools_root'])):
        raise ValueError('actual tool root mismatch')
    debug = bool(sysconfig.get_config_var('Py_DEBUG'))
    configured = shlex.split(sysconfig.get_config_var('CONFIG_ARGS'))
    asan = '--with-address-sanitizer' in configured
    tailcall = '--with-tail-call-interp' in configured
    kind = profile['build_kind']
    if debug != (kind in ('debug', 'asan')) or asan != (kind == 'asan') or tailcall != (kind == 'tailcall'):
        raise ValueError('actual build kind disagrees with profile')
    if not __debug__ or sys.flags.optimize > 0:
        raise ValueError('verification profiles require Python assertions; -O/-OO are unsupported')
    if profile['asan_options'] is not None:
        expected_asan = ':'.join(f'{key}={value}' for key, value in profile['asan_options'].items())
        if os.environ.get('ASAN_OPTIONS') != expected_asan:
            raise ValueError('actual ASan environment disagrees with profile')
    origins = []
    modules = ['sysconfig', '_testinternalcapi']
    if target != 'platform':
        modules.extend(['test', 'test.support', 'test.libregrtest', 'test.libregrtest.result',
                        'test.libregrtest.single', 'platform'])
    if role == 'driver':
        modules.extend(['test.libregrtest.main', 'test.libregrtest.run_workers',
                        'test.libregrtest.worker'])
    for name in modules:
        spec = importlib.util.find_spec(name)
        if spec is None:
            raise ValueError(f'missing infrastructure module: {name}')
        declared = origin_record(name, spec.origin, identity)
        actual = module_origin(importlib.import_module(name), identity)
        if declared != actual:
            raise ValueError(f'infrastructure origin changed during import: {name}')
        origins.append(actual)
    if role == 'worker':
        # run_workers imports worker. Pre-importing the runpy target makes
        # Python warn (and -Werror fail) before worker.main can execute.
        spec = importlib.util.find_spec(target)
        if spec is None or target in sys.modules:
            raise ValueError('worker/auxiliary target missing or imported before runpy')
        origins.append(origin_record(target, spec.origin, identity))
    if sysconfig._get_sysconfigdata_name() in sys.modules:
        origins.append(module_origin(sys.modules[sysconfig._get_sysconfigdata_name()], identity))
    return identity, origins


def witness(plan, profile, metadata, phase):
    mode, stress = actual_state()
    write_record(plan, dict(metadata, phase=phase, mode=mode, stress=stress))
    expected_mode = {'off': 0, 'retain': 1, 'kill': 2}[profile['verify']]
    extras = sum({'pymem': 1, 'structs': 2}[value] for value in profile['extras'])
    if mode != (expected_mode, extras) or stress != profile['stress']:
        raise ValueError(f'actual state mismatch at {phase}: mode={mode}, stress={stress}')


def record_scheduling(plan, metadata):
    # This is the binding used by regrtest's parallel scheduler. Capture
    # the same serialized config that create_worker_process passes on.
    from test.libregrtest import run_workers
    from test.libregrtest.result import TestResult
    from test.libregrtest import main

    original = run_workers.create_worker_process
    original_result = TestResult.from_json
    scheduled = {}

    def create_worker_process(runtests, output_fd, tmp_dir=None):
        config = runtests.as_json()
        worker = json.loads(config)
        if len(worker['tests']) != 1 or worker['tests'][0] in scheduled:
            raise ValueError('duplicate or non-isolated scheduled module')
        process = original(runtests, output_fd, tmp_dir)
        binding = dict(worker_pid=process.pid, tests=worker['tests'],
                       match_tests=worker['match_tests'], random_seed=worker['random_seed'],
                       worker_config_sha256=hashlib.sha256(config.encode()).hexdigest())
        scheduled[worker['tests'][0]] = binding
        write_record(plan, dict(metadata, phase='scheduled', **binding))
        return process

    def result_from_json(worker_json):
        result = original_result(worker_json)
        write_record(plan, dict(metadata, phase='returned', result=dataclasses.asdict(result),
                                **scheduled[result.test_name]))
        return result

    reporter_factory = main.get_reporter

    def get_reporter():
        reporter = reporter_factory()
        started, summary = reporter.run_started, reporter.summary

        def run_started(runtests):
            write_record(plan, dict(metadata, phase='selection', tests=runtests.tests,
                                    match_tests=runtests.match_tests, random_seed=runtests.random_seed))
            return started(runtests)

        def run_summary(results, state):
            write_record(plan, dict(metadata, phase='summary', state=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', state),
                                    stats=dataclasses.asdict(results.stats)))
            return summary(results, state)

        reporter.run_started, reporter.summary = run_started, run_summary
        return reporter

    run_workers.create_worker_process = create_worker_process
    TestResult.from_json = staticmethod(result_from_json)
    main.get_reporter = get_reporter


def record_worker_results(plan, metadata, identity):
    import unittest
    from test.libregrtest import single

    original_skip = unittest.TestResult.addSkip
    original_run = single.run_unittest
    original_load = single._load_run_test

    def add_skip(result, test, reason):
        original_skip(result, test, reason)
        write_record(plan, dict(metadata, phase='skipped', test_id=test.id(), reason=reason))

    def run_unittest(test_mod, runtests):
        origin = module_origin(test_mod, identity)
        write_record(plan, dict(metadata, phase='test-module', **origin))
        return original_run(test_mod, runtests)

    def load_run_test(result, runtests):
        name = single.abs_module_name(result.test_name, runtests.test_dir)
        spec = importlib.util.find_spec(name)
        if spec is None:
            raise ValueError(f'missing scheduled module: {name}')
        origin = origin_record(name, spec.origin, identity)
        write_record(plan, dict(metadata, phase='test-module-spec', **origin))
        return original_load(result, runtests)

    unittest.TestResult.addSkip = add_skip
    single.run_unittest = run_unittest
    single._load_run_test = load_run_test


def execute(plan, data, profile, role, command, plan_hash):
    metadata = {
        'run': plan['run'], 'profile': plan['profile'], 'stage': plan['stage'],
        'pid': os.getpid(), 'role': role, 'executable': sys.executable,
        'version': sys.version, 'git': sys._git,
        'warnoptions': sys.warnoptions[:], 'xoptions': sys._xoptions.copy(),
        'configuration': {key: sysconfig.get_config_var(key)
                          for key in ('CONFIG_ARGS', 'PY_CORE_CFLAGS', 'Py_DEBUG')},
        'python_assertions': __debug__,
        'identity': plan['identity'], 'manifest_sha256': plan['manifest_sha256'],
        'launcher_sha256': plan['launcher_sha256'], 'worker_sha256': plan['worker_sha256'],
        'validator_sha256': plan['validator_sha256'], 'plan_sha256': plan_hash,
        'flags': str(sys.flags), 'asan_options': os.environ.get('ASAN_OPTIONS'),
        'tests': [], 'match_tests': [], 'worker_config_sha256': None,
        'arguments': command,
    }
    try:
        if command[:1] == ['--']:
            command = command[1:]
        if len(command) < 2 or command[0] != '-m':
            raise ValueError('expected a fixed -m launch form')
        module = command[1]
        metadata['module'] = module
        if role == 'driver':
            if module != 'test' or command[2:] != driver_arguments(plan, data, profile):
                raise ValueError('driver arguments disagree with the named profile')
            metadata['arguments'] = command[2:]
        elif module == 'test.libregrtest.worker' and len(command) == 3:
            worker = json.loads(command[2])
            metadata.update(tests=worker['tests'], match_tests=worker['match_tests'],
                            random_seed=worker['random_seed'],
                            worker_config_sha256=hashlib.sha256(command[2].encode()).hexdigest())
        elif module == 'platform' and len(command) == 2:
            metadata['role'] = 'auxiliary'
        else:
            raise ValueError('unsupported worker or auxiliary launch form')
        identity, origins = runtime_identity(plan, profile, role, module)
        metadata.update(qualification=identity['qualification'], origins=origins,
                        source_context=verify.source_context(identity))
        witness(plan, profile, metadata, 'start')
        sys.argv = command[1:]
        if role == 'driver':
            record_scheduling(plan, metadata)
        elif metadata['role'] == 'worker':
            record_worker_results(plan, metadata, identity)
        try:
            runpy.run_module(module, run_name='__main__', alter_sys=True)
        except BaseException as exc:
            if not isinstance(exc, SystemExit) or exc.code not in (None, 0):
                metadata['module_error'] = f'{type(exc).__name__}: {exc}'
            raise
        finally:
            try:
                witness(plan, profile, metadata, 'end')
            finally:
                if role == 'driver':
                    verify.load_identity(plan['identity'], complete=True)
    except BaseException as exc:
        if not isinstance(exc, SystemExit) or exc.code not in (None, 0):
            write_record(plan, dict(metadata, phase='failure', error=f'{type(exc).__name__}: {exc}'))
        raise


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', required=True, type=Path)
    parser.add_argument('--role', required=True, choices=('driver', 'worker'))
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    plan_hash = os.environ['GATE_VERIFY_PLAN_SHA256']
    plan, data, profile = load_plan(args.plan, plan_hash)
    execute(plan, data, profile, args.role, args.command, plan_hash)


if __name__ == '__main__':
    main()
