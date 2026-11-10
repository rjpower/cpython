"""Capture unchanged lookup/call instruction workloads with frozen identities."""

import hashlib
import importlib.util
import json
import os
import csv
from pathlib import Path
import subprocess
import sys


def main():
    packet = Path(__file__).resolve().parent
    root = packet.parents[1]
    base = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
    evidence = base / 'pyjit-link-wave1/object-call-20261109'
    output = evidence / 'instructions41'
    output.mkdir(exist_ok=False)
    source = root / 'issues/checkpoint-closure/measure.py'
    spec = importlib.util.spec_from_file_location('micro_checkpoint', source)
    checkpoint = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = checkpoint
    spec.loader.exec_module(checkpoint)
    checkpoint.EXPECTED_MODULE_COUNTS['golden'] = 117
    checkpoint.CONTROL_HEADS['golden'] = '4d666e1b482cb4983c7683f91c5922af14d0ab40'
    builds = {
        'upstream': base / 'upstream-release-devlibs-3a440b6',
        'golden': base / 'golden-20261109-release',
        'candidate': base / 'execution-boundaries-release',
    }
    logs = {
        'upstream': base / 'pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log',
        'golden': evidence / 'golden39-build-sysroot.log',
        'candidate': evidence / 'object41-release-build.log',
    }
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', UV_CACHE_DIR='/home/power/.cache/uv')

    def snapshot():
        result = checkpoint.snapshot_all(builds, env, logs)
        paths = [Path(__file__), source, *sorted((root / 'bench').rglob('*.py'))]
        result['inputs'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in paths}
        result['executable_sha256'] = {
            role: hashlib.sha256((build / 'python').read_bytes()).hexdigest()
            for role, build in builds.items()}
        if result['builds']['candidate']['git'] != {
                'head': '0a35a382909ca169ce3f381cb38206b1725001b3', 'status': ''}:
            result['errors'].append('candidate revision or cleanliness changed')
        return result

    before = after = None
    failure = postflight_error = None
    rows = []
    try:
        before = snapshot()
        (output / 'identity-before.json').write_text(json.dumps(before, indent=2))
        assert not before['errors'], before['errors']
        for role, build in builds.items():
            command = ['taskset', '-c', '15', 'uv', 'run', '--no-project', 'python', '-B',
                       str(root / 'bench/run.py'), '--build', str(build), '--label', role,
                       '--set', 'micro', '--only', 'class_attr_load,call_py_kw',
                       '--metric', 'instr', '--instr-runs', '4', '--max-load', '3',
                       '--perf-records', str(output / f'{role}-perf.jsonl'),
                       '--output', str(output / f'{role}.json')]
            with (output / f'{role}.log').open('x') as stream:
                run = subprocess.run(command, env=env, stdout=stream,
                                     stderr=subprocess.STDOUT)
            row = dict(role=role, command=command, returncode=run.returncode,
                       coverage_verified=False)
            rows.append(row)
            (output / 'commands.json').write_text(json.dumps(rows, indent=2))
            assert run.returncode == 0, role
            records = [json.loads(line) for line in
                       (output / f'{role}-perf.jsonl').read_text().splitlines()]
            finished = [record for record in records if record['phase'] == 'finish']
            assert finished and len(records) == 2 * len(finished)
            for record in finished:
                assert record['returncode'] == 0 and 'error' not in record
                events = [fields for fields in csv.reader(record['perf_text'].splitlines())
                          if len(fields) > 2 and fields[2].startswith('instructions')]
                assert len(events) == 1
                [fields] = events
                assert len(fields) > 4 and float(fields[3]) > 0
                assert 99.9 <= float(fields[4]) <= 100.0, fields
            row['coverage_verified'] = True
            (output / 'commands.json').write_text(json.dumps(rows, indent=2))
            print(role, 'complete', flush=True)
    except BaseException as error:
        failure = f'{type(error).__name__}: {error}'
        raise
    finally:
        try:
            after = snapshot()
            (output / 'identity-after.json').write_text(json.dumps(after, indent=2))
        except BaseException as error:
            postflight_error = f'{type(error).__name__}: {error}'
            (output / 'identity-after-error.json').write_text(json.dumps(
                dict(error=postflight_error), indent=2))
        complete = (failure is None and postflight_error is None and
                    before is not None and before == after and not after['errors'] and
                    len(rows) == 3 and all(row['returncode'] == 0 and
                                          row['coverage_verified'] for row in rows))
        (output / 'status.json').write_text(json.dumps(dict(
            complete=complete, stable_identity=before is not None and before == after,
            failure=failure, postflight_error=postflight_error), indent=2))
        if failure is None and not complete:
            raise RuntimeError('instruction capture rejected; see status.json')


if __name__ == '__main__':
    main()
