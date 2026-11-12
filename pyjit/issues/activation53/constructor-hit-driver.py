"""Unchanged-kernel constructor census driver; run only under constructor-hits.gdb."""

import importlib.util
import json
import os
from pathlib import Path
import signal
import sys


path = Path(sys.argv[1]).resolve()
spec = importlib.util.spec_from_file_location('_activation_kernel', path)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
setup = vars(module).get('setup')
if setup is not None:
    setup(None)
module.run(1)
# The debugger enables its native breakpoint after warmup, then swallows this signal.
os.kill(os.getpid(), signal.SIGUSR1)
module.run(1)
print(json.dumps({'kernel': str(path), 'loops': 1, 'warmup_loops': 1,
                  'git': list(sys._git), 'hash_randomization': sys.flags.hash_randomization}))
