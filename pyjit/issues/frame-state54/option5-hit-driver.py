"""Untimed unchanged-kernel census; run only under option5-hits.gdb."""

import importlib.util
import json
import os
from pathlib import Path
import signal
import sys


assert not sys._jit.is_enabled()
assert not sys.flags.hash_randomization
path = Path(sys.argv[1]).resolve()
variant = sys.argv[2] if len(sys.argv) > 2 else None
spec = importlib.util.spec_from_file_location('_frame54_kernel', path)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
setup = vars(module).get('setup')
if setup is not None:
    setup(variant)
module.run(1)
# GDB swallows both signals. Count only the next unchanged run(1), excluding
# warmup, JSON reporting and process shutdown.
os.kill(os.getpid(), signal.SIGUSR1)
module.run(1)
os.kill(os.getpid(), signal.SIGUSR1)
print(json.dumps({'kind': 'diagnostic-not-timing', 'kernel': str(path),
                  'variant': variant, 'loops': 1, 'warmup_loops': 1,
                  'git': list(sys._git),
                  'hash_randomization': sys.flags.hash_randomization}))
