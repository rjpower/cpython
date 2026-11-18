"""Balanced timings against upstream, golden39 or this round's frozen baseline."""

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
# Each invocation may wait this long for its core to become quiet, and is
# rerun at most this many times if other work disturbed it.
QUIET_WAIT_S = 20 * 60
MAX_ATTEMPTS = 8


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--control', required=True, type=Path)
    parser.add_argument('--control-role', choices=['upstream', 'golden'], default='upstream')
    parser.add_argument('--control-head', help='expected full golden-role commit; default is golden39')
    parser.add_argument('--candidate', required=True, type=Path)
    parser.add_argument('--module-log', action='append', required=True)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--cases', default='logging_silent')
    parser.add_argument('--group', choices=['kernels', 'micro'], default='kernels')
    parser.add_argument('--rounds', type=int, default=6)
    parser.add_argument('--single-pair-confirmation', action='store_true',
                        help='allow one confirmation pair; requires --rounds 1')
    parser.add_argument('--cpu', type=int, default=15)
    parser.add_argument('--loops', type=int, help='fixed micro loop count for both roles')
    args = parser.parse_args(argv)
    if args.single_pair_confirmation:
        if args.rounds != 1:
            parser.error('--single-pair-confirmation requires --rounds 1')
    elif args.rounds <= 0 or args.rounds % 2:
        parser.error('--rounds must be positive and even without --single-pair-confirmation')
    return args


def main(argv=None):
    args = parse_args(argv)
    assert args.loops is None or (args.group == 'micro' and args.loops > 0)
    assert args.cpu in os.sched_getaffinity(0)
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'run-request.json').write_text(json.dumps(
        {'mode': 'single-pair-confirmation' if args.single_pair_confirmation else 'balanced',
         'requested_rounds': args.rounds}, indent=2))
    checkpoint = load('checkpoint_measure', ROOT / 'issues/checkpoint-closure/measure.py')
    quiet = load('core_quiet', Path(__file__).with_name('core_quiet.py'))
    bench = load('kernel_registry', ROOT / 'bench/run.py')
    roster = {item.name: item for item in bench.BENCHMARKS if item.group == args.group}
    names = args.cases.split(',')
    assert len(names) == len(set(names)) and set(names) <= roster.keys()
    checkpoint.EXPECTED_MODULE_COUNTS['golden'] = 117
    checkpoint.CONTROL_HEADS['golden'] = args.control_head or '4d666e1b482cb4983c7683f91c5922af14d0ab40'
    assert args.control_head is None or args.control_role == 'golden'
    logs = checkpoint.parse_module_logs(args.module_log)
    assert set(logs) == {args.control_role, 'candidate'}
    builds = {args.control_role: args.control.resolve(),
              'candidate': args.candidate.resolve()}
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', LD_BIND_NOW='1')
    # Keep the original benchmark loader, warmup, timing boundaries and loop body.
    driver = (ROOT / 'bench/internal/driver.py').read_text()
    driver = driver.replace('import importlib.util\n',
                            'import importlib.util\nimport os\nimport _testinternalcapi as capi\n')
    gate = (f'        assert sorted(os.sched_getaffinity(0)) == [{args.cpu}]\n'
            '        assert not sys._jit.is_enabled()\n'
            "        assert 'jit_method_natural' not in vars(capi)\n")
    assert driver.count('        t0 = time.perf_counter_ns()\n') == 1
    driver = driver.replace('        t0 = time.perf_counter_ns()\n',
                            gate + '        t0 = time.perf_counter_ns()\n')
    driver = driver.replace('    print(json.dumps(durations))\n',
                            ''.join(line[4:] + '\n' for line in gate.splitlines()) +
                            "    print(json.dumps({'git': sys._git, 'affinity': sorted(os.sched_getaffinity(0)), "
                            "'jit_enabled': sys._jit.is_enabled(), 'hash_randomization': sys.flags.hash_randomization}))\n"
                            '    print(json.dumps(durations))\n')
    driver_path = args.output / 'native-driver.py'
    compile(driver, str(driver_path), 'exec')
    driver_path.write_text(driver)

    def snapshot():
        value = checkpoint.snapshot_all(builds, env, logs)
        paths = sorted((ROOT / 'bench').rglob('*.py'))
        paths += [Path(__file__), Path(checkpoint.__file__), Path(quiet.__file__), driver_path]
        value['driver_sources'] = {str(path): digest(path) for path in paths}
        return value

    before = snapshot()
    (args.output / 'identity-before.json').write_text(json.dumps(before, indent=2))
    if before['errors']:
        (args.output / 'commands.json').write_text('[]\n')
        (args.output / 'status.json').write_text(json.dumps(
            {'failure': 'preflight rejected', 'errors': before['errors'],
             'phase': 'preflight', 'stable_identity': None}, indent=2))
        raise RuntimeError(f"preflight rejected: {before['errors']}")
    journal = []
    pairs = {name: [] for name in names}
    gate = quiet.CoreGate(args.cpu)
    journal.append({'phase': 'core-gate', 'cpu': args.cpu, 'siblings': sorted(gate.siblings),
                    'l3_peers': sorted(gate.l3), 'limits': vars(gate.limits)})
    failure = None
    try:
        for round_index in range(args.rounds):
            for name in names:
                item = roster[name]
                loops = item.loops if args.loops is None else args.loops
                order = tuple(builds) if round_index % 2 == 0 else tuple(reversed(builds))
                means = {}
                for role in order:
                    python = Path(before['builds'][role]['python'])
                    command = ['taskset', '-c', str(args.cpu), str(python), '-S', '-B',
                               str(driver_path), 'time', str(item.path), str(loops),
                               '7', *([item.variant] if item.variant is not None else [])]
                    stem = f'r{round_index + 1}-{name}-{role}'
                    # Retry an invocation whose core was disturbed. Rejected attempts
                    # stay in the journal and on disk and never enter a paired ratio.
                    for attempt in range(1, MAX_ATTEMPTS + 1):
                        gate.wait_quiet(time.monotonic() + QUIET_WAIT_S, journal)
                        row = {'round': round_index + 1, 'case': name, 'role': role,
                               'attempt': attempt, 'command': command,
                               'load_before': os.getloadavg(), 'started': time.time()}
                        journal.append(row)
                        token = gate.begin()
                        with subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE, text=True,
                                              start_new_session=True) as child:
                            try:
                                stdout, stderr = child.communicate(timeout=120)
                            except subprocess.TimeoutExpired:
                                os.killpg(child.pid, signal.SIGKILL)
                                stdout, stderr = child.communicate()
                                (args.output / f'{stem}-stdout.txt').write_text(stdout)
                                (args.output / f'{stem}-stderr.txt').write_text(stderr)
                                row['failure'] = 'timeout'
                                raise
                        row.update(returncode=child.returncode, core_gate=gate.end(token),
                                   load_after=os.getloadavg(), finished=time.time())
                        assert child.returncode == 0, row
                        if row['core_gate']['ok']:
                            break
                        row['rejected'] = 'core interference'
                        (args.output / f'{stem}-rejected{attempt}-stdout.txt').write_text(stdout)
                        (args.output / f'{stem}-rejected{attempt}-stderr.txt').write_text(stderr)
                        (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
                    else:
                        raise RuntimeError(f'{stem}: core interference in all {MAX_ATTEMPTS} attempts')
                    (args.output / f'{stem}-stdout.txt').write_text(stdout)
                    (args.output / f'{stem}-stderr.txt').write_text(stderr)
                    lines = stdout.strip().splitlines()
                    assert len(lines) == 2, (
                        f"{name}/{role}: unexpected benchmark output; "
                        f"see {args.output / (stem + '-stdout.txt')}")
                    metadata, values = map(json.loads, lines[-2:])
                    assert metadata['git'] == before['builds'][role]['probe']['git']
                    assert metadata['affinity'] == [args.cpu] and not metadata['jit_enabled']
                    assert metadata['hash_randomization'] == 0
                    assert len(values) == 7 and all(value > 0 for value in values)
                    means[role] = statistics.mean(values)
                    row.update(metadata=metadata, values_ns=values, loops=loops, mean_ns=means[role])
                    (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
                pairs[name].append(means['candidate'] / means[args.control_role])
            print(f'round {round_index + 1} complete', flush=True)
        summary = {name: {'paired_ratios': values, 'geomean': statistics.geometric_mean(values),
                          'min': min(values), 'max': max(values)} for name, values in pairs.items()}
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
            if after['errors']:
                postflight_error = f"postflight rejected: {after['errors']}"
        except Exception as error:
            postflight_error = f'{type(error).__name__}: {error}'
        (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
        (args.output / 'status.json').write_text(json.dumps(
            {'failure': failure, 'postflight_error': postflight_error,
             'stable_identity': before == after}, indent=2))
        if failure is None and (postflight_error or before != after):
            raise RuntimeError('postflight identity failed')


if __name__ == '__main__':
    main()
