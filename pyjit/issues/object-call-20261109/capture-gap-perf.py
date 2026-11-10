"""Reuse the reviewed FIFO PMU protocol for the object/call checkpoint."""

import argparse
import importlib.util
from pathlib import Path
import sys


def main():
    packet = Path(__file__).resolve().parent
    previous = packet.parent / 'frame-call-model-20261109/capture-closing39-gap-perf.py'
    spec = importlib.util.spec_from_file_location('checkpoint_perf', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--candidate-head', default='0a35a382909ca169ce3f381cb38206b1725001b3')
    args, remaining = parser.parse_known_args()
    capture.HEADS['candidate'] = args.candidate_head
    sys.argv[1:] = remaining
    capture.CASES = ('deltablue', 'generators', 'nbody', 'logging_silent', 'coroutines')
    capture.REPEAT = capture.BUILD / 'pyjit-link-wave1/object-call-20261109/measure41-all29-upstream'
    previous_snapshot = capture.snapshot

    def snapshot(identity, checkpoint, env, retained, input_files):
        return previous_snapshot(identity, checkpoint, env, retained,
                                 [*input_files, Path(__file__).resolve()])

    capture.snapshot = snapshot
    capture.main()


if __name__ == '__main__':
    main()
