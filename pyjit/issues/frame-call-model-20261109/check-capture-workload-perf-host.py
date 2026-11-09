"""Enter the real PMU collector without starting a native or perf process."""

import importlib.util
from pathlib import Path
import subprocess
import tempfile
from unittest import mock


SCRIPT = Path(__file__).with_name('capture-workload-perf.py')
spec = importlib.util.spec_from_file_location('capture_workload_perf', SCRIPT)
collector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(collector)
protocol = collector.load('host_perf_protocol', collector.PROTOCOL)
protocol.CASES = collector.CASES


def fake_copy(source, destination):
    Path(destination).write_bytes(b'host-only placeholder')


def run_case(fail):
    calls = []

    def invocation(output, role, case, round_no, name, events, mode,
                   executable, bench, loops, env, median, load_average,
                   waits, perf):
        calls.append((role, case, round_no, load_average))
        valid = not fail or len(calls) != 1
        return {'valid': valid, 'errors': [] if valid else ['host failure'],
                'mode': mode, 'role': role, 'case': case,
                'stem': f'{role}-{case}-{round_no}'}

    args = type('Args', (), {'repeat': collector.REPEAT,
                             'cases': collector.CASES, 'mode': 'stat'})()
    with tempfile.TemporaryDirectory(prefix='pmu-collector-host-') as name:
        output = Path(name)
        with mock.patch.object(collector, 'load', side_effect=lambda which, path:
                               protocol if which == 'sealed_perf_protocol' else object()), \
             mock.patch.object(collector, 'snapshot', return_value={'same': True}), \
             mock.patch.object(collector, 'record_check'), \
             mock.patch.object(collector.os, 'sched_getaffinity', return_value={15}), \
             mock.patch.object(collector.shutil, 'copy2', side_effect=fake_copy), \
             mock.patch.object(protocol, 'wait_for_quiet_load',
                               return_value=([], [0.1, 0.1, 0.1], False)), \
             mock.patch.object(protocol, 'make_invocation', side_effect=invocation), \
             mock.patch.object(subprocess, 'run', side_effect=AssertionError('native process')), \
             mock.patch.object(subprocess, 'Popen', side_effect=AssertionError('native process')):
            before, rows, after, error, postflight_error = collector.collect(args, output)
        assert before == after == {'same': True}
        assert len(rows) == (1 if fail else 18)
        assert bool(error) == fail and postflight_error is None
        assert len(calls) == len(rows)
        assert all(load_average == [0.1, 0.1, 0.1]
                   for _, _, _, load_average in calls)
        assert (output / 'identity-after.json').is_file()


run_case(False)
run_case(True)
print('collector host success/failure paths passed without native processes')
