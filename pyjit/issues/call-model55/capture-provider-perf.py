"""Profile the frozen provider checkpoint with the sealed interval protocol."""

import importlib.util
from pathlib import Path
import sys


def main():
    previous = (Path(__file__).resolve().parent.parent /
                'frame-call-model-20261109/capture-closing39-gap-perf.py')
    spec = importlib.util.spec_from_file_location('model55_provider_capture', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    capture.HEADS['candidate'] = 'de95475accbf90e8abc305177b5c6442eae4daf7'
    capture.EXTENSION_COUNTS['candidate'] = 79
    capture.CASES = ('deltablue', 'generators', 'coroutines', 'logging_silent')
    capture.REPEAT = (capture.BUILD /
        'pyjit-link-wave1/call-model55/m3-provider-all29-upstream')
    previous_snapshot = capture.snapshot

    def snapshot(identity, checkpoint, env, retained, input_files):
        return previous_snapshot(identity, checkpoint, env, retained,
                                 [*input_files, Path(__file__).resolve()])

    capture.snapshot = snapshot
    capture.main()


if __name__ == '__main__':
    main()
