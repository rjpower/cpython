"""Compare warmed nbody work with golden using the retained FIFO protocol."""

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import time


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/frame-call-model-20261109'
SOURCE = BASE / 'worktrees/execution-boundaries-20261108'
PROTOCOL = BASE / 'pyjit-link-wave1/interpreter-parity-20261108/final-diagnostic-perf.py'
BUILDS = {'golden': BASE / 'pyvalue-rc-repr-release/python',
          'candidate': BASE / 'execution-boundaries-release/python'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(expected_head):
    head = subprocess.check_output(
        ['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'], text=True).strip()
    dirty = subprocess.check_output(
        ['git', '-C', str(SOURCE), 'status', '--porcelain'], text=True)
    assert head == expected_head and not dirty, (head, dirty)
    control = BASE / 'worktrees/pyvalue-rc-universal'
    control_head = subprocess.check_output(
        ['git', '-C', str(control), 'rev-parse', 'HEAD'], text=True).strip()
    control_dirty = subprocess.check_output(
        ['git', '-C', str(control), 'status', '--porcelain'], text=True)
    assert control_head == '0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae' and not control_dirty
    workload = Path(__file__).resolve().parents[2] / 'bench/internal/kernels/nbody.py'
    return dict(source_head=head, control_head=control_head,
                workload=digest(workload), executables={role: digest(path)
                for role, path in BUILDS.items()}, protocol=digest(PROTOCOL),
                driver=digest(Path(__file__)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tag')
    parser.add_argument('--mode', choices=('stat', 'record'), required=True)
    parser.add_argument('--source-head', required=True)
    parser.add_argument('--timings', type=Path, required=True)
    args = parser.parse_args()
    assert sorted(os.sched_getaffinity(0)) == [15]
    spec = importlib.util.spec_from_file_location('warmed_perf', PROTOCOL)
    protocol = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(protocol)
    protocol.CASES = ('nbody',)
    bench = protocol.benchmark_roster()['nbody']
    rows = json.loads((args.timings / 'commands.json').read_text())
    calibration = {role: [duration / row['loops'] for row in rows
                         if row.get('case') == 'nbody' and row.get('role') == role
                         for duration in row['values_ns']]
                   for role in BUILDS}
    assert all(len(values) >= 14 for values in calibration.values()), calibration
    output = EVIDENCE / args.tag
    output.mkdir(exist_ok=False)
    before = snapshot(args.source_head)
    protocol.write_json(output / 'identity-before.json', before)
    protocol.write_json(output / 'calibration.json', calibration)
    for role, executable in BUILDS.items():
        shutil.copy2(executable, output / f'{role}-python')
    env = protocol.child_environment()
    perf = shutil.which('perf')
    assert perf is not None
    rounds = 3 if args.mode == 'stat' else 1
    groups = protocol.EVENT_GROUPS[:2] if args.mode == 'stat' else (
        ('cycles-record', ('cycles:u',)),)
    invocations = []
    after = None
    try:
        for round_index in range(rounds):
            roles = list(BUILDS)
            if round_index % 2:
                roles.reverse()
            for role in roles:
                median = statistics.median(calibration[role])
                target = protocol.STAT_TARGET_NS if args.mode == 'stat' else protocol.RECORD_TARGET_NS
                loops = max(1, math.ceil(target / median))
                for name, events in groups:
                    waits, load, expired = protocol.wait_for_quiet_load(time.monotonic() + 300)
                    assert not expired, waits
                    result = protocol.make_invocation(
                        output, role, 'nbody', round_index + 1, name, events,
                        args.mode, BUILDS[role], bench, loops, env, median,
                        load, waits, perf)
                    invocations.append(result)
                    protocol.write_json(output / 'invocations.json', invocations)
                    print(json.dumps(dict(role=role, mode=args.mode,
                                          round=round_index + 1, group=name,
                                          valid=result['valid'])), flush=True)
                    assert result['valid'], result['errors']
    finally:
        after = snapshot(args.source_head)
        protocol.write_json(output / 'identity-after.json', after)
        protocol.write_json(output / 'status.json', dict(
            stable_identity=before == after, invocations=len(invocations),
            complete=len(invocations) == rounds * len(BUILDS) * len(groups),
            valid=all(row['valid'] for row in invocations)))
    assert before == after


if __name__ == '__main__':
    main()
