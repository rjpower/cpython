"""Steady exec/class mapping-path diagnostic; coordinator owns quiet execution.

Timer compiles the statement and setup before any sample. The exec payload is
also compiled before Timer construction; its environment and locals mapping
are reused within each sample. Class creation uses timeit's default GC policy:
automatic cyclic collection is disabled during each run and its prior state is
restored afterward. Class setup explicitly collects before each run, outside
the measured interval, to retire type cycles from the preceding sample.
Allocation and reference counting remain active. This is not a
startup measurement or an all29 workload.
"""

import argparse
import builtins
import gc
import json
import os
import sys
import timeit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', choices=('exec', 'class'))
    parser.add_argument('--iterations', type=int, default=20_000)
    args = parser.parse_args()
    if args.iterations <= 0:
        parser.error('--iterations must be positive')
    if sys._jit.is_enabled():
        parser.error('requires the JIT to be disabled')
    if os.environ.get('PYTHONHASHSEED') != '0' or sys.flags.hash_randomization:
        parser.error('requires PYTHONHASHSEED=0')

    namespace = {'__name__': '__mapping_path_probe__',
                 '__builtins__': builtins.__dict__}
    if args.case == 'exec':
        payload = 'answer=17'
        namespace['code'] = compile(payload, '<mapping-path-exec>', 'exec')
        statement = 'exec(code, environment, namespace)'
        setup = "environment = {'__builtins__': __builtins__}; namespace = {}"
    else:
        payload = None
        namespace['gc'] = gc
        statement = 'class C: pass'
        setup = 'gc.collect()'

    timer = timeit.Timer(statement, setup, globals=namespace)
    gc_enabled_before = gc.isenabled()
    warmup_seconds = timer.timeit(number=args.iterations)
    durations_seconds = timer.repeat(repeat=7, number=args.iterations)
    print(json.dumps({
        'kind': 'mapping-path-steady-diagnostic',
        'case': args.case,
        'iterations_per_sample': args.iterations,
        'warmup_samples': 1,
        'timed_samples': 7,
        'warmup_seconds': warmup_seconds,
        'durations_seconds': durations_seconds,
        'timer': 'time.perf_counter',
        'statement': statement,
        'setup': setup,
        'exec_payload': payload,
        'gc_policy': 'timeit default: cyclic GC disabled during each run; prior state restored',
        'class_setup_collects_before_each_run': args.case == 'class',
        'gc_enabled_before': gc_enabled_before,
        'gc_enabled_after': gc.isenabled(),
        'executable': sys.executable,
        'version': sys.version,
        'git': list(sys._git),
        'jit_enabled': sys._jit.is_enabled(),
        'python_jit_env': os.environ.get('PYTHON_JIT'),
        'python_hash_seed': os.environ.get('PYTHONHASHSEED'),
        'hash_randomization': sys.flags.hash_randomization,
    }))


if __name__ == '__main__':
    main()
