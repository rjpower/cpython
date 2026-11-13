"""Profile an explicit model55 checkpoint after its complete upstream campaign.

Reuse the sealed interval protocol and all29 admission checks. The caller must
name the complete timing campaign and exact candidate revision; this wrapper
does not infer either from whichever build happens to be present.
"""

import argparse
import importlib.util
from pathlib import Path
import re
import sys


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--candidate-head', required=True)
    parser.add_argument('--repeat', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    if re.fullmatch(r'[0-9a-f]{40}', args.candidate_head) is None:
        parser.error('--candidate-head requires a full hexadecimal commit ID')

    previous = (Path(__file__).resolve().parent.parent /
                'frame-call-model-20261109/capture-closing39-gap-perf.py')
    spec = importlib.util.spec_from_file_location('model55_checkpoint_capture', previous)
    capture = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = capture
    spec.loader.exec_module(capture)
    capture.HEADS['candidate'] = args.candidate_head
    capture.EXTENSION_COUNTS['candidate'] = 79
    capture.CASES = ('deltablue', 'generators', 'coroutines', 'richards',
                     'comprehensions', 'logging_silent')
    capture.REPEAT = args.repeat.resolve()
    previous_snapshot = capture.snapshot

    def snapshot(identity, checkpoint, env, retained, input_files):
        return previous_snapshot(identity, checkpoint, env, retained,
                                 [*input_files, Path(__file__).resolve()])

    capture.snapshot = snapshot
    sys.argv = [sys.argv[0], *remaining]
    capture.main()


if __name__ == '__main__':
    main()
