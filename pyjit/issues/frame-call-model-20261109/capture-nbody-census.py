"""Capture existing handler counters for unchanged warmed nbody; never timing.

Run capture with a stats-build Python, CPU affinity 15 and a fresh
-X pystats_dir=ABSOLUTE_DIRECTORY. Capture outer loop counts 0, N and 2N
separately; summarize those three JSON files with the host Python.
"""

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys


def write_json(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def function_routes(module, capi):
    functions = []
    for name in ('advance', 'report_energy', 'offset_momentum', 'bench_nbody'):
        function = getattr(module, name)
        code = function.__code__
        words = capi.get_ir(code, True)
        functions.append(dict(
            name=name, filename=code.co_filename, firstline=code.co_firstlineno,
            nlocalsplus=len(capi.get_co_localskinds(code)), selected_words=words,
            selected_counts=dict(Counter(word[0] for word in words))))
    return functions


def capture(args):
    import os
    import _testinternalcapi as capi

    assert sorted(os.sched_getaffinity(0)) == [15]
    assert not sys._jit.is_enabled()
    assert sys.gettrace() is None and sys.getprofile() is None
    assert args.loops >= 0 and args.warmup > 0
    kernel = args.kernel.resolve()
    spec = importlib.util.spec_from_file_location('_nbody_census', kernel)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    # Setup runs once per run() call. Warm its helper too, before subtraction.
    for _ in range(args.warmup):
        module.run(1)
    functions = function_routes(module, capi)
    global_events = [sys.monitoring.get_events(tool) for tool in range(6)]
    local_events = {row['name']: [sys.monitoring.get_local_events(
        tool, getattr(module, row['name']).__code__) for tool in range(6)]
        for row in functions}
    assert not any(global_events), global_events
    assert not any(mask for masks in local_events.values() for mask in masks), local_events
    directory = args.stats_dir.resolve()
    assert sys._xoptions['pystats_dir'] == str(directory)
    directory.mkdir(parents=True, exist_ok=True)
    before = set(directory.glob('*.txt'))
    result = dict(git=sys._git, executable=sys.executable,
                  executable_sha256=hashlib.sha256(Path(sys.executable).read_bytes()).hexdigest(),
                  kernel=str(kernel), kernel_sha256=hashlib.sha256(kernel.read_bytes()).hexdigest(),
                  jit_enabled=sys._jit.is_enabled(), affinity=sorted(os.sched_getaffinity(0)),
                  loops=args.loops, warmup=args.warmup, functions=functions,
                  global_monitoring=global_events, local_monitoring=local_events)
    sys._stats_clear()
    sys._stats_on()
    module.run(args.loops)
    dumped = sys._stats_dump()
    sys._stats_off()
    sys._stats_clear()
    assert dumped
    files = set(directory.glob('*.txt')) - before
    assert len(files) == 1, files
    result['stats_file'] = str(files.pop())
    result['functions_after'] = function_routes(module, capi)
    write_json(args.output, result)


def read_stats(path):
    pattern = re.compile(r'^\s*(.+?)\s*:\s*(\d+)\s*$')
    return {match[1]: int(match[2]) for line in path.read_text().splitlines()
            if (match := pattern.match(line))}


def summarize(args):
    samples = sorted((json.loads(path.read_text()) for path in args.samples),
                     key=lambda row: row['loops'])
    empty, first, second = samples
    assert empty['loops'] == 0 < first['loops']
    assert second['loops'] == 2 * first['loops']
    for key in ('git', 'kernel_sha256', 'executable_sha256', 'warmup'):
        assert all(row[key] == first[key] for row in samples), key
    stats = [read_stats(Path(row['stats_file'])) for row in samples]
    keys = set().union(*(row.keys() for row in stats))
    entered = sorted(key for key in keys if key.endswith('.execution_count'))
    rows = []
    for key in (*entered, 'Calls to Python functions inlined', 'Frames pushed'):
        values = [row.get(key, 0) for row in stats]
        delta = values[1] - values[0]
        if delta or values[2] != values[0]:
            rows.append(dict(counter=key, empty=values[0],
                             per_outer_loop=delta / first['loops'],
                             linear_2x=values[2] - values[0] == 2 * delta))
    totals = [sum(row.get(key, 0) for key in entered) for row in stats]
    write_json(args.output, dict(
        git=first['git'], executable_sha256=first['executable_sha256'],
        kernel_sha256=first['kernel_sha256'],
        loops=[row['loops'] for row in samples],
        entered_handlers_per_outer_loop=(totals[1] - totals[0]) / first['loops'],
        entered_handlers_linear_2x=totals[2] - totals[0] == 2 * (totals[1] - totals[0]),
        counts=rows, selected_functions=first['functions'],
        routes_match_across_samples=all(row['functions'] == first['functions']
                                       for row in samples),
        routes_stable_during_capture=all(row['functions'] == row['functions_after']
                                        for row in samples),
        route_snapshots=[dict(loops=row['loops'], before=row['functions'],
                              after=row['functions_after']) for row in samples],
        limits='Global entered-handler counters with empty subtraction; static '
               'words establish possible sites, not per-site hits. One outer '
               'loop runs 20,000 advance steps and energy bookends. Stats changes '
               'code generation; these are not native instruction counts or timings.'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    native = commands.add_parser('capture')
    native.add_argument('--kernel', type=Path, required=True)
    native.add_argument('--loops', type=int, required=True)
    native.add_argument('--warmup', type=int, default=20)
    native.add_argument('--stats-dir', type=Path, required=True)
    native.add_argument('--output', type=Path, required=True)
    host = commands.add_parser('summarize')
    host.add_argument('samples', type=Path, nargs=3)
    host.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'capture':
        capture(args)
    else:
        summarize(args)


if __name__ == '__main__':
    main()
