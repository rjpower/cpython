"""Capture the bounded entry revision with the established post-warmup protocol."""

import importlib.util
from pathlib import Path
import sys


def main():
    packet = Path(__file__).resolve().parent
    previous = packet.parent / 'frame-call-model-20261109/capture-closing39-gap-perf.py'
    spec = importlib.util.spec_from_file_location('delegation52_entry_perf', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    capture.HEADS['candidate'] = 'e85171923c08b0d68ee169119e761d7583bbcb26'
    capture.EXTENSION_COUNTS['candidate'] = 79
    capture.CASES = ('generators', 'coroutines', 'deltablue', 'logging_silent')
    capture.REPEAT = capture.BUILD / 'pyjit-link-wave1/generator-delegation52/entry52-all29-upstream'
    previous_snapshot = capture.snapshot

    def snapshot(identity, checkpoint, env, retained, input_files):
        return previous_snapshot(identity, checkpoint, env, retained,
                                 [*input_files, Path(__file__).resolve()])

    capture.snapshot = snapshot
    capture.main()


if __name__ == '__main__':
    main()
