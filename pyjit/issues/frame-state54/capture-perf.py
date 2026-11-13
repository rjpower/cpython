"""Capture selected frame54 activation profiles with the sealed protocol."""

import importlib.util
from pathlib import Path
import sys


def main():
    previous = Path(__file__).resolve().parent.parent / 'frame-call-model-20261109/capture-closing39-gap-perf.py'
    spec = importlib.util.spec_from_file_location('frame54_capture', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    capture.HEADS['candidate'] = '61015a9452d9069646fb5fede9ff963896833b89'
    capture.EXTENSION_COUNTS['candidate'] = 79
    capture.CASES = ('generators', 'coroutines', 'deltablue', 'logging_silent')
    capture.REPEAT = capture.BUILD / 'pyjit-link-wave1/frame-state54/final54-all29-upstream'
    previous_snapshot = capture.snapshot

    def snapshot(identity, checkpoint, env, retained, input_files):
        return previous_snapshot(identity, checkpoint, env, retained,
                                 [*input_files, Path(__file__).resolve()])

    capture.snapshot = snapshot
    capture.main()


if __name__ == '__main__':
    main()
