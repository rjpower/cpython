#!/usr/bin/env python3
"""Fixed regrtest launcher with actual process-state witnesses."""

import argparse
import fcntl
import hashlib
import json
import os
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
    'executable_sha256', 'config_sha256', 'source_revision', 'source_sha256',
    'launcher_sha256', 'worker_sha256', 'validator_sha256', 'evidence', 'stage', 'selection_index',
    'inventory', 'inventory_sha256', 'jobs',
}
DIGEST_FIELDS = {
    'manifest_sha256', 'executable_sha256', 'config_sha256', 'source_sha256',
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
    if type(plan['version']) is not int or plan['version'] != 1:
        raise ValueError('unsupported launch-plan version')
    for field in DIGEST_FIELDS:
        require_digest(plan[field], field)
    for field in ('run', 'profile', 'source_revision'):
        verify.nonempty_string(plan[field], field)
    for field in ('manifest', 'build', 'evidence'):
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
    data = verify.validate_registry(verify.read_json(plan['manifest']), inventory=inventory)
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
        args = ['-q', '-u', 'cpu', 'test_cext']
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
    return args + ['--python', shlex.join([str(WORKER)])]


def write_record(plan, record):
    with Path(plan['evidence']).open('a') as stream:
        fcntl.flock(stream, fcntl.LOCK_EX)
        stream.write(json.dumps(record) + '\n')


def actual_state():
    import _testinternalcapi

    return _testinternalcapi.gc_verify_mode(), _testinternalcapi.gc_get_stress()


def runtime_identity(plan, profile):
    executable = Path(sys.executable).resolve()
    expected = Path(plan['build']).resolve() / 'python'
    if executable != expected or digest(executable) != plan['executable_sha256']:
        raise ValueError('unexpected or changed interpreter')
    if digest(executable.parent / 'Makefile') != plan['config_sha256']:
        raise ValueError('build configuration changed')
    debug = bool(sysconfig.get_config_var('Py_DEBUG'))
    configured = shlex.split(sysconfig.get_config_var('CONFIG_ARGS'))
    asan = '--with-address-sanitizer' in configured
    tailcall = '--with-tail-call-interp' in configured
    kind = profile['build_kind']
    if debug != (kind in ('debug', 'asan')) or asan != (kind == 'asan') or tailcall != (kind == 'tailcall'):
        raise ValueError('actual build kind disagrees with profile')
    if not __debug__ or sys.flags.optimize > 0:
        raise ValueError('verification profiles require Python assertions; -O/-OO are unsupported')


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

    original = run_workers.create_worker_process

    def create_worker_process(runtests, output_fd, tmp_dir=None):
        config = runtests.as_json()
        worker = json.loads(config)
        process = original(runtests, output_fd, tmp_dir)
        write_record(plan, dict(metadata, phase='scheduled', worker_pid=process.pid,
                                tests=worker['tests'], match_tests=worker['match_tests'],
                                worker_config_sha256=hashlib.sha256(config.encode()).hexdigest()))
        return process

    run_workers.create_worker_process = create_worker_process


def execute(plan, data, profile, role, command, plan_hash):
    metadata = {
        'run': plan['run'], 'profile': plan['profile'], 'stage': plan['stage'],
        'pid': os.getpid(), 'role': role, 'executable': sys.executable,
        'version': sys.version, 'git': sys._git,
        'warnoptions': sys.warnoptions[:], 'xoptions': sys._xoptions.copy(),
        'configuration': {key: sysconfig.get_config_var(key)
                          for key in ('CONFIG_ARGS', 'PY_CORE_CFLAGS', 'Py_DEBUG')},
        'python_assertions': __debug__,
        'source_revision': plan['source_revision'], 'source_sha256': plan['source_sha256'],
        'source_validation': 'supplied provenance; not checked by this launcher',
        'config_sha256': plan['config_sha256'], 'manifest_sha256': plan['manifest_sha256'],
        'launcher_sha256': plan['launcher_sha256'], 'worker_sha256': plan['worker_sha256'],
        'validator_sha256': plan['validator_sha256'], 'plan_sha256': plan_hash,
        'executable_sha256': plan['executable_sha256'], 'flags': str(sys.flags),
        'tests': [], 'match_tests': [], 'worker_config_sha256': None,
        'arguments': command,
    }
    try:
        runtime_identity(plan, profile)
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
                            worker_config_sha256=hashlib.sha256(command[2].encode()).hexdigest())
        elif module == 'platform' and len(command) == 2:
            metadata['role'] = 'auxiliary'
        else:
            raise ValueError('unsupported worker or auxiliary launch form')
        witness(plan, profile, metadata, 'start')
        sys.argv = command[1:]
        if role == 'driver':
            record_scheduling(plan, metadata)
        try:
            runpy.run_module(module, run_name='__main__', alter_sys=True)
        finally:
            witness(plan, profile, metadata, 'end')
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
