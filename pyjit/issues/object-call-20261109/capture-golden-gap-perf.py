"""Compare golden/candidate using the existing FIFO PMU interval and validators."""

import argparse
import hashlib
import importlib.util
import os
from pathlib import Path
import sys


def main():
    packet = Path(__file__).resolve().parent
    previous = packet.parent / 'frame-call-model-20261109/capture-closing39-gap-perf.py'
    spec = importlib.util.spec_from_file_location('golden_checkpoint_perf', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--candidate-head', required=True)
    parser.add_argument('--repeat', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    sys.argv[1:] = remaining
    capture.ROLES = ('golden', 'candidate')
    capture.HEADS = {'golden': '4d666e1b482cb4983c7683f91c5922af14d0ab40',
                     'candidate': args.candidate_head}
    capture.REPEAT = args.repeat
    capture.CASES = ('coroutines', 'spectral_norm', 'comprehensions')

    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def snapshot(identity, checkpoint, env, retained, input_files):
        checkpoint.EXPECTED_MODULE_COUNTS['golden'] = 117
        checkpoint.CONTROL_HEADS['golden'] = capture.HEADS['golden']
        builds = {role: Path(identity['builds'][role]['python']).parent
                  for role in capture.ROLES}
        logs = {role: Path(identity['builds'][role]['module_log']['path'])
                for role in capture.ROLES}
        current = checkpoint.snapshot_all(builds, env, logs)
        capture.require(not current['errors'], current['errors'])
        capture.require(current['builds'] == identity['builds'],
                        'build identity differs from matched timing')
        executables = {}
        extensions = {}
        for role in capture.ROLES:
            expected = identity['builds'][role]
            capture.require(expected['git'] == {'head': capture.HEADS[role], 'status': ''},
                            f'{role} source head/cleanliness mismatch')
            executable = Path(expected['python'])
            capture.require(checkpoint.elf_build_id(retained[role]) ==
                            expected['python_build_id'], f'{role} retained build ID mismatch')
            capture.require(digest(executable) == digest(retained[role]),
                            f'{role} retained executable mismatch')
            executables[role] = digest(executable)
            extensions[role] = {str(path): digest(path) for path in
                                sorted(builds[role].glob('build/lib*/*.so'))}
        for key in ('CC', 'CFLAGS', 'OPT', 'Py_DEBUG', 'Py_TRACE_REFS',
                    'WITH_MIMALLOC', 'WITH_PYMALLOC'):
            capture.require(current['builds']['golden']['probe']['config'][key] ==
                            current['builds']['candidate']['probe']['config'][key],
                            f'build configuration differs: {key}')
        paths = [*input_files, Path(__file__).resolve()]
        return {'builds': current['builds'], 'executable_sha256': executables,
                'extensions_sha256': extensions,
                'inputs': {str(path.absolute()): digest(path) for path in paths},
                'affinity': sorted(os.sched_getaffinity(0))}

    capture.snapshot = snapshot
    capture.main()


if __name__ == '__main__':
    main()
