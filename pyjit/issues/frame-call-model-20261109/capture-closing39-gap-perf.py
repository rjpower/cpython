#!/usr/bin/env python3
"""Collect current upstream/candidate PMU data with the sealed FIFO interval."""

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]
BUILD = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
PROTOCOL = BUILD / 'pyjit-link-wave1/interpreter-parity-20261108/final-diagnostic-perf.py'
CHECKPOINT = ROOT / 'issues/checkpoint-closure/measure.py'
REPEAT = BUILD / 'pyjit-link-wave1/frame-call-model-20261109/closing39-kernels-upstream'
CASES = ('deltablue', 'generators', 'nbody')
ROLES = ('upstream', 'candidate')
HEADS = {'upstream': '3a440b65695615e9fd9912342b79cdd54fced0d2',
         'candidate': '4d666e1b482cb4983c7683f91c5922af14d0ab40'}
EXTENSION_COUNTS = {'upstream': 78, 'candidate': 213}
sys.dont_write_bytecode = True


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def repeat_inputs(repeat, roster):
    before = json.loads((repeat / 'identity-before.json').read_text())
    after = json.loads((repeat / 'identity-after.json').read_text())
    status = json.loads((repeat / 'status.json').read_text())
    all_rows = json.loads((repeat / 'commands.json').read_text())
    quiet_rows = [row for row in all_rows if row.get('phase') == 'quiet-wait']
    rows = [row for row in all_rows if 'case' in row]
    require(len(all_rows) == len(rows) + len(quiet_rows),
            'unexpected timing campaign control rows')
    require(before == after and status == {'failure': None,
                                          'postflight_error': None,
                                          'stable_identity': True},
            'matched timing identity/status changed or failed')
    require(not before['errors'], 'matched timing identity contains errors')
    for path, expected_hash in before['driver_sources'].items():
        require(digest(Path(path)) == expected_hash,
                f'matched timing input changed: {path}')
    require(len(rows) == 2 * 29 * len(ROLES), 'incomplete 29-kernel timing rows')
    calibration = {role: {case: [] for case in CASES} for role in ROLES}
    seen = set()
    for row in rows:
        key = (row['round'], row['case'], row['role'])
        require(key not in seen and row['round'] in range(1, 3)
                and row['case'] in roster and row['role'] in ROLES,
                f'invalid repeat row: {key}')
        seen.add(key)
        require(row['returncode'] == 0 and row['loops'] > 0
                and len(row['values_ns']) == 7
                and all(isinstance(value, (int, float)) and value > 0
                        and math.isfinite(value) for value in row['values_ns']),
                f'invalid repeat values: {key}')
        identity = before['builds'][row['role']]
        bench = roster[row['case']]
        require(row['loops'] == bench.loops,
                f'matched timing workload shape changed: {key}')
        native = identity['probe']['git'][2]
        expected_command = ['taskset', '-c', '15', identity['python'], '-S', '-B',
                            str(repeat / 'native-driver.py'), 'time',
                            str(bench.path),
                            str(row['loops']), '7']
        if bench.variant:
            expected_command.append(bench.variant)
        require(row['command'] == expected_command,
                f'matched timing command changed: {key}')
        require(re.fullmatch(r'[0-9a-f]{7,40}', native) is not None
                and HEADS[row['role']].startswith(native)
                and row['metadata']['git'] == identity['probe']['git']
                and row['metadata']['affinity'] == [15]
                and row['metadata']['jit_enabled'] is False
                and row['metadata']['hash_randomization'] == 0,
                f'invalid repeat native conditions: {key}')
        if row['case'] in CASES:
            calibration[row['role']][row['case']].extend(
                value / row['loops'] for value in row['values_ns'])
    require(len(seen) == 116 and all(len(calibration[role][case]) == 14
                                  for role in ROLES for case in CASES),
            'incomplete calibration geometry')
    return before, calibration


def snapshot(identity, checkpoint, env, retained, input_files):
    roles = {}
    for role in ROLES:
        expected = identity['builds'][role]
        executable = Path(expected['python'])
        source = Path(expected['source'])
        actual_git = checkpoint.git_identity(source)
        require(actual_git == {'head': HEADS[role], 'status': ''}
                and expected['git'] == actual_git,
                f'{role} source revision or cleanliness changed')
        build = executable.parent
        source_from_make, makefile = checkpoint.source_tree_for(build)
        require(source_from_make == source and digest(makefile) == expected['makefile_sha256'],
                f'{role} configured source/Makefile changed')
        build_id = checkpoint.elf_build_id(executable)
        require(build_id == expected['python_build_id'] ==
                checkpoint.elf_build_id(retained[role]),
                f'{role} executable/retained build ID changed')
        require(digest(executable) == digest(retained[role]),
                f'{role} executable/retained ELF differs')
        probe = checkpoint.interpreter_probe(executable, env)
        previous = expected['probe']
        previous_without_ids = dict(previous)
        previous_without_ids['modules'] = {
            name: {key: value for key, value in module.items() if key != 'build_id'}
            for name, module in previous['modules'].items()}
        native = previous['git'][2]
        require(re.fullmatch(r'[0-9a-f]{7,40}', native) is not None
                and HEADS[role].startswith(native)
                and probe == previous_without_ids
                and probe['debug_flag'] == probe['config']['Py_DEBUG'] == 0
                and probe['jit']['enabled'] is False
                and probe['method_jit']['available'] is False,
                f'{role} native/config/JIT probe changed')
        for name, module in previous['modules'].items():
            if module['build_id'] is not None:
                require(checkpoint.elf_build_id(Path(module['file'])) == module['build_id'],
                        f'{role} imported {name} build ID changed')
        log = expected['module_log']
        current_log = checkpoint.module_log_info(
            'upstream' if role == 'upstream' else 'candidate', Path(log['path']))
        require(current_log == log and log['modules'] == (116 if role == 'upstream' else 117)
                and log['missing'] == log['failed_import'] == 0,
                f'{role} module gate changed')
        extensions = sorted(build.glob('build/lib*/*.so'))
        require(len(extensions) == EXTENSION_COUNTS[role],
                f'{role} extension set changed')
        roles[role] = {'source': actual_git, 'probe': probe,
                       'build_id': build_id, 'executable_sha256': digest(executable),
                       'retained_sha256': digest(retained[role]),
                       'module_log': current_log,
                       'extensions': {str(path.absolute()): digest(path)
                                      for path in extensions}}
    files = {str(path.absolute()): digest(path) for path in input_files}
    shared_config = ('CC', 'CFLAGS', 'OPT', 'Py_DEBUG', 'Py_TRACE_REFS',
                     'WITH_MIMALLOC', 'WITH_PYMALLOC')
    for key in shared_config:
        require(roles['upstream']['probe']['config'][key] ==
                roles['candidate']['probe']['config'][key],
                f'upstream/candidate build configuration differs: {key}')
    return {'roles': roles, 'inputs': files,
            'affinity': sorted(os.sched_getaffinity(0))}


def record_check(row, role, identity, perf, env, output):
    expected = identity['builds'][role]
    child = row.get('child') or {}
    for phase in ('native_before', 'native_after'):
        native = child.get(phase) or {}
        require(native.get('git') == expected['probe']['git']
                and native.get('affinity') == [15]
                and native.get('jit_enabled') is False
                and native.get('natural_method_api_present') is False
                and native.get('hash_randomization') == 0,
                f'{role} child {phase} revision/native gate failed')
    if row['mode'] != 'record':
        return
    listing = row['buildid_list']
    text = Path(listing['stdout']).read_text().lower()
    require(expected['python_build_id'] in text,
            f'{role} executable build ID absent from recorded hits')
    command = [perf, 'script', '-D', '-i', row['perf_data']]
    dump = subprocess.run(command, capture_output=True, text=True, env=env,
                          timeout=60)
    stem = row['stem']
    (output / f'{stem}-dump-stderr.txt').write_text(dump.stderr)
    # The dump is large. Keep the event count and lost-record lines, while
    # perf.data itself remains the complete raw record.
    lost = [line for line in (dump.stdout + '\n' + dump.stderr).splitlines()
            if re.search(r'PERF_RECORD_LOST|PERF_RECORD_LOST_SAMPLES', line)]
    (output / f'{stem}-lost.txt').write_text('\n'.join(lost) + ('\n' if lost else ''))
    require(dump.returncode == 0 and not lost,
            f'{role} perf sample dump failed or contains lost records')


def collect(args, output):
    require(args.mode == 'stat' or args.stat_group == 'cycles-instructions',
            'record mode does not accept a stat group')
    protocol = load('sealed_perf_protocol', PROTOCOL)
    checkpoint = load('perf_checkpoint', CHECKPOINT)
    protocol.CASES = CASES
    require(sorted(os.sched_getaffinity(0)) == [15], 'orchestrator must run on CPU 15')
    roster = protocol.benchmark_roster()
    identity, calibration = repeat_inputs(args.repeat, roster)
    require(tuple(args.cases) and len(args.cases) == len(set(args.cases))
            and set(args.cases) <= set(CASES), 'invalid case selection')
    executables = {role: Path(identity['builds'][role]['python']) for role in ROLES}
    env = protocol.child_environment()
    perf = shutil.which('perf')
    require(perf is not None, 'perf not found')
    perf = str(Path(perf).resolve())
    retained = {}
    for role in ROLES:
        target = output / f'{role}-python'
        shutil.copy2(executables[role], target)
        retained[role] = target
    input_files = [Path(__file__), PROTOCOL, CHECKPOINT]
    input_files += [args.repeat / name for name in ('identity-before.json',
                    'identity-after.json', 'status.json', 'commands.json')]
    input_files += [Path(path) for path in identity['driver_sources']]
    input_files += sorted((ROOT / 'bench').rglob('*.py'))
    input_files += sorted((ROOT / 'bench').rglob('*.pyc'))
    for role in ROLES:
        expected = identity['builds'][role]
        source = Path(expected['source'])
        build = executables[role].parent
        input_files += sorted((source / 'Lib').rglob('*.py'))
        input_files += sorted((source / 'Lib').rglob('*.pyc'))
        input_files += [build / 'Makefile', build / 'pyconfig.h',
                        Path(expected['module_log']['path'])]
        for library in build.glob('build/lib*'):
            input_files += sorted(library.rglob('*sysconfig*.py'))
            input_files += sorted(library.rglob('*sysconfig*.pyc'))
    require(len(input_files) > 1000, 'input census incomplete')
    before = snapshot(identity, checkpoint, env, retained, input_files)
    protocol.write_json(output / 'identity-before.json', before)
    protocol.write_json(output / 'calibration.json', calibration)
    shutil.copy2(args.repeat / 'commands.json', output / 'calibration-commands.json')
    results = []
    rounds = 3 if args.mode == 'stat' else 1
    if args.mode == 'record':
        group = ('cycles-record', ('cycles:u',))
    else:
        groups = dict(protocol.EVENT_GROUPS)
        require(args.stat_group in ('cycles-instructions', 'branches')
                and args.stat_group in groups, 'unsupported stat group')
        group = (args.stat_group, groups[args.stat_group])
    failure = postflight_error = None
    after = None
    deadline = time.monotonic() + 45 * 60
    try:
        for round_index in range(rounds):
            for case in args.cases:
                roles = ROLES if round_index % 2 == 0 else tuple(reversed(ROLES))
                for role in roles:
                    median = statistics.median(calibration[role][case])
                    target = (protocol.STAT_TARGET_NS if args.mode == 'stat'
                              else protocol.RECORD_TARGET_NS)
                    loops = max(1, math.ceil(target / median))
                    waits, load_average, expired = protocol.wait_for_quiet_load(deadline)
                    require(not expired, f'{case}/{role} quiet-load wait expired')
                    row = protocol.make_invocation(
                        output, role, case, round_index + 1, *group,
                        args.mode, executables[role], roster[case], loops, env,
                        median, load_average, waits, perf)
                    results.append(row)
                    protocol.write_json(output / 'invocations.json', results)
                    require(row['valid'],
                            f'{case}/{role} perf invocation failed: {row["errors"]}')
                    try:
                        record_check(row, role, identity, perf, env, output)
                    except BaseException as exc:
                        row['errors'].append(f'post-interval check: {type(exc).__name__}: {exc}')
                        row['valid'] = False
                        protocol.write_json(output / f'{row["stem"]}-result.json', row)
                        protocol.write_json(output / 'invocations.json', results)
                        raise
    except BaseException as exc:
        failure = f'{type(exc).__name__}: {exc}'
    finally:
        try:
            after = snapshot(identity, checkpoint, env, retained, input_files)
            protocol.write_json(output / 'identity-after.json', after)
        except BaseException as exc:
            postflight_error = f'{type(exc).__name__}: {exc}'
            protocol.write_json(output / 'identity-after-error.json',
                                {'error': postflight_error})
    return before, results, after, failure, postflight_error


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('stat', 'record'), required=True)
    parser.add_argument('--stat-group', choices=('cycles-instructions', 'branches'),
                        default='cycles-instructions')
    parser.add_argument('--cases', default=','.join(CASES))
    parser.add_argument('--repeat', type=Path, default=REPEAT)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.cases = tuple(args.cases.split(','))
    args.output.mkdir(parents=True, exist_ok=False)
    before = after = None
    rows = []
    error = postflight_error = None
    try:
        before, rows, after, error, postflight_error = collect(args, args.output)
    except BaseException as exc:
        error = f'{type(exc).__name__}: {exc}'
        raise
    finally:
        if (args.output / 'invocations.json').exists():
            rows = json.loads((args.output / 'invocations.json').read_text())
        stable = before is not None and after is not None and before == after
        status = {'mode': args.mode, 'cases': args.cases,
                  'scheduled_invocations': (3 if args.mode == 'stat' else 1)
                                           * len(args.cases) * len(ROLES),
                  'invocations': len(rows), 'valid_invocations': sum(
                      bool(row.get('valid')) for row in rows),
                  'stable_identity': stable, 'failure': error,
                  'postflight_error': postflight_error,
                  'complete': error is None and postflight_error is None and stable
                              and len(rows) == ((3 if args.mode == 'stat' else 1)
                                                * len(args.cases) * len(ROLES))
                              and all(row.get('valid') for row in rows)}
        (args.output / 'status.json').write_text(json.dumps(status, indent=2) + '\n')
    require(status['complete'], 'PMU campaign failed; see status.json')


if __name__ == '__main__':
    main()
