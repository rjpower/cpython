"""Capture the selected checkpoint with the reviewed post-warmup PMU protocol."""

import importlib.util
from pathlib import Path
import sys


def main():
    packet = Path(__file__).resolve().parent
    previous = packet.parent / 'frame-call-model-20261109/capture-closing39-gap-perf.py'
    spec = importlib.util.spec_from_file_location('round51_perf', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    capture.HEADS['candidate'] = '0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254'
    capture.EXTENSION_COUNTS['candidate'] = 79
    capture.CASES = ('deltablue', 'generators', 'coroutines', 'logging_silent')
    capture.REPEAT = capture.BUILD / 'pyjit-link-wave1/interpreter-round51/final51-all29-upstream'
    previous_snapshot = capture.snapshot

    def snapshot(identity, checkpoint, env, retained, input_files):
        return previous_snapshot(identity, checkpoint, env, retained,
                                 [*input_files, Path(__file__).resolve()])

    capture.snapshot = snapshot
    capture.main()


if __name__ == '__main__':
    main()
